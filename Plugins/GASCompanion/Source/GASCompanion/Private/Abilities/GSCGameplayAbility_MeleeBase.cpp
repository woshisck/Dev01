// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Abilities/GSCGameplayAbility_MeleeBase.h"

#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Abilities/Tasks/GSCTask_PlayMontageWaitForEvent.h"
#include "Components/GSCComboManagerComponent.h"

UGSCGameplayAbility_MeleeBase::UGSCGameplayAbility_MeleeBase()
{
}

void UGSCGameplayAbility_MeleeBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(AvatarActor);
	if (!ComboManagerComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboManagerComponent->IncrementCombo();

	UAnimMontage* Montage = GetNextComboMontage();

	UGSCTask_PlayMontageWaitForEvent* Task = UGSCTask_PlayMontageWaitForEvent::PlayMontageAndWaitForEvent(this, NAME_None, Montage, WaitForEventTag, Rate, NAME_None, true, 1.0f);
	Task->OnBlendOut.AddDynamic(this, &UGSCGameplayAbility_MeleeBase::OnMontageCompleted);
	Task->OnCompleted.AddDynamic(this, &UGSCGameplayAbility_MeleeBase::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &UGSCGameplayAbility_MeleeBase::OnMontageCancelled);
	Task->OnCancelled.AddDynamic(this, &UGSCGameplayAbility_MeleeBase::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &UGSCGameplayAbility_MeleeBase::OnEventReceived);
	Task->ReadyForActivation();
}

void UGSCGameplayAbility_MeleeBase::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGSCGameplayAbility_MeleeBase::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGSCGameplayAbility_MeleeBase::OnEventReceived(const FGameplayTag EventTag, const FGameplayEventData EventData)
{
	ApplyEffectContainer(EventTag, EventData);
}

UAnimMontage* UGSCGameplayAbility_MeleeBase::GetNextComboMontage()
{
	if (!ComboManagerComponent)
	{
		return nullptr;
	}

	int32 ComboIndex = ComboManagerComponent->ComboIndex;

	if (ComboIndex >= Montages.Num())
	{
		ComboIndex = 0;
	}

	return Montages.IsValidIndex(ComboIndex) ? Montages[ComboIndex] : nullptr;
}
