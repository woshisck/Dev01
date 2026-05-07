#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/CombatLogEditorUtilityWidget.h"

#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "Layout/Children.h"
#include "Widgets/SWidget.h"

namespace
{
bool ContainsSlateWidgetType(const TSharedRef<SWidget>& Widget, const FName TypeName)
{
	if (Widget->GetType() == TypeName)
	{
		return true;
	}

	if (FChildren* Children = Widget->GetAllChildren())
	{
		for (int32 Index = 0; Index < Children->Num(); ++Index)
		{
			if (ContainsSlateWidgetType(Children->GetChildAt(Index), TypeName))
			{
				return true;
			}
		}
	}

	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogEditorUtilityNativeWidgetBootTest,
	"DevKit.UI.CombatLog.EditorWindow.NativeWidgetBoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogEditorUtilityNativeWidgetBootTest::RunTest(const FString& Parameters)
{
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("Editor world is available"), EditorWorld))
	{
		return false;
	}

	UCombatLogEditorUtilityWidget* Widget = CreateWidget<UCombatLogEditorUtilityWidget>(
		EditorWorld,
		UCombatLogEditorUtilityWidget::StaticClass());
	if (!TestNotNull(TEXT("Native combat log widget can be created directly"), Widget))
	{
		return false;
	}

	const TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	TestTrue(TEXT("First Slate build contains the generated log scroll box"),
		ContainsSlateWidgetType(SlateWidget, FName(TEXT("SScrollBox"))));

	return true;
}

#endif
