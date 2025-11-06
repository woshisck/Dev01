// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayEffect/YogGameplayEffectRequirement.h"

#include "GameplayEffect.h"

#include "AbilitySystemComponent.h"

bool UYogGameplayEffectRequirement::CanApplyGameplayEffect_Implementation(const UGameplayEffect* GameplayEffect, const FGameplayEffectSpec& Spec, UAbilitySystemComponent* ASC) const
{
    // The source is the owner of the effect spec. We can get the source actor from the spec.
    AActor* SourceActor = Spec.GetEffectContext().GetInstigator();
    AActor* TargetActor = ASC->GetAvatarActor();

    // If we have both source and target actors, we can check variables on them.
    if (SourceActor && TargetActor)
    {
        // Example: Check a variable on the source actor (assuming it has a specific component or interface)
        // Let's assume we are checking a boolean variable named `bCanApplyEffect` on the source.
        // You might use an interface or a component to access custom variables.

        // Example for source: Check if the source has an attribute set or a component with a variable.
        // For demonstration, let's assume we have an interface `ICombatInterface` with a function `CanApplyEffect`.
        // Similarly for the target.

        // If you are using an interface, you can do:
        // ICombatInterface* SourceCombat = Cast<ICombatInterface>(SourceActor);
        // ICombatInterface* TargetCombat = Cast<ICombatInterface>(TargetActor);
        // if (SourceCombat && TargetCombat)
        // {
        //     return SourceCombat->CanApplyEffect() && TargetCombat->CanReceiveEffect();
        // }

        // Alternatively, if you are using components, you might get the component and check the variable.

        // For this example, let's assume we are checking a simple condition: 
        // Source must have a tag and target must not have a tag.

        // Example: Check if source has a tag "CanApply" and target does not have "Blocked"
        UAbilitySystemComponent* SourceASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
        if (SourceASC && ASC)
        {
            // Check for tags on source and target
            if (SourceASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.CanApply")) &&
                !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Blocked")))
            {
                return true;
            }
        }
    }

    return false;
}
