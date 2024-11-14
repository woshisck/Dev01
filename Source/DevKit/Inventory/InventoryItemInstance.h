
#pragma once

#include "GameplayTagContainer.h"
#include "CoreMinimal.h"

#include "Templates/SubclassOf.h"
#include "InventoryItemDefinition.h"

#include "InventoryItemInstance.generated.h"




UCLASS(BlueprintType)
class UInventoryItemInstance : public UObject
{
	GENERATED_BODY()
public:
	UInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;


    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddTag(FGameplayTag Tag);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveTag(FGameplayTag Tag);

private:

	void SetItemDef(TSubclassOf<UInventoryItemDefinition> InDef);
	friend struct FInventoryList;


	// The item definition
	UPROPERTY(Replicated)
	TSubclassOf<UInventoryItemDefinition> ItemDef;

};