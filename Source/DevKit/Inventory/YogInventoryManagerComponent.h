// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include <DevKit/Equipment/YogEquipmentInstance.h>
#include "Net/UnrealNetwork.h"


#include "YogInventoryManagerComponent.generated.h"


class UObject;
struct FLyraInventoryList;
struct FNetDeltaSerializeInfo;

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 ExampleIntProperty;
};


USTRUCT(BlueprintType)
struct FInventoryArray : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()


	// Replicated list of items
	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryArray>(Entries, DeltaParms, *this);
	}
};


template<>
struct TStructOpsTypeTraits<FInventoryArray> : public TStructOpsTypeTraitsBase2<FInventoryArray>
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
