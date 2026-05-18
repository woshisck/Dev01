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
#include "GameModes/YogGameMode.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UI/YogEntryMenuWidget.h"
#include "UI/YogGameOverWidget.h"
#include "UI/YogUIManagerSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{
	bShouldLoadSaveAfterMap = false;
	MainGameMap = FSoftObjectPath(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom.InitialRoom"));
	FrontendMap = FSoftObjectPath(TEXT("/Game/Maps/L_EntryMenu.L_EntryMenu"));
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

	GetFrontendMainMenuBackgroundBrush();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UYogUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Cannot show entry menu: UIManager is missing."));
		return;
	}

	TSubclassOf<UYogEntryMenuWidget> WidgetClass = EntryMenuClass;
	if (!WidgetClass)
	{
		WidgetClass = UYogEntryMenuWidget::StaticClass();
	}
	UIManager->SetWidgetClassOverride(EYogUIScreenId::EntryMenu, WidgetClass);
	FYogUIScreenInputPolicy Policy;
	Policy.bShowMouseCursor = true;
	Policy.bPauseGame = false;
	Policy.bDisablePawnInput = false;
	Policy.bAffectsMajorUI = false;
	UIManager->SetInputPolicyOverride(EYogUIScreenId::EntryMenu, Policy);
	EntryMenuWidget = Cast<UYogEntryMenuWidget>(UIManager->EnsureWidget(EYogUIScreenId::EntryMenu));
	if (!EntryMenuWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Cannot show entry menu: EntryMenu class is invalid."));
		return;
	}

	EntryMenuWidget->SetBackgroundTexture(FrontendMainMenuTexture);
	EntryMenuWidget->OnStartRequested.RemoveAll(this);
	EntryMenuWidget->OnContinueRequested.RemoveAll(this);
	EntryMenuWidget->OnOptionsRequested.RemoveAll(this);
	EntryMenuWidget->OnQuitRequested.RemoveAll(this);
	EntryMenuWidget->OnStartRequested.AddDynamic(this, &UYogGameInstanceBase::StartNewRunFromFrontend);
	EntryMenuWidget->OnContinueRequested.AddDynamic(this, &UYogGameInstanceBase::StartNewRunFromFrontend);
	EntryMenuWidget->OnOptionsRequested.AddDynamic(this, &UYogGameInstanceBase::HandleEntryOptionsRequested);
	EntryMenuWidget->OnQuitRequested.AddDynamic(this, &UYogGameInstanceBase::QuitFromFrontend);
	UIManager->PushScreen(EYogUIScreenId::EntryMenu);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UYogGameInstanceBase::RefocusEntryMenuWidget);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("AutoStart")))
	{
		StartNewRunFromFrontend();
	}
}

const FSlateBrush* UYogGameInstanceBase::GetFrontendMainMenuBackgroundBrush()
{
	if (!FrontendMainMenuTexture)
	{
		static const TCHAR* CandidateTexturePaths[] =
		{
			TEXT("/Game/UI/Playtest_UI/UI_Tex/Frontend/T_MainMenu_GothicDungeon_20260509.T_MainMenu_GothicDungeon_20260509"),
			TEXT("/Game/UI/Playtest_UI/UI_Tex/Frontend/T_MainMenu_Dungeon.T_MainMenu_Dungeon")
		};
		for (const TCHAR* TexturePath : CandidateTexturePaths)
		{
			FrontendMainMenuTexture = Cast<UTexture2D>(StaticLoadObject(
				UTexture2D::StaticClass(),
				nullptr,
				TexturePath));
			if (FrontendMainMenuTexture)
			{
				break;
			}
		}
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

	RemoveFrontendWidget();
	ClearRunState();
	BeginLoadMainGameMap();
}

void UYogGameInstanceBase::HandleEntryOptionsRequested()
{
	HandleOptionsClicked();
}

void UYogGameInstanceBase::QuitFromFrontend()
{
	HandleQuitClicked();
}

void UYogGameInstanceBase::BeginLoadMainGameMap()
{
	const FString PackageName = MainGameMap.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Could not resolve map package from %s"), *MainGameMap.ToString());
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

	UE_LOG(LogTemp, Log, TEXT("[Frontend] Opening gameplay map %s."), *PackageName);
	UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
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

	const FSlateBrush* BackgroundBrush = GetFrontendMainMenuBackgroundBrush();
	TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.012f, 0.016f, 1.f))
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
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.58f))
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
	ApplyFrontendInputMode(true, FrontendWidget);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UYogGameInstanceBase::RefocusFrontendWidget);
	}
}

