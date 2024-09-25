// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct FGameplayEffectContext;
struct FGraphPanelNodeFactory;
class IAssetTypeActions;

class FComboGraphEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:

	void RegisterAssetTypeAction(class IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

	/** Check for configuration to get a warning on startup if settings aren't configured correctly (for AbilitySystemGlobals class name) */
	void HandleAbilitySystemGlobalsSettings();

	/** Validation warnings token handlers */
	void AddAbilitySystemGlobalsClassName();

private:
	/** All created asset type actions. Cached here so that we can unregister it during shutdown. */
	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;

	/** Register panel node factory so that we can unregister during shutdown*/
	TSharedPtr<FGraphPanelNodeFactory> GraphNodeFactory;
	
	void OnPostEngineInit();
};
