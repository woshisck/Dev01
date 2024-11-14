#include "InventoryItemInstance.h"


#include "Net/UnrealNetwork.h"



UInventoryItemInstance::UInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemDef);
}



void UInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
}

void UInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
}

int32 UInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return int32(12);
}

bool UInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return false;
}

void UInventoryItemInstance::AddTag(FGameplayTag Tag)
{
}

void UInventoryItemInstance::RemoveTag(FGameplayTag Tag)
{
}

void UInventoryItemInstance::SetItemDef(TSubclassOf<UInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}
