// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "GameplayTagContainer.h"
#include "GameModes/LevelFlowTypes.h"
#include "GameModes/SpawnTypes.h"
#include "GameModes/GameLifecycleTypes.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "Data/SacrificeGraceDA.h"
#include "Containers/Ticker.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class APlayerCharacterBase;
class UYogSaveGame;
class AEnemyCharacterBase;
class APortal;
class ARewardPickup;
class AAltarActor;
class AShopActor;
class ULootSelectionWidget;
class ULevelFlowAsset;
class UFlowComponent;
class AMobSpawner;
class UNiagaraSystem;
class URuneDataAsset;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapClean);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ELevelPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootGenerated, const TArray<FLootOption>&, LootOptions);
DECLARE_MULTICAST_DELEGATE(FOnLootSelected);


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

	static bool ShouldSkipCombatForRoom(const URoomDataAsset* RoomData);



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
	// 敌人注册表（供相机感知使用）
	// =========================================================

	/** 敌人出生时注册，提供给 CameraPawn 做战斗感知偏移 */
	void RegisterEnemy(AEnemyCharacterBase* Enemy);

	/** 敌人死亡时注销 */
	void UnregisterEnemy(AEnemyCharacterBase* Enemy);

	/** 当前是否有存活的敌人（包含已注册但尚未死亡的） */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	bool HasAliveEnemies() const;

	/**
	 * 获取 Origin 半径 WithinRadius 内所有存活敌人的位置质心。
	 * 若范围内无敌人，返回 Origin 本身（偏移量为零）。
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	FVector GetEnemyCentroid(FVector Origin, float WithinRadius) const;

	/**
	 * 获取距 Origin 最近的存活敌人方向（单位向量）。
	 * 若无存活敌人，返回 FVector::ZeroVector。
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	FVector GetNearestEnemyDirection(FVector Origin) const;

	/**
	 * 获取 Origin 半径 WithinRadius 内的所有存活敌人指针列表。
	 * 不包含已死亡或已销毁的对象。
	 */
	TArray<AEnemyCharacterBase*> GetNearbyEnemies(FVector Origin, float WithinRadius) const;

	/** 返回当前所有存活敌人（无距离过滤，供 EnemyArrowWidget 使用） */
	TArray<AEnemyCharacterBase*> GetAllAliveEnemies() const;

	// =========================================================
	// 关卡流程
	// =========================================================

	// 当前阶段（战斗 / 整理 / 切换中）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	ELevelPhase CurrentPhase = ELevelPhase::Combat;

	// 当前生成的战利品选项（供 UI 读取）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	TArray<FLootOption> CurrentLootOptions;

	// 最后一个被击杀敌人的位置（用于生成奖励拾取物）
	FVector LastEnemyKillLocation = FVector::ZeroVector;

	// 关卡结算奖励拾取物的 Actor 类（在 GameMode BP 中指定 ARewardPickup）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow")
	TSubclassOf<ARewardPickup> RewardPickupClass;



	// 进关后延迟多少秒再开始刷怪（给特效/动画和 AI 初始化预留时间）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow", meta = (ClampMin = "0.0"))
	float InitialSpawnDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow|Temporary Finisher")
	bool bCountCombatClearsForTemporaryFinisherUnlock = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow|Temporary Finisher")
	int32 CompletedCombatBattleCount = 0;

	UFUNCTION(BlueprintPure, Category = "LevelFlow|Temporary Finisher")
	int32 GetCompletedCombatBattleCount() const { return CompletedCombatBattleCount; }

	// 阶段变化事件
	UPROPERTY(BlueprintAssignable, Category = "LevelFlow|Events")
	FOnPhaseChanged OnPhaseChanged;

	// 战利品生成事件（传入 3 个选项）
	UPROPERTY(BlueprintAssignable, Category = "LevelFlow|Events")
	FOnLootGenerated OnLootGenerated;

	// 玩家选定符文后广播（供 LevelFlow LENode_WaitForLootSelected 等待）
	FOnLootSelected OnLootSelected;

	// LootSelectionWidget 被 CommonUI 销毁后重建时，NativeConstruct 检查此标志自动激活
	UPROPERTY(BlueprintReadOnly, Category = "LevelFlow")
	bool bLootOptionsPending = false;

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

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void HandlePlayerDeath(APlayerCharacterBase* Player);

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	bool RevivePlayerFromDeath();

	UFUNCTION(BlueprintPure, Category = "GameOver")
	bool IsPlayerDeathPending() const { return bPlayerDeathPending; }

	static bool CanOfferPlayerDeathRevive(bool bInGameOverTriggered, bool bInPlayerDeathReviveUsed);
	static float CalculatePlayerReviveHealth(float MaxHealth, float ReviveHealthPercent);

	// 从 LootPool 中随机生成战利品选项并广播（由 ARewardPickup 兜底路径触发）
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void GenerateLootOptions();

	// 独立生成战利品（不广播、不调 HUD），供 ARewardPickup 手动放置路径使用
	// 设置 CurrentLootOptions/bLootOptionsPending 后返回结果，由调用方决定何时显示 UI
	TArray<FLootOption> GenerateIndependentLootOptions();

	/**
	 * 将已生成的选项广播给 UI（由 ARewardPickup::TryPickup 使用预分配路径调用）。
	 * 不重新生成选项，直接用传入的 Options。
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void ShowLootOptions(const TArray<FLootOption>& Options);

	/**
	 * 兜底符文池：当 ActiveRoomData 为空时（如初始关卡/独立测试关卡）使用
	 * 在 GameMode BP 的 Details 面板中填入 DA_Rune_* 资产即可
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelFlow")
	TArray<TObjectPtr<URuneDataAsset>> FallbackLootPool;

	// ─── 献祭恩赐额外掉落 ────────────────────────────────────────────

	/** 献祭恩赐 DA 候选池（随机抽一个） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SacrificeGrace")
	TArray<TObjectPtr<USacrificeGraceDA>> SacrificeGracePool;

	/** 每关触发概率（非主城关卡，0~1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SacrificeGrace",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SacrificeDropChance = 0.15f;

	/** 生成的献祭拾取物 Actor 类（在 BP 中配置，接受/拒绝弹窗由拾取物自己处理） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SacrificeGrace")
	TSubclassOf<AActor> SacrificePickupClass;

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

	UFUNCTION(BlueprintPure, Category = "LevelFlow|RoomBuff")
	TArray<FBuffEntry> GetActiveRoomBuffs() const { return ActiveRoomBuffs; }

	UFUNCTION(BlueprintPure, Category = "LevelFlow|RoomBuff")
	URoomDataAsset* GetActiveRoomData() const { return ActiveRoomData; }

	/**
	 * 根据当前总难度分选取房间的难度档位（Low / Medium / High）。
	 * 公共静态：StartLevelSpawning（当前关）和 ActivatePortals 预骰下一关 Buff 时共用，
	 * 避免选档逻辑漂移。
	 */
	static const FRoomDifficultyTier& ResolveTier(const URoomDataAsset& Room, int32 TotalScore,
	                                              int32 LowMax, int32 HighMin);

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// ---- 传送门激活 ----
	void ActivatePortals();

	// 主城/枢纽房间：立即全开所有已配置的传送门（不走 50% 随机规则）
	void ActivateHubPortals();

	// 根据 FFloorConfig 的概率权重骰出此关的房间类型 Tag（Room.Type.Normal/Elite/Shop/Event）
	FGameplayTag RollRoomTypeForFloor(const FFloorConfig& Config);

	// 按类型 Tag 选取 DA_Room：先查传送门专属 RoomPool，再查 Campaign 全局 RoomPool，最后退化为 Normal
	URoomDataAsset* SelectRoomByTag(const FPortalDestConfig* PortalDest, FGameplayTag RequiredTag);

	// ---- 刷怪算法 ----

	// 单只待刷敌人的完整计划（类型 + 本次选中的敌人专属 Buff）
	struct FPlannedEnemy
	{
		TSubclassOf<AEnemyCharacterBase> EnemyClass;
		// Rolled from EnemyData.EnemyBuffPool. Only entries that pass ApplyChance are granted.
		TArray<TObjectPtr<URuneDataAsset>> EnemyBuffs;
		// 从 EnemyData 复制，运行时只读
		TObjectPtr<UNiagaraSystem> PreSpawnFX;
		float PreSpawnFXDuration = 0.f;
	};

	// 波次计划（运行时数据，不需要 UE 反射）
	struct FWavePlan
	{
		ESpawnTriggerType TriggerType = ESpawnTriggerType::AllEnemiesDead;
		ESpawnMode        SpawnMode   = ESpawnMode::Wave;
		float             OneByOneInterval = 3.0f;
		TArray<FPlannedEnemy> EnemiesToSpawn;
		int32 TotalSpawnedInWave = 0;
		int32 TotalKilledInWave  = 0;

		// 按需补刷队列（预算剩余且整关上限仍有空间时，每死一只补刷一只）
		TArray<FPlannedEnemy> DemandEnemyPool;
		// 剩余可按需补刷的次数（0 = 无补刷）
		int32 DemandCount = 0;

		// TimeInterval 触发时使用的间隔（秒），由 BuildWavePlan 从所选触发条件中读取
		float WaveTriggerInterval = 3.0f;
	};

	// 根据总难度分和档位最大波次数生成所有波次计划
	void GenerateWavePlans(int32 TotalScore, int32 MaxWaveCount, URoomDataAsset* Room);

	// 生成单波计划（程序自动决定触发条件和刷怪方式）
	FWavePlan BuildWavePlan(int32 Budget, URoomDataAsset* Room, int32 MaxEnemiesPerWave, int32 MaxEnemiesPerLevel);

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
	void HandleTimedClearObjectiveExpired();
	void SpawnSacrificeEventAltar(const FVector& LootSpawnLoc);
	bool IsSacrificeEventRoom() const;
	void SpawnShopActorForRoom();
	bool IsShopRoom() const;

	// 波次系统未能初始化时，降级统计场景内预放置敌人数量
	void FallbackToPreplacedEnemies();

	// 从随机 MobSpawner 刷出指定敌人（含专属 Buff 激活）【旧路径，DemandSpawn 仍使用】
	bool SpawnEnemyFromPool(const FPlannedEnemy& Planned);

	// 新路径：播放预生成 FX，FX 结束后才真正 SpawnActor
	// 返回 false 表示找不到兼容 Spawner（调用方可立即跳过，不等 Timer）
	bool BeginSpawnEnemyFromPool(const FPlannedEnemy& Planned);
	void FinishSpawnFromPool(FPlannedEnemy Planned,
		TWeakObjectPtr<AMobSpawner> WeakSpawner, FVector Location, int32 WaveIdx);

	// 从 BuffPool 随机选取 BuffCount 个关卡 Buff 条目
	TArray<FBuffEntry> SelectRoomBuffs(const URoomDataAsset& Room, int32 BuffCount);

	// ---- 刷怪运行时状态 ----
	TArray<FWavePlan> WavePlans;
	int32             CurrentWaveIndex    = -1;
	int32             TotalAliveEnemies   = 0;
	bool              bAllWavesSpawned    = false;
	// FX 播放中、尚未真正 SpawnActor 的敌人数（防止 CheckLevelComplete 提前结算）
	int32             PendingSpawnCount   = 0;

	FTimerHandle WaveTriggerTimer;
	FTimerHandle OneByOneTimer;
	FTimerHandle InitialSpawnDelayTimer;
	FTimerHandle DemandSpawnTimer;
	FTimerHandle TimedClearObjectiveTimer;
	bool bTimedClearObjectiveActive = false;
	bool bTimedClearObjectiveExpired = false;
	FTimerHandle PlayerDeathGameOverTimer;
	TArray<FPlannedEnemy> OneByOneSpawnQueue;
	int32 OneByOneSpawnIndex = 0;

	// Wave 模式下使用随机错开间隔（而非 OneByOne 的固定间隔）
	bool bWaveStaggerMode = false;

	// 关卡内各类型已计划刷出数量（BuildWavePlan 时累计，跨波次）
	TMap<TSubclassOf<AEnemyCharacterBase>, int32> LevelTypeSpawnCounts;
	int32 TotalLevelPlannedEnemies = 0;

	// 按需补刷：每当有敌人死亡且当前波次 DemandCount > 0 时，延迟补刷一只
	void CheckDemandSpawn();

	// ---- 敌人注册表（相机战斗感知）----
	// 使用原始指针 + 手动生命期管理（RegisterEnemy/UnregisterEnemy 保持同步）
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> AliveEnemies;

	// 本关激活的关卡 Buff（进关时骰子选好，新怪刷出时在其 BuffFlowComponent 上激活）
	TArray<FBuffEntry> ActiveRoomBuffs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	float PlayerDeathGameOverDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "1.0"))
	float PlayerReviveHealthPercent = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerReviveProtectionDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "1.0"))
	float PlayerDeathTimeDilationScale = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bGameOverTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bPlayerDeathReviveUsed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bPlayerDeathPending = false;

	UPROPERTY()
	TWeakObjectPtr<APlayerCharacterBase> PendingDeathPlayer;

	FTSTicker::FDelegateHandle PlayerDeathGameOverTickerHandle;

	// 当前关卡的房间配置（StartLevelSpawning 时缓存，整理阶段使用）
	UPROPERTY()
	TObjectPtr<URoomDataAsset> ActiveRoomData;

	// 当前关卡的奖励配置（从 FloorConfig 缓存，整理阶段使用）
	int32 ActiveGoldMin  = 10;
	int32 ActiveGoldMax  = 20;
	int32 ActiveBuffCount = 1;

	// ---- 刷怪参数（在 GameMode BP 中可调）----

	// 选档阈值：TotalDifficultyScore ≤ 此值 → Low 档
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	int32 LowDifficultyScoreMax = 25;

	// 选档阈值：TotalDifficultyScore ≥ 此值 → High 档（中间为 Medium）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	int32 HighDifficultyScoreMin = 50;

	// Wave 模式每只怪之间的随机错开延迟范围（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0"))
	float SpawnStaggerMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.0"))
	float SpawnStaggerMax = 0.5f;

	// OneByOne 模式每只怪之间的固定间隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0.1"))
	float OneByOneDefaultInterval = 3.0f;

	UPROPERTY()
	int32 Current_CallCount;

	// ---- 战利品生成 ----

	/**
	 * 从符文池中生成一批（最多 3 个）不与 AlreadyOffered 重复的选项。
	 * 生成的符文 DA 指针会被写入 AlreadyOffered，供同关下一次调用去重。
	 */
	TArray<FLootOption> GenerateLootBatch(TSet<URuneDataAsset*>& AlreadyOffered);

	// 本关已分配给各拾取物的符文集合（防止同关多个拾取物提供重复选项）
	TSet<URuneDataAsset*> LootAssignedThisLevel;

	/**
	 * 寻找 Loot 最佳落点（玩家前方优先）：
	 * 从 8 个方向候选中选取同时满足以下条件的第一个：
	 *   1. 玩家→候选点之间无碰撞（不在角落/墙后）
	 *   2. 相机→候选点之间无遮挡（相机可见）
	 *   3. 候选点在屏幕范围内（含 5% 边缘余量）
	 * 全部失败时退回玩家原位。
	 */
	FVector FindLootSpawnLocation(APawn* PlayerPawn, APlayerController* PC) const;

