// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "GameModes/LevelFlowTypes.h"
#include "GameModes/SpawnTypes.h"
#include "Data/CampaignDataAsset.h"
#include "Data/BuffDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class UYogSaveGame;
class AEnemyCharacterBase;
class APortal;
class ARewardPickup;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapClean);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ELevelPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootGenerated, const TArray<FLootOption>&, LootOptions);


DECLARE_DELEGATE(FCleanAllMobInMap);
DECLARE_DELEGATE(FSpawnMobStart);
DECLARE_DELEGATE(FSpawnMobFinish);

USTRUCT(BlueprintType)
struct FSpawnConfig
{
	GENERATED_BODY()

public:
	// Number of mobs to spawn in this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCall = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FirstDelay = 2.0;

	// Interval between each spawn in this wave (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Interval = 1.0f;

	// Optional delay before starting this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartDelay = 0.0f;

	// Mob class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> MobClass = nullptr;
};



UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
	

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay()override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;



	//UFUNCTION(BlueprintNativeEvent, Category = Game)
	//virtual void HandleStartingNewPlayer(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);


	///////////////////////////////  AI  ////////////////////////////////
	// Timer handle for repeated calls
	FTimerHandle SpawnTimerHandle;

	UPROPERTY(BlueprintAssignable) 
	FOnMapClean OnMapClean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	FSpawnConfig SpawnConfig;

	UFUNCTION(BlueprintCallable)
	void StartSpawnTimer();

	// Function to call repeatedly
	void SpawnMob();

	void TriggerImmediateSpawn();
	void SomeEventThatTriggersImmediateSpawn();

	///////////////////////////////  AI  ////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bAutoSpawnPlayer = false;

	UPROPERTY(BlueprintAssignable)
	FOnFinishLevel OnFinishLevel;

	DECLARE_EVENT(AYogGameMode, FFinishLevelEvent)

	FFinishLevelEvent FinishLevelEvent;

	FFinishLevelEvent& OnFinishLevelEvent()
	{
		return FinishLevelEvent;
	}

	UFUNCTION(BlueprintCallable, Category = "KillCount")
	void UpdateFinishLevel(int count);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillCount")
	int MonsterKillCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillCount")
	int RemainKillCount;

	// =========================================================
	// 关卡流程
	// =========================================================

	// 当前阶段（战斗 / 整理 / 切换中）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	ELevelPhase CurrentPhase = ELevelPhase::Combat;

	// 关卡序列配置（在关卡 BP 或 GameMode BP 中指定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow")
	TObjectPtr<ULevelSequenceDataAsset> LevelSequenceData;

	// 当前生成的战利品选项（供 UI 读取）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	TArray<FLootOption> CurrentLootOptions;

	// 最后一个被击杀敌人的位置（用于生成奖励拾取物）
	FVector LastEnemyKillLocation = FVector::ZeroVector;

	// 关卡结算奖励拾取物的 Actor 类（在 GameMode BP 中指定 ARewardPickup）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow")
	TSubclassOf<AActor> RewardPickupClass;

	// 阶段变化事件
	UPROPERTY(BlueprintAssignable, Category = "LevelFlow|Events")
	FOnPhaseChanged OnPhaseChanged;

	// 战利品生成事件（传入 3 个选项）
	UPROPERTY(BlueprintAssignable, Category = "LevelFlow|Events")
	FOnLootGenerated OnLootGenerated;

	// 关卡清空后调用，进入整理阶段
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void EnterArrangementPhase();

	// 玩家选择一个战利品（0-2）
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void SelectLoot(int32 LootIndex);

	// 整理完成，锁背包并加载下一关（旧系统保留，新系统由 Portal 触发 TransitionToLevel）
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void ConfirmArrangementAndTransition();

	// Portal 触发切关：保存跑局状态（含选定房间）后 OpenLevel（由 APortal::EnterPortal 调用）
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void TransitionToLevel(FName NextLevel, URoomDataAsset* NextRoom = nullptr);

	// 从 LootPool 中随机生成战利品选项并广播（由 ARewardPickup 触发）
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void GenerateLootOptions();

	// =========================================================
	// 新刷怪系统（难度分预算波次）
	// =========================================================

	// 本次局内的关卡序列配置（在 GameMode BP 或 World Settings 中指定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	TObjectPtr<UCampaignDataAsset> CampaignData;

	// 当前是第几关（从 1 开始，对应 FloorTable 下标 0）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	int32 CurrentFloor = 1;

	// 开始关卡刷怪（在 StartPlay 中自动调用，也可在 BP 中手动调用）
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void StartLevelSpawning();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// ---- 传送门激活 ----
	void ActivatePortals();

	// 根据 FFloorConfig 的概率权重，从 CampaignData 的类型池中随机选取一个 DA_Room
	URoomDataAsset* RollRoomForFloor(const FFloorConfig& Config);

	// ---- 刷怪算法 ----

	// 波次计划（运行时数据，不需要 UE 反射）
	struct FWavePlan
	{
		ESpawnTriggerType TriggerType = ESpawnTriggerType::AllEnemiesDead;
		ESpawnMode        SpawnMode   = ESpawnMode::Wave;
		float             OneByOneInterval = 3.0f;
		TArray<TSubclassOf<AEnemyCharacterBase>> EnemiesToSpawn;
		int32 TotalSpawnedInWave = 0;
		int32 TotalKilledInWave  = 0;
	};

	// 根据难度配置生成所有波次计划
	void GenerateWavePlans(const FDifficultyConfig& Config, URoomDataAsset* Room);

	// 生成单波计划（含预算分配三步骤）
	FWavePlan BuildWavePlan(int32 Budget, const FDifficultyConfig& Config, URoomDataAsset* Room);

	// 触发并执行下一波（下标推进）
	void TriggerNextWave();

	// OneByOne 模式下，定时器回调，逐只刷怪
	void SpawnNextOneByOne();

	// 设置当前波次的触发条件监听
	void SetupWaveTrigger(const FWavePlan& Wave);

	// 时间间隔触发条件的定时器回调
	void OnWaveTriggerFired();

	// 每次有敌人死亡时检查是否满足下一波触发条件
	void CheckWaveTrigger();

	// 检查关卡是否完成（所有波次结束 + 场内清空）
	void CheckLevelComplete();

	// 从随机 MobSpawner 刷出指定类型的敌人
	void SpawnEnemyFromPool(TSubclassOf<AEnemyCharacterBase> EnemyClass);

	// 从 BuffPool 按难度数量随机选取 Buff
	TArray<UBuffDataAsset*> SelectRoomBuffs(
		const URoomDataAsset& Room, const FDifficultyConfig& Config);

	// ---- 刷怪运行时状态 ----
	TArray<FWavePlan> WavePlans;
	int32             CurrentWaveIndex    = -1;
	int32             TotalAliveEnemies   = 0;
	bool              bAllWavesSpawned    = false;
	bool              bCurrentRoomIsElite = false;

	FTimerHandle WaveTriggerTimer;
	FTimerHandle OneByOneTimer;
	TArray<TSubclassOf<AEnemyCharacterBase>> OneByOneSpawnQueue;
	int32 OneByOneSpawnIndex = 0;

	// 本关激活的敌人 Buff（进关时选好，新怪刷出时施加）
	TArray<UBuffDataAsset*> ActiveRoomBuffs;

	// 当前关卡的房间配置和难度配置（StartLevelSpawning 时缓存，整理阶段使用）
	UPROPERTY()
	TObjectPtr<URoomDataAsset> ActiveRoomData;
	FDifficultyConfig ActiveDifficultyConfig;

	UPROPERTY()
	int32 Current_CallCount;
};
