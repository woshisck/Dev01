#include "AbilitySystem/Abilities/GA_WeaponSkill.h"

#include "GameplayTagContainer.h"


UGA_WeaponSkill::UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
}
