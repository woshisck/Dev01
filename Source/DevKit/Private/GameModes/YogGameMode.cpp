// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/YogGameMode.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/YogPlayerControllerBase.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/BackpackGridComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/LevelInfoPopupDA.h"
#include "Data/RoomDataAsset.h"
#include "Data/EnemyData.h"
#include <Kismet/GameplayStatics.h>
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "System/YogGameInstanceBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Mob/MobSpawner.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Data/RuneDataAsset.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Actors/BuffFlowLifecycleProxy.h"
#include "BuffFlow/LifecycleFlowAsset.h"
#include "Component/CharacterDataComponent.h"
#include "Map/Portal.h"
#include "Map/RewardPickup.h"
#include "Map/AltarActor.h"
#include "Map/ShopActor.h"
#include "UI/LootSelectionWidget.h"
#include "Tutorial/TutorialManager.h"
#include "UI/YogHUD.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryEventManager.h"
#include "Story/FirstRunTutorialDirectorSubsystem.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "GameFramework/PlayerController.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "World/HubFacilityActor.h"

namespace
{
constexpr const TCHAR* FirstRunForcedSurvivalEnemyDataPath = TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard.DA_RottenGuard");
constexpr const TCHAR* FirstRunRewardBurnRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn.DA_Rune512_Burn");
constexpr const TCHAR* FirstRunRewardPoisonRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Poison.DA_Rune512_Poison");
constexpr const TCHAR* FirstRunRewardKnockbackRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback.DA_Rune512_Knockback");
constexpr const TCHAR* FirstRunRewardSplashRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Splash.DA_Rune512_Splash");
constexpr const TCHAR* FirstRunGoldIconPath = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.T_GoldCoinIcon");
constexpr int32 FirstRunInitialGoldRewardAmount = 50;

UEnemyData* LoadFirstRunForcedSurvivalEnemyData()
{
	return LoadObject<UEnemyData>(nullptr, FirstRunForcedSurvivalEnemyDataPath);
}

FName ResolveRoomLevelNameForOpen(FName RequestedLevel, const URoomDataAsset* Room)
{
	const FString Requested = RequestedLevel.ToString();
	const FString RoomAssetName = GetNameSafe(Room);

	if (Requested.Equals(TEXT("L1_CommonLevel_Corridor_01b"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_Room_CL_corridor_01b"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_CL_Corridor_01b"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b"));
	}

	if (Requested.Equals(TEXT("PrayRoom"), ESearchCase::IgnoreCase)
		|| Requested.Equals(TEXT("L1_CommonLevel_PrayRoom"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom"));
	}

	if (Requested.Equals(TEXT("ShopRoom"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_Room_512_Shop"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom"));
	}

	return RequestedLevel;
}

FString DescribeGameModeEnumValueForRewardDebug(const UEnum* Enum, int64 Value)
{
	return Enum ? Enum->GetNameStringByValue(Value) : FString::Printf(TEXT("%lld"), Value);
}

FString DescribeGameModeLootOptionsForRewardDebug(const TArray<FLootOption>& Options)
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
			*DescribeGameModeEnumValueForRewardDebug(StaticEnum<ELootType>(), static_cast<int64>(Option.LootType)),
			Option.Amount,
			*Option.DisplayName.ToString(),
			*GetNameSafe(Option.RuneAsset.Get()),
			*GetNameSafe(Option.Icon.Get()),
			*Option.MetaCurrencyTag.ToString()));
	}

	return FString::Printf(TEXT("Count=%d [%s]"), Options.Num(), *FString::Join(Parts, TEXT("; ")));
}

void SealPortalsExcept(const TMap<int32, APortal*>& PortalMap, int32 OpenPortalIndex, const TCHAR* Context)
{
	for (const TPair<int32, APortal*>& Entry : PortalMap)
	{
		APortal* Portal = Entry.Value;
		if (!Portal || Entry.Key == OpenPortalIndex)
		{
			continue;
		}

		Portal->MarkUnavailable();
		UE_LOG(LogTemp, Log, TEXT("[%s] sealed portal [%d], forced portal [%d]."),
			Context ? Context : TEXT("PortalPlan"),
			Entry.Key,
			OpenPortalIndex);
	}
}

bool ResolveUsableForcedPortalIndex(
	const TArray<FPortalDestConfig>& Configs,
	const TMap<int32, APortal*>& PortalMap,
	int32& InOutForcedPortalIndex,
	const TCHAR* Context)
{
	for (const FPortalDestConfig& Cfg : Configs)
	{
		if (Cfg.PortalIndex == InOutForcedPortalIndex && PortalMap.Contains(Cfg.PortalIndex))
		{
			return true;
		}
	}

	for (const FPortalDestConfig& Cfg : Configs)
	{
		if (PortalMap.Contains(Cfg.PortalIndex))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("%s: forced portal [%d] is not available in this room; fallback to portal [%d]."),
				Context ? Context : TEXT("PortalPlan"),
				InOutForcedPortalIndex,
				Cfg.PortalIndex);
			InOutForcedPortalIndex = Cfg.PortalIndex;
			return true;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("%s: no usable portal exists for forced single-portal room."),
		Context ? Context : TEXT("PortalPlan"));
	return false;
}

bool PlayerHasEquippedWeapon(const UWorld* World)
{
	const APlayerCharacterBase* Player = World
		? Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(World, 0))
		: nullptr;
	return Player && (Player->EquippedWeaponDef || Player->EquippedWeaponInstance);
}

FLootOption MakeFirstRunGoldLootOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Gold;
	Option.Amount = FMath::Max(1, Amount);
	Option.DisplayName = FText::Format(NSLOCTEXT("FirstRunTutorial", "InitialGoldRewardFmt", "Gold x{0}"), Option.Amount);
	Option.Icon = LoadObject<UTexture2D>(nullptr, FirstRunGoldIconPath);
	return Option;
}

void ShowFirstRunWorldRewindHint(UObject* Outer, APlayerController* PC)
{
	if (!Outer || !PC)
	{
		return;
	}

	AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD());
	if (!HUD)
	{
		return;
	}

	ULevelInfoPopupDA* Popup = NewObject<ULevelInfoPopupDA>(Outer);
	Popup->Title = FText::GetEmpty();
	Popup->Body = NSLOCTEXT("FirstRunTutorial", "WorldRewindHintBody", "世界回溯");
	Popup->HUDSummaryText = Popup->Body;
	Popup->DisplayDuration = 3.0f;
	HUD->ShowInfoPopup(Popup);
}
}

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
	DefaultPawnClass = nullptr;

	// 事件总线统一使用的 FlowComponent，同时只跑一个 Flow（新触发停旧的）
	LifecycleFlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("LifecycleFlowComponent"));
	SingleChoiceLootGameplayTag = FGameplayTag::RequestGameplayTag(TEXT("Loot.Reward.SingleChoice"), false);
}

bool AYogGameMode::ShouldSkipCombatForRoom(const URoomDataAsset* RoomData)
{
	return RoomData
		&& RoomData->SacrificeEventAltarData != nullptr
		&& RoomData->EnemyPool.IsEmpty();
}

bool AYogGameMode::ShouldPreserveCurrentMapForEditorPlay(bool bIsPlayInEditorWorld, bool bHasPendingRoomData)
{
	return bIsPlayInEditorWorld && !bHasPendingRoomData;
}

bool AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
	const URoomDataAsset* RoomData,
	bool bIsSacrificeEventRoom,
	bool bHasRewardPickupClass,
	bool bEnableExtraRewardPickups)
{
	const bool bIsHubRoom = RoomData && RoomData->bIsHubRoom;
	return bEnableExtraRewardPickups
		&& !bIsHubRoom
		&& !bIsSacrificeEventRoom
		&& bHasRewardPickupClass;
}

bool AYogGameMode::HasActiveStoryEventTag(FGameplayTag EventTag) const
{
	return EventTag.IsValid() && ActiveStoryEventTags.HasTag(EventTag);
}

UCampaignDataAsset* AYogGameMode::GetActiveCampaignData() const
{
	return ActiveCampaignData ? ActiveCampaignData.Get() : CampaignData.Get();
}

void AYogGameMode::ResolveActiveCampaignData()
{
	UCampaignDataAsset* ResolvedCampaign = CampaignData;
	const TCHAR* Source = TEXT("CampaignData");

	if (UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr)
	{
		if (SaveSys->IsFirstRunTutorialActive() && FirstRunTutorialCampaignData)
		{
			ResolvedCampaign = FirstRunTutorialCampaignData;
			Source = TEXT("FirstRunTutorialCampaignData");
		}
	}

	if (const UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		if (GI->HasCampaignOverride())
		{
			ResolvedCampaign = GI->GetCampaignOverrideData();
			Source = TEXT("GameInstance.CampaignOverrideData");
		}
	}

	ActiveCampaignData = ResolvedCampaign;

	UE_LOG(LogTemp, Log, TEXT("[Campaign] Active=%s Source=%s Main=%s FirstRun=%s"),
		*GetNameSafe(ActiveCampaignData),
		Source,
		*GetNameSafe(CampaignData),
		*GetNameSafe(FirstRunTutorialCampaignData));
}


void AYogGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (GI && GI->PersistentSaveData && GI->PersistentSaveData->SavedCharacterClass)
	{
		// 使用 PlayerStart 作为出生位置，避免在 (0,0,0) 发生碰撞
		AActor* StartSpot = FindPlayerStart(NewPlayer);
		const FVector  SpawnLoc = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
		const FRotator SpawnRot = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;

		FActorSpawnParameters Params;
		Params.Owner = NewPlayer;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APlayerCharacterBase* LoadedChar = GetWorld()->SpawnActor<APlayerCharacterBase>(
			GI->PersistentSaveData->SavedCharacterClass,
			SpawnLoc, SpawnRot,
			Params
		);

		if (!LoadedChar)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleStartingNewPlayer: SpawnActor failed, falling back to default"));
			Super::HandleStartingNewPlayer_Implementation(NewPlayer);
			return;
		}

		NewPlayer->Possess(LoadedChar);

		// Possess 完成后 ASC 已初始化，可以安全恢复跑局状态
		LoadedChar->RestoreRunStateFromGI();
	}
	else
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer); // fallback
	}
}



void AYogGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!bAutoSpawnPlayer)
	{
		// Do nothing - prevent auto spawning
		UE_LOG(LogTemp, Warning, TEXT("Auto player spawning disabled"));
		return;
	}

	// Fall back to default behavior if enabled
	Super::RestartPlayer(NewPlayer);

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

}

void AYogGameMode::StartPlay()
{
	Super::StartPlay();

	ResolveActiveCampaignData();

	if (GetActiveCampaignData())
	{
		StartLevelSpawning();
	}
	else
	{
		FallbackToPreplacedEnemies();
	}
}

void AYogGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

}

void AYogGameMode::SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName)
{
	//YogSpawnPoint_0
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	//TODO: NEED TO CHANGE IN THE FUTURE DEV
	APlayerController* PlayerController = World->GetFirstPlayerController();


	AActor* PlayerStarter = FindPlayerStart(PlayerController, TEXT("YogSpawnPoint_0"));
	if (PlayerStarter)
	{
		if (PlayerController)
		{
			//if (APawn* ExistingPawn = PlayerController->GetPawn())
			//{
			//	ExistingPawn->Destroy();
			//}


			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APlayerCharacterBase* NewCharacter = World->SpawnActor<APlayerCharacterBase>
			(
				DefaultPawnClass,
				PlayerStarter->GetActorLocation(),
				PlayerStarter->GetActorRotation(),
				SpawnParams
			);
			NewCharacter = player;

			if (NewCharacter)
			{
				
				PlayerController->Possess(Cast<APawn>(NewCharacter));
				UE_LOG(LogTemp, Log, TEXT("Player spawned manually at specified location"));
			}

		}
	}

}


//TArray<AActor*> OutActors
//TArray<AMobSpawner*> Spawners;
//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);






///////////////////////////////  AI  ////////////////////////////////
void AYogGameMode::StartSpawnTimer()
{
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AYogGameMode::SpawnMob, SpawnConfig.Interval, true, SpawnConfig.FirstDelay);

}

void AYogGameMode::SpawnMob()
{
	//Spawn Algo:
	UE_LOG(LogTemp, Warning, TEXT("SpawnMob called at %f"), GetWorld()->GetTimeSeconds());

	TArray<AActor*> OutActors;
	TArray<AMobSpawner*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);

	if (OutActors.Num() <= 0)
	{
		return;
	}

	for (AActor* a : OutActors)
	{
		Spawners.Add(Cast<AMobSpawner>(a));
	}

	//const int32 TotalMobToSpawn = 15; 


	//for (int32 i = 0; i < TotalMobToSpawn; i++)
	for (int32 i = 0; i < SpawnConfig.MaxCall; i++)
	{
		int32 RandomIndex = FMath::RandRange(0, Spawners.Num() - 1);
		AMobSpawner* ChosenSpawner = Spawners[RandomIndex];

		if (ChosenSpawner)
		{
			ChosenSpawner->SpawnMob(SpawnConfig.MobClass); // Assuming AMobSpawner has SpawnMob()
			UE_LOG(LogTemp, Warning, TEXT("Mob %d spawned at spawner %d"), i + 1, RandomIndex);
		}
	}


	Current_CallCount++; 
	if (Current_CallCount >= SpawnConfig.MaxCall)
	{ 
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

}


void AYogGameMode::TriggerImmediateSpawn()
{
	// Clear the current timer
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	// Calculate how much time has passed since the last spawn
	float TimeSinceLastSpawn = GetWorld()->GetTimerManager().GetTimerElapsed(SpawnTimerHandle);
	float TimeRemaining = SpawnConfig.Interval - TimeSinceLastSpawn;

	// Call spawn immediately
	SpawnMob();

	// Restart the timer with the remaining time
	if (Current_CallCount < SpawnConfig.MaxCall)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AYogGameMode::SpawnMob,
			SpawnConfig.Interval,
			true,
			FMath::Max(TimeRemaining, 0.1f) // Ensure there's at least a small delay
		);
	}
}


///////////////////////////////  AI  ////////////////////////////////


void AYogGameMode::SomeEventThatTriggersImmediateSpawn()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AYogGameMode::TriggerImmediateSpawn);

}

 
void AYogGameMode::UpdateFinishLevel(int count)
{
	this->MonsterKillCount += count;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SS = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			SS->RecordEnemyKilled(count);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("MonsterKillCount: %d"), this->MonsterKillCount);

	// ---- 新刷怪系统 ----
	if (GetActiveCampaignData())
	{
		// 全局存活数递减
		TotalAliveEnemies = FMath::Max(0, TotalAliveEnemies - count);

		// 记录当前波次击杀数（用于百分比触发条件）
		if (WavePlans.IsValidIndex(CurrentWaveIndex))
		{
			WavePlans[CurrentWaveIndex].TotalKilledInWave += count;
		}

		// 场内已无存活敌人，但本波仍在分批刷（OneByOneTimer 等待中）→ 立即加速
		if (TotalAliveEnemies == 0 && !bAllWavesSpawned
			&& OneByOneSpawnIndex < OneByOneSpawnQueue.Num())
		{
			FTimerManager& TM = GetWorld()->GetTimerManager();
			if (TM.IsTimerActive(OneByOneTimer))
			{
				const float Remaining = TM.GetTimerRemaining(OneByOneTimer);
				if (Remaining > 0.5f)
				{
					TM.ClearTimer(OneByOneTimer);
					TM.SetTimer(OneByOneTimer, this, &AYogGameMode::SpawnNextOneByOne, 0.3f, false);
					UE_LOG(LogTemp, Log, TEXT("UpdateFinishLevel: 场内无敌人，加速下一只刷出（原剩余 %.1fs → 0.3s）"), Remaining);
				}
			}
		}

		CheckWaveTrigger();
		CheckLevelComplete();
		return;
	}

	// ---- 旧系统兜底（未配置 CampaignData 时）----
	if (this->MonsterKillCount >= RemainKillCount)
	{
		OnFinishLevel.Broadcast();
		FinishLevelEvent.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnFinishLevel.Broadcast() Calling;"));
		EnterArrangementPhase();
	}
}

void AYogGameMode::BeginPlay()
{
	Super::BeginPlay();

	// HUD 在 PC->ClientRestart 才创建（晚于 BeginPlay），用延帧+重试方式订阅
	TryBindHUDDelegates();

	// 触发游戏开始事件（一次性，跨 PIE 重启会重置 — 单机 Roguelite 行为可接受）
	TriggerLifecycleEvent(EGameLifecycleEvent::GameStart);
}

void AYogGameMode::TryBindHUDDelegates()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->OnLevelEndEffectFinished.AddUObject(this, &AYogGameMode::HandleLevelEndEffectFinished);
			UE_LOG(LogTemp, Log, TEXT("[LifecycleEvents] 已订阅 HUD->OnLevelEndEffectFinished"));
			return;
		}
	}

	// HUD 还没创建，最多重试 ~1s（每帧一次）
	if (HUDBindRetryCount++ < 60)
	{
		FTimerHandle TmpHandle;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AYogGameMode::TryBindHUDDelegates);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LifecycleEvents] 60 帧内未拿到 AYogHUD，放弃订阅 OnLevelEndEffectFinished — LevelClearRevealed 事件将不会触发"));
	}
}

// =========================================================
// 关卡流程
// =========================================================

void AYogGameMode::SetRoomRewardOptionsOverride(const TArray<FLootOption>& InOptions)
{
	RoomRewardOptionsOverride = InOptions;
	bHasRoomRewardOptionsOverride = true;

	UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GM SetRoomRewardOptionsOverride Options=%s"),
		*DescribeGameModeLootOptionsForRewardDebug(RoomRewardOptionsOverride));
}

void AYogGameMode::ClearRoomRewardOptionsOverride()
{
	if (bHasRoomRewardOptionsOverride || !RoomRewardOptionsOverride.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GM ClearRoomRewardOptionsOverride OldOptions=%s"),
			*DescribeGameModeLootOptionsForRewardDebug(RoomRewardOptionsOverride));
	}

	bHasRoomRewardOptionsOverride = false;
	RoomRewardOptionsOverride.Reset();
}

bool AYogGameMode::ApplyPendingRoomRewardOptionsOverride(UYogGameInstanceBase* GameInstance)
{
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GM ApplyPendingRoomRewardOptionsOverride skipped: GameInstance is null."));
		return false;
	}

	TArray<FLootOption> PendingOptions;
	if (!GameInstance->ConsumePendingRoomRewardOptionsOverride(PendingOptions))
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GM ApplyPendingRoomRewardOptionsOverride found no pending override."));
		return false;
	}

	SetRoomRewardOptionsOverride(PendingOptions);
	UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] GM ApplyPendingRoomRewardOptionsOverride applied Options=%s"),
		*DescribeGameModeLootOptionsForRewardDebug(PendingOptions));
	return true;
}

bool AYogGameMode::ApplyPendingRoomRewardOptionsOverrideForRoom(
	UYogGameInstanceBase* GameInstance,
	const URoomDataAsset* RoomData)
{
	if (RoomData && RoomData->bIsHubRoom)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[StoryRewardDebug] GM ApplyPendingRoomRewardOptionsOverrideForRoom deferred for hub room Room=%s GI=%s"),
			*GetNameSafe(RoomData),
			*GetNameSafe(GameInstance));
		return false;
	}

	return ApplyPendingRoomRewardOptionsOverride(GameInstance);
}

void AYogGameMode::SetForcedPortalOverride(int32 PortalIndex)
{
	ForcedPortalOverrideIndex = PortalIndex;
	bHasForcedPortalOverride = true;

	UE_LOG(LogTemp, Log, TEXT("[StoryOverride] Forced portal override set. PortalIndex=%d"),
		ForcedPortalOverrideIndex);
}

