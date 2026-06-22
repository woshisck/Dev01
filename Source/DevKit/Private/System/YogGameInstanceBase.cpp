// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/YogGameInstanceBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Data/CampaignDataAsset.h"
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
#include "Map/Portal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/SlateBrush.h"
#include "TimerManager.h"
#include "UObject/Stack.h"
#include "HAL/PlatformStackWalk.h"
#include "Misc/AssertionMacros.h"
#include "Misc/PackageName.h"
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

namespace
{
FString DescribeGameInstanceEnumValueForRewardDebug(const UEnum* Enum, int64 Value)
{
	return Enum ? Enum->GetNameStringByValue(Value) : FString::Printf(TEXT("%lld"), Value);
}

FString DescribeGameInstanceLootOptionsForRewardDebug(const TArray<FLootOption>& Options)
{
	if (Options.IsEmpty())
	{
		return TEXT("Count=0 []");
	}

	TArray<FString> Parts;
	Parts.Reserve(Options.Num());
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const FLootOption& Option = Options[Index];
		Parts.Add(FString::Printf(
			TEXT("#%d{Type=%s,Amount=%d,Display=%s,Rune=%s,Icon=%s,Meta=%s}"),
			Index,
			*DescribeGameInstanceEnumValueForRewardDebug(StaticEnum<ELootType>(), static_cast<int64>(Option.LootType)),
			Option.Amount,
			*Option.DisplayName.ToString(),
			*GetNameSafe(Option.RuneAsset.Get()),
			*GetNameSafe(Option.Icon.Get()),
			*Option.MetaCurrencyTag.ToString()));
	}

	return FString::Printf(TEXT("Count=%d [%s]"), Options.Num(), *FString::Join(Parts, TEXT("; ")));
}
}

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{
	bShouldLoadSaveAfterMap = false;
	MainGameMap = FSoftObjectPath(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom.InitialRoom"));
	FirstRunTutorialMap = MainGameMap;
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
	UE_LOG(LogTemp, Log, TEXT("[Frontend] OnPostLoadMap: World=%s FrontendLoading=%d FrontendWidget=%d"),
		World ? *World->GetName() : TEXT("None"),
		bFrontendLoadingGameplayMap ? 1 : 0,
		FrontendWidget.IsValid() ? 1 : 0);

	if (FrontendWidget.IsValid())
	{
		RemoveFrontendWidget();
	}

	if (bFrontendLoadingGameplayMap)
	{
		UE_LOG(LogTemp, Log, TEXT("[Frontend] PostLoadMap received for %s; clearing frontend loading screen."),
			World ? *World->GetName() : TEXT("None"));
		bFrontendMapLoaded = true;
		bFrontendMinLoadTimeElapsed = true;
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
	EntryMenuWidget->OnStartRequested.AddDynamic(this, &UYogGameInstanceBase::ShowSlotSelectMenu);
	EntryMenuWidget->OnContinueRequested.AddDynamic(this, &UYogGameInstanceBase::ShowSlotSelectMenu);
	EntryMenuWidget->OnOptionsRequested.AddDynamic(this, &UYogGameInstanceBase::HandleEntryOptionsRequested);
	EntryMenuWidget->OnQuitRequested.AddDynamic(this, &UYogGameInstanceBase::QuitFromFrontend);
	UIManager->PushScreen(EYogUIScreenId::EntryMenu);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UYogGameInstanceBase::RefocusEntryMenuWidget);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("AutoStart")))
	{
		StartNormalRunFromFrontend();
	}
}

void UYogGameInstanceBase::ShowSlotSelectMenu()
{
	if (bFrontendLoadingGameplayMap)
	{
		return;
	}

	GetFrontendMainMenuBackgroundBrush();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UYogUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Cannot show slot select menu: UIManager is missing."));
		return;
	}

	RemoveFrontendWidget();

	TSubclassOf<UYogEntryMenuWidget> WidgetClass = SlotSelectMenuClass;
	if (!WidgetClass)
	{
		WidgetClass = EntryMenuClass;
		if (!WidgetClass)
		{
			WidgetClass = UYogEntryMenuWidget::StaticClass();
		}
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
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Cannot show slot select menu: widget class is invalid."));
		return;
	}

	EntryMenuWidget->SetBackgroundTexture(FrontendMainMenuTexture);
	EntryMenuWidget->OnStartRequested.RemoveAll(this);
	EntryMenuWidget->OnContinueRequested.RemoveAll(this);
	EntryMenuWidget->OnOptionsRequested.RemoveAll(this);
	EntryMenuWidget->OnQuitRequested.RemoveAll(this);
	EntryMenuWidget->OnStartRequested.AddDynamic(this, &UYogGameInstanceBase::StartNormalRunFromFrontend);
	EntryMenuWidget->OnContinueRequested.AddDynamic(this, &UYogGameInstanceBase::ContinueRunFromFrontend);
	EntryMenuWidget->OnOptionsRequested.AddDynamic(this, &UYogGameInstanceBase::HandleEntryOptionsRequested);
	EntryMenuWidget->OnQuitRequested.AddDynamic(this, &UYogGameInstanceBase::ShowMainMenu);
	UIManager->PushScreen(EYogUIScreenId::EntryMenu);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UYogGameInstanceBase::RefocusEntryMenuWidget);
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

