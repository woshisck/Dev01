// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionAbility.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"
#include "DevKit/Character/YogCharacterBase.h"

UActionAbility::UActionAbility(const FObjectInitializer& ObjectInitializer)
{
}

void UActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);




	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());
	FActionData* action_data = Owner->AbilityData->AbilityMap.Find(ability_tag);
	//cache_action_data = action_data;
	
    if (action_data)
    {
        // create Dynamic GameplayEffect
        UGameplayEffect* ActionEffect = NewObject<UGameplayEffect>(GetTransientPackage(),
            FName(TEXT("ActionEffect")));
        ActionEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
        ActionEffect->DurationMagnitude = FScalableFloat(0.0f); // forever longer

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

        // SAVE ActiveEffectHandle FOR LATER REMOVE
        ActiveEffectHandles.Add(ActiveEffectHandle);

        // DATA CACHE from player DEPRECATED
        cache_action_data = MakeShared<FActionData>(*action_data);
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }



	//if (action_data)
	//{
	//
	//	cache_action_data = MakeShared<FActionData>(*action_data);

	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), action_data->ActDamage);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), action_data->ActRange);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), action_data->ActResilience);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), action_data->ActDmgReduce);

	//}
	//else
	//{
	//	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	//}

}


void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

    // 移除所有关联的Gameplay Effects
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
        for (const FActiveGameplayEffectHandle& Handle : ActiveEffectHandles)
        {
            if (Handle.IsValid())
            {
                ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
            }
        }
    }

    ActiveEffectHandles.Empty();
    cache_action_data = nullptr;

	//AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	//if (cache_action_data.IsValid())
	//{
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), -cache_action_data->ActDamage);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), -cache_action_data->ActRange);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), -cache_action_data->ActResilience);
	//	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), -cache_action_data->ActDmgReduce);
	//	cache_action_data = nullptr;
	//}

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

