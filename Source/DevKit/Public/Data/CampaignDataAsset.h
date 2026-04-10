// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModes/SpawnTypes.h"
#include "Data/RoomDataAsset.h"
#include "CampaignDataAsset.generated.h"

/**
 * FPortalDestConfig — 单个传送门的目标关卡配置
 * 每个传送门（APortal）对应一条，PortalIndex 与场景中 APortal.Index 一致
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FPortalDestConfig
{
    GENERATED_BODY()

    // 匹配场景中 APortal.Index
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    int32 PortalIndex = 0;

    // 目标关卡随机池，关卡结束时从中随机选一个
    // 填关卡资产名（与 Content Browser 中的名称一致）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    TArray<FName> NextLevelPool;
};

/**
 * FFloorEntry — 局内序列中，单关的配置
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorEntry
{
    GENERATED_BODY()

    // 第几关（从 1 开始，供策划排序参考，不强制连续）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    int32 FloorNumber = 1;

    // 此关使用的房间配置资产
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    TObjectPtr<URoomDataAsset> RoomData;

    // 此关的难度等级（系统据此选对应的 DifficultyEntry）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    EDifficultyTier Difficulty = EDifficultyTier::Low;

    // 此关对应的 UE 地图名（仅用于旧系统回退，新系统由传送门目标决定）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    FName LevelName;

    // 此关各传送门的目标池配置（Index 对应场景中 APortal.Index）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    TArray<FPortalDestConfig> PortalDestinations;
};

/**
 * UCampaignDataAsset — 一次完整局内流程的关卡序列表
 *
 * 命名规范：DA_Campaign_<名称>，例：DA_Campaign_MainRun
 *
 * 策划手动排列每一关，精确控制难度曲线和精英关出现位置。
 * GameMode 根据 CurrentFloor 下标逐关查表。
 */
UCLASS(BlueprintType)
class DEVKIT_API UCampaignDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // 关卡序列表（按游玩顺序从上到下填写）
    // 数组下标 0 对应第 1 关，下标 1 对应第 2 关，以此类推
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TArray<FFloorEntry> FloorTable;
};
