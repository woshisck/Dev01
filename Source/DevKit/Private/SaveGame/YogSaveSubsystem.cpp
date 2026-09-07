#include "SaveGame/YogSaveSubsystem.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "System/YogPerformanceSettingsLibrary.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveGameArchive.h"
#include "SaveGame/YogWeaponSaveSupport.h"
#include "Async/Async.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"
#include "DevAssetManager.h"
#include "YogBlueprintFunctionLibrary.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Data/WeaponSkillDataAsset.h"
#include "Kismet/GameplayStatics.h"

static const int32 GNumSaveSlots    = 3;
static const int32 GSettingsUserIdx = 0;
static const FString GSettingsSlot  = TEXT("Settings");
static const int32 GFirstRunTutorialStageNone = 0;
static const int32 GFirstRunTutorialStageCompleted = 8;

// =========================================================
// 初始化
// =========================================================

void UYogSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureSaveQueue();
	SaveTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UYogSaveSubsystem::TickSaveQueue));
	FCoreDelegates::OnEnginePreExit.AddUObject(this, &UYogSaveSubsystem::HandleEnginePreExit);

	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}

	LoadSettings();
	UYogPerformanceSettingsLibrary::ApplySavedGraphicsSettings(GetGameInstance());
	EnsureReservedNormalGameSlot();

	if (CurrentSettings && CurrentSettings->LastActiveSlot >= 0 && CurrentSettings->LastActiveSlot < GNumSaveSlots)
	{
		SelectSlot(CurrentSettings->LastActiveSlot);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYogSaveSubsystem::OnLevelLoaded);

	if (UWorld* World = GetWorld())
	{
		if (AYogGameMode* GameMode = Cast<AYogGameMode>(World->GetAuthGameMode()))
		{
			GameMode->OnFinishLevelEvent().AddUObject(this, &UYogSaveSubsystem::WriteSaveGame);
		}
	}
}

void UYogSaveSubsystem::HandleEnginePreExit()
{
	if (bDeinitializing) return;
	bDeinitializing = true;
	if (SaveQueue)
	{
		const bool bSaved = SaveQueue->DrainForShutdown();
		DispatchSaveResults();
		if (!bSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Shutdown save FAILED after one final retry. Unsaved snapshots cannot be recovered after this process exits."));
		}
	}
}

void UYogSaveSubsystem::Deinitialize()
{
	HandleEnginePreExit(); // Same once-only drain whether or not the engine exit hook ran.
	FTSTicker::GetCoreTicker().RemoveTicker(SaveTickerHandle);
	SaveTickerHandle.Reset();
	FCoreDelegates::OnEnginePreExit.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	SaveQueue.Reset();
	Super::Deinitialize();
}

// =========================================================
// 多槽位管理
// =========================================================

FString UYogSaveSubsystem::GetSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("SaveSlot_%d"), FMath::Clamp(SlotIndex, 0, GNumSaveSlots - 1));
}

static const int32 GCurrentSaveFormatVersion = 2;

bool UYogSaveSubsystem::IsNormalGameSlot(int32 SlotIndex) const
{
	return FMath::Clamp(SlotIndex, 0, GNumSaveSlots - 1) == GNumSaveSlots - 1;
}

int32 UYogSaveSubsystem::GetNormalGameSlotIndex() const
{
	return GNumSaveSlots - 1;
}

void UYogSaveSubsystem::InitializeSaveForNewGame(UYogSaveGame* Save, bool bFirstRunTutorial) const
{
	if (!Save)
	{
		return;
	}

	Save->MetaProgression = FMetaProgressionData{};
	Save->SelectedSkillLoadout.Reset();
	Save->RunCheckpoint = FRunCheckpointData{};
	Save->PlayerStateData = FPlayerGASData{};
	Save->WeaponInstanceItems.Reset();
	Save->MapStateData = FYogMapStateData{};
	Save->SavedCharacter.Reset();
	Save->TutorialState = bFirstRunTutorial ? ETutorialState::NeedWeaponTutorial : ETutorialState::Completed;
	Save->FirstRunTutorialStage = bFirstRunTutorial
		? GFirstRunTutorialStageNone
		: GFirstRunTutorialStageCompleted;
	Save->ShownPopupKeys.Empty();
	Save->StoryFlags.Empty();

	const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
	const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
	if (bFirstRunTutorial)
	{
		if (ActiveTag.IsValid())
		{
			Save->StoryFlags.Add(ActiveTag, true);
		}
		if (CompletedTag.IsValid())
		{
			Save->StoryFlags.Remove(CompletedTag);
		}
	}
	else
	{
		if (ActiveTag.IsValid())
		{
			Save->StoryFlags.Remove(ActiveTag);
		}
		if (CompletedTag.IsValid())
		{
			Save->StoryFlags.Add(CompletedTag, true);
		}
	}

	Save->StoryFiredRuleIds.Empty();
	Save->StoryQuestTasks.Empty();
	Save->SlotCreatedTime = FDateTime::Now();
	Save->SlotLastPlayTime = FDateTime::Now();
	Save->SaveFormatVersion = GCurrentSaveFormatVersion;
}

