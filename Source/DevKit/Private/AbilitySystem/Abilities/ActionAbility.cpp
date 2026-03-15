// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ActionAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"

UActionAbility::UActionAbility(const FObjectInitializer& ObjectInitializer)
{
    //InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    //bRetriggerInstancedAbility = true;
    //NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        return;
    }
    
    UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());

    if (!Owner->AbilityData && ability_tag.IsValid())
    {
        return;
    }

	FActionData* action_data = Owner->AbilityData->AbilityMap.Find(ability_tag);
	//cache_action_data = action_data;
	
    if (action_data)
    {
        // create Dynamic GameplayEffect
        UGameplayEffect* ActionEffect = NewObject<UGameplayEffect>(GetTransientPackage(),FName(TEXT("ActionEffect")));
        ActionEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
        //ActionEffect->DurationMagnitude = FScalableFloat(0.0f); // forever longer

        // modifier
        FGameplayModifierInfo ModifierInfo;

        // ATTACK
        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDamage);
        ModifierInfo.ModifierOp = EGameplayModOp::Additive;
        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackAttribute();
        ActionEffect->Modifiers.Add(ModifierInfo);

        // ATTACK RAGE
        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActRange);
        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackRangeAttribute();
        ActionEffect->Modifiers.Add(ModifierInfo);

        // RESILIENCE
        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActResilience);
        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetResilienceAttribute();
        ActionEffect->Modifiers.Add(ModifierInfo);

        // DMG TAKEN
        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDmgReduce);
        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetDmgTakenAttribute();
        ActionEffect->Modifiers.Add(ModifierInfo);

        // APPLY GameplayEffect
        FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        EffectContext.AddSourceObject(this);

        FActiveGameplayEffectHandle ActiveEffectHandle =
            ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
                ActionEffect, 1.0f, EffectContext);
        UE_LOG(LogTemp, Warning, TEXT("ApplyGameplayEffectToSelf"));

        // SAVE ActiveEffectHandle FOR LATER REMOVE
        ActiveEffectHandles.Add(ActiveEffectHandle);

        // DATA CACHE from player DEPRECATED
        cache_action_data = MakeShared<FActionData>(*action_data);
    }
    else
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        //EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}


void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

    // remove related Gameplay Effects
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        for (const FActiveGameplayEffectHandle& effect_Handle : ActiveEffectHandles)
        {
            if (effect_Handle.IsValid())
            {
                ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(effect_Handle);
            }
        }
    }

    ActiveEffectHandles.Empty();
    cache_action_data = nullptr;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

