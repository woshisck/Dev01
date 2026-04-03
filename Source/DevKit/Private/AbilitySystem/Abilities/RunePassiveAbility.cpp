#include "AbilitySystem/Abilities/RunePassiveAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

void URunePassiveAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
}