void UYogSaveSubsystem::EnsureReservedNormalGameSlot()
{
	const int32 SlotIndex = GNumSaveSlots - 1;
	const FString SlotName = GetSlotName(SlotIndex);
	FlushPendingSaves();
	TArray<uint8> UnsavedBytes;
	if (WriteProtectedSlots.Contains(SlotIndex)
		|| (SaveQueue && SaveQueue->GetUnsavedBytes(SlotName, UnsavedBytes)))
	{
		return; // Never replace an unsaved session snapshot with an empty reserved slot.
	}
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (UYogSaveGame* ExistingSave = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			if (ExistingSave->SaveFormatVersion > GCurrentSaveFormatVersion)
			{
				WriteProtectedSlots.Add(SlotIndex);
				ReportSaveFailure(SlotName, TEXT("Load"), TEXT("Newer save format; original file preserved."));
				return;
			}
			bool bChanged = false;
			if (ExistingSave->TutorialState != ETutorialState::Completed)
			{
				ExistingSave->TutorialState = ETutorialState::Completed;
				bChanged = true;
			}
			if (ExistingSave->FirstRunTutorialStage != GFirstRunTutorialStageCompleted)
			{
				ExistingSave->FirstRunTutorialStage = GFirstRunTutorialStageCompleted;
				bChanged = true;
			}
			if (const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
				ActiveTag.IsValid())
			{
				bChanged |= ExistingSave->StoryFlags.Remove(ActiveTag) > 0;
			}
			if (const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
				CompletedTag.IsValid())
			{
				const bool bWasCompleted = ExistingSave->StoryFlags.FindRef(CompletedTag);
				ExistingSave->StoryFlags.Add(CompletedTag, true);
				bChanged |= !bWasCompleted;
			}
			if (ExistingSave->SaveFormatVersion < GCurrentSaveFormatVersion)
			{
				ExistingSave->SaveFormatVersion = GCurrentSaveFormatVersion;
				bChanged = true;
			}
			if (bChanged)
			{
				EnqueueSave(ExistingSave, SlotIndex);
			}
		}
		else
		{
			WriteProtectedSlots.Add(SlotIndex);
			ReportSaveFailure(SlotName, TEXT("Load"), TEXT("Unreadable save; original file preserved."));
		}
		return;
	}

	UYogSaveGame* NormalSave = Cast<UYogSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	InitializeSaveForNewGame(NormalSave, false);
	EnqueueSave(NormalSave, SlotIndex);
}

void UYogSaveSubsystem::SelectSlot(int32 SlotIndex)
{
	if (!SlotState.TryBeginMutation())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Ignored reentrant SelectSlot(%d)."), SlotIndex);
		return;
	}
	ON_SCOPE_EXIT { SlotState.EndMutation(); };
	SelectSlotInternal(SlotIndex);
}

UYogSaveGame* UYogSaveSubsystem::SelectSlotInternal(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= GNumSaveSlots)
	{
		ReportSaveFailure(FString::FromInt(SlotIndex), TEXT("Select"), TEXT("Invalid slot index."));
		return nullptr;
	}
	FlushPendingSaves();
	CurrentSlotIndex = SlotIndex;
	const FString SlotName = GetSlotName(CurrentSlotIndex);
	TArray<uint8> UnsavedBytes;

	if (SaveQueue && SaveQueue->GetUnsavedBytes(SlotName, UnsavedBytes))
	{
		CurrentSaveGame = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromMemory(UnsavedBytes));
	}
	else if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (!CurrentSaveGame)
		{
			WriteProtectedSlots.Add(SlotIndex);
			ReportSaveFailure(SlotName, TEXT("Load"), TEXT("Unreadable save; automatic overwrite disabled. Explicit reset/delete is required."));
		}
	}
	else
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		InitializeSaveForNewGame(CurrentSaveGame, !IsNormalGameSlot(CurrentSlotIndex));
	}
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		InitializeSaveForNewGame(CurrentSaveGame, !IsNormalGameSlot(CurrentSlotIndex));
	}
	if (CurrentSaveGame->SaveFormatVersion > GCurrentSaveFormatVersion)
	{
		WriteProtectedSlots.Add(SlotIndex);
		ReportSaveFailure(SlotName, TEXT("Load"), TEXT("Newer save format; automatic overwrite disabled."));
	}

	if (CurrentSaveGame && CurrentSaveGame->SaveFormatVersion < GCurrentSaveFormatVersion)
	{
		MigrateSaveGame(CurrentSaveGame, CurrentSaveGame->SaveFormatVersion, GCurrentSaveFormatVersion);
		DoAsyncSave();
	}

	if (CurrentSaveGame && IsNormalGameSlot(CurrentSlotIndex))
	{
		bool bChanged = false;
		if (CurrentSaveGame->TutorialState != ETutorialState::Completed)
		{
			CurrentSaveGame->TutorialState = ETutorialState::Completed;
			bChanged = true;
		}
		if (CurrentSaveGame->FirstRunTutorialStage != GFirstRunTutorialStageCompleted)
		{
			CurrentSaveGame->FirstRunTutorialStage = GFirstRunTutorialStageCompleted;
			bChanged = true;
		}
		if (const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
			ActiveTag.IsValid())
		{
			bChanged |= CurrentSaveGame->StoryFlags.Remove(ActiveTag) > 0;
		}
		if (const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
			CompletedTag.IsValid())
		{
			const bool bWasCompleted = CurrentSaveGame->StoryFlags.FindRef(CompletedTag);
			CurrentSaveGame->StoryFlags.Add(CompletedTag, true);
			bChanged |= !bWasCompleted;
		}
		if (bChanged)
		{
			DoAsyncSave();
		}
	}

	if (CurrentSettings)
	{
		CurrentSettings->LastActiveSlot = CurrentSlotIndex;
		SaveSettings();
	}

	TStrongObjectPtr<UYogSaveGame> SelectedSave(CurrentSaveGame.Get());
	OnSaveGameLoaded.Broadcast(SelectedSave.Get());
	return SelectedSave.Get();
}

