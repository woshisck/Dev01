// Copyright 2020 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeCategories.h"
#include "AddToProjectConfig.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;

class FGASCompanionEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	* Opens a dialog to add code files or blueprints to the current project.
	*
	* @param	Config			Configuration options for the dialog
	*/
	static void OpenAddToProjectDialog(const FAddToProjectConfig& Config = FAddToProjectConfig());

	EAssetTypeCategories::Type GetAssetTypeCategory() const;

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static FGASCompanionEditorModule& Get() {
		return FModuleManager::LoadModuleChecked<FGASCompanionEditorModule>("GASCompanionEditor");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("GASCompanionEditor");
	}

private:

	FName SettingsContainerName = FName(TEXT("Project"));
	FName SettingsCategoryName = FName(TEXT("GAS Companion"));
	FName SettingsGameplayEffectsSectionName = FName(TEXT("Gameplay Effect Definitions"));
	FName SettingsGameplayAbilitiesSectionName = FName(TEXT("Gameplay Abilities Definitions"));
	FName SettingsAttributeSetSectionName = FName(TEXT("GASCompanion_AbilitySystemGlobals"));

	TSharedPtr<class FUICommandList> PluginCommands;
	EAssetTypeCategories::Type AssetTypeCategory = EAssetTypeCategories::None;
	FDelegateHandle MapChangedHandle;

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
	/** All registered customization. Cached here so that we can unregister during shutdown */
    TArray<FName> RegisteredClassCustomizations;

	void RegisterMenus();
	void RegisterComboMenus() const;

	static void RegisterCommands();
	static void UnregisterCommands();
	static void InitializeStyles();
	static void ShutdownStyle();

	template <typename TDetailsCustomization>
	void RegisterDetailsCustomization(const FName ClassToCustomize, FPropertyEditorModule& PropertyModule)
	{
		PropertyModule.RegisterCustomClassLayout(ClassToCustomize, FOnGetDetailCustomizationInstance::CreateStatic(&TDetailsCustomization::MakeInstance));
		RegisteredClassCustomizations.Add(ClassToCustomize);
	}

	void UnregisterDetailsCustomization(FPropertyEditorModule& PropertyEditorModule);

	/**
	* Generates menu content for the toolbar combo button drop down menu
	*
	* @return	Menu content widget
	*/
	static TSharedRef<SWidget> GenerateToolbarSettingsMenu(TSharedRef<FUICommandList> InCommandList);
};
