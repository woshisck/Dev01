// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "InventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class UInventoryItemInstance;
struct FFrame;

//////////////////////////////////////////////////////////////////////

// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class DEVKIT_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};

//////////////////////////////////////////////////////////////////////

/**
 * ULyraInventoryItemDefinition
 */
UCLASS(Blueprintable, Const, Abstract)
class UInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

public:
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;
};

//@TODO: Make into a subsystem instead?
UCLASS()
class ULyraInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UInventoryItemDefinition> ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass);
};