void UYogSaveSubsystem::DeleteSlot(int32 SlotIndex)
{
	if (!SlotState.TryBeginMutation())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Ignored reentrant DeleteSlot(%d)."), SlotIndex);
		return;
	}
	ON_SCOPE_EXIT { SlotState.EndMutation(); };
	if (SlotIndex < 0 || SlotIndex >= GNumSaveSlots)
	{
		ReportSaveFailure(FString::FromInt(SlotIndex), TEXT("Delete"), TEXT("Invalid slot index."));
		return;
	}
	EnsureSaveQueue();
	SlotState.AdvancePreviewRevision(SlotIndex);
	const FString SlotName = GetSlotName(SlotIndex);
	SaveQueue->Delete(SlotName);
	SaveQueue->Flush(); // Delete follows the in-flight write; never races it.
	const bool bStillExists = UGameplayStatics::DoesSaveGameExist(SlotName, 0);
	if (bStillExists)
	{
		DispatchSaveResults();
		return;
	}
	WriteProtectedSlots.Remove(SlotIndex);

	if (SlotIndex == CurrentSlotIndex)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		InitializeSaveForNewGame(CurrentSaveGame, !IsNormalGameSlot(CurrentSlotIndex));
	}
	DispatchSaveResults();

	if (IsNormalGameSlot(SlotIndex))
	{
		EnsureReservedNormalGameSlot();
	}
}

void UYogSaveSubsystem::ResetSlotForNewGame(int32 SlotIndex)
{
	if (!SlotState.TryBeginMutation())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Ignored reentrant ResetSlotForNewGame(%d)."), SlotIndex);
		return;
	}
	ON_SCOPE_EXIT { SlotState.EndMutation(); };
	if (SlotIndex < 0 || SlotIndex >= GNumSaveSlots) return;
	SlotState.AdvancePreviewRevision(SlotIndex);
	TStrongObjectPtr<UYogSaveGame> RequestedSave(SelectSlotInternal(SlotIndex));
	if (!FYogSaveSlotState::IsSameSelection(SlotIndex, RequestedSave.Get(), CurrentSlotIndex, CurrentSaveGame.Get()))
	{
		// Do not broadcast another failure from a callback-induced identity mismatch.
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] ResetSlotForNewGame(%d) rejected: selected slot/object changed during notification."), SlotIndex);
		return;
	}
	WriteProtectedSlots.Remove(SlotIndex); // Explicit user reset, not automatic repair.

	// 保留 Statistics，清空其余局外数据和存档点
	InitializeSaveForNewGame(RequestedSave.Get(), !IsNormalGameSlot(SlotIndex));
	EnqueueSave(RequestedSave.Get(), SlotIndex);
}

bool UYogSaveSubsystem::IsFirstRunTutorialActive() const
{
	if (!CurrentSaveGame)
	{
		return false;
	}

	const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
	const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
	const bool bActive = ActiveTag.IsValid()
		&& CurrentSaveGame->StoryFlags.FindRef(ActiveTag);
	const bool bCompleted = CompletedTag.IsValid()
		&& CurrentSaveGame->StoryFlags.FindRef(CompletedTag);
	return bActive && !bCompleted;
}

bool UYogSaveSubsystem::IsFirstRunTutorialCompleted() const
{
	if (!CurrentSaveGame)
	{
		return false;
	}

	const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
	return CompletedTag.IsValid() && CurrentSaveGame->StoryFlags.FindRef(CompletedTag);
}

void UYogSaveSubsystem::MarkFirstRunTutorialCompleted()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	if (const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
		ActiveTag.IsValid())
	{
		CurrentSaveGame->StoryFlags.Remove(ActiveTag);
	}
	if (const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
		CompletedTag.IsValid())
	{
		CurrentSaveGame->StoryFlags.Add(CompletedTag, true);
	}
	CurrentSaveGame->TutorialState = ETutorialState::Completed;
	CurrentSaveGame->FirstRunTutorialStage = GFirstRunTutorialStageCompleted;

	DoAsyncSave();
}

void UYogSaveSubsystem::SetFirstRunTutorialStage(int32 Stage)
{
	if (!CurrentSaveGame)
	{
		return;
	}

	CurrentSaveGame->FirstRunTutorialStage = Stage;
	DoAsyncSave();
}

int32 UYogSaveSubsystem::GetFirstRunTutorialStage() const
{
	return CurrentSaveGame ? CurrentSaveGame->FirstRunTutorialStage : 0;
}

