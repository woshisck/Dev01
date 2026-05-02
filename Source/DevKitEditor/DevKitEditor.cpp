// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKitEditor.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "PropertyEditorModule.h"
#include "ComboGraph/AssetTypeActions_GameplayAbilityComboGraph.h"
#include "DevKitEditor/Util/YogEntryCustomization.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"


class FDevKitEditorModule : public FDefaultGameModuleImpl {
	typedef FDevKitEditorModule ThisClass;


	virtual void StartupModule() override
	{
		FEditorDelegates::OnMapOpened.AddRaw(this, &FDevKitEditorModule::OnMapOpened);
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout("ShopEntry", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FYogEntryCustomization::MakeInstance));

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const EAssetTypeCategories::Type CombatCategory = AssetTools.RegisterAdvancedAssetCategory(
			TEXT("DevKitCombat"),
			LOCTEXT("DevKitCombatAssetCategory", "DevKit Combat"));
		RegisterAssetTypeAction(AssetTools, MakeShared<FAssetTypeActions_GameplayAbilityComboGraph>(CombatCategory));
	}


	virtual void ShutdownModule() override
	{
		FEditorDelegates::OnMapOpened.RemoveAll(this);

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);

		// Unregister customization and callback
		FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

		if (PropertyEditorModule)
		{
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(TEXT("YogTagContainerWrapper"));
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(TEXT("ActionData"));
		}

		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetTypeActions)
			{
				if (Action.IsValid())
				{
					AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
				}
			}
		}
		RegisteredAssetTypeActions.Reset();
	}

	void OnMapOpened(const FString& Filename, bool bAsTemplate);

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
};


void FDevKitEditorModule::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	//UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	//if (!EditorWorld) return;
	//// Spawn your actor in the editor world or modify existing actors here
	//// Example: Spawn actor at origin
	//FActorSpawnParameters SpawnParams;
	//SpawnParams.Name = FName(TEXT("MyAutoSpawnedActor"));
	//EditorWorld->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}


IMPLEMENT_MODULE(FDevKitEditorModule, DevKitEditor);
#undef LOCTEXT_NAMESPACE
