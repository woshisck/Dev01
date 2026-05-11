#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/AltarMenuWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UI/YogCommonUITextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAltarMenuWidgetUsesYogCommonTextTest,
	"DevKit.UI.AltarMenuWidget.UsesYogCommonUITextBlockClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAltarMenuWidgetUsesYogCommonTextTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(
		TEXT("Altar menu replacement text class is Yog Common UI Text Block"),
		UAltarMenuWidget::GetMenuTextBlockClassForTests()->IsChildOf(UYogCommonUITextBlock::StaticClass()));

	TestTrue(
		TEXT("Sacrifice selection text class is Yog Common UI Text Block"),
		USacrificeSelectionWidget::GetMenuTextBlockClassForTests()->IsChildOf(UYogCommonUITextBlock::StaticClass()));

	return true;
}

#endif
