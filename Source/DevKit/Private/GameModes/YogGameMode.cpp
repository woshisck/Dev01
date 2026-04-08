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
#include "Map/YogLevelScript.h"
#include "System/YogGameInstanceBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Mob/MobSpawner.h"
#include "AbilitySystemComponent.h"

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

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	ULevel* CurrentLevel = World->GetCurrentLevel();
	if (!CurrentLevel)
	{
		return;
	}

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

	UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());
	UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>();

	//if (SaveSubsystem->CurrentSaveGame)
	//{
	//	SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
	//}
	//else
	//{
	//	// spawn default player char
	//}

	//TODO: this function calls after openLevel : 
	//[get player + get transform -> spawn player -> poccess ->] in game mode
	



	AYogLevelScript* LevelScriptActor = Cast<AYogLevelScript>(CurrentLevel->GetLevelScriptActor());
	if (LevelScriptActor)
	{
		RemainKillCount = LevelScriptActor->MonsterKillCountTarget;

		UE_LOG(LogTemp, Warning, TEXT("Found LevelScriptActor: %s"), *LevelScriptActor->GetName());

		this->OnFinishLevelEvent().AddUObject(SaveSubsystem, &UYogSaveSubsystem::WriteSaveGame);
	}

	// 新刷怪系统：若配置了 CampaignData，自动启动波次刷怪
	if (CampaignData)
	{
		StartLevelSpawning();
	}



	//if (UWorld* World = GetWorld())
	//{
	//	AYogGameMode* GameMode = Cast<AYogGameMode>(World->GetAuthGameMode());
	//	if (GameMode)
	//	{
	//		// Bind the subsystem's function to the GameMode's event.
	//		GameMode->OnFinishLevelEvent().AddUObject(this, &UYogSaveSubsystem::WriteSaveGame);
	//	}
	//}



	//SaveSubsystem->WriteSaveGame().AddUObject(this, &AYogGameMode::OnFinishLevel);


	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);




	UYogSaveSubsystem* save_subsystem = UGameInstance::GetSubsystem<UYogSaveSubsystem>(GetGameInstance());
	if (save_subsystem->CurrentSaveGame)
	{
		//NEXT MOVE : save_subsystem->CurrentSaveGame->
		//APlayerCharacterBase* currentSave_player = Cast<APlayerCharacterBase>(save_subsystem->LoadData());
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

	// 生成战利品选项
	GenerateLootOptions();
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

	// 加载下一关：优先读 CampaignData（新系统），回退 LevelSequenceData（旧系统）
	FName NextLevelName;
	if (CampaignData && CampaignData->FloorTable.IsValidIndex(CurrentFloor))
	{
		NextLevelName = CampaignData->FloorTable[CurrentFloor].LevelName;
	}
	else if (LevelSequenceData && !LevelSequenceData->NextLevelName.IsNone())
	{
		NextLevelName = LevelSequenceData->NextLevelName;
	}

	if (!NextLevelName.IsNone())
	{
		// 存储下一关楼层编号到 GameInstance，供新 GameMode 的 StartLevelSpawning 读取
		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			GI->PendingNextFloor = CurrentFloor + 1;
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
	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		if (GI->PendingNextFloor > 1)
		{
			CurrentFloor = GI->PendingNextFloor;
		}
	}

	// FloorTable 下标从 0 开始，CurrentFloor 从 1 开始
	const int32 TableIndex = CurrentFloor - 1;
	if (!CampaignData->FloorTable.IsValidIndex(TableIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: FloorTable 没有第 %d 关的配置"), CurrentFloor);
		return;
	}

	const FFloorEntry& Entry = CampaignData->FloorTable[TableIndex];
	if (!Entry.RoomData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 第 %d 关的 RoomData 为空"), CurrentFloor);
		return;
	}

	bCurrentRoomIsElite = Entry.RoomData->bIsEliteRoom;

	// 根据难度等级选取对应的配置
	const FDifficultyConfig* Config = nullptr;
	switch (Entry.Difficulty)
	{
		case EDifficultyTier::Low:    Config = &Entry.RoomData->LowConfig;    break;
		case EDifficultyTier::Medium: Config = &Entry.RoomData->MediumConfig; break;
		case EDifficultyTier::High:   Config = &Entry.RoomData->HighConfig;   break;
		case EDifficultyTier::Elite:  Config = &Entry.RoomData->HighConfig;   break;
		default: Config = &Entry.RoomData->LowConfig; break;
	}

	// 缓存当前房间数据和难度配置（整理阶段发放战利品/金币时使用）
	ActiveRoomData = Entry.RoomData;
	ActiveDifficultyConfig = *Config;

	// 选取本关 Buff（进关时确定，新怪刷出时施加）
	ActiveRoomBuffs = SelectRoomBuffs(*Entry.RoomData, *Config);

	// 生成波次计划
	GenerateWavePlans(*Config, Entry.RoomData);

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
			if (!E.EnemyClass) continue;
			if (E.bEliteOnly && !bCurrentRoomIsElite) continue;
			// 第一只不过滤预算（允许超出），后续必须在预算内
			if (!bFirstEnemy && E.DifficultyScore > RemainingBudget) continue;
			Candidates.Add(E);
		}

		if (Candidates.IsEmpty()) break;

		const FEnemyEntry& Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		Plan.EnemiesToSpawn.Add(Chosen.EnemyClass);
		RemainingBudget -= Chosen.DifficultyScore;
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
			UAbilitySystemComponent* EnemyASC = SpawnedEnemy->GetAbilitySystemComponent();
			if (EnemyASC)
			{
				for (TSubclassOf<UGameplayEffect> GEClass : ActiveRoomBuffs)
				{
					if (!GEClass) continue;
					FGameplayEffectContextHandle Context = EnemyASC->MakeEffectContext();
					FGameplayEffectSpecHandle Spec = EnemyASC->MakeOutgoingSpec(GEClass, 1.0f, Context);
					if (Spec.IsValid())
					{
						EnemyASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
					}
				}
			}
		}
	}
}

TArray<TSubclassOf<UGameplayEffect>> AYogGameMode::SelectRoomBuffs(
	const URoomDataAsset& Room, const FDifficultyConfig& Config)
{
	TArray<TSubclassOf<UGameplayEffect>> Selected;
	if (Room.BuffPool.IsEmpty() || Config.BuffCount <= 0)
		return Selected;

	// 复制池子并洗牌
	TArray<TSubclassOf<UGameplayEffect>> Pool = Room.BuffPool;
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
	else if (LevelSequenceData && !LevelSequenceData->LootPool.IsEmpty())
	{
		SourcePool = &LevelSequenceData->LootPool;
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
