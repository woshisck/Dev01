#include "CelesLightEditorModule.h"

#include "Actors/CelesLightCaptureBox.h"
#include "CelesLightCaptureBoxDetails.h"
#include "CelesLightEditorLibrary.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FCelesLightEditorModule"

void FCelesLightEditorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCelesLightEditorModule::RegisterMenus));

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	PropertyEditorModule.RegisterCustomClassLayout(
		ACelesLightCaptureBox::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FCelesLightCaptureBoxDetails::MakeInstance));
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

void FCelesLightEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditorModule.UnregisterCustomClassLayout(ACelesLightCaptureBox::StaticClass()->GetFName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

void FCelesLightEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("CelesLight"), LOCTEXT("CelesLightSection", "赛璐璐灯光"));
	Section.AddSubMenu(
		TEXT("CelesLightCaptureTools"),
		LOCTEXT("CelesLightCaptureToolsLabel", "Celes Light 灯光采集"),
		LOCTEXT("CelesLightCaptureToolsTooltip", "创建采集盒体、创建 Celes 灯光，并把盒体范围内灯光写入 RT。插件不会创建动态材质。"),
		FNewToolMenuDelegate::CreateRaw(this, &FCelesLightEditorModule::FillCelesLightMenu),
		false,
		FSlateIcon());
}

void FCelesLightEditorModule::FillCelesLightMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("CelesLightCapture"), LOCTEXT("CelesLightCaptureSection", "灯光采集"));
	Section.AddMenuEntry(
		TEXT("CelesLightCreateCaptureBox"),
		LOCTEXT("CelesLightCreateCaptureBoxLabel", "创建灯光采集盒体"),
		LOCTEXT("CelesLightCreateCaptureBoxTooltip", "在当前关卡中创建 Celes Light Capture Box。盒体负责把范围内灯光写入指定 RT。"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCelesLightEditorModule::CreateCaptureBox)));

	Section.AddMenuEntry(
		TEXT("CelesLightCreatePointLight"),
		LOCTEXT("CelesLightCreatePointLightLabel", "创建 Celes 灯光"),
		LOCTEXT("CelesLightCreatePointLightTooltip", "在当前关卡中创建可被采集盒体记录的 Celes Point Light。"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCelesLightEditorModule::CreatePointLight)));

	Section.AddMenuEntry(
		TEXT("CelesLightCreateStylizedEmissiveLight"),
		LOCTEXT("CelesLightCreateStylizedEmissiveLightLabel", "Create Stylized Emissive Light"),
		LOCTEXT("CelesLightCreateStylizedEmissiveLightTooltip", "Create an invisible-in-game light source. It has no visible mesh and is stylized by the global lighting settings."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCelesLightEditorModule::CreateStylizedEmissiveLight)));

	Section.AddMenuEntry(
		TEXT("CelesLightOpenStylizedLightingSettings"),
		LOCTEXT("CelesLightOpenStylizedLightingSettingsLabel", "Stylized Lighting Settings"),
		LOCTEXT("CelesLightOpenStylizedLightingSettingsTooltip", "Open artist-facing controls for global stylized Lumen lighting."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCelesLightEditorModule::OpenStylizedLightingSettings)));

	Section.AddMenuEntry(
		TEXT("CelesLightManualUpdate"),
		LOCTEXT("CelesLightManualUpdateLabel", "更新全部采集盒体"),
		LOCTEXT("CelesLightManualUpdateTooltip", "手动更新当前关卡中所有 Celes Light Capture Box 的 Light Info RT。"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCelesLightEditorModule::ManualUpdateCelesLights)));
}

void FCelesLightEditorModule::CreateCaptureBox()
{
	UCelesLightEditorLibrary::CreateCelesLightCaptureBox(nullptr);
}

void FCelesLightEditorModule::CreatePointLight()
{
	UCelesLightEditorLibrary::CreateCelesPointLight(nullptr);
}

void FCelesLightEditorModule::CreateStylizedEmissiveLight()
{
	UCelesLightEditorLibrary::CreateStylizedEmissiveLight(nullptr);
}

void FCelesLightEditorModule::OpenStylizedLightingSettings()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("Stylized Lighting"));
}

void FCelesLightEditorModule::ManualUpdateCelesLights()
{
	UCelesLightEditorLibrary::ManualUpdateCelesLights(nullptr);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCelesLightEditorModule, CelesLightEditor)
