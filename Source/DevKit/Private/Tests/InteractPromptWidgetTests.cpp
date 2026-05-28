#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/InteractPromptWidget.h"
#include "UI/RuneRewardFloatWidget.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRuneRewardFloatPromptPulseScaleTest,
	"DevKit.UI.RuneRewardFloatWidget.PromptPulseScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuneRewardFloatPromptPulseScaleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(
		TEXT("Prompt pulse starts above base scale"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(0.f, 2.f) > 1.f);

	TestEqual(
		TEXT("Prompt pulse returns to base scale at the end"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(2.f, 2.f),
		1.f);

	TestEqual(
		TEXT("Prompt pulse clamps elapsed time beyond duration"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(4.f, 2.f),
		1.f);

	return true;
}

#endif
