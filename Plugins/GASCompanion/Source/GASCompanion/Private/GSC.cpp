// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GSC.h"

#include "AbilitySystemGlobals.h"
#include "GSCAssetManager.h"
#include "Core/Settings/GSCDeveloperSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "GSCLog.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#define LOCTEXT_NAMESPACE "FGSCModule"

void FGSCModule::StartupModule()
{
	GSC_LOG(Display, TEXT("Startup GASCompanionModule Module"));

	// Register post engine delegate to handle init Ability System Global data initialization
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FGSCModule::OnPostEngineInit);

#if WITH_EDITOR
	// Register custom project settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		const FText SettingsDescription = LOCTEXT("SettingsDescription", "General Settings for GAS Companion Plugin and Ability System Globals configuration.");
		const TSharedPtr<ISettingsSection> SettingsSection = SettingsModule->RegisterSettings(
			"Project",
			"GAS Companion",
			"GASCompanion_AbilitySystemGlobals",
			LOCTEXT("DeveloperSettingsName", "Ability System Globals"),
			SettingsDescription,
			GetMutableDefault<UGSCDeveloperSettings>()
		);

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FGSCModule::HandleSettingsSaved);
		}
	}
#endif
}

void FGSCModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	GSC_LOG(Display, TEXT("Shutdown GSC module"));

	// Remove delegates
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

#if WITH_EDITOR
	// unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->UnregisterSettings("Project", "GAS Companion", "GASCompanion_AbilitySystemGlobals");
	}
#endif
}

FString FGSCModule::GetDocumentationURL(FString InPath)
{
	static FString DocumentationURL = "https://gascompanion.github.io";
	// static FString DocumentationURL = "https://gascompanion-dev.netlify.app";
	if (InPath.IsEmpty())
	{
		return DocumentationURL;
	}

	if (!InPath.StartsWith("/"))
	{
		InPath = "/" + InPath;
	}

	return DocumentationURL + InPath;
}

bool FGSCModule::HandleSettingsSaved()
{
#if WITH_EDITOR
	const UGSCDeveloperSettings* Settings = GetDefault<UGSCDeveloperSettings>();

	// You can put any validation code in here and resave the settings in case an invalid value has been entered

	if (Settings->bPreventGlobalDataInitialization)
	{
		const UEngine* EngineSettings = GetDefault<UEngine>();
		GSC_LOG(Verbose, TEXT("FGSCModule::HandleSettingsSaved - Current AssetManagerClassName: %s"), *EngineSettings->AssetManagerClassName.ToString())
		if (EngineSettings->AssetManagerClassName.ToString() == "/Script/Engine.AssetManager")
		{
			FNotificationInfo Info(LOCTEXT(
				"SettingsHasPreventGlobalDataInitialization",
				"If you disable Ability System Globals initialization in GAS Companion module, it is recommended to use a\n"
				"game-specific AssetManager to handle it and the current Asset Manager is set to the Engine's default class.\n"
				"\n"
				"Would you like to update Asset Manager Class Name to use GSCAssetManager?\n"
				"\n"
				"(Clicking the link below will update your DefaultEngine.ini settings and may require an editor restart.)\n\n"
			));

			Info.FadeInDuration = 0.2f;
			Info.ExpireDuration = 15.0f;
			Info.FadeOutDuration = 1.0f;
			Info.bUseThrobber = false;
			Info.bUseLargeFont = false;
			Info.WidthOverride = 540.f;

			Info.Hyperlink = FSimpleDelegate::CreateRaw(this, &FGSCModule::UpdateAssetManagerClass);
			Info.HyperlinkText = LOCTEXT("SettingsNotifLinkText", "Update Asset Manager Class to use GSCAssetManager ?");

			SettingsNotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
			if (SettingsNotificationItem.IsValid())
			{
				SettingsNotificationItem->SetCompletionState(SNotificationItem::CS_None);
			}
		}
	}
#endif

	return true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FGSCModule::UpdateAssetManagerClass()
{
#if WITH_EDITOR
	GSC_LOG(Warning, TEXT("Update asset manager class ..."))

	// Update engine settings for Asset Manager Class name
	UEngine* EngineSettings = GetMutableDefault<UEngine>();
	EngineSettings->AssetManagerClassName = UGSCAssetManager::StaticClass();

	const FString DefaultConfigFilename = EngineSettings->GetDefaultConfigFilename();
	GSC_LOG(Verbose, TEXT("Update asset manager class - Save settings to %s"), *DefaultConfigFilename)
	EngineSettings->UpdateSinglePropertyInConfigFile(EngineSettings->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEngine, AssetManagerClassName)), DefaultConfigFilename);

	// Notify user
	FNotificationInfo Info(LOCTEXT(
		"SettingsAssetManagerClassUpdated",
		"Asset Manager Class Name has been updated to use GSCAssetManager.\n\n"
		"You may need to restart the editor for it to be active."
	));

	Info.FadeInDuration = 0.2f;
	Info.ExpireDuration = 3.0f;
	Info.FadeOutDuration = 1.0f;
	Info.bUseThrobber = false;
	Info.bUseLargeFont = false;

	const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_None);
	}

	// Fadeout previous notif
	if (SettingsNotificationItem.IsValid())
	{
		SettingsNotificationItem->Fadeout();
	}
#endif

}

void FGSCModule::OnPostEngineInit()
{
	const UGSCDeveloperSettings* Settings = GetDefault<UGSCDeveloperSettings>();
	if (Settings->bPreventGlobalDataInitialization)
	{
		// Configuration is explicitly set to prevent initialization of Ability System Globals, do nothing
		return;
	}

	// UAbilitySystemGlobals' IsAbilitySystemGlobalsInitialized doesn't really work as it's checking for GlobalAttributeSetInitter and
	// GlobalAttributeSetDefaultsTableNames needs to be configured for AllocAttributeSetInitter to be called (at least for 4.26)

	const TArray<UScriptStruct*> StructsCache = UAbilitySystemGlobals::Get().TargetDataStructCache.ScriptStructs;
	const bool bStructsCacheInitialized = StructsCache.Num() > 0;

	GSC_LOG(
		Verbose,
		TEXT("GSCModule::PostEngineInit - AbilitySystemGlobals Initialized: %s (Num: %d)"),
		bStructsCacheInitialized ? TEXT("true") : TEXT("false"),
		StructsCache.Num()
	)

	GSC_LOG(
		Verbose,
		TEXT("GSCModule::PostEngineInit - AbilitySystemGlobals IsAbilitySystemGlobalsInitialized() returns %s"),
		UAbilitySystemGlobals::Get().IsAbilitySystemGlobalsInitialized() ? TEXT("true") : TEXT("false")
	)

	if (!bStructsCacheInitialized)
	{
		GSC_LOG(Display, TEXT("GSCModule - AbilitySystemGlobals is not initialized. Initializing now."))
		UAbilitySystemGlobals::Get().InitGlobalData();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGSCModule, GASCompanion)
