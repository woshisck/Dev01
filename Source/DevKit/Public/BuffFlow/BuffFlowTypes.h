#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlowTypes.generated.h"

/**
 * GE 堆叠模式
 *   None      — 每次命中独立创建一个 GE 实例（无去重）
 *   Unique    — 同目标只保留一个实例，重复命中刷新持续时间
 *   Stackable — 同目标共享一个实例，可叠加多层（配合 Max Stacks 使用）
 */
UENUM(BlueprintType)
enum class EBFGEStackMode : uint8
{
	None      UMETA(DisplayName = "None"),
	Unique    UMETA(DisplayName = "Unique"),
	Stackable UMETA(DisplayName = "Stackable"),
};

/**
 * 目标选择器 —— BuffFlow 节点中选择作用目标
 */
UENUM(BlueprintType)
enum class EBFTargetSelector : uint8
{
	BuffOwner        UMETA(DisplayName = "Buff拥有者"),
	BuffGiver        UMETA(DisplayName = "Buff施加者"),
	LastDamageTarget UMETA(DisplayName = "上次伤害目标"),   // DamageReceiver（被击者）
	DamageCauser     UMETA(DisplayName = "伤害来源"),       // DamageCauser（攻击者）
};

/**
 * 事件上下文 —— 由触发器节点写入，供后续节点读取
 * 统一替代之前分散的 LastDamageTargetActor 字段
 */
USTRUCT(BlueprintType)
struct FBFEventContext
{
	GENERATED_BODY()

	/** 伤害来源（攻击者） */
	UPROPERTY()
	TWeakObjectPtr<AActor> DamageCauser;

	/** 受到伤害的人（被击者） */
	UPROPERTY()
	TWeakObjectPtr<AActor> DamageReceiver;

	/** 伤害量 */
	UPROPERTY()
	float DamageAmount = 0.f;

	/** 供 OnBuffAdded / OnBuffRemoved 节点使用的 Tag */
	UPROPERTY()
	FGameplayTag EventTag;

	void Reset()
	{
		DamageCauser.Reset();
		DamageReceiver.Reset();
		DamageAmount = 0.f;
		EventTag = FGameplayTag::EmptyTag;
	}
};

/**
 * 数学运算类型 —— 供 BFNode_MathFloat / BFNode_MathInt 使用
 */
UENUM(BlueprintType)
enum class EBFMathOp : uint8
{
	Add      UMETA(DisplayName = "+"),
	Subtract UMETA(DisplayName = "-"),
	Multiply UMETA(DisplayName = "×"),
	Divide   UMETA(DisplayName = "÷"),
};
