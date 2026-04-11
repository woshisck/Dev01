// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyData.h"
#include "SpawnTypes.generated.h"

// =========================================================
// 下一波触发条件（消耗难度分预算）
// =========================================================

UENUM(BlueprintType)
enum class ESpawnTriggerType : uint8
{
    // 场内无敌人后触发（最安全）—— 消耗 0 分
    AllEnemiesDead      UMETA(DisplayName = "全部死亡"),
    // 本波 50% 敌人死亡后触发 —— 消耗 2 分
    PercentKilled_50    UMETA(DisplayName = "死亡50%"),
    // 本波 20% 敌人死亡后触发（压力最大）—— 消耗 3 分
    PercentKilled_20    UMETA(DisplayName = "死亡20%"),
    // 计时触发（间隔由 FSpawnTriggerOption.TriggerInterval 配置）—— 消耗 2 分
    // 保底：场内无敌人时立即触发，不等计时结束
    TimeInterval        UMETA(DisplayName = "计时触发"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FSpawnTriggerOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    ESpawnTriggerType TriggerType = ESpawnTriggerType::AllEnemiesDead;

    // 使用此触发条件所消耗的难度分
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    int32 DifficultyScore = 0;

    // 计时触发的间隔（秒），仅 TimeInterval 类型有效
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (EditCondition = "TriggerType == ESpawnTriggerType::TimeInterval", ClampMin = "0.5"))
    float TriggerInterval = 3.0f;
};

// =========================================================
// 刷怪方式（消耗难度分预算）
// =========================================================

UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    // 所有敌人随机错开刷出 —— 消耗 1 分
    Wave        UMETA(DisplayName = "波次错开刷出"),
    // 按固定时间间隔逐只刷出 —— 消耗 1 分
    OneByOne    UMETA(DisplayName = "逐个刷入"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FSpawnModeOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    ESpawnMode SpawnMode = ESpawnMode::Wave;

    // 使用此刷怪方式所消耗的难度分
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    int32 DifficultyScore = 1;

    // OneByOne 模式下，每只怪之间的间隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (EditCondition = "SpawnMode == ESpawnMode::OneByOne"))
    float OneByOneInterval = 3.0f;
};

// =========================================================
// 敌人池条目
// =========================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyEntry
{
    GENERATED_BODY()

    // 引用敌人数据资产（含难度分、是否精英专属、Actor 类）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    TObjectPtr<UEnemyData> EnemyData;

    // 该类型敌人在本关卡内最多出现多少只（跨所有波次累计）
    // -1 = 不限制
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy",
        meta = (ClampMin = "-1"))
    int32 MaxCountPerLevel = -1;
};

// =========================================================
// 难度配置（现移至 DA_Campaign 的 FFloorConfig 内）
// =========================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FDifficultyConfig
{
    GENERATED_BODY()

    // ---- 波次数量（随机值域）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMin = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMax = 3;

    // ---- 单波难度分预算（每波独立随机）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveBudgetMin = 15;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveBudgetMax = 25;

    // ---- 可用触发条件池 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    TArray<FSpawnTriggerOption> AllowedTriggers;

    // ---- 可用刷怪方式池 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    TArray<FSpawnModeOption> AllowedSpawnModes;

    // ---- 数值倍率 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float HealthMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float DamageMultiplier = 1.0f;

    // ---- 关卡 Buff 数量（从 RoomDA.BuffPool 随机选取的数量）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 BuffCount = 1;

    // ---- 金币奖励范围 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMin = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMax = 20;

    // ---- 关卡总敌人上限（跨所有波次累计，-1 = 不限制）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves",
        meta = (ClampMin = "-1"))
    int32 MaxTotalEnemies = -1;

    // ---- 刷敌时间错开（每只之间的随机延迟上限，秒）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (ClampMin = "0.0"))
    float SpawnStaggerMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (ClampMin = "0.0"))
    float SpawnStaggerMax = 0.5f;
};

// FPortalDestConfig 已移至 RoomDataAsset.h（需引用 URoomDataAsset* 自身）
