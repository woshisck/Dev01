// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"
#include "Character/YogCharacterBase.h"
//#include "Data/CharacterData.h"
#include "Component/CharacterDataComponent.h"

UGA_PlayMontage::UGA_PlayMontage(const FObjectInitializer& ObjectInitializer)
{
    //InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    //bRetriggerInstancedAbility = true;
    //NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_PlayMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {

        return;
    }
   
    UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);

    AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
    FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());

    const FActionData* action_data = Owner->GetCharacterDataComponent()->GetCharacterData()->AbilityData->AbilityMap.Find(ability_tag);
    
    UAnimMontage* MontageToPlay = action_data ? action_data->Montage : nullptr;
    if(MontageToPlay)
    {

        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DynamicEffectClass, GetAbilityLevel(), Context);

        if (SpecHandle.IsValid())
        {
            FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

            // Override attributes using ActionData
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDamage")), action_data->ActDamage);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActRange")), action_data->ActRange);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActResilience")), action_data->ActResilience);
            Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDmgReduce")), action_data->ActDmgReduce);

            // Apply to self
            ActiveEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec));
        }

        UYogTask_PlayMontageAbility* PlayMontageTask = UYogTask_PlayMontageAbility::YogPlayMontageAbility(this, NAME_None, MontageToPlay, FGameplayTagContainer(), 1.0f, NAME_None);
        if (PlayMontageTask)
        {
            //PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayMontage::OnMontageBlendOut);
 
            PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayMontage::OnMontageBlendOut);
            PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayMontage::OnMontageCompleted);
            PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayMontage::OnMontageInterrupted);
            PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayMontage::OnMontageCancelled);
            PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);
            
            PlayMontageTask->ReadyForActivation();
            //PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);
        }   
    }
    else
    {
        //EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
    

//    if (action_data)
//    {
//        // create Dynamic GameplayEffect
//        UGameplayEffect* ActionEffect = NewObject<UGameplayEffect>(GetTransientPackage(),FName(TEXT("ActionEffect")));
//        ActionEffect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
//        ActionEffect->DurationMagnitude = FScalableFloat(0.0f); // forever longer

//        // modifier
//        FGameplayModifierInfo ModifierInfo;

//        // ATTACK
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDamage);
//        ModifierInfo.ModifierOp = EGameplayModOp::Additive;
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // ATTACK RAGE
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActRange);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetAttackRangeAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // RESILIENCE
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActResilience);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetResilienceAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // DMG TAKEN
//        ModifierInfo.ModifierMagnitude = FScalableFloat(action_data->ActDmgReduce);
//        ModifierInfo.Attribute = Owner->BaseAttributeSet->GetDmgTakenAttribute();
//        ActionEffect->Modifiers.Add(ModifierInfo);

//        // APPLY GameplayEffect
//        FGameplayEffectContextHandle EffectContext = ActorInfo->AbilitySystemComponent->MakeEffectContext();
//        EffectContext.AddSourceObject(this);

//        FActiveGameplayEffectHandle ActiveEffectHandle =
//            ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
//                ActionEffect, 1.0f, EffectContext);
//        UE_LOG(LogTemp, Warning, TEXT("ApplyGameplayEffectToSelf"));

//        // SAVE ActiveEffectHandle FOR LATER REMOVE
//        ActiveEffectHandles.Add(ActiveEffectHandle);

//        // DATA CACHE from player DEPRECATED
//        cache_action_data = MakeShared<FActionData>(*action_data);
//    }

}


void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{

    // remove related Gameplay Effects and gameplaytags(hardcode)
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
    //ActiveEffectHandles.Empty();
    GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UGA_PlayMontage::OnMontageCompleted()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
    ApplyEffectContainer(EventTag, EventData, -1);
}