void AYogGameMode::ClearForcedPortalOverride()
{
	if (bHasForcedPortalOverride)
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryOverride] Forced portal override cleared."));
	}

	bHasForcedPortalOverride = false;
	ForcedPortalOverrideIndex = 0;
}

void AYogGameMode::ApplyStoryNextRoomPlanForCurrentRoom(const FStoryNextRoomPlan& Plan)
{
	if (Plan.bOverrideRewardOptions)
	{
		SetRoomRewardOptionsOverride(Plan.RewardOptionsOverride);
		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			GI->ClearPendingRoomRewardOptionsOverride();
		}
	}

	bSuppressRoomClearRewardPickup = Plan.bSuppressRoomClearRewardPickup;
	bStorySpecialRewardEnemyEnabled = Plan.bMarkLastEnemyAsSpecialRewardEnemy;
	StorySpecialRewardEnemyLootOptions = Plan.SpecialRewardEnemyLootOptions;
	StorySpecialRewardEnemyAuraFX = Plan.SpecialRewardEnemyAuraFX;

	UE_LOG(LogTemp, Log,
		TEXT("[FirstRunTutorialDirector] Applied story plan to current room. RewardOverride=%d RewardCount=%d SuppressClearReward=%d SpecialEnemy=%d SpecialLootCount=%d"),
		Plan.bOverrideRewardOptions ? 1 : 0,
		Plan.RewardOptionsOverride.Num(),
		bSuppressRoomClearRewardPickup ? 1 : 0,
		bStorySpecialRewardEnemyEnabled ? 1 : 0,
		StorySpecialRewardEnemyLootOptions.Num());
}

void AYogGameMode::EnterArrangementPhase()
{
	if (CurrentPhase != ELevelPhase::Combat)
		return;

	GetWorldTimerManager().ClearTimer(TimedClearObjectiveTimer);

	CurrentPhase = ELevelPhase::Arrangement;
	OnPhaseChanged.Broadcast(CurrentPhase);

	bool bRefreshTemporaryFinisherLockView = false;
	if (bCountCombatClearsForTemporaryFinisherUnlock)
	{
		++CompletedCombatBattleCount;
		bRefreshTemporaryFinisherLockView = true;
		UE_LOG(LogTemp, Log, TEXT("[TemporaryFinisher] Completed combat battles: %d"), CompletedCombatBattleCount);
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	const bool bUseFirstRunInitialSplitReward = ShouldSpawnFirstRunInitialCardReward();

	if (Player)
	{
		// 解锁背包
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Backpack->SetLocked(false);
		}

		// 发放金币（按当前关卡 FloorConfig 中的范围）
		if (GetActiveCampaignData() && ActiveGoldMax > 0 && !bHasRoomRewardOptionsOverride && !bUseFirstRunInitialSplitReward)
		{
			const int32 GoldReward = FMath::RandRange(ActiveGoldMin, ActiveGoldMax);
			if (Player->BackpackGridComponent)
				Player->BackpackGridComponent->AddGold(GoldReward);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 发放金币 %d"), GoldReward);
		}
	}

	// 发放局外成长货币（不依赖 Player，独立发放）
	if (ActiveRoomData && !ActiveRoomData->MetaCurrencyRewards.IsEmpty())
	{
		if (UYogMetaProgressionSubsystem* Meta = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr)
		{
			for (const FMetaCurrencyCost& Reward : ActiveRoomData->MetaCurrencyRewards)
			{
				if (Reward.Amount > 0)
				{
					Meta->AddCurrency(Reward.CurrencyTag, Reward.Amount);
				}
			}
		}
	}

	// 重置本关已分配符文的追踪集合（多拾取物去重用）
	if (Player && bRefreshTemporaryFinisherLockView && Player->CombatDeckComponent)
	{
		Player->CombatDeckComponent->RefreshDeckView();
	}

	LootAssignedThisLevel.Empty();

	// Loot 落点：优先玩家前方，被墙阻挡/角落时自动选相机可见方向
	FVector LootSpawnLoc = LastEnemyKillLocation;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (APawn* P = PC->GetPawn())
			LootSpawnLoc = FindLootSpawnLocation(P, PC);

	// 关卡结束视觉特效（时间膨胀 + 变黑 + 圆形揭幕），传入 Loot 位置供圆心定位
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->TriggerLevelEndEffect(LootSpawnLoc);

	// 关卡完成事件（用户可在 BP_GameMode_Default 配 LFA 做"金币结算 UI"等表演，
	// 教程类弹窗建议挂到 LevelClearRevealed 等揭幕完成后再触发，避免与黑屏过渡同帧冲突）
	TriggerLifecycleEvent(EGameLifecycleEvent::LevelClear);

	// 生成奖励拾取物
	if (RewardPickupClass && !bSuppressRoomClearRewardPickup)
	{
		FVector SpawnLoc = LootSpawnLoc;
		if (!SpawnLoc.IsZero())
		{
			AActor* Spawned = GetWorld()->SpawnActor<AActor>(RewardPickupClass, SpawnLoc, FRotator::ZeroRotator);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 生成 RewardPickup @ %s"), *SpawnLoc.ToString());

			// 预分配战利品选项，拾取时直接展示，无需再次生成
			if (ARewardPickup* Pickup = Cast<ARewardPickup>(Spawned))
			{
				TArray<FLootOption> Batch;
				const TCHAR* RewardSource = TEXT("Generated");
				if (bHasRoomRewardOptionsOverride)
				{
					Batch = RoomRewardOptionsOverride;
					RewardSource = TEXT("RoomRewardOverride");
				}
				else if (bUseFirstRunInitialSplitReward)
				{
					Batch.Add(MakeFirstRunGoldLootOption(FirstRunInitialGoldRewardAmount));
					RewardSource = TEXT("FirstRunInitialGold");
				}
				else
				{
					Batch = (ActiveRoomData && ActiveRoomData->bUseFixedRewardOptions)
						? ActiveRoomData->FixedRewardOptions
						: GenerateLootBatch(LootAssignedThisLevel);
					RewardSource = (ActiveRoomData && ActiveRoomData->bUseFixedRewardOptions)
						? TEXT("RoomDataFixedRewardOptions")
						: TEXT("GenerateLootBatch");
				}
				UE_LOG(LogTemp, Log,
					TEXT("[StoryRewardDebug] GM EnterArrangementPhase reward batch before limit Source=%s HasOverride=%d ActiveRoom=%s Options=%s"),
					RewardSource,
					bHasRoomRewardOptionsOverride ? 1 : 0,
					*GetNameSafe(ActiveRoomData),
					*DescribeGameModeLootOptionsForRewardDebug(Batch));
				Batch = ApplyLootOptionLimit(Batch);
				UE_LOG(LogTemp, Log,
					TEXT("[StoryRewardDebug] GM EnterArrangementPhase assigning RewardPickup=%s Options=%s"),
					*GetNameSafe(Pickup),
					*DescribeGameModeLootOptionsForRewardDebug(Batch));
				Pickup->AssignLoot(Batch);
				Pickup->PlaySpawnFocusCue();
				UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: assigned %d loot option(s) to RewardPickup"), Batch.Num());
			}
		}
	}
	else if (bSuppressRoomClearRewardPickup)
	{
		UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Room clear RewardPickup suppressed by story plan."));
	}

	SpawnFirstRunInitialCardRewardPickup(LootSpawnLoc);

	// Legacy extra reward pickup roll. Disabled by default while rooms should emit one pickup.
	SpawnSacrificeEventAltar(LootSpawnLoc);

	const bool bIsHubRoom = ActiveRoomData && ActiveRoomData->bIsHubRoom;
	const bool bIsSacrificeRoom = IsSacrificeEventRoom();
	if (ShouldAllowExtraRewardPickupForRoom(
			ActiveRoomData,
			bIsSacrificeRoom,
			RewardPickupClass != nullptr,
			bEnableExtraRewardPickups)
		&& FMath::FRand() < SacrificeDropChance)
	{
		TArray<FLootOption> ExtraBatch = ApplyLootOptionLimit(GenerateLootBatch(LootAssignedThisLevel));
		if (ExtraBatch.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("EnterArrangementPhase: skipped extra RewardPickup because no extra rune options were available."));
		}
		else
		{
			// Spawn an extra rune reward beside the normal reward.
			FVector ExtraRewardSpawnLoc = LootSpawnLoc + FVector(250.f, 0.f, 0.f);

			ARewardPickup* ExtraPickup = GetWorld()->SpawnActor<ARewardPickup>(
				RewardPickupClass, ExtraRewardSpawnLoc, FRotator::ZeroRotator);
			if (ExtraPickup)
			{
				ExtraPickup->AssignLoot(ExtraBatch);
				ExtraPickup->PlaySpawnFocusCue();
				UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: spawned extra RewardPickup with %d rune options @ %s"),
					ExtraBatch.Num(), *ExtraRewardSpawnLoc.ToString());
			}
		}
	}

	if (Player)
	{
		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			FRunState NewState;
			NewState.bIsValid = true;
			NewState.CurrentGold = Player->BackpackGridComponent ? Player->BackpackGridComponent->Gold : 0;

			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				NewState.CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
			}

			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				NewState.CurrentPhase = Backpack->GetCurrentPhase();
				for (const FPlacedRune& PR : Backpack->GetAllPlacedRunes())
				{
					if (!PR.bIsPermanent)
					{
						NewState.PlacedRunes.Add(PR);
					}
				}
				NewState.HiddenPassiveRuneInstances = Backpack->GetRuntimeHiddenPassiveRunes();
			}

			NewState.EquippedWeaponDef = Player->EquippedWeaponDef;
			NewState.PendingRunes = Player->PendingRunes;
			if (UCombatDeckComponent* CombatDeck = Player->CombatDeckComponent)
			{
				for (const FCombatCardInstance& Card : CombatDeck->GetFullDeckSnapshot())
				{
					if (Card.SourceData)
					{
						NewState.CombatDeckCards.Add(Card.SourceData);
						NewState.CombatDeckCardOrientations.Add(Card.LinkOrientation);
					}
				}
				NewState.CombatDeckShuffleCooldownDuration = CombatDeck->GetShuffleCooldownDuration();
				NewState.CombatDeckMaxActiveSequenceSize = CombatDeck->GetMaxActiveSequenceSize();
			}
			NewState.CompletedCombatBattleCount = CompletedCombatBattleCount;
			NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;
			NewState.SacrificeOfferingCosts = Player->GetSacrificeOfferingCosts();
			if (Player->ActiveSkillComponent)
			{
				for (UActiveSkillDataAsset* Skill : Player->ActiveSkillComponent->GetSkillLoadout())
				{
					NewState.SelectedSkillLoadout.Add(Skill);
				}
			}
			GI->PendingRunState = NewState;
		}
	}

	if (UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>())
	{
		SaveSys->TriggerCheckpoint(CurrentFloor);
	}

	// 开启传送门
	if (UFirstRunTutorialDirectorSubsystem* Director = GetGameInstance()->GetSubsystem<UFirstRunTutorialDirectorSubsystem>())
	{
		Director->HandleArrangementPhase(this);
	}

	ActivatePortals();

	// v3：启用 HUD 传送门引导（单例浮窗 + 屏幕边缘方位箭头）。主城（HubRoom）跳过
	if (!bIsHubRoom)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->ShowPortalGuidance();
			}
		}
	}
}

void AYogGameMode::SelectLoot(int32 LootIndex)
{
	if (!CurrentLootOptions.IsValidIndex(LootIndex))
		return;

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player)
		return;

	bLootOptionsPending = false;

	const FLootOption& Chosen = CurrentLootOptions[LootIndex];
	bool bAddedCombatCardToDeck = false;
	if (Chosen.LootType == ELootType::Rune && Chosen.RuneAsset)
	{
		Player->AddRuneToInventory(Chosen.RuneAsset->CreateInstance());
		if (Player->CombatDeckComponent)
		{
			bAddedCombatCardToDeck = Player->CombatDeckComponent->AddCardFromRuneReward(Chosen.RuneAsset);
		}
	}
	else if (Chosen.LootType == ELootType::Gold)
	{
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Backpack->AddGold(FMath::Max(0, Chosen.Amount));
		}
	}
	else if (Chosen.LootType == ELootType::Material)
	{
		const FGameplayTag MaterialCurrencyTag = ARewardPickup::ResolveMaterialCurrencyTag(Chosen.MetaCurrencyTag);
		if (MaterialCurrencyTag.IsValid())
		{
			if (UYogMetaProgressionSubsystem* Meta = UGameInstance::GetSubsystem<UYogMetaProgressionSubsystem>(GetGameInstance()))
			{
				Meta->AddCurrency(MaterialCurrencyTag, FMath::Max(0, Chosen.Amount));
			}
		}
	}

	// 512 reward-card tutorial: only trigger when the reward actually entered the combat deck.
	if (bAddedCombatCardToDeck)
	{
		if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			const FGameplayTag RuneItemTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Item.Rune"), false);
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Item.Obtained"), false),
				RuneItemTag,
				ActiveGlobalStageTag,
				RuneItemTag,
				Player,
				PC);
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.FirstRuneObtained"), false),
				RuneItemTag,
				ActiveGlobalStageTag,
				RuneItemTag,
				Player,
				PC);
		}

		if (UFirstRunTutorialDirectorSubsystem* Director = GetGameInstance()->GetSubsystem<UFirstRunTutorialDirectorSubsystem>())
		{
			Director->HandleRewardRuneAdded(Chosen.RuneAsset, Player);
		}
	}

	// 通知 LevelFlow 节点：玩家已选符文
	OnLootSelected.Broadcast();
}

void AYogGameMode::HandleTimedClearObjectiveExpired()
{
	if (!bTimedClearObjectiveActive || CurrentPhase != ELevelPhase::Combat)
	{
		return;
	}

	bTimedClearObjectiveExpired = true;
	UE_LOG(LogTemp, Warning, TEXT("[RoomEvent] Timed clear objective expired: Room=%s"),
		*GetNameSafe(ActiveRoomData));
}

bool AYogGameMode::IsSacrificeEventRoom() const
{
	return ActiveRoomData && ActiveRoomData->SacrificeEventAltarData != nullptr;
}

bool AYogGameMode::IsShopRoom() const
{
	if (!ActiveRoomData)
	{
		return false;
	}

	if (ActiveRoomData->ShopData)
	{
		return true;
	}

	const FGameplayTag ShopTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Shop"), false);
	return ShopTag.IsValid() && ActiveRoomData->RoomTags.HasTagExact(ShopTag);
}

void AYogGameMode::SpawnShopActorForRoom()
{
	if (!IsShopRoom() || !ActiveRoomData || !ActiveRoomData->ShopData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(World, 0));
	auto ConfigureShop = [this, Player](AShopActor* Shop)
	{
		if (!Shop || !ActiveRoomData)
		{
			return;
		}

		Shop->SetShopData(ActiveRoomData->ShopData);
		Shop->SetShopWidgetClass(ActiveRoomData->ShopWidgetClass);

		if (Player)
		{
			constexpr float ImmediateInteractRadiusSq = 320.0f * 320.0f;
			if (FVector::DistSquared(Player->GetActorLocation(), Shop->GetActorLocation()) <= ImmediateInteractRadiusSq)
			{
				Player->PendingShop = Shop;
			}
		}
	};

	TArray<AActor*> ExistingShopActors;
	UGameplayStatics::GetAllActorsOfClass(World, AShopActor::StaticClass(), ExistingShopActors);
	if (!ExistingShopActors.IsEmpty())
	{
		for (AActor* Actor : ExistingShopActors)
		{
			ConfigureShop(Cast<AShopActor>(Actor));
		}
		UE_LOG(LogTemp, Log, TEXT("[ShopRoom] Configured %d preplaced shop actor(s): Room=%s"),
			ExistingShopActors.Num(),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	TSubclassOf<AShopActor> ShopClass = ActiveRoomData->ShopActorClass;
	if (!ShopClass)
	{
		ShopClass = AShopActor::StaticClass();
	}

	FVector SpawnLoc = ActiveRoomData->ShopSpawnOffset;
	FRotator SpawnRot = FRotator::ZeroRotator;
	if (Player)
	{
		SpawnLoc = Player->GetActorLocation() + Player->GetActorRotation().RotateVector(ActiveRoomData->ShopSpawnOffset);
		SpawnRot = Player->GetActorRotation();
	}

	AShopActor* Shop = World->SpawnActor<AShopActor>(ShopClass, SpawnLoc, SpawnRot);
	if (!Shop)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShopRoom] Failed to spawn shop actor: Room=%s"),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	ConfigureShop(Shop);
	UE_LOG(LogTemp, Log, TEXT("[ShopRoom] Shop actor spawned: Room=%s Shop=%s Location=%s"),
		*GetNameSafe(ActiveRoomData),
		*GetNameSafe(Shop),
		*SpawnLoc.ToCompactString());
}

void AYogGameMode::SpawnSacrificeEventAltar(const FVector& LootSpawnLoc)
{
	if (!IsSacrificeEventRoom())
	{
		return;
	}

	if (bTimedClearObjectiveActive && bTimedClearObjectiveExpired)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoomEvent] Sacrifice altar skipped because timed clear failed: Room=%s"),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<AAltarActor> AltarClass = ActiveRoomData->SacrificeEventAltarClass;
	if (!AltarClass)
	{
		AltarClass = AAltarActor::StaticClass();
	}

	auto ConfigureSacrificeAltar = [this](AAltarActor* Altar)
	{
		if (!Altar || !ActiveRoomData)
		{
			return;
		}

		Altar->SetAltarData(ActiveRoomData->SacrificeEventAltarData);
		Altar->SetSacrificeWidgetClass(ActiveRoomData->SacrificeEventWidgetClass);
		Altar->SetOpenSacrificeDirectly(true);
		Altar->EnsureInteractBoxMinimumExtent(FVector(240.f, 240.f, 180.f));

		if (UGameInstance* GI = GetGameInstance())
		{
			const UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>();
			if (SaveSys && SaveSys->IsFirstRunTutorialActive())
			{
				if (UFirstRunTutorialDirectorSubsystem* Director = GI->GetSubsystem<UFirstRunTutorialDirectorSubsystem>())
				{
					Director->SetStage(EFirstRunTutorialStage::PrayerRoom);
					UE_LOG(LogTemp, Warning,
						TEXT("[AltarInteractDebug] First-run sacrifice altar configured as PrayerRoom. Room=%s Altar=%s FinisherRune=%s"),
						*GetNameSafe(ActiveRoomData),
						*GetNameSafe(Altar),
						*GetNameSafe(UFirstRunTutorialDirectorSubsystem::LoadFirstRunFinisherRune()));
				}
			}
		}

		Altar->SetAltarActive(true);

	};

	TArray<AActor*> ExistingAltarActors;
	UGameplayStatics::GetAllActorsOfClass(World, AAltarActor::StaticClass(), ExistingAltarActors);
	int32 ConfiguredPreplacedCount = 0;
	for (AActor* Actor : ExistingAltarActors)
	{
		AAltarActor* ExistingAltar = Cast<AAltarActor>(Actor);
		if (!ExistingAltar || !ExistingAltar->IsA(AltarClass))
		{
			continue;
		}

		ConfigureSacrificeAltar(ExistingAltar);
		++ConfiguredPreplacedCount;
	}

	if (ConfiguredPreplacedCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[RoomEvent] Configured %d preplaced sacrifice altar actor(s): Room=%s Class=%s"),
			ConfiguredPreplacedCount,
			*GetNameSafe(ActiveRoomData),
			*GetNameSafe(AltarClass));
		return;
	}

	const FVector SpawnLoc = LootSpawnLoc + ActiveRoomData->SacrificeEventAltarSpawnOffset;
	AAltarActor* Altar = World->SpawnActor<AAltarActor>(AltarClass, SpawnLoc, FRotator::ZeroRotator);
	if (!Altar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoomEvent] Failed to spawn sacrifice altar: Room=%s"),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	ConfigureSacrificeAltar(Altar);
	UE_LOG(LogTemp, Log, TEXT("[RoomEvent] Sacrifice altar spawned: Room=%s Altar=%s Location=%s"),
		*GetNameSafe(ActiveRoomData),
		*GetNameSafe(Altar),
		*SpawnLoc.ToCompactString());
}

