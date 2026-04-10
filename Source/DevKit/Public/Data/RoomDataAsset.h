// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameModes/SpawnTypes.h"
#include "Data/RuneDataAsset.h"
#include "RoomDataAsset.generated.h"

// 前向声明（FPortalDestConfig.RoomPool 需要引用本类）
class URoomDataAsset;

/**
 * FPortalDestConfig — 单个传送门的目标关卡配置
 *
 * 放在 RoomDataAsset.h 而非 SpawnTypes.h，因为需要引用 URoomDataAsset*（自引用）。
 * PortalIndex 与场景中 APortal.Index 一致。
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FPortalDestConfig
{
    GENERATED_BODY()

    // 匹配场景中 APortal.Index
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    int32 PortalIndex = 0;

    // 目标关卡随机池（填 UE 关卡资产名）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    TArray<FName> NextLevelPool;

    // 此门专属的 DA_Room 候选池（按 RoomTypeTag 过滤后使用）
    // 若此池中找不到所需类型，系统自动回退到 DA_Campaign 的全局 RoomPool
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
    TArray<TObjectPtr<URoomDataAsset>> RoomPool;
};

/**
 * URoomDataAsset — 单个关卡房间的完整配置
 *
 * 命名规范：DA_Room_<场景名>_<类型>
 * 例：DA_Room_Prison_Normal、DA_Room_Prison_Elite、DA_Room_Prison_Shop
 *
 * 同一张美术地图可对应多个 RoomDataAsset（类型不同），
 * 通过 RoomTypeTag 区分，系统按需选取对应类型的配置。
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

    // 房间类型标签（Room.Type.Normal / Room.Type.Elite / Room.Type.Shop / Room.Type.Event）
    // 决定此 DA_Room 属于哪种关卡类型，系统按 FloorConfig 概率骰子匹配
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    FGameplayTag RoomTypeTag;

    // =========================================================
    // 敌人池
    // =========================================================

    // 本房间可刷出的所有敌人（含难度分和精英标记）
    // 精英专属敌人（bEliteOnly）只在 RoomTypeTag == Room.Type.Elite 时出现
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemies")
    TArray<FEnemyEntry> EnemyPool;

    // =========================================================
    // 关卡符文池（给所有敌人的词条 Buff）
    // =========================================================

    // 进入关卡时从此池随机选取 N 个 RuneDA 施加给所有刷出的敌人
    // N 由当前难度的 FDifficultyConfig.BuffCount 决定
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RoomBuffs")
    TArray<TObjectPtr<URuneDataAsset>> BuffPool;

    // =========================================================
    // 玩家战利品池（符文三选一）
    // =========================================================

    // 关卡结算时从此池随机抽 3 个供玩家选择
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<TObjectPtr<URuneDataAsset>> LootPool;

    // =========================================================
    // 传送门目标配置
    // =========================================================

    // 关卡结算时各传送门可去往的关卡池（Index 对应场景中 APortal.Index）
    // 每个门可配置专属 DA_Room 候选池，找不到时回退到 Campaign 全局 RoomPool
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portals")
    TArray<FPortalDestConfig> PortalDestinations;

    // =========================================================
    // 难度配置（按需填，不强制三档全填）
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    TArray<FDifficultyEntry> DifficultyConfigs;
};
