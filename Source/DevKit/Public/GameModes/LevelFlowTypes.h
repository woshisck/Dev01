// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

