#include "YogComboGraphEditorModule.h"

#include "AssetTypeActions_GameplayAbilityComboGraph.h"
#include "AssetToolsModule.h"
#include "EdNode_ComboGraphRoot.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleManager.h"
#include "SEdNode_ComboGraphRoot.h"

namespace
{
	struct FComboGraphNodeFactory : public FGraphPanelNodeFactory
	{
		virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
		{
			if (UEdNode_ComboGraphRoot* RootNode = Cast<UEdNode_ComboGraphRoot>(Node))
			{
				return SNew(SEdNode_ComboGraphRoot, RootNode);
			}
			return nullptr;
		}
	};
}

#define LOCTEXT_NAMESPACE "YogComboGraphEditor"

void FYogComboGraphEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(
		TEXT("YogComboGraph"),
		LOCTEXT("YogComboGraphAssetCategory", "Yog Combo Graph"));

	RegisteredAction = MakeShared<FAssetTypeActions_GameplayAbilityComboGraph>(Category);
	AssetTools.RegisterAssetTypeActions(RegisteredAction.ToSharedRef());

	ComboGraphNodeFactory = MakeShared<FComboGraphNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(ComboGraphNodeFactory);
}

void FYogComboGraphEditorModule::ShutdownModule()
{
	if (ComboGraphNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(ComboGraphNodeFactory);
		ComboGraphNodeFactory.Reset();
	}

	if (RegisteredAction.IsValid() && FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(RegisteredAction.ToSharedRef());
	}
	RegisteredAction.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYogComboGraphEditorModule, YogComboGraphEditor)