void AYogGameMode::ConfirmArrangementAndTransition()
{
	if (CurrentPhase != ELevelPhase::Arrangement)
		return;

	CurrentPhase = ELevelPhase::Transitioning;
	OnPhaseChanged.Broadcast(CurrentPhase);

	// 锁背包
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Backpack->SetLocked(true);
		}
	}

	// 旧系统入口：LevelSequenceData 已废弃，由 Portal 触发 TransitionToLevel
	FName NextLevelName;

	if (!NextLevelName.IsNone())
	{
		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			GI->PendingNextFloor = CurrentFloor + 1;

			// 切关前将玩家状态写入 GI，供新关卡恢复
			if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
				UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
			{
				FRunState NewState;
				NewState.bIsValid = true;
				NewState.CurrentGold = Player->BackpackGridComponent ? Player->BackpackGridComponent->Gold : 0;

				if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
				{
					NewState.CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());

					// 热度切关：默认归零，符文可保留一阶或两阶
					static const FGameplayTag TagRetainTwo = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.TwoPhase"), false);
					static const FGameplayTag TagRetainOne = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.OnePhase"), false);
					const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
					if (TagRetainTwo.IsValid() && ASC->HasMatchingGameplayTag(TagRetainTwo))
						NewState.CurrentHeat = MaxHeat * 2.f;
					else if (TagRetainOne.IsValid() && ASC->HasMatchingGameplayTag(TagRetainOne))
						NewState.CurrentHeat = MaxHeat;
					else
						NewState.CurrentHeat = 0.f;
				}

				if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
				{
					NewState.CurrentPhase = Backpack->GetCurrentPhase();
					// 只保存非永久符文（永久符文由 BeginPlay 自动重放）
					for (const FPlacedRune& PR : Backpack->GetAllPlacedRunes())
					{
						if (!PR.bIsPermanent)
						{
							NewState.PlacedRunes.Add(PR);
						}
					}
				}

				if (UCombatDeckComponent* CombatDeck = Player->CombatDeckComponent)
				{
					for (const FCombatCardInstance& Card : CombatDeck->GetFullDeckSnapshot())
					{
						if (Card.SourceData)
						{
							NewState.CombatDeckCards.Add(Card.SourceData);
							NewState.CombatDeckCardOrientations.Add(Card.LinkOrientation);
						}
					}
					NewState.CombatDeckShuffleCooldownDuration = CombatDeck->GetShuffleCooldownDuration();
					NewState.CombatDeckMaxActiveSequenceSize = CombatDeck->GetMaxActiveSequenceSize();
				}
				NewState.CompletedCombatBattleCount = CompletedCombatBattleCount;

				NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;
				NewState.SacrificeOfferingCosts = Player->GetSacrificeOfferingCosts();
				if (Player->ActiveSkillComponent)
				{
					for (UActiveSkillDataAsset* Skill : Player->ActiveSkillComponent->GetSkillLoadout())
					{
						NewState.SelectedSkillLoadout.Add(Skill);
					}
				}

				GI->PendingRunState = NewState;
				UE_LOG(LogTemp, Warning, TEXT("[RunState] SAVE — HP=%.1f Gold=%d Phase=%d Heat=%.0f Runes=%d"),
					NewState.CurrentHP, NewState.CurrentGold, NewState.CurrentPhase, NewState.CurrentHeat, NewState.PlacedRunes.Num());
			}
		}
		UGameplayStatics::OpenLevel(GetWorld(), NextLevelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ConfirmArrangementAndTransition: 没有找到下一关的 LevelName，请检查 CampaignData.FloorTable[%d].LevelName"), CurrentFloor);
	}
}

void AYogGameMode::HandlePlayerDeath(APlayerCharacterBase* Player)
{
	if (bPlayerDeathPending || bGameOverTriggered)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SS = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			SS->RecordPlayerDeath();
		}
	}

	bPlayerDeathPending = true;
	PendingDeathPlayer = Player;
	CurrentPhase = ELevelPhase::Transitioning;
	OnPhaseChanged.Broadcast(CurrentPhase);

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(WaveTriggerTimer);
	TM.ClearTimer(OneByOneTimer);
	TM.ClearTimer(InitialSpawnDelayTimer);
	TM.ClearTimer(DemandSpawnTimer);
	TM.ClearTimer(ForcedSurvivalSpawnTimer);

	if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
	{
		StoryEngine->BroadcastStoryEventWithPayload(
			FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Player.Died"), false),
			ActiveGlobalStageTag,
			ActiveGlobalStageTag,
			FGameplayTag(),
			Player,
			Player ? Cast<APlayerController>(Player->GetController()) : GetWorld()->GetFirstPlayerController());
	}

	TriggerLifecycleEvent(EGameLifecycleEvent::PlayerDeath);
	BeginPlayerDeathVisuals(Player);

	if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetShowMouseCursor(false);
	}

	bScriptedDefeatGameOver = false;
	if (UFirstRunTutorialDirectorSubsystem* Director = GetGameInstance()->GetSubsystem<UFirstRunTutorialDirectorSubsystem>())
	{
		if (Director->ShouldHandleScriptedDefeatDeath())
		{
			// Apply tutorial completion side-effects immediately so subsequent map loads
			// (whichever path the player picks from the death menu) treat the tutorial as finished
			// and the world-rewind hint fires on the first hub room.
			Director->HandleScriptedDefeatDeath(this);
			bScriptedDefeatGameOver = true;
			bPlayerDeathReviveUsed = true;
		}
	}

	if (!bScriptedDefeatGameOver && CanOfferPlayerDeathRevive(bGameOverTriggered, bPlayerDeathReviveUsed) && Player)
	{
		Player->PrepareForDeathReviveChoice();
	}

	ClearPlayerDeathGameOverTicker();
	TWeakObjectPtr<AYogGameMode> WeakThis(this);
	PlayerDeathGameOverTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->FinishPlayerDeathGameOver();
			}
			return false;
		}),
		FMath::Max(0.f, PlayerDeathGameOverDelay));

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Player died. Game over will finalize in %.2fs."), PlayerDeathGameOverDelay);
}

bool AYogGameMode::CanOfferPlayerDeathRevive(bool bInGameOverTriggered, bool bInPlayerDeathReviveUsed)
{
	return !bInGameOverTriggered && !bInPlayerDeathReviveUsed;
}

bool AYogGameMode::ShouldBroadcastRunSummaryForPlayerDeathGameOver(bool /*bCanRevive*/)
{
	return false;
}

float AYogGameMode::CalculatePlayerReviveHealth(float MaxHealth, float ReviveHealthPercent)
{
	if (MaxHealth <= 0.f || ReviveHealthPercent <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(MaxHealth * ReviveHealthPercent, 1.f, MaxHealth);
}

void AYogGameMode::FinishPlayerDeathGameOver()
{
	ClearPlayerDeathGameOverTicker();
	const bool bCanRevive = CanOfferPlayerDeathRevive(bGameOverTriggered, bPlayerDeathReviveUsed)
		&& PendingDeathPlayer.IsValid();

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Finalize death. CanRevive=%d ReviveUsed=%d PendingPlayer=%s"),
		(int32)bCanRevive,
		(int32)bPlayerDeathReviveUsed,
		*GetNameSafe(PendingDeathPlayer.Get()));

	if (!bCanRevive)
	{
		bGameOverTriggered = true;

		if (APlayerCharacterBase* Player = PendingDeathPlayer.Get())
		{
			Player->ResetToDefaultUnarmedCombatState();
		}

		// 广播本局结算数据（展示结算界面、触发成长货币结算存档）
		if (ShouldBroadcastRunSummaryForPlayerDeathGameOver(bCanRevive))
		{
			if (UYogMetaProgressionSubsystem* Meta = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr)
			{
				Meta->BroadcastRunEnded(CurrentFloor, MonsterKillCount);
			}
		}

		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			GI->ClearRunState();
			// 死亡才清磁盘存档点；中途退出走 ReturnToMainMenu，不清除
			if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
			{
				SaveSys->ClearRunCheckpoint();
			}
		}
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (APlayerCharacterBase* Player = PendingDeathPlayer.Get())
	{
		Player->CustomTimeDilation = 1.f;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ShowGameOverScreen(bCanRevive, bScriptedDefeatGameOver);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Game paused after player death."));
}

bool AYogGameMode::RevivePlayerFromDeath()
{
	// Gameplay-only revive: flip the death flags, unpause, restore time dilation, hand control
	// back to the pawn. UI teardown (death menu, mouse cursor, InputMode, SetBlockGameInput) is
	// owned by the death-menu widget that called this; do not touch UI state from here.
	APlayerCharacterBase* Player = PendingDeathPlayer.Get();
	if (!bPlayerDeathPending || bPlayerDeathReviveUsed || !Player)
	{
		return false;
	}

	ClearPlayerDeathGameOverTicker();
	bPlayerDeathPending = false;
	bPlayerDeathReviveUsed = true;
	PendingDeathPlayer.Reset();

	// Hub 房间无战斗，保持 Arrangement Phase；仅在普通战斗房间里才切回 Combat
	if (!ActiveRoomData || !ActiveRoomData->bIsHubRoom)
	{
		CurrentPhase = ELevelPhase::Combat;
		OnPhaseChanged.Broadcast(CurrentPhase);
	}

	// Restore global time dilation and clear the per-pawn dilation override set in BeginPlayerDeathVisuals.
	// (HUD->EndDeathEffect() inside this helper is the one UI bit still piggy-backing here — extract
	// it to the death-menu widget's OnDeactivated next pass.)
	EndPlayerDeathVisuals(Player);

	UGameplayStatics::SetGamePaused(GetWorld(), false);
	Player->ReviveFromDeath(PlayerReviveHealthPercent, PlayerReviveProtectionDuration);

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
		{
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Player.Revived"), false),
				ActiveGlobalStageTag,
				ActiveGlobalStageTag,
				FGameplayTag(),
				Player,
				PC);
		}

		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->EnableInput(PC);
		PC->SetPause(false);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Player revived at %.0f%% HP with %.1fs protection."),
		PlayerReviveHealthPercent * 100.f, PlayerReviveProtectionDuration);
	return true;
}

void AYogGameMode::ClearPlayerDeathGameOverTicker()
{
	if (PlayerDeathGameOverTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PlayerDeathGameOverTickerHandle);
		PlayerDeathGameOverTickerHandle.Reset();
	}
}

void AYogGameMode::BeginPlayerDeathVisuals(APlayerCharacterBase* Player)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float DilationScale = FMath::Clamp(PlayerDeathTimeDilationScale, 0.01f, 1.f);
	UGameplayStatics::SetGlobalTimeDilation(World, DilationScale);
	if (Player)
	{
		Player->CustomTimeDilation = 1.f / DilationScale;
	}
	if (AYogHUD* HUD = Cast<AYogHUD>(UGameplayStatics::GetPlayerController(World, 0)
		? UGameplayStatics::GetPlayerController(World, 0)->GetHUD()
		: nullptr))
	{
		HUD->BeginDeathEffect();
	}
}

void AYogGameMode::EndPlayerDeathVisuals(APlayerCharacterBase* Player)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
		if (AYogHUD* HUD = Cast<AYogHUD>(UGameplayStatics::GetPlayerController(World, 0)
			? UGameplayStatics::GetPlayerController(World, 0)->GetHUD()
			: nullptr))
		{
			HUD->EndDeathEffect();
		}
	}
	if (Player)
	{
		Player->CustomTimeDilation = 1.f;
	}
}

// =========================================================
// 新刷怪系统实现
// =========================================================

