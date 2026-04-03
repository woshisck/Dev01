// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModes/SpawnTypes.h"
#include "Data/RuneDataAsset.h"
#include "GameplayEffect.h"
#include "RoomDataAsset.generated.h"

/**
 * URoomDataAsset — 单个关卡房间的完整配置
 *
 * 命名规范：DA_Room_<场景名>_<类型>
 * 例：DA_Room_Prison_Normal、DA_Room_Prison_Elite
 *
 * 每个 UE 关卡场景对应一个（或多个）RoomDataAsset。
 * GameMode 根据 CampaignDataAsset 的 FloorTable 决定使用哪套配置和哪个难度。
 */
UCLASS(BlueprintType)
class DEVKIT_API URoomDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // =========================================================
    // 房间标识
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    FName RoomName;

    // 是否为精英关（精英关才能刷出 bEliteOnly == true 的敌人）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    bool bIsEliteRoom = false;

    // =========================================================
    // 敌人池
    // =========================================================

    // 本房间可刷出的所有敌人（含难度分和精英标记）
    // 系统根据难度分预算随机抽取，精英专属敌人只在精英关出现
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemies")
    TArray<FEnemyEntry> EnemyPool;

    // =========================================================
    // 关卡 Buff 池（给所有敌人的词条）
    // =========================================================

    // 进入关卡时从此池随机选取 N 个 GE 施加给所有敌人
    // N 由当前难度的 FDifficultyConfig.BuffCount 决定
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    TArray<TSubclassOf<UGameplayEffect>> BuffPool;

    // =========================================================
    // 玩家战利品池（符文三选一）
    // =========================================================

    // 关卡结算时从此池随机抽 3 个供玩家选择
    // 至少填 3 个；若少于 3 个，有几个显示几个
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<TObjectPtr<URuneDataAsset>> LootPool;

    // =========================================================
    // 三套难度配置（低 / 中 / 高）
    // =========================================================

    // 低难度：通常出现在局内前期关卡
    // 建议：2-3 波，每波 15 分以内，触发条件全死，Wave 刷怪
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig LowConfig;

    // 中难度：局内中期关卡
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig MediumConfig;

    // 高难度：局内后期及精英关复用此套参数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig HighConfig;
};
