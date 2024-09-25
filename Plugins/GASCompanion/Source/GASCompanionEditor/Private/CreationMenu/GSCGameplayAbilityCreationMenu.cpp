// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "CreationMenu/GSCGameplayAbilityCreationMenu.h"

#include "ContentBrowserModule.h"
#include "Abilities/GSCGameplayAbility_MeleeBase.h"
#include "CreationMenu/GSCCreationMenu.h"
#include "Framework/Commands/UICommandList.h"

UGSCGameplayAbilityCreationMenu::UGSCGameplayAbilityCreationMenu()
{
	AddDefinition(
		"Common|Melee",
		"Melee",
		UGSCGameplayAbility_MeleeBase::StaticClass(),
		NSLOCTEXT("GASCompanionEditor", "GA_GSC_Melee_Base_Tooltip", "Melee Ability with combo capabilities.\n\nDefine a list of Montages to play and setup the Combo Notify States in Montages")
	);
}

void UGSCGameplayAbilityCreationMenu::AddMenuExtensions() const
{
	TSharedPtr<FUICommandList> CommandBindings = MakeShareable(new FUICommandList());

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TWeakObjectPtr<const UGSCGameplayAbilityCreationMenu> WeakThis(this);

	ContentBrowserModule.GetAllAssetContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda([WeakThis](const TArray<FString>& SelectedPaths)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();
		if (WeakThis.IsValid())
		{
			const FText MenuLabel = NSLOCTEXT("GASCompanionEditor", "CreateGameplayAbility", "GAS Companion | Gameplay Ability");
			const FText MenuTooltip = NSLOCTEXT("GASCompanionEditor", "CreateGameplayAbility_Tooltip", "Create new Gameplay Ability from list of curated parents");
			Extender->AddMenuExtension(
				"ContentBrowserNewAdvancedAsset",
				EExtensionHook::After,
				TSharedPtr<FUICommandList>(),
				FMenuExtensionDelegate::CreateStatic(&TopMenuBuild, MenuLabel, MenuTooltip, SelectedPaths, WeakThis->Definitions)
			);
		}

		return Extender;
	}));
}

void UGSCGameplayAbilityCreationMenu::TopMenuBuild(FMenuBuilder& TopMenuBuilder, const FText InMenuLabel, const FText InMenuTooltip, const TArray<FString> SelectedPaths, TArray<FGSCGameplayAbilityCreationData> Definitions)
{
	return UGSCCreationMenu::TopMenuBuild<FGSCGameplayAbilityCreationData>(TopMenuBuilder, InMenuLabel, InMenuTooltip, SelectedPaths, Definitions);
}

void UGSCGameplayAbilityCreationMenu::AddDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UGameplayAbility> ParentClass, const FText TooltipText)
{
	Definitions.Add(CreateDefinition(MenuPath, BaseName, ParentClass, TooltipText));
}

FGSCGameplayAbilityCreationData UGSCGameplayAbilityCreationMenu::CreateDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UGameplayAbility> ParentClass, const FText TooltipText)
{
	FGSCGameplayAbilityCreationData Definition;
	Definition.MenuPath = MenuPath;
	Definition.BaseName = BaseName;
	Definition.ParentClass = ParentClass;
	Definition.TooltipText = TooltipText;
	return Definition;
}
