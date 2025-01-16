#pragma once

#include "Components/ActorComponent.h"

#include "InventoryManagerComponent.generated.h"


/**
 * Manages an inventory
 */
UCLASS(BlueprintType)
class DEVKIT_API UInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};