public:
	// =========================================================
	// 关卡生命周期事件总线
	// =========================================================

	/**
	 * 编辑器内可配：事件 -> 跑哪个 LevelFlowAsset。
	 * 在 BP_GameMode_Default Class Defaults 里配 EGameLifecycleEvent::LevelClearRevealed -> LFA_FirstRuneTutorial
	 * 等映射，节点图里用 LENode_ShowTutorial / LENode_Delay 编排具体行为。
	 * 没配映射的事件触发时不做任何事（沉默）。
	 */
	UPROPERTY(Transient)
	TMap<EGameLifecycleEvent, TObjectPtr<ULevelFlowAsset>> LifecycleEventFlows;

	/**
	 * 触发一个生命周期事件 — 查 LifecycleEventFlows 拿 Flow Asset，跑节点图。
	 * 一次性事件（FirstRune* / HeatPhase* / GameStart / PlayerDeath）内部自动去重。
	 */
	UFUNCTION(BlueprintCallable, Category = "Lifecycle Events")
	void TriggerLifecycleEvent(EGameLifecycleEvent Event);

protected:
	/** 一次性事件去重集合（Transient，跨 PIE 不持久；存档恢复用 TutorialState 兜底）*/
	UPROPERTY(Transient)
	TSet<EGameLifecycleEvent> FiredOnceEvents;

	/** 事件总线统一使用的 FlowComponent（同一时间只跑一个 Flow，新触发会停旧的）*/
	UPROPERTY()
	TObjectPtr<UFlowComponent> LifecycleFlowComponent;

private:
	/** HUD 揭幕动画完成的回调（GameMode::BeginPlay 里订阅 HUD->OnLevelEndEffectFinished）*/
	void HandleLevelEndEffectFinished();

	/** 尝试绑定 HUD 委托：HUD 由 PC->ClientRestart 异步创建，BeginPlay 时通常还没准备好 */
	void TryBindHUDDelegates();
	int32 HUDBindRetryCount = 0;

	void FinishPlayerDeathGameOver();
	void ClearPlayerDeathGameOverTicker();
	void BeginPlayerDeathVisuals(APlayerCharacterBase* Player);
	void EndPlayerDeathVisuals(APlayerCharacterBase* Player);

	/** 是否为一次性事件（FirstRune* / HeatPhase* / GameStart / PlayerDeath）*/
	static bool IsOneShotEvent(EGameLifecycleEvent Event);
};