void AYogGameMode::StartLevelSpawning()
{
	const FString CurrentMapNameForFrontend = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	UCampaignDataAsset* Campaign = GetActiveCampaignData();
	UE_LOG(LogTemp, Warning, TEXT("[StartLevelSpawning] Map=%s CampaignData=%s ActiveRoomData=%s"),
		*CurrentMapNameForFrontend, *GetNameSafe(Campaign), *GetNameSafe(ActiveRoomData));

	ClearRoomRewardOptionsOverride();
	ClearForcedPortalOverride();
	bSuppressRoomClearRewardPickup = false;
	bStorySpecialRewardEnemyEnabled = false;
	StorySpecialRewardEnemyLootOptions.Reset();
	StorySpecialRewardEnemyAuraFX = nullptr;
	bForcedSurvivalActive = false;
	GetWorldTimerManager().ClearTimer(ForcedSurvivalSpawnTimer);

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

	if (!Campaign)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: CampaignData 未配置，跳过新刷怪系统"));
		return;
	}

	// 从 GameInstance 读取上一关存储的楼层推进（切关后 GameMode 重建，CurrentFloor 默认为 1）
	if (GI && GI->PendingNextFloor > 1)
	{
		CurrentFloor = GI->PendingNextFloor;
	}
	CompletedCombatBattleCount = (GI && GI->PendingRunState.bIsValid)
		? FMath::Max(0, GI->PendingRunState.CompletedCombatBattleCount)
		: 0;

	if (GI && GI->PendingRunState.bIsValid)
	{
		if (UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>())
		{
			SaveSys->TriggerCheckpoint(CurrentFloor);
		}
	}

	const bool bNewRunStart = CurrentFloor <= 1 && (!GI || !GI->PendingRunState.bIsValid);
	if (bNewRunStart)
	{
		if (UStoryEventManager* StoryEventManager = GetGameInstance()->GetSubsystem<UStoryEventManager>())
		{
			StoryEventManager->ResetRunEvents();
		}
	}

	// FloorTable 下标从 0 开始，CurrentFloor 从 1 开始
	const int32 TableIndex = CurrentFloor - 1;
	if (!Campaign->FloorTable.IsValidIndex(TableIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: FloorTable 没有第 %d 关的配置，降级为场景预放置敌人统计"), CurrentFloor);
		FallbackToPreplacedEnemies();
		return;
	}

	const FFloorConfig& Config = Campaign->FloorTable[TableIndex];

	// ---- 确定本关使用的 DA_Room ----
	// 优先读取 GI 中由传送门写入的 PendingRoomData（切关传递）
	// 若为空（第一关或测试场景），直接按本关 FloorConfig 骰子选取
	bool bUsedPendingRoomData = false;
	if (GI && GI->PendingRoomData)
	{
		ActiveRoomData = GI->PendingRoomData;
		GI->PendingRoomData = nullptr; // 清除，避免复用
		bUsedPendingRoomData = true;
		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 Portal 传递的 RoomData = %s"), *ActiveRoomData->GetName());
	}
	else
	{
		// 第一关：优先使用 DefaultStartingRoom（策划手动指定）；未填则骰子选取
		if (CurrentFloor == 1 && Campaign->DefaultStartingRoom)
		{
			const FName DefaultRoomName = Campaign->DefaultStartingRoom->RoomName;
			const FName DefaultLevelName = ResolveRoomLevelNameForOpen(DefaultRoomName, Campaign->DefaultStartingRoom);
			bool bPreservedCurrentEditorMap = false;
			if (!DefaultLevelName.IsNone())
			{
				// 检查当前加载的关卡是否已经是 DefaultStartingRoom 指定的关卡
				// GetCurrentLevelName(true) 会去掉 PIE 前缀（如 "UEDPIE_0_"）
				const FString CurrentMapName = FPackageName::GetShortName(UGameplayStatics::GetCurrentLevelName(GetWorld(), true));
				const bool bCurrentMapMatchesDefault = CurrentMapName.Equals(FPackageName::GetShortName(DefaultLevelName.ToString()), ESearchCase::IgnoreCase);
				if (!bCurrentMapMatchesDefault
					&& ShouldPreserveCurrentMapForEditorPlay(GetWorld() && GetWorld()->WorldType == EWorldType::PIE, GI && GI->PendingRoomData != nullptr))
				{
					bPreservedCurrentEditorMap = true;
					UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: PIE current map [%s] differs from DefaultStartingRoom [%s]; preserving editor map."),
						*CurrentMapName, *DefaultLevelName.ToString());
				}
				if (!bCurrentMapMatchesDefault && !bPreservedCurrentEditorMap)
				{
					// 当前关卡不匹配，重定向到 DefaultStartingRoom 的关卡
					UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 当前关卡 [%s] ≠ DefaultStartingRoom [%s]，重定向..."),
						*CurrentMapName, *DefaultLevelName.ToString());
					if (GI) GI->PendingRoomData = Campaign->DefaultStartingRoom;
					UGameplayStatics::OpenLevel(GetWorld(), DefaultLevelName);
					return;
				}
			}

			if (bPreservedCurrentEditorMap)
			{
				ActiveRoomData = nullptr;
				UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: no RoomData selected for current editor map; using preplaced enemy fallback."));
			}
			else
			{
				ActiveRoomData = Campaign->DefaultStartingRoom;
				UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 DefaultStartingRoom = %s"), *ActiveRoomData->GetName());
			}
		}
		else
		{
			// 按 FloorConfig 概率骰出类型 Tag，再从全局 RoomPool 中选取
			const FGameplayTag RequiredType = RollRoomTypeForFloor(Config);
			ActiveRoomData = SelectRoomByTag(nullptr, RequiredType);
			UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 骰子选取 RoomData = %s"),
				ActiveRoomData ? *ActiveRoomData->GetName() : TEXT("null"));
		}
	}

	if (!ActiveRoomData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 第 %d 关无法获取 RoomData，降级为场景预放置敌人统计"), CurrentFloor);
		FallbackToPreplacedEnemies();
		return;
	}

	// ── 主城/枢纽房间：无战斗，立即全开传送门 ──────────────────────────────
	FStoryNextRoomPlan CurrentRoomStoryPlan;
	const bool bConsumedStoryPlan = GI
		&& ActiveRoomData
		&& !ActiveRoomData->bIsHubRoom
		&& GI->ConsumePendingStoryNextRoomPlan(CurrentRoomStoryPlan);
	if (bConsumedStoryPlan)
	{
		ApplyStoryNextRoomPlanForCurrentRoom(CurrentRoomStoryPlan);
	}

	const bool bAppliedPendingRewardOverride = bConsumedStoryPlan && CurrentRoomStoryPlan.bOverrideRewardOptions
		? true
		: ApplyPendingRoomRewardOptionsOverrideForRoom(GI, ActiveRoomData);
	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] GM StartLevelSpawning after room-aware pending reward check Applied=%d HasCurrentOverride=%d ActiveRoom=%s IsHub=%d CurrentOverrideOptions=%s GI=%s"),
		bAppliedPendingRewardOverride ? 1 : 0,
		bHasRoomRewardOptionsOverride ? 1 : 0,
		*GetNameSafe(ActiveRoomData),
		ActiveRoomData->bIsHubRoom ? 1 : 0,
		*DescribeGameModeLootOptionsForRewardDebug(RoomRewardOptionsOverride),
		*GetNameSafe(GI));

	const bool bNonCombatEventRoom = ShouldSkipCombatForRoom(ActiveRoomData);
	ActiveGlobalStageTag = Config.GlobalStageTag;
	ActiveStoryEventTags = bNonCombatEventRoom ? FGameplayTagContainer() : Config.StoryEventTags;
	OnCampaignStageEntered.Broadcast(CurrentFloor, ActiveGlobalStageTag, ActiveStoryEventTags, ActiveRoomData);
	if (bDispatchStoryEventsFromCampaign)
	{
		if (UStoryEventManager* StoryEventManager = GetGameInstance()->GetSubsystem<UStoryEventManager>())
		{
			StoryEventManager->SetRegistry(StoryEventRegistry);
			StoryEventManager->ProcessCampaignStage(
				CurrentFloor,
				ActiveGlobalStageTag,
				ActiveStoryEventTags,
				ActiveRoomData,
				UGameplayStatics::GetPlayerController(this, 0));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[CampaignStage] Floor=%d Stage=%s EventTags=%s Room=%s"),
		CurrentFloor,
		*ActiveGlobalStageTag.ToString(),
		*ActiveStoryEventTags.ToStringSimple(),
		*GetNameSafe(ActiveRoomData));

	bTimedClearObjectiveActive = ActiveRoomData->bEnableTimedClearObjective && ActiveRoomData->TimedClearSeconds > 0.0f;
	bTimedClearObjectiveExpired = false;
	GetWorldTimerManager().ClearTimer(TimedClearObjectiveTimer);
	if (bTimedClearObjectiveActive)
	{
		GetWorldTimerManager().SetTimer(
			TimedClearObjectiveTimer,
			this,
			&AYogGameMode::HandleTimedClearObjectiveExpired,
			ActiveRoomData->TimedClearSeconds,
			false);
		UE_LOG(LogTemp, Log, TEXT("[RoomEvent] Timed clear objective started: Room=%s Seconds=%.1f"),
			*GetNameSafe(ActiveRoomData),
			ActiveRoomData->TimedClearSeconds);
	}

	if (ActiveRoomData->bIsHubRoom)
	{
		// Hub 视为第 0 关，TransitionToLevel 写入 PendingNextFloor = 0+1 = 1
		// → 下一关读 FloorTable[0]（第一个战斗关）
		if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			const FGameplayTag HubAreaTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Area.Hub"), false);
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Area.Entered"), false),
				HubAreaTag,
				ActiveGlobalStageTag,
				FGameplayTag(),
				nullptr,
				PC);
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Hub.FirstEntered"), false),
				HubAreaTag,
				ActiveGlobalStageTag,
				FGameplayTag(),
				nullptr,
				PC);
		}

		CurrentFloor = 0;
		CurrentPhase = ELevelPhase::Arrangement; // 跳过战斗阶段

		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		{
			Player->ClearRunCarriedStateForHub();
		}
		if (GI)
		{
			GI->ClearRunState();
			if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
			{
				SaveSys->ClearRunCheckpoint();
			}
		}

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->HideCurrentRoomBuffs();
			}

			if (GI && GI->ConsumeFirstRunWorldRewindHint())
			{
				ShowFirstRunWorldRewindHint(this, PC);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: [HubRoom] %s — 跳过刷怪，立即开启传送门"), *ActiveRoomData->GetName());

		// 标记场景中未登记的门为 NeverOpen
		{
			TArray<AActor*> AllPortalActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), AllPortalActors);
			for (AActor* PortalActor : AllPortalActors)
			{
				APortal* Portal = Cast<APortal>(PortalActor);
				if (!Portal) continue;
				bool bCanOpen = false;
				for (const FPortalDestConfig& Dest : ActiveRoomData->PortalDestinations)
				{
					if (Dest.PortalIndex == Portal->Index) { bCanOpen = true; break; }
				}
				if (!bCanOpen)
				{
					Portal->MarkUnavailable();
				}
			}
		}

		EnsureHubActiveSkillTerminal();
		if (ShouldDelayInitialRoomPortalsUntilWeapon())
		{
			UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: initial hub portals delayed until the player has a weapon."));
			return;
		}
		ActivateHubPortals();
		return;
	}

	if (IsShopRoom())
	{
		CurrentPhase = ELevelPhase::Arrangement;
		OnPhaseChanged.Broadcast(CurrentPhase);

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->HideCurrentRoomBuffs();
			}
		}

		{
			TArray<AActor*> AllPortalActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), AllPortalActors);
			for (AActor* PortalActor : AllPortalActors)
			{
				APortal* Portal = Cast<APortal>(PortalActor);
				if (!Portal) continue;

				bool bCanOpen = false;
				for (const FPortalDestConfig& Dest : ActiveRoomData->PortalDestinations)
				{
					if (Dest.PortalIndex == Portal->Index)
					{
						bCanOpen = true;
						break;
					}
				}

				if (!bCanOpen)
				{
					Portal->MarkUnavailable();
				}
			}
		}

		SpawnShopActorForRoom();
		ActivatePortals();

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->ShowPortalGuidance();
			}
		}

		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: [ShopRoom] %s - skip combat and open shop"),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	if (ShouldSkipCombatForRoom(ActiveRoomData))
	{
		CurrentPhase = ELevelPhase::Arrangement;
		OnPhaseChanged.Broadcast(CurrentPhase);

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->HideCurrentRoomBuffs();
			}
		}

		{
			TArray<AActor*> AllPortalActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), AllPortalActors);
			for (AActor* PortalActor : AllPortalActors)
			{
				APortal* Portal = Cast<APortal>(PortalActor);
				if (!Portal) continue;

				bool bCanOpen = false;
				for (const FPortalDestConfig& Dest : ActiveRoomData->PortalDestinations)
				{
					if (Dest.PortalIndex == Portal->Index)
					{
						bCanOpen = true;
						break;
					}
				}

				if (!bCanOpen)
				{
					Portal->MarkUnavailable();
				}
			}
		}

		FVector EventAnchorLoc = FVector::ZeroVector;
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* P = PC->GetPawn())
			{
				EventAnchorLoc = P->GetActorLocation();
			}
		}

		SpawnSacrificeEventAltar(EventAnchorLoc);
		ActivatePortals();

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->ShowPortalGuidance();
			}
		}

		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: [EventRoom] %s - skip combat reward flow and open event room"),
			*GetNameSafe(ActiveRoomData));
		return;
	}

	// 根据总难度分选取房间难度档位（决定最大波次数 + 奖励配置）
	const int32 Score = Config.TotalDifficultyScore;
	const FRoomDifficultyTier& Tier = (Score <= LowDifficultyScoreMax)
		? ActiveRoomData->LowDifficulty
		: (Score >= HighDifficultyScoreMin)
			? ActiveRoomData->HighDifficulty
			: ActiveRoomData->MediumDifficulty;

	// 缓存奖励配置（金币/Buff 现在按难度档位配置，整理阶段使用）
	ActiveGoldMin   = Tier.GoldMin;
	ActiveGoldMax   = Tier.GoldMax;
	ActiveBuffCount = Tier.BuffCount;

	UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 总难度分=%d → 档位=%s MaxWaveCount=%d"),
		Score,
		Score <= LowDifficultyScoreMax ? TEXT("Low") : (Score >= HighDifficultyScoreMin ? TEXT("High") : TEXT("Medium")),
		Tier.MaxWaveCount);

	// v3：本次切关来自 Portal（PendingRoomData 非空）→ 直接用预骰好的 Buff，跳过现场抽
	// 即使 PendingRoomBuffs 为空数组也是合法预骰结果（如商店/事件房 BuffCount=0）
	if (bUsedPendingRoomData && GI)
	{
		ActiveRoomBuffs = GI->PendingRoomBuffs;
		GI->PendingRoomBuffs.Reset();
		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 Portal 预骰 Buff，数=%d"), ActiveRoomBuffs.Num());
	}
	else
	{
		ActiveRoomBuffs = SelectRoomBuffs(*ActiveRoomData, ActiveBuffCount);
		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 现场抽 Buff（无 Portal 预骰），数=%d"), ActiveRoomBuffs.Num());
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->ShowCurrentRoomBuffs(ActiveRoomData, ActiveRoomBuffs);
		}
	}

	// 生成波次计划
	GenerateWavePlans(Score, Tier.MaxWaveCount, ActiveRoomData);

	// 重置运行时状态
	CurrentWaveIndex  = -1;
	TotalAliveEnemies = 0;
	PendingSpawnCount = 0;
	bAllWavesSpawned  = false;

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(WaveTriggerTimer);
	TimerManager.ClearTimer(OneByOneTimer);
	TimerManager.ClearTimer(InitialSpawnDelayTimer);
	TimerManager.ClearTimer(DemandSpawnTimer);

	// ---- 标记本关永不开启的传送门 ----
	// PortalDestinations 中未登记的门：关卡开始时即确定不会开启，调用 NeverOpen() 显示静态装饰
	{
		TArray<AActor*> AllPortalActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), AllPortalActors);
		for (AActor* PortalActor : AllPortalActors)
		{
			APortal* Portal = Cast<APortal>(PortalActor);
			if (!Portal) continue;

			bool bCanOpen = false;
			for (const FPortalDestConfig& Dest : ActiveRoomData->PortalDestinations)
			{
				if (Dest.PortalIndex == Portal->Index)
				{
					bCanOpen = true;
					break;
				}
			}

			if (!bCanOpen)
			{
				Portal->MarkUnavailable();
				UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: Portal[%d] 永不开启 → NeverOpen"), Portal->Index);
			}
		}
	}

	// 延迟后启动第一波（给特效/动画和 AI 初始化预留时间）
	if (InitialSpawnDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			InitialSpawnDelayTimer,
			this,
			&AYogGameMode::TriggerNextWave,
			InitialSpawnDelay,
			false
		);
	}
	else
	{
		TriggerNextWave();
	}
}

void AYogGameMode::GenerateWavePlans(int32 TotalScore, int32 MaxWaveCount, URoomDataAsset* Room)
{
	WavePlans.Empty();
	LevelTypeSpawnCounts.Empty();
	TotalLevelPlannedEnemies = 0;

	// 在 [1, MaxWaveCount] 内随机波次数
	const int32 WaveCount = FMath::RandRange(1, FMath::Max(1, MaxWaveCount));
	UE_LOG(LogTemp, Log, TEXT("GenerateWavePlans: 总难度分=%d MaxWaveCount=%d → 实际 %d 波"), TotalScore, MaxWaveCount, WaveCount);

	// 将总分均分到各波，余数加到第一波
	const int32 BasePerWave  = (WaveCount > 0) ? (TotalScore / WaveCount) : TotalScore;
	const int32 Remainder    = (WaveCount > 0) ? (TotalScore % WaveCount)  : 0;

	// 读取当前难度档位的刷怪上限（GenerateWavePlans 调用前 ActiveRoomData 已缓存）
	const FRoomDifficultyTier& Tier = [&]() -> const FRoomDifficultyTier&
	{
		if (Room)
		{
			const int32 Score = TotalScore;
			if (Score >= HighDifficultyScoreMin) return Room->HighDifficulty;
			if (Score <= LowDifficultyScoreMax)  return Room->LowDifficulty;
			return Room->MediumDifficulty;
		}
		static FRoomDifficultyTier Default;
		return Default;
	}();

	for (int32 i = 0; i < WaveCount; i++)
	{
		if (Tier.MaxEnemiesPerLevel > 0 && TotalLevelPlannedEnemies >= Tier.MaxEnemiesPerLevel)
		{
			UE_LOG(LogTemp, Log, TEXT("GenerateWavePlans: 已达到整关敌人上限 %d，后续波次为空"), Tier.MaxEnemiesPerLevel);
			break;
		}

		const int32 Budget = BasePerWave + (i == 0 ? Remainder : 0);
		WavePlans.Add(BuildWavePlan(Budget, Room, Tier.MaxEnemiesPerWave, Tier.MaxEnemiesPerLevel));
	}

	if (Room && Room->bSpawnSpecialRewardEnemy && Room->SpecialRewardEnemyClass)
	{
		if (WavePlans.IsEmpty())
		{
			WavePlans.Add(FWavePlan());
		}

		FPlannedEnemy SpecialEnemy;
		SpecialEnemy.EnemyClass = Room->SpecialRewardEnemyClass;
		SpecialEnemy.PreSpawnFX = Room->SpecialRewardEnemyPreSpawnFX;
		SpecialEnemy.PreSpawnFXDuration = Room->SpecialRewardEnemyPreSpawnFXDuration;
		SpecialEnemy.bAllowAnySpawner = true;
		SpecialEnemy.bSpecialRewardEnemy = true;

		FWavePlan& TargetWave = WavePlans.Last();
		TargetWave.EnemiesToSpawn.Add(SpecialEnemy);
		TotalLevelPlannedEnemies++;
		LevelTypeSpawnCounts.FindOrAdd(SpecialEnemy.EnemyClass)++;

		UE_LOG(LogTemp, Log, TEXT("GenerateWavePlans: appended special reward enemy %s to the final wave."),
			*GetNameSafe(SpecialEnemy.EnemyClass));
	}

	if (bStorySpecialRewardEnemyEnabled)
	{
		if (WavePlans.IsEmpty())
		{
			WavePlans.Add(FWavePlan());
		}

		FWavePlan* TargetWave = nullptr;
		for (int32 WaveIndex = WavePlans.Num() - 1; WaveIndex >= 0; --WaveIndex)
		{
			if (!WavePlans[WaveIndex].EnemiesToSpawn.IsEmpty())
			{
				TargetWave = &WavePlans[WaveIndex];
				break;
			}
		}

		if (!TargetWave)
		{
			TargetWave = &WavePlans.Last();
			if (Room)
			{
				for (const FEnemyEntry& Entry : Room->EnemyPool)
				{
					if (Entry.EnemyData && Entry.EnemyData->EnemyClass)
					{
						FPlannedEnemy Planned;
						Planned.EnemyClass = Entry.EnemyData->EnemyClass;
						Planned.EnemyData = Entry.EnemyData;
						Planned.PreSpawnFX = Entry.EnemyData->PreSpawnFX;
						Planned.PreSpawnFXDuration = Entry.EnemyData->PreSpawnFXDuration;
						Planned.SpawnLifecycleFlow = Entry.EnemyData->SpawnLifecycleFlow;
						TargetWave->EnemiesToSpawn.Add(Planned);
						TotalLevelPlannedEnemies++;
						LevelTypeSpawnCounts.FindOrAdd(Planned.EnemyClass)++;
						break;
					}
				}
			}
		}

		if (TargetWave && !TargetWave->EnemiesToSpawn.IsEmpty())
		{
			FPlannedEnemy& SpecialEnemy = TargetWave->EnemiesToSpawn.Last();
			SpecialEnemy.bSpecialRewardEnemy = true;
			SpecialEnemy.SpecialRewardOptions = StorySpecialRewardEnemyLootOptions;
			SpecialEnemy.SpecialRewardAuraFX = StorySpecialRewardEnemyAuraFX;
			UE_LOG(LogTemp, Log,
				TEXT("[FirstRunTutorialDirector] Marked last planned enemy as story special reward enemy. Class=%s LootCount=%d"),
				*GetNameSafe(SpecialEnemy.EnemyClass),
				SpecialEnemy.SpecialRewardOptions.Num());
		}
	}
}

static bool ShouldApplyBuffEntry(const FBuffEntry& Entry)
{
	return Entry.ApplyChance >= 1.f || FMath::FRand() <= FMath::Clamp(Entry.ApplyChance, 0.f, 1.f);
}

// EnemyData.EnemyBuffPool is rolled per planned enemy. Only entries that pass ApplyChance are granted.
static int32 CollectEnemyBuffs(UEnemyData* EnemyData, TArray<TObjectPtr<URuneDataAsset>>& OutBuffs)
{
	OutBuffs.Reset();
	if (!EnemyData || EnemyData->EnemyBuffPool.IsEmpty())
	{
		return 0;
	}

	int32 TotalCost = 0;
	for (const FBuffEntry& Entry : EnemyData->EnemyBuffPool)
	{
		if (!Entry.RuneDA)
		{
			continue;
		}
		if (!ShouldApplyBuffEntry(Entry))
		{
			continue;
		}

		OutBuffs.AddUnique(Entry.RuneDA);
		TotalCost += Entry.DifficultyScore;
	}
	return TotalCost;
}

static FGameplayTag GetEnemyRuneEventTagForTriggerType(ERuneTriggerType Type)
{
	switch (Type)
	{
	case ERuneTriggerType::OnAttackHit:      return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
	case ERuneTriggerType::OnDash:           return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Dash"));
	case ERuneTriggerType::OnKill:           return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Kill"));
	case ERuneTriggerType::OnCritHit:        return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.CritHit"));
	case ERuneTriggerType::OnDamageReceived: return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"));
	default:                                  return FGameplayTag();
	}
}

static const TCHAR* GetEnemyRuneTriggerName(ERuneTriggerType Type)
{
	switch (Type)
	{
	case ERuneTriggerType::Passive:          return TEXT("Passive");
	case ERuneTriggerType::OnAttackHit:      return TEXT("OnAttackHit");
	case ERuneTriggerType::OnDash:           return TEXT("OnDash");
	case ERuneTriggerType::OnKill:           return TEXT("OnKill");
	case ERuneTriggerType::OnCritHit:        return TEXT("OnCritHit");
	case ERuneTriggerType::OnDamageReceived: return TEXT("OnDamageReceived");
	default:                                  return TEXT("Unknown");
	}
}

static bool IsEnemyDeathPoisonRune(const URuneDataAsset* RuneDA)
{
	const FString RunePath = GetPathNameSafe(RuneDA);
	return RunePath.Contains(TEXT("/DeathPoison/"));
}

static ERuneTriggerType ResolveEnemyRuneTriggerType(const URuneDataAsset* RuneDA)
{
	if (!RuneDA)
	{
		return ERuneTriggerType::Passive;
	}

	const ERuneTriggerType ConfiguredTrigger = RuneDA->GetTriggerType();
	const FCombatCardConfig& CombatCard = RuneDA->RuneInfo.CombatCard;
	if (ConfiguredTrigger == ERuneTriggerType::Passive
		&& CombatCard.bIsCombatCard
		&& CombatCard.TriggerTiming == ECombatCardTriggerTiming::OnHit)
	{
		return ERuneTriggerType::OnAttackHit;
	}

	return ConfiguredTrigger;
}

static FString GetEnemyRuneDebugName(const URuneDataAsset* RuneDA)
{
	if (!RuneDA)
		return TEXT("None");

	const FName RuneName = RuneDA->GetRuneName();
	return RuneName.IsNone() ? GetNameSafe(RuneDA) : RuneName.ToString();
}

