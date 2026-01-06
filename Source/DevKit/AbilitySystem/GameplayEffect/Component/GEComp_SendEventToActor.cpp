// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayEffect/Component/GEComp_SendEventToActor.h"
#include "AbilitySystemLog.h"
#include "Misc/DataValidation.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"


UGEComp_SendEventToActor::UGEComp_SendEventToActor()
{
}
/*
* This Function call the send event data to player AFTER gain ability in parent class (after super) to trigger the passive ability
* NB. The passive ability need to set the trigger tag and MUST DELETE Activate Ability Blueprint Node, this is protential bug 
*/
void UGEComp_SendEventToActor::OnInhibitionChanged(FActiveGameplayEffectHandle ActiveGEHandle, bool bIsInhibited) const
{
    Super::OnInhibitionChanged(ActiveGEHandle, bIsInhibited);

    // Get the Ability System Component
    UAbilitySystemComponent* ASC = ActiveGEHandle.GetOwningAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    // Get the active gameplay effect
    const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveGEHandle);
    if (!ActiveGE)
    {
        return;
    }

    // Get source (instigator) and target actors
    AActor* SourceActor = nullptr;
    AActor* TargetActor = nullptr;

    // Get source actor from the effect context
    FGameplayEffectContextHandle ContextHandle = ActiveGE->Spec.GetContext();
    if (ContextHandle.IsValid())
    {
        SourceActor = ContextHandle.GetInstigator();
    }

    // Get target actor from the ASC owner
    TargetActor = ASC->GetOwner();

    // Make sure we have valid actors
    if (!TargetActor)
    {
        return;
    }

    // Create and setup gameplay event data
    FGameplayEventData EventData;


    // Set basic event info
    EventData.Instigator = SourceActor;
    EventData.Target = TargetActor;
    EventData.OptionalObject = GetOwner(); // The GameplayEffect
    EventData.EventTag = Trigger_EventTag;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        TargetActor,
        EventData.EventTag,
        EventData
    );
    UE_LOG(LogTemp, Warning, TEXT("Sent event data"));

}






//AActor* Instigator = local_GeSpec.GetContext().GetInstigator();
//AActor* EffectCauser = local_GeSpec.GetContext().GetEffectCauser();
//AActor* OriginalInstigator = nullptr;

//if (const FGameplayEffectContext* Context = local_GeSpec.GetContext().Get())
//{
//    OriginalInstigator = Context->GetOriginalInstigator();
//}
//AActor* DirectTarget = ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetAvatarActor() : nullptr;

////FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Damage"));
//FGameplayEventData EventData;
//EventData.EventTag = Trigger_EventTag;
//EventData.EventMagnitude = 4.4444f;
//EventData.Instigator = Instigator;
//EventData.Target = DirectTarget;

//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
//    DirectTarget,
//    Trigger_EventTag,
//    EventData
//);

//UE_LOG(LogTemp, Log, TEXT("Instigator:  %s"), *Instigator->GetName());
//UE_LOG(LogTemp, Log, TEXT("DirectTarget:  %s"), *DirectTarget->GetName());

//return true;