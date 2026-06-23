#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Data/SpecialAttackDataAsset.h"
#include "GameplayTagContainer.h"
#include "GA_PlayerSpecialAttack.generated.h"

// Deprecated compatibility ability. Active skill data now grants regular gameplay abilities directly.
UCLASS(DisplayName = "Deprecated Player Special Attack Ability")
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

	UFUNCTION(BlueprintPure, Category = "Deprecated Special Attack")
	FSpecialAttackConfig GetActiveSpecialAttackConfig() const { return FSpecialAttackConfig(); }

	UFUNCTION(BlueprintImplementableEvent, Category = "Deprecated Special Attack", DisplayName = "On Special Attack Event")
	void K2_OnSpecialAttackEvent(FGameplayTag EventTag, const FGameplayEventData& EventData);
};