static void ActivateEnemyRune(AEnemyCharacterBase* Enemy, URuneDataAsset* RuneDA, const TCHAR* Source)
{
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Enemy=null Source=%s"), Source);
		return;
	}

	if (!RuneDA)
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyRune] Skip: Rune=null Enemy=%s Source=%s"),
			*GetNameSafe(Enemy), Source);
		return;
	}

	UFlowAsset* FlowAsset = RuneDA->GetFlowAsset();
	const ERuneTriggerType TriggerType = ResolveEnemyRuneTriggerType(RuneDA);
	const ERuneTriggerType ConfiguredTriggerType = RuneDA->GetTriggerType();
	const bool bUseDeathAnimCompleteTrigger = IsEnemyDeathPoisonRune(RuneDA);
	const FString EffectiveTriggerName = bUseDeathAnimCompleteTrigger
		? FString(TEXT("DeathAnimComplete"))
		: FString(GetEnemyRuneTriggerName(TriggerType));
	const FString RuneDebugName = GetEnemyRuneDebugName(RuneDA);

	if (!FlowAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing Flow Enemy=%s Source=%s Rune=%s DA=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(RuneDA), *EffectiveTriggerName);
		return;
	}

	UBuffFlowComponent* BFC = Enemy->BuffFlowComponent;
	if (!BFC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing BuffFlowComponent Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(RuneDA), *GetNameSafe(FlowAsset), *EffectiveTriggerName);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Granted Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s"),
		*GetNameSafe(Enemy),
		Source,
		*RuneDebugName,
		*GetNameSafe(RuneDA),
		*GetNameSafe(FlowAsset),
		*EffectiveTriggerName);
	if (bUseDeathAnimCompleteTrigger && ConfiguredTriggerType == ERuneTriggerType::Passive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] TriggerOverride Enemy=%s Rune=%s Configured=%s Effective=%s Reason=DeathPoisonWaitsForDeathAnimComplete"),
			*GetNameSafe(Enemy),
			*RuneDebugName,
			GetEnemyRuneTriggerName(ConfiguredTriggerType),
			*EffectiveTriggerName);
	}
	if (TriggerType != ConfiguredTriggerType)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] TriggerOverride Enemy=%s Rune=%s Configured=%s Effective=%s Reason=CombatCardOnHit"),
			*GetNameSafe(Enemy),
			*RuneDebugName,
			GetEnemyRuneTriggerName(ConfiguredTriggerType),
			GetEnemyRuneTriggerName(TriggerType));
	}

	if (TriggerType == ERuneTriggerType::Passive && !bUseDeathAnimCompleteTrigger)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] StartPassive Enemy=%s Source=%s Rune=%s Flow=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset));
		BFC->StartBuffFlowWithRune(FlowAsset, FGuid::NewGuid(), RuneDA, Enemy);
		return;
	}

	UAbilitySystemComponent* ASC = Enemy->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing ASC Enemy=%s Source=%s Rune=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset), *EffectiveTriggerName);
		return;
	}

	const FGameplayTag EventTag = bUseDeathAnimCompleteTrigger
		? FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.DeathAnimComplete"), false)
		: GetEnemyRuneEventTagForTriggerType(TriggerType);
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Invalid EventTag Enemy=%s Source=%s Rune=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset), *EffectiveTriggerName);
		return;
	}

	TWeakObjectPtr<AEnemyCharacterBase> WeakEnemy = Enemy;
	TWeakObjectPtr<UBuffFlowComponent> WeakBFC = BFC;
	TWeakObjectPtr<UFlowAsset> WeakFlowAsset = FlowAsset;
	TWeakObjectPtr<URuneDataAsset> WeakRuneDA = RuneDA;
	const FString CapturedRuneName = RuneDebugName;
	const FString CapturedRuneDAName = GetNameSafe(RuneDA);
	const FString CapturedFlowName = GetNameSafe(FlowAsset);
	const FString CapturedSource = Source;
	const FString CapturedTriggerName = EffectiveTriggerName;
	ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag)
		.AddWeakLambda(BFC, [WeakEnemy, WeakBFC, WeakFlowAsset, WeakRuneDA, EventTag, CapturedRuneName, CapturedRuneDAName, CapturedFlowName, CapturedSource, CapturedTriggerName](const FGameplayEventData* Payload)
		{
			AEnemyCharacterBase* CapturedEnemy = WeakEnemy.Get();
			UBuffFlowComponent* CapturedBFC = WeakBFC.Get();
			UFlowAsset* CapturedFlow = WeakFlowAsset.Get();
			if (!CapturedEnemy || !CapturedBFC || !CapturedFlow)
				return;

			const FGameplayTag EffectiveEventTag =
				(Payload && Payload->EventTag.IsValid()) ? Payload->EventTag : EventTag;
			AActor* InstigatorActor = Payload ? const_cast<AActor*>(Payload->Instigator.Get()) : nullptr;
			AActor* TargetActor = Payload ? const_cast<AActor*>(Payload->Target.Get()) : nullptr;

			CapturedBFC->LastEventContext.EventTag = EffectiveEventTag;
			CapturedBFC->LastEventContext.DamageCauser = InstigatorActor ? InstigatorActor : CapturedEnemy;
			CapturedBFC->LastEventContext.DamageReceiver = TargetActor ? TargetActor : CapturedEnemy;
			CapturedBFC->LastEventContext.DamageAmount = Payload ? Payload->EventMagnitude : 0.f;
			CapturedBFC->LastEventContext.AttackDirection = FVector::ZeroVector;

			UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] EventFired Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s Event=%s Instigator=%s Target=%s Magnitude=%.2f"),
				*GetNameSafe(CapturedEnemy),
				*CapturedSource,
				*CapturedRuneName,
				*CapturedRuneDAName,
				*CapturedFlowName,
				*CapturedTriggerName,
				*EffectiveEventTag.ToString(),
				*GetNameSafe(InstigatorActor),
				*GetNameSafe(TargetActor),
				Payload ? Payload->EventMagnitude : 0.f);

			CapturedBFC->StartBuffFlowWithRune(CapturedFlow, FGuid::NewGuid(), WeakRuneDA.Get(), CapturedEnemy, true);
		});

	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Registered Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s Event=%s"),
		*GetNameSafe(Enemy),
		Source,
		*RuneDebugName,
		*GetNameSafe(RuneDA),
		*GetNameSafe(FlowAsset),
		*EffectiveTriggerName,
		*EventTag.ToString());
}

static void ActivateEnemyRunes(AEnemyCharacterBase* Enemy, const TArray<TObjectPtr<URuneDataAsset>>& Runes, const TCHAR* Source)
{
	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][GrantList] Enemy=%s Source=%s Count=%d"),
		*GetNameSafe(Enemy),
		Source,
		Runes.Num());
	for (int32 Index = 0; Index < Runes.Num(); ++Index)
	{
		URuneDataAsset* RuneDA = Runes[Index].Get();
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][GrantList]   Index=%d Rune=%s DA=%s"),
			Index,
			*GetEnemyRuneDebugName(RuneDA),
			*GetNameSafe(RuneDA));
	}

	if (Runes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: RuneList empty Enemy=%s Source=%s"),
			*GetNameSafe(Enemy), Source);
		return;
	}

	for (const TObjectPtr<URuneDataAsset>& Rune : Runes)
	{
		ActivateEnemyRune(Enemy, Rune.Get(), Source);
	}
}

static void ActivateEnemyDataRunes(AEnemyCharacterBase* Enemy, UEnemyData* EnemyData, const TCHAR* Source)
{
	TArray<TObjectPtr<URuneDataAsset>> EnemyBuffs;
	CollectEnemyBuffs(EnemyData, EnemyBuffs);
	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][EnemyData] Enemy=%s EnemyData=%s Source=%s Pool=%d Collected=%d"),
		*GetNameSafe(Enemy),
		*GetNameSafe(EnemyData),
		Source,
		EnemyData ? EnemyData->EnemyBuffPool.Num() : 0,
		EnemyBuffs.Num());
	if (EnemyData)
	{
		for (int32 Index = 0; Index < EnemyData->EnemyBuffPool.Num(); ++Index)
		{
			const FBuffEntry& Entry = EnemyData->EnemyBuffPool[Index];
			UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][EnemyData]   Entry=%d Rune=%s DA=%s Cost=%d Chance=%.2f"),
				Index,
				*GetEnemyRuneDebugName(Entry.RuneDA.Get()),
				*GetNameSafe(Entry.RuneDA.Get()),
				Entry.DifficultyScore,
				Entry.ApplyChance);
		}
	}
	ActivateEnemyRunes(Enemy, EnemyBuffs, Source);
}

// 辅助：计算关卡 Buff 对每只怪的额外扣分（所有激活关卡 Buff 的 DifficultyScore 之和）
static int32 CalcRoomBuffCost(const TArray<FBuffEntry>& ActiveRoomBuffs)
{
	int32 Cost = 0;
	for (const FBuffEntry& B : ActiveRoomBuffs)
		Cost += B.DifficultyScore;
	return Cost;
}

AYogGameMode::FWavePlan AYogGameMode::BuildWavePlan(int32 Budget, URoomDataAsset* Room, int32 MaxEnemiesPerWave, int32 MaxEnemiesPerLevel)
{
	FWavePlan Plan;
	int32 RemainingBudget = Budget;

	if (!Room)
	{
		return Plan;
	}

	// ---- Step 1: 程序决定触发条件（默认 AllEnemiesDead，最稳定）----
	Plan.TriggerType = ESpawnTriggerType::AllEnemiesDead;

	// ---- Step 2: 程序决定刷怪方式（随机 Wave / OneByOne）----
	Plan.SpawnMode        = FMath::RandBool() ? ESpawnMode::Wave : ESpawnMode::OneByOne;
	Plan.OneByOneInterval = OneByOneDefaultInterval;

	// 关卡 Buff 对每只怪的额外扣分（进关时已确定）
	const int32 RoomBuffCostPerEnemy = CalcRoomBuffCost(ActiveRoomBuffs);

	auto HasLevelSlot = [&]() -> bool
	{
		return MaxEnemiesPerLevel <= 0 || TotalLevelPlannedEnemies < MaxEnemiesPerLevel;
	};

	auto HasTypeCapacity = [&](const FEnemyEntry& Entry) -> bool
	{
		if (!Entry.EnemyData || !Entry.EnemyData->EnemyClass)
		{
			return false;
		}
		if (Entry.MaxCountPerLevel < 0)
		{
			return true;
		}

		const int32 ExistingCount = LevelTypeSpawnCounts.FindRef(Entry.EnemyData->EnemyClass);
		return ExistingCount < Entry.MaxCountPerLevel;
	};

	auto GetBaseEffectiveCost = [&](const FEnemyEntry& Entry) -> int32
	{
		return Entry.EnemyData ? Entry.EnemyData->DifficultyScore + RoomBuffCostPerEnemy : MAX_int32;
	};

	auto BuildCandidates = [&](int32 CurrentBudget, bool bAllowBudgetOverflow)
	{
		TArray<FEnemyEntry> Candidates;
		if (!HasLevelSlot())
		{
			return Candidates;
		}

		for (const FEnemyEntry& Entry : Room->EnemyPool)
		{
			if (!Entry.EnemyData || !Entry.EnemyData->EnemyClass) continue;
			if (!HasTypeCapacity(Entry)) continue;

			const int32 EffectiveCost = GetBaseEffectiveCost(Entry);
			if (!bAllowBudgetOverflow && EffectiveCost > CurrentBudget) continue;

			Candidates.Add(Entry);
		}
		return Candidates;
	};

	auto BuildCandidatesWithFallback = [&](int32 CurrentBudget, bool bAllowBudgetFallback)
	{
		TArray<FEnemyEntry> Candidates = BuildCandidates(CurrentBudget, false);
		if (Candidates.IsEmpty() && bAllowBudgetFallback)
		{
			Candidates = BuildCandidates(CurrentBudget, true);
		}
		return Candidates;
	};

	TMap<TSubclassOf<AEnemyCharacterBase>, int32> WaveTypeSpawnCounts;
	auto GetCandidateWeight = [&](const FEnemyEntry& Entry) -> int32
	{
		if (!Entry.EnemyData || !Entry.EnemyData->EnemyClass)
		{
			return 0;
		}

		const int32 BaseScore = FMath::Max(1, Entry.EnemyData->DifficultyScore);
		const int32 LevelCount = LevelTypeSpawnCounts.FindRef(Entry.EnemyData->EnemyClass);
		const int32 WaveCount = WaveTypeSpawnCounts.FindRef(Entry.EnemyData->EnemyClass);
		const int32 RepeatPenalty = 100 + LevelCount * 75 + WaveCount * 125;
		return FMath::Max(1, (BaseScore * 100) / RepeatPenalty);
	};

	auto PickCandidate = [&](const TArray<FEnemyEntry>& Candidates) -> const FEnemyEntry&
	{
		int32 TotalWeight = 0;
		for (const FEnemyEntry& Candidate : Candidates)
		{
			TotalWeight += GetCandidateWeight(Candidate);
		}

		if (TotalWeight <= 0)
		{
			return Candidates[0];
		}

		int32 Roll = FMath::RandRange(1, TotalWeight);
		for (const FEnemyEntry& Candidate : Candidates)
		{
			Roll -= GetCandidateWeight(Candidate);
			if (Roll <= 0)
			{
				return Candidate;
			}
		}

		return Candidates.Last();
	};

	auto MakePlannedEnemy = [&](const FEnemyEntry& Chosen, FPlannedEnemy& OutPlanned) -> int32
	{
		TArray<TObjectPtr<URuneDataAsset>> SelectedEnemyBuffs;
		const int32 EnemyBuffCost = CollectEnemyBuffs(Chosen.EnemyData, SelectedEnemyBuffs);

		OutPlanned.EnemyClass         = Chosen.EnemyData->EnemyClass;
		OutPlanned.EnemyData          = Chosen.EnemyData;
		OutPlanned.EnemyBuffs         = SelectedEnemyBuffs;
		OutPlanned.PreSpawnFX         = Chosen.EnemyData->PreSpawnFX;
		OutPlanned.PreSpawnFXDuration = Chosen.EnemyData->PreSpawnFXDuration;
		OutPlanned.SpawnLifecycleFlow = Chosen.EnemyData->SpawnLifecycleFlow;

		LevelTypeSpawnCounts.FindOrAdd(Chosen.EnemyData->EnemyClass)++;
		WaveTypeSpawnCounts.FindOrAdd(Chosen.EnemyData->EnemyClass)++;
		TotalLevelPlannedEnemies++;

		return Chosen.EnemyData->DifficultyScore + RoomBuffCostPerEnemy + EnemyBuffCost;
	};

	// ---- Step 3: 用预算填充敌人，遵守类型上限和每波上限 ----
	bool bFirstEnemy = true;
	while (true)
	{
		// MaxEnemiesPerWave == 0 表示不限制
		if (MaxEnemiesPerWave > 0 && Plan.EnemiesToSpawn.Num() >= MaxEnemiesPerWave)
			break;
		if (!HasLevelSlot())
			break;

		TArray<FEnemyEntry> Candidates = BuildCandidatesWithFallback(RemainingBudget, bFirstEnemy);
		if (Candidates.IsEmpty()) break;

		FPlannedEnemy Planned;
		const int32 ActualCost = MakePlannedEnemy(PickCandidate(Candidates), Planned);
		Plan.EnemiesToSpawn.Add(Planned);

		RemainingBudget -= ActualCost;
		bFirstEnemy      = false;

		if (RemainingBudget <= 0) break;
	}

	// ---- Step 4: 剩余预算无法正常填充时，构建具体按需补刷队列 ----
	while (RemainingBudget > 0 && HasLevelSlot())
	{
		TArray<FEnemyEntry> Candidates = BuildCandidates(RemainingBudget, false);
		if (Candidates.IsEmpty())
		{
			break;
		}

		FPlannedEnemy Demand;
		const int32 ActualCost = MakePlannedEnemy(PickCandidate(Candidates), Demand);
		Plan.DemandEnemyPool.Add(Demand);
		RemainingBudget -= FMath::Max(1, ActualCost);
	}
	Plan.DemandCount = Plan.DemandEnemyPool.Num();

	UE_LOG(LogTemp, Log, TEXT("BuildWavePlan: 触发=%d 方式=%d 敌人数=%d 补刷=%d"),
		(int32)Plan.TriggerType, (int32)Plan.SpawnMode,
		Plan.EnemiesToSpawn.Num(), Plan.DemandCount);

	return Plan;
}

void AYogGameMode::TriggerNextWave()
{
	// 切换波次时清除上一波可能残留的补刷定时器
	GetWorld()->GetTimerManager().ClearTimer(DemandSpawnTimer);

	CurrentWaveIndex++;

	if (!WavePlans.IsValidIndex(CurrentWaveIndex))
	{
		bAllWavesSpawned = true;
		UE_LOG(LogTemp, Log, TEXT("TriggerNextWave: 所有波次已刷完，等待场内清空"));
		CheckLevelComplete();
		return;
	}

	FWavePlan& Wave = WavePlans[CurrentWaveIndex];
	Wave.TotalSpawnedInWave = 0;
	Wave.TotalKilledInWave  = 0;

	UE_LOG(LogTemp, Log, TEXT("TriggerNextWave: 开始第 %d 波，共 %d 只敌人"),
		CurrentWaveIndex + 1, Wave.EnemiesToSpawn.Num());

	// Wave 和 OneByOne 都走队列，区别在于每只之间的延迟：
	// Wave     → SpawnStaggerMin ~ SpawnStaggerMax 的随机错开
	// OneByOne → OneByOneInterval（固定间隔）
	bWaveStaggerMode = (Wave.SpawnMode == ESpawnMode::Wave);
	OneByOneSpawnQueue = Wave.EnemiesToSpawn;
	OneByOneSpawnIndex = 0;

	const float FirstDelay = FMath::FRandRange(
		SpawnStaggerMin,
		SpawnStaggerMax);

	GetWorld()->GetTimerManager().SetTimer(
		OneByOneTimer,
		this,
		&AYogGameMode::SpawnNextOneByOne,
		FirstDelay,
		false // 非重复，SpawnNextOneByOne 内部重新调度
	);
}

void AYogGameMode::SpawnNextOneByOne()
{
	if (!WavePlans.IsValidIndex(CurrentWaveIndex)) return;
	FWavePlan& Wave = WavePlans[CurrentWaveIndex];

	if (!OneByOneSpawnQueue.IsValidIndex(OneByOneSpawnIndex))
	{
		// 本波全部刷出，设置触发条件
		SetupWaveTrigger(Wave);
		return;
	}

	const bool bSpawned = BeginSpawnEnemyFromPool(OneByOneSpawnQueue[OneByOneSpawnIndex]);
	OneByOneSpawnIndex++;

	if (OneByOneSpawnIndex >= OneByOneSpawnQueue.Num())
	{
		// 队列已空，设置触发条件，不再重新调度
		SetupWaveTrigger(Wave);
		return;
	}

	// 找不到 Spawner 时立即处理下一只，不浪费等待时间
	if (!bSpawned)
	{
		SpawnNextOneByOne();
		return;
	}

	// 计算下一只的延迟：Wave 模式随机错开，OneByOne 固定间隔
	const float NextDelay = bWaveStaggerMode
		? FMath::FRandRange(SpawnStaggerMin, SpawnStaggerMax)
		: Wave.OneByOneInterval;

	GetWorld()->GetTimerManager().SetTimer(
		OneByOneTimer, this, &AYogGameMode::SpawnNextOneByOne, NextDelay, false);
}

void AYogGameMode::SetupWaveTrigger(const FWavePlan& Wave)
{
	// 最后一波：所有敌人已生成完毕，此时才标记并检查完成
	if (CurrentWaveIndex >= WavePlans.Num() - 1)
	{
		bAllWavesSpawned = true;
		CheckLevelComplete();
		return;
	}

	switch (Wave.TriggerType)
	{
		case ESpawnTriggerType::AllEnemiesDead:
			// 队列刷完后立即检查一次：
			// 若本波全部刷出失败（无 Spawner 支持），TotalAliveEnemies 已为 0，
			// 但不会再有敌人死亡事件触发 CheckWaveTrigger，需在此主动检查推进。
			CheckWaveTrigger();
			break;

		case ESpawnTriggerType::TimeInterval:
			GetWorld()->GetTimerManager().SetTimer(
				WaveTriggerTimer, this, &AYogGameMode::OnWaveTriggerFired, Wave.WaveTriggerInterval, false);
			break;

		case ESpawnTriggerType::PercentKilled_50:
		case ESpawnTriggerType::PercentKilled_20:
			// 由 CheckWaveTrigger 在 UpdateFinishLevel 中处理
			break;
	}
}

void AYogGameMode::OnWaveTriggerFired()
{
	TriggerNextWave();
}

