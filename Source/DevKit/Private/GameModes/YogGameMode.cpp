// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/YogGameMode.h"
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
#include "Map/Portal.h"
#include "Map/RewardPickup.h"

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
	DefaultPawnClass = nullptr;

	//CurrentWaveIndex = 0;
	//SpawnedInWave = 0;
	//ActiveMobCount = 0;

}


void AYogGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (GI && GI->PersistentSaveData)
	{
		FActorSpawnParameters Params;
		Params.Owner = NewPlayer;

		APlayerCharacterBase* LoadedChar = GetWorld()->SpawnActor<APlayerCharacterBase>(
			GI->PersistentSaveData->SavedCharacterClass,
			Params
		);

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

		// 发放金币（新系统：按当前难度配置随机范围）
		if (CampaignData && ActiveDifficultyConfig.GoldMax > 0)
		{
			const int32 GoldReward = FMath::RandRange(
				ActiveDifficultyConfig.GoldMin, ActiveDifficultyConfig.GoldMax);
			Player->AddGold(GoldReward);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 发放金币 %d"), GoldReward);
		}
	}

	// 在最后击杀位置生成奖励拾取物（玩家走近后触发战利品选择界面）
	if (RewardPickupClass && !LastEnemyKillLocation.IsZero())
	{
		GetWorld()->SpawnActor<AActor>(RewardPickupClass, LastEnemyKillLocation, FRotator::ZeroRotator);
		UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 生成 RewardPickup @ %s"), *LastEnemyKillLocation.ToString());
	}

	// 开启传送门
	ActivatePortals();
}

void AYogGameMode::SelectLoot(int32 LootIndex)
{
	if (!CurrentLootOptions.IsValidIndex(LootIndex))
		return;

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (!Player)
		return;

	const FLootOption& Chosen = CurrentLootOptions[LootIndex];
	if (Chosen.LootType == ELootType::Rune && Chosen.RuneAsset)
	{
		Player->AddRuneToInventory(Chosen.RuneAsset->CreateInstance());
	}
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
				NewState.CurrentGold = Player->GetGold();

				if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
				{
					NewState.CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
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

				GI->PendingRunState = NewState;
				UE_LOG(LogTemp, Warning, TEXT("[RunState] SAVE — HP=%.1f Gold=%d Phase=%d Runes=%d"),
					NewState.CurrentHP, NewState.CurrentGold, NewState.CurrentPhase, NewState.PlacedRunes.Num());
			}
		}
		UGameplayStatics::OpenLevel(GetWorld(), NextLevelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ConfirmArrangementAndTransition: 没有找到下一关的 LevelName，请检查 CampaignData.FloorTable[%d].LevelName"), CurrentFloor);
	}
}

// =========================================================
// 新刷怪系统实现
// =========================================================

