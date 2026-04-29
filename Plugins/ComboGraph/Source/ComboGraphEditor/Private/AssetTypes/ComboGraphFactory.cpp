// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "AssetTypes/ComboGraphFactory.h"

#include "BlueprintEditorSettings.h"
#include "Graph/ComboGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UComboGraphFactory"

UComboGraphFactory::UComboGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UComboGraph::StaticClass();

	// If we ever need to pick up a different Parent class in editor (have to implement ConfigureProperties())
	// ParentClass = UComboGraphAbility::StaticClass();
}

bool UComboGraphFactory::ConfigureProperties()
{
	// TSharedRef<SGameplayAbilityBlueprintCreateDialog> Dialog = SNew(SGameplayAbilityBlueprintCreateDialog);
	// return Dialog->ConfigureProperties(this);
	return Super::ConfigureProperties();
}

UObject* UComboGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UComboGraph::StaticClass()));
	return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
}

#undef LOCTEXT_NAMESPACE
