#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Melee   UMETA(DisplayName = "近战"),
    Ranged  UMETA(DisplayName = "远程"),
};
