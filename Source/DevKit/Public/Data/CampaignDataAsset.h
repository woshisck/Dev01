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
 * 具体 DA_Room 在关卡结算时由系统按类型 Tag 从传送门 RoomPool / 全局 RoomPool 中选取。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FFloorConfig
{
    GENERATED_BODY()

    // 第几关（从 1 开始，供策划排序参考，不强制连续）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    int32 FloorNumber = 1;

    // ---- 此关的刷怪难度配置（波数、预算、触发条件等）----
    // 难度分预算决定本关刷出的敌人总量，波次数量在 [WaveCountMin, WaveCountMax] 间随机
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    FDifficultyConfig DifficultyConfig;

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
};

/**
 * UCampaignDataAsset — 一次完整局内流程的关卡序列配置
 *
 * 命名规范：DA_Campaign_<名称>，例：DA_Campaign_MainRun
 *
 * 关卡序列通过 FloorTable 定义难度曲线和房间类型概率；
 * 具体 DA_Room 由系统在关卡结算时按 RoomTypeTag 从各传送门专属池或全局 RoomPool 中选取。
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
    // 策划在此填写所有可用的 DA_Room 资产（Normal / Elite / Shop / Event 均放在一起）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomPool")
    TArray<TObjectPtr<URoomDataAsset>> RoomPool;

    // ---- 第一关默认使用的 DA_Room ----
    // 未填写时，StartLevelSpawning 按 FloorTable[0] 的概率从全局 RoomPool 中骰子选取
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TObjectPtr<URoomDataAsset> DefaultStartingRoom;
};
