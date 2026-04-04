// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GameplayTagContainer.h"

#include "GameplayEffect.h"

#include "AbilitySystem/Abilities/YogGameplayAbility.h"

#include "YogBuffDefinition.generated.h"

class UFlowAsset;


USTRUCT(Blueprintable, BlueprintType)
struct FYogBuffDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UYogBuffDefinition> BuffDefinition;

};


UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UYogBuffDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:


	// ----------------------------------------------------------------------
	//	Properties
	// ----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = Duration)
	EGameplayEffectDurationType DurationPolicy;

	/** Duration in seconds. 0.0 for instantaneous effects; -1.0 for infinite duration. */
	UPROPERTY(EditDefaultsOnly, Category = Duration, meta = (EditCondition = "DurationPolicy == EGameplayEffectDurationType::HasDuration", EditConditionHides))
	FGameplayEffectModifierMagnitude DurationMagnitude;

	/** Period in seconds. 0.0 for non-periodic effects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duration|Period", meta = (EditCondition = "DurationPolicy != EGameplayEffectDurationType::Instant", EditConditionHides))
	FScalableFloat	Period;

	/** If true, the effect executes on application and then at every period interval. If false, no execution occurs until the first period elapses. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duration|Period", meta = (EditCondition = "true", EditConditionHides)) // EditCondition in FGameplayEffectDetails
		bool bExecutePeriodicEffectOnApplication;

	/** How we should respond when a periodic gameplay effect is no longer inhibited */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Duration|Period", meta = (EditCondition = "true", EditConditionHides)) // EditCondition in FGameplayEffectDetails
		EGameplayEffectPeriodInhibitionRemovedPolicy PeriodicInhibitionPolicy;

	/** Array of modifiers that will affect the target of this effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect, meta = (TitleProperty = Attribute))
	TArray<FGameplayModifierInfo> Modifiers;

	/** Array of executions that will affect the target of this effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect)
	TArray<FGameplayEffectExecutionDefinition> Executions;


	/** Effects to apply when a stacking effect "overflows" its stack count through another attempted application. Added whether the overflow application succeeds or not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking|Overflow", meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	TArray<TSubclassOf<UGameplayEffect>> OverflowEffects;

	/** If true, stacking attempts made while at the stack count will fail, resulting in the duration and context not being refreshed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking|Overflow", meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	bool bDenyOverflowApplication;

	/** If true, the entire stack of the effect will be cleared once it overflows */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking|Overflow", meta = (EditConditionHides, EditCondition = "(StackingType != EGameplayEffectStackingType::None) && bDenyOverflowApplication"))
	bool bClearStackOnOverflow;


	/** If true, cues will only trigger when GE modifiers succeed being applied (whether through modifiers or executions) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCues")
	bool bRequireModifierSuccessToTriggerCues;

	/** If true, GameplayCues will only be triggered for the first instance in a stacking GameplayEffect. */
	UPROPERTY(EditDefaultsOnly, Category = "GameplayCues")
	bool bSuppressStackingCues;

	/** Cues to trigger non-simulated reactions in response to this GameplayEffect such as sounds, particle effects, etc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCues")
	TArray<FGameplayEffectCue>	GameplayCues;


	// ----------------------------------------------------------------------
	//	Stacking
	// ----------------------------------------------------------------------
	/** How this GameplayEffect stacks with other instances of this same GameplayEffect */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingType	StackingType;

	/** Stack limit for StackingType */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking, meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	int32 StackLimitCount;

	/** Policy for how the effect duration should be refreshed while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking, meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy;

	/** Policy for how the effect period should be reset (or not) while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking, meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	EGameplayEffectStackingPeriodPolicy StackPeriodResetPolicy;

	/** Policy for how to handle duration expiring on this gameplay effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking, meta = (EditConditionHides, EditCondition = "StackingType != EGameplayEffectStackingType::None"))
	EGameplayEffectStackingExpirationPolicy StackExpirationPolicy;


	// ----------------------------------------------------------------------
	//	Ability Class
	// ----------------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UYogGameplayAbility> PassiveAbilityClass;

	// ----------------------------------------------------------------------
	//	BuffFlow 扩展字段
	// ----------------------------------------------------------------------

	/** 标识该 Buff 的 Tag（用于 EffectRegistry 查找，也可用于移除时精确定位） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffFlow")
	FGameplayTag BuffTag;

	/** 施加到目标 ASC 的 Tag（GE 激活期间持有，GE 移除时自动撤销） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffFlow")
	FGameplayTagContainer GrantedTagsToTarget;

	/** 关联的 BuffFlow Asset（符文激活时自动启动，符文卸下时自动停止） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffFlow")
	TObjectPtr<UFlowAsset> BuffFlowAsset;

	/**
	 * 运行时从 Definition 数据动态构建一个临时 GE 对象
	 * 供 BuffFlow 的 AddBuff 节点和其他运行时系统使用
	 */
	UGameplayEffect* CreateTransientGE(UObject* Outer) const;
};