void AYogGameMode::StartLevelSpawning()
{
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
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: FloorTable 没有第 %d 关的配置"), CurrentFloor);
		return;
	}

	const FFloorConfig& Config = CampaignData->FloorTable[TableIndex];

	// ---- 确定本关使用的 DA_Room ----
	// 优先读取 GI 中由传送门写入的 PendingRoomData（切关传递）
	// 若为空（第一关或测试场景），直接按本关 FloorConfig 骰子选取
	if (GI && GI->PendingRoomData)
	{
		ActiveRoomData = GI->PendingRoomData;
		GI->PendingRoomData = nullptr; // 清除，避免复用
		UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 Portal 传递的 RoomData = %s"), *ActiveRoomData->GetName());
	}
	else
	{
		// 第一关：优先使用 DefaultStartingRoom（策划手动指定）；未填则骰子选取
		if (CurrentFloor == 1 && CampaignData->DefaultStartingRoom)
		{
			ActiveRoomData = CampaignData->DefaultStartingRoom;
			UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 使用 DefaultStartingRoom = %s"), *ActiveRoomData->GetName());
		}
		else
		{
			ActiveRoomData = RollRoomForFloor(Config);
			UE_LOG(LogTemp, Log, TEXT("StartLevelSpawning: 骰子选取 RoomData = %s"),
				ActiveRoomData ? *ActiveRoomData->GetName() : TEXT("null"));
		}
	}

	if (!ActiveRoomData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 第 %d 关无法获取 RoomData，检查 CampaignData 的 RoomPool 是否为空"), CurrentFloor);
		return;
	}

	bCurrentRoomIsElite = ActiveRoomData->bIsEliteRoom;

	// 根据难度等级选取对应的 DifficultyConfig
	// 按请求难度查表，找不到则降级到列表中第一档（最低难度）
	const FDifficultyConfig* DiffConfig = nullptr;
	for (const FDifficultyEntry& DE : ActiveRoomData->DifficultyConfigs)
	{
		if (DE.Tier == Config.Difficulty)
		{
			DiffConfig = &DE.Config;
			break;
		}
	}
	if (!DiffConfig && ActiveRoomData->DifficultyConfigs.Num() > 0)
	{
		DiffConfig = &ActiveRoomData->DifficultyConfigs[0].Config;
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 难度 %d 无匹配配置，降级到第一档"),
			(int32)Config.Difficulty);
	}
	if (!DiffConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("StartLevelSpawning: RoomData 没有任何难度配置，跳过"));
		return;
	}

	// 缓存难度配置（整理阶段发放战利品/金币时使用）
	ActiveDifficultyConfig = *DiffConfig;

	// 选取本关 Buff（进关时确定，新怪刷出时施加）
	ActiveRoomBuffs = SelectRoomBuffs(*ActiveRoomData, *DiffConfig);

	// 生成波次计划
	GenerateWavePlans(*DiffConfig, ActiveRoomData);

	// 重置运行时状态
	CurrentWaveIndex  = -1;
	TotalAliveEnemies = 0;
	bAllWavesSpawned  = false;

	// 启动第一波
	TriggerNextWave();
}

void AYogGameMode::GenerateWavePlans(const FDifficultyConfig& Config, URoomDataAsset* Room)
{
	WavePlans.Empty();

	const int32 WaveCount = FMath::RandRange(Config.WaveCountMin, Config.WaveCountMax);
	UE_LOG(LogTemp, Log, TEXT("GenerateWavePlans: 本关共 %d 波"), WaveCount);

	for (int32 i = 0; i < WaveCount; i++)
	{
		const int32 Budget = Config.WaveBudgets.IsValidIndex(i)
			? Config.WaveBudgets[i]
			: Config.WaveBudgets.Last();

		WavePlans.Add(BuildWavePlan(Budget, Config, Room));
	}
}

AYogGameMode::FWavePlan AYogGameMode::BuildWavePlan(
	int32 Budget, const FDifficultyConfig& Config, URoomDataAsset* Room)
{
	FWavePlan Plan;
	int32 RemainingBudget = Budget;

	// ---- Step 1: 选择触发条件 ----
	if (Config.AllowedTriggers.Num() > 0)
	{
		TArray<FSpawnTriggerOption> Candidates;
		for (const FSpawnTriggerOption& T : Config.AllowedTriggers)
		{
			if (T.DifficultyScore <= RemainingBudget)
				Candidates.Add(T);
		}
		if (Candidates.Num() > 0)
		{
			const FSpawnTriggerOption& Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			Plan.TriggerType  = Chosen.TriggerType;
			RemainingBudget  -= Chosen.DifficultyScore;
		}
		// 若无法负担任何触发条件，保持默认 AllEnemiesDead（0分）
	}

	// ---- Step 2: 选择刷怪方式 ----
	if (Config.AllowedSpawnModes.Num() > 0)
	{
		TArray<FSpawnModeOption> Candidates;
		for (const FSpawnModeOption& M : Config.AllowedSpawnModes)
		{
			if (M.DifficultyScore <= RemainingBudget)
				Candidates.Add(M);
		}
		if (Candidates.Num() > 0)
		{
			const FSpawnModeOption& Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			Plan.SpawnMode        = Chosen.SpawnMode;
			Plan.OneByOneInterval = Chosen.OneByOneInterval;
			RemainingBudget      -= Chosen.DifficultyScore;
		}
	}

	// ---- Step 3: 用剩余预算填充敌人（允许超出预算，保证至少 1 只）----
	bool bFirstEnemy = true;
	while (true)
	{
		TArray<FEnemyEntry> Candidates;
		for (const FEnemyEntry& E : Room->EnemyPool)
		{
			if (!E.EnemyData || !E.EnemyData->EnemyClass) continue;
			if (E.EnemyData->bEliteOnly && !bCurrentRoomIsElite) continue;
			// 第一只不过滤预算（允许超出），后续必须在预算内
			if (!bFirstEnemy && E.EnemyData->DifficultyScore > RemainingBudget) continue;
			Candidates.Add(E);
		}

		if (Candidates.IsEmpty()) break;

		const FEnemyEntry& Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		Plan.EnemiesToSpawn.Add(Chosen.EnemyData->EnemyClass);
		RemainingBudget -= Chosen.EnemyData->DifficultyScore;
		bFirstEnemy      = false;

		if (RemainingBudget <= 0) break;
	}

	UE_LOG(LogTemp, Log, TEXT("BuildWavePlan: 触发=%d 方式=%d 敌人数=%d"),
		(int32)Plan.TriggerType, (int32)Plan.SpawnMode, Plan.EnemiesToSpawn.Num());

	return Plan;
}

