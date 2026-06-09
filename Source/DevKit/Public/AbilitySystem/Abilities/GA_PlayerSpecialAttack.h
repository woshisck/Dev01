#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Component/CombatDeckComponent.h"
#include "Data/SpecialAttackDataAsset.h"
#include "GameplayTagContainer.h"
#include "GA_PlayerSpecialAttack.generated.h"

class APlayerCharacterBase;
class UAnimMontage;
class UPlayerSpecialAttackComponent;
class UYogAbilityTask_PlayMontageAndWaitForEvent;

UCLASS()
class DEVKIT_API UGA_PlayerSpecialAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlayerSpecialAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UFUNCTION(BlueprintPure, Category = "Special Attack")
	FSpecialAttackConfig GetActiveSpecialAttackConfig() const { return ActiveConfig; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Special Attack", DisplayName = "On Special Attack Event")
	void K2_OnSpecialAttackEvent(FGameplayTag EventTag, const FGameplayEventData& EventData);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	TObjectPtr<APlayerCharacterBase> PlayerOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	TObjectPtr<UPlayerSpecialAttackComponent> SpecialAttackComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	FSpecialAttackConfig ActiveConfig;

	UPROPERTY(BlueprintReadOnly, Category = "Special Attack")
	TObjectPtr<UAnimMontage> ActiveMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Special Attack")
	FGameplayTagContainer EventTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Special Attack")
	bool bApplyDamageFromMontageEvents = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Special Attack")
	bool bEndWhenMontageBlendsOut = true;

private:
	UPROPERTY()
	TObjectPtr<UYogAbilityTask_PlayMontageAndWaitForEvent> ActiveMontageTask = nullptr;

	FDelegateHandle CanComboTagHandle;
	float AbilityActivationTime = 0.0f;
	bool bAddedSpecialAttackLooseTag = false;
	bool bIsHandlingSpecialAttackEvent = false;
	bool bHasActiveCombatDeckContext = false;
	bool bCombatDeckCardResolvedThisActivation = false;

	UPROPERTY()
	FCombatDeckActionContext ActiveCombatDeckContext;

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

	void CaptureCombatDeckContext();
	void PrimeCombatDeckHitContext(const TArray<AActor*>& HitActors) const;
	FCombatCardResolveResult ResolveCombatDeckOnHit();
	void ApplyDamageFromEvent(FGameplayTag EventTag, const FGameplayEventData& EventData);
};
