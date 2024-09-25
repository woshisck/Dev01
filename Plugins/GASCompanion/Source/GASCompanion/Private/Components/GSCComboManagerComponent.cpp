// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Components/GSCComboManagerComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Abilities/GSCGameplayAbility_MeleeBase.h"
#include "Components/GSCCoreComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GSCLog.h"

UGSCComboManagerComponent::UGSCComboManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);


	MeleeBaseAbility = UGSCGameplayAbility_MeleeBase::StaticClass();
}

void UGSCComboManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGSCComboManagerComponent, ComboIndex, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCComboManagerComponent, bComboWindowOpened, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCComboManagerComponent, bShouldTriggerCombo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCComboManagerComponent, bRequestTriggerCombo, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGSCComboManagerComponent, bNextComboAbilityActivated, COND_None, REPNOTIFY_Always);
}

void UGSCComboManagerComponent::IncrementCombo()
{
	if (bComboWindowOpened)
	{
		ComboIndex = ComboIndex + 1;
	}
}

void UGSCComboManagerComponent::ResetCombo()
{
	GSC_LOG(Verbose, TEXT("UGSCComboManagerComponent::ResetCombo()"))
	SetComboIndex(0);
}

void UGSCComboManagerComponent::ActivateComboAbility(const TSubclassOf<UGSCGameplayAbility> AbilityClass, const bool bAllowRemoteActivation)
{
	if (IsOwnerActorAuthoritative())
	{
		ActivateComboAbilityInternal(AbilityClass, bAllowRemoteActivation);
	}
	else
	{
		ServerActivateComboAbility(AbilityClass, bAllowRemoteActivation);
	}
}

void UGSCComboManagerComponent::SetComboIndex(const int32 InComboIndex)
{
	if (IsOwnerActorAuthoritative())
	{
		ComboIndex = InComboIndex;
	}
	else
	{
		ComboIndex = InComboIndex;
		ServerSetComboIndex(InComboIndex);
	}
}

bool UGSCComboManagerComponent::IsOwnerActorAuthoritative() const
{
	return !bCachedIsNetSimulated;
}

// Called when the game starts
void UGSCComboManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Setup Owner references
	SetupOwner();

	// Cache net role here as well since for map-placed actors on clients, the Role may not be set correctly yet in OnRegister.
	CacheIsNetSimulated();
}

void UGSCComboManagerComponent::OnRegister()
{
	Super::OnRegister();

	// Cached off netrole to avoid constant checking on owning actor
	CacheIsNetSimulated();
}

void UGSCComboManagerComponent::SetupOwner()
{
	if(!GetOwner())
	{
		return;
	}

	OwningCharacter = Cast<ACharacter>(GetOwner());
	if(!OwningCharacter)
	{
		return;
	}

	OwnerCoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(OwningCharacter);
}

UGameplayAbility* UGSCComboManagerComponent::GetCurrentActiveComboAbility() const
{
	if (!OwnerCoreComponent)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent::GetCurrentActiveComboAbility() CompanionCoreComponent is not valid"))
		return nullptr;
	}

	TArray<UGameplayAbility*> Abilities = OwnerCoreComponent->GetActiveAbilitiesByClass(MeleeBaseAbility);
	return Abilities.IsValidIndex(0) ? Abilities[0] : nullptr;
}

void UGSCComboManagerComponent::ActivateComboAbilityInternal(const TSubclassOf<UGSCGameplayAbility> AbilityClass, const bool bAllowRemoteActivation)
{
	bShouldTriggerCombo = false;
	if (!OwningCharacter)
	{
		GSC_LOG(Error, TEXT("UGSCComboManagerComponent::ActivateComboAbility() OwningCharacter is null"))
		return;
	}

	if (!OwnerCoreComponent)
	{
		GSC_LOG(
			Error,
			TEXT("UGSCComboManagerComponent::ActivateComboAbility() OwningCoreComponent is null. GSCCoreComponent is required for GSCComboManagerComponent to work properly.\n\nPlease make sure it is added to `%s` in Blueprint Components list."),
			*GetNameSafe(GetOwner())
		)
		return;
	}

	if (!AbilityClass)
	{
		GSC_LOG(Error, TEXT("UGSCComboManagerComponent::ActivateComboAbility() provided AbilityClass is null"))
		return;
	}

	if (OwnerCoreComponent->IsUsingAbilityByClass(AbilityClass))
	{
		GSC_LOG(
			Verbose,
			TEXT("UGSCComboManagerComponent::ActivateComboAbility() %s is using %s already, update should trigger combo to %s"),
			*GetName(),
			*AbilityClass->GetName(),
			bComboWindowOpened ? TEXT("true") : TEXT("false")
		)
		bShouldTriggerCombo = bComboWindowOpened;
	}
	else
	{
		GSC_LOG(Verbose, TEXT("UGSCComboManagerComponent::ActivateComboAbility() %s is not in combo, activate %s"), *GetName(), *AbilityClass->GetName())
	    UGSCGameplayAbility* TempActivateAbility;
        OwnerCoreComponent->ActivateAbilityByClass(AbilityClass, TempActivateAbility, bAllowRemoteActivation);
    }
}

void UGSCComboManagerComponent::CacheIsNetSimulated()
{
	bCachedIsNetSimulated = IsNetSimulating();
}

void UGSCComboManagerComponent::ServerSetComboIndex_Implementation(const int32 InComboIndex)
{
	MulticastSetComboIndex(InComboIndex);
}

void UGSCComboManagerComponent::MulticastSetComboIndex_Implementation(const int32 InComboIndex)
{
	if (OwningCharacter && !OwningCharacter->IsLocallyControlled())
	{
		ComboIndex = InComboIndex;
	}
}

void UGSCComboManagerComponent::MulticastActivateComboAbility_Implementation(const TSubclassOf<UGSCGameplayAbility> AbilityClass, const bool bAllowRemoteActivation)
{
    if (OwningCharacter && !OwningCharacter->IsLocallyControlled())
    {
        ActivateComboAbilityInternal(AbilityClass, bAllowRemoteActivation);
    }
}

void UGSCComboManagerComponent::ServerActivateComboAbility_Implementation(const TSubclassOf<UGSCGameplayAbility> AbilityClass, const bool bAllowRemoteActivation)
{
    MulticastActivateComboAbility(AbilityClass, bAllowRemoteActivation);
}
