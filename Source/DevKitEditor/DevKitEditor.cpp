// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKitEditor.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "Brushes/SlateImageBrush.h"
#include "IAssetTools.h"
#include "IAssetTypeActions.h"
#include "PropertyEditorModule.h"
#include "DevKitEditor/Util/YogEntryCustomization.h"
#include "Customization/RuneDataAssetDetails.h"
#include "Data/RuneDataAsset.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "FileHelpers.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "LevelEditor.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PlayInEditorDataTypes.h"
#include "RuneEditor/SRuneEditorWidget.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
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
#include "Tools/SRuntimeGMSettingsWidget.h"
#include "Tools/STextureVTAuditWidget.h"
#include "Tools/SVirtualTextureCollectionManagerWidget.h"
#include "Tools/StoryEncounter/SStoryEncounterWorkbenchWidget.h"
#include "Customization/GameplayAbilityComboGraphNodeDetails.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "System/YogRuntimeGMSubsystem.h"
#include "UI/CombatLogEditorUtilityWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"

DEFINE_LOG_CATEGORY(DevKitEditor);

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
	const FName TextureVTAuditTabName(TEXT("DevKitTextureVTAudit"));
	const FName VirtualTextureCollectionManagerTabName(TEXT("DevKitVirtualTextureCollectionManager"));
	const FName PerformanceToolsLauncherTabName(TEXT("DevKitPerformanceToolsLauncher"));
	const FName RuntimeGMSettingsTabName(TEXT("DevKitRuntimeGMSettings"));
	const FName DevKitEditorStyleSetName(TEXT("DevKitEditorStyle"));
	const FName ModelAssetComplianceIconName(TEXT("DevKitEditor.ModelAssetCompliance"));
	const FName ModelAssetComplianceIconSmallName(TEXT("DevKitEditor.ModelAssetCompliance.Small"));
	const FName MaterialTextureRulesIconName(TEXT("DevKitEditor.MaterialTextureRules"));
	const FName MaterialTextureRulesIconSmallName(TEXT("DevKitEditor.MaterialTextureRules.Small"));
	const FName TextureVTAuditIconName(TEXT("DevKitEditor.TextureVTAudit"));
	const FName TextureVTAuditIconSmallName(TEXT("DevKitEditor.TextureVTAudit.Small"));
	const FName VTCManagerIconName(TEXT("DevKitEditor.VTCManager"));
	const FName VTCManagerIconSmallName(TEXT("DevKitEditor.VTCManager.Small"));
	const FName PerformanceToolsIconName(TEXT("DevKitEditor.PerformanceTools"));
	const FName PerformanceToolsIconSmallName(TEXT("DevKitEditor.PerformanceTools.Small"));
	const FName PlayFromMainMenuIconName(TEXT("DevKitEditor.PlayFromMainMenu"));
	const FName PlayFromMainMenuIconSmallName(TEXT("DevKitEditor.PlayFromMainMenu.Small"));
	const FName EnvBatchTaggerIconName(TEXT("DevKitEditor.EnvBatchTagger"));
	const FName EnvBatchTaggerIconSmallName(TEXT("DevKitEditor.EnvBatchTagger.Small"));
	const TCHAR* DefaultEntryMenuMapPackagePath = TEXT("/Game/Maps/L_EntryMenu");

	TSharedPtr<FSlateStyleSet> DevKitEditorStyleSet;

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

	void RegisterToolIcon(const FName& IconName, const FName& IconSmallName, const TCHAR* IconBaseName)
	{
		const FString IconPath = DevKitEditorStyleSet->RootToContentDir(IconBaseName, TEXT(".png"));
		DevKitEditorStyleSet->Set(IconName, new FSlateImageBrush(IconPath, FVector2D(40.f, 40.f)));
		DevKitEditorStyleSet->Set(IconSmallName, new FSlateImageBrush(IconPath, FVector2D(16.f, 16.f)));
	}

	void RegisterDevKitEditorStyle()
	{
		if (DevKitEditorStyleSet.IsValid())
		{
			return;
		}

		DevKitEditorStyleSet = MakeShared<FSlateStyleSet>(DevKitEditorStyleSetName);
		DevKitEditorStyleSet->SetContentRoot(FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/DevKitEditor/Resources")));

		RegisterToolIcon(ModelAssetComplianceIconName, ModelAssetComplianceIconSmallName, TEXT("ToolIcon_ModelCompliance"));
		RegisterToolIcon(MaterialTextureRulesIconName, MaterialTextureRulesIconSmallName, TEXT("ToolIcon_MaterialCompliance"));
		RegisterToolIcon(TextureVTAuditIconName, TextureVTAuditIconSmallName, TEXT("ToolIcon_TextureVTAudit"));
		RegisterToolIcon(VTCManagerIconName, VTCManagerIconSmallName, TEXT("ToolIcon_VTCManager"));
		RegisterToolIcon(PerformanceToolsIconName, PerformanceToolsIconSmallName, TEXT("ToolIcon_PerformanceTools"));
		RegisterToolIcon(PlayFromMainMenuIconName, PlayFromMainMenuIconSmallName, TEXT("ToolIcon_PlayMainMenu"));
		RegisterToolIcon(EnvBatchTaggerIconName, EnvBatchTaggerIconSmallName, TEXT("ToolIcon_EnvBatchTagger"));

		FSlateStyleRegistry::RegisterSlateStyle(*DevKitEditorStyleSet);
	}

	void UnregisterDevKitEditorStyle()
	{
		if (!DevKitEditorStyleSet.IsValid())
		{
			return;
		}

		FSlateStyleRegistry::UnRegisterSlateStyle(*DevKitEditorStyleSet);
		DevKitEditorStyleSet.Reset();
	}

	FSlateIcon GetModelAssetComplianceIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, ModelAssetComplianceIconName, ModelAssetComplianceIconSmallName);
	}

	FSlateIcon GetMaterialTextureRulesIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, MaterialTextureRulesIconName, MaterialTextureRulesIconSmallName);
	}

	FSlateIcon GetTextureVTAuditIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, TextureVTAuditIconName, TextureVTAuditIconSmallName);
	}

	FSlateIcon GetVTCManagerIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, VTCManagerIconName, VTCManagerIconSmallName);
	}

	FSlateIcon GetPerformanceToolsIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, PerformanceToolsIconName, PerformanceToolsIconSmallName);
	}

	FSlateIcon GetPlayFromMainMenuIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, PlayFromMainMenuIconName, PlayFromMainMenuIconSmallName);
	}

	FSlateIcon GetEnvBatchTaggerIcon()
	{
		return FSlateIcon(DevKitEditorStyleSetName, EnvBatchTaggerIconName, EnvBatchTaggerIconSmallName);
	}

	class FRuntimeGMPIEInputProcessor final : public IInputProcessor
	{
	public:
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
#if !UE_BUILD_SHIPPING
			if (InKeyEvent.GetKey() != EKeys::F12 || !GEditor || !GEditor->PlayWorld)
			{
				return false;
			}

			UWorld* PlayWorld = GEditor->PlayWorld;
			APlayerController* PlayerController = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
			UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
			UYogRuntimeGMSubsystem* RuntimeGM = GameInstance ? GameInstance->GetSubsystem<UYogRuntimeGMSubsystem>() : nullptr;
			if (!RuntimeGM)
			{
				return false;
			}

			const bool bHandled = RuntimeGM->ToggleGMPanel(PlayerController);
			if (bHandled)
			{
				UE_LOG(DevKitEditor, Display, TEXT("[RuntimeGM] PIE F12 toggled Runtime GM panel from editor input processor."));
			}
			return bHandled;
#else
			return false;
#endif
		}

		virtual const TCHAR* GetDebugName() const override
		{
			return TEXT("DevKitRuntimeGMPIEInputProcessor");
		}
	};
}

