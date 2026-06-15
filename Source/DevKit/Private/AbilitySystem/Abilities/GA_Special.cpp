#include "AbilitySystem/Abilities/GA_Special.h"

#include "GameplayTagContainer.h"

UGA_Special::UGA_Special(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo1")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo1")));
}
