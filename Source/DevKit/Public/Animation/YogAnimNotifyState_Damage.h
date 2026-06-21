// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogAnimNotifyState.h"
#include "Animation/AN_MeleeDamage.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Data/AbilityData.h"
#include "UObject/ObjectKey.h"
#include "YogAnimNotifyState_Damage.generated.h"

/**
 * Damage notify state version of AN_MeleeDamage.
 *
 * This is intentionally parallel to the existing one-frame notify so montages can
 * opt into a real active hit window later without changing the GAS damage path.
 */
class AActor;
class AYogCharacterBase;
class UAbilitySystemComponent;
class UMontageAttackDataAsset;
class URuneDataAsset;
class USkeletalMeshComponent;

UCLASS(meta = (DisplayName = "ANS Melee Damage Window"))
class DEVKIT_API UYogAnimNotifyState_Damage : public UYogAnimNotifyState
{
	GENERATED_BODY()
	
public:
	UYogAnimNotifyState_Damage();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual void Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<UMontageAttackDataAsset> AttackDataOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActRange = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActResilience = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDmgReduce = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	TArray<FYogHitboxType> HitboxTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (DisplayName = "Draw Debug Hitbox"))
	bool bDrawDebugHitbox = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
	bool bEvaluateOnBegin = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
	bool bEvaluateEveryTick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Window")
	bool bHitEachTargetOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	EHitStopMode HitStopMode = EHitStopMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Freeze", EditConditionHides, ClampMin = 0.0f))
	float HitStopFrozenDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.0f, ClampMax = 0.5f))
	float HitStopSlowDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.01f, ClampMax = 1.0f))
	float HitStopSlowRate = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 1.01f, ClampMax = 5.0f))
	float HitStopCatchUpRate = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects|HitImpact")
	FGameplayTag HitImpactCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<TObjectPtr<URuneDataAsset>> AdditionalRuneEffects;

	FActionData BuildActionData() const;
	FActionData ResolveActionData(AYogCharacterBase* Character) const;

	void FilterHitActorsForEvent(const AYogCharacterBase* TargetingCharacter,
		const FGameplayEventData& EventData, TArray<AActor*>& InOutActors) const;

	bool IsDamageWindowEvent(const FGameplayEventData& EventData) const;

private:
	struct FDamageWindowRuntime
	{
		int32 NextSampleId = 0;
		int32 LastFilteredSampleId = INDEX_NONE;
		TSet<TObjectKey<AActor>> HitActorKeys;
		TArray<TWeakObjectPtr<AActor>> LastFilteredActors;
		TWeakObjectPtr<UAbilitySystemComponent> AttackFrameASC;
		FActiveGameplayEffectHandle AttackFrameGEHandle;
	};

	mutable TMap<TObjectKey<USkeletalMeshComponent>, FDamageWindowRuntime> RuntimeByMesh;

	void DispatchDamageSample(USkeletalMeshComponent* MeshComp);
	FGameplayTag ResolveEventTag(AYogCharacterBase* Character) const;
	void SendSwingEvent(AYogCharacterBase* Character) const;
	void PopulatePendingHitData(AYogCharacterBase* Character) const;
	void ApplyAttackFrameGE(AYogCharacterBase* Character, UAbilitySystemComponent* ASC,
		const FActionData& ActionData, FDamageWindowRuntime& Runtime) const;
	void RemoveAttackFrameGE(FDamageWindowRuntime& Runtime) const;
};
