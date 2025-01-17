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


	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	bool CanAddItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void AddItemInstance(UItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void RemoveItemInstance(UItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category = Inventory, BlueprintPure = false)
	TArray<UItemInstance*> GetAllItems() const;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int MaxCap;
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<UItemInstance*> InventoryList;




};
