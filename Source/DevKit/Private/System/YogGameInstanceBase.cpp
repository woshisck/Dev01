// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/YogGameInstanceBase.h"
#include "Character/PlayerCharacterBase.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"



UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{
	bShouldLoadSaveAfterMap = false;
	MainGameMap = FSoftObjectPath(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom.InitialRoom"));
}

APlayerCharacterBase* UYogGameInstanceBase::GetPlayerCharacter()
{
	UWorld* World = this->GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			return Cast<APlayerCharacterBase>(PlayerController->GetPawn());
		}
	}

	return nullptr;
}

////////////// AI //////////////
void UYogGameInstanceBase::AddStep(const FTimerDelegate& InDelegate, float Interval)
{
	FSequenceStep Step;
	Step.Delegate = InDelegate;
	Step.Interval = Interval;
	Steps.Add(MoveTemp(Step));


}

void UYogGameInstanceBase::StartSequence()
{
	if (Steps.Num() == 0) return;
	CurrentIndex = 0;
	PlayCurrentStep();
}

void UYogGameInstanceBase::ForceNext()
{
	// Immediately cancel current wait and advance
	GetWorld()->GetTimerManager().ClearTimer(StepTimerHandle);
	AdvanceToNextStep();
}

void UYogGameInstanceBase::PlayCurrentStep()
{
	if (!GetWorld() || !Steps.IsValidIndex(CurrentIndex)) return;

	UE_LOG(LogTemp, Warning, TEXT("PlayCurrentStep"));


	// Schedule the timer to call the internal callback after the step's interval
	float Delay = Steps[CurrentIndex].Interval;
	GetWorld()->GetTimerManager().SetTimer(StepTimerHandle, this, &UYogGameInstanceBase::AdvanceToNextStep, Delay, false);

}

void UYogGameInstanceBase::AdvanceToNextStep()
{
	if (!Steps.IsValidIndex(CurrentIndex)) return;

	// Execute the bound delegate for this step
	Steps[CurrentIndex].Delegate.ExecuteIfBound();

	// Move to next
	++CurrentIndex;
	if (Steps.IsValidIndex(CurrentIndex))
	{
		PlayCurrentStep();
	}
	else
	{
		// Sequence finished; optionally loop or clear
		GetWorld()->GetTimerManager().ClearTimer(StepTimerHandle);
	}
}


////////////// AI //////////////







/* ASAP: SAMPLE USE -- */

//void AMyActor::BeginPlay()
//{
//	Super::BeginPlay();
//
//	// Fill function array
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepOne));
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepTwo));
//	FunctionArray.Add(FMyStepDelegate::CreateUObject(this, &AMyActor::StepThree));
//
//	StartSequence();
//}
//
//void AMyActor::StepOne() { UE_LOG(LogTemp, Warning, TEXT("Step One")); }
//void AMyActor::StepTwo() { UE_LOG(LogTemp, Warning, TEXT("Step Two")); }
//void AMyActor::StepThree() { UE_LOG(LogTemp, Warning, TEXT("Step Three")); }






void UYogGameInstanceBase::Init()
{
	Super::Init();
	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::Init()"));

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UYogGameInstanceBase::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYogGameInstanceBase::OnPostLoadMap);

}

void UYogGameInstanceBase::Shutdown()
{
	if (FrontendMapLoadHandle.IsValid())
	{
		FrontendMapLoadHandle->CancelHandle();
		FrontendMapLoadHandle.Reset();
	}
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Shutdown();
}

void UYogGameInstanceBase::AddCurrentMapKill(int32 adder)
{

	CurrentMapKills += adder;

	if (CurrentMapKills >= TargetMapKills)
	{
		OnFinishLevel.Broadcast();
	}
}

void UYogGameInstanceBase::OpenMapAndLoadSave(const TSoftObjectPtr<UWorld> Level)
{
	// Set the pending save slot and flag
	PendingSaveSlot = SaveSlot;
	bShouldLoadSaveAfterMap = true;

	// Bind the delegate to know when the map is loaded

	// Open the map
	//void UGameplayStatics::OpenLevelBySoftObjectPtr(const UObject * WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute, FString Options)
	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::OpenMapAndLoadSave"));
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level, true);
}

void UYogGameInstanceBase::OnPostLoadMap(UWorld* World)
{
	if (bFrontendLoadingGameplayMap)
	{
		bFrontendMapLoaded = true;
		FinishFrontendLoadingIfReady();
	}
	else if (IsFrontendStartupWorld(World))
	{
		ShowMainMenu();
	}

	// If we are supposed to load a save after the map, do it
	if (bShouldLoadSaveAfterMap && World)
	{

		UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::OnPostLoadMap"));

		UYogSaveSubsystem* SaveSubsystem = this->GetSubsystem<UYogSaveSubsystem>();

		if (SaveSubsystem->CurrentSaveGame)
		{
			SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
		}

		// Reset the flag
		bShouldLoadSaveAfterMap = false;

	}
}

