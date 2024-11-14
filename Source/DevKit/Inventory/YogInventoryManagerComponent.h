// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include <DevKit/Equipment/YogEquipmentInstance.h>
#include "Net/UnrealNetwork.h"

#include "InventoryItemInstance.h"

#include "YogInventoryManagerComponent.generated.h"


class UObject;
struct FInventoryList;
struct FNetDeltaSerializeInfo;

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()


private:
	friend FInventoryList;
	friend UYogInventoryManagerComponent;


	UPROPERTY()
	TObjectPtr<UInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};


USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	// Replicated list of items

public:
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParms, *this);
	}

	UInventoryItemInstance* AddEntry(TSubclassOf<UInventoryItemDefinition> ItemClass, int32 StackCount);

	void AddEntry(UInventoryItemInstance* Instance);

	void RemoveEntry(UInventoryItemInstance* Instance);


private:
	friend UYogInventoryManagerComponent;

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};


template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};



UCLASS(BlueprintType)
class DEVKIT_API UYogInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UYogInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


};