void UYogGameInstanceBase::ContinueRunFromFrontend()
{
	if (bFrontendLoadingGameplayMap) return;

	UE_LOG(LogTemp, Warning,
		TEXT("[Frontend] ContinueRunFromFrontend entry — stale state before clear: PendingRoomData=%s PendingNextFloor=%d PendingRunState.bIsValid=%d"),
		*GetNameSafe(PendingRoomData),
		PendingNextFloor,
		PendingRunState.bIsValid ? 1 : 0);

	// 防御：清掉同进程内残留的过渡字段（PendingRoomData / PendingNextFloor 等），
	// 否则 StartLevelSpawning 会把它当成"从传送门进新房间"，在 Hub 里 spawn 出商店之类的脏物体。
	// RestoreRunStateFromCheckpoint 只恢复 PendingRunState，这里清过渡字段与 Restore 不冲突。
	ClearPendingTransitionFields();

	UYogSaveSubsystem* SaveSys = GetSubsystem<UYogSaveSubsystem>();
	if (SaveSys && SaveSys->TryRestoreRunCheckpoint())
	{
		// Checkpoint valid — restore and go straight into the level
		RemoveFrontendWidget();
		BeginLoadMainGameMap();
	}
	else
	{
		// No checkpoint — start a fresh run
		StartNewRunFromFrontend();
	}
}

void UYogGameInstanceBase::StartNewRunFromFrontend()
{
	if (bFrontendLoadingGameplayMap)
	{
		UE_LOG(LogTemp, Log, TEXT("[Frontend] Start ignored because the gameplay map is already loading."));
		return;
	}

	if (UYogSaveSubsystem* SS = GetSubsystem<UYogSaveSubsystem>())
	{
		SS->RecordRunStarted();
	}

	RemoveFrontendWidget();
	ClearRunState();
	BeginLoadMainGameMap();
}

void UYogGameInstanceBase::StartNormalRunFromFrontend()
{
	if (bFrontendLoadingGameplayMap)
	{
		UE_LOG(LogTemp, Log, TEXT("[Frontend] Normal start ignored because the gameplay map is already loading."));
		return;
	}

	if (UYogSaveSubsystem* SS = GetSubsystem<UYogSaveSubsystem>())
	{
		SS->SelectSlot(SS->GetNormalGameSlotIndex());
	}

	StartNewRunFromFrontend();
}

void UYogGameInstanceBase::QueueFirstRunWorldRewindHint()
{
	bPendingFirstRunWorldRewindHint = true;
}

bool UYogGameInstanceBase::ConsumeFirstRunWorldRewindHint()
{
	const bool bWasPending = bPendingFirstRunWorldRewindHint;
	bPendingFirstRunWorldRewindHint = false;
	return bWasPending;
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
	const bool bUseFirstRunTutorialMap = [&]()
	{
		if (UYogSaveSubsystem* SaveSys = GetSubsystem<UYogSaveSubsystem>())
		{
			return SaveSys->IsFirstRunTutorialActive() && FirstRunTutorialMap.IsValid();
		}
		return false;
	}();
	const FSoftObjectPath& TargetMap = bUseFirstRunTutorialMap ? FirstRunTutorialMap : MainGameMap;
	const FString PackageName = TargetMap.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Could not resolve map package from %s"), *TargetMap.ToString());
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

	UE_LOG(LogTemp, Log, TEXT("[Frontend] Opening gameplay map %s (FirstRunTutorial=%d)."), *PackageName, bUseFirstRunTutorialMap ? 1 : 0);
	UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
}

