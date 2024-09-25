// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GASCompanionEditorModule.h"

#include "AssetToolsModule.h"
#include "GSC.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#include "SourceCodeNavigation.h"
#include "ToolMenus.h"
#include "Core/Common/GSCAttributesWizardCommands.h"
#include "Core/Logging/GASCompanionEditorLog.h"
#include "CreationMenu/GSCGameplayAbilityCreationMenu.h"
#include "CreationMenu/GSCGameplayEffectCreationMenu.h"
#include "CreationMenu/GSCGameplayEffectTemplateDetails.h"
#include "Interfaces/IMainFrameModule.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UI/Widgets/SGSCNewAttributeSetClassDialog.h"

#define LOCTEXT_NAMESPACE "GASCompanionEditor"

void FGASCompanionEditorModule::StartupModule()
{
	InitializeStyles();
	RegisterCommands();

	EDITOR_LOG(Display, TEXT("FGASCompanionEditorModule StartupModule"))

	// Create an editor interface, so the runtime module can access it

	PluginCommands = MakeShareable(new FUICommandList);

	// Register the details customization
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		RegisterDetailsCustomization<FGSCGameplayEffectTemplateDetails>("GSCGameplayEffectTemplate", PropertyModule);
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// Register custom project settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			SettingsContainerName,
			SettingsCategoryName,
			SettingsGameplayEffectsSectionName,
			NSLOCTEXT("GASCompanionEditor", "GameplayEffectParentName", "Gameplay Effect Definitions"),
			NSLOCTEXT("GASCompanionEditor", "GameplayEffectParentNameDesc", "Data Driven way of specifying common parent Gameplay Effect classes that are accessible through File menu"),
			GetMutableDefault<UGSCGameplayEffectCreationMenu>()
		);

		GetDefault<UGSCGameplayEffectCreationMenu>()->AddMenuExtensions();

		SettingsModule->RegisterSettings(
			SettingsContainerName,
			SettingsCategoryName,
			SettingsGameplayAbilitiesSectionName,
			NSLOCTEXT("GASCompanionEditor", "GameplayAbilityParentName", "Gameplay Abilities Definitions"),
			NSLOCTEXT("GASCompanionEditor", "GameplayAbilityParentNameDesc", "Data Driven way of specifying common parent Gameplay Ability classes that are accessible through File menu"),
			GetMutableDefault<UGSCGameplayAbilityCreationMenu>()
		);

		GetDefault<UGSCGameplayAbilityCreationMenu>()->AddMenuExtensions();
	}

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGASCompanionEditorModule::RegisterMenus));
}

void FGASCompanionEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	ShutdownStyle();
	UnregisterCommands();

	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().Remove(MapChangedHandle);

	// Unregister property editor customizations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		UnregisterDetailsCustomization(PropertyModule);
	}

	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->UnregisterSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayEffectsSectionName);
		SettingsModule->UnregisterSettings(SettingsContainerName, SettingsCategoryName, SettingsGameplayAbilitiesSectionName);
	}
}

void FGASCompanionEditorModule::RegisterCommands()
{
	FGSCAttributesWizardCommands::Register();
}

void FGASCompanionEditorModule::UnregisterCommands()
{
	FGSCAttributesWizardCommands::Unregister();
}

void FGASCompanionEditorModule::InitializeStyles()
{
	FGSCEditorStyle::Initialize();
	FGSCEditorStyle::ReloadTextures();
}

void FGASCompanionEditorModule::ShutdownStyle()
{
	FGSCEditorStyle::Shutdown();
}

void FGASCompanionEditorModule::OpenAddToProjectDialog(const FAddToProjectConfig& Config)
{
	// If we've been given a class then we only show the second page of the dialog, so we can make the window smaller as that page doesn't have as much content
	const FVector2D WindowSize = FVector2D(940, 720);

	FText WindowTitle = Config._WindowTitle;
	if (WindowTitle.IsEmpty())
	{
		WindowTitle = LOCTEXT("AddCodeWindowHeader_Native", "Add AttributeSet C++ Class");
	}

	TSharedRef<SWindow> AddCodeWindow =
		SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(WindowSize)
		.SizingRule(ESizingRule::UserSized)
		.MinWidth(WindowSize.X)
		.MinHeight(WindowSize.Y)
		.SupportsMinimize(false)
		.SupportsMaximize(true);

	const TSharedRef<SGSCNewAttributeSetClassDialog> NewClassDialog =
		SNew(SGSCNewAttributeSetClassDialog)
		.ParentWindow(AddCodeWindow)
		.InitialPath(Config._InitialPath)
		.OnAddedToProject(Config._OnAddedToProject)
		.DefaultClassPrefix(Config._DefaultClassPrefix)
		.DefaultClassName(Config._DefaultClassName);

	AddCodeWindow->SetContent(NewClassDialog);

	TSharedPtr<SWindow> ParentWindow = Config._ParentWindow;
	if (!ParentWindow.IsValid())
	{
		static const FName MainFrameModuleName = "MainFrame";
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(MainFrameModuleName);
		ParentWindow = MainFrameModule.GetParentWindow();
	}

	if (Config._bModal)
	{
		FSlateApplication::Get().AddModalWindow(AddCodeWindow, ParentWindow);
	}
	else if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(AddCodeWindow, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(AddCodeWindow);
	}
}

