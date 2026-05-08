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
#include "Customization/RuneDataAssetDetails.h"
#include "Data/RuneDataAsset.h"
#include "Editor.h"
#include "RuneEditor/SRuneEditorWidget.h"
#include "ToolMenus.h"
#include "Tools/SActionBalanceWidget.h"
#include "Tools/SCharacterBalanceWidget.h"
#include "Tools/SDataEditorWidget.h"
#include "UI/CombatLogEditorUtilityWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"

namespace
{
	const FName RuneBalanceTabName(TEXT("DevKitRuneBalance"));
	const FName RuneEditorTabName(TEXT("DevKitRuneEditor"));
	const FName CharacterBalanceTabName(TEXT("DevKitCharacterBalance"));
	const FName ActionBalanceTabName(TEXT("DevKitActionBalance"));
	const FName CombatLogTabName(TEXT("DevKitCombatLog"));
}

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

		// 注册 URuneDataAsset 自定义 Detail Panel（在 Detail 顶部加快捷按钮）
		PropertyModule.RegisterCustomClassLayout(
			URuneDataAsset::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FRuneDataAssetDetails::MakeInstance));

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			RuneBalanceTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnRuneBalanceTab))
			.SetDisplayName(LOCTEXT("RuneBalanceTabTitle", "Rune Balance"))
			.SetTooltipText(LOCTEXT("RuneBalanceTabTooltip", "Open the DevKit Rune Balance panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			RuneEditorTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnRuneEditorTab))
			.SetDisplayName(LOCTEXT("RuneEditorTabTitle", "Rune Editor"))
			.SetTooltipText(LOCTEXT("RuneEditorTabTooltip", "Open the DevKit Rune Editor panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			CharacterBalanceTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnCharacterBalanceTab))
			.SetDisplayName(LOCTEXT("CharacterBalanceTabTitle", "Character Balance"))
			.SetTooltipText(LOCTEXT("CharacterBalanceTabTooltip", "Open the DevKit Character Balance panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ActionBalanceTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnActionBalanceTab))
			.SetDisplayName(LOCTEXT("ActionBalanceTabTitle", "Action Balance"))
			.SetTooltipText(LOCTEXT("ActionBalanceTabTooltip", "Open the DevKit Action Balance panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			CombatLogTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnCombatLogTab))
			.SetDisplayName(LOCTEXT("CombatLogTabTitle", "Combat Log"))
			.SetTooltipText(LOCTEXT("CombatLogTabTooltip", "Open the DevKit Combat Log panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDevKitEditorModule::RegisterDataEditorMenus));
	}


	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnregisterOwner(this);
		}
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuneBalanceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuneEditorTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CharacterBalanceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ActionBalanceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CombatLogTabName);
		CombatLogWidgetInstance.Reset();

		FEditorDelegates::OnMapOpened.RemoveAll(this);

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);

		// Unregister customization and callback
		FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

		if (PropertyEditorModule)
		{
			// 必须与 StartupModule() 中 Register 的名字一一对应
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(TEXT("ShopEntry"));
			PropertyEditorModule->UnregisterCustomClassLayout(URuneDataAsset::StaticClass()->GetFName());
			PropertyEditorModule->NotifyCustomizationModuleChanged();
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

	TSharedRef<SDockTab> SpawnRuneBalanceTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("RuneBalanceTabLabel", "Rune Balance"))
			[
				SNew(SDataEditorWidget)
			];
	}

	TSharedRef<SDockTab> SpawnRuneEditorTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("RuneEditorTabLabel", "Rune Editor"))
			[
				SNew(SRuneEditorWidget)
			];
	}

	TSharedRef<SDockTab> SpawnCharacterBalanceTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("CharacterBalanceTabLabel", "Character Balance"))
			[
				SNew(SCharacterBalanceWidget)
			];
	}

	TSharedRef<SDockTab> SpawnActionBalanceTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("ActionBalanceTabLabel", "Action Balance"))
			[
				SNew(SActionBalanceWidget)
			];
	}

	TSharedRef<SDockTab> SpawnCombatLogTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		TSharedRef<SWidget> Content = SNew(STextBlock)
			.Text(LOCTEXT("CombatLogNoEditorWorld", "Editor world is not available, Combat Log cannot be opened."));

		if (EditorWorld)
		{
			UCombatLogEditorUtilityWidget* CombatLogWidget = CreateWidget<UCombatLogEditorUtilityWidget>(
				EditorWorld,
				UCombatLogEditorUtilityWidget::StaticClass());

			if (CombatLogWidget)
			{
				CombatLogWidgetInstance.Reset(CombatLogWidget);
				Content = CombatLogWidget->TakeWidget();
			}
		}

		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("CombatLogTabLabel", "Combat Log"))
			.OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FDevKitEditorModule::OnCombatLogTabClosed))
			[
				Content
			];
	}

	void OnCombatLogTabClosed(TSharedRef<SDockTab> ClosedTab)
	{
		CombatLogWidgetInstance.Reset();
	}

	void RegisterDataEditorMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("DevKitTools"), LOCTEXT("DevKitToolsSection", "DevKit"));
		Section.AddSubMenu(
			TEXT("DevKitDataMenu"),
			LOCTEXT("DevKitDataMenuLabel", "DevKit Data"),
			LOCTEXT("DevKitDataMenuTooltip", "Open DevKit balance data editor panels."),
			FNewToolMenuDelegate::CreateRaw(this, &FDevKitEditorModule::FillDevKitDataMenu));

		Section.AddMenuEntry(
			TEXT("OpenDataEditor"),
			LOCTEXT("OpenDataEditorLabel", "DataEditor"),
			LOCTEXT("OpenDataEditorTooltip", "Open the DevKit Rune Balance panel. Kept for old DataEditor workflows."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuneBalanceTab)));

		Section.AddMenuEntry(
			TEXT("OpenCombatLog"),
			LOCTEXT("OpenCombatLogLabel", "Combat Log"),
			LOCTEXT("OpenCombatLogTooltip", "Open the DevKit Combat Log editor window."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenCombatLogTab)));
	}

	void FillDevKitDataMenu(UToolMenu* Menu)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("DevKitDataBalance"), LOCTEXT("DevKitDataBalanceSection", "Balance Editors"));
		Section.AddMenuEntry(
			TEXT("OpenRuneEditor"),
			LOCTEXT("OpenRuneEditorLabel", "Rune Editor"),
			LOCTEXT("OpenRuneEditorTooltip", "Open the Yog Rune Flow editor for rune assets, graph authoring, and validation work."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuneEditorTab)));
		Section.AddMenuEntry(
			TEXT("OpenCharacterBalance"),
			LOCTEXT("OpenCharacterBalanceLabel", "Character Balance"),
			LOCTEXT("OpenCharacterBalanceTooltip", "Edit character base and movement values."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenCharacterBalanceTab)));
		Section.AddMenuEntry(
			TEXT("OpenActionBalance"),
			LOCTEXT("OpenActionBalanceLabel", "Action Balance"),
			LOCTEXT("OpenActionBalanceTooltip", "Edit melee and musket action values."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenActionBalanceTab)));
		Section.AddMenuEntry(
			TEXT("OpenRuneBalance"),
			LOCTEXT("OpenRuneBalanceLabel", "Rune Balance"),
			LOCTEXT("OpenRuneBalanceTooltip", "Edit rune economy, trigger, and tuning values."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuneBalanceTab)));
	}

	void OpenRuneBalanceTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(RuneBalanceTabName);
	}

	void OpenRuneEditorTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(RuneEditorTabName);
	}

	void OpenCharacterBalanceTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(CharacterBalanceTabName);
	}

	void OpenActionBalanceTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ActionBalanceTabName);
	}

	void OpenCombatLogTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(CombatLogTabName);
	}

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
	TStrongObjectPtr<UCombatLogEditorUtilityWidget> CombatLogWidgetInstance;
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