void UYogGameInstanceBase::OnPreLoadMap(const FString& MapName)
{
	if (bFrontendLoadingGameplayMap)
	{
		ShowLoadingScreen(
			NSLOCTEXT("DevKitFrontend", "LoadingTitle", "Loading"),
			FText::FromString(MapName));
	}
}

void UYogGameInstanceBase::ShowMainMenu()
{
	if (FrontendMapLoadHandle.IsValid() && FrontendMapLoadHandle->IsActive())
	{
		FrontendMapLoadHandle->CancelHandle();
	}
	FrontendMapLoadHandle.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FrontendLoadingTimerHandle);
	}

	RemoveFrontendWidget();
	bFrontendLoadingGameplayMap = false;
	bFrontendMapLoaded = false;
	bFrontendMinLoadTimeElapsed = false;

	const FSlateBrush* BackgroundBrush = GetFrontendMainMenuBackgroundBrush();
	const FSlateFontInfo MenuFont = FCoreStyle::GetDefaultFontStyle("Regular", 26);
	const FSlateColor MenuTextColor(FLinearColor(0.80f, 0.82f, 0.84f, 1.f));
	auto BuildMenuText = [&](const FText& Label)
	{
		return SNew(STextBlock)
			.Text(Label)
			.Font(MenuFont)
			.ColorAndOpacity(MenuTextColor);
	};

	TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.018f, 0.015f, 1.f))
			.Padding(0.f)
		]
		+ SOverlay::Slot()
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage)
				.Image(BackgroundBrush)
				.ColorAndOpacity(FLinearColor::White)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.16f))
			.Padding(0.f)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(96.f, 0.f, 0.f, 132.f))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.005f, 0.007f, 0.009f, 0.58f))
			.Padding(FMargin(24.f, 18.f))
			[
				SNew(SBox)
				.WidthOverride(260.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SAssignNew(FrontendStartButton, SButton)
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(20.f, 8.f))
						.ButtonColorAndOpacity(FLinearColor(0.04f, 0.08f, 0.12f, 0.42f))
						.OnClicked_UObject(this, &UYogGameInstanceBase::HandleStartClicked)
						[
							BuildMenuText(NSLOCTEXT("DevKitFrontend", "StartGame", "Start"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(20.f, 8.f))
						.ButtonColorAndOpacity(FLinearColor(0.04f, 0.08f, 0.12f, 0.34f))
						.OnClicked_UObject(this, &UYogGameInstanceBase::HandleContinueClicked)
						[
							BuildMenuText(NSLOCTEXT("DevKitFrontend", "ContinueGame", "Continue"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(20.f, 8.f))
						.ButtonColorAndOpacity(FLinearColor(0.04f, 0.08f, 0.12f, 0.34f))
						.OnClicked_UObject(this, &UYogGameInstanceBase::HandleOptionsClicked)
						[
							BuildMenuText(NSLOCTEXT("DevKitFrontend", "Options", "Options"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SButton)
						.HAlign(HAlign_Left)
						.ContentPadding(FMargin(20.f, 8.f))
						.ButtonColorAndOpacity(FLinearColor(0.04f, 0.08f, 0.12f, 0.34f))
						.OnClicked_UObject(this, &UYogGameInstanceBase::HandleQuitClicked)
						[
							BuildMenuText(NSLOCTEXT("DevKitFrontend", "QuitGame", "Quit"))
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(34.f, 0.f, 0.f, 28.f))
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.01f, 0.015f, 0.52f))
			.Padding(FMargin(14.f, 7.f))
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DevKitFrontend", "MainMenuInputPrompt", "A Select    D-Pad / Left Stick Navigate"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 15))
				.ColorAndOpacity(FLinearColor(0.68f, 0.78f, 0.86f, 0.92f))
			]
		];

	FrontendWidget = Widget;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Widget, 10000);
	}
	ApplyFrontendInputMode(true, FrontendStartButton);

	if (FParse::Param(FCommandLine::Get(), TEXT("AutoStart")))
	{
		StartNewRunFromFrontend();
	}
}

const FSlateBrush* UYogGameInstanceBase::GetFrontendMainMenuBackgroundBrush()
{
	if (!FrontendMainMenuTexture)
	{
		FrontendMainMenuTexture = Cast<UTexture2D>(StaticLoadObject(
			UTexture2D::StaticClass(),
			nullptr,
			TEXT("/Game/UI/Playtest_UI/UI_Tex/Frontend/T_MainMenu_Dungeon.T_MainMenu_Dungeon")));
	}

	if (!FrontendMainMenuTexture)
	{
		return nullptr;
	}

	if (!FrontendMainMenuBrush.IsValid())
	{
		FrontendMainMenuBrush = MakeShared<FSlateBrush>();
		FrontendMainMenuBrush->SetResourceObject(FrontendMainMenuTexture);
		FrontendMainMenuBrush->ImageSize = FVector2D(FrontendMainMenuTexture->GetSizeX(), FrontendMainMenuTexture->GetSizeY());
		FrontendMainMenuBrush->DrawAs = ESlateBrushDrawType::Image;
	}

	return FrontendMainMenuBrush.Get();
}

