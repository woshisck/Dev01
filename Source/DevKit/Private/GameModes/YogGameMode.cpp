// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/YogGameMode.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/YogPlayerControllerBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Data/RoomDataAsset.h"
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
#include "Component/CharacterDataComponent.h"
#include "Map/Portal.h"
#include "Map/RewardPickup.h"
#include "UI/LootSelectionWidget.h"
#include "Tutorial/TutorialManager.h"
#include "UI/YogHUD.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "Misc/PackageName.h"

namespace
{
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

	return RequestedLevel;
}
}

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
	DefaultPawnClass = nullptr;

	// 事件总线统一使用的 FlowComponent，同时只跑一个 Flow（新触发停旧的）
	LifecycleFlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("LifecycleFlowComponent"));
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

	if (CampaignData)
	{
		StartLevelSpawning();
	}
	else
	{
		const FString MapName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
		if (!MapName.Equals(TEXT("Entry"), ESearchCase::IgnoreCase))
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
	UE_LOG(LogTemp, Log, TEXT("MonsterKillCount: %d"), this->MonsterKillCount);

	// ---- 新刷怪系统 ----
	if (CampaignData)
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

void AYogGameMode::EnterArrangementPhase()
{
	if (CurrentPhase != ELevelPhase::Combat)
		return;

	CurrentPhase = ELevelPhase::Arrangement;
	OnPhaseChanged.Broadcast(CurrentPhase);

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (Player)
	{
		// 解锁背包
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Backpack->SetLocked(false);
		}

		// 发放金币（按当前关卡 FloorConfig 中的范围）
		if (CampaignData && ActiveGoldMax > 0)
		{
			const int32 GoldReward = FMath::RandRange(ActiveGoldMin, ActiveGoldMax);
			if (Player->BackpackGridComponent)
				Player->BackpackGridComponent->AddGold(GoldReward);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 发放金币 %d"), GoldReward);
		}
	}

	// 重置本关已分配符文的追踪集合（多拾取物去重用）
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
	if (RewardPickupClass)
	{
		FVector SpawnLoc = LootSpawnLoc;
		if (!SpawnLoc.IsZero())
		{
			AActor* Spawned = GetWorld()->SpawnActor<AActor>(RewardPickupClass, SpawnLoc, FRotator::ZeroRotator);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 生成 RewardPickup @ %s"), *SpawnLoc.ToString());

			// 预分配战利品选项，拾取时直接展示，无需再次生成
			if (ARewardPickup* Pickup = Cast<ARewardPickup>(Spawned))
			{
				TArray<FLootOption> Batch = GenerateLootBatch(LootAssignedThisLevel);
				Pickup->AssignLoot(Batch);
				UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 预分配 %d 个符文选项给 RewardPickup"), Batch.Num());
			}
		}
	}

	// 献祭恩赐额外掉落（非主城关卡，15% 概率）
	const bool bIsHubRoom = ActiveRoomData && ActiveRoomData->bIsHubRoom;
	if (!bIsHubRoom && SacrificePickupClass && SacrificeGracePool.Num() > 0
		&& FMath::FRand() < SacrificeDropChance)
	{
		const int32 ChosenIdx = FMath::RandRange(0, SacrificeGracePool.Num() - 1);
		USacrificeGraceDA* ChosenDA = SacrificeGracePool[ChosenIdx];

		// 在普通奖励旁边偏移 250cm 处生成
		FVector SacrificeSpawnLoc = LootSpawnLoc + FVector(250.f, 0.f, 0.f);

		AActor* SacrificePickup = GetWorld()->SpawnActor<AActor>(
			SacrificePickupClass, SacrificeSpawnLoc, FRotator::ZeroRotator);

		// 把选中的 DA 注入拾取物（拾取物需实现 SetSacrificeGraceDA BlueprintNativeEvent 或公开 UPROPERTY）
		if (SacrificePickup)
		{
			if (UFunction* SetDA = SacrificePickup->FindFunction(TEXT("SetSacrificeGraceDA")))
			{
				struct { USacrificeGraceDA* DA; } Params{ ChosenDA };
				SacrificePickup->ProcessEvent(SetDA, &Params);
			}
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 献祭恩赐掉落 [%s] @ %s"),
				*ChosenDA->GetName(), *SacrificeSpawnLoc.ToString());
		}
	}

	// 开启传送门
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
	if (Chosen.LootType == ELootType::Rune && Chosen.RuneAsset)
	{
		Player->AddRuneToInventory(Chosen.RuneAsset->CreateInstance());
	}

	// 战斗后教程：符文选完后引导玩家配置背包
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
	{
		if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(
			UGameplayStatics::GetPlayerController(GetWorld(), 0)))
		{
			TM->TryPostCombatTutorial(PC);
		}
	}

	// 通知 LevelFlow 节点：玩家已选符文
	OnLootSelected.Broadcast();
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

				NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;

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
	if (bGameOverTriggered)
	{
		return;
	}

	bGameOverTriggered = true;
	CurrentPhase = ELevelPhase::Transitioning;
	OnPhaseChanged.Broadcast(CurrentPhase);

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(WaveTriggerTimer);
	TM.ClearTimer(OneByOneTimer);
	TM.ClearTimer(InitialSpawnDelayTimer);
	TM.ClearTimer(DemandSpawnTimer);

	TriggerLifecycleEvent(EGameLifecycleEvent::PlayerDeath);

	if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetShowMouseCursor(false);
	}

	TM.SetTimer(
		PlayerDeathGameOverTimer,
		this,
		&AYogGameMode::FinishPlayerDeathGameOver,
		FMath::Max(0.f, PlayerDeathGameOverDelay),
		false);

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Player died. Game over will finalize in %.2fs."), PlayerDeathGameOverDelay);
}

