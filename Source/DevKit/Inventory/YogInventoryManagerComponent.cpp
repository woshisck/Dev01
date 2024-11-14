// Fill out your copyright notice in the Description page of Project Settings.


#include "YogInventoryManagerComponent.h"



UYogInventoryManagerComponent::UYogInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

UInventoryItemInstance* FInventoryList::AddEntry(TSubclassOf<UInventoryItemDefinition> ItemClass, int32 StackCount)
{
	UInventoryItemInstance* Result = nullptr;

	check(ItemClass != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());


	FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UInventoryItemInstance>(OwnerComponent->GetOwner());  //@TODO: Using the actor instead of component as the outer due to UE-127172
	NewEntry.Instance->SetItemDef(ItemClass);
	for (UInventoryItemFragment* Fragment : GetDefault<UInventoryItemDefinition>(ItemClass)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	//const UInventoryItemDefinition* ItemCDO = GetDefault<UInventoryItemDefinition>(ItemDef);
	MarkItemDirty(NewEntry);

	return Result;
}

void FInventoryList::AddEntry(UInventoryItemInstance* Instance)
{
	unimplemented();
}

void FInventoryList::RemoveEntry(UInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}
