// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventoryItemDefinition.h"

const UInventoryItemFragment* ULyraInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UInventoryItemDefinition> ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	return nullptr;
}

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}




const UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	return nullptr;
}
