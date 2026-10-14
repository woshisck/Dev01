#pragma once

#include "Abilities/GameplayAbility.h"
#include "ActionRuleTestAbilities.generated.h"

// Editor-module fixtures only: never granted by project data, never shipped in the game.
UCLASS(Transient, NotBlueprintable)
class UActionRuleTestAttack : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UActionRuleTestAttack();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override {}
};

UCLASS(Transient, NotBlueprintable)
class UActionRuleTestDash : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UActionRuleTestDash();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override {}
};

UCLASS(Transient, NotBlueprintable)
class UActionRuleTestPerExecutionDash : public UActionRuleTestDash
{
	GENERATED_BODY()
public:
	UActionRuleTestPerExecutionDash();
};

UCLASS(Transient, NotBlueprintable)
class UActionRuleTestDeniedDash : public UActionRuleTestDash
{
	GENERATED_BODY()
public:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }
};