void AYogGameMode::TriggerNextWave()
{
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

	// 如果这是最后一波，刷怪完成后就标记所有波次已结束
	if (CurrentWaveIndex == WavePlans.Num() - 1)
	{
		bAllWavesSpawned = true;
	}

	if (Wave.SpawnMode == ESpawnMode::Wave)
	{
		// 所有敌人同时刷出
		for (TSubclassOf<AEnemyCharacterBase> EnemyClass : Wave.EnemiesToSpawn)
		{
			SpawnEnemyFromPool(EnemyClass);
			Wave.TotalSpawnedInWave++;
			TotalAliveEnemies++;
		}
		SetupWaveTrigger(Wave);
	}
	else // OneByOne
	{
		OneByOneSpawnQueue = Wave.EnemiesToSpawn;
		OneByOneSpawnIndex = 0;
		GetWorld()->GetTimerManager().SetTimer(
			OneByOneTimer,
			this,
			&AYogGameMode::SpawnNextOneByOne,
			Wave.OneByOneInterval,
			true,
			0.5f // 第一只在 0.5 秒后刷出
		);
	}
}

void AYogGameMode::SpawnNextOneByOne()
{
	if (!WavePlans.IsValidIndex(CurrentWaveIndex)) return;
	FWavePlan& Wave = WavePlans[CurrentWaveIndex];

	if (!OneByOneSpawnQueue.IsValidIndex(OneByOneSpawnIndex))
	{
		// 本波全部刷出，停止定时器并设置触发条件
		GetWorld()->GetTimerManager().ClearTimer(OneByOneTimer);
		SetupWaveTrigger(Wave);
		return;
	}

	SpawnEnemyFromPool(OneByOneSpawnQueue[OneByOneSpawnIndex]);
	Wave.TotalSpawnedInWave++;
	TotalAliveEnemies++;
	OneByOneSpawnIndex++;

	if (OneByOneSpawnIndex >= OneByOneSpawnQueue.Num())
	{
		GetWorld()->GetTimerManager().ClearTimer(OneByOneTimer);
		SetupWaveTrigger(Wave);
	}
}

