// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#include "Abilities/GSCAbilitySet.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GSCLog.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "Abilities/GSCAbilitySystemUtils.h"
#include "Components/GSCAbilityInputBindingComponent.h"
#include "Components/GSCCoreComponent.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "GSCAbilitySet"

bool UGSCAbilitySet::GrantToAbilitySystem(UAbilitySystemComponent* InASC, FGSCAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText, const bool bShouldRegisterCoreDelegates) const
{
	if (!IsValid(InASC))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC", "ASC is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	UGSCAbilitySystemComponent* ASC = Cast<UGSCAbilitySystemComponent>(InASC);
	if (!ASC)
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC_Type", "%s is not a UGSCAbilitySystemComponent");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	const bool bSuccess = FGSCAbilitySystemUtils::TryGrantAbilitySet(ASC, this, OutAbilitySetHandle);

	// Make sure to re-register delegates, this set may have added
	if (bSuccess && bShouldRegisterCoreDelegates)
	{
		TryRegisterCoreComponentDelegates(InASC);
	}
	
	return bSuccess;
}

bool UGSCAbilitySet::GrantToAbilitySystem(const AActor* InActor, FGSCAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText) const
{
	if (!IsValid(InActor))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_Actor", "Passed in actor is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("Invalid_Actor_ASC", "Unable to get valid ASC from actor {0}"), FText::FromString(GetNameSafe(InActor)));
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	return GrantToAbilitySystem(ASC, OutAbilitySetHandle, OutErrorText);
}

bool UGSCAbilitySet::RemoveFromAbilitySystem(UAbilitySystemComponent* InASC, FGSCAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText, const bool bShouldRegisterCoreDelegates)
{
	if (!IsValid(InASC))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_ASC", "ASC is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}
	
	// Make sure to notify we may have added attributes, this will shutdown all previously bound delegates to current attributes
	// Delegate registering will be called once again once we're done removing this set
	if (bShouldRegisterCoreDelegates)
	{
		TryUnregisterCoreComponentDelegates(InASC);
	}

	// Clear up abilities / bindings
	UGSCAbilityInputBindingComponent* InputComponent = nullptr;
	if (InASC->AbilityActorInfo.IsValid())
	{
		if (const AActor* AvatarActor = InASC->GetAvatarActor())
		{
			InputComponent = AvatarActor->FindComponentByClass<UGSCAbilityInputBindingComponent>();
		}
	}
	
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InAbilitySetHandle.Abilities)
	{
		if (!AbilitySpecHandle.IsValid())
		{
			continue;
		}

		// Failsafe cleanup of the input binding - even without clearing the input, this shouldn't cause issue as new binding
		// will always get a new InputID
		if (InputComponent)
		{
			InputComponent->ClearInputBinding(AbilitySpecHandle);
		}
		
		// Only Clear abilities on authority, on end if currently active, or right away
		if (InASC->IsOwnerActorAuthoritative())
		{
			InASC->SetRemoveAbilityOnEnd(AbilitySpecHandle);
		}
	}

	// Remove Effects
	for (const FActiveGameplayEffectHandle& EffectHandle : InAbilitySetHandle.EffectHandles)
	{
		if (EffectHandle.IsValid())
		{
			InASC->RemoveActiveGameplayEffect(EffectHandle);
		}
	}
	
	// Remove Attributes
	for (UAttributeSet* AttributeSet : InAbilitySetHandle.Attributes)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		InASC->RemoveSpawnedAttribute(AttributeSet);
#else
		InASC->GetSpawnedAttributes_Mutable().Remove(AttributeSet);
#endif
	}

	// Remove Owned Gameplay Tags
	if (InAbilitySetHandle.OwnedTags.IsValid())
	{
		// Remove tags (on server, replicated to all other clients - on owning client, for itself)
		FGSCAbilitySystemUtils::RemoveLooseGameplayTagsUnique(InASC, InAbilitySetHandle.OwnedTags);
	}

	// Clear any delegate handled bound previously for this actor
	if (UGSCAbilitySystemComponent* ASC = Cast<UGSCAbilitySystemComponent>(InASC))
	{
		// Clear any delegate handled bound previously for this actor
		for (FDelegateHandle InputBindingDelegateHandle : InAbilitySetHandle.InputBindingDelegateHandles)
		{
			ASC->OnGiveAbilityDelegate.Remove(InputBindingDelegateHandle);
			InputBindingDelegateHandle.Reset();
		}
	}

	// Make sure to re-register delegates, this set may have removed some but other sets may need delegate again
	if (bShouldRegisterCoreDelegates)
	{
		TryRegisterCoreComponentDelegates(InASC);
	}

	// Clears out the handle
	InAbilitySetHandle.Invalidate();
	return true;
}

bool UGSCAbilitySet::RemoveFromAbilitySystem(const AActor* InActor, FGSCAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText)
{
	if (!IsValid(InActor))
	{
		const FText ErrorMessage = LOCTEXT("Invalid_Actor", "Passed in actor is nullptr or invalid (pending kill)");
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("Invalid_Actor_ASC", "Unable to get valid ASC from actor {0}"), FText::FromString(GetNameSafe(InActor)));
		if (OutErrorText)
		{
			*OutErrorText = ErrorMessage;
		}

		GSC_PLOG(Error, TEXT("%s"), *ErrorMessage.ToString());
		return false;
	}

	return RemoveFromAbilitySystem(ASC, InAbilitySetHandle, OutErrorText);
}

bool UGSCAbilitySet::HasInputBinding() const
{
	for (const FGSCGameFeatureAbilityMapping& GrantedAbility : GrantedAbilities)
	{
		// Needs binding whenever one of the granted abilities has a non nullptr Input Action
		if (!GrantedAbility.InputAction.IsNull())
		{
			return true;
		}
	}
	
	return false;
}

void UGSCAbilitySet::TryRegisterCoreComponentDelegates(UAbilitySystemComponent* InASC)
{
	check(InASC);

	const AActor* AvatarActor = InASC->GetAvatarActor_Direct();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	// UGSCCoreComponent could be added to avatars
	if (UGSCCoreComponent* CoreComponent = AvatarActor->FindComponentByClass<UGSCCoreComponent>())
	{
		// Make sure to notify we may have added attributes (on server)
		CoreComponent->RegisterAbilitySystemDelegates(InASC);
	}
}

void UGSCAbilitySet::TryUnregisterCoreComponentDelegates(UAbilitySystemComponent* InASC)
{
	check(InASC);

	if (!InASC->AbilityActorInfo.IsValid())
	{
		return;
	}

	const AActor* AvatarActor = InASC->GetAvatarActor();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	// UGSCCoreComponent could be added to avatars
	if (UGSCCoreComponent* CoreComponent = AvatarActor->FindComponentByClass<UGSCCoreComponent>())
	{
		// Make sure to notify we may have removed attributes (on server)
		CoreComponent->ShutdownAbilitySystemDelegates(InASC);
	}
}

#undef LOCTEXT_NAMESPACE
