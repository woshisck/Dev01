#pragma once

#include "CoreMinimal.h"

class UWeaponDefinition;

namespace WeaponComboTextUtils
{
	DEVKIT_API FText BuildComboHintText(
		const UWeaponDefinition* WeaponDefinition,
		int32 MaxLines = 0,
		bool bCompactSpacing = false);
}
