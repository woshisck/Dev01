#pragma once

#include "CoreMinimal.h"
#include "CombatDeckEditTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatDeckEditCardLinkHintState : uint8
{
	None UMETA(DisplayName = "None"),
	ForwardLink UMETA(DisplayName = "Forward Link"),
	ForwardTarget UMETA(DisplayName = "Forward Target"),
	ReversedLink UMETA(DisplayName = "Reversed Link"),
	ReversedTarget UMETA(DisplayName = "Reversed Target"),
};
