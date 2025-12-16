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

bool UGEComp_SendEventToActor::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	if (ActiveGEContainer.IsNetAuthority())
	{

        //UGameplayEffect* source_gameplayeffect = GetOwner();

        //source_gameplayeffect->


        //FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Damage"));
        //FGameplayEventData EventData;
        //EventData.EventTag = EventTag;
        //EventData.EventMagnitude = 4.4444f;
        //EventData.Instigator = GetInstigatorActor();
        //EventData.Target = TargetActor;

        //// Send the event
        //UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        //    TargetActor,
        //    EventTag,
        //    EventData
        //);


		//UAbilitySystemBlueprintLibrary::SendGameplayEventToActor();
        return true;
	}

	return false;
}

void UGEComp_SendEventToActor::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
    UE_LOG(LogTemp, Log, TEXT("Effect %s applied at level %f"),*GetNameSafe(GESpec.Def),GESpec.GetLevel());


    AActor* Instigator = GESpec.GetContext().GetInstigator();
    AActor* EffectCauser = GESpec.GetContext().GetEffectCauser();
    AActor* OriginalInstigator = nullptr;
    if (const FGameplayEffectContext* Context = GESpec.GetContext().Get())
    {
        OriginalInstigator = Context->GetOriginalInstigator();
    }

    // Target: From the container that's receiving the effect
    AActor* DirectTarget = ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetAvatarActor() : nullptr;


    //ASAP!! : 12-16-2025
    FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Damage"));
    FGameplayEventData EventData;
    EventData.EventTag = EventTag;
    EventData.EventMagnitude = 4.4444f;
    EventData.Instigator = Instigator;
    EventData.Target = DirectTarget;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        DirectTarget,
        EventTag,
        EventData
    );

    UE_LOG(LogTemp, Log, TEXT("Instigator:  %s"), *Instigator->GetName());
    UE_LOG(LogTemp, Log, TEXT("DirectTarget:  %s"), *DirectTarget->GetName());

}


void UGEComp_SendEventToActor::SendEventDataToActor()
{
}

EDataValidationResult UGEComp_SendEventToActor::IsDataValid(FDataValidationContext& Context) const
{
	return EDataValidationResult();
}