void UYogGameInstanceBase::HandleMainGameMapPreloaded()
{
	const bool bUseFirstRunTutorialMap = [&]()
	{
		if (UYogSaveSubsystem* SaveSys = GetSubsystem<UYogSaveSubsystem>())
		{
			return SaveSys->IsFirstRunTutorialActive() && FirstRunTutorialMap.IsValid();
		}
		return false;
	}();
	const FSoftObjectPath& TargetMap = bUseFirstRunTutorialMap ? FirstRunTutorialMap : MainGameMap;
	const FString PackageName = TargetMap.GetLongPackageName();
	if (PackageName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[Frontend] Could not resolve map package from %s"), *TargetMap.ToString());
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

void UYogGameInstanceBase::ShowGameOverScreen(bool bCanRevive, bool bScriptedDefeat)
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

	GameOverWidget->Setup(bCanRevive, bScriptedDefeat);
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
	// 先清过渡字段，避免 QuickSave 之外的派生（如 PendingRoomData）继续指向上一关 → 后续 Continue 误用
	ClearPendingTransitionFields();
	if (UYogSaveSubsystem* SaveSys = GetSubsystem<UYogSaveSubsystem>())
	{
		SaveSys->QuickSave();
	}
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
	StartNormalRunFromFrontend();
	return FReply::Handled();
}

FReply UYogGameInstanceBase::HandleContinueClicked()
{
	ContinueRunFromFrontend();
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
	ClearPendingTransitionFields();

	UE_LOG(LogTemp, Log, TEXT("ClearRunState: 跑局状态已清空（含传送门预骰）"));
}

void UYogGameInstanceBase::ClearPendingTransitionFields()
{
	// v3：传送门跨关字段同点清，避免下一局误读上一局的预骰数据
	PendingRoomData = nullptr;
	PendingRoomBuffs.Reset();
	ClearPendingRoomRewardOptionsOverride();
	ClearPendingStoryNextRoomPlan();
	PendingNextFloor = 1;
	bPlayLevelIntroFadeIn = false;
	ClearCampaignOverride();
}





void UYogGameInstanceBase::SetPendingRoomRewardOptionsOverride(const TArray<FLootOption>& InOptions)
{
	PendingRoomRewardOptionsOverride = InOptions;
	bHasPendingRoomRewardOptionsOverride = true;

	UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GI SetPendingRoomRewardOptionsOverride Options=%s"),
		*DescribeGameInstanceLootOptionsForRewardDebug(PendingRoomRewardOptionsOverride));
	RefreshOpenPortalRewardPreviews();
}

void UYogGameInstanceBase::ClearPendingRoomRewardOptionsOverride()
{
	if (bHasPendingRoomRewardOptionsOverride || !PendingRoomRewardOptionsOverride.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GI ClearPendingRoomRewardOptionsOverride OldOptions=%s"),
			*DescribeGameInstanceLootOptionsForRewardDebug(PendingRoomRewardOptionsOverride));
	}

	bHasPendingRoomRewardOptionsOverride = false;
	PendingRoomRewardOptionsOverride.Reset();
	RefreshOpenPortalRewardPreviews();
}

bool UYogGameInstanceBase::ConsumePendingRoomRewardOptionsOverride(TArray<FLootOption>& OutOptions)
{
	if (!bHasPendingRoomRewardOptionsOverride)
	{
		OutOptions.Reset();
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GI ConsumePendingRoomRewardOptionsOverride no pending override."));
		return false;
	}

	OutOptions = PendingRoomRewardOptionsOverride;
	UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GI ConsumePendingRoomRewardOptionsOverride consuming Options=%s"),
		*DescribeGameInstanceLootOptionsForRewardDebug(OutOptions));
	ClearPendingRoomRewardOptionsOverride();
	return true;
}

bool UYogGameInstanceBase::GetPendingRoomRewardOptionsOverride(TArray<FLootOption>& OutOptions) const
{
	if (!bHasPendingRoomRewardOptionsOverride)
	{
		OutOptions.Reset();
		return false;
	}

	OutOptions = PendingRoomRewardOptionsOverride;
	return true;
}

void UYogGameInstanceBase::SetPendingStoryNextRoomPlan(const FStoryNextRoomPlan& InPlan)
{
	PendingStoryNextRoomPlan = InPlan;
	bHasPendingStoryNextRoomPlan = InPlan.HasAnyOverride();

	if (InPlan.bOverrideRewardOptions)
	{
		SetPendingRoomRewardOptionsOverride(InPlan.RewardOptionsOverride);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[StoryNextRoomPlan] Set Pending=%d ForcePortal=%d PortalIndex=%d Room=%s RewardOverride=%d RewardCount=%d BuffOverride=%d BuffCount=%d"),
		bHasPendingStoryNextRoomPlan ? 1 : 0,
		InPlan.bForceSinglePortal ? 1 : 0,
		InPlan.PortalIndex,
		*GetNameSafe(InPlan.RoomDataOverride.Get()),
		InPlan.bOverrideRewardOptions ? 1 : 0,
		InPlan.RewardOptionsOverride.Num(),
		InPlan.bOverrideBuffs ? 1 : 0,
		InPlan.BuffsOverride.Num());

	RefreshOpenPortalRewardPreviews();
}

