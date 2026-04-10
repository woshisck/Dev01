// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameModes/SpawnTypes.h"
#include "CampaignDataAsset.generated.h"

class URoomDataAsset;

/**
 * FFloorConfig — 局内序列中，单关的宏观配置
 *
 * 不直接指定 DA_Room，而是配置难度曲线和各房间类型的概率权重。
 * 具体 DA_Room 在关卡结束时由骰子从 CampaignDataAsset 的类型池中随机选取。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorConfig
{
    GENERATED_BODY()

    // 第几关（从 1 开始，供策划排序参考，不强制连续）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    int32 FloorNumber = 1;

    // 此关的难度等级，决定 DA_Room 中使用哪套 DifficultyConfig
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    EDifficultyTier Difficulty = EDifficultyTier::Low;

    // ---- 房间类型概率 ----
    // 强制精英关（覆盖 EliteChance，必出精英）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomType")
    bool bForceElite = false;

    // 精英关概率（0.0 ~ 1.0），bForceElite=false 时生效
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomType", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EliteChance = 0.2f;

    // 商店概率（0.0 ~ 1.0）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomType", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ShopChance = 0.15f;

    // 事件房间概率（0.0 ~ 1.0）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomType", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EventChance = 0.1f;

    // ---- 符文奖励稀有度权重（相对权重，无需归一化）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float CommonWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float RareWeight = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float EpicWeight = 0.1f;
    // 传送门目标配置已移至 RoomDataAsset.PortalDestinations
};

/**
 * UCampaignDataAsset — 一次完整局内流程的关卡序列配置
 *
 * 命名规范：DA_Campaign_<名称>，例：DA_Campaign_MainRun
 *
 * 关卡序列通过 FloorTable 定义难度曲线；
 * 具体使用哪个 DA_Room 由运行时骰子从下方四个房间类型池中随机选取。
 */
UCLASS(BlueprintType)
class DEVKIT_API UCampaignDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // 关卡序列表（按游玩顺序从上到下填写，数量 = 局内总关数）
    // 每关只填宏观配置（难度等级 + 房间类型概率），不直接指定 DA_Room
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TArray<FFloorConfig> FloorTable;

    // ---- 按房间类型划分的 DA_Room 池 ----
    // 关卡结束时，ActivatePortals 为每个传送门独立从对应池中随机抽取 DA_Room

    // 普通战斗房（最常见，通用战斗配置）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPools")
    TArray<TObjectPtr<URoomDataAsset>> NormalRoomPool;

    // 精英战斗房（强敌，更好奖励）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPools")
    TArray<TObjectPtr<URoomDataAsset>> EliteRoomPool;

    // 商店房（金币消耗，符文购买）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPools")
    TArray<TObjectPtr<URoomDataAsset>> ShopRoomPool;

    // 事件房（选择、交换、特殊机制）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPools")
    TArray<TObjectPtr<URoomDataAsset>> EventRoomPool;

    // ---- 第一关默认使用的 DA_Room ----
    // 未填写时，StartLevelSpawning 自动按 FloorTable[0] 的概率骰子选取
    // 主城传送门直接进入当前编辑器关卡，DA_Room 由此字段决定
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TObjectPtr<URoomDataAsset> DefaultStartingRoom;
};