void AYogGameMode::SetupWaveTrigger(const FWavePlan& Wave)
{
	// 最后一波不需要触发条件（等待 CheckLevelComplete）
	if (CurrentWaveIndex >= WavePlans.Num() - 1)
		return;

	switch (Wave.TriggerType)
	{
		case ESpawnTriggerType::AllEnemiesDead:
			// 由 CheckWaveTrigger 在 UpdateFinishLevel 中处理
			break;

		case ESpawnTriggerType::TimeInterval_5s:
			GetWorld()->GetTimerManager().SetTimer(
				WaveTriggerTimer, this, &AYogGameMode::OnWaveTriggerFired, 5.0f, false);
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

	const FWavePlan& Wave = WavePlans[CurrentWaveIndex];
	if (Wave.TotalSpawnedInWave == 0) return; // 本波还没开始刷怪

	switch (Wave.TriggerType)
	{
		case ESpawnTriggerType::AllEnemiesDead:
			if (TotalAliveEnemies <= 0)
				TriggerNextWave();
			break;

		case ESpawnTriggerType::PercentKilled_50:
			if (Wave.TotalKilledInWave >= FMath::CeilToInt(Wave.TotalSpawnedInWave * 0.5f))
				TriggerNextWave();
			break;

		case ESpawnTriggerType::PercentKilled_20:
			if (Wave.TotalKilledInWave >= FMath::CeilToInt(Wave.TotalSpawnedInWave * 0.2f))
				TriggerNextWave();
			break;

		case ESpawnTriggerType::TimeInterval_5s:
			// 由定时器处理，无需在这里判断
			break;
	}
}

void AYogGameMode::CheckLevelComplete()
{
	if (bAllWavesSpawned && TotalAliveEnemies <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("CheckLevelComplete: 关卡完成！进入整理阶段"));
		OnFinishLevel.Broadcast();
		FinishLevelEvent.Broadcast();
		EnterArrangementPhase();
	}
}

void AYogGameMode::SpawnEnemyFromPool(TSubclassOf<AEnemyCharacterBase> EnemyClass)
{
	if (!EnemyClass) return;

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);
	if (OutActors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemyFromPool: 场景中没有 MobSpawner！"));
		return;
	}

	AMobSpawner* Spawner = Cast<AMobSpawner>(
		OutActors[FMath::RandRange(0, OutActors.Num() - 1)]);
	if (Spawner)
	{
		AEnemyCharacterBase* SpawnedEnemy = Spawner->SpawnMob(EnemyClass);
		if (SpawnedEnemy && ActiveRoomBuffs.Num() > 0)
		{
			// 在敌人的 BuffFlowComponent 上直接激活关卡符文（永久激活，无需背包）
			UBuffFlowComponent* BFC = SpawnedEnemy->BuffFlowComponent;
			if (BFC)
			{
				for (URuneDataAsset* RuneDA : ActiveRoomBuffs)
				{
					if (!RuneDA || !RuneDA->RuneInfo.Flow.FlowAsset) continue;
					BFC->StartBuffFlow(RuneDA->RuneInfo.Flow.FlowAsset, FGuid::NewGuid(), SpawnedEnemy);
				}
			}
		}
	}
}

TArray<URuneDataAsset*> AYogGameMode::SelectRoomBuffs(
	const URoomDataAsset& Room, const FDifficultyConfig& Config)
{
	TArray<URuneDataAsset*> Selected;
	if (Room.BuffPool.IsEmpty() || Config.BuffCount <= 0)
		return Selected;

	// 复制池子并洗牌（Fisher-Yates）
	TArray<TObjectPtr<URuneDataAsset>> Pool = Room.BuffPool;
	for (int32 i = Pool.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}

	const int32 Count = FMath::Min(Config.BuffCount, Pool.Num());
	for (int32 i = 0; i < Count; i++)
	{
		if (Pool[i]) Selected.Add(Pool[i]);
	}

	return Selected;
}

// =========================================================
// 关卡流程
// =========================================================

void AYogGameMode::GenerateLootOptions()
{
	CurrentLootOptions.Empty();

	// 优先使用新系统（CampaignData + RoomDataAsset.LootPool）
	const TArray<TObjectPtr<URuneDataAsset>>* SourcePool = nullptr;
	if (CampaignData && ActiveRoomData && !ActiveRoomData->LootPool.IsEmpty())
	{
		SourcePool = &ActiveRoomData->LootPool;
	}
	if (!SourcePool)
	{
		OnLootGenerated.Broadcast(CurrentLootOptions);
		return;
	}

	// 复制掉落池并 Fisher-Yates 洗牌
	TArray<URuneDataAsset*> Pool;
	for (const TObjectPtr<URuneDataAsset>& Asset : *SourcePool)
	{
		if (Asset) Pool.Add(Asset);
	}

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
		CurrentLootOptions.Add(Option);
	}

	UE_LOG(LogTemp, Log, TEXT("GenerateLootOptions: 生成 %d 个符文选项"), CurrentLootOptions.Num());
	OnLootGenerated.Broadcast(CurrentLootOptions);
}

