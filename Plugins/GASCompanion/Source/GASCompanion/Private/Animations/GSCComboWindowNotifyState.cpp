// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Animations/GSCComboWindowNotifyState.h"

#include "GSCLog.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Abilities/GSCGameplayAbility.h"
#include "Components/GSCComboManagerComponent.h"
#include "Components/GSCCoreComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UGSCComboWindowNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	const AActor* Owner = GetOwnerActor(MeshComp);
	if (!Owner)
	{
		return;
	}

	// run only on server
	if (!Owner->HasAuthority())
	{
		return;
	}

	UGSCComboManagerComponent* ComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(Owner);
	if (ComboManagerComponent)
	{
		ComboManagerComponent->bComboWindowOpened = true;
	}
}

void UGSCComboWindowNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	const AActor* Owner = GetOwnerActor(MeshComp);
	if (!Owner)
	{
		return;
	}

	// run only on server
	if (!Owner->HasAuthority())
	{
		return;
	}

	UGSCComboManagerComponent* ComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(Owner);
	if (ComboManagerComponent)
	{
		GSC_LOG(Verbose, TEXT("NotifyEnd: bNextComboAbilityActivated %s (%s)"), ComboManagerComponent->bNextComboAbilityActivated ? TEXT("true") : TEXT("false"), *Owner->GetName())
		GSC_LOG(Verbose, TEXT("NotifyEnd: bEndCombo %s (%s)"), bEndCombo ? TEXT("true") : TEXT("false"), *Owner->GetName())
		if (!ComboManagerComponent->bNextComboAbilityActivated || bEndCombo)
		{
			GSC_LOG(Verbose, TEXT("NotifyEnd: ResetCombo  (%s)"), *Owner->GetName())
			ComboManagerComponent->ResetCombo();
		}

		ComboManagerComponent->bComboWindowOpened = false;
		ComboManagerComponent->bRequestTriggerCombo = false;
		ComboManagerComponent->bShouldTriggerCombo = false;
		ComboManagerComponent->bNextComboAbilityActivated = false;
	}
}

void UGSCComboWindowNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	const AActor* Owner = GetOwnerActor(MeshComp);
	if (!Owner)
	{
		return;
	}

	// run only on server
	if (!Owner->HasAuthority())
	{
		return;
	}

	UGSCComboManagerComponent* ComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(Owner);
	if (!ComboManagerComponent)
	{
		return;
	}

	if (ComboManagerComponent->bComboWindowOpened && ComboManagerComponent->bShouldTriggerCombo && ComboManagerComponent->bRequestTriggerCombo && !bEndCombo)
	{
		UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Owner);
		// prevent reactivate of ability in this tick window (especially on networked environment with some lags)
		if (CoreComponent && !ComboManagerComponent->bNextComboAbilityActivated)
		{
			const UGameplayAbility* ComboAbility = ComboManagerComponent->GetCurrentActiveComboAbility();
			if (ComboAbility)
			{
				UGSCGameplayAbility* ActivatedAbility;
				const bool bSuccess = CoreComponent->ActivateAbilityByClass(ComboAbility->GetClass(), ActivatedAbility);
				if (bSuccess)
				{
					ComboManagerComponent->bNextComboAbilityActivated = true;
				}
				else
				{
					GSC_LOG(Verbose, TEXT("ComboWindowNotifyState:NotifyTick Ability %s didn't activate"), *ComboAbility->GetClass()->GetName())
				}
			}
		}
	}
}

FString UGSCComboWindowNotifyState::GetEditorComment()
{
	return TEXT("probably not yet implemented");
}

FString UGSCComboWindowNotifyState::GetNotifyName_Implementation() const
{
	return bEndCombo ? "GSC Combo Window (ending)     " : "GSC Combo Window    ";
}

AActor* UGSCComboWindowNotifyState::GetOwnerActor(USkeletalMeshComponent* MeshComponent) const
{
	AActor* OwnerActor = MeshComponent->GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	const FString ActorName = OwnerActor->GetName();
	if (ActorName.StartsWith(AnimationEditorPreviewActorString))
	{
		return nullptr;
	}

	return OwnerActor;
}
