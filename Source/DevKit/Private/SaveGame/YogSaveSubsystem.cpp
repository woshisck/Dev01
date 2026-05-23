#include "SaveGame/YogSaveSubsystem.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"
#include "DevAssetManager.h"
#include "YogBlueprintFunctionLibrary.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Kismet/GameplayStatics.h"

static const int32 GNumSaveSlots    = 3;
static const int32 GSettingsUserIdx = 0;
static const FString GSettingsSlot  = TEXT("Settings");

// =========================================================
// 初始化
// =========================================================

void UYogSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}

	LoadSettings();

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

// =========================================================
// 多槽位管理
// =========================================================

FString UYogSaveSubsystem::GetSlotName(int32 SlotIndex) const
{
	return FString::Printf(TEXT("SaveSlot_%d"), FMath::Clamp(SlotIndex, 0, GNumSaveSlots - 1));
}

static const int32 GCurrentSaveFormatVersion = 1;

void UYogSaveSubsystem::SelectSlot(int32 SlotIndex)
{
	CurrentSlotIndex = FMath::Clamp(SlotIndex, 0, GNumSaveSlots - 1);
	const FString SlotName = GetSlotName(CurrentSlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}
	else
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		CurrentSaveGame->SlotCreatedTime = FDateTime::Now();
	}

	if (CurrentSaveGame && CurrentSaveGame->SaveFormatVersion < GCurrentSaveFormatVersion)
	{
		MigrateSaveGame(CurrentSaveGame, CurrentSaveGame->SaveFormatVersion, GCurrentSaveFormatVersion);
		DoAsyncSave();
	}

	if (CurrentSettings)
	{
		CurrentSettings->LastActiveSlot = CurrentSlotIndex;
		SaveSettings();
	}

	OnSaveGameLoaded.Broadcast(CurrentSaveGame);
}

void UYogSaveSubsystem::DeleteSlot(int32 SlotIndex)
{
	const FString SlotName = GetSlotName(SlotIndex);
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	if (SlotIndex == CurrentSlotIndex)
	{
		CurrentSaveGame = Cast<UYogSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
}

void UYogSaveSubsystem::ResetSlotForNewGame(int32 SlotIndex)
{
	SelectSlot(SlotIndex);

	// 保留 Statistics，清空其余局外数据和存档点
	CurrentSaveGame->MetaProgression = FMetaProgressionData{};
	CurrentSaveGame->SelectedSkillLoadout.Reset();
	CurrentSaveGame->RunCheckpoint   = FRunCheckpointData{};
	CurrentSaveGame->PlayerStateData = FPlayerGASData{};
	CurrentSaveGame->WeaponInstanceItems.Reset();
	CurrentSaveGame->MapStateData = FYogMapStateData{};
	CurrentSaveGame->SavedCharacter.Reset();
	CurrentSaveGame->TutorialState   = ETutorialState::NeedWeaponTutorial;
	CurrentSaveGame->ShownPopupKeys.Empty();
	CurrentSaveGame->StoryFlags.Empty();
	if (const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Active"), false);
		ActiveTag.IsValid())
	{
		CurrentSaveGame->StoryFlags.Add(ActiveTag, true);
	}
	if (const FGameplayTag CompletedTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Flag.FirstRunTutorial.Completed"), false);
		CompletedTag.IsValid())
	{
		CurrentSaveGame->StoryFlags.Remove(CompletedTag);
	}
	CurrentSaveGame->StoryFiredRuleIds.Empty();
	CurrentSaveGame->StoryQuestTasks.Empty();
	CurrentSaveGame->SlotCreatedTime  = FDateTime::Now();

	DoAsyncSave();
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

	DoAsyncSave();
}

void UYogSaveSubsystem::RequestSlotPreview(int32 SlotIndex, FOnSlotPreviewReady Callback)
{
	const FString SlotName = GetSlotName(SlotIndex);

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		FSlotPreviewData Empty;
		Callback.ExecuteIfBound(Empty);
		return;
	}

	// 异步加载，避免主线程卡顿
	FAsyncLoadGameFromSlotDelegate LoadDelegate;
	LoadDelegate.BindLambda([Callback](const FString&, const int32, USaveGame* LoadedGame)
	{
		FSlotPreviewData Preview;
		if (UYogSaveGame* Save = Cast<UYogSaveGame>(LoadedGame))
		{
			Preview.bHasData             = true;
			Preview.LastPlayTime         = Save->SlotLastPlayTime;
			Preview.HighestFloor         = Save->Statistics.HighestFloor;
			Preview.bHasPendingRun       = Save->RunCheckpoint.bIsValid; // 单一事实源
			Preview.TotalPlayTimeSeconds = Save->Statistics.TotalPlayTimeSeconds;
		}
		Callback.ExecuteIfBound(Preview);
	});

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
	Out.PlacedRunes                       = RS.PlacedRunes;
	Out.PendingRunes                      = RS.PendingRunes;
	Out.HiddenPassiveRuneInstances        = RS.HiddenPassiveRuneInstances;
	Out.SacrificeOfferingCosts            = RS.SacrificeOfferingCosts;
	Out.CombatDeckCardOrientations        = RS.CombatDeckCardOrientations;
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
	Out.ActiveSacrificeGrace = RS.ActiveSacrificeGrace.Get();

	Out.CombatDeckCards.Reset(RS.CombatDeckCards.Num());
	for (const TObjectPtr<URuneDataAsset>& Card : RS.CombatDeckCards)
	{
		Out.CombatDeckCards.Add(Card.Get());
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
	RS.PlacedRunes                       = In.PlacedRunes;
	RS.PendingRunes                      = In.PendingRunes;
	RS.HiddenPassiveRuneInstances        = In.HiddenPassiveRuneInstances;
	RS.SacrificeOfferingCosts            = In.SacrificeOfferingCosts;
	RS.CombatDeckCardOrientations        = In.CombatDeckCardOrientations;
	RS.SelectedSkillLoadout.Reset(In.SelectedSkillLoadout.Num());
	for (const TSoftObjectPtr<UActiveSkillDataAsset>& SoftSkill : In.SelectedSkillLoadout)
	{
		RS.SelectedSkillLoadout.Add(SoftSkill.LoadSynchronous());
	}

	// TSoftObjectPtr → 同步加载（这里只恢复指针；调用方可在之后 AsyncLoad）
	RS.EquippedWeaponDef  = In.EquippedWeaponDef.LoadSynchronous();
	RS.ActiveSacrificeGrace = In.ActiveSacrificeGrace.LoadSynchronous();

	RS.CombatDeckCards.Reset(In.CombatDeckCards.Num());
	for (const TSoftObjectPtr<URuneDataAsset>& SoftCard : In.CombatDeckCards)
	{
		RS.CombatDeckCards.Add(SoftCard.LoadSynchronous());
	}
}

// =========================================================
// 全局设置
// =========================================================

void UYogSaveSubsystem::SaveSettings()
{
	if (!CurrentSettings)
	{
		CurrentSettings = Cast<UYogSettingsSave>(
			UGameplayStatics::CreateSaveGameObject(UYogSettingsSave::StaticClass()));
	}
	UGameplayStatics::SaveGameToSlot(CurrentSettings, GSettingsSlot, GSettingsUserIdx);
}

void UYogSaveSubsystem::LoadSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(GSettingsSlot, GSettingsUserIdx))
	{
		CurrentSettings = Cast<UYogSettingsSave>(
			UGameplayStatics::LoadGameFromSlot(GSettingsSlot, GSettingsUserIdx));
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
	if (!CurrentSaveGame)
	{
		return;
	}

	if (bAsyncSavePending)
	{
		bAsyncSaveQueued = true;
		return;
	}

	bAsyncSavePending = true;

	const FString SlotName = GetSlotName(CurrentSlotIndex);
	FAsyncSaveGameToSlotDelegate SaveDelegate;
	SaveDelegate.BindUObject(this, &UYogSaveSubsystem::OnAsyncSaveComplete);

	UGameplayStatics::AsyncSaveGameToSlot(CurrentSaveGame, SlotName, 0, SaveDelegate);
}