class FDevKitEditorModule : public FDefaultGameModuleImpl {
	typedef FDevKitEditorModule ThisClass;


	virtual void StartupModule() override
	{
		RegisterDevKitEditorStyle();
		RegisterRuntimeGMPIEInputProcessor();

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
			.SetDisplayName(LOCTEXT("MaterialTextureRulesTabTitle", "材质合规检查"))
			.SetTooltipText(LOCTEXT("MaterialTextureRulesTabTooltip", "检查贴图命名、sRGB、尺寸、VT 建议和材质性能分级接口。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			TextureVTAuditTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnTextureVTAuditTab))
			.SetDisplayName(LOCTEXT("TextureVTAuditTabTitle", "贴图 VT 审计"))
			.SetTooltipText(LOCTEXT("TextureVTAuditTabTooltip", "扫描 /Game/Art 下 Texture2D，检查 VT 状态、尺寸警告和 VTC 兼容性。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			VirtualTextureCollectionManagerTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnVirtualTextureCollectionManagerTab))
			.SetDisplayName(LOCTEXT("VTCManagerTabTitle", "VTC 管理器"))
			.SetTooltipText(LOCTEXT("VTCManagerTabTooltip", "管理项目中所有 VirtualTextureCollection，支持批量创建、追加成员、合规检查。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			PerformanceToolsLauncherTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnPerformanceToolsLauncherTab))
			.SetDisplayName(LOCTEXT("PerformanceToolsLauncherTabTitle", "性能工具快捷入口"))
			.SetTooltipText(LOCTEXT("PerformanceToolsLauncherTabTooltip", "打开性能分级、美术标记和主菜单运行的常用入口。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			RuntimeGMSettingsTabName,
			FOnSpawnTab::CreateRaw(this, &FDevKitEditorModule::SpawnRuntimeGMSettingsTab))
			.SetDisplayName(LOCTEXT("RuntimeGMSettingsTabTitle", "运行时 GM 配置"))
			.SetTooltipText(LOCTEXT("RuntimeGMSettingsTabTooltip", "配置 F12 Runtime GM 面板使用的武器、敌人、刷敌数量和快速打包入口。"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDevKitEditorModule::RegisterDataEditorMenus));
	}


	virtual void ShutdownModule() override
	{
		UnregisterRuntimeGMPIEInputProcessor();
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
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TextureVTAuditTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(VirtualTextureCollectionManagerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PerformanceToolsLauncherTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RuntimeGMSettingsTabName);
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
		UnregisterDevKitEditorStyle();
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
			.Label(LOCTEXT("MaterialTextureRulesTabLabel", "材质合规检查"))
			[
				SNew(SMaterialTextureRulesWidget)
			];
	}

	TSharedRef<SDockTab> SpawnTextureVTAuditTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("TextureVTAuditTabLabel", "贴图 VT 审计"))
			[
				SNew(STextureVTAuditWidget)
			];
	}

	TSharedRef<SDockTab> SpawnVirtualTextureCollectionManagerTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("VTCManagerTabLabel", "VTC 管理器"))
			[
				SNew(SVirtualTextureCollectionManagerWidget)
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
							.Text(LOCTEXT("PerformanceToolsLauncherDescription", "常用性能入口集中在这里：从游戏主菜单运行、配置 PIE/Development 下 F12 Runtime GM、打开环境合批标记、模型/材质/贴图/VTC 检查也已经放到编辑器工具栏和 DevKit 工具菜单中。"))
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
								.Text(LOCTEXT("LauncherOpenRuntimeGMSettings", "运行时 GM 配置"))
								.ToolTipText(LOCTEXT("LauncherOpenRuntimeGMSettingsTooltip", "配置 PIE、Standalone 和 Development 包中 F12 Runtime GM 面板使用的武器、敌人、刷敌数量和快速性能测试打包入口。"))
								.OnClicked(FOnClicked::CreateRaw(this, &FDevKitEditorModule::HandleOpenRuntimeGMSettingsClicked))
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.f, 0.f, 8.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("LauncherOpenEnvBatchTagger", "环境合批标记"))
								.ToolTipText(LOCTEXT("LauncherOpenEnvBatchTaggerTooltip", "打开 EnvBatch.Source.<关卡>.<类型>.<方式>.<VTC组>.<流水号> Actor Tag 标记窗口。"))
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
							.Text(LOCTEXT("PerformanceToolsLauncherWorkflow", "推荐顺序：1. 检查模型、材质、贴图 VT 和 VTC；2. 使用环境合批标记给关卡对象写 EnvBatch.Source.* Actor Tag，其中 VTC-A/VTC-B 是共享贴图集合父组，流水号是模型 merge 组；3. 后续正式工具链再执行按通道拆 VTC、几何合并、代理和替换。"))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 10.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PerformanceToolsLauncherNote", "主菜单运行入口读取 YogGameInstanceBase.FrontendMap；当前项目默认是 /Game/Maps/L_EntryMenu。PIE 运行时按 F12 可打开/关闭 Runtime GM 面板。"))
							.AutoWrapText(true)
						]
					]
				]
			];
	}

	TSharedRef<SDockTab> SpawnRuntimeGMSettingsTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(LOCTEXT("RuntimeGMSettingsTabLabel", "运行时 GM 配置"))
			[
				SNew(SRuntimeGMSettingsWidget)
			];
	}

	void OnCombatLogTabClosed(TSharedRef<SDockTab> ClosedTab)
	{
		CombatLogWidgetInstance.Reset();
	}

	void RegisterDataEditorMenus()
	{
		RegisterRuntimeGMPIEInputProcessor();

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
			GetPlayFromMainMenuIcon()));
		QuickPlayEntry.ToolBarData.LabelOverride = FText::GetEmpty();

		UToolMenu* UserToolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
		FToolMenuSection& UserSection = UserToolbar->FindOrAddSection(TEXT("DevKitPerformanceTools"));
		FToolMenuEntry& LauncherEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitPerformanceToolsLauncher"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenPerformanceToolsLauncherTab)),
			LOCTEXT("OpenPerformanceToolsLauncherToolbarLabel", "性能工具"),
			LOCTEXT("OpenPerformanceToolsLauncherToolbarTooltip", "打开性能工具快捷入口。"),
			GetPerformanceToolsIcon()));
		LauncherEntry.ToolBarData.LabelOverride = LOCTEXT("OpenPerformanceToolsLauncherToolbarShortLabel", "性能工具");

		FToolMenuEntry& RuntimeGMEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitRuntimeGMSettings"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuntimeGMSettingsTab)),
			LOCTEXT("OpenRuntimeGMSettingsToolbarLabel", "Runtime GM"),
			LOCTEXT("OpenRuntimeGMSettingsToolbarTooltip", "打开运行时 GM 配置；PIE、Standalone 和 Development 包里按 F12 打开/关闭 GM 面板。"),
			GetPerformanceToolsIcon()));
		RuntimeGMEntry.ToolBarData.LabelOverride = LOCTEXT("OpenRuntimeGMSettingsToolbarShortLabel", "Runtime GM");

		FToolMenuEntry& ModelComplianceEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitModelAssetCompliance"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenModelAssetComplianceTab)),
			LOCTEXT("OpenModelAssetComplianceToolbarLabel", "模型合规"),
			LOCTEXT("OpenModelAssetComplianceToolbarTooltip", "打开模型资产合规检查。"),
			GetModelAssetComplianceIcon()));
		ModelComplianceEntry.ToolBarData.LabelOverride = LOCTEXT("OpenModelAssetComplianceToolbarShortLabel", "模型合规");

		FToolMenuEntry& TextureRulesEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitMaterialTextureRules"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenMaterialTextureRulesTab)),
			LOCTEXT("OpenMaterialTextureRulesToolbarLabel", "材质合规"),
			LOCTEXT("OpenMaterialTextureRulesToolbarTooltip", "打开材质合规检查，检查贴图规则和材质性能分级接口。"),
			GetMaterialTextureRulesIcon()));
		TextureRulesEntry.ToolBarData.LabelOverride = LOCTEXT("OpenMaterialTextureRulesToolbarShortLabel", "材质合规");

		FToolMenuEntry& TextureVTAuditEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitTextureVTAudit"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenTextureVTAuditTab)),
			LOCTEXT("OpenTextureVTAuditToolbarLabel", "贴图 VT"),
			LOCTEXT("OpenTextureVTAuditToolbarTooltip", "打开贴图 VT 审计，检查 VT 状态、尺寸、格式和 VTC 兼容性。"),
			GetTextureVTAuditIcon()));
		TextureVTAuditEntry.ToolBarData.LabelOverride = LOCTEXT("OpenTextureVTAuditToolbarShortLabel", "贴图 VT");

		FToolMenuEntry& VTCManagerEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitVTCManager"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenVirtualTextureCollectionManagerTab)),
			LOCTEXT("OpenVTCManagerToolbarLabel", "VTC"),
			LOCTEXT("OpenVTCManagerToolbarTooltip", "打开 VTC 管理器，管理 VirtualTextureCollection 的创建、成员和合规检查。"),
			GetVTCManagerIcon()));
		VTCManagerEntry.ToolBarData.LabelOverride = LOCTEXT("OpenVTCManagerToolbarShortLabel", "VTC");

		FToolMenuEntry& EnvBatchTaggerEntry = UserSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			TEXT("OpenDevKitEnvBatchTagger"),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenEnvBatchTaggerTab)),
			LOCTEXT("OpenEnvBatchTaggerToolbarLabel", "环境合批"),
			LOCTEXT("OpenEnvBatchTaggerToolbarTooltip", "打开环境合批 Actor 标记工具，写入 EnvBatch.Source.* 并按 tag 反选 Actor。"),
			GetEnvBatchTaggerIcon()));
		EnvBatchTaggerEntry.ToolBarData.LabelOverride = LOCTEXT("OpenEnvBatchTaggerToolbarShortLabel", "环境合批");

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
			GetModelAssetComplianceIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenModelAssetComplianceTab)));
		ArtAssetSection.AddMenuEntry(
			TEXT("OpenMaterialTextureRules"),
			LOCTEXT("OpenMaterialTextureRulesLabel", "材质合规检查"),
			LOCTEXT("OpenMaterialTextureRulesTooltip", "检查贴图命名、sRGB、尺寸、VT 建议和材质性能分级接口。"),
			GetMaterialTextureRulesIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenMaterialTextureRulesTab)));
		ArtAssetSection.AddMenuEntry(
			TEXT("OpenTextureVTAudit"),
			LOCTEXT("OpenTextureVTAuditLabel", "贴图 VT 审计"),
			LOCTEXT("OpenTextureVTAuditTooltip", "检查所有 Texture2D 的 VT 开启状态、尺寸、格式，以及是否可加入 VirtualTextureCollection。支持批量开关 VT。"),
			GetTextureVTAuditIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenTextureVTAuditTab)));
		ArtAssetSection.AddMenuEntry(
			TEXT("OpenVTCManager"),
			LOCTEXT("OpenVTCManagerLabel", "VTC 管理器"),
			LOCTEXT("OpenVTCManagerTooltip", "管理项目中所有 VirtualTextureCollection：新建、批量添加成员、合规校验。"),
			GetVTCManagerIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenVirtualTextureCollectionManagerTab)));

		FToolMenuSection& PerformanceSection = Menu->FindOrAddSection(TEXT("DevKitPerformanceTools"), LOCTEXT("DevKitPerformanceToolsSection", "性能工具"));
		PerformanceSection.AddMenuEntry(
			TEXT("OpenPerformanceToolsLauncher"),
			LOCTEXT("OpenPerformanceToolsLauncherLabel", "性能工具快捷入口"),
			LOCTEXT("OpenPerformanceToolsLauncherTooltip", "打开主菜单运行入口和关卡环境合批标记入口；模型、贴图资产检查在美术资产工具中打开。"),
			GetPerformanceToolsIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenPerformanceToolsLauncherTab)));
		PerformanceSection.AddMenuEntry(
			TEXT("DevKitPlayFromMainMenu"),
			LOCTEXT("OpenPlayFromMainMenuLabel", "从主菜单运行"),
			LOCTEXT("OpenPlayFromMainMenuTooltip", "打开入口地图并从游戏主菜单开始 Play。"),
			GetPlayFromMainMenuIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::PlayFromMainMenu)));
		PerformanceSection.AddMenuEntry(
			TEXT("OpenRuntimeGMSettings"),
			LOCTEXT("OpenRuntimeGMSettingsLabel", "运行时 GM 配置"),
			LOCTEXT("OpenRuntimeGMSettingsTooltip", "配置 F12 Runtime GM 面板使用的武器、敌人、数量、半径和快速打包入口。"),
			GetPerformanceToolsIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDevKitEditorModule::OpenRuntimeGMSettingsTab)));
		PerformanceSection.AddMenuEntry(
			TEXT("OpenEnvBatchTagger"),
			LOCTEXT("OpenEnvBatchTaggerLabel", "环境合批标记"),
			LOCTEXT("OpenEnvBatchTaggerTooltip", "给当前选中的关卡 Actor 写入 EnvBatch.Source.<关卡>.<类型>.<方式>.<VTC组>.<流水号>，并可按 tag 反选 Actor。"),
			GetEnvBatchTaggerIcon(),
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

	FReply HandleOpenRuntimeGMSettingsClicked()
	{
		OpenRuntimeGMSettingsTab();
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

	void OpenTextureVTAuditTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(TextureVTAuditTabName);
	}

	void OpenVirtualTextureCollectionManagerTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(VirtualTextureCollectionManagerTabName);
	}

	void OpenPerformanceToolsLauncherTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(PerformanceToolsLauncherTabName);
	}

	void OpenRuntimeGMSettingsTab()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(RuntimeGMSettingsTabName);
	}

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

private:
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;
	TStrongObjectPtr<UCombatLogEditorUtilityWidget> CombatLogWidgetInstance;
	TSharedPtr<IInputProcessor> RuntimeGMPIEInputProcessor;

	void RegisterRuntimeGMPIEInputProcessor()
	{
		if (RuntimeGMPIEInputProcessor.IsValid() || !FSlateApplication::IsInitialized())
		{
			return;
		}

		RuntimeGMPIEInputProcessor = MakeShared<FRuntimeGMPIEInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(RuntimeGMPIEInputProcessor);
	}

	void UnregisterRuntimeGMPIEInputProcessor()
	{
		if (!RuntimeGMPIEInputProcessor.IsValid() || !FSlateApplication::IsInitialized())
		{
			RuntimeGMPIEInputProcessor.Reset();
			return;
		}

		FSlateApplication::Get().UnregisterInputPreProcessor(RuntimeGMPIEInputProcessor);
		RuntimeGMPIEInputProcessor.Reset();
	}
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