void AYogGameMode::CheckWaveTrigger()
{
	if (bAllWavesSpawned) return;
	if (!WavePlans.IsValidIndex(CurrentWaveIndex)) return;
	if (CurrentWaveIndex >= WavePlans.Num() - 1) return; // 最后一波，不触发下一波

	FWavePlan& Wave = WavePlans[CurrentWaveIndex];

	// ---- 百分比/时间触发：不受补刷影响，直接判断 ----
	switch (Wave.TriggerType)
	{
		case ESpawnTriggerType::PercentKilled_50:
			if (Wave.TotalSpawnedInWave == 0) return; // 本波还未成功刷出任何怪，等待
			if (Wave.TotalKilledInWave >= FMath::CeilToInt(Wave.TotalSpawnedInWave * 0.5f))
			{
				GetWorld()->GetTimerManager().ClearTimer(DemandSpawnTimer);
				Wave.DemandCount = 0; // 触发下一波，取消剩余补刷
				TriggerNextWave();
			}
			return;

		case ESpawnTriggerType::PercentKilled_20:
			if (Wave.TotalSpawnedInWave == 0) return;
			if (Wave.TotalKilledInWave >= FMath::CeilToInt(Wave.TotalSpawnedInWave * 0.2f))
			{
				GetWorld()->GetTimerManager().ClearTimer(DemandSpawnTimer);
				Wave.DemandCount = 0;
				TriggerNextWave();
			}
			return;

		case ESpawnTriggerType::TimeInterval:
			// 保底：场内已无敌人时不等计时，立即触发下一波
			if (TotalAliveEnemies <= 0)
			{
				GetWorld()->GetTimerManager().ClearTimer(WaveTriggerTimer);
				TriggerNextWave();
			}
			return;

		default: break;
	}

	// ---- AllEnemiesDead 触发：结合按需补刷逻辑 ----
	if (TotalAliveEnemies <= 0)
	{
		if (Wave.DemandCount > 0 && !Wave.DemandEnemyPool.IsEmpty())
		{
			// 还有补刷配额，延迟刷出一只替补（1-3s 随机）
			const float Delay = FMath::FRandRange(
				SpawnStaggerMin,
				SpawnStaggerMax);
			GetWorld()->GetTimerManager().SetTimer(
				DemandSpawnTimer, this, &AYogGameMode::CheckDemandSpawn, Delay, false);
		}
		else
		{
			// 无补刷，触发下一波
			TriggerNextWave();
		}
	}
}

void AYogGameMode::CheckDemandSpawn()
{
	if (!WavePlans.IsValidIndex(CurrentWaveIndex)) return;
	FWavePlan& Wave = WavePlans[CurrentWaveIndex];

	if (Wave.DemandCount <= 0 || Wave.DemandEnemyPool.IsEmpty()) return;

	// 从已规划好的补刷队列中取一只，避免整关上限和类型上限在运行时被突破。
	const int32 DemandIndex = FMath::RandRange(0, Wave.DemandEnemyPool.Num() - 1);
	const FPlannedEnemy DemandPlanned = Wave.DemandEnemyPool[DemandIndex];
	Wave.DemandEnemyPool.RemoveAtSwap(DemandIndex);
	Wave.DemandCount = Wave.DemandEnemyPool.Num();

	if (BeginSpawnEnemyFromPool(DemandPlanned))
	{
		UE_LOG(LogTemp, Log, TEXT("CheckDemandSpawn: 补刷 %s，剩余补刷次数=%d"),
			DemandPlanned.EnemyClass ? *DemandPlanned.EnemyClass->GetName() : TEXT("?"), Wave.DemandCount);
	}
	// 若刷出失败（没有 Spawner），DemandCount 已减，避免死循环
}

void AYogGameMode::FallbackToPreplacedEnemies()
{
	// 波次系统未能初始化时（FloorTable 缺配置 / RoomData 为空），
	// 扫描场景中已存在的 AEnemyCharacterBase，将其存活数作为 TotalAliveEnemies，
	// 并标记 bAllWavesSpawned = true，使 CheckLevelComplete 能正常结算。
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacterBase::StaticClass(), FoundEnemies);

	int32 AliveCount = 0;
	for (AActor* Actor : FoundEnemies)
	{
		if (AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Actor))
		{
			if (!Enemy->bIsDead)
			{
				AliveCount++;

				for (const FBuffEntry& Entry : ActiveRoomBuffs)
				{
					ActivateEnemyRune(Enemy, Entry.RuneDA.Get(), TEXT("RoomBuff.Preplaced"));
				}
				if (UCharacterDataComponent* CDC = Enemy->GetCharacterDataComponent())
				{
					if (UEnemyData* ED = Cast<UEnemyData>(CDC->GetCharacterData()))
					{
						ActivateEnemyDataRunes(Enemy, ED, TEXT("EnemyBuff.Preplaced"));
					}
				}
			}
		}
	}

	TotalAliveEnemies = AliveCount;
	bAllWavesSpawned  = true;
	CurrentPhase      = ELevelPhase::Combat; // 确保结算阶段检查通过

	UE_LOG(LogTemp, Warning, TEXT("[FallbackToPreplacedEnemies] 场景预放置敌人 %d 只，bAllWavesSpawned=true"), AliveCount);

	// 若场景里根本没有敌人，直接进入整理阶段
	if (AliveCount == 0)
	{
		EnterArrangementPhase();
	}
}

void AYogGameMode::CheckLevelComplete()
{
	if (bForcedSurvivalActive)
	{
		return;
	}

	if (!bAllWavesSpawned || TotalAliveEnemies > 0 || PendingSpawnCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckLevelComplete] 未通过: bAllWavesSpawned=%s TotalAlive=%d PendingSpawn=%d"),
			bAllWavesSpawned ? TEXT("true") : TEXT("false"), TotalAliveEnemies, PendingSpawnCount);
		return;
	}

	// 最后一波可能有按需补刷剩余，先补刷，待全部死亡后再结算
	if (WavePlans.IsValidIndex(CurrentWaveIndex))
	{
		FWavePlan& Wave = WavePlans[CurrentWaveIndex];
		if (Wave.DemandCount > 0 && !Wave.DemandEnemyPool.IsEmpty())
		{
			const float Delay = FMath::FRandRange(
				SpawnStaggerMin,
				SpawnStaggerMax);
			GetWorld()->GetTimerManager().SetTimer(
				DemandSpawnTimer, this, &AYogGameMode::CheckDemandSpawn, Delay, false);
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CheckLevelComplete: 关卡完成！进入整理阶段"));
	OnFinishLevel.Broadcast();
	FinishLevelEvent.Broadcast();
	EnterArrangementPhase();
}

bool AYogGameMode::SpawnEnemyFromPool(const FPlannedEnemy& Planned)
{
	if (!Planned.EnemyClass) return false;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);
	if (OutActors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemyFromPool: 场景中没有 MobSpawner！"));
		return false;
	}

	// 只选包含该敌人 Class 的 Spawner（Spawner 的 EnemySpawnClassis 作为白名单）
	TArray<AMobSpawner*> ValidSpawners;
	for (AActor* Actor : OutActors)
	{
		if (AMobSpawner* S = Cast<AMobSpawner>(Actor))
		{
			if (Planned.bAllowAnySpawner || S->EnemySpawnClassis.Contains(Planned.EnemyClass))
				ValidSpawners.Add(S);
		}
	}

	if (ValidSpawners.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("SpawnEnemyFromPool: 没有 Spawner 支持 %s，跳过"), *Planned.EnemyClass->GetName());
		return false;
	}

	AMobSpawner* Spawner = ValidSpawners[FMath::RandRange(0, ValidSpawners.Num() - 1)];
	if (Spawner)
	{
		AEnemyCharacterBase* SpawnedEnemy = Spawner->SpawnMob(Planned.EnemyClass);
		if (SpawnedEnemy)
		{
			// 施加关卡 Buff（进关时选好的，对所有怪生效）
			for (const FBuffEntry& Entry : ActiveRoomBuffs)
			{
				ActivateEnemyRune(SpawnedEnemy, Entry.RuneDA.Get(), TEXT("RoomBuff.Spawn"));
			}
			// 施加敌人专属 Buff（BuildWavePlan 时选好的，只对此只怪生效）
			ActivateEnemyRunes(SpawnedEnemy, Planned.EnemyBuffs, TEXT("EnemyBuff.Spawn"));
			MarkStorySpecialRewardEnemy(SpawnedEnemy, Planned);
		}
		return SpawnedEnemy != nullptr;
	}
	return false;
}

void AYogGameMode::HandleLifecycleEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FBuffFlowLifecycleContext& Context)
{
	if (!SpawnedEnemy || Context.bSpawnFinalized)
	{
		return;
	}

	for (const FBuffEntry& Entry : ActiveRoomBuffs)
	{
		ActivateEnemyRune(SpawnedEnemy, Entry.RuneDA.Get(), TEXT("RoomBuff.LifecycleSpawn"));
	}
	ActivateEnemyRunes(SpawnedEnemy, Context.EnemyBuffs, TEXT("EnemyBuff.LifecycleSpawn"));

	if (WavePlans.IsValidIndex(Context.WaveIndex))
	{
		WavePlans[Context.WaveIndex].TotalSpawnedInWave++;
	}

	TotalAliveEnemies++;
	PendingSpawnCount = FMath::Max(0, PendingSpawnCount - 1);
	Context.bSpawnFinalized = true;
	CheckLevelComplete();
}

void AYogGameMode::HandleLifecycleEnemySpawnFailed(FBuffFlowLifecycleContext& Context)
{
	if (Context.bSpawnFinalized)
	{
		return;
	}

	PendingSpawnCount = FMath::Max(0, PendingSpawnCount - 1);
	Context.bSpawnFinalized = true;
	CheckLevelComplete();
}

bool AYogGameMode::BeginSpawnEnemyFromPool(const FPlannedEnemy& Planned)
{
	if (!Planned.EnemyClass) return false;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);

	TArray<AMobSpawner*> ValidSpawners;
	for (AActor* A : OutActors)
		if (AMobSpawner* S = Cast<AMobSpawner>(A))
			if (Planned.bAllowAnySpawner || S->EnemySpawnClassis.Contains(Planned.EnemyClass))
				ValidSpawners.Add(S);

	if (ValidSpawners.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginSpawnEnemyFromPool: 没有 Spawner 支持 %s，跳过"), *Planned.EnemyClass->GetName());
		return false;
	}

	AMobSpawner* Spawner = ValidSpawners[FMath::RandRange(0, ValidSpawners.Num() - 1)];
	FVector Location = Spawner->PrepareSpawnLocation();
	if (Location == FVector::ZeroVector) return false;

	if (Planned.SpawnLifecycleFlow)
	{
		PendingSpawnCount++;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABuffFlowLifecycleProxy* Proxy = GetWorld()->SpawnActor<ABuffFlowLifecycleProxy>(
			ABuffFlowLifecycleProxy::StaticClass(),
			Location,
			Spawner->GetActorRotation(),
			Params);
		if (!Proxy || !Proxy->GetBuffFlowComponent())
		{
			PendingSpawnCount = FMath::Max(0, PendingSpawnCount - 1);
			if (Proxy)
			{
				Proxy->Destroy();
			}
			CheckLevelComplete();
			return false;
		}

		FBuffFlowLifecycleContext Context;
		Context.Type = EBuffFlowLifecycleType::Spawn;
		Context.LifecycleTarget = Proxy;
		Context.Spawner = Spawner;
		Context.EnemyData = Planned.EnemyData;
		Context.EnemyClass = Planned.EnemyClass;
		Context.SpawnTransform = FTransform(Spawner->GetActorRotation(), Location);
		Context.EnemyBuffs = Planned.EnemyBuffs;
		Context.WaveIndex = CurrentWaveIndex;

		UBuffFlowComponent* ProxyBuffFlow = Proxy->GetBuffFlowComponent();
		ProxyBuffFlow->SetLifecycleContext(Context);
		ProxyBuffFlow->StartBuffFlow(Planned.SpawnLifecycleFlow, FGuid::NewGuid(), Spawner, true);
		return true;
	}

	const float FXDuration = FMath::Max(0.f,
		Planned.PreSpawnFXDuration + FMath::FRandRange(-Spawner->SpawnFXVariance, Spawner->SpawnFXVariance));

	if (Planned.PreSpawnFX)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Planned.PreSpawnFX, Location);

	PendingSpawnCount++;

	const int32 WaveIdx = CurrentWaveIndex;
	TWeakObjectPtr<AYogGameMode> WeakThis(this);
	TWeakObjectPtr<AMobSpawner>  WeakSpawner(Spawner);

	if (FXDuration <= 0.f)
	{
		FinishSpawnFromPool(Planned, WeakSpawner, Location, WaveIdx);
		return true;
	}

	FTimerHandle SpawnHandle;
	GetWorld()->GetTimerManager().SetTimer(SpawnHandle,
		FTimerDelegate::CreateLambda([WeakThis, Planned, WeakSpawner, Location, WaveIdx]()
		{
			if (WeakThis.IsValid())
				WeakThis->FinishSpawnFromPool(Planned, WeakSpawner, Location, WaveIdx);
		}),
		FXDuration, false);
	return true;
}

void AYogGameMode::FinishSpawnFromPool(FPlannedEnemy Planned,
	TWeakObjectPtr<AMobSpawner> WeakSpawner, FVector Location, int32 WaveIdx)
{
	PendingSpawnCount = FMath::Max(0, PendingSpawnCount - 1);

	if (!WeakSpawner.IsValid())
	{
		CheckLevelComplete();
		return;
	}

	AEnemyCharacterBase* SpawnedEnemy = WeakSpawner->SpawnMobAtLocation(Planned.EnemyClass, Location);
	if (SpawnedEnemy)
	{
		for (const FBuffEntry& Entry : ActiveRoomBuffs)
		{
			ActivateEnemyRune(SpawnedEnemy, Entry.RuneDA.Get(), TEXT("RoomBuff.DelayedSpawn"));
		}
		ActivateEnemyRunes(SpawnedEnemy, Planned.EnemyBuffs, TEXT("EnemyBuff.DelayedSpawn"));
		MarkStorySpecialRewardEnemy(SpawnedEnemy, Planned);

		if (WavePlans.IsValidIndex(WaveIdx))
			WavePlans[WaveIdx].TotalSpawnedInWave++;
		TotalAliveEnemies++;

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				FString::Printf(TEXT("[刷怪] 波次%d | 存活%d | %s（FX完成）"),
					WaveIdx + 1, TotalAliveEnemies, *Planned.EnemyClass->GetName()));
	}

	CheckLevelComplete();
}

void AYogGameMode::MarkStorySpecialRewardEnemy(AEnemyCharacterBase* Enemy, const FPlannedEnemy& Planned)
{
	if (!Enemy || !Planned.bSpecialRewardEnemy)
	{
		return;
	}

	TArray<FLootOption> RewardOptions = Planned.SpecialRewardOptions;
	if (RewardOptions.IsEmpty())
	{
		RewardOptions = StorySpecialRewardEnemyLootOptions;
	}

	Enemy->Tags.AddUnique(TEXT("Story.SpecialRewardEnemy"));

	if (Planned.SpecialRewardAuraFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Planned.SpecialRewardAuraFX,
			Enemy->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
	}

	if (RewardOptions.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Special reward enemy has no loot options. Enemy=%s"),
			*GetNameSafe(Enemy));
		return;
	}

	TWeakObjectPtr<AYogGameMode> WeakThis(this);
	Enemy->OnCharacterDiedNative.AddWeakLambda(this, [WeakThis, RewardOptions](AYogCharacterBase* DeadCharacter)
	{
		if (AYogGameMode* GameMode = WeakThis.Get())
		{
			GameMode->SpawnStorySpecialRewardPickup(DeadCharacter, RewardOptions);
		}
	});

	UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Special reward enemy marked. Enemy=%s LootCount=%d"),
		*GetNameSafe(Enemy),
		RewardOptions.Num());
}

void AYogGameMode::SpawnStorySpecialRewardPickup(AYogCharacterBase* DeadCharacter, const TArray<FLootOption> RewardOptions)
{
	if (!DeadCharacter || !RewardPickupClass || RewardOptions.IsEmpty())
	{
		return;
	}

	const FVector SpawnLoc = DeadCharacter->GetActorLocation();
	ARewardPickup* Pickup = GetWorld()->SpawnActor<ARewardPickup>(RewardPickupClass, SpawnLoc, FRotator::ZeroRotator);
	if (!Pickup)
	{
		return;
	}

	Pickup->bAllowPickupOutsideArrangement = true;
	Pickup->AssignLoot(RewardOptions);
	Pickup->RefreshPickupAvailability();
	Pickup->PlaySpawnFocusCue();

	UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Spawned special RewardPickup=%s at %s LootCount=%d"),
		*GetNameSafe(Pickup),
		*SpawnLoc.ToString(),
		RewardOptions.Num());
}

void AYogGameMode::StartForcedSurvivalEncounter()
{
	if (bForcedSurvivalActive)
	{
		return;
	}

	bForcedSurvivalActive = true;
	CurrentPhase = ELevelPhase::Combat;
	OnPhaseChanged.Broadcast(CurrentPhase);

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> PortalActors;
		UGameplayStatics::GetAllActorsOfClass(World, APortal::StaticClass(), PortalActors);
		for (AActor* PortalActor : PortalActors)
		{
			if (APortal* Portal = Cast<APortal>(PortalActor))
			{
				Portal->DisablePortal();
			}
		}

		World->GetTimerManager().SetTimer(
			ForcedSurvivalSpawnTimer,
			this,
			&AYogGameMode::SpawnForcedSurvivalEnemy,
			1.5f,
			true,
			0.1f);
	}

	UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Forced survival encounter started."));
}

void AYogGameMode::SpawnForcedSurvivalEnemy()
{
	if (!bForcedSurvivalActive || !ActiveRoomData)
	{
		return;
	}

	FPlannedEnemy Planned;
	for (const FEnemyEntry& Entry : ActiveRoomData->EnemyPool)
	{
		if (Entry.EnemyData && Entry.EnemyData->EnemyClass)
		{
			Planned.EnemyClass = Entry.EnemyData->EnemyClass;
			Planned.EnemyData = Entry.EnemyData;
			Planned.EnemyBuffs.Reset();
			CollectEnemyBuffs(Entry.EnemyData, Planned.EnemyBuffs);
			Planned.PreSpawnFX = Entry.EnemyData->PreSpawnFX;
			Planned.PreSpawnFXDuration = Entry.EnemyData->PreSpawnFXDuration;
			Planned.SpawnLifecycleFlow = Entry.EnemyData->SpawnLifecycleFlow;
			Planned.bAllowAnySpawner = true;
			break;
		}
	}

	if (!Planned.EnemyClass && !WavePlans.IsEmpty())
	{
		for (const FWavePlan& Wave : WavePlans)
		{
			if (!Wave.EnemiesToSpawn.IsEmpty())
			{
				Planned = Wave.EnemiesToSpawn[0];
				Planned.bAllowAnySpawner = true;
				break;
			}
		}
	}

	if (!Planned.EnemyClass)
	{
		if (UEnemyData* FallbackEnemyData = LoadFirstRunForcedSurvivalEnemyData())
		{
			Planned.EnemyClass = FallbackEnemyData->EnemyClass;
			Planned.EnemyData = FallbackEnemyData;
			Planned.EnemyBuffs.Reset();
			CollectEnemyBuffs(FallbackEnemyData, Planned.EnemyBuffs);
			Planned.PreSpawnFX = FallbackEnemyData->PreSpawnFX;
			Planned.PreSpawnFXDuration = FallbackEnemyData->PreSpawnFXDuration;
			Planned.SpawnLifecycleFlow = FallbackEnemyData->SpawnLifecycleFlow;
			Planned.bAllowAnySpawner = true;
			UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Forced survival using fallback enemy data %s."),
				*GetNameSafe(FallbackEnemyData));
		}
	}

	if (Planned.EnemyClass)
	{
		if (!BeginSpawnEnemyFromPool(Planned))
		{
			SpawnForcedSurvivalEnemyWithoutSpawner(Planned);
		}
	}
}