void UYogGameInstanceBase::ShowGameOverScreen(bool bCanRevive)
{
	RemoveFrontendWidget();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UYogUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameOver] Cannot show game over screen: UIManager is missing."));
		return;
	}

	TSubclassOf<UYogGameOverWidget> WidgetClass = GameOverWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UYogGameOverWidget::StaticClass();
	}
	UIManager->SetWidgetClassOverride(EYogUIScreenId::GameOver, WidgetClass);

	FYogUIScreenInputPolicy Policy;
	Policy.bShowMouseCursor = true;
	Policy.bPauseGame = true;
	Policy.bDisablePawnInput = true;
	Policy.bAffectsMajorUI = false;
	UIManager->SetInputPolicyOverride(EYogUIScreenId::GameOver, Policy);

	UYogGameOverWidget* GameOverWidget = Cast<UYogGameOverWidget>(UIManager->EnsureWidget(EYogUIScreenId::GameOver));
	if (!GameOverWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameOver] Cannot show game over screen: widget class is invalid."));
		return;
	}

	GameOverWidget->Setup(bCanRevive);
	UIManager->PushScreen(EYogUIScreenId::GameOver);
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<UYogGameOverWidget> WeakWidget(GameOverWidget);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakWidget]()
		{
			if (UYogGameOverWidget* Widget = WeakWidget.Get())
			{
				Widget->SetKeyboardFocus();
			}
		}));
	}
}

void UYogGameInstanceBase::ReturnToMainMenu()
{
	ClearRunState();
	ShowLoadingScreen(
		NSLOCTEXT("DevKitFrontend", "ReturningTitle", "Returning"),
		NSLOCTEXT("DevKitFrontend", "ReturningSubtitle", "Opening title screen"));
	const FString PackageName = FrontendMap.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Could not resolve frontend map package from %s"), *FrontendMap.ToString());
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
}

void UYogGameInstanceBase::RemoveFrontendWidget()
{
	if (EntryMenuWidget)
	{
		if (ULocalPlayer* LocalPlayer = EntryMenuWidget->GetOwningLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				UIManager->PopScreen(EYogUIScreenId::EntryMenu);
			}
		}
		EntryMenuWidget->OnStartRequested.RemoveAll(this);
		EntryMenuWidget->OnContinueRequested.RemoveAll(this);
		EntryMenuWidget->OnOptionsRequested.RemoveAll(this);
		EntryMenuWidget->OnQuitRequested.RemoveAll(this);
		EntryMenuWidget = nullptr;
	}

	if (FrontendWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(FrontendWidget.ToSharedRef());
	}
	FrontendWidget.Reset();
}

void UYogGameInstanceBase::ApplyFrontendInputMode(bool bUIOnly, TSharedPtr<SWidget> WidgetToFocus)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	PC->SetShowMouseCursor(bUIOnly);
	if (bUIOnly)
	{
		// GameAndUI (not UIOnly): UIOnly killed D-pad navigation by blocking game-input routing.
		FInputModeGameAndUI Mode;
		if (WidgetToFocus.IsValid())
		{
			Mode.SetWidgetToFocus(WidgetToFocus);
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		if (WidgetToFocus.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(WidgetToFocus, EFocusCause::SetDirectly);
			FSlateApplication::Get().SetAllUserFocus(WidgetToFocus, EFocusCause::SetDirectly);
		}
	}
	else
	{
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->EnableInput(PC);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void UYogGameInstanceBase::RefocusFrontendWidget()
{
	ApplyFrontendInputMode(FrontendWidget.IsValid(), FrontendWidget);
}

void UYogGameInstanceBase::RefocusEntryMenuWidget()
{
	if (EntryMenuWidget)
	{
		EntryMenuWidget->SetKeyboardFocus();
	}
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
