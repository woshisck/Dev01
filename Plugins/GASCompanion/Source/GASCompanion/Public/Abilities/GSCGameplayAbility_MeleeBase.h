// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GSCGameplayAbility.h"
#include "GSCGameplayAbility_MeleeBase.generated.h"

class UGSCComboManagerComponent;
/**
 *
 */
UCLASS()
class GASCOMPANION_API UGSCGameplayAbility_MeleeBase : public UGSCGameplayAbility
{
	GENERATED_BODY()


public:
	UGSCGameplayAbility_MeleeBase();

protected:
	UPROPERTY()
	TObjectPtr<UGSCComboManagerComponent> ComboManagerComponent;

	/** List of animation montages you want to cycle through when activating this ability */
	UPROPERTY(EditDefaultsOnly, Category="Montages")
	TArray<TObjectPtr<UAnimMontage>> Montages;

	/** Change to play the montage faster or slower */
	UPROPERTY(EditDefaultsOnly, Category="Montages")
	float Rate = 1.f;

	/** Any gameplay events matching this tag will activate the OnEventReceived callback and apply the gameplay effect containers for this ability */
	UPROPERTY(EditDefaultsOnly, Category="Montages")
	FGameplayTagContainer WaitForEventTag;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION(BlueprintPure, Category="GAS Companion|Ability|Melee")
	UAnimMontage* GetNextComboMontage();
};
