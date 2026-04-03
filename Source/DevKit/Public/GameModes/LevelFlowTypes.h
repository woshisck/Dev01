// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/RuneDataAsset.h"
#include "LevelFlowTypes.generated.h"

/**
 * 关卡阶段
 * Combat       战斗阶段：背包锁定，敌人刷新
 * Arrangement  整理阶段：背包解锁，显示战利品选择
 * Transitioning 切换中：已确认整理，正在加载下一关
 */
UENUM(BlueprintType)
enum class ELevelPhase : uint8
{
	Combat			UMETA(DisplayName = "Combat"),
	Arrangement		UMETA(DisplayName = "Arrangement"),
	Transitioning	UMETA(DisplayName = "Transitioning"),
};

/** 战利品类型（预留扩展） */
UENUM(BlueprintType)
enum class ELootType : uint8
{
	Rune			UMETA(DisplayName = "Rune"),
};

/** 单个战利品选项，由 GameMode 生成后广播给 UI */
USTRUCT(BlueprintType)
struct DEVKIT_API FLootOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	ELootType LootType = ELootType::Rune;

	// LootType == Rune 时有效
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<URuneDataAsset> RuneAsset = nullptr;
};

/**
 * 关卡序列数据资产
 * 在编辑器中为每一关创建一个实例，配置下一关场景名和掉落池。
 * 命名规范：DA_LevelSequence_<LevelName>，例如 DA_LevelSequence_Run01
 */
UCLASS(BlueprintType)
class DEVKIT_API ULevelSequenceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 下一关场景名（FName 对应 UE 中的关卡资产名）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FName NextLevelName;

	// 本关击杀目标（覆盖 LevelScript 中的 MonsterKillCountTarget）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	int32 KillTarget = 10;

	// 掉落池（从中随机选 3 个显示给玩家）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<TObjectPtr<URuneDataAsset>> LootPool;
};
