// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "CreationMenu/GSCGameplayEffectCreationMenu.h"

#include "ContentBrowserModule.h"
#include "CreationMenu/GSCCreationMenu.h"
#include "Framework/Commands/UICommandList.h"

FString FGSCGameplayEffectCreationData::ToString() const
{
	return FString::Printf(
		TEXT("MenuPath: %s, BaseName: %s, ParentClass: %s, AssetPrefix: %s, TooltipText: %s"),
		*MenuPath,
		*BaseName,
		*ParentClass->GetName(),
		*AssetPrefix,
		*TooltipText.ToString()
	);
}

UGSCGameplayEffectCreationMenu::UGSCGameplayEffectCreationMenu()
{
	AddDefinition(
		"Abilities|Damage|Instant",
		"Damage",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Damage_Instant.BP_GE_Template_Damage_Instant_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_DamageInstant_Tooltip", "Instant Damage Effect\n\nInstant Gameplay Effect that apply Damage (and substracts health)")
	);

	AddDefinition(
		"Abilities|Damage|Periodic",
		"Damage_Periodic",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Damage_Periodic.BP_GE_Template_Damage_Periodic_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_DamagePeriodic_Tooltip", "Damage Over Time Effect\n\nGameplay Effect with a set Duration that periodically apply Damage (and substracts health)")
	);

	AddDefinition(
		"Abilities|Heal|Instant",
		"Heal",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Heal_Instant.BP_GE_Template_Heal_Instant_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_Heal_Tooltip", "Instant Heal Effect\n\nInstant Gameplay Effect that increase Health by a set amount")
	);

	AddDefinition(
		"Abilities|Heal|Periodic",
		"Heal_Periodic",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Heal_Periodic.BP_GE_Template_Heal_Periodic_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_HealPeriodic_Tooltip", "Heal Over Time Effect\n\nGameplay Effect with a set Duration that periodically increase the Health Attribute")
	);

	AddDefinition(
		"Abilities|Cost",
		"Cost",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Cost.BP_GE_Template_Cost_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_Cost_Tooltip", "Cost Gameplay Effect\n\nInstant Gameplay meant to be used as a Cost Gameplay Effect in Abilities, substracts one more more Attributes")
	);

	AddDefinition(
		"Attributes|Regen",
		"Regen",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Regen.BP_GE_Template_Regen_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_Regen_Tooltip", "Gameplay Effect with an Infinite Duration that periodically increase one or more Attributes")
	);

	AddDefinition(
		"Attributes|Regen Negative",
		"Regen_Negative",
		StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/GASCompanion/Templates/GE_Definitions/BP_GE_Template_Regen_Negative.BP_GE_Template_Regen_Negative_C")),
		NSLOCTEXT("GASCompanionEditor", "UGSCGameplayEffect_NegativeRegen_Tooltip", "Gameplay Effect an with Infinite Duration that periodically decrease one or more Attributes")
	);
}

void UGSCGameplayEffectCreationMenu::AddMenuExtensions() const
{
	TSharedPtr<FUICommandList> CommandBindings = MakeShareable(new FUICommandList());

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TWeakObjectPtr<const UGSCGameplayEffectCreationMenu> WeakThis(this);

	ContentBrowserModule.GetAllAssetContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda([WeakThis](const TArray<FString>& SelectedPaths)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();
		if (WeakThis.IsValid())
		{
			const FText MenuLabel = NSLOCTEXT("GASCompanionEditor", "CreateGameplayEffect", "GAS Companion | Gameplay Effect");
			const FText MenuTooltip = NSLOCTEXT("GASCompanionEditor", "CreateGameplayEffect_Tooltip", "Create new Gameplay Effect from list of curated parents");
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

void UGSCGameplayEffectCreationMenu::TopMenuBuild(FMenuBuilder& TopMenuBuilder, const FText InMenuLabel, const FText InMenuTooltip, const TArray<FString> SelectedPaths, const TArray<FGSCGameplayEffectCreationData> Definitions)
{
	return UGSCCreationMenu::TopMenuBuild<FGSCGameplayEffectCreationData>(TopMenuBuilder, InMenuLabel, InMenuTooltip, SelectedPaths, Definitions);
}

void UGSCGameplayEffectCreationMenu::AddDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UObject> ParentClass, const FText TooltipText)
{
	if (!ParentClass)
	{
		EDITOR_LOG(Warning, TEXT("Can't add Gameplay Effect Definitions for %s, no valid parent class"), *MenuPath)
		return;
	}

	Definitions.Add(CreateDefinition(MenuPath, BaseName, ParentClass, TooltipText));
}

FGSCGameplayEffectCreationData UGSCGameplayEffectCreationMenu::CreateDefinition(const FString MenuPath, const FString BaseName, const TSubclassOf<UObject> ParentClass, const FText TooltipText)
{
	FGSCGameplayEffectCreationData Definition;
	Definition.MenuPath = MenuPath;
	Definition.BaseName = BaseName;
	Definition.ParentClass = ParentClass;
	Definition.TooltipText = TooltipText;
	return Definition;
}