void UYogGameInstanceBase::StartNewRunFromFrontend()
{
	if (bFrontendLoadingGameplayMap)
	{
		UE_LOG(LogTemp, Log, TEXT("[Frontend] Start ignored because the gameplay map is already loading."));
		return;
	}

	ClearRunState();
	BeginLoadMainGameMap();
}

void UYogGameInstanceBase::BeginLoadMainGameMap()
{
	if (!MainGameMap.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] MainGameMap is invalid."));
		return;
	}

	ShowLoadingScreen(
		NSLOCTEXT("DevKitFrontend", "PreparingTitle", "Preparing"),
		NSLOCTEXT("DevKitFrontend", "PreparingSubtitle", "Loading the first level"));

	bFrontendLoadingGameplayMap = true;
	bFrontendMapLoaded = false;
	bFrontendMinLoadTimeElapsed = MinimumLoadingScreenTime <= 0.f;
	FrontendLoadingStartedSeconds = FPlatformTime::Seconds();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FrontendLoadingTimerHandle);
	}

	FrontendMapLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		MainGameMap,
		FStreamableDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleMainGameMapPreloaded),
		FStreamableManager::AsyncLoadHighPriority);
}

void UYogGameInstanceBase::HandleMainGameMapPreloaded()
{
	const FString PackageName = MainGameMap.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Could not resolve map package from %s"), *MainGameMap.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Frontend] Preloaded %s, opening level."), *PackageName);
	UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
}

void UYogGameInstanceBase::HandleMinimumLoadingScreenTimeElapsed()
{
	bFrontendMinLoadTimeElapsed = true;
	FinishFrontendLoadingIfReady();
}

void UYogGameInstanceBase::FinishFrontendLoadingIfReady()
{
	if (!bFrontendLoadingGameplayMap || !bFrontendMapLoaded)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float RemainingLoadScreenTime = FMath::Max(
			0.f,
			MinimumLoadingScreenTime - static_cast<float>(FPlatformTime::Seconds() - FrontendLoadingStartedSeconds));
		if (!bFrontendMinLoadTimeElapsed && RemainingLoadScreenTime > 0.f)
		{
			if (!World->GetTimerManager().IsTimerActive(FrontendLoadingTimerHandle))
			{
				World->GetTimerManager().SetTimer(
					FrontendLoadingTimerHandle,
					FTimerDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleMinimumLoadingScreenTimeElapsed),
					RemainingLoadScreenTime,
					false);
			}
			return;
		}

		World->GetTimerManager().ClearTimer(FrontendLoadingTimerHandle);
	}

	bFrontendMinLoadTimeElapsed = true;
	bFrontendLoadingGameplayMap = false;
	bFrontendMapLoaded = false;
	FrontendMapLoadHandle.Reset();
	UE_LOG(LogTemp, Log, TEXT("[Frontend] Gameplay map ready; loading screen removed."));
	RemoveFrontendWidget();
	ApplyFrontendInputMode(false);
}

void UYogGameInstanceBase::ShowLoadingScreen(const FText& Title, const FText& Subtitle)
{
	RemoveFrontendWidget();

	TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.012f, 0.016f, 1.f))
			.Padding(0.f)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 14.f)
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 38))
				.ColorAndOpacity(FLinearColor(0.9f, 0.88f, 0.78f, 1.f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Subtitle)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				.ColorAndOpacity(FLinearColor(0.66f, 0.68f, 0.72f, 1.f))
			]
		];

	FrontendWidget = Widget;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Widget, 10000);
	}
	ApplyFrontendInputMode(true);
}

void UYogGameInstanceBase::ShowGameOverScreen()
{
	RemoveFrontendWidget();

	TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.82f))
			.Padding(0.f)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(560.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 0.f, 0.f, 26.f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("DevKitFrontend", "GameOver", "Game Over"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 50))
					.ColorAndOpacity(FLinearColor(0.92f, 0.3f, 0.24f, 1.f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FMargin(28.f, 12.f))
					.OnClicked_UObject(this, &UYogGameInstanceBase::HandleRetryClicked)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("DevKitFrontend", "Retry", "Try Again"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.ContentPadding(FMargin(28.f, 12.f))
					.OnClicked_UObject(this, &UYogGameInstanceBase::HandleReturnToMenuClicked)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("DevKitFrontend", "ReturnToTitle", "Return to Title"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
					]
				]
			]
		];

	FrontendWidget = Widget;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Widget, 10000);
	}
	ApplyFrontendInputMode(true);
}

