#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "BFNode_BlueprintBase.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class URuneDataAsset;
class UYogAbilitySystemComponent;

USTRUCT(BlueprintType)
struct DEVKIT_API FBFBlueprintSetByCallerMagnitude
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetByCaller")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetByCaller")
	float Value = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SetByCaller")
	bool bUseCombatCardEffectMultiplier = false;
};

/**
 * Blueprintable base for FA Flow Node Blueprint modules.
 *
 * Use this as the parent class for reusable FA modules such as attack payloads,
 * status application, finisher payloads, or moonlight link payloads. It exposes
 * the BuffFlow runtime context to Blueprint without making each BP node rediscover
 * ASC, targets, source rune data, and combat-card link state.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "FA Flow Node Blueprint Base", Category = "BuffFlow|Blueprint"))
class DEVKIT_API UBFNode_BlueprintBase : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "BuffFlow|Context", DisplayName = "Get BuffFlow Component")
	UBuffFlowComponent* BP_GetBuffFlowComponent() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Context", DisplayName = "Get Buff Owner")
	AYogCharacterBase* BP_GetBuffOwner() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Context", DisplayName = "Get Owner ASC")
	UYogAbilitySystemComponent* BP_GetOwnerASC() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Context", DisplayName = "Resolve Target")
	AActor* BP_ResolveTarget(EBFTargetSelector Selector) const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Context", DisplayName = "Get ASC From Actor")
	UAbilitySystemComponent* BP_GetASCFromActor(AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Rune", DisplayName = "Get Source Rune")
	URuneDataAsset* BP_GetSourceRune() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Rune", DisplayName = "Get Rune Tuning Value")
	float BP_GetRuneTuningValue(FName Key, float DefaultValue = 0.f) const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Combat Card", DisplayName = "Has Combat Card Context")
	bool BP_HasCombatCardContext() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Combat Card", DisplayName = "Get Combat Card Context")
	bool BP_GetCombatCardContext(FCombatCardEffectContext& OutContext) const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Combat Card", DisplayName = "Get Combat Card Effect Multiplier")
	float BP_GetCombatCardEffectMultiplier(float DefaultValue = 1.f) const;

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Effect", DisplayName = "Apply Gameplay Effect To Target")
	bool BP_ApplyGameplayEffectToTarget(
		TSubclassOf<UGameplayEffect> EffectClass,
		EBFTargetSelector Target,
		float Level,
		int32 ApplicationCount,
		const TArray<FBFBlueprintSetByCallerMagnitude>& SetByCallerMagnitudes,
		FActiveGameplayEffectHandle& OutHandle);

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Event", DisplayName = "Send Gameplay Event To Target")
	bool BP_SendGameplayEventToTarget(
		FGameplayTag EventTag,
		EBFTargetSelector EventReceiver,
		EBFTargetSelector PayloadTarget,
		EBFTargetSelector Instigator,
		float EventMagnitude);

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Trace", DisplayName = "Record BuffFlow Trace")
	void BP_RecordTrace(EBuffFlowTraceResult Result, AActor* TargetActor, const FString& Message, const FString& Values);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", DisplayName = "Trigger Out")
	void BP_TriggerOut(bool bFinish = false);

	UFUNCTION(BlueprintCallable, Category = "FlowNode", DisplayName = "Trigger Failed")
	void BP_TriggerFailed(bool bFinish = true);
};
