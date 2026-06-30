// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKitEditor.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "PropertyEditorModule.h"
#include "DevKitEditor/Util/YogEntryCustomization.h"
#include "Customization/RuneDataAssetDetails.h"
#include "Data/RuneDataAsset.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "FileHelpers.h"
#include "LevelEditor.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "PlayInEditorDataTypes.h"
#include "RuneEditor/SRuneEditorWidget.h"
#include "Styling/AppStyle.h"
#include "ToolMenuEntry.h"
#include "ToolMenus.h"
#include "Tools/SActionBalanceWidget.h"
#include "Tools/SBuffFlowDebugWidget.h"
#include "Tools/SCharacterBalanceWidget.h"
#include "Tools/SComboGraphManagerWidget.h"
#include "Tools/SDataEditorWidget.h"
#include "Tools/SEnvBatchTaggerWidget.h"
#include "Tools/SLevelDataWorkbenchWidget.h"
#include "Tools/SMaterialBatchToolsWidget.h"
#include "Tools/SMaterialTextureRulesWidget.h"
#include "Tools/SMetaProgressionWorkbenchWidget.h"
#include "Tools/SModelAssetComplianceWidget.h"
#include "Tools/StoryEncounter/SStoryEncounterWorkbenchWidget.h"
#include "Customization/GameplayAbilityComboGraphNodeDetails.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "UI/CombatLogEditorUtilityWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"

namespace
{
	const FName RuneBalanceTabName(TEXT("DevKitRuneBalance"));
	const FName RuneEditorTabName(TEXT("DevKitRuneEditor"));
	const FName CharacterBalanceTabName(TEXT("DevKitCharacterBalance"));
	const FName ComboManagerTabName(TEXT("DevKitComboManager"));
	const FName LevelDataWorkbenchTabName(TEXT("DevKitLevelDataWorkbench"));
	const FName ActionBalanceTabName(TEXT("DevKitActionBalance"));
	const FName CombatLogTabName(TEXT("DevKitCombatLog"));
	const FName BuffFlowDebugTabName(TEXT("DevKitBuffFlowDebug"));
	const FName MetaProgressionWorkbenchTabName(TEXT("DevKitMetaProgressionWorkbench"));
	const FName StoryEncounterWorkbenchTabName(TEXT("DevKitStoryEncounterWorkbench"));
	const FName EnvBatchTaggerTabName(TEXT("DevKitEnvBatchTagger"));
	const FName MaterialBatchToolsTabName(TEXT("DevKitMaterialBatchTools"));
	const FName ModelAssetComplianceTabName(TEXT("DevKitModelAssetCompliance"));
	const FName MaterialTextureRulesTabName(TEXT("DevKitMaterialTextureRules"));
	const FName PerformanceToolsLauncherTabName(TEXT("DevKitPerformanceToolsLauncher"));
	const TCHAR* DefaultEntryMenuMapPackagePath = TEXT("/Game/Maps/L_EntryMenu");

	FString NormalizeConfiguredMapPath(FString MapPath)
	{
		MapPath.TrimStartAndEndInline();
		if (MapPath.IsEmpty() || MapPath.Equals(TEXT("None"), ESearchCase::IgnoreCase))
		{
			return FString();
		}

		int32 ObjectNameSeparatorIndex = INDEX_NONE;
		if (MapPath.FindChar(TEXT('.'), ObjectNameSeparatorIndex))
		{
			MapPath.LeftInline(ObjectNameSeparatorIndex);
		}
		return MapPath;
	}
}

class FDevKitEditorModule : public FDefaultGameModuleImpl {
	typedef FDevKitEditorModule ThisClass;


	virtual void StartupModule() override
	{
		FEditorDelegates::OnMapOpened.AddRaw(this, &FDevKitEditorModule::OnMapOpened);
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout("ShopEntry", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FYogEntryCustomization::MakeInstance));