void UYogGameInstanceBase::ReturnToMainMenu()
{
	ClearRunState();
	ShowLoadingScreen(
		NSLOCTEXT("DevKitFrontend", "ReturningTitle", "Returning"),
		NSLOCTEXT("DevKitFrontend", "ReturningSubtitle", "Opening title screen"));
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Engine/Maps/Entry")), true);
}

void UYogGameInstanceBase::RemoveFrontendWidget()
{
	if (FrontendWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(FrontendWidget.ToSharedRef());
	}
	FrontendWidget.Reset();
	FrontendStartButton.Reset();
}

void UYogGameInstanceBase::ApplyFrontendInputMode(bool bUIOnly, TSharedPtr<SWidget> WidgetToFocus)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	PC->SetShowMouseCursor(bUIOnly);
	if (bUIOnly)
	{
		FInputModeUIOnly Mode;
		if (WidgetToFocus.IsValid())
		{
			Mode.SetWidgetToFocus(WidgetToFocus);
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		if (WidgetToFocus.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(WidgetToFocus, EFocusCause::SetDirectly);
		}
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

bool UYogGameInstanceBase::IsFrontendStartupWorld(const UWorld* World) const
{
	if (!World) return false;

	const FString PackageName = World->GetOutermost()->GetName();
	return PackageName.Equals(TEXT("/Engine/Maps/Entry"), ESearchCase::IgnoreCase);
}

FReply UYogGameInstanceBase::HandleStartClicked()
{
	StartNewRunFromFrontend();
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleContinueClicked()
{
	StartNewRunFromFrontend();
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleOptionsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[Frontend] Options clicked; settings screen is not implemented yet."));
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleRetryClicked()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	StartNewRunFromFrontend();
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleReturnToMenuClicked()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	ReturnToMainMenu();
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleQuitClicked()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	return FReply::Handled();
}

void UYogGameInstanceBase::SaveGame()
{
	if (!PersistentSaveData)
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
	APlayerCharacterBase* CurrentCharacter = GetPlayerCharacter();
	if (!CurrentCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveGame skipped: no player character is currently possessed."));
		return;
	}
	PersistentSaveData->SavedCharacterClass = CurrentCharacter->GetClass();

	UGameplayStatics::SaveGameToSlot(PersistentSaveData, "Slot1", 0);
}

UYogSaveGame* UYogGameInstanceBase::GetCurrentSaveGame()
{
	return CurrentSaveGame;
}

void UYogGameInstanceBase::SetSavingEnabled(bool bEnabled)
{
	bSavingEnabled = bEnabled;
}

bool UYogGameInstanceBase::LoadOrCreateSaveGame()
{
	return false;
}

bool UYogGameInstanceBase::HandleSaveGameLoaded(USaveGame* SaveGameObject)
{
	bool bLoaded = false;
	return bLoaded;
}

void UYogGameInstanceBase::GetSaveSlotInfo(FString& SlotName, int32& UserIndex) const
{

}

bool UYogGameInstanceBase::WriteSaveGame()
{
	if (bSavingEnabled)
	{
		USaveGame* currentSaveGame = Cast<USaveGame>(GetCurrentSaveGame());
		if (!currentSaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("WriteSaveGame skipped: CurrentSaveGame is null."));
			return false;
		}

		if (bCurrentlySaving)
		{
			bPendingSaveRequested = true;
			return true;
		}
		bCurrentlySaving = true;
		UGameplayStatics::AsyncSaveGameToSlot(currentSaveGame, SaveSlot, SaveUserIndex, FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleAsyncSave));
		return true;
	}
	else 
	{
		return false;
	}
}

void UYogGameInstanceBase::SpawnMobInMap()
{

	UE_LOG(LogTemp, Warning, TEXT("UYogGameInstanceBase::SpawnMobInMap()"));

}

void UYogGameInstanceBase::ClearRunState()
{
	PendingRunState = FRunState(); // 重置为默认值，bIsValid = false

	// v3：传送门跨关字段同点清，避免下一局误读上一局的预骰数据
	PendingRoomData = nullptr;
	PendingRoomBuffs.Reset();
	PendingNextFloor = 1;
	bPlayLevelIntroFadeIn = false;

	UE_LOG(LogTemp, Log, TEXT("ClearRunState: 跑局状态已清空（含传送门预骰）"));
}





void UYogGameInstanceBase::HandleAsyncSave(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	ensure(bCurrentlySaving);
	bCurrentlySaving = false;

	if (bPendingSaveRequested)
	{

		bPendingSaveRequested = false;
		WriteSaveGame();
	}

}
