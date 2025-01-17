#include "InventoryManagerComponent.h"



UInventoryManagerComponent::UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UInventoryManagerComponent::CanAddItem()
{
	return true;
}

void UInventoryManagerComponent::AddItemInstance(UItemInstance* ItemInstance)
{
	InventoryList.Add(ItemInstance);
}

void UInventoryManagerComponent::RemoveItemInstance(UItemInstance* ItemInstance)
{
	if (InventoryList.Max() <= MaxCap)
	{
		InventoryList.Add(ItemInstance);
	}

}

TArray<UItemInstance*> UInventoryManagerComponent::GetAllItems() const
{
	TArray<UItemInstance*> Results;
	Results.Reserve(InventoryList.Num());
	for ( UItemInstance* Item : InventoryList)
	{
		if (Item != nullptr)
		{
			Results.Add(Item);
		}
	}
	return Results;
}