void AYogGameMode::FinishPlayerDeathGameOver()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ShowGameOverScreen();
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Game paused after player death."));
}

// =========================================================
// 新刷怪系统实现
// =========================================================

void AYogGameMode::StartLevelSpawning()
{
	const FString CurrentMapNameForFrontend = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	if (CurrentMapNameForFrontend.Equals(TEXT("Entry"), ESearchCase::IgnoreCase))
	{
		CurrentPhase = ELevelPhase::Transitioning;
		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: Entry frontend map detected, gameplay spawning is skipped."));
		return;
	}

	if (!CampaignData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: CampaignData 未配置，跳过新刷怪系统"));
		return;
	}

	// 从 GameInstance 读取上一关存储的楼层推进（切关后 GameMode 重建，CurrentFloor 默认为 1）
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (GI && GI->PendingNextFloor > 1)
	{
		CurrentFloor = GI->PendingNextFloor;
	}

	// FloorTable 下标从 0 开始，CurrentFloor 从 1 开始
	const int32 TableIndex = CurrentFloor - 1;
	if (!CampaignData->FloorTable.IsValidIndex(TableIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: FloorTable 没有第 %d 关的配置，降级为场景预放置敌人统计"), CurrentFloor);
		FallbackToPreplacedEnemies();
		return;
	}

	const FFloorConfig& Config = CampaignData->FloorTable[TableIndex];

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
		if (CurrentFloor == 1 && CampaignData->DefaultStartingRoom)
		{
			const FName DefaultRoomName = CampaignData->DefaultStartingRoom->RoomName;
			const FName DefaultLevelName = ResolveRoomLevelNameForOpen(DefaultRoomName, CampaignData->DefaultStartingRoom);
			if (!DefaultLevelName.IsNone())
			{
				// 检查当前加载的关卡是否已经是 DefaultStartingRoom 指定的关卡
				// GetCurrentLevelName(true) 会去掉 PIE 前缀（如 "UEDPIE_0_"）
				const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
				if (!CurrentMapName.Equals(FPackageName::GetShortName(DefaultLevelName.ToString()), ESearchCase::IgnoreCase))
				{
					// 当前关卡不匹配，重定向到 DefaultStartingRoom 的关卡
					UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 当前关卡 [%s] ≠ DefaultStartingRoom [%s]，重定向..."),
						*CurrentMapName, *DefaultLevelName.ToString());
					if (GI) GI->PendingRoomData = CampaignData->DefaultStartingRoom;
					UGameplayStatics::OpenLevel(GetWorld(), DefaultLevelName);
					return;
				}
			}

			ActiveRoomData = CampaignData->DefaultStartingRoom;
			UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 DefaultStartingRoom = %s"), *ActiveRoomData->GetName());
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
	if (ActiveRoomData->bIsHubRoom)
	{
		// Hub 视为第 0 关，TransitionToLevel 写入 PendingNextFloor = 0+1 = 1
		// → 下一关读 FloorTable[0]（第一个战斗关）
		CurrentFloor = 0;
		CurrentPhase = ELevelPhase::Arrangement; // 跳过战斗阶段

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
					Portal->bWillNeverOpen = true;
					Portal->NeverOpen();
				}
			}
		}

		ActivateHubPortals();
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
				Portal->bWillNeverOpen = true;
				Portal->NeverOpen();
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

	// 读取当前难度档位的 MaxEnemiesPerWave（GenerateWavePlans 调用前 ActiveRoomData 已缓存）
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
		const int32 Budget = BasePerWave + (i == 0 ? Remainder : 0);
		WavePlans.Add(BuildWavePlan(Budget, Room, Tier.MaxEnemiesPerWave));
	}
}

// EnemyData.EnemyBuffPool is the list of runes this enemy carries by default.
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

		OutBuffs.AddUnique(Entry.RuneDA);
		TotalCost += Entry.DifficultyScore;
	}
	return TotalCost;
}

static int32 CalcEnemyBuffCost(UEnemyData* EnemyData)
{
	TArray<TObjectPtr<URuneDataAsset>> IgnoredBuffs;
	return CollectEnemyBuffs(EnemyData, IgnoredBuffs);
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

static FString GetEnemyRuneDebugName(const URuneDataAsset* RuneDA)
{
	if (!RuneDA)
		return TEXT("None");

	const FName RuneName = RuneDA->RuneInfo.RuneConfig.RuneName;
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

	UFlowAsset* FlowAsset = RuneDA->RuneInfo.Flow.FlowAsset;
	const ERuneTriggerType TriggerType = RuneDA->RuneInfo.RuneConfig.TriggerType;
	const FString RuneDebugName = GetEnemyRuneDebugName(RuneDA);

	if (!FlowAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing Flow Enemy=%s Source=%s Rune=%s DA=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(RuneDA), GetEnemyRuneTriggerName(TriggerType));
		return;
	}

	UBuffFlowComponent* BFC = Enemy->BuffFlowComponent;
	if (!BFC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing BuffFlowComponent Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(RuneDA), *GetNameSafe(FlowAsset), GetEnemyRuneTriggerName(TriggerType));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Granted Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s"),
		*GetNameSafe(Enemy),
		Source,
		*RuneDebugName,
		*GetNameSafe(RuneDA),
		*GetNameSafe(FlowAsset),
		GetEnemyRuneTriggerName(TriggerType));

	if (TriggerType == ERuneTriggerType::Passive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] StartPassive Enemy=%s Source=%s Rune=%s Flow=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset));
		BFC->StartBuffFlow(FlowAsset, FGuid::NewGuid(), Enemy);
		return;
	}

	UAbilitySystemComponent* ASC = Enemy->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Missing ASC Enemy=%s Source=%s Rune=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset), GetEnemyRuneTriggerName(TriggerType));
		return;
	}

	const FGameplayTag EventTag = GetEnemyRuneEventTagForTriggerType(TriggerType);
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Skip: Invalid EventTag Enemy=%s Source=%s Rune=%s Flow=%s Trigger=%s"),
			*GetNameSafe(Enemy), Source, *RuneDebugName, *GetNameSafe(FlowAsset), GetEnemyRuneTriggerName(TriggerType));
		return;
	}

	TWeakObjectPtr<AEnemyCharacterBase> WeakEnemy = Enemy;
	TWeakObjectPtr<UBuffFlowComponent> WeakBFC = BFC;
	TWeakObjectPtr<UFlowAsset> WeakFlowAsset = FlowAsset;
	const FString CapturedRuneName = RuneDebugName;
	const FString CapturedRuneDAName = GetNameSafe(RuneDA);
	const FString CapturedFlowName = GetNameSafe(FlowAsset);
	const FString CapturedSource = Source;
	const FString CapturedTriggerName = GetEnemyRuneTriggerName(TriggerType);
	ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag)
		.AddWeakLambda(BFC, [WeakEnemy, WeakBFC, WeakFlowAsset, EventTag, CapturedRuneName, CapturedRuneDAName, CapturedFlowName, CapturedSource, CapturedTriggerName](const FGameplayEventData* Payload)
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

			CapturedBFC->StartBuffFlow(CapturedFlow, FGuid::NewGuid(), CapturedEnemy, true);
		});

	UE_LOG(LogTemp, Warning, TEXT("[EnemyRune] Registered Enemy=%s Source=%s Rune=%s DA=%s Flow=%s Trigger=%s Event=%s"),
		*GetNameSafe(Enemy),
		Source,
		*RuneDebugName,
		*GetNameSafe(RuneDA),
		*GetNameSafe(FlowAsset),
		GetEnemyRuneTriggerName(TriggerType),
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
			UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][EnemyData]   Entry=%d Rune=%s DA=%s Cost=%d"),
				Index,
				*GetEnemyRuneDebugName(Entry.RuneDA.Get()),
				*GetNameSafe(Entry.RuneDA.Get()),
				Entry.DifficultyScore);
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

