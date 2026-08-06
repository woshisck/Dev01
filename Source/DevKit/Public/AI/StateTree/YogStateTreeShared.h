#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

namespace YogStateTree
{
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
