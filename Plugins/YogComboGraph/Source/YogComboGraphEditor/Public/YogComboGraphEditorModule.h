#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "Modules/ModuleInterface.h"

class IAssetTypeActions;

class FYogComboGraphEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<IAssetTypeActions> RegisteredAction;
	TSharedPtr<FGraphPanelNodeFactory> ComboGraphNodeFactory;
};