bool AYogGameMode::SpawnForcedSurvivalEnemyWithoutSpawner(const FPlannedEnemy& Planned)
{
	if (!bForcedSurvivalActive || !Planned.EnemyClass || !GetWorld())
	{
		return false;
	}

	APawn* PlayerPawn = nullptr;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PlayerPawn = PC->GetPawn();
	}

	const FVector Origin = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	const FVector Forward = PlayerPawn ? PlayerPawn->GetActorForwardVector() : FVector::ForwardVector;
	const FVector Right = PlayerPawn ? PlayerPawn->GetActorRightVector() : FVector::RightVector;
	const FVector SpawnLocation = Origin
		+ Forward.GetSafeNormal() * FMath::FRandRange(650.f, 900.f)
		+ Right.GetSafeNormal() * FMath::FRandRange(-350.f, 350.f)
		+ FVector(0.f, 0.f, 120.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AEnemyCharacterBase* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(
		Planned.EnemyClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		Params);
	if (!SpawnedEnemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Forced survival fallback spawn failed. Class=%s Location=%s"),
			*GetNameSafe(Planned.EnemyClass.Get()),
			*SpawnLocation.ToCompactString());
		return false;
	}

	if (!SpawnedEnemy->GetController())
	{
		SpawnedEnemy->SpawnDefaultController();
	}

	for (const FBuffEntry& Entry : ActiveRoomBuffs)
	{
		ActivateEnemyRune(SpawnedEnemy, Entry.RuneDA.Get(), TEXT("RoomBuff.ForcedSurvivalFallback"));
	}
	ActivateEnemyRunes(SpawnedEnemy, Planned.EnemyBuffs, TEXT("EnemyBuff.ForcedSurvivalFallback"));
	MarkStorySpecialRewardEnemy(SpawnedEnemy, Planned);
	RegisterEnemy(SpawnedEnemy);
	++TotalAliveEnemies;

	UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Forced survival fallback spawned %s without MobSpawner at %s."),
		*GetNameSafe(SpawnedEnemy),
		*SpawnLocation.ToCompactString());
	return true;
}

bool AYogGameMode::ShouldSpawnFirstRunInitialCardReward() const
{
	const UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr;
	if (!SaveSys || !SaveSys->IsFirstRunTutorialActive())
	{
		return false;
	}

	return CurrentFloor <= 1
		&& SaveSys->GetFirstRunTutorialStage() == static_cast<int32>(EFirstRunTutorialStage::None);
}

void AYogGameMode::SpawnFirstRunInitialCardRewardPickup(const FVector& BaseSpawnLocation)
{
	if (!RewardPickupClass || !ShouldSpawnFirstRunInitialCardReward() || !GetWorld())
	{
		return;
	}

	static const TCHAR* CandidateRunePaths[] =
	{
		FirstRunRewardSplashRunePath,
		FirstRunRewardKnockbackRunePath,
		FirstRunRewardBurnRunePath,
		FirstRunRewardPoisonRunePath,
	};

	TArray<URuneDataAsset*> CandidateRunes;
	for (const TCHAR* RunePath : CandidateRunePaths)
	{
		if (URuneDataAsset* RuneAsset = LoadObject<URuneDataAsset>(nullptr, RunePath))
		{
			CandidateRunes.Add(RuneAsset);
		}
	}

	if (CandidateRunes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Initial card reward skipped: no candidate rune assets loaded."));
		return;
	}

	TArray<FLootOption> CardOptions;
	const int32 DesiredOptionCount = FMath::Min(3, CandidateRunes.Num());
	for (int32 OptionIndex = 0; OptionIndex < DesiredOptionCount; ++OptionIndex)
	{
		const int32 RuneIndex = FMath::RandRange(0, CandidateRunes.Num() - 1);

		FLootOption CardOption;
		CardOption.LootType = ELootType::Rune;
		CardOption.RuneAsset = CandidateRunes[RuneIndex];
		CardOptions.Add(CardOption);

		CandidateRunes.RemoveAtSwap(RuneIndex);
	}

	if (CardOptions.IsEmpty())
	{
		return;
	}

	const FVector SpawnLocation = BaseSpawnLocation + FVector(250.f, 0.f, 0.f);
	ARewardPickup* CardPickup = GetWorld()->SpawnActor<ARewardPickup>(
		RewardPickupClass,
		SpawnLocation,
		FRotator::ZeroRotator);
	if (!CardPickup)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialDirector] Initial card reward spawn failed at %s."),
			*SpawnLocation.ToCompactString());
		return;
	}

	CardPickup->AssignLoot(CardOptions);
	CardPickup->PlaySpawnFocusCue();
	UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Initial card reward spawned %d options at %s."),
		CardOptions.Num(),
		*SpawnLocation.ToCompactString());
}

TArray<FBuffEntry> AYogGameMode::SelectRoomBuffs(const URoomDataAsset& Room, int32 BuffCount)
{
	TArray<FBuffEntry> Selected;
	if (Room.BuffPool.IsEmpty() || BuffCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Select none Room=%s BuffCount=%d Pool=%d"),
			*GetNameSafe(&Room),
			BuffCount,
			Room.BuffPool.Num());
		return Selected;
	}

	// 复制池子并洗牌（Fisher-Yates）
	TArray<FBuffEntry> Pool = Room.BuffPool;
	for (int32 i = Pool.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}

	for (int32 i = 0; i < Pool.Num() && Selected.Num() < BuffCount; i++)
	{
		if (Pool[i].RuneDA)
		{
			if (!ShouldApplyBuffEntry(Pool[i]))
			{
				UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Chance failed Room=%s Index=%d Rune=%s Chance=%.2f"),
					*GetNameSafe(&Room),
					i,
					*GetEnemyRuneDebugName(Pool[i].RuneDA.Get()),
					Pool[i].ApplyChance);
				continue;
			}

			Selected.Add(Pool[i]);
			UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Selected Room=%s Index=%d Rune=%s DA=%s Cost=%d Chance=%.2f"),
				*GetNameSafe(&Room),
				i,
				*GetEnemyRuneDebugName(Pool[i].RuneDA.Get()),
				*GetNameSafe(Pool[i].RuneDA.Get()),
				Pool[i].DifficultyScore,
				Pool[i].ApplyChance);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Selected null Room=%s Index=%d"),
				*GetNameSafe(&Room),
				i);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Select result Room=%s Requested=%d Selected=%d Pool=%d"),
		*GetNameSafe(&Room),
		BuffCount,
		Selected.Num(),
		Room.BuffPool.Num());

	return Selected;
}

// =========================================================
// 关卡流程
// =========================================================

int32 AYogGameMode::GetCurrentLootOptionCount() const
{
	int32 OptionCount = FMath::Clamp(DefaultLootOptionCount, 1, 3);

	const FGameplayTag EffectiveSingleChoiceTag = SingleChoiceLootGameplayTag.IsValid()
		? SingleChoiceLootGameplayTag
		: FGameplayTag::RequestGameplayTag(TEXT("Loot.Reward.SingleChoice"), false);

	if (EffectiveSingleChoiceTag.IsValid())
	{
		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
			UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		{
			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(EffectiveSingleChoiceTag))
				{
					OptionCount = FMath::Clamp(SingleChoiceLootOptionCount, 1, 3);
				}
			}
		}
	}

	return OptionCount;
}

TArray<FLootOption> AYogGameMode::ApplyLootOptionLimit(const TArray<FLootOption>& Options) const
{
	const int32 OptionCount = FMath::Min(GetCurrentLootOptionCount(), Options.Num());
	TArray<FLootOption> Limited;
	Limited.Reserve(OptionCount);
	for (int32 Index = 0; Index < OptionCount; ++Index)
	{
		Limited.Add(Options[Index]);
	}
	return Limited;
}

TArray<FLootOption> AYogGameMode::GenerateLootBatch(TSet<URuneDataAsset*>& AlreadyOffered)
{
	TArray<FLootOption> Batch;
	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] GM GenerateLootBatch start ActiveRoom=%s Campaign=%s AlreadyOffered=%d"),
		*GetNameSafe(ActiveRoomData),
		*GetNameSafe(GetActiveCampaignData()),
		AlreadyOffered.Num());

	// 确定符文源池
	const TArray<TObjectPtr<URuneDataAsset>>* SourcePool = nullptr;
	if (GetActiveCampaignData() && ActiveRoomData && !ActiveRoomData->LootPool.IsEmpty())
	{
		SourcePool = &ActiveRoomData->LootPool;
	}
	if (!SourcePool && !FallbackLootPool.IsEmpty())
	{
		SourcePool = &FallbackLootPool;
	}

	if (!SourcePool)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GenerateLootBatch] 无可用符文池，请在 GameMode BP 配置 FallbackLootPool"));
		return Batch;
	}

	// 去重 + 排除本关已分配过的符文
	TArray<URuneDataAsset*> Pool;
	TSet<URuneDataAsset*> Seen;
	for (const TObjectPtr<URuneDataAsset>& Asset : *SourcePool)
	{
		if (Asset && !Seen.Contains(Asset.Get()) && !AlreadyOffered.Contains(Asset.Get()))
		{
			Pool.Add(Asset.Get());
			Seen.Add(Asset.Get());
		}
	}

	// 排除玩家背包中已满级（Lv.III）的符文
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (Player->BackpackGridComponent)
		{
			TArray<FName> MaxLevelNames = Player->BackpackGridComponent->GetMaxLevelRuneNames();
			if (!MaxLevelNames.IsEmpty())
			{
				TSet<FName> MaxLevelSet(MaxLevelNames);
				Pool = Pool.FilterByPredicate([&](URuneDataAsset* DA)
				{
					return DA && !MaxLevelSet.Contains(DA->GetRuneName());
				});
			}
		}
	}

	// 洗牌
	for (int32 i = Pool.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}

	const int32 OptionsCount = FMath::Min(GetCurrentLootOptionCount(), Pool.Num());
	for (int32 i = 0; i < OptionsCount; i++)
	{
		FLootOption Option;
		Option.LootType  = ELootType::Rune;
		Option.RuneAsset = Pool[i];
		Batch.Add(Option);
		AlreadyOffered.Add(Pool[i]); // 写入已分配集合，下次调用时排除
	}

	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] GM GenerateLootBatch result Options=%s"),
		*DescribeGameModeLootOptionsForRewardDebug(Batch));
	return Batch;
}

void AYogGameMode::ShowLootOptions(const TArray<FLootOption>& Options)
{
	CurrentLootOptions = Options;
	bLootOptionsPending = true;
	OnLootGenerated.Broadcast(CurrentLootOptions);
}

TArray<FLootOption> AYogGameMode::GenerateIndependentLootOptions()
{
	TSet<URuneDataAsset*> Local;
	TArray<FLootOption> Batch = GenerateLootBatch(Local);
	CurrentLootOptions = Batch;
	bLootOptionsPending = true;
	return Batch;
}

void AYogGameMode::GenerateLootOptions()
{
	ShowLootOptions(GenerateIndependentLootOptions());
}

void AYogGameMode::TransitionToLevel(FName NextLevel, URoomDataAsset* NextRoom)
{
	if (NextLevel.IsNone()) return;
	const FName ResolvedNextLevel = ResolveRoomLevelNameForOpen(NextLevel, NextRoom);
	if (ResolvedNextLevel != NextLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("TransitionToLevel: resolved map [%s] -> [%s] for room [%s]"),
			*NextLevel.ToString(), *ResolvedNextLevel.ToString(), *GetNameSafe(NextRoom));
	}

	CurrentPhase = ELevelPhase::Transitioning;
	OnPhaseChanged.Broadcast(CurrentPhase);

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->PendingNextFloor = CurrentFloor + 1;

		// 写入下一关的房间配置（由传送门的骰子结果决定）
		GI->PendingRoomData = NextRoom;

		if (Player)
		{
			// 锁背包
			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				Backpack->SetLocked(true);
			}

			// 保存跑局状态
			FRunState NewState;
			NewState.bIsValid    = true;
			NewState.CurrentGold = Player->BackpackGridComponent ? Player->BackpackGridComponent->Gold : 0;

			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				NewState.CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());

				// 热度切关：默认归零，符文可保留一阶或两阶
				static const FGameplayTag TagRetainTwo = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.TwoPhase"), false);
				static const FGameplayTag TagRetainOne = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.OnePhase"), false);
				const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
				if (TagRetainTwo.IsValid() && ASC->HasMatchingGameplayTag(TagRetainTwo))
					NewState.CurrentHeat = MaxHeat * 2.f;
				else if (TagRetainOne.IsValid() && ASC->HasMatchingGameplayTag(TagRetainOne))
					NewState.CurrentHeat = MaxHeat;
				else
					NewState.CurrentHeat = 0.f;
			}

			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				NewState.CurrentPhase = Backpack->GetCurrentPhase();
				for (const FPlacedRune& PR : Backpack->GetAllPlacedRunes())
				{
					if (!PR.bIsPermanent)
					{
						NewState.PlacedRunes.Add(PR);
					}
				}
			}

			// 保存当前武器
			NewState.EquippedWeaponDef = Player->EquippedWeaponDef;

			// 保存整理阶段选出但尚未放入格子的符文
			NewState.PendingRunes = Player->PendingRunes;

			if (UCombatDeckComponent* CombatDeck = Player->CombatDeckComponent)
			{
				for (const FCombatCardInstance& Card : CombatDeck->GetFullDeckSnapshot())
				{
					if (Card.SourceData)
					{
						NewState.CombatDeckCards.Add(Card.SourceData);
						NewState.CombatDeckCardOrientations.Add(Card.LinkOrientation);
					}
				}
				NewState.CombatDeckShuffleCooldownDuration = CombatDeck->GetShuffleCooldownDuration();
				NewState.CombatDeckMaxActiveSequenceSize = CombatDeck->GetMaxActiveSequenceSize();
			}
			NewState.CompletedCombatBattleCount = CompletedCombatBattleCount;

			// 保存运行时隐藏被动符文（无形状、不进格子）
			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				NewState.HiddenPassiveRuneInstances = Backpack->GetRuntimeHiddenPassiveRunes();
			}

			// 保存献祭恩赐
			NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;
			NewState.SacrificeOfferingCosts = Player->GetSacrificeOfferingCosts();
			if (Player->ActiveSkillComponent)
			{
				for (UActiveSkillDataAsset* Skill : Player->ActiveSkillComponent->GetSkillLoadout())
				{
					NewState.SelectedSkillLoadout.Add(Skill);
				}
			}

			GI->PendingRunState = NewState;
			UE_LOG(LogTemp, Warning, TEXT("[RunState] SAVE (Portal) — HP=%.1f Gold=%d Phase=%d Runes=%d Weapon=%s Room=%s"),
				NewState.CurrentHP, NewState.CurrentGold, NewState.CurrentPhase, NewState.PlacedRunes.Num(),
				NewState.EquippedWeaponDef ? *NewState.EquippedWeaponDef->GetName() : TEXT("none"),
				NextRoom ? *NextRoom->GetName() : TEXT("null"));

			// 保存角色类，供下一关 HandleStartingNewPlayer_Implementation 重建玩家
			if (!GI->PersistentSaveData)
			{
				GI->PersistentSaveData = NewObject<UYogSaveGame>(GI);
			}
			GI->PersistentSaveData->SavedCharacterClass = Player->GetClass();
		}
	}

	UGameplayStatics::OpenLevel(GetWorld(), ResolvedNextLevel);
}

FGameplayTag AYogGameMode::RollRoomTypeForFloor(const FFloorConfig& Config)
{
	static const FGameplayTag EliteTag  = FGameplayTag::RequestGameplayTag(FName("Room.Type.Elite"));
	static const FGameplayTag ShopTag   = FGameplayTag::RequestGameplayTag(FName("Room.Type.Shop"));
	static const FGameplayTag EventTag  = FGameplayTag::RequestGameplayTag(FName("Room.Type.Event"));
	static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));

	if (Config.bForceElite) return EliteTag;

	const float Roll = FMath::FRand();
	if (Roll < Config.EliteChance)                                              return EliteTag;
	if (Roll < Config.EliteChance + Config.ShopChance)                         return ShopTag;
	if (Roll < Config.EliteChance + Config.ShopChance + Config.EventChance)    return EventTag;
	return NormalTag;
}

URoomDataAsset* AYogGameMode::SelectRoomByTag(
	const FPortalDestConfig* PortalDest, FGameplayTag RequiredTag)
{
	static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));

	// 层级过滤：Campaign 设置了 LayerTag 时，只选包含该 Tag 的房间
	UCampaignDataAsset* Campaign = GetActiveCampaignData();
	const FGameplayTag& LayerTag = Campaign ? Campaign->LayerTag : FGameplayTag::EmptyTag;

	auto PickByTag = [&](const TArray<TObjectPtr<URoomDataAsset>>& Pool) -> URoomDataAsset*
	{
		TArray<URoomDataAsset*> Candidates;
		for (const TObjectPtr<URoomDataAsset>& Room : Pool)
		{
			if (!Room) continue;
			if (!Room->RoomTags.HasTag(RequiredTag)) continue;
			// LayerTag 有效时，房间必须也包含该层级 Tag
			if (LayerTag.IsValid() && !Room->RoomTags.HasTag(LayerTag)) continue;
			Candidates.Add(Room);
		}
		return Candidates.IsEmpty()
			? nullptr
			: Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	};

	// 1. 传送门专属 RoomPool
	if (PortalDest && !PortalDest->RoomPool.IsEmpty())
	{
		if (URoomDataAsset* Found = PickByTag(PortalDest->RoomPool))
			return Found;
	}

	if (!Campaign) return nullptr;

	// 2. Campaign 全局 RoomPool（同类型 + 同层级）
	if (URoomDataAsset* Found = PickByTag(Campaign->RoomPool))
		return Found;

	// 3. 门专属池兜底：类型不匹配时无视类型随机取门专属池里的任意一个
	//    （优先于 Campaign 全局 Normal，避免回退到同一张地图）
	if (PortalDest && !PortalDest->RoomPool.IsEmpty())
	{
		TArray<URoomDataAsset*> AnyInPool;
		for (const TObjectPtr<URoomDataAsset>& Room : PortalDest->RoomPool)
		{
			if (Room) AnyInPool.Add(Room);
		}
		if (!AnyInPool.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("SelectRoomByTag: 类型 [%s] 无精确匹配，使用门专属池兜底"), *RequiredTag.ToString());
			return AnyInPool[FMath::RandRange(0, AnyInPool.Num() - 1)];
		}
	}

	// 4. Campaign 全局 Normal 兜底（门专属池为空时才走到这里）
	if (RequiredTag != NormalTag)
	{
		TArray<URoomDataAsset*> Fallback;
		for (const TObjectPtr<URoomDataAsset>& Room : Campaign->RoomPool)
		{
			if (!Room) continue;
			if (!Room->RoomTags.HasTag(NormalTag)) continue;
			if (LayerTag.IsValid() && !Room->RoomTags.HasTag(LayerTag)) continue;
			Fallback.Add(Room);
		}
		if (!Fallback.IsEmpty())
			return Fallback[FMath::RandRange(0, Fallback.Num() - 1)];
	}

	return nullptr;
}

