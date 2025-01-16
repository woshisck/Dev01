#pragma once

#include "Components/ActorComponent.h"
#include "../Item/ItemInstance.h"


#include "InventoryManagerComponent.generated.h"

class UItemInstance;
/**
 * Manages an inventory
 */
UCLASS(BlueprintType)
class DEVKIT_API UInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UItemInstance*> InventoryList;
};