void UYogSaveSubsystem::RequestSlotPreview(int32 SlotIndex, FOnSlotPreviewReady Callback)
{
	if (SlotIndex < 0 || SlotIndex >= GNumSaveSlots)
	{
		Callback.ExecuteIfBound(FSlotPreviewData{});
		return;
	}
	if (IsNormalGameSlot(SlotIndex))
	{
		EnsureReservedNormalGameSlot();
	}
	FlushPendingSaves();

	const FString SlotName = GetSlotName(SlotIndex);
	const uint64 PreviewRevision = SlotState.GetPreviewRevision(SlotIndex);
	TWeakObjectPtr<UYogSaveSubsystem> WeakThis(this);

	// 异步加载，避免主线程卡顿
	FAsyncLoadGameFromSlotDelegate LoadDelegate;
	LoadDelegate.BindLambda([WeakThis, SlotIndex, PreviewRevision, Callback](const FString&, const int32, USaveGame* LoadedGame)
	{
		UYogSaveSubsystem* SaveSubsystem = WeakThis.Get();
		if (!SaveSubsystem || SaveSubsystem->bDeinitializing
			|| !SaveSubsystem->SlotState.IsPreviewCurrent(SlotIndex, PreviewRevision)) return;
		FSlotPreviewData Preview;
		if (UYogSaveGame* Save = Cast<UYogSaveGame>(LoadedGame))
		{
			Preview.bHasData             = true;
			Preview.LastPlayTime         = Save->SlotLastPlayTime;
			Preview.HighestFloor         = Save->Statistics.HighestFloor;
			Preview.bHasPendingRun       = Save->RunCheckpoint.bIsValid; // 单一事实源
			Preview.TotalPlayTimeSeconds = Save->Statistics.TotalPlayTimeSeconds;

			const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
			const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
			Preview.bFirstRunTutorialCompleted = CompletedTag.IsValid() && Save->StoryFlags.FindRef(CompletedTag);
			Preview.bFirstRunTutorialActive = ActiveTag.IsValid()
				&& Save->StoryFlags.FindRef(ActiveTag)
				&& !Preview.bFirstRunTutorialCompleted;
		}
		Callback.ExecuteIfBound(Preview);
	});

	TArray<uint8> UnsavedBytes;
	if (SaveQueue && SaveQueue->GetUnsavedBytes(SlotName, UnsavedBytes))
	{
		LoadDelegate.Execute(SlotName, 0, UGameplayStatics::LoadGameFromMemory(UnsavedBytes));
		return;
	}
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		Callback.ExecuteIfBound(FSlotPreviewData{});
		return;
	}
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, 0, LoadDelegate);
}

// =========================================================
// 存档点
// =========================================================

void UYogSaveSubsystem::TriggerCheckpoint(int32 CurrentFloor)
{
	if (!CurrentSaveGame)
	{
		return;
	}

	PopulateCheckpointFromRunState(CurrentSaveGame->RunCheckpoint, CurrentFloor);
	CurrentSaveGame->SlotLastPlayTime = FDateTime::Now();

	if (CurrentFloor > CurrentSaveGame->Statistics.HighestFloor)
	{
		CurrentSaveGame->Statistics.HighestFloor = CurrentFloor;
	}

	DoAsyncSave();
}

void UYogSaveSubsystem::ClearRunCheckpoint()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	CurrentSaveGame->RunCheckpoint = FRunCheckpointData{};
	DoAsyncSave();
}

// =========================================================
// 快速存档（背包 UI 关闭时调用）
// =========================================================

void UYogSaveSubsystem::QuickSave()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	// 复用 TriggerCheckpoint 逻辑，楼层从 GameInstance 读取
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	const int32 Floor = GI ? GI->PendingNextFloor : CurrentSaveGame->RunCheckpoint.CheckpointFloor;

	PopulateCheckpointFromRunState(CurrentSaveGame->RunCheckpoint, Floor);
	CurrentSaveGame->SlotLastPlayTime = FDateTime::Now();
	DoAsyncSave();
}

// =========================================================
// FRunState ↔ FRunCheckpointData 互转
// =========================================================