void UYogGameInstanceBase::ClearPendingStoryNextRoomPlan()
{
	if (bHasPendingStoryNextRoomPlan)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[StoryNextRoomPlan] Cleared Room=%s PortalIndex=%d"),
			*GetNameSafe(PendingStoryNextRoomPlan.RoomDataOverride.Get()),
			PendingStoryNextRoomPlan.PortalIndex);
	}

	bHasPendingStoryNextRoomPlan = false;
	PendingStoryNextRoomPlan = FStoryNextRoomPlan();
}

bool UYogGameInstanceBase::ConsumePendingStoryNextRoomPlan(FStoryNextRoomPlan& OutPlan)
{
	if (!bHasPendingStoryNextRoomPlan)
	{
		OutPlan = FStoryNextRoomPlan();
		return false;
	}

	OutPlan = PendingStoryNextRoomPlan;
	ClearPendingStoryNextRoomPlan();
	return true;
}

bool UYogGameInstanceBase::GetPendingStoryNextRoomPlan(FStoryNextRoomPlan& OutPlan) const
{
	if (!bHasPendingStoryNextRoomPlan)
	{
		OutPlan = FStoryNextRoomPlan();
		return false;
	}

	OutPlan = PendingStoryNextRoomPlan;
	return true;
}

void UYogGameInstanceBase::RefreshOpenPortalRewardPreviews()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[StoryRewardDebug] GI RefreshOpenPortalRewardPreviews skipped: World is null Pending=%d Options=%s"),
			bHasPendingRoomRewardOptionsOverride ? 1 : 0,
			*DescribeGameInstanceLootOptionsForRewardDebug(PendingRoomRewardOptionsOverride));
		return;
	}

	TArray<AActor*> PortalActors;
	UGameplayStatics::GetAllActorsOfClass(World, APortal::StaticClass(), PortalActors);
	int32 RefreshedCount = 0;
	for (AActor* Actor : PortalActors)
	{
		APortal* Portal = Cast<APortal>(Actor);
		if (Portal && Portal->bIsOpen)
		{
			++RefreshedCount;
			UE_LOG(LogTemp, Log,
				TEXT("[StoryRewardDebug] GI RefreshOpenPortalRewardPreviews refreshing Portal=%s Index=%d RevisionBefore=%d Pending=%d"),
				*GetNameSafe(Portal),
				Portal->Index,
				Portal->GetPreviewRevision(),
				bHasPendingRoomRewardOptionsOverride ? 1 : 0);
			Portal->RefreshPreviewInfo();
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] GI RefreshOpenPortalRewardPreviews World=%s Pending=%d PortalActors=%d OpenRefreshed=%d Options=%s"),
		*GetNameSafe(World),
		bHasPendingRoomRewardOptionsOverride ? 1 : 0,
		PortalActors.Num(),
		RefreshedCount,
		*DescribeGameInstanceLootOptionsForRewardDebug(PendingRoomRewardOptionsOverride));
}

void UYogGameInstanceBase::SetCampaignOverride(UCampaignDataAsset* InCampaignData)
{
	CampaignOverrideData = InCampaignData;
	bCampaignOverrideActive = InCampaignData != nullptr;

	UE_LOG(LogTemp, Log, TEXT("[CampaignOverride] Set active=%d campaign=%s"),
		bCampaignOverrideActive ? 1 : 0,
		*GetNameSafe(CampaignOverrideData));
}

void UYogGameInstanceBase::ClearCampaignOverride()
{
	if (bCampaignOverrideActive || CampaignOverrideData)
	{
		UE_LOG(LogTemp, Log, TEXT("[CampaignOverride] Cleared campaign=%s"),
			*GetNameSafe(CampaignOverrideData));
	}
	CampaignOverrideData = nullptr;
	bCampaignOverrideActive = false;
}

bool UYogGameInstanceBase::HasCampaignOverride() const
{
	return bCampaignOverrideActive && CampaignOverrideData != nullptr;
}

UCampaignDataAsset* UYogGameInstanceBase::GetCampaignOverrideData() const
{
	return HasCampaignOverride() ? CampaignOverrideData.Get() : nullptr;
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
