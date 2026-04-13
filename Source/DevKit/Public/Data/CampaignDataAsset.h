// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CampaignDataAsset.generated.h"

class URoomDataAsset;

/**
 * FFloorConfig — 局内序列中，单关的宏观配置
 *
 * 策划只需填写：
 *   - TotalDifficultyScore：本关总难度分（决定刷出敌人总量）
 *   - GoldMin / GoldMax：本关结算金币范围
 *   - BuffCount：从 RoomDA.BuffPool 中随机抽几个 Buff 施加给敌人
 *   - 房间类型概率 & 符文稀有度权重
 *
 * 波次数量、Wave/OneByOne 模式、触发条件等均由程序自动决定，
 * 具体档位上限（MaxWaveCount）在 RoomDataAsset 的 Low/Medium/High 档位中配置。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorConfig
{
    GENERATED_BODY()

    // 本关总难度分（程序用此值决定刷出的敌人总量 + 选取 RoomDA 难度档位）
    // 程序自动将总分按波次数均分：每波预算 ≈ TotalDifficultyScore / 实际波次数
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    int32 TotalDifficultyScore = 30;

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

};
// 注意：GoldMin/GoldMax/BuffCount/CommonWeight/RareWeight/EpicWeight
// 已移至 FRoomDifficultyTier（SpawnTypes.h），按 Low/Medium/High 档位分别配置。

/**
 * UCampaignDataAsset — 一次完整局内流程的关卡序列配置
 *
 * 命名规范：DA_Campaign_<名称>，例：DA_Campaign_MainRun
 */
UCLASS(BlueprintType)
class DEVKIT_API UCampaignDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // 关卡序列表（按游玩顺序从上到下填写，数量 = 局内总关数）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TArray<FFloorConfig> FloorTable;

    // 此 Campaign 对应的大关卡层级（Room.Layer.L1 / L2 / L3 ...）
    // SelectRoomByTag 过滤时，只选包含此 LayerTag 的 DA_Room
    // 若不填，层级过滤跳过（调试/单关测试时可留空）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FGameplayTag LayerTag;

    // ---- 全局 DA_Room 池（各房间自带 RoomTags 标明类型+层级）----
    // 关卡结算时，系统先查各传送门的专属 RoomPool，找不到对应类型才从此全局池回退
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPool")
    TArray<TObjectPtr<URoomDataAsset>> RoomPool;

    // ---- 第一关默认使用的 DA_Room ----
    // 未填写时，StartLevelSpawning 按 FloorTable[0] 的概率从全局 RoomPool 中骰子选取
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TObjectPtr<URoomDataAsset> DefaultStartingRoom;
};