AYogGameMode::FWavePlan AYogGameMode::BuildWavePlan(int32 Budget, URoomDataAsset* Room, int32 MaxEnemies)
{
	FWavePlan Plan;
	int32 RemainingBudget = Budget;

	// ---- Step 1: 程序决定触发条件（默认 AllEnemiesDead，最稳定）----
	Plan.TriggerType = ESpawnTriggerType::AllEnemiesDead;

	// ---- Step 2: 程序决定刷怪方式（随机 Wave / OneByOne）----
	Plan.SpawnMode        = FMath::RandBool() ? ESpawnMode::Wave : ESpawnMode::OneByOne;
	Plan.OneByOneInterval = OneByOneDefaultInterval;

	// 关卡 Buff 对每只怪的额外扣分（进关时已确定）
	const int32 RoomBuffCostPerEnemy = CalcRoomBuffCost(ActiveRoomBuffs);

	// ---- Step 3: 用预算填充敌人，遵守类型上限和每波上限 ----
	bool bFirstEnemy = true;
	while (true)
	{
		// MaxEnemiesPerWave == 0 表示不限制
		if (MaxEnemies > 0 && Plan.EnemiesToSpawn.Num() >= MaxEnemies)
			break;

		TArray<FEnemyEntry> Candidates;
		for (const FEnemyEntry& E : Room->EnemyPool)
		{
			if (!E.EnemyData || !E.EnemyData->EnemyClass) continue;

			// Cost includes the room runes applied to every enemy and this enemy's carried runes.
			const int32 EnemyBuffCost = CalcEnemyBuffCost(E.EnemyData);
			const int32 EffectiveCostMin = E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy + EnemyBuffCost;

			// 第一只不过滤预算（允许超出），后续必须在预算内
			if (!bFirstEnemy && EffectiveCostMin > RemainingBudget) continue;
			// 类型上限检查
			if (E.MaxCountPerLevel >= 0)
			{
				const int32* ExistingCount = LevelTypeSpawnCounts.Find(E.EnemyData->EnemyClass);
				if (ExistingCount && *ExistingCount >= E.MaxCountPerLevel) continue;
			}
			Candidates.Add(E);
		}

		if (Candidates.IsEmpty()) break;

		// 若已达到每波上限且还有预算，优先选有效成本（基础分）最高的候选
		// 否则随机选取
		const FEnemyEntry* Chosen = nullptr;
		if (MaxEnemies > 0 && Plan.EnemiesToSpawn.Num() == MaxEnemies - 1)
		{
			// 最后一个名额：选有效成本最高的（充分消耗预算）
			int32 MaxScore = -1;
			for (const FEnemyEntry& C : Candidates)
			{
				if (C.EnemyData->DifficultyScore > MaxScore)
				{
					MaxScore = C.EnemyData->DifficultyScore;
					Chosen   = &C;
				}
			}
		}
		if (!Chosen)
			Chosen = &Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

		// Copy all self-carried runes from EnemyData.
		TArray<TObjectPtr<URuneDataAsset>> SelectedEnemyBuffs;
		int32 EnemyBuffCost = 0;
		if (!Chosen->EnemyData->EnemyBuffPool.IsEmpty())
		{
			EnemyBuffCost = CollectEnemyBuffs(Chosen->EnemyData, SelectedEnemyBuffs);
		}

		FPlannedEnemy Planned;
		Planned.EnemyClass         = Chosen->EnemyData->EnemyClass;
		Planned.EnemyBuffs         = SelectedEnemyBuffs;
		Planned.PreSpawnFX         = Chosen->EnemyData->PreSpawnFX;
		Planned.PreSpawnFXDuration = Chosen->EnemyData->PreSpawnFXDuration;
		Plan.EnemiesToSpawn.Add(Planned);

		const int32 ActualCost = Chosen->EnemyData->DifficultyScore + RoomBuffCostPerEnemy + EnemyBuffCost;
		RemainingBudget -= ActualCost;
		bFirstEnemy      = false;

		// 更新跨波次计数
		LevelTypeSpawnCounts.FindOrAdd(Chosen->EnemyData->EnemyClass)++;
		TotalLevelPlannedEnemies++;

		if (RemainingBudget <= 0) break;
	}

	// ---- Step 4: 剩余预算无法正常填充时，构建按需补刷池 ----
	if (RemainingBudget > 0)
	{
		for (const FEnemyEntry& E : Room->EnemyPool)
		{
			if (!E.EnemyData || !E.EnemyData->EnemyClass) continue;
			const int32 EffCost = E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy + CalcEnemyBuffCost(E.EnemyData);
			if (EffCost <= RemainingBudget)
			{
				FPlannedEnemy Demand;
				Demand.EnemyClass        = E.EnemyData->EnemyClass;
				CollectEnemyBuffs(E.EnemyData, Demand.EnemyBuffs);
				Plan.DemandEnemyPool.Add(Demand);
			}
		}

		// 去重（同类型保留第一个）
		TSet<TSubclassOf<AEnemyCharacterBase>> SeenClasses;
		Plan.DemandEnemyPool = Plan.DemandEnemyPool.FilterByPredicate([&](const FPlannedEnemy& P)
		{
			bool bAlreadySeen = SeenClasses.Contains(P.EnemyClass);
			SeenClasses.Add(P.EnemyClass);
			return !bAlreadySeen;
		});

		if (!Plan.DemandEnemyPool.IsEmpty())
		{
			int32 AvgCost = 0;
			for (const FPlannedEnemy& P : Plan.DemandEnemyPool)
			{
				for (const FEnemyEntry& E : Room->EnemyPool)
				{
					if (E.EnemyData && E.EnemyData->EnemyClass == P.EnemyClass)
					{
						AvgCost += E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy + CalcEnemyBuffCost(E.EnemyData);
						break;
					}
				}
			}
			AvgCost = FMath::Max(1, AvgCost / Plan.DemandEnemyPool.Num());
			Plan.DemandCount = FMath::Max(1, RemainingBudget / AvgCost);
			UE_LOG(LogTemp, Log, TEXT("BuildWavePlan: 剩余预算 %d → 按需补刷 %d 只"),
				RemainingBudget, Plan.DemandCount);
		}
	}

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

	// 随机选一个补刷条目
	const FPlannedEnemy& DemandPlanned =
		Wave.DemandEnemyPool[FMath::RandRange(0, Wave.DemandEnemyPool.Num() - 1)];

	Wave.DemandCount--;

	if (SpawnEnemyFromPool(DemandPlanned))
	{
		Wave.TotalSpawnedInWave++;
		TotalAliveEnemies++;
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
			if (S->EnemySpawnClassis.Contains(Planned.EnemyClass))
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
		}
		return SpawnedEnemy != nullptr;
	}
	return false;
}

