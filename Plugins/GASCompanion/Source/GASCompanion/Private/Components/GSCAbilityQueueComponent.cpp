// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Components/GSCAbilityQueueComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GSCDelegates.h"
#include "GSCLog.h"
#include "Abilities/GSCGameplayAbility.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UGSCAbilityQueueComponent::UGSCAbilityQueueComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGSCAbilityQueueComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	SetupOwner();
}

void UGSCAbilityQueueComponent::SetupOwner()
{
	if(!GetOwner())
	{
		return;
	}

	OwnerPawn = Cast<APawn>(GetOwner());
	if(!OwnerPawn)
	{
		return;
	}

	OwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
}

void UGSCAbilityQueueComponent::OpenAbilityQueue()
{
	if (!bAbilityQueueEnabled)
	{
		return;
	}

	bAbilityQueueOpened = true;
}

void UGSCAbilityQueueComponent::CloseAbilityQueue()
{
	if (!bAbilityQueueEnabled)
	{
		return;
	}

	if (OwnerPawn)
	{
		GSC_LOG(Verbose, TEXT("UGSCAbilityQueueComponent:CloseAbilityQueue() Closing Ability Queue for %s"), *OwnerPawn->GetName())
	}

	bAbilityQueueOpened = false;
}

void UGSCAbilityQueueComponent::UpdateAllowedAbilitiesForAbilityQueue(TArray<TSubclassOf<UGameplayAbility>> AllowedAbilities)
{
	if (!bAbilityQueueEnabled)
	{
		return;
	}

	QueuedAllowedAbilities = AllowedAbilities;

	// Notify Debug Widget if any is on screen
	UpdateDebugWidgetAllowedAbilities();
}

void UGSCAbilityQueueComponent::SetAllowAllAbilitiesForAbilityQueue(const bool bAllowAllAbilities)
{
	if (!bAbilityQueueEnabled)
	{
		return;
	}

	bAllowAllAbilitiesForAbilityQueue = bAllowAllAbilities;

	UpdateDebugWidgetAllowedAbilities();
}

bool UGSCAbilityQueueComponent::IsAbilityQueueOpened() const
{
    return bAbilityQueueOpened;
}

bool UGSCAbilityQueueComponent::IsAllAbilitiesAllowedForAbilityQueue() const
{
    return bAllowAllAbilitiesForAbilityQueue;
}

const UGameplayAbility* UGSCAbilityQueueComponent::GetCurrentQueuedAbility() const
{
	return QueuedAbility;
}

TArray<TSubclassOf<UGameplayAbility>> UGSCAbilityQueueComponent::GetQueuedAllowedAbilities() const
{
    return QueuedAllowedAbilities;
}

void UGSCAbilityQueueComponent::OnAbilityEnded(const UGameplayAbility* InAbility)
{
	const UGSCGameplayAbility* CompanionAbility = Cast<UGSCGameplayAbility>(InAbility);
	if (CompanionAbility)
	{
		GSC_LOG(
			Verbose,
			TEXT("UGSCAbilityQueueComponent::OnAbilityEnded() %s, ability queue enabled: %d, component queue enabled %d"),
			*CompanionAbility->GetName(),
			CompanionAbility->bEnableAbilityQueue ? 1 : -1,
			bAbilityQueueEnabled ? 1 : -1
		)

		if (CompanionAbility->bEnableAbilityQueue)
		{
			// Make sure to close the ability queue for abilities that have it enabled
			CloseAbilityQueue();
		}
	}
	else
	{
		GSC_LOG(
			Verbose,
			TEXT("UGSCAbilityQueueComponent::OnAbilityEnded() %s (not GSCGameplayAbility), component queue enabled %d"),
			*InAbility->GetName(),
			bAbilityQueueEnabled ? 1 : -1
		)

		// Close ability queue no matter what for non GSCGameplayAbilities (most likely using anim notify state to open / close queue)
		// CloseAbilityQueue();
	}

	if (bAbilityQueueEnabled)
	{
		if (QueuedAbility)
		{
			// Store queue ability in a local var, it is cleared in ResetAbilityQueueState
			const UGameplayAbility* AbilityToActivate = QueuedAbility;
			GSC_LOG(Log, TEXT("UGSCAbilityQueueComponent::OnAbilityEnded() has a queued input: %s [AbilityQueueSystem]"), *AbilityToActivate->GetName())
			if (bAllowAllAbilitiesForAbilityQueue || QueuedAllowedAbilities.Contains(AbilityToActivate->GetClass()))
			{
				ResetAbilityQueueState();

				GSC_LOG(Log, TEXT("UGSCAbilityQueueComponent::OnAbilityEnded() %s is within Allowed Abilties, try activate [AbilityQueueSystem]"), *AbilityToActivate->GetName())
				if (OwnerAbilitySystemComponent)
				{
					OwnerAbilitySystemComponent->TryActivateAbilityByClass(AbilityToActivate->GetClass());
				}
			}
			else
			{
				ResetAbilityQueueState();
				GSC_LOG(Verbose, TEXT("UGSCAbilityQueueComponent::OnAbilityEnded() not allowed ability, do nothing: %s [AbilityQueueSystem]"), *AbilityToActivate->GetName())
			}
		}
		else
		{
			ResetAbilityQueueState();
		}
	}
}

void UGSCAbilityQueueComponent::OnAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& ReasonTags)
{
	GSC_LOG(Verbose, TEXT("UGSCAbilityQueueComponent::OnAbilityFailed() %s, Reason: %s"), *Ability->GetName(), *ReasonTags.ToStringSimple())
	if (bAbilityQueueEnabled && bAbilityQueueOpened)
	{
		GSC_LOG(Verbose, TEXT("UGSCAbilityQueueComponent::OnAbilityFailed() Set QueuedAbility to %s"), *Ability->GetName())

		// Only queue the ability if it's allowed (or AllowAllAbilities is turned on)
		if (bAllowAllAbilitiesForAbilityQueue || QueuedAllowedAbilities.Contains(Ability->GetClass()))
		{
			QueuedAbility = TObjectPtr<UGameplayAbility>(const_cast<UGameplayAbility*>(Ability));
		}
	}
}

void UGSCAbilityQueueComponent::ResetAbilityQueueState()
{
	GSC_LOG(Verbose, TEXT("UGSCAbilityQueueComponent::ResetAbilityQueueState()"))
	QueuedAbility = nullptr;
	bAllowAllAbilitiesForAbilityQueue = false;
	QueuedAllowedAbilities.Empty();

	// Notify Debug Widget if any is on screen
	UpdateDebugWidgetAllowedAbilities();
}

void UGSCAbilityQueueComponent::UpdateDebugWidgetAllowedAbilities()
{
	FGSCDelegates::OnUpdateAllowedAbilities.Broadcast(QueuedAllowedAbilities);
}