void UYogSaveSubsystem::PopulateCheckpointFromRunState(FRunCheckpointData& Out, int32 Floor)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (!GI || !GI->PendingRunState.bIsValid)
	{
		return;
	}

	const FRunState& RS = GI->PendingRunState;

	Out.bIsValid        = true;
	Out.CheckpointFloor = Floor;
	Out.CurrentHP       = RS.CurrentHP;
	Out.CurrentGold     = RS.CurrentGold;
	Out.CurrentPhase    = RS.CurrentPhase;
	Out.CurrentHeat     = RS.CurrentHeat;
	Out.CompletedCombatBattleCount        = RS.CompletedCombatBattleCount;
	Out.CombatDeckShuffleCooldownDuration = RS.CombatDeckShuffleCooldownDuration;
	Out.CombatDeckMaxActiveSequenceSize   = RS.CombatDeckMaxActiveSequenceSize;
	Out.InactiveCombatDeckShuffleCooldownDuration = RS.InactiveCombatDeckShuffleCooldownDuration;
	Out.InactiveCombatDeckMaxActiveSequenceSize   = RS.InactiveCombatDeckMaxActiveSequenceSize;
	Out.PlacedRunes                       = RS.PlacedRunes;
	Out.PendingRunes                      = RS.PendingRunes;
	Out.HiddenPassiveRuneInstances        = RS.HiddenPassiveRuneInstances;
	Out.SacrificeOfferingCosts            = RS.SacrificeOfferingCosts;
	Out.CombatDeckCardOrientations        = RS.CombatDeckCardOrientations;
	Out.InactiveCombatDeckCardOrientations = RS.InactiveCombatDeckCardOrientations;
	Out.SelectedSkillLoadout.Reset(RS.SelectedSkillLoadout.Num());
	for (const TObjectPtr<UActiveSkillDataAsset>& Skill : RS.SelectedSkillLoadout)
	{
		Out.SelectedSkillLoadout.Add(Skill.Get());
	}
	if (CurrentSaveGame)
	{
		CurrentSaveGame->SelectedSkillLoadout = Out.SelectedSkillLoadout;
	}

	// TObjectPtr → TSoftObjectPtr（仅存路径，不强制加载）
	Out.EquippedWeaponDef  = RS.EquippedWeaponDef.Get();
	Out.InactiveWeaponDef = RS.InactiveWeaponDef.Get();
	Out.EquippedWeaponSkill = RS.EquippedWeaponSkill.Get();
	Out.InactiveWeaponSkill = RS.InactiveWeaponSkill.Get();
	Out.ActiveSacrificeGrace = RS.ActiveSacrificeGrace.Get();

	Out.CombatDeckCards.Reset(RS.CombatDeckCards.Num());
	for (const TObjectPtr<URuneDataAsset>& Card : RS.CombatDeckCards)
	{
		Out.CombatDeckCards.Add(Card.Get());
	}

	Out.InactiveCombatDeckCards.Reset(RS.InactiveCombatDeckCards.Num());
	for (const TObjectPtr<URuneDataAsset>& Card : RS.InactiveCombatDeckCards)
	{
		Out.InactiveCombatDeckCards.Add(Card.Get());
	}
}

bool UYogSaveSubsystem::TryRestoreRunCheckpoint()
{
	UYogSaveGame* Save = GetCurrentSave();
	if (!Save || !Save->RunCheckpoint.bIsValid)
	{
		return false;
	}
	RestoreRunStateFromCheckpoint(Save->RunCheckpoint);
	return true;
}

void UYogSaveSubsystem::RestoreRunStateFromCheckpoint(const FRunCheckpointData& In)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (!GI || !In.bIsValid)
	{
		return;
	}

	FRunState& RS = GI->PendingRunState;

	RS.bIsValid        = true;
	RS.CurrentHP       = In.CurrentHP;
	RS.CurrentGold     = In.CurrentGold;
	RS.CurrentPhase    = In.CurrentPhase;
	RS.CurrentHeat     = In.CurrentHeat;
	RS.CompletedCombatBattleCount        = In.CompletedCombatBattleCount;
	RS.CombatDeckShuffleCooldownDuration = In.CombatDeckShuffleCooldownDuration;
	RS.CombatDeckMaxActiveSequenceSize   = In.CombatDeckMaxActiveSequenceSize;
	RS.InactiveCombatDeckShuffleCooldownDuration = In.InactiveCombatDeckShuffleCooldownDuration;
	RS.InactiveCombatDeckMaxActiveSequenceSize   = In.InactiveCombatDeckMaxActiveSequenceSize;
	RS.PlacedRunes                       = In.PlacedRunes;
	RS.PendingRunes                      = In.PendingRunes;
	RS.HiddenPassiveRuneInstances        = In.HiddenPassiveRuneInstances;
	RS.SacrificeOfferingCosts            = In.SacrificeOfferingCosts;
	RS.CombatDeckCardOrientations        = In.CombatDeckCardOrientations;
	RS.InactiveCombatDeckCardOrientations = In.InactiveCombatDeckCardOrientations;
	RS.SelectedSkillLoadout.Reset(In.SelectedSkillLoadout.Num());
	for (const TSoftObjectPtr<UActiveSkillDataAsset>& SoftSkill : In.SelectedSkillLoadout)
	{
		RS.SelectedSkillLoadout.Add(SoftSkill.LoadSynchronous());
	}

	// TSoftObjectPtr → 同步加载（这里只恢复指针；调用方可在之后 AsyncLoad）
	RS.EquippedWeaponDef  = In.EquippedWeaponDef.LoadSynchronous();
	RS.InactiveWeaponDef = In.InactiveWeaponDef.LoadSynchronous();
	RS.EquippedWeaponSkill = In.EquippedWeaponSkill.LoadSynchronous();
	RS.InactiveWeaponSkill = In.InactiveWeaponSkill.LoadSynchronous();
	RS.ActiveSacrificeGrace = In.ActiveSacrificeGrace.LoadSynchronous();

	RS.CombatDeckCards.Reset(In.CombatDeckCards.Num());
	for (const TSoftObjectPtr<URuneDataAsset>& SoftCard : In.CombatDeckCards)
	{
		RS.CombatDeckCards.Add(SoftCard.LoadSynchronous());
	}

	RS.InactiveCombatDeckCards.Reset(In.InactiveCombatDeckCards.Num());
	for (const TSoftObjectPtr<URuneDataAsset>& SoftCard : In.InactiveCombatDeckCards)
	{
		RS.InactiveCombatDeckCards.Add(SoftCard.LoadSynchronous());
	}
}

// =========================================================
// 全局设置
// =========================================================

