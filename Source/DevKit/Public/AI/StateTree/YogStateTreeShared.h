#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "GameFramework/Controller.h"
#include "YogStateTreeShared.generated.h"

// Where a range-gated StateTree node reads its threshold from.
UENUM()
enum class EYogDistanceSource : uint8
{
	/** Use the distance authored on the node. */
	FixedDistance,
	/**
	 * Use the owner's current AttackRange attribute so range buffs and weapon
	 * swaps move the gate without re-authoring every tree. Falls back to the
	 * authored distance when the owner has no ASC or no AttackRange attribute.
	 */
	AttackRangeAttribute,
};

namespace YogStateTree
{
	inline const UAbilitySystemComponent* ResolveASC(const AActor* Actor)
	{
		const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Actor);
		return ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	}

	// Under StateTreeComponentSchema the Actor context resolves to the AIController,
	// whose own location never follows its pawn. Step down to the pawn so range
	// measurements are taken from the body rather than the controller's spawn point.
	inline const AActor* ResolveSelfBody(const AActor* Actor)
	{
		if (const AController* AsController = Cast<AController>(Actor))
		{
			return AsController->GetPawn();
		}
		return Actor;
	}

	// Unset when the actor has no ASC or no AttackRange attribute, which callers must
	// treat as "fall back to the authored distance" rather than as a range of zero.
	inline TOptional<float> ResolveAttackRange(const AActor* Self)
	{
		const UAbilitySystemComponent* ASC = ResolveASC(Self);
		if (!ASC || !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetAttackRangeAttribute()))
		{
			return TOptional<float>();
		}

		return ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackRangeAttribute());
	}

	inline float ResolveDistanceThreshold(EYogDistanceSource Source, float AuthoredDistance, const AActor* Self)
	{
		if (Source != EYogDistanceSource::AttackRangeAttribute)
		{
			return AuthoredDistance;
		}

		return ResolveAttackRange(Self).Get(AuthoredDistance);
	}

	// Returns the owner's current Health / MaxHealth in [0,1]. Falls back to 1.0
	// when the ASC lacks the attributes or MaxHealth is degenerate, so callers can
	// treat "no data" as full HP rather than accidentally tripping low-HP gates.
	inline float ResolveHealthPercent(const UAbilitySystemComponent* ASC)
	{
		if (!ASC
			|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute())
			|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
		{
			return 1.0f;
		}

		const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth <= KINDA_SMALL_NUMBER)
		{
			return 1.0f;
		}

		const float Health = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
	}
}
