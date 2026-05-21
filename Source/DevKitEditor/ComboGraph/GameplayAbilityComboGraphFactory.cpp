#include "ComboGraph/GameplayAbilityComboGraphFactory.h"

#include "Data/GameplayAbilityComboGraph.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraphFactory"

UGameplayAbilityComboGraphFactory::UGameplayAbilityComboGraphFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UGameplayAbilityComboGraph::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

FText UGameplayAbilityComboGraphFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Gameplay Ability Combo Graph");
}

UObject* UGameplayAbilityComboGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UGameplayAbilityComboGraph>(InParent, UGameplayAbilityComboGraph::StaticClass(), Name, Flags | RF_Transactional);
}

#undef LOCTEXT_NAMESPACE