void AYogGameMode::TransitionToLevel(FName NextLevel, URoomDataAsset* NextRoom)
{
	if (NextLevel.IsNone()) return;

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
			NewState.CurrentGold = Player->GetGold();

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
			}

			GI->PendingRunState = NewState;
			UE_LOG(LogTemp, Warning, TEXT("[RunState] SAVE (Portal) — HP=%.1f Gold=%d Phase=%d Runes=%d Room=%s"),
				NewState.CurrentHP, NewState.CurrentGold, NewState.CurrentPhase, NewState.PlacedRunes.Num(),
				NextRoom ? *NextRoom->GetName() : TEXT("null"));
		}
	}

	UGameplayStatics::OpenLevel(GetWorld(), NextLevel);
}

URoomDataAsset* AYogGameMode::RollRoomForFloor(const FFloorConfig& Config)
{
	if (!CampaignData) return nullptr;

	// 强制精英关：直接从精英池选
	if (Config.bForceElite)
	{
		if (CampaignData->EliteRoomPool.Num() > 0)
			return CampaignData->EliteRoomPool[FMath::RandRange(0, CampaignData->EliteRoomPool.Num() - 1)];
	}

	// 按概率阈值决定房间类型（相加后超出1.0的概率自动折算为Normal）
	const float Roll = FMath::FRand();
	TArray<TObjectPtr<URoomDataAsset>>* Pool = nullptr;

	if (Roll < Config.EliteChance)
		Pool = &CampaignData->EliteRoomPool;
	else if (Roll < Config.EliteChance + Config.ShopChance)
		Pool = &CampaignData->ShopRoomPool;
	else if (Roll < Config.EliteChance + Config.ShopChance + Config.EventChance)
		Pool = &CampaignData->EventRoomPool;
	else
		Pool = &CampaignData->NormalRoomPool;

	// 若选中的类型池为空，降级到 Normal 池
	if (!Pool || Pool->Num() == 0)
		Pool = &CampaignData->NormalRoomPool;

	if (Pool && Pool->Num() > 0)
		return (*Pool)[FMath::RandRange(0, Pool->Num() - 1)];

	return nullptr;
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

	// 下一关的 FloorConfig（用于骰子房间类型）
	const int32 NextIdx = CurrentFloor; // 0-based，CurrentFloor 是 1-based 下一关的 0-based 下标
	const FFloorConfig* NextConfig = CampaignData->FloorTable.IsValidIndex(NextIdx)
		? &CampaignData->FloorTable[NextIdx]
		: nullptr;

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
		if (Cfg.NextLevelPool.IsEmpty()) continue;

		APortal** Found = PortalMap.Find(Cfg.PortalIndex);
		if (!Found || !(*Found)) continue;

		// 洗牌后第一个必开；后续 50% 概率
		const bool bShouldOpen = !bAtLeastOneOpened || FMath::RandBool();
		if (!bShouldOpen)
		{
			UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 随机未开启"), Cfg.PortalIndex);
			continue;
		}

		// 每个传送门独立骰子决定下一关的房间类型
		URoomDataAsset* ChosenRoom = NextConfig ? RollRoomForFloor(*NextConfig) : nullptr;

		// 从该门的关卡池随机选一张地图（类型无关）
		const FName ChosenLevel = Cfg.NextLevelPool[FMath::RandRange(0, Cfg.NextLevelPool.Num() - 1)];

		(*Found)->Open(ChosenLevel, ChosenRoom);
		bAtLeastOneOpened = true;

		UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 开启 → 关卡=%s 房间=%s"),
			Cfg.PortalIndex, *ChosenLevel.ToString(),
			ChosenRoom ? *ChosenRoom->GetName() : TEXT("null"));
	}
}
