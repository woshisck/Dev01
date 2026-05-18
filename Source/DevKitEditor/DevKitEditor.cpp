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
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "Tools/SBuffFlowDebugWidget.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"

namespace
{
	const FName BuffFlowDebugTabName(TEXT("DevKitBuffFlowDebug"));
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

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			BuffFlowDebugTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnBuffFlowDebugTab))
			.SetDisplayName(LOCTEXT("BuffFlowDebugTabTitle", "BuffFlow Debug"))
			.SetTooltipText(LOCTEXT("BuffFlowDebugTabTooltip", "Open the DevKit BuffFlow debug panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		OpenBuffFlowDebugCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("DevKit.OpenBuffFlowDebug"),
			TEXT("Open the DevKit BuffFlow Debug panel."),
			FConsoleCommandDelegate::CreateLambda([]
			{
				FGlobalTabmanager::Get()->TryInvokeTab(BuffFlowDebugTabName);
			}));
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
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ComboManagerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LevelDataWorkbenchTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StoryEventWorkbenchTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ActionBalanceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CombatLogTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BuffFlowDebugTabName);
		CombatLogWidgetInstance.Reset();

		FEditorDelegates::OnMapOpened.RemoveAll(this);

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BuffFlowDebugTabName);
		OpenBuffFlowDebugCommand.Reset();

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);

		// Unregister customization and callback
		FPropertyEditorModule* PropertyEditorModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

		if (PropertyEditorModule)
		{
			// 必须与 StartupModule() 中 Register 的名字一一对应
			PropertyEditorModule->UnregisterCustomPropertyTypeLayout(TEXT("ShopEntry"));
			PropertyEditorModule->UnregisterCustomClassLayout(URuneDataAsset::StaticClass()->GetFName());
			PropertyEditorModule->UnregisterCustomClassLayout(UGameplayAbilityComboGraphNode::StaticClass()->GetFName());
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
			.Label(LOCTEXT("CharacterBalanceTabLabel", "Character Data Workbench"))
			[
				SNew(SCharacterBalanceWidget)
			];
	}

	TSharedRef<SDockTab> SpawnComboManagerTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("ComboManagerTabLabel", "Combo Manager"))
			[
				SNew(SComboGraphManagerWidget)
			];
	}

	TSharedRef<SDockTab> SpawnLevelDataWorkbenchTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("LevelDataWorkbenchTabLabel", "Level Data Workbench"))
			[
				SNew(SLevelDataWorkbenchWidget)
			];
	}

	TSharedRef<SDockTab> SpawnStoryEventWorkbenchTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("StoryEventWorkbenchTabLabel", "Story Event Workbench"))
			[
				SNew(SStoryEventWorkbenchWidget)
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

	TSharedRef<SDockTab> SpawnBuffFlowDebugTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("BuffFlowDebugTabLabel", "BuffFlow Debug"))
			[
				SNew(SBuffFlowDebugWidget)
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
			TEXT("DevKitToolsMenu"),
			LOCTEXT("DevKitToolsMenuLabel", "DevKit Tools"),
			LOCTEXT("DevKitToolsMenuTooltip", "Open DevKit authoring, balance, and debug editor tools."),
			FNewToolMenuDelegate::CreateRaw(this, &FDevKitEditorModule::FillDevKitDataMenu));
	}

	void FillDevKitDataMenu(UToolMenu* Menu)
	{
		FToolMenuSection& RuneSection = Menu->FindOrAddSection(TEXT("DevKitRuneTools"), LOCTEXT("DevKitRuneToolsSection", "Rune Tools"));
		RuneSection.AddMenuEntry(
			TEXT("OpenRuneEditor"),
			LOCTEXT("OpenRuneEditorLabel", "Rune Editor"),
			LOCTEXT("OpenRuneEditorTooltip", "Open the Yog Rune Flow editor for rune assets, graph authoring, and validation work."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuneEditorTab)));
		RuneSection.AddMenuEntry(
			TEXT("OpenRuneBalance"),
			LOCTEXT("OpenRuneBalanceLabel", "Rune Balance"),
			LOCTEXT("OpenRuneBalanceTooltip", "Open the legacy DataEditor panel for rune economy, trigger, migration, and tuning values."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuneBalanceTab)));

		FToolMenuSection& BalanceSection = Menu->FindOrAddSection(TEXT("DevKitBalanceEditors"), LOCTEXT("DevKitBalanceEditorsSection", "Balance Editors"));
		BalanceSection.AddMenuEntry(
			TEXT("OpenCharacterBalance"),
			LOCTEXT("OpenCharacterBalanceLabel", "Character Data Workbench"),
			LOCTEXT("OpenCharacterBalanceTooltip", "Edit character attributes, DA references, montage chains, and Act values."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenCharacterBalanceTab)));
		BalanceSection.AddMenuEntry(
			TEXT("OpenComboManager"),
			LOCTEXT("OpenComboManagerLabel", "Combo Manager"),
			LOCTEXT("OpenComboManagerTooltip", "Edit player weapon combo graphs and independent enemy combo graphs."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenComboManagerTab)));
		BalanceSection.AddMenuEntry(
			TEXT("OpenLevelDataWorkbench"),
			LOCTEXT("OpenLevelDataWorkbenchLabel", "Level Data Workbench"),
			LOCTEXT("OpenLevelDataWorkbenchTooltip", "Edit RoomData and CampaignData assets in one level data workbench."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenLevelDataWorkbenchTab)));
		BalanceSection.AddMenuEntry(
			TEXT("OpenStoryEventWorkbench"),
			LOCTEXT("OpenStoryEventWorkbenchLabel", "Story Event Workbench"),
			LOCTEXT("OpenStoryEventWorkbenchTooltip", "Edit story event registries and compare Campaign StoryEventTags against configured story actions."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenStoryEventWorkbenchTab)));

		FToolMenuSection& DebugSection = Menu->FindOrAddSection(TEXT("DevKitDebugTools"), LOCTEXT("DevKitDebugToolsSection", "Debug Tools"));
		DebugSection.AddMenuEntry(
			TEXT("OpenCombatLog"),
			LOCTEXT("OpenCombatLogLabel", "Combat Log"),
			LOCTEXT("OpenCombatLogTooltip", "Open the DevKit Combat Log editor window."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenCombatLogTab)));
		DebugSection.AddMenuEntry(
			TEXT("OpenBuffFlowDebug"),
			LOCTEXT("OpenBuffFlowDebugLabel", "BuffFlow Debug"),
			LOCTEXT("OpenBuffFlowDebugTooltip", "Open the DevKit BuffFlow debug editor window."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenBuffFlowDebugTab)));
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

	void OpenComboManagerTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ComboManagerTabName);
	}

	void OpenLevelDataWorkbenchTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(LevelDataWorkbenchTabName);
	}

	void OpenStoryEventWorkbenchTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(StoryEventWorkbenchTabName);
	}

	void OpenActionBalanceTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ActionBalanceTabName);
	}

	void OpenCombatLogTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(CombatLogTabName);
	}

	void OpenBuffFlowDebugTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(BuffFlowDebugTabName);
	}

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	TSharedRef<SDockTab> SpawnBuffFlowDebugTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("BuffFlowDebugTabLabel", "BuffFlow Debug"))
			[
				SNew(SBuffFlowDebugWidget)
			];
	}

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
	TUniquePtr<FAutoConsoleCommand> OpenBuffFlowDebugCommand;
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
