// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModes/SpawnTypes.h"
#include "Data/RuneDataAsset.h"
#include "Data/BuffDataAsset.h"
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

    // 进入关卡时从此池随机选取 N 个施加给所有敌人
    // N 由当前难度的 FDifficultyConfig.BuffCount 决定
    // 每个条目是一个 DA_Buff_* 资产（含名称/描述/GE）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    TArray<TObjectPtr<UBuffDataAsset>> BuffPool;

    // =========================================================
    // 玩家战利品池（符文三选一）
    // =========================================================

    // 关卡结算时从此池随机抽 3 个供玩家选择
    // 至少填 3 个；若少于 3 个，有几个显示几个
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<TObjectPtr<URuneDataAsset>> LootPool;

    // =========================================================
    // 难度配置（按需填，不强制三档全填）
    // =========================================================

    // 此房间支持的难度档位，CampaignData 填写的 Difficulty 必须在此列表中
    // 若 CampaignData 请求的难度不在列表中，自动降级到列表中最低的一档
    // 示例：只填 Low → 此房间永远是低难度；填 Low+High → 跳过 Medium
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    TArray<FDifficultyEntry> DifficultyConfigs;
};
