#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/YogSlotSelectWidgetBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlotSelectWidgetContractTest,
	"DevKit.UI.SlotSelectWidget.ExposesDesignerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlotSelectWidgetContractTest::RunTest(const FString& Parameters)
{
	const UClass* WidgetClass = UYogSlotSelectWidgetBase::StaticClass();
	TestNotNull(TEXT("Slot select widget class exists"), WidgetClass);

	const TCHAR* RequiredFunctions[] =
	{
		TEXT("HandleContinueSlot0"),
		TEXT("HandleContinueSlot1"),
		TEXT("HandleContinueSlot2"),
		TEXT("HandleNewGameSlot0"),
		TEXT("HandleNewGameSlot1"),
		TEXT("HandleNewGameSlot2"),
		TEXT("HandleDeleteSlot0"),
		TEXT("HandleDeleteSlot1"),
		TEXT("HandleDeleteSlot2"),
		TEXT("ApplyPreview_0"),
		TEXT("ApplyPreview_1"),
		TEXT("ApplyPreview_2")
	};

	for (const TCHAR* FunctionName : RequiredFunctions)
	{
		TestNotNull(FString::Printf(TEXT("Function %s is exposed"), FunctionName),
			WidgetClass->FindFunctionByName(FunctionName));
	}

	const TCHAR* RequiredDesignerBindings[] =
	{
		TEXT("SlotCard_0"), TEXT("SlotTitleText_0"), TEXT("SlotPreviewText_0"), TEXT("BtnContinue_0"), TEXT("BtnNewGame_0"), TEXT("BtnDelete_0"),
		TEXT("SlotCard_1"), TEXT("SlotTitleText_1"), TEXT("SlotPreviewText_1"), TEXT("BtnContinue_1"), TEXT("BtnNewGame_1"), TEXT("BtnDelete_1"),
		TEXT("SlotCard_2"), TEXT("SlotTitleText_2"), TEXT("SlotPreviewText_2"), TEXT("BtnContinue_2"), TEXT("BtnNewGame_2"), TEXT("BtnDelete_2")
	};

	for (const TCHAR* PropertyName : RequiredDesignerBindings)
	{
		TestNotNull(FString::Printf(TEXT("Designer binding %s exists"), PropertyName),
			FindFProperty<FObjectProperty>(WidgetClass, PropertyName));
	}

	FSlotPreviewData EmptyPreview;
	TestEqual(TEXT("Empty slot summary is explicit"),
		UYogSlotSelectWidgetBase::BuildPreviewSummary(EmptyPreview).ToString(),
		FString(TEXT("Empty Slot")));

	FSlotPreviewData PendingPreview;
	PendingPreview.bHasData = true;
	PendingPreview.bHasPendingRun = true;
	PendingPreview.HighestFloor = 7;
	PendingPreview.TotalPlayTimeSeconds = 3661;
	PendingPreview.LastPlayTime = FDateTime(2026, 5, 18, 13, 45, 0);
	const FString PendingSummary = UYogSlotSelectWidgetBase::BuildPreviewSummary(PendingPreview).ToString();
	TestTrue(TEXT("Pending summary includes highest floor"), PendingSummary.Contains(TEXT("Floor 7")));
	TestTrue(TEXT("Pending summary includes pending run state"), PendingSummary.Contains(TEXT("Continue Available")));
	TestTrue(TEXT("Pending summary includes play time"), PendingSummary.Contains(TEXT("01:01:01")));

	FSlotPreviewData FirstRunPreview;
	FirstRunPreview.bHasData = true;
	FirstRunPreview.bFirstRunTutorialActive = true;
	const FString FirstRunSummary = UYogSlotSelectWidgetBase::BuildPreviewSummary(FirstRunPreview).ToString();
	TestTrue(TEXT("First-run tutorial save is explicit in slot preview"), FirstRunSummary.Contains(TEXT("First Run Tutorial")));

	return true;
}

#endif