EAssetTypeCategories::Type FGASCompanionEditorModule::GetAssetTypeCategory() const
{
	return AssetTypeCategory;
}

void FGASCompanionEditorModule::UnregisterDetailsCustomization(FPropertyEditorModule& PropertyEditorModule)
{
	for (const FName& ClassName : RegisteredClassCustomizations)
	{
		PropertyEditorModule.UnregisterCustomClassLayout(ClassName);
	}
	RegisteredClassCustomizations.Reset();
}

TSharedRef<SWidget> FGASCompanionEditorModule::GenerateToolbarSettingsMenu(TSharedRef<FUICommandList> InCommandList)
{
	// Get all menu extenders for this context menu from the level editor module
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	const TSharedPtr<FExtender> MenuExtender = LevelEditorModule.AssembleExtenders(InCommandList, LevelEditorModule.GetAllLevelEditorToolbarViewMenuExtenders());

	const FToolMenuContext MenuContext(InCommandList, MenuExtender);
	return UToolMenus::Get()->GenerateWidget("LevelEditor.LevelEditorToolBar.GASCompanionEditor.ComboMenu", MenuContext);
}

void FGASCompanionEditorModule::RegisterMenus()
{
	RegisterComboMenus();

	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		// UToolMenu * Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		// FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
		// Section.AddMenuEntryWithCommandList(FGSCAttributesWizardCommands::Get().OpenPluginWindow, PluginCommands);
	}

	{
#if ENGINE_MAJOR_VERSION < 5
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
		const FSlateIcon SlateIcon = FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "GASCompanionEditor.OpenPluginWindow");
		FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitComboButton(
			"LevelToolbarGASCompanionSettings",
			FUIAction(),
			FOnGetContent::CreateStatic(&FGASCompanionEditorModule::GenerateToolbarSettingsMenu, PluginCommands.ToSharedRef()),
			LOCTEXT("QuickSettingsCombo", "GAS Companion"),
			LOCTEXT("QuickSettingsCombo_ToolTip", "GAS Companion"),
			SlateIcon,
			false
		));
		Entry.SetCommandList(PluginCommands);
#else
		UToolMenu* SettingsToolbar = UToolMenus::Get()->RegisterMenu("LevelEditor.LevelEditorToolBar.SettingsToolbar", NAME_None, EMultiBoxType::SlimHorizontalToolBar);
		SettingsToolbar->StyleName = "AssetEditorToolbar";
		{
			FToolMenuSection& SettingsSection = SettingsToolbar->FindOrAddSection("GASCompanion");
			FToolMenuEntry SettingsEntry = FToolMenuEntry::InitComboButton(
				"LevelToolbarGASCompanionSettings",
				FUIAction(),
				FOnGetContent::CreateStatic(&FGASCompanionEditorModule::GenerateToolbarSettingsMenu, PluginCommands.ToSharedRef()),
				LOCTEXT("QuickSettingsCombo", "GAS Companion"),
				LOCTEXT("QuickSettingsCombo_ToolTip", "GAS Companion settings & tooling"),
				FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "GASCompanionEditor.OpenPluginWindow"),
				false
			);
			SettingsEntry.StyleNameOverride = "CalloutToolbar";
			SettingsSection.AddEntry(SettingsEntry);
		}
#endif
	}
}

