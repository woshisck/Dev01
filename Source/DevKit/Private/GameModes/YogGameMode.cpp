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
#include "UI/LootSelectionWidget.h"

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
	

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);


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
			Player->AddGold(GoldReward);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 发放金币 %d"), GoldReward);
		}
	}

	// 在最后击杀位置生成奖励拾取物（若无敌人被击杀则退而在玩家位置生成）
	if (RewardPickupClass)
	{
		FVector SpawnLoc = LastEnemyKillLocation;
		if (SpawnLoc.IsZero())
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (APawn* P = PC->GetPawn())
				{
					float Angle = FMath::FRandRange(0.f, 2.f * PI);
					float Radius = FMath::FRandRange(200.f, 250.0f);
					FVector2D RandomPoint(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));

					SpawnLoc = P->GetActorLocation() + FVector(RandomPoint, 0.f);
				}
			}
		}
		if (!SpawnLoc.IsZero())
		{
			GetWorld()->SpawnActor<AActor>(RewardPickupClass, SpawnLoc, FRotator::ZeroRotator);
			UE_LOG(LogTemp, Log, TEXT("EnterArrangementPhase: 生成 RewardPickup @ %s"), *SpawnLoc.ToString());

			// DEBUG: 打印 LootPool 中可拾取的符文名称
			if (GEngine && ActiveRoomData && !ActiveRoomData->LootPool.IsEmpty())
			{
				FString RuneNames;
				for (const TObjectPtr<URuneDataAsset>& Rune : ActiveRoomData->LootPool)
				{
					if (Rune) RuneNames += Rune->GetName() + TEXT(" | ");
				}
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
					FString::Printf(TEXT("[奖励拾取物] LootPool(%d): %s"),
						ActiveRoomData->LootPool.Num(), *RuneNames));
			}
		}
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
		UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: FloorTable 没有第 %d 关的配置，降级为场景预放置敌人统计"), CurrentFloor);
		FallbackToPreplacedEnemies();
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
			const FName DefaultRoomName = CampaignData->DefaultStartingRoom->RoomName;
			if (!DefaultRoomName.IsNone())
			{
				// 检查当前加载的关卡是否已经是 DefaultStartingRoom 指定的关卡
				// GetCurrentLevelName(true) 会去掉 PIE 前缀（如 "UEDPIE_0_"）
				const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
				if (!CurrentMapName.Equals(DefaultRoomName.ToString(), ESearchCase::IgnoreCase))
				{
					// 当前关卡不匹配，重定向到 DefaultStartingRoom 的关卡
					UE_LOG(LogTemp, Warning, TEXT("StartLevelSpawning: 当前关卡 [%s] ≠ DefaultStartingRoom [%s]，重定向..."),
						*CurrentMapName, *DefaultRoomName.ToString());
					if (GI) GI->PendingRoomData = CampaignData->DefaultStartingRoom;
					UGameplayStatics::OpenLevel(GetWorld(), DefaultRoomName);
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

	// 选取本关 Buff（进关时确定，新怪刷出时施加）
	ActiveRoomBuffs = SelectRoomBuffs(*ActiveRoomData, ActiveBuffCount);

	// 生成波次计划
	GenerateWavePlans(Score, Tier.MaxWaveCount, ActiveRoomData);

	// 重置运行时状态
	CurrentWaveIndex  = -1;
	TotalAliveEnemies = 0;
	bAllWavesSpawned  = false;

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

// 辅助：从 EnemyData 的 EnemyBuffPool 中随机选一个 Buff（返回 nullptr 表示不选）
static URuneDataAsset* PickEnemyBuff(UEnemyData* EnemyData)
{
	if (!EnemyData || EnemyData->EnemyBuffPool.IsEmpty()) return nullptr;
	const FBuffEntry& Entry = EnemyData->EnemyBuffPool[FMath::RandRange(0, EnemyData->EnemyBuffPool.Num() - 1)];
	return Entry.RuneDA.Get();
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

			// 计算此敌人的有效成本（基础分 + 关卡Buff扣分 + 该敌人专属Buff中最低扣分）
			// 敌人专属 Buff 在确认刷出时才选取，这里用最低可能成本做候选过滤
			int32 MinEnemyBuffCost = 0;
			if (!E.EnemyData->EnemyBuffPool.IsEmpty())
			{
				MinEnemyBuffCost = E.EnemyData->EnemyBuffPool[0].DifficultyScore;
				for (const FBuffEntry& B : E.EnemyData->EnemyBuffPool)
					MinEnemyBuffCost = FMath::Min(MinEnemyBuffCost, B.DifficultyScore);
			}
			const int32 EffectiveCostMin = E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy;
			(void)MinEnemyBuffCost; // 敌人专属Buff是可选的，候选时只用基础+房间Buff成本

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

		// 从敌人专属 Buff 池中选取一个（若有）；若会导致成本超出，则不选
		URuneDataAsset* SelectedEnemyBuff = nullptr;
		int32 EnemyBuffCost = 0;
		if (!Chosen->EnemyData->EnemyBuffPool.IsEmpty())
		{
			URuneDataAsset* Candidate = PickEnemyBuff(Chosen->EnemyData);
			// 查找对应的 DifficultyScore
			for (const FBuffEntry& B : Chosen->EnemyData->EnemyBuffPool)
			{
				if (B.RuneDA.Get() == Candidate)
				{
					EnemyBuffCost = B.DifficultyScore;
					break;
				}
			}
			// 预算足够或是第一只时才应用（第一只强制刷出，允许成本超出）
			const int32 TotalCost = Chosen->EnemyData->DifficultyScore + RoomBuffCostPerEnemy + EnemyBuffCost;
			if (bFirstEnemy || TotalCost <= RemainingBudget)
				SelectedEnemyBuff = Candidate;
			else
				EnemyBuffCost = 0;
		}

		FPlannedEnemy Planned;
		Planned.EnemyClass        = Chosen->EnemyData->EnemyClass;
		Planned.SelectedEnemyBuff = SelectedEnemyBuff;
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
			const int32 EffCost = E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy;
			if (EffCost <= RemainingBudget)
			{
				FPlannedEnemy Demand;
				Demand.EnemyClass        = E.EnemyData->EnemyClass;
				Demand.SelectedEnemyBuff = PickEnemyBuff(E.EnemyData);
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
						AvgCost += E.EnemyData->DifficultyScore + RoomBuffCostPerEnemy;
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

	if (SpawnEnemyFromPool(OneByOneSpawnQueue[OneByOneSpawnIndex]))
	{
		Wave.TotalSpawnedInWave++;
		TotalAliveEnemies++;

		// DEBUG: 打印本次刷怪数据
		if (GEngine)
		{
			const TSubclassOf<AEnemyCharacterBase>& EC = OneByOneSpawnQueue[OneByOneSpawnIndex].EnemyClass;
			const FString EnemyName = EC ? EC->GetName() : TEXT("?");
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				FString::Printf(TEXT("[刷怪] 波次%d | 队列%d/%d | 存活%d | %s"),
					CurrentWaveIndex + 1,
					OneByOneSpawnIndex + 1,
					OneByOneSpawnQueue.Num(),
					TotalAliveEnemies,
					*EnemyName));
		}
	}
	OneByOneSpawnIndex++;

	if (OneByOneSpawnIndex >= OneByOneSpawnQueue.Num())
	{
		// 队列已空，设置触发条件，不再重新调度
		SetupWaveTrigger(Wave);
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
				AliveCount++;
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
	if (!bAllWavesSpawned || TotalAliveEnemies > 0) return;

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
			UBuffFlowComponent* BFC = SpawnedEnemy->BuffFlowComponent;
			if (BFC)
			{
				// 施加关卡 Buff（进关时选好的，对所有怪生效）
				for (const FBuffEntry& Entry : ActiveRoomBuffs)
				{
					if (!Entry.RuneDA || !Entry.RuneDA->RuneInfo.Flow.FlowAsset) continue;
					BFC->StartBuffFlow(Entry.RuneDA->RuneInfo.Flow.FlowAsset, FGuid::NewGuid(), SpawnedEnemy);
				}
				// 施加敌人专属 Buff（BuildWavePlan 时选好的，只对此只怪生效）
				if (Planned.SelectedEnemyBuff && Planned.SelectedEnemyBuff->RuneInfo.Flow.FlowAsset)
				{
					BFC->StartBuffFlow(Planned.SelectedEnemyBuff->RuneInfo.Flow.FlowAsset, FGuid::NewGuid(), SpawnedEnemy);
				}
			}
		}
		return SpawnedEnemy != nullptr;
	}
	return false;
}

TArray<FBuffEntry> AYogGameMode::SelectRoomBuffs(const URoomDataAsset& Room, int32 BuffCount)
{
	TArray<FBuffEntry> Selected;
	if (Room.BuffPool.IsEmpty() || BuffCount <= 0)
		return Selected;

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
		if (Pool[i].RuneDA) Selected.Add(Pool[i]);
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

			// 保存当前武器
			NewState.EquippedWeaponDef = Player->EquippedWeaponDef;

			// 保存整理阶段选出但尚未放入格子的符文
			NewState.PendingRunes = Player->PendingRunes;

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

	UGameplayStatics::OpenLevel(GetWorld(), NextLevel);
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

		(*Found)->Open(ChosenRoom->RoomName, ChosenRoom);
		UE_LOG(LogTemp, Log, TEXT("ActivateHubPortals: 门[%d] 开启 → 关卡=%s 房间=%s"),
			Cfg.PortalIndex, *ChosenRoom->RoomName.ToString(), *ChosenRoom->GetName());
	}
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
		const FName LevelName = ChosenRoom->RoomName;
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

		(*Found)->Open(LevelName, ChosenRoom);
		bAtLeastOneOpened = true;

		UE_LOG(LogTemp, Log, TEXT("ActivatePortals: 门[%d] 开启 → 关卡=%s 房间=%s"),
			Cfg.PortalIndex, *LevelName.ToString(), *ChosenRoom->GetName());
	}
}
