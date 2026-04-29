// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "ComboGraphEditorModule.h"

#include "AbilitySystemGlobals.h"
#include "AssetToolsModule.h"
#include "ComboGraphAbilityTypes.h"
#include "ComboGraphEditorLog.h"
#include "Abilities/ComboGraphAbilitySystemGlobals.h"
#include "AssetTypes/AssetTypeActions_ComboGraph.h"
#include "AssetTypes/AssetTypeActions_ComboGraphNodes.h"
#include "Editors/ComboGraphEditorStyle.h"
#include "Graph/Factories/ComboGraphPanelNodeFactory.h"
#include "Logging/MessageLog.h"
#include "Utils/ComboGraphUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ComboGraphEditorModule"

void FComboGraphEditorModule::StartupModule()
{
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - Startup Module"))

	// Init style
	FComboGraphEditorStyle::Get();

	// Visual node factory
	GraphNodeFactory = MakeShareable(new FComboGraphNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);

	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const EAssetTypeCategories::Type AssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("ComboGraph")),
		LOCTEXT("ComboGraphAssetCategory", "Combo Graph")
	);

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_ComboGraph(AssetCategory)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_ComboGraphNodeMontage(AssetCategory)));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_ComboGraphNodeSequence(AssetCategory)));

	// Register post engine delegate to handle init Ability System Global data initialization
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FComboGraphEditorModule::OnPostEngineInit);
}

void FComboGraphEditorModule::ShutdownModule()
{
	// Unregister asset type actions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& AssetTypeAction : CreatedAssetTypeActions)
		{
			if (AssetTypeAction.IsValid())
			{
				AssetToolsModule.UnregisterAssetTypeActions(AssetTypeAction.ToSharedRef());
			}
		}
	}
	CreatedAssetTypeActions.Empty();

	// Remove delegates
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	// Unregister node factories
	FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
}

void FComboGraphEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

void FComboGraphEditorModule::HandleAbilitySystemGlobalsSettings()
{
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - HandleAbilitySystemGlobalsSettings"))

	// What we really need to know is if the AbilitySystemGlobals is implementing AllocGameplayEffectContext and returns a valid context we expect for cues container,
	// which should be either FComboGraphGameplayEffectContext or a child struct of it

	// if (UComboGraphBlueprintLibrary::IsValidContext(UAbilitySystemGlobals::Get().AllocGameplayEffectContext()))
	// {
	// 	// We good
	// 	return;
	// }
	//
	// CG_EDITOR_LOG(Warning, TEXT("ComboGraphEditorModule - UAbilitySystemGlobals is not a UComboGraphAbilitySystemGlobals most likely. Returned Effect Context is of invalid type."))

	// Checking now for actual class to be either UComboGraphAbilitySystemGlobals or a child of it
	const UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	if (AbilitySystemGlobals.IsA(UComboGraphAbilitySystemGlobals::StaticClass()))
	{
		// We good
		return;
	}

	CG_EDITOR_LOG(Warning, TEXT("ComboGraphEditorModule - UAbilitySystemGlobals is not a UComboGraphAbilitySystemGlobals. It is required to subclass Effect Context for gameplay cues container."))

	// Check UAbilitySystemGlobals config
	const FSoftClassPath AbilitySystemGlobalsClassName = GetDefault<UAbilitySystemGlobals>()->AbilitySystemGlobalsClassName;
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - AbilitySystemGlobalsClassName: %s"), *AbilitySystemGlobalsClassName.ToString())

	const bool bIgnoreStartupValidationWarnings = FComboGraphUtils::GetPluginProjectSettings()->bIgnoreStartupValidationWarnings;

	// If this is still the default value, warn and add option for users to set it up to our custom subclass
	if (!bIgnoreStartupValidationWarnings && AbilitySystemGlobalsClassName.ToString() == "/Script/GameplayAbilities.AbilitySystemGlobals")
	{
		CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - AbilitySystemGlobalsClassName is default, handle message log validation."), *AbilitySystemGlobalsClassName.ToString())

		const FText MessageEntry = LOCTEXT("MissingRuleForAbilitySystemGlobals", "Combo Graph: AbilitySystemGlobals settings do not include an entry for class of type {0} which is required for Gameplay Cues Container (custom Effect Context).");
		const FText TokenText = LOCTEXT("AddRuleForAbilitySystemGlobalsAction", "Set AbilitySystemGlobalsClassName to ComboGraph.ComboGraphAbilitySystemGlobals (recommended)?");

		FMessageLog("LoadErrors").Error()->AddToken(FTextToken::Create(FText::Format(
			MessageEntry,
			FText::FromName(UComboGraphAbilitySystemGlobals::StaticClass()->GetFName())
		)));

		FMessageLog("LoadErrors").Error()
		->AddToken(FTextToken::Create(LOCTEXT("AddRuleForAbilitySystemGlobalsLabel", "Click the link to add en entry to DefaultGame.ini file:")))
		->AddToken(FActionToken::Create(
			TokenText,
			FText(),
			FOnActionTokenExecuted::CreateRaw(this, &FComboGraphEditorModule::AddAbilitySystemGlobalsClassName), true
		));
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FComboGraphEditorModule::AddAbilitySystemGlobalsClassName()
{
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - AddAbilitySystemGlobalsClassName token handler"))
	UAbilitySystemGlobals* AbilitySystemGlobals = GetMutableDefault<UAbilitySystemGlobals>();
	if (!AbilitySystemGlobals)
	{
		return;
	}

	AbilitySystemGlobals->AbilitySystemGlobalsClassName = UComboGraphAbilitySystemGlobals::StaticClass();
	const FString DefaultConfigFilename = AbilitySystemGlobals->GetDefaultConfigFilename();
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - Should write to %s"), *DefaultConfigFilename)

	AbilitySystemGlobals->UpdateSinglePropertyInConfigFile(AbilitySystemGlobals->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAbilitySystemGlobals, AbilitySystemGlobalsClassName)), DefaultConfigFilename);

	// Notify user
	FNotificationInfo Info(LOCTEXT(
		"SettingsAbilitySystemGlobalsClassUpdated",
		"AbilitySystemGlobals Class Name has been updated to use ComboGraphAbilitySystemGlobals.\n\n"
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
}

void FComboGraphEditorModule::OnPostEngineInit()
{
	CG_EDITOR_LOG(Verbose, TEXT("ComboGraphEditorModule - OnPostEngineInit"))
	// Check for configuration to get a warning on startup if settings aren't configured correctly (for AbilitySystemGlobals class name)
	HandleAbilitySystemGlobalsSettings();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FComboGraphEditorModule, ComboGraphEditor)
