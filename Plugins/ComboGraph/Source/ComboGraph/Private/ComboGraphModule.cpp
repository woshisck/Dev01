// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "ComboGraphModule.h"

#include "AbilitySystemGlobals.h"
#include "ComboGraphLog.h"
#include "AnimNotifies/ComboGraphANS_ComboWindow.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Settings/ComboGraphProjectSettings.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/CoreDelegates.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#define LOCTEXT_NAMESPACE "ComboGraphModule"

void FComboGraphModule::StartupModule()
{
	// Register Settings
	SetupSettings();

	// Register post engine delegate to handle various checks related to user settings
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FComboGraphModule::OnPostEngineInit);
}

void FComboGraphModule::ShutdownModule()
{
	// Unregister Settings
	ShutdownSettings();

	// Remove delegates
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
}

void FComboGraphModule::SetupSettings()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		const TSharedPtr<ISettingsSection> SettingsSection = SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"AbilityCombatEditor",
			LOCTEXT("ProjectSettingsName", "Combo Graph"),
			LOCTEXT("ProjectSettingsDescription", "Configure Combo Graph Plugin."),
			GetMutableDefault<UComboGraphProjectSettings>()
		);

		// Register the save handler to your settings, you might want to use it to
		// validate those or just act to settings changes.
		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FComboGraphModule::HandleSettingsSaved);
		}
	}
#endif
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphModule::ShutdownSettings()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "AbilityCombatEditor");
	}
#endif
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool FComboGraphModule::HandleSettingsSaved()
{

#if WITH_EDITOR
	UComboGraphProjectSettings* Settings = GetMutableDefault<UComboGraphProjectSettings>();

	// You can put any validation code in here and resave the settings in case an invalid value has been entered
	const bool bValid = Settings->NotifyStates.Contains(UComboGraphANS_ComboWindow::StaticClass());
	if (!bValid)
	{
		Settings->NotifyStates.Add(UComboGraphANS_ComboWindow::StaticClass(), FComboGraphNotifyStateAutoSetup(0.25f, 0.75f));
		Settings->SaveConfig();

		FNotificationInfo Info(LOCTEXT("SettingsHasNoComboWindow", "Notifies State Auto Setup must include Combo Window.\n\nSettings has been updated to include it with default value."));
		// Info.Image = nullptr;
		Info.FadeInDuration = 0.2f;
		Info.ExpireDuration = 3.0f;
		Info.FadeOutDuration = 1.0f;
		Info.bUseThrobber = false;
		Info.bUseLargeFont = false;
		const TSharedPtr<SNotificationItem> Notif = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notif.IsValid())
		{
			Notif->SetCompletionState(SNotificationItem::CS_None);
		}
	}
#endif

	return true;
}

void FComboGraphModule::OnPostEngineInit()
{
	// UAbilitySystemGlobals' IsAbilitySystemGlobalsInitialized doesn't really work as it's checking for GlobalAttributeSetInitter and
	// GlobalAttributeSetDefaultsTableNames needs to be configured for AllocAttributeSetInitter to be called (at least for 4.26)

	const TArray<UScriptStruct*> StructsCache = UAbilitySystemGlobals::Get().TargetDataStructCache.ScriptStructs;
	const bool bStructsCacheInitialized = StructsCache.Num() > 0;
	CG_RUNTIME_LOG(Verbose, TEXT("ComboGraphModule::PostEngineInit - AbilitySystemGlobals Initialized: %s (Num: %d)"), *GetBoolText(bStructsCacheInitialized), StructsCache.Num())
	CG_RUNTIME_LOG(Verbose, TEXT("ComboGraphModule::PostEngineInit - AbilitySystemGlobals IsAbilitySystemGlobalsInitialized() returns %s"), *GetBoolText(UAbilitySystemGlobals::Get().IsAbilitySystemGlobalsInitialized()))

	if (!bStructsCacheInitialized)
	{
		CG_RUNTIME_LOG(Display, TEXT("ComboGraphModule - AbilitySystemGlobals is not initialized. Initializing now."))
		UAbilitySystemGlobals::Get().InitGlobalData();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FComboGraphModule, ComboGraph)