void AYogGameMode::EnsureHubActiveSkillTerminal()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TCHAR* TerminalClassPath = TEXT("/Game/Code/Core/Hub/BP_HubActiveSkillTerminal.BP_HubActiveSkillTerminal_C");
	UClass* TerminalClass = StaticLoadClass(AHubFacilityActor::StaticClass(), nullptr, TerminalClassPath);
	if (!TerminalClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ActiveSkill] Hub terminal class missing: %s"), TerminalClassPath);
		return;
	}

	TArray<AActor*> ExistingTerminals;
	UGameplayStatics::GetAllActorsOfClass(World, TerminalClass, ExistingTerminals);
	if (ExistingTerminals.Num() > 0)
	{
		bool bAnyVisible = false;
		for (AActor* ExistingTerminal : ExistingTerminals)
		{
			if (ExistingTerminal && !ExistingTerminal->IsHidden())
			{
				bAnyVisible = true;
				break;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[ActiveSkill] Runtime hub terminal already exists. Count=%d AnyVisible=%d"),
			ExistingTerminals.Num(),
			bAnyVisible ? 1 : 0);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("BP_HubActiveSkillTerminal_Runtime");
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHubFacilityActor* Terminal = World->SpawnActor<AHubFacilityActor>(
		TerminalClass,
		FVector(320.0f, -260.0f, 90.0f),
		FRotator::ZeroRotator,
		SpawnParams);

	UE_LOG(LogTemp, Warning, TEXT("[ActiveSkill] Runtime hub terminal spawn %s at InitialRoom hub."),
		Terminal ? TEXT("OK") : TEXT("FAILED"));
}

void AYogGameMode::ActivateHubPortals()
{
	if (ShouldDelayInitialRoomPortalsUntilWeapon())
	{
		UE_LOG(LogTemp, Log, TEXT("ActivateHubPortals: delayed until the player equips a weapon."));
		return;
	}

	UCampaignDataAsset* Campaign = GetActiveCampaignData();
	UE_LOG(LogTemp, Warning, TEXT("[ActivateHubPortals] CampaignData=%s ActiveRoomData=%s IsHub=%d PortalDests=%d"),
		*GetNameSafe(Campaign),
		*GetNameSafe(ActiveRoomData),
		ActiveRoomData ? (int32)ActiveRoomData->bIsHubRoom : -1,
		ActiveRoomData ? ActiveRoomData->PortalDestinations.Num() : -1);

	if (!Campaign || !ActiveRoomData) return;
	if (ActiveRoomData->PortalDestinations.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateHubPortals: HubRoom [%s] 没有配置 PortalDestinations"), *ActiveRoomData->GetName());
		return;
	}

	// 目标是 FloorTable[0]（第一个战斗关），骰子决定房间类型
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	FStoryNextRoomPlan StoryPlan;
	const bool bHasStoryPlan = GI && GI->GetPendingStoryNextRoomPlan(StoryPlan);

	const FGameplayTag RequiredRoomType = Campaign->FloorTable.IsValidIndex(0)
		? RollRoomTypeForFloor(Campaign->FloorTable[0])
		: FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));

	UE_LOG(LogTemp, Log, TEXT("ActivateHubPortals: 第一关类型 = %s"), *RequiredRoomType.ToString());

	// 建立场景中 APortal 的 Index → Actor 映射
	TArray<AActor*> PortalActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), PortalActors);
	TMap<int32, APortal*> PortalMap;
	for (AActor* Actor : PortalActors)
	{
		if (APortal* Portal = Cast<APortal>(Actor))
			PortalMap.Add(Portal->Index, Portal);
	}

	int32 ForcedPortalIndex = StoryPlan.PortalIndex;
	const bool bForceSinglePortal = bHasStoryPlan && StoryPlan.bForceSinglePortal;
	if (bForceSinglePortal)
	{
		if (!ResolveUsableForcedPortalIndex(ActiveRoomData->PortalDestinations, PortalMap, ForcedPortalIndex, TEXT("ActivateHubPortals")))
		{
			return;
		}

		SealPortalsExcept(PortalMap, ForcedPortalIndex, TEXT("ActivateHubPortals"));
	}

	// 主城所有传送门全开（不走 50% 随机）
	for (const FPortalDestConfig& Cfg : ActiveRoomData->PortalDestinations)
	{
		if (bForceSinglePortal && Cfg.PortalIndex != ForcedPortalIndex)
		{
			UE_LOG(LogTemp, Log, TEXT("ActivateHubPortals: skipping portal [%d], story plan forces portal [%d]."),
				Cfg.PortalIndex, ForcedPortalIndex);
			continue;
		}

		APortal** Found = PortalMap.Find(Cfg.PortalIndex);
		if (!Found || !(*Found)) continue;

		URoomDataAsset* ChosenRoom = StoryPlan.RoomDataOverride
			? StoryPlan.RoomDataOverride.Get()
			: SelectRoomByTag(&Cfg, RequiredRoomType);
		if (!ChosenRoom || ChosenRoom->RoomName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivateHubPortals: 门[%d] 找不到合适的 DA_Room，跳过"), Cfg.PortalIndex);
			continue;
		}

		// 主城 → 第一关：用 FloorTable[0] 的难度分选下一关 Tier，预骰本门 Buff
		const FFloorConfig* NextConfig = Campaign->FloorTable.IsValidIndex(0)
			? &Campaign->FloorTable[0]
			: nullptr;
		const int32 NextScore = NextConfig ? NextConfig->TotalDifficultyScore : 30;
		const FRoomDifficultyTier& NextTier = ResolveTier(
			*ChosenRoom, NextScore, LowDifficultyScoreMax, HighDifficultyScoreMin);
		const TArray<FBuffEntry> PreRolled = (bHasStoryPlan && StoryPlan.bOverrideBuffs)
			? StoryPlan.BuffsOverride
			: SelectRoomBuffs(*ChosenRoom, NextTier.BuffCount);

		const FName LevelName = ResolveRoomLevelNameForOpen(ChosenRoom->RoomName, ChosenRoom);
		(*Found)->Open(LevelName, ChosenRoom, PreRolled);
		UE_LOG(LogTemp, Log, TEXT("ActivateHubPortals: 门[%d] 开启 → 关卡=%s 房间=%s 预骰Buff数=%d"),
			Cfg.PortalIndex, *LevelName.ToString(), *ChosenRoom->GetName(),
			PreRolled.Num());
	}

	// 主城无战斗 → EnterArrangementPhase 永不触发，需在此手动启用 HUD 引导
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->ShowPortalGuidance();
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 公共选档辅助（StartLevelSpawning 与 ActivatePortals 共用，避免漂移）
// ─────────────────────────────────────────────────────────────────────────────
const FRoomDifficultyTier& AYogGameMode::ResolveTier(const URoomDataAsset& Room, int32 TotalScore,
                                                     int32 LowMax, int32 HighMin)
{
	if (TotalScore <= LowMax)        return Room.LowDifficulty;
	if (TotalScore >= HighMin)       return Room.HighDifficulty;
	return Room.MediumDifficulty;
}

void AYogGameMode::ActivatePortals()
{
	UCampaignDataAsset* Campaign = GetActiveCampaignData();
	if (!Campaign || !ActiveRoomData) return;

	// 传送门目标配置现在存在 ActiveRoomData（当前关卡的 DA_Room）里
	if (ActiveRoomData->PortalDestinations.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivatePortals: 当前 RoomData [%s] 没有配置 PortalDestinations"), *ActiveRoomData->GetName());
		return;
	}

	// 下一关的 FloorConfig（所有门共享同一次类型骰子）
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	FStoryNextRoomPlan StoryPlan;
	const bool bHasStoryPlan = GI && GI->GetPendingStoryNextRoomPlan(StoryPlan);

	const int32 NextIdx = CurrentFloor; // CurrentFloor 是 1-based，NextIdx 是下一关的 0-based 下标
	const FFloorConfig* NextConfig = Campaign->FloorTable.IsValidIndex(NextIdx)
		? &Campaign->FloorTable[NextIdx]
		: nullptr;

	// 骰一次类型：所有门的目标房间类型相同（保证下一关体验一致）
	static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));
	const FGameplayTag RequiredRoomType = NextConfig
		? RollRoomTypeForFloor(*NextConfig)
		: NormalTag;

	UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 下一关类型 = %s"), *RequiredRoomType.ToString());

	// 收集场景中所有 APortal，建立 Index → APortal* 映射
	TArray<AActor*> PortalActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), PortalActors);

	TMap<int32, APortal*> PortalMap;
	for (AActor* Actor : PortalActors)
	{
		if (APortal* Portal = Cast<APortal>(Actor))
			PortalMap.Add(Portal->Index, Portal);
	}

	// Fisher-Yates 洗牌 PortalDestinations，保证第一个必开
	TArray<FPortalDestConfig> Configs = ActiveRoomData->PortalDestinations;
	for (int32 i = Configs.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Configs.Swap(i, j);
	}

	bool bAtLeastOneOpened = false;
	const bool bStoryForcesSinglePortal = bHasStoryPlan && StoryPlan.bForceSinglePortal;
	const bool bForceSinglePortal = bStoryForcesSinglePortal || bHasForcedPortalOverride || ActiveRoomData->bForceSinglePortal;
	int32 ForcedPortalIndex = bStoryForcesSinglePortal
		? StoryPlan.PortalIndex
		: bHasForcedPortalOverride
		? ForcedPortalOverrideIndex
		: ActiveRoomData->ForcedPortalIndex;
	if (bForceSinglePortal)
	{
		if (!ResolveUsableForcedPortalIndex(Configs, PortalMap, ForcedPortalIndex, TEXT("ActivatePortals")))
		{
			return;
		}

		SealPortalsExcept(PortalMap, ForcedPortalIndex, TEXT("ActivatePortals"));
	}

	for (const FPortalDestConfig& Cfg : Configs)
	{
		if (bForceSinglePortal && Cfg.PortalIndex != ForcedPortalIndex)
		{
			UE_LOG(LogTemp, Log, TEXT("ActivatePortals: skipping portal [%d], tutorial forces portal [%d]."),
				Cfg.PortalIndex, ForcedPortalIndex);
			continue;
		}

		APortal** Found = PortalMap.Find(Cfg.PortalIndex);
		if (!Found || !(*Found)) continue;

		// 先查此门专属 RoomPool，再查 Campaign 全局，最后退化为 Normal
		URoomDataAsset* ChosenRoom = StoryPlan.RoomDataOverride
			? StoryPlan.RoomDataOverride.Get()
			: SelectRoomByTag(&Cfg, RequiredRoomType);

		if (!ChosenRoom)
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivatePortals: 门[%d] 找不到可用 DA_Room，跳过"), Cfg.PortalIndex);
			continue;
		}

		// RoomName 即关卡文件名，直接使用
		const FName LevelName = ResolveRoomLevelNameForOpen(ChosenRoom->RoomName, ChosenRoom);
		if (LevelName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivatePortals: 门[%d] DA_Room [%s] 的 RoomName 为空，跳过"),
				Cfg.PortalIndex, *ChosenRoom->GetName());
			continue;
		}

		// 洗牌后第一个必开；后续 50% 概率
		const bool bShouldOpen = bForceSinglePortal || !bAtLeastOneOpened || FMath::RandBool();
		if (!bShouldOpen)
		{
			(*Found)->MarkUnavailableForPreview(LevelName, ChosenRoom);
			UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 随机未开启"), Cfg.PortalIndex);
			continue;
		}

		// 预骰下一关 Buff（每扇门各自缓存；玩家确认进入时由 Portal::TryEnter 写入 GI）
		const int32 NextScore = NextConfig ? NextConfig->TotalDifficultyScore : 30;
		const FRoomDifficultyTier& NextTier = ResolveTier(
			*ChosenRoom, NextScore, LowDifficultyScoreMax, HighDifficultyScoreMin);
		const TArray<FBuffEntry> PreRolled = (bHasStoryPlan && StoryPlan.bOverrideBuffs)
			? StoryPlan.BuffsOverride
			: SelectRoomBuffs(*ChosenRoom, NextTier.BuffCount);

		(*Found)->Open(LevelName, ChosenRoom, PreRolled);
		bAtLeastOneOpened = true;

		UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 开启 → 关卡=%s 房间=%s 预骰Buff数=%d"),
			Cfg.PortalIndex, *LevelName.ToString(), *ChosenRoom->GetName(), PreRolled.Num());
	}

}

// ─────────────────────────────────────────────────────────────────────────────
// 敌人注册表（相机战斗感知）
// ─────────────────────────────────────────────────────────────────────────────

bool AYogGameMode::ShouldDelayInitialRoomPortalsUntilWeapon() const
{
	if (!bInitialRoomPortalsRequireWeapon || !ActiveRoomData || !ActiveRoomData->bIsHubRoom)
	{
		return false;
	}

	return !PlayerHasEquippedWeapon(GetWorld());
}

void AYogGameMode::NotifyPlayerWeaponEquipped(APlayerCharacterBase* Player)
{
	if (!Player || !ActiveRoomData || !ActiveRoomData->bIsHubRoom)
	{
		return;
	}

	if (bInitialRoomPortalsRequireWeapon && (Player->EquippedWeaponDef || Player->EquippedWeaponInstance))
	{
		ActivateHubPortals();
	}
}

void AYogGameMode::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	if (!IsValid(Enemy)) return;

	// 去重（防止同一敌人注册两次）
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (W.Get() == Enemy) return;
	}

	// 清理失效条目（防止数组无限增长）
	AliveEnemies.RemoveAllSwap([](const TWeakObjectPtr<AEnemyCharacterBase>& W)
	{
		return !W.IsValid();
	});

	AliveEnemies.Add(Enemy);
}

void AYogGameMode::UnregisterEnemy(AEnemyCharacterBase* Enemy)
{
	AliveEnemies.RemoveAllSwap([Enemy](const TWeakObjectPtr<AEnemyCharacterBase>& W)
	{
		return !W.IsValid() || W.Get() == Enemy;
	});
}

bool AYogGameMode::HasAliveEnemies() const
{
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (E->IsAlive()) return true;
		}
	}
	return false;
}

FVector AYogGameMode::GetEnemyCentroid(FVector Origin, float WithinRadius) const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	const float RadiusSq = WithinRadius * WithinRadius;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			const float DistSq = FVector::DistSquared(E->GetActorLocation(), Origin);
			if (DistSq <= RadiusSq)
			{
				Sum += E->GetActorLocation();
				Count++;
			}
		}
	}

	return (Count > 0) ? (Sum / static_cast<float>(Count)) : Origin;
}

FVector AYogGameMode::GetNearestEnemyDirection(FVector Origin) const
{
	float MinDistSq = FLT_MAX;
	FVector NearestDir = FVector::ZeroVector;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			const float DistSq = FVector::DistSquared(E->GetActorLocation(), Origin);
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				NearestDir = (E->GetActorLocation() - Origin).GetSafeNormal();
			}
		}
	}
	return NearestDir;
}

TArray<AEnemyCharacterBase*> AYogGameMode::GetNearbyEnemies(FVector Origin, float WithinRadius) const
{
	TArray<AEnemyCharacterBase*> Result;
	const float RadiusSq = WithinRadius * WithinRadius;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			if (FVector::DistSquared(E->GetActorLocation(), Origin) <= RadiusSq)
			{
				Result.Add(E);
			}
		}
	}
	return Result;
}

TArray<AEnemyCharacterBase*> AYogGameMode::GetAllAliveEnemies() const
{
	TArray<AEnemyCharacterBase*> Result;
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get(); E && E->IsAlive())
			Result.Add(E);
	}
	return Result;
}


FVector AYogGameMode::FindLootSpawnLocation(APawn* PlayerPawn, APlayerController* PC) const
{
	if (!PlayerPawn || !PC) return FVector::ZeroVector;

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const float   SpawnDist = 200.f;

	// 前方优先，依次尝试 8 个方向（步进 45°）
	static const float AngleDegrees[] = { 0.f, 45.f, -45.f, 90.f, -90.f, 135.f, -135.f, 180.f };

	FVector Forward = PlayerPawn->GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.IsNearlyZero()) Forward.Normalize();

	FVector  CamLoc; FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector2D ViewportSize(1920.f, 1080.f);
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	const float MarginX = ViewportSize.X * 0.05f;
	const float MarginY = ViewportSize.Y * 0.05f;

	FCollisionQueryParams TraceParams(TEXT("LootSpawnFind"), false, PlayerPawn);

	for (float AngleDeg : AngleDegrees)
	{
		const FVector Dir       = Forward.RotateAngleAxis(AngleDeg, FVector::UpVector);
		const FVector Candidate = PlayerLoc + Dir * SpawnDist;

		FHitResult Hit;

		// 1. 玩家到候选点之间无墙
		if (GetWorld()->LineTraceSingleByChannel(Hit, PlayerLoc, Candidate, ECC_Visibility, TraceParams))
			continue;

		// 2. 相机到候选点之间无遮挡
		if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, Candidate, ECC_Visibility, TraceParams))
			continue;

		// 3. 候选点在屏幕范围内（留 5% 边缘余量）
		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(Candidate, ScreenPos)) continue;
		if (ScreenPos.X < MarginX || ScreenPos.X > ViewportSize.X - MarginX) continue;
		if (ScreenPos.Y < MarginY || ScreenPos.Y > ViewportSize.Y - MarginY) continue;

		return Candidate;
	}

	// 全部候选失败，退回玩家原位（极少发生）
	UE_LOG(LogTemp, Warning, TEXT("[LootSpawn] 所有方向被遮挡，退回玩家位置"));
	return PlayerLoc;
}

// =========================================================
// 关卡生命周期事件总线
// =========================================================

bool AYogGameMode::IsOneShotEvent(EGameLifecycleEvent Event)
{
	switch (Event)
	{
	case EGameLifecycleEvent::FirstRuneAcquired:
	case EGameLifecycleEvent::FirstRunePlaced:
	case EGameLifecycleEvent::HeatPhaseEntered:
	case EGameLifecycleEvent::PlayerDeath:
	case EGameLifecycleEvent::GameStart:
		return true;
	default:
		return false;
	}
}

void AYogGameMode::TriggerLifecycleEvent(EGameLifecycleEvent Event)
{
	// 一次性事件去重（提前查询，但延后到"成功启动 Flow"才记录，
	// 否则未配 LFA 时会永久吞掉事件，将来配了也不再触发）
	const bool bIsOneShot = IsOneShotEvent(Event);
	if (bIsOneShot && FiredOnceEvents.Contains(Event))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[LifecycleEvents] 事件 %d 已触发过，忽略"), (int32)Event);
		return;
	}

	TObjectPtr<ULevelFlowAsset>* AssetPtr = LifecycleEventFlows.Find(Event);
	if (!AssetPtr || !AssetPtr->Get())
	{
		// 没配映射 = 沉默（用户不想要这个事件的表演），且不写 FiredOnceEvents 留给后续配置生效
		UE_LOG(LogTemp, Log, TEXT("[LifecycleEvents] TriggerLifecycleEvent(%d) 未配 LFA，跳过"), (int32)Event);
		return;
	}

	if (!LifecycleFlowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LifecycleEvents] LifecycleFlowComponent 为空，无法跑 Flow"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LifecycleEvents] TriggerLifecycleEvent(%d) -> %s"),
		(int32)Event, *AssetPtr->Get()->GetName());

	// 停旧 Flow 再启新 Flow（同一时间只跑一个）
	LifecycleFlowComponent->FinishRootFlow(LifecycleFlowComponent->RootFlow, EFlowFinishPolicy::Keep);
	LifecycleFlowComponent->RootFlow = AssetPtr->Get();
	LifecycleFlowComponent->StartRootFlow();

	// 成功启动 Flow 之后再记录，避免静默吞事件
	if (bIsOneShot)
		FiredOnceEvents.Add(Event);
}

bool AYogGameMode::RunStoryLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow)
{
	if (!FlowAsset)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StoryEvent] RunStoryLevelFlow skipped: FlowAsset is null"));
		return false;
	}

	if (!LifecycleFlowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEvent] RunStoryLevelFlow failed: LifecycleFlowComponent is null"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[StoryEvent] RunStoryLevelFlow -> %s"), *FlowAsset->GetName());
	if (bStopExistingFlow)
	{
		LifecycleFlowComponent->FinishRootFlow(LifecycleFlowComponent->RootFlow, EFlowFinishPolicy::Keep);
	}
	LifecycleFlowComponent->RootFlow = FlowAsset;
	LifecycleFlowComponent->StartRootFlow();
	return true;
}

void AYogGameMode::HandleLevelEndEffectFinished()
{
	// HUD 揭幕动画完成 = 玩家视觉回到正常 = 适合弹教程的时机
	TriggerLifecycleEvent(EGameLifecycleEvent::LevelClearRevealed);
}
