// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyData.h"
#include "SpawnTypes.generated.h"

// =========================================================
// 下一波触发条件（内部代码使用，不在 DA 中配置）
// =========================================================

UENUM(BlueprintType)
enum class ESpawnTriggerType : uint8
{
    // 场内无敌人后触发（最安全，默认）
    AllEnemiesDead      UMETA(DisplayName = "全部死亡"),
    // 本波 50% 敌人死亡后触发
    PercentKilled_50    UMETA(DisplayName = "死亡50%"),
    // 本波 20% 敌人死亡后触发（压力最大）
    PercentKilled_20    UMETA(DisplayName = "死亡20%"),
    // 计时触发，保底：场内无敌人时立即触发
    TimeInterval        UMETA(DisplayName = "计时触发"),
};

// =========================================================
// 刷怪方式（内部代码使用，不在 DA 中配置）
// =========================================================

UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    // 所有敌人随机错开刷出
    Wave        UMETA(DisplayName = "波次错开刷出"),
    // 按固定时间间隔逐只刷出
    OneByOne    UMETA(DisplayName = "逐个刷入"),
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
// 房间难度档位（在 RoomDataAsset 中配置 Low/Medium/High）
//
// 程序根据 DA_Campaign 中该关的 TotalDifficultyScore 自动选档：
//   Low    — Score ≤ GameMode.LowDifficultyScoreMax
//   Medium — Score ∈ (LowMax, HighMin)
//   High   — Score ≥ GameMode.HighDifficultyScoreMin
//
// 策划只需在此填写最大波次数；是 Wave 还是 OneByOne 由程序自动决定。
// =========================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRoomDifficultyTier
{
    GENERATED_BODY()

    // 本档位允许的最大波次数（程序在 [1, MaxWaveCount] 内随机选取实际波次）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "1"))
    int32 MaxWaveCount = 3;

    // 单波最多同屏存活敌人数（0 = 不限制）
    // 超出上限时，BuildWavePlan 优先选取难度分 + Buff 合计最高的敌人填满上限
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "0"))
    int32 MaxEnemiesPerWave = 0;

    // 本关最多计划刷出的敌人数（0 = 不限制）。小房间建议 10，大房间建议 20。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "0"))
    int32 MaxEnemiesPerLevel = 0;

    // ---- 关卡奖励（按档位配置，难度越高奖励越丰厚）----

    // 关卡结算金币范围
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMin = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMax = 20;

    // 从 RoomDA.BuffPool 中随机选取施加给所有敌人的 Buff 数量（0 = 不施加）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward",
        meta = (ClampMin = "0"))
    int32 BuffCount = 1;

    // 符文奖励稀有度权重（相对权重，无需归一化）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward",
        meta = (ClampMin = "0.0"))
    float CommonWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward",
        meta = (ClampMin = "0.0"))
    float RareWeight = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward",
        meta = (ClampMin = "0.0"))
    float EpicWeight = 0.1f;
};