bool AYogGameMode::BeginSpawnEnemyFromPool(const FPlannedEnemy& Planned)
{
	if (!Planned.EnemyClass) return false;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);

	TArray<AMobSpawner*> ValidSpawners;
	for (AActor* A : OutActors)
		if (AMobSpawner* S = Cast<AMobSpawner>(A))
			if (S->EnemySpawnClassis.Contains(Planned.EnemyClass))
				ValidSpawners.Add(S);

	if (ValidSpawners.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginSpawnEnemyFromPool: 没有 Spawner 支持 %s，跳过"), *Planned.EnemyClass->GetName());
		return false;
	}

	AMobSpawner* Spawner = ValidSpawners[FMath::RandRange(0, ValidSpawners.Num() - 1)];
	FVector Location = Spawner->PrepareSpawnLocation();
	if (Location == FVector::ZeroVector) return false;

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

	const int32 Count = FMath::Min(BuffCount, Pool.Num());
	for (int32 i = 0; i < Count; i++)
	{
		if (Pool[i].RuneDA)
		{
			Selected.Add(Pool[i]);
			UE_LOG(LogTemp, Warning, TEXT("[RoomBuff] Selected Room=%s Index=%d Rune=%s DA=%s Cost=%d"),
				*GetNameSafe(&Room),
				i,
				*GetEnemyRuneDebugName(Pool[i].RuneDA.Get()),
				*GetNameSafe(Pool[i].RuneDA.Get()),
				Pool[i].DifficultyScore);
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

TArray<FLootOption> AYogGameMode::GenerateLootBatch(TSet<URuneDataAsset*>& AlreadyOffered)
{
	TArray<FLootOption> Batch;

	// 确定符文源池
	const TArray<TObjectPtr<URuneDataAsset>>* SourcePool = nullptr;
	if (CampaignData && ActiveRoomData && !ActiveRoomData->LootPool.IsEmpty())
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
					return DA && !MaxLevelSet.Contains(DA->RuneInfo.RuneConfig.RuneName);
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

	const int32 OptionsCount = FMath::Min(3, Pool.Num());
	for (int32 i = 0; i < OptionsCount; i++)
	{
		FLootOption Option;
		Option.LootType  = ELootType::Rune;
		Option.RuneAsset = Pool[i];
		Batch.Add(Option);
		AlreadyOffered.Add(Pool[i]); // 写入已分配集合，下次调用时排除
	}

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

			// 保存运行时隐藏被动符文（无形状、不进格子）
			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				NewState.HiddenPassiveRuneInstances = Backpack->GetRuntimeHiddenPassiveRunes();
			}

			// 保存献祭恩赐
			NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;

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
	const FGameplayTag& LayerTag = CampaignData ? CampaignData->LayerTag : FGameplayTag::EmptyTag;

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

	if (!CampaignData) return nullptr;

	// 2. Campaign 全局 RoomPool（同类型 + 同层级）
	if (URoomDataAsset* Found = PickByTag(CampaignData->RoomPool))
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
		for (const TObjectPtr<URoomDataAsset>& Room : CampaignData->RoomPool)
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

void AYogGameMode::ActivateHubPortals()
{
	if (!CampaignData || !ActiveRoomData) return;
	if (ActiveRoomData->PortalDestinations.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateHubPortals: HubRoom [%s] 没有配置 PortalDestinations"), *ActiveRoomData->GetName());
		return;
	}

	// 目标是 FloorTable[0]（第一个战斗关），骰子决定房间类型
	const FGameplayTag RequiredRoomType = CampaignData->FloorTable.IsValidIndex(0)
		? RollRoomTypeForFloor(CampaignData->FloorTable[0])
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

	// 主城所有传送门全开（不走 50% 随机）
	for (const FPortalDestConfig& Cfg : ActiveRoomData->PortalDestinations)
	{
		APortal** Found = PortalMap.Find(Cfg.PortalIndex);
		if (!Found || !(*Found)) continue;

		URoomDataAsset* ChosenRoom = SelectRoomByTag(&Cfg, RequiredRoomType);
		if (!ChosenRoom || ChosenRoom->RoomName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("ActivateHubPortals: 门[%d] 找不到合适的 DA_Room，跳过"), Cfg.PortalIndex);
			continue;
		}

		// 主城 → 第一关：用 FloorTable[0] 的难度分选下一关 Tier，预骰本门 Buff
		const FFloorConfig* NextConfig = CampaignData->FloorTable.IsValidIndex(0)
			? &CampaignData->FloorTable[0]
			: nullptr;
		const int32 NextScore = NextConfig ? NextConfig->TotalDifficultyScore : 30;
		const FRoomDifficultyTier& NextTier = ResolveTier(
			*ChosenRoom, NextScore, LowDifficultyScoreMax, HighDifficultyScoreMin);
		const TArray<FBuffEntry> PreRolled = SelectRoomBuffs(*ChosenRoom, NextTier.BuffCount);

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
	if (!CampaignData || !ActiveRoomData) return;

	// 传送门目标配置现在存在 ActiveRoomData（当前关卡的 DA_Room）里
	if (ActiveRoomData->PortalDestinations.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivatePortals: 当前 RoomData [%s] 没有配置 PortalDestinations"), *ActiveRoomData->GetName());
		return;
	}

	// 下一关的 FloorConfig（所有门共享同一次类型骰子）
	const int32 NextIdx = CurrentFloor; // CurrentFloor 是 1-based，NextIdx 是下一关的 0-based 下标
	const FFloorConfig* NextConfig = CampaignData->FloorTable.IsValidIndex(NextIdx)
		? &CampaignData->FloorTable[NextIdx]
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
	for (const FPortalDestConfig& Cfg : Configs)
	{
		APortal** Found = PortalMap.Find(Cfg.PortalIndex);
		if (!Found || !(*Found)) continue;

		// 先查此门专属 RoomPool，再查 Campaign 全局，最后退化为 Normal
		URoomDataAsset* ChosenRoom = SelectRoomByTag(&Cfg, RequiredRoomType);

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
		const bool bShouldOpen = !bAtLeastOneOpened || FMath::RandBool();
		if (!bShouldOpen)
		{
			UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 随机未开启"), Cfg.PortalIndex);
			continue;
		}

		// 预骰下一关 Buff（每扇门各自缓存；玩家确认进入时由 Portal::TryEnter 写入 GI）
		const int32 NextScore = NextConfig ? NextConfig->TotalDifficultyScore : 30;
		const FRoomDifficultyTier& NextTier = ResolveTier(
			*ChosenRoom, NextScore, LowDifficultyScoreMax, HighDifficultyScoreMin);
		const TArray<FBuffEntry> PreRolled = SelectRoomBuffs(*ChosenRoom, NextTier.BuffCount);

		(*Found)->Open(LevelName, ChosenRoom, PreRolled);
		bAtLeastOneOpened = true;

		UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 开启 → 关卡=%s 房间=%s 预骰Buff数=%d"),
			Cfg.PortalIndex, *LevelName.ToString(), *ChosenRoom->GetName(), PreRolled.Num());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 敌人注册表（相机战斗感知）
// ─────────────────────────────────────────────────────────────────────────────

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

void AYogGameMode::HandleLevelEndEffectFinished()
{
	// HUD 揭幕动画完成 = 玩家视觉回到正常 = 适合弹教程的时机
	TriggerLifecycleEvent(EGameLifecycleEvent::LevelClearRevealed);
}