void FGASCompanionEditorModule::RegisterComboMenus() const
{
	UToolMenu* Menu = UToolMenus::Get()->RegisterMenu("LevelEditor.LevelEditorToolBar.GASCompanionEditor.ComboMenu");

	struct Local
	{
		static void OpenSettings(const FName ContainerName, const FName CategoryName, const FName SectionName)
		{
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(ContainerName, CategoryName, SectionName);
		}

		static void OpenClassWizard()
		{
			OpenAddToProjectDialog();
		}

		static void OpenDocumentation()
		{
			const FString URL = FGSCModule::GetDocumentationURL();
			FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
		}

		static void OpenMailSupport()
		{
			FPlatformProcess::LaunchURL(TEXT("mailto:daniel.mickael@gmail.com?subject=GAS Companion Support"), nullptr, nullptr);
		}

		static void OpenDiscord()
		{
			FPlatformProcess::LaunchURL(TEXT("https://discord.gg/d4rs4vcX6t"), nullptr, nullptr);
		}
	};

	{
		FToolMenuSection& Section = Menu->AddSection("ProjectSettingsSection", LOCTEXT("ProjectSettingsSection", "Configuration"));

		Section.AddMenuEntry(
			"GASCompanionSettings",
			LOCTEXT("ProjectSettingsMenuLabel", "GAS Companion Settings..."),
			LOCTEXT("ProjectSettingsMenuToolTip", "Change GAS Companion settings of the currently loaded project"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenSettings, SettingsContainerName, SettingsCategoryName, SettingsAttributeSetSectionName))
		);

		Section.AddMenuEntry(
			"GASCompanionGameplayAbilitySettings",
			LOCTEXT("ProjectGameplayAbilitiesSettingsMenuLabel", "Gameplay Ability Definitions..."),
			LOCTEXT("ProjectGameplayAbilitiesSettingsMenuToolTip", "Data Driven way of specifying common parent Gameplay Ability classes that are accessible through File menu"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenSettings, SettingsContainerName, SettingsCategoryName, SettingsGameplayAbilitiesSectionName))
		);

		Section.AddMenuEntry(
			"GASCompanionGameplayEffectsSettings",
			LOCTEXT("ProjectGameplayEffectsSettingsMenuLabel", "Gameplay Effect Definitions..."),
			LOCTEXT("ProjectGameplayEffectsSettingsMenuToolTip", "Data Driven way of specifying common parent Gameplay Effect classes that are accessible through File menu"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenSettings, SettingsContainerName, SettingsCategoryName, SettingsGameplayEffectsSectionName))
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection("ProjectSection", LOCTEXT("LaunchPadHeading", "Launch Pad"));

		const FText TooltipText = LOCTEXT("LaunchPadMenuToolTip", "Browse GAS Companion Examples - Launch Pad is disabled for now. It'll be reworked and added back in a future release.");

		FToolMenuEntry Entry = Section.AddMenuEntry(
			"GASCompanionSettings",
			LOCTEXT("LaunchPadMenuLabel", "Launch Pad Window"),
			TooltipText,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon"),
			FUIAction(
				FExecuteAction(),
				FCanExecuteAction::CreateLambda([] { return false; })
			)
		);
	}

	{
		const FText ShortIDEName = FSourceCodeNavigation::GetSelectedSourceCodeIDE();
		FToolMenuSection& Section = Menu->AddSection("ProjectSection", LOCTEXT("ProjectHeading", "Project"));
		Section.AddMenuEntry(
			"GASCompanionNewAttributeSetClass",
			LOCTEXT("AddAttributeSetClass", "New C++ AttributeSet Class..."),
			FText::Format(LOCTEXT("AddAttributeSetClassTooltip", "Adds C++ AttributeSet code to the project. The code can only be compiled if you have {0} installed."), ShortIDEName),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenClassWizard))
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection("Documentation", LOCTEXT("DocumentationHeading", "Documentation"));
		Section.AddMenuEntry(
			"GASCompanionViewDocumentation",
			LOCTEXT("GASCompanionDocumentation", "GAS Companion Documentation"),
			LOCTEXT("GASCompanionDocumentationToolTip", "View online documentation"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.BrowseDocumentation"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenDocumentation))
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection("Support", LOCTEXT("SupportHeading", "Support"));
		Section.AddMenuEntry(
			"GASCompanionDiscord",
			LOCTEXT("GASCompanionDiscord", "Discord Server"),
			LOCTEXT("GASCompanionDiscordToolTip", "Join the Discord Server for support or chat with the community of GAS Companion users"),
			FSlateIcon(FGSCEditorStyle::GetStyleSetName(), "Icons.Discord"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenDiscord))
		);

		Section.AddMenuEntry(
			"GASCompanionMailSupport",
			LOCTEXT("GASCompanionSupport", "Email support"),
			LOCTEXT("GASCompanionSupportToolTip", "Email the dev: daniel.mickael@gmail.com"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Contact"),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenMailSupport))
		);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGASCompanionEditorModule, GASCompanionEditor)