		// 注册 URuneDataAsset 自定义 Detail Panel（在 Detail 顶部加快捷按钮）
		PropertyModule.RegisterCustomClassLayout(
			URuneDataAsset::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FRuneDataAssetDetails::MakeInstance));
		PropertyModule.RegisterCustomClassLayout(
			UGameplayAbilityComboGraphNode::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FGameplayAbilityComboGraphNodeDetails::MakeInstance));

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
			.SetDisplayName(LOCTEXT("CharacterBalanceTabTitle", "Character Data Workbench"))
			.SetTooltipText(LOCTEXT("CharacterBalanceTabTooltip", "Open the DevKit character, montage, and Act data workbench."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ComboManagerTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnComboManagerTab))
			.SetDisplayName(LOCTEXT("ComboManagerTabTitle", "Combo Manager"))
			.SetTooltipText(LOCTEXT("ComboManagerTabTooltip", "Open the weapon and independent enemy combo graph manager."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			LevelDataWorkbenchTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnLevelDataWorkbenchTab))
			.SetDisplayName(LOCTEXT("LevelDataWorkbenchTabTitle", "Level Data Workbench"))
			.SetTooltipText(LOCTEXT("LevelDataWorkbenchTabTooltip", "Open the campaign and room data workbench."))
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

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			BuffFlowDebugTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnBuffFlowDebugTab))
			.SetDisplayName(LOCTEXT("BuffFlowDebugTabTitle", "BuffFlow Debug"))
			.SetTooltipText(LOCTEXT("BuffFlowDebugTabTooltip", "Open the DevKit BuffFlow debug panel."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			MetaProgressionWorkbenchTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnMetaProgressionWorkbenchTab))
			.SetDisplayName(LOCTEXT("MetaProgressionWorkbenchTabTitle", "Meta Progression Workbench"))
			.SetTooltipText(LOCTEXT("MetaProgressionWorkbenchTabTooltip", "Open the meta-progression node and currency workbench."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			StoryEncounterWorkbenchTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnStoryEncounterWorkbenchTab))
			.SetDisplayName(LOCTEXT("StoryEncounterWorkbenchTabTitle", "剧情教学工作台"))
			.SetTooltipText(LOCTEXT("StoryEncounterWorkbenchTabTooltip", "打开剧情、教程、触发点和流程画布工作台。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			EnvBatchTaggerTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnEnvBatchTaggerTab))
			.SetDisplayName(LOCTEXT("EnvBatchTaggerTabTitle", "环境合批标记"))
			.SetTooltipText(LOCTEXT("EnvBatchTaggerTabTooltip", "给当前选中的关卡 Actor 批量写入 EnvBatch 标记。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			MaterialBatchToolsTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnMaterialBatchToolsTab))
			.SetDisplayName(LOCTEXT("MaterialBatchToolsTabTitle", "关卡模型材质合批"))
			.SetTooltipText(LOCTEXT("MaterialBatchToolsTabTooltip", "根据 EnvBatch Actor Tag 生成关卡模型材质合批 dry-run 和 partial apply 命令。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			ModelAssetComplianceTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnModelAssetComplianceTab))
			.SetDisplayName(LOCTEXT("ModelAssetComplianceTabTitle", "模型资产合规检查"))
			.SetTooltipText(LOCTEXT("ModelAssetComplianceTabTooltip", "扫描 /Game/Art 下 StaticMesh，检查 LOD、材质槽、碰撞和静态环境分类是否符合性能分级制作标准。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			MaterialTextureRulesTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnMaterialTextureRulesTab))
			.SetDisplayName(LOCTEXT("MaterialTextureRulesTabTitle", "贴图命名规则与检查"))
			.SetTooltipText(LOCTEXT("MaterialTextureRulesTabTooltip", "创建贴图命名规则表，并检查选中 Texture2D 后缀与 sRGB 设置。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			PerformanceToolsLauncherTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnPerformanceToolsLauncherTab))
			.SetDisplayName(LOCTEXT("PerformanceToolsLauncherTabTitle", "性能工具快捷入口"))
			.SetTooltipText(LOCTEXT("PerformanceToolsLauncherTabTooltip", "打开性能分级、美术标记和主菜单运行的常用入口。"))
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
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ComboManagerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LevelDataWorkbenchTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ActionBalanceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CombatLogTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BuffFlowDebugTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MetaProgressionWorkbenchTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StoryEncounterWorkbenchTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EnvBatchTaggerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialBatchToolsTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ModelAssetComplianceTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MaterialTextureRulesTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PerformanceToolsLauncherTabName);
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

	TSharedRef<SDockTab> SpawnMetaProgressionWorkbenchTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("MetaProgressionWorkbenchTabLabel", "Meta Progression Workbench"))
			[
				SNew(SMetaProgressionWorkbenchWidget)
			];
	}

	TSharedRef<SDockTab> SpawnStoryEncounterWorkbenchTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("StoryEncounterWorkbenchTabLabel", "剧情教学工作台"))
			[
				SNew(SStoryEncounterWorkbenchWidget)
			];
	}

	TSharedRef<SDockTab> SpawnEnvBatchTaggerTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("EnvBatchTaggerTabLabel", "环境合批标记"))
			[
				SNew(SEnvBatchTaggerWidget)
			];
	}

	TSharedRef<SDockTab> SpawnMaterialBatchToolsTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("MaterialBatchToolsTabLabel", "关卡模型材质合批"))
			[
				SNew(SMaterialBatchToolsWidget)
			];
	}

	TSharedRef<SDockTab> SpawnModelAssetComplianceTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("ModelAssetComplianceTabLabel", "模型资产合规检查"))
			[
				SNew(SModelAssetComplianceWidget)
			];
	}

	TSharedRef<SDockTab> SpawnMaterialTextureRulesTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("MaterialTextureRulesTabLabel", "贴图命名规则与检查"))
			[
				SNew(SMaterialTextureRulesWidget)
			];
	}

	TSharedRef<SDockTab> SpawnPerformanceToolsLauncherTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("PerformanceToolsLauncherTabLabel", "性能工具快捷入口"))
			[
				SNew(SBorder)
				.Padding(16.f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PerformanceToolsLauncherTitle", "性能工具快捷入口"))
							.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 8.f, 0.f, 14.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PerformanceToolsLauncherDescription", "常用性能入口集中在这里：从游戏主菜单运行，以及打开环境合批标记工具。模型合规和贴图命名规则在编辑器工具栏或 DevKit 工具的美术资产工具中打开。"))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 8.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.f, 0.f, 8.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("LauncherPlayFromMainMenu", "从主菜单运行"))
								.ToolTipText(LOCTEXT("LauncherPlayFromMainMenuTooltip", "打开项目入口地图，然后用编辑器正常 Play 流程启动。"))
								.OnClicked(FOnClicked::CreateRaw(this, &FDevKitEditorModule::HandlePlayFromMainMenuClicked))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.f, 0.f, 8.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("LauncherOpenEnvBatchTagger", "环境合批标记"))
								.ToolTipText(LOCTEXT("LauncherOpenEnvBatchTaggerTooltip", "打开 Source / Proxy / Baked / Exclude 标记窗口。"))
								.OnClicked(FOnClicked::CreateRaw(this, &FDevKitEditorModule::HandleOpenEnvBatchTaggerClicked))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.Text(LOCTEXT("LauncherOpenThisPanel", "刷新快捷入口"))
								.ToolTipText(LOCTEXT("LauncherOpenThisPanelTooltip", "重新聚焦当前性能工具快捷入口窗口。"))
								.OnClicked(FOnClicked::CreateRaw(this, &FDevKitEditorModule::HandleOpenPerformanceToolsLauncherClicked))
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 4.f, 0.f, 6.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PerformanceToolsLauncherWorkflow", "推荐顺序：1. 在 DevKit 工具的美术资产工具中检查模型合规和贴图命名；2. 在这里使用环境合批标记给关卡对象写 EnvBatch Actor Tag；3. 正式打包链再执行关卡模型材质合批、VT Atlas、代理和替换。"))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 10.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PerformanceToolsLauncherNote", "主菜单运行入口读取 YogGameInstanceBase.FrontendMap；当前项目默认是 /Game/Maps/L_EntryMenu。"))
							.AutoWrapText(true)
						]
					]
				]
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
			LOCTEXT("DevKitToolsMenuLabel", "DevKit 工具"),
			LOCTEXT("DevKitToolsMenuTooltip", "打开 DevKit 编辑器工具。"),
			FNewToolMenuDelegate::CreateRaw(this, &FDevKitEditorModule::FillDevKitDataMenu));

		UToolMenu* PlayToolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
		FToolMenuSection& PlaySection = PlayToolbar->FindOrAddSection(TEXT("Play"));
		FToolMenuEntry& QuickPlayEntry = PlaySection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("DevKitPlayFromMainMenu"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::PlayFromMainMenu)),
			LOCTEXT("PlayFromMainMenuToolbarLabel", "从主菜单运行"),
			LOCTEXT("PlayFromMainMenuToolbarTooltip", "打开入口地图并从游戏主菜单开始 Play。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Play"))));
		QuickPlayEntry.ToolBarData.LabelOverride = FText::GetEmpty();

		UToolMenu* UserToolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
		FToolMenuSection& UserSection = UserToolbar->FindOrAddSection(TEXT("DevKitPerformanceTools"));
		FToolMenuEntry& LauncherEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitPerformanceToolsLauncher"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenPerformanceToolsLauncherTab)),
			LOCTEXT("OpenPerformanceToolsLauncherToolbarLabel", "性能工具"),
			LOCTEXT("OpenPerformanceToolsLauncherToolbarTooltip", "打开性能工具快捷入口。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Tools"))));
		LauncherEntry.ToolBarData.LabelOverride = LOCTEXT("OpenPerformanceToolsLauncherToolbarShortLabel", "性能工具");

		FToolMenuEntry& ModelComplianceEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitModelAssetCompliance"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenModelAssetComplianceTab)),
			LOCTEXT("OpenModelAssetComplianceToolbarLabel", "模型合规"),
			LOCTEXT("OpenModelAssetComplianceToolbarTooltip", "打开模型资产合规检查。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Audit"))));
		ModelComplianceEntry.ToolBarData.LabelOverride = LOCTEXT("OpenModelAssetComplianceToolbarShortLabel", "模型合规");

		FToolMenuEntry& TextureRulesEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitMaterialTextureRules"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenMaterialTextureRulesTab)),
			LOCTEXT("OpenMaterialTextureRulesToolbarLabel", "贴图规则"),
			LOCTEXT("OpenMaterialTextureRulesToolbarTooltip", "打开贴图命名规则与检查。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Search"))));
		TextureRulesEntry.ToolBarData.LabelOverride = LOCTEXT("OpenMaterialTextureRulesToolbarShortLabel", "贴图规则");

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
		FToolMenuSection& StorySection = Menu->FindOrAddSection(TEXT("DevKitStoryTools"), LOCTEXT("DevKitStoryToolsSection", "剧情工具"));
		StorySection.AddMenuEntry(
			TEXT("OpenStoryEncounterWorkbench"),
			LOCTEXT("OpenStoryEncounterWorkbenchLabel", "剧情教学工作台"),
			LOCTEXT("OpenStoryEncounterWorkbenchTooltip", "用流程图和剧情点DA配置剧情、教程和触发节点。"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenStoryEncounterWorkbenchTab)));
		BalanceSection.AddMenuEntry(
			TEXT("OpenMetaProgressionWorkbench"),
			LOCTEXT("OpenMetaProgressionWorkbenchLabel", "Meta Progression Workbench"),
			LOCTEXT("OpenMetaProgressionWorkbenchTooltip", "Edit meta upgrade nodes, currency rules, and local save slots."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenMetaProgressionWorkbenchTab)));

		FToolMenuSection& ArtAssetSection = Menu->FindOrAddSection(TEXT("DevKitArtAssetTools"), LOCTEXT("DevKitArtAssetToolsSection", "美术资产工具"));
		ArtAssetSection.AddMenuEntry(
			TEXT("OpenModelAssetCompliance"),
			LOCTEXT("OpenModelAssetComplianceLabel", "模型资产合规检查"),
			LOCTEXT("OpenModelAssetComplianceTooltip", "扫描 /Game/Art 下 StaticMesh，检查性能分级制作合规，不执行合批或写入生成资产。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Audit")),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenModelAssetComplianceTab)));
		ArtAssetSection.AddMenuEntry(
			TEXT("OpenMaterialTextureRules"),
			LOCTEXT("OpenMaterialTextureRulesLabel", "贴图命名规则与检查"),
			LOCTEXT("OpenMaterialTextureRulesTooltip", "创建贴图命名规则表，并检查 Content Browser 选中的 Texture2D。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Search")),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenMaterialTextureRulesTab)));

		FToolMenuSection& PerformanceSection = Menu->FindOrAddSection(TEXT("DevKitPerformanceTools"), LOCTEXT("DevKitPerformanceToolsSection", "性能工具"));
		PerformanceSection.AddMenuEntry(
			TEXT("OpenPerformanceToolsLauncher"),
			LOCTEXT("OpenPerformanceToolsLauncherLabel", "性能工具快捷入口"),
			LOCTEXT("OpenPerformanceToolsLauncherTooltip", "打开主菜单运行入口和关卡环境合批标记入口；模型、贴图资产检查在美术资产工具中打开。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Tools")),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenPerformanceToolsLauncherTab)));
		PerformanceSection.AddMenuEntry(
			TEXT("DevKitPlayFromMainMenu"),
			LOCTEXT("OpenPlayFromMainMenuLabel", "从主菜单运行"),
			LOCTEXT("OpenPlayFromMainMenuTooltip", "打开入口地图并从游戏主菜单开始 Play。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Play")),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::PlayFromMainMenu)));
		PerformanceSection.AddMenuEntry(
			TEXT("OpenEnvBatchTagger"),
			LOCTEXT("OpenEnvBatchTaggerLabel", "环境合批标记"),
			LOCTEXT("OpenEnvBatchTaggerTooltip", "给当前选中的静态环境 Actor 写入 Source、Proxy、Baked 或 Exclude 标记。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Audit")),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenEnvBatchTaggerTab)));

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

	FReply HandlePlayFromMainMenuClicked()
	{
		PlayFromMainMenu();
		return FReply::Handled();
	}

	FReply HandleOpenEnvBatchTaggerClicked()
	{
		OpenEnvBatchTaggerTab();
		return FReply::Handled();
	}

	FReply HandleOpenMaterialBatchToolsClicked()
	{
		OpenMaterialBatchToolsTab();
		return FReply::Handled();
	}

	FReply HandleOpenModelAssetComplianceClicked()
	{
		OpenModelAssetComplianceTab();
		return FReply::Handled();
	}

	FReply HandleOpenMaterialTextureRulesClicked()
	{
		OpenMaterialTextureRulesTab();
		return FReply::Handled();
	}

	FReply HandleOpenPerformanceToolsLauncherClicked()
	{
		OpenPerformanceToolsLauncherTab();
		return FReply::Handled();
	}

	FString ResolveEntryMenuMapPackagePath() const
	{
		FString ConfiguredFrontendMap;
		if (GConfig)
		{
			GConfig->GetString(
				TEXT("/Script/DevKit.YogGameInstanceBase"),
				TEXT("FrontendMap"),
				ConfiguredFrontendMap,
				GGameIni);
		}

		FString MapPackagePath = NormalizeConfiguredMapPath(ConfiguredFrontendMap);
		if (MapPackagePath.IsEmpty())
		{
			MapPackagePath = DefaultEntryMenuMapPackagePath;
		}
		return MapPackagePath;
	}

	void PlayFromMainMenu()
	{
		if (!GEditor)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PlayFromMainMenuNoEditor", "当前没有可用的编辑器实例。"));
			return;
		}

		if (GEditor->IsPlaySessionInProgress())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("PlayFromMainMenuAlreadyPlaying", "当前已经在 Play 或正在启动 Play。请先停止当前会话。"));
			return;
		}

		const FString MapPackagePath = ResolveEntryMenuMapPackagePath();
		FString MapFilename;
		if (!FPackageName::DoesPackageExist(MapPackagePath, &MapFilename))
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(LOCTEXT("PlayFromMainMenuMapMissing", "找不到入口地图：{0}"), FText::FromString(MapPackagePath)));
			return;
		}

		const bool bLoadedMap = FEditorFileUtils::LoadMap(MapFilename, false, true);
		if (!bLoadedMap)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(LOCTEXT("PlayFromMainMenuMapLoadFailed", "入口地图加载失败：{0}"), FText::FromString(MapPackagePath)));
			return;
		}

		FRequestPlaySessionParams SessionParams;
		SessionParams.WorldType = EPlaySessionWorldType::PlayInEditor;

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		if (TSharedPtr<IAssetViewport> ActiveViewport = LevelEditorModule.GetFirstActiveViewport())
		{
			SessionParams.DestinationSlateViewport = TWeakPtr<IAssetViewport>(ActiveViewport);
		}

		GEditor->RequestPlaySession(SessionParams);
		GEditor->StartQueuedPlaySessionRequest();
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

	void OpenMetaProgressionWorkbenchTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(MetaProgressionWorkbenchTabName);
	}

	void OpenStoryEncounterWorkbenchTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(StoryEncounterWorkbenchTabName);
	}

	void OpenEnvBatchTaggerTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(EnvBatchTaggerTabName);
	}

	void OpenMaterialBatchToolsTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(MaterialBatchToolsTabName);
	}

	void OpenModelAssetComplianceTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ModelAssetComplianceTabName);
	}

	void OpenMaterialTextureRulesTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(MaterialTextureRulesTabName);
	}

	void OpenPerformanceToolsLauncherTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(PerformanceToolsLauncherTabName);
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