void UYogSaveSubsystem::SaveSettings()
{
	if (bSettingsWriteProtected)
	{
		ReportSaveFailure(GSettingsSlot, TEXT("Write"), TEXT("Unreadable settings file preserved; automatic overwrite disabled."));
		return;
	}
	if (!CurrentSettings)
	{
		CurrentSettings = Cast<UYogSettingsSave>(
			UGameplayStatics::CreateSaveGameObject(UYogSettingsSave::StaticClass()));
	}
	if (!UGameplayStatics::SaveGameToSlot(CurrentSettings, GSettingsSlot, GSettingsUserIdx))
	{
		ReportSaveFailure(GSettingsSlot, TEXT("Write"), TEXT("Settings write failed."));
	}
}

void UYogSaveSubsystem::LoadSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(GSettingsSlot, GSettingsUserIdx))
	{
		CurrentSettings = Cast<UYogSettingsSave>(
			UGameplayStatics::LoadGameFromSlot(GSettingsSlot, GSettingsUserIdx));
		bSettingsWriteProtected = CurrentSettings == nullptr;
		if (bSettingsWriteProtected)
		{
			ReportSaveFailure(GSettingsSlot, TEXT("Load"), TEXT("Unreadable settings file preserved."));
		}
	}

	if (!CurrentSettings)
	{
		CurrentSettings = Cast<UYogSettingsSave>(
			UGameplayStatics::CreateSaveGameObject(UYogSettingsSave::StaticClass()));
	}
}

// =========================================================
// 异步写盘（核心，防止并发）
// =========================================================

void UYogSaveSubsystem::DoAsyncSave()
{
	EnqueueSave(CurrentSaveGame, CurrentSlotIndex);
}

void UYogSaveSubsystem::EnsureSaveQueue()
{
	if (!SaveQueue)
	{
		SaveQueue = MakeUnique<FYogSaveOperationQueue>([](const FYogSaveOperation& Operation)
		{
			// UE's generic async save also calls these synchronous storage APIs on a worker.
			// Own the future so shutdown can wait without needing a game-thread callback.
			return Async(EAsyncExecution::ThreadPool, [Request = Operation]()
			{
				if (Request.Kind == EYogSaveOperation::Delete)
				{
					return !UGameplayStatics::DoesSaveGameExist(Request.SlotName, 0)
						|| UGameplayStatics::DeleteGameInSlot(Request.SlotName, 0);
				}
				return UGameplayStatics::SaveDataToSlot(Request.Bytes, Request.SlotName, 0);
			});
		});
	}
}

void UYogSaveSubsystem::EnqueueSave(UYogSaveGame* Save, int32 SlotIndex)
{
	check(IsInGameThread());
	if (!Save || bDeinitializing) return;
	if (SlotIndex < 0 || SlotIndex >= GNumSaveSlots || WriteProtectedSlots.Contains(SlotIndex))
	{
		ReportSaveFailure(FString::FromInt(SlotIndex), TEXT("Write"), TEXT("Slot is invalid or write-protected."));
		return;
	}
	TArray<uint8> Bytes;
	if (!UGameplayStatics::SaveGameToMemory(Save, Bytes) || Bytes.IsEmpty())
	{
		ReportSaveFailure(GetSlotName(SlotIndex), TEXT("Serialize"), TEXT("Could not capture save snapshot."));
		return;
	}
	EnsureSaveQueue();
	SlotState.AdvancePreviewRevision(SlotIndex);
	SaveQueue->Write(GetSlotName(SlotIndex), MoveTemp(Bytes));
}

bool UYogSaveSubsystem::TickSaveQueue(float DeltaTime)
{
	if (SaveQueue)
	{
		SaveQueue->Poll();
		DispatchSaveResults();
	}
	return true;
}

bool UYogSaveSubsystem::FlushPendingSaves()
{
	if (!SaveQueue) return true;
	const bool bSuccess = SaveQueue->Flush();
	DispatchSaveResults();
	return bSuccess;
}

void UYogSaveSubsystem::RetryFailedSaves()
{
	if (SaveQueue && !bDeinitializing) SaveQueue->RetryFailures();
}

void UYogSaveSubsystem::ReportSaveFailure(const FString& SlotName, const FString& Operation, const FString& Reason)
{
	UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] %s FAILED for %s: %s"), *Operation, *SlotName, *Reason);
	if (!bDeinitializing) OnSaveOperationFailed.Broadcast(SlotName, Operation, Reason);
}

void UYogSaveSubsystem::DispatchSaveResults()
{
	if (!SaveQueue || bDispatchingSaveResults) return;
	TGuardValue<bool> Guard(bDispatchingSaveResults, true);
	for (const FYogSaveOperationResult& Result : SaveQueue->TakeResults())
	{
		const FYogSaveOperation& Op = Result.Operation;
		if (!Result.bSuccess)
		{
			ReportSaveFailure(Op.SlotName, Op.Kind == EYogSaveOperation::Write ? TEXT("Write") : TEXT("Delete"),
				TEXT("Storage operation failed; request retained for explicit retry."));
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] %s succeeded: %s"),
			Op.Kind == EYogSaveOperation::Write ? TEXT("Write") : TEXT("Delete"), *Op.SlotName);
		if (Op.Kind == EYogSaveOperation::Write && !bDeinitializing)
		{
			// The listener receives the completed immutable snapshot, not another slot's live object.
			TStrongObjectPtr<UYogSaveGame> Snapshot(Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromMemory(Op.Bytes)));
			OnSaveGameWritten.Broadcast(Snapshot.Get());
		}
	}
}

