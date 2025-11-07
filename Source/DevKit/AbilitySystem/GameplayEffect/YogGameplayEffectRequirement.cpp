// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayEffect/YogGameplayEffectRequirement.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "GameplayEffect.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"
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
            /*UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(SourceASC);*/
            UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ASC);
            
            float target_resiliecne = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetResilienceAttribute());

            float source_act_resiliecne = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetResilienceAttribute());
            UE_LOG(LogTemp, Warning, TEXT("target_resiliecne: %f, source_act_resiliecne: %f"), target_resiliecne, source_act_resiliecne);
            // Check for tags on source and target
            //if (SourceASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.CanApply")) &&
            //    !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Blocked")))
            //{
            //    return true;
            //}
        }



    }

            // For example, check if the source has a certain attribute value and the target has another.
            //UAbilitySystemComponent* SourceASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
            //if (SourceASC && TargetASC)
            //{
            //    // Assuming you have an attribute set with a 'Health' attribute, you can get the current value.
            //    float SourceHealth = SourceASC->GetNumericAttribute(UMyAttributeSet::GetHealthAttribute());
            //    float TargetHealth = TargetASC->GetNumericAttribute(UMyAttributeSet::GetHealthAttribute());

            //    // Example condition: source health must be above 50 and target health below 75.
            //    if (SourceHealth > 50.0f && TargetHealth < 75.0f)
            //    {
            //        return true;
            //    }
            //}


    return false;
}
