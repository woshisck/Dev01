// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModes/SpawnTypes.h"
#include "Data/RoomDataAsset.h"
#include "CampaignDataAsset.generated.h"

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

    // 此关的难度等级（系统据此选 Low/Medium/High/Elite Config）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    EDifficultyTier Difficulty = EDifficultyTier::Low;

    // 对应的 UE 关卡场景名（ConfirmArrangementAndTransition 时用于 OpenLevel）
    // 必须与 Content Browser 中的关卡资产名完全一致
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    FName LevelName;
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
