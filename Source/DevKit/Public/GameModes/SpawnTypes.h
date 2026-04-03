// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpawnTypes.generated.h"

class AEnemyCharacterBase;
class UGameplayEffect;

// =========================================================
// 难度等级
// =========================================================

UENUM(BlueprintType)
enum class EDifficultyTier : uint8
{
    Low     UMETA(DisplayName = "低难度"),
    Medium  UMETA(DisplayName = "中难度"),
    High    UMETA(DisplayName = "高难度"),
    Elite   UMETA(DisplayName = "精英关"),
};

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
    // 本波开始 5 秒后触发 —— 消耗 2 分
    TimeInterval_5s     UMETA(DisplayName = "5秒后触发"),
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
};

// =========================================================
// 刷怪方式（消耗难度分预算）
// =========================================================

UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    // 所有敌人同时刷出 —— 消耗 1 分
    Wave        UMETA(DisplayName = "波次同时刷出"),
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

    // 敌人 Actor 类
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    TSubclassOf<AEnemyCharacterBase> EnemyClass;

    // 该敌人在难度分系统中的费用
    // 建议：普通怪 2-4 分，精英怪 6-10 分
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    int32 DifficultyScore = 3;

    // 是否为精英专属（仅精英关才能刷出）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    bool bEliteOnly = false;

    // 高难度版本（高难度关卡下替换此类；若未配置则附加霸体GE）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    TSubclassOf<AEnemyCharacterBase> EliteVariantClass;
};

// =========================================================
// 难度等级配置（低/中/高三套，各自独立）
// =========================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FDifficultyConfig
{
    GENERATED_BODY()

    // ---- 波次数量 ----
    // 随机值域 [WaveCountMin, WaveCountMax]，策划可写相同值固定波次数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMin = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMax = 3;

    // 每波的难度分预算，数组长度对应最大波数
    // 若实际波数超出数组长度，循环使用最后一个值
    // 例：[15, 20, 15] → 第1波15分，第2波20分，第3波15分
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    TArray<int32> WaveBudgets = { 15, 20, 15 };

    // ---- 可用触发条件池 ----
    // 策划填入此难度档允许出现的触发条件，系统从中随机抽取
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

    // ---- 关卡 Buff 数量（从 BuffPool 随机选取的数量）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 BuffCount = 2;

    // ---- 金币奖励范围 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMin = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMax = 20;
};