void UYogSaveSubsystem::OnAsyncSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	bAsyncSavePending = false;

	if (bSuccess)
	{
		OnSaveGameWritten.Broadcast(CurrentSaveGame);
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Async save succeeded: %s"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Async save FAILED: %s"), *SlotName);
	}

	if (bAsyncSaveQueued)
	{
		bAsyncSaveQueued = false;
		DoAsyncSave();
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
	FMemoryWriter MemoryWriter(Data, true);
	FYogSaveGameArchive MyArchive(MemoryWriter);
	Object->Serialize(MyArchive);
}

void UYogSaveSubsystem::LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data)
{
	if (!Object) return;
	FMemoryReader MemoryReader(Data, true);
	FYogSaveGameArchive Ar(MemoryReader);
	Object->Serialize(Ar);
}

void UYogSaveSubsystem::SavePlayer(UYogSaveGame* SaveGame)
{
	SaveGame->WeaponInstanceItems.Empty();
	SaveGame->PlayerStateData.Abilities.Empty();

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player) return;

	SaveGame->PlayerStateData.SetupAttribute(*Player->BaseAttributeSet);
	SaveGame->PlayerStateData.WeaponAbilities =
		Player->GetCharacterDataComponent()->GetCharacterData()->AbilityData;

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
		Data.WeaponLayer          = WeaponInst->WeaponLayer->GetClass();
		Data.WeaponLayerClassPath = WeaponInst->WeaponLayer->GetClass()->GetPathName();
		SaveData(WeaponInst, Data.ByteData);
		SaveGame->WeaponInstanceItems.Add(Data);
	}
}

void UYogSaveSubsystem::LoadPlayer(UYogSaveGame* SaveGame)
{
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
		if (!WeaponClass) continue;

		UBlueprint* LayerBP = LoadObject<UBlueprint>(nullptr, *WeaponData.WeaponLayerClassPath);
		if (!LayerBP) continue;

		FWeaponSpawnData SpawnData;
		SpawnData.WeaponLayer  = LayerBP->GeneratedClass;
		SpawnData.ActorToSpawn = WeaponClass;
		SpawnData.AttachSocket = WeaponData.AttachSocket;
		SpawnData.AttachTransform  = WeaponData.Transform;
		SpawnData.bShouldSaveToGame = true;

		AWeaponInstance* WeaponActor = UYogBlueprintFunctionLibrary::SpawnWeaponOnCharacter(
			Player, Player->GetTransform(), SpawnData);
		if (WeaponActor)
		{
			LoadData(WeaponActor, WeaponData.ByteData);
		}
	}

	Player->GetCharacterDataComponent()->GetCharacterData()->AbilityData =
		SaveGame->PlayerStateData.WeaponAbilities;

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
