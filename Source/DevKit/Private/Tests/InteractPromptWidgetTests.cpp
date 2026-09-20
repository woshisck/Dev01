#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Character/YogPlayerControllerBase.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractHoldProgressTest,
	"DevKit.Interact.HoldProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractHoldProgressTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(
		TEXT("Hold starts empty"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, 0.6f),
		0.f);

	TestEqual(
		TEXT("Half the duration fills half the bar"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.3f, 0.6f),
		0.5f);

	TestEqual(
		TEXT("Reaching the duration completes the hold"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.6f, 0.6f),
		1.f);

	TestEqual(
		TEXT("Overshooting the duration clamps to full"),
		AYogPlayerControllerBase::ComputeHoldProgress(5.f, 0.6f),
		1.f);

	// A zero duration is how a target opts back into instant interaction.
	TestEqual(
		TEXT("Zero duration completes immediately"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, 0.f),
		1.f);

	TestEqual(
		TEXT("Negative duration completes immediately"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, -1.f),
		1.f);

	TestEqual(
		TEXT("Negative elapsed clamps to empty"),
		AYogPlayerControllerBase::ComputeHoldProgress(-1.f, 0.6f),
		0.f);

	return true;
}

#endif