// =========================================================
// 兼容旧代码的同步写盘（内部改为异步）
// =========================================================

void UYogSaveSubsystem::WriteSaveGame()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WriteSaveGame);

	if (!CurrentSaveGame)
	{
		return;
	}

	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (GS)
	{
		SavePlayer(CurrentSaveGame);
		SaveMap(CurrentSaveGame);
	}

	DoAsyncSave();
}

// =========================================================
// 其余原有接口（保留原逻辑）
// =========================================================

void UYogSaveSubsystem::OnLevelLoaded(UWorld* LoadedWorld)
{
}

UYogSaveGame* UYogSaveSubsystem::GetCurrentSave()
{
	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
	return CurrentSaveGame;
}

UYogSaveGame* UYogSaveSubsystem::CreateSaveGameInst()
{
	return Cast<UYogSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
}

void UYogSaveSubsystem::LoadSaveGame(UYogSaveGame* SaveGame)
{
	LoadPlayer(SaveGame);
}

void UYogSaveSubsystem::SaveData(UObject* Object, UPARAM(ref) TArray<uint8>& Data)
{
	if (!Object) return;
	Data.Reset();
	FMemoryWriter MemoryWriter(Data, true);
	FYogSaveGameArchive MyArchive(MemoryWriter);
	Object->Serialize(MyArchive);
}

void UYogSaveSubsystem::LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data)
{
	if (!Object || Data.IsEmpty()) return;
	FMemoryReader MemoryReader(Data, true);
	FYogSaveGameArchive Ar(MemoryReader);
	Object->Serialize(Ar);
}

void UYogSaveSubsystem::SavePlayer(UYogSaveGame* SaveGame)
{
	if (!SaveGame) return;
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player || !Player->BaseAttributeSet || !Player->GetASC()) return;

	SaveGame->WeaponInstanceItems.Empty();
	SaveGame->PlayerStateData.Abilities.Empty();
	SaveGame->PlayerStateData.PlayerOwnedTags.Empty();

	SaveGame->PlayerStateData.SetupAttribute(*Player->BaseAttributeSet);

	TMap<FGameplayTag, int32> Container = Player->GetASC()->GetPlayerOwnedTagsWithCounts();
	for (const auto& Pair : Container)
	{
		SaveGame->PlayerStateData.PlayerOwnedTags.Add(Pair.Key, Pair.Value);
	}

	SaveData(Player, SaveGame->PlayerStateData.CharacterByteData);
	SaveGame->SelectedSkillLoadout.Reset();
	if (Player->ActiveSkillComponent)
	{
		for (UActiveSkillDataAsset* Skill : Player->ActiveSkillComponent->GetSkillLoadout())
		{
			SaveGame->SelectedSkillLoadout.Add(Skill);
		}
	}

	TArray<AActor*> AttachedActors;
	Player->GetAttachedActors(AttachedActors, true, true);

	for (AActor* Actor : AttachedActors)
	{
		AWeaponInstance* WeaponInst = Cast<AWeaponInstance>(Actor);
		if (!WeaponInst) continue;

		FWeaponInstanceData Data;
		Data.ActorClassPath       = WeaponInst->GetClass()->GetPathName();
		Data.AttachSocket         = WeaponInst->AttachSocket;
		Data.Transform            = WeaponInst->AttachTransform;
		YogWeaponSaveSupport::CaptureLayer(WeaponInst->WeaponLayer, Data);
		SaveData(WeaponInst, Data.ByteData);
		SaveGame->WeaponInstanceItems.Add(Data);
	}
}

void UYogSaveSubsystem::LoadPlayer(UYogSaveGame* SaveGame)
{
	if (!SaveGame) return;
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* LocalPC = UGameplayStatics::GetPlayerController(World, 0);
	if (!LocalPC) return;

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(LocalPC->GetPawn());
	if (!Player || !Player->GetASC()) return;

	GiveAbilitiesFromSaveData(Player->GetASC(), SaveGame->PlayerStateData.Abilities);
	if (Player->ActiveSkillComponent && !SaveGame->SelectedSkillLoadout.IsEmpty())
	{
		TArray<UActiveSkillDataAsset*> Loadout;
		Loadout.Reserve(SaveGame->SelectedSkillLoadout.Num());
		for (const TSoftObjectPtr<UActiveSkillDataAsset>& SoftSkill : SaveGame->SelectedSkillLoadout)
		{
			Loadout.Add(SoftSkill.LoadSynchronous());
		}
		Player->ActiveSkillComponent->SetSkillLoadout(Loadout);
	}

	for (FWeaponInstanceData& WeaponData : SaveGame->WeaponInstanceItems)
	{
		UClass* WeaponClass = StaticLoadClass(AActor::StaticClass(), nullptr, *WeaponData.ActorClassPath);
		if (!WeaponClass || !WeaponClass->IsChildOf(AWeaponInstance::StaticClass())
			|| WeaponClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) continue;
		const AWeaponInstance* WeaponDefaults = WeaponClass->GetDefaultObject<AWeaponInstance>();

		FWeaponSpawnData SpawnData;
		SpawnData.WeaponLayer  = YogWeaponSaveSupport::ResolveLayer(WeaponData,
			WeaponDefaults ? WeaponDefaults->WeaponLayer : TSubclassOf<UYogAnimInstance>());
		SpawnData.ActorToSpawn = WeaponClass;
		SpawnData.AttachSocket = WeaponData.AttachSocket;
		SpawnData.AttachTransform  = WeaponData.Transform;
		SpawnData.bShouldSaveToGame = true;

		AWeaponInstance* WeaponActor = UYogBlueprintFunctionLibrary::SpawnWeaponOnCharacter(
			Player, Player->GetTransform(), SpawnData, false);
		if (WeaponActor)
		{
			LoadData(WeaponActor, WeaponData.ByteData);
			if (WeaponActor->WeaponLayer && Player->GetMesh() && Player->GetMesh()->GetAnimInstance())
			{
				Player->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(WeaponActor->WeaponLayer);
			}
		}
	}

	for (const auto& Pair : SaveGame->PlayerStateData.PlayerOwnedTags)
	{
		Player->GetASC()->AddGameplayTagWithCount(Pair.Key, Pair.Value);
	}
}

