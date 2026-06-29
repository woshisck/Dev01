#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Animation/AN_MeleeDamage.h"
#include "Component/CombatDeckComponent.h"
#include "Data/EnemyData.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "GA_MeleeAttack.generated.h"

class UMontageConfigDA;
class UMontageAttackDataAsset;
class UYogAnimNotifyState_Damage;
class UAbilityTask_ApplyRootMotionMoveToForce;
class UAbilitySystemComponent;
class AYogCharacterBase;


UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_MeleeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MeleeAttack();

	static bool TryQueueJustComboSpeedBonus(UAbilitySystemComponent* ASC);
	static bool TryConsumeJustComboBonus(UAbilitySystemComponent* ASC);


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	bool bRequireCommit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Deck")
	ECombatDeckFlowRole CombatDeckFlowRole = ECombatDeckFlowRole::Any;

	virtual FActionData GetAbilityActionData_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECardRequiredAction GetCombatDeckActionType() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECombatDeckFlowRole GetCombatDeckFlowRole() const { return CombatDeckFlowRole; }

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	bool IsCombatDeckComboFinisher() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Melee")
	bool HasConfiguredAttackData() const;

	const UMontageAttackDataAsset* GetConfiguredAttackData() const { return ActiveComboAttackData; }
	const FComboAttackConfig* GetConfiguredAttackConfig() const { return bActiveComboAttackConfigValid ? &ActiveComboAttackConfig : nullptr; }
	FGameplayTag GetConfiguredAttackEventTag(FGameplayTag FallbackTag) const;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	void SetNextActivationFromDashSave(bool bFromDashSave);

private:
	struct FMeleeHitAttrSnapshot
	{
		bool  bValid              = false;
		float PreCardAttack       = 0.f;
		float PreCardAttackPower  = 0.f;
	};

	/**
	 * 涓嶄娇鐢?UPROPERTY 鏄洜涓?AnimNotify 涓嶅弬涓?GC锛岀敓鍛藉懆鏈熼殢钂欏お濂囪祫浜с€?
	 */
	TObjectPtr<UAN_MeleeDamage> CachedDamageNotify;

	UPROPERTY()
	TObjectPtr<UMontageConfigDA> ActiveMontageConfig;

	UPROPERTY()
	TObjectPtr<UMontageAttackDataAsset> ActiveComboAttackData;

	UPROPERTY()
	FComboAttackConfig ActiveComboAttackConfig;

	UPROPERTY()
	TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> EnemyLungeTask;

	UPROPERTY()
	FEnemyAIAttackRuntimeContext ActiveEnemyAttackContext;

	/**
	 * StatAfterATK 浼樺厛鐢ㄦ鍊硷紙浠ｈ〃鏈€鍚庝竴鍑伙級锛屾湭鍛戒腑杩囨椂 fallback 鍒?CachedDamageNotify銆?
	 * EventData.OptionalObject 鏄?const UObject*锛屾晠鐢ㄥ師濮?const 鎸囬拡锛汚nimNotify 涓嶅弬涓?GC銆?
	 */
	const UAN_MeleeDamage* LastFiredDamageNotify = nullptr;
	const UYogAnimNotifyState_Damage* LastFiredDamageWindow = nullptr;

	bool bCombatDeckCardResolvedThisActivation = false;
	bool bCombatDeckFromDashSave = false;
	bool bNextActivationFromDashSave = false;
	bool bActiveComboAttackConfigValid = false;
	bool bComboContinued = true;
	bool bExitedComboState = false;
	int32 CombatDeckHitResolveCounter = 0;
	bool bHasStatBeforeAttributeSnapshot = false;
	float LocalPreStatBeforeAttack = 0.f;
	float LocalPreStatBeforeAttackPower = 0.f;
	float LocalStatBeforeAttackDelta = 0.f;
	float LocalStatBeforeAttackPowerDelta = 0.f;
	FDelegateHandle CanComboTagHandle;
	FDelegateHandle LegacyCanComboTagHandle;
	float AbilityActivationTime = 0.f;

	UPROPERTY()
	FCombatCardResolveResult ActiveCombatCardResult;

	FGuid ActiveAttackGuid;
	int32 ActiveComboIndex = 0;
	FGameplayTagContainer ActiveComboTags;

	bool bIsHandlingMeleeEvent = false;

	/** Finds the first AN_MeleeDamage notify on the selected montage. */
	static UAN_MeleeDamage* GetFirstDamageNotify(UAnimMontage* Montage);

	void TryStartEnemyRadialLunge();

	AActor* PreviewFirstCombatDeckHitTarget(const FGameplayEventData& EventData) const;
	void PrimeCombatDeckHitContext(const FGameplayEventData& EventData);

	void TryResolveCombatDeckOnHit();
	FCombatCardResolveResult ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming);

	UFUNCTION()
	void OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	FMeleeHitAttrSnapshot NormalizeAttrsPreCard(UAbilitySystemComponent* ASC);
	void NormalizeAttrsPostCard(UAbilitySystemComponent* ASC, const FMeleeHitAttrSnapshot& Snapshot);
	void ApplyHitStop(AYogCharacterBase* Owner, const TArray<AActor*>& HitActors);
	void ApplyHitReactions(AYogCharacterBase* Owner, const FYogGameplayEffectContainerSpec& ContainerSpec);
	void RestoreAttrsPostCard(UAbilitySystemComponent* ASC, const FCombatCardResolveResult& CardResult, const FMeleeHitAttrSnapshot& Snapshot);
};
