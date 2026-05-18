#include "YogComboGraphEditorModule.h"

#include "AssetTypeActions_GameplayAbilityComboGraph.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "YogComboGraphEditor"

void FYogComboGraphEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(
		TEXT("YogComboGraph"),
		LOCTEXT("YogComboGraphAssetCategory", "Yog Combo Graph"));

	RegisteredAction = MakeShared<FAssetTypeActions_GameplayAbilityComboGraph>(Category);
	AssetTools.RegisterAssetTypeActions(RegisteredAction.ToSharedRef());
}

void FYogComboGraphEditorModule::ShutdownModule()
{
	if (RegisteredAction.IsValid() && FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(RegisteredAction.ToSharedRef());
	}
	RegisteredAction.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYogComboGraphEditorModule, YogComboGraphEditor)