void UYogSaveSubsystem::LoadMap(UYogSaveGame* SaveGame)
{
}

void UYogSaveSubsystem::SaveMap(UYogSaveGame* SaveGame)
{
	if (UWorld* W = GetWorld())
	{
		SaveGame->MapStateData.LevelName = FName(UGameplayStatics::GetCurrentLevelName(W, true));
	}
}

FAbilitySaveData UYogSaveSubsystem::ConvertAbilitySpecToSaveData(const FGameplayAbilitySpec& Spec)
{
	FAbilitySaveData SaveData;
	if (Spec.Ability)
	{
		SaveData.AbilityClassPath = Spec.Ability->GetClass()->GetPathName();
		SaveData.AbilityClass     = Spec.Ability->GetClass();
	}
	SaveData.Level   = Spec.Level;
	SaveData.InputID = Spec.InputID;
	return SaveData;
}

FGameplayAbilitySpecHandle UYogSaveSubsystem::ConvertSaveDataToAbilitySpec(
	UYogAbilitySystemComponent* ASC, const FAbilitySaveData& SaveData)
{
	UClass* AbilityClass = SaveData.AbilityClassPath.TryLoadClass<UYogGameplayAbility>();
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Failed to load ability class: %s"),
			*SaveData.AbilityClassPath.ToString());
		return FGameplayAbilitySpecHandle();
	}
	FGameplayAbilitySpec Spec(AbilityClass, SaveData.Level, SaveData.InputID);
	return ASC->GiveAbility(Spec);
}

void UYogSaveSubsystem::GiveAbilitiesFromSaveData(UYogAbilitySystemComponent* ASC,
	const TArray<FAbilitySaveData>& AbilitiesData)
{
	for (const FAbilitySaveData& SaveData : AbilitiesData)
	{
		ConvertSaveDataToAbilitySpec(ASC, SaveData);
	}
}

// =========================================================
// 统计写入
// =========================================================

void UYogSaveSubsystem::RecordRunStarted()
{
	RunStartTime = FDateTime::Now();
	if (!CurrentSaveGame) return;
	CurrentSaveGame->Statistics.TotalRuns++;

	// 清零本局货币累计器（新局/续局均执行）
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogMetaProgressionSubsystem* Meta = GI->GetSubsystem<UYogMetaProgressionSubsystem>())
		{
			Meta->ClearRunCurrencyAccumulator();
		}
	}
}

void UYogSaveSubsystem::RecordEnemyKilled(int32 Count)
{
	if (!CurrentSaveGame || Count <= 0) return;
	CurrentSaveGame->Statistics.TotalKills += Count;
}

void UYogSaveSubsystem::RecordPlayerDeath()
{
	if (!CurrentSaveGame) return;
	CurrentSaveGame->Statistics.TotalDeaths++;

	const int32 Elapsed = FMath::FloorToInt((FDateTime::Now() - RunStartTime).GetTotalSeconds());
	if (Elapsed > 0)
	{
		CurrentSaveGame->Statistics.TotalPlayTimeSeconds += Elapsed;
	}

	DoAsyncSave();
}

void UYogSaveSubsystem::RecordGoldEarned(int32 Amount)
{
	if (!CurrentSaveGame || Amount <= 0) return;
	CurrentSaveGame->Statistics.TotalGoldEarned += Amount;
}

// =========================================================
// 存档版本迁移
// =========================================================

void UYogSaveSubsystem::MigrateSaveGame(UYogSaveGame* Save, int32 FromVersion, int32 ToVersion)
{
	if (!Save || FromVersion >= ToVersion) return;

	UE_LOG(LogTemp, Log, TEXT("[SaveMigration] Migrating slot %d: v%d → v%d"),
		CurrentSlotIndex, FromVersion, ToVersion);

	for (int32 V = FromVersion; V < ToVersion; ++V)
	{
		switch (V)
		{
		case 1:
			// v1 → v2: 预留，当前无破坏性变更
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("[SaveMigration] Unknown version step %d → %d; skipping"), V, V + 1);
			break;
		}
		Save->SaveFormatVersion = V + 1;
	}
}
