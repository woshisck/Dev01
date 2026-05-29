#include "WeaveLanguage.h"
#include "Slate/SWeaverDebugger.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/InputChord.h"
#include "ToolMenus.h"
#include "Core/WeaveOperator.h"

#define LOCTEXT_NAMESPACE "FWeaveLanguageModule"

void FWeaveLanguageModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWeaveLanguageModule::RegisterMenus));
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		if (GEditor)
		{
			FTimerHandle TimerHandle;
			GEditor->GetTimerManager()->SetTimer(TimerHandle, []()
			{
				UWeaveOperator::GenerateWeaveLanguage();
			}, 2.0f, false);
		}
	});
}

void FWeaveLanguageModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FWeaveLanguageModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Weaver");

	Section.AddSubMenu(
		"WeaverMenu",
		LOCTEXT("WeaverMenu", "Weaver"),
		LOCTEXT("WeaverMenuTooltip", "Weaver Plugin Menu"),
		FNewToolMenuDelegate::CreateRaw(this, &FWeaveLanguageModule::FillWeaverMenu)
	);
}

void FWeaveLanguageModule::FillWeaverMenu(UToolMenu* Menu)
{
	FToolMenuSection& Section = Menu->FindOrAddSection("WeaverActions");

	Section.AddMenuEntry(
		"WeaveDebugger",
		LOCTEXT("WeaveDebugger", "Open Weave Debugger"),
		LOCTEXT("WeaveDebuggerTooltip", "Generate Weave code from selected nodes or interpret Weave code."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FWeaveLanguageModule::OnOpenDebugger))
	);
}

void FWeaveLanguageModule::OnGenerateWeave()
{
	UWeaveOperator::GenerateWeaveLanguage();
}

void FWeaveLanguageModule::OnOpenDebugger()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Weave Debugger")))
		.ClientSize(FVector2D(800, 600))
		.SupportsMaximize(true)
		.SupportsMinimize(true)
		.IsTopmostWindow(true);

	Window->SetContent(SNew(SWeaverDebugger));

	FSlateApplication::Get().AddWindow(Window);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWeaveLanguageModule, WeaveLanguage)
