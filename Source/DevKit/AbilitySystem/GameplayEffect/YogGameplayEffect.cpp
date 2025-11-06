// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameplayEffect.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>
#include <DevKit/AbilitySystem/GameplayEffect/YogGameplayEffectRequirement.h>
#include <AbilitySystemBlueprintLibrary.h>


UYogGameplayEffect::UYogGameplayEffect()
{
    TSubclassOf<UGameplayEffectCustomApplicationRequirement> CustomReq = UYogGameplayEffectRequirement::StaticClass();
    //ApplicationRequirements.Add(CustomReq);
}

void UYogGameplayEffect::CreateOwnEffectContext(AActor* TargetActor)
{
    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

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

}
