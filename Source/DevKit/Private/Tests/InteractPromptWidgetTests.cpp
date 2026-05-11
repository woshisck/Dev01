#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/InteractPromptWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractPromptWidgetMarkupTest,
	"DevKit.UI.InteractPromptWidget.UsesCommonInteractAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractPromptWidgetMarkupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(
		TEXT("Prompt markup uses the shared Interact action token"),
		UInteractPromptWidget::MakePromptMarkup(FText::FromString(TEXT("Open"))).ToString(),
		FString(TEXT("<input action=\"Interact\"/> Open")));

	TestEqual(
		TEXT("Empty prompt still renders the shared Interact action token"),
		UInteractPromptWidget::MakePromptMarkup(FText::GetEmpty()).ToString(),
		FString(TEXT("<input action=\"Interact\"/>")));

	return true;
}

#endif
