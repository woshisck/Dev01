#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/WeaponFloatWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFloatCollapseDelegateSelfUnbindTest,
	"DevKit.UI.WeaponFloat.CollapseDelegateSelfUnbind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponFloatCollapseDelegateSelfUnbindTest::RunTest(const FString& Parameters)
{
	UWeaponFloatWidget* Widget = NewObject<UWeaponFloatWidget>();
	TestNotNull(TEXT("Weapon float widget can be created for delegate test"), Widget);
	if (!Widget)
	{
		return false;
	}

	bool bTailCallbackRan = false;
	FSimpleDelegate TailCallback = FSimpleDelegate::CreateLambda([&bTailCallbackRan]()
	{
		bTailCallbackRan = true;
	});

	Widget->OnCollapseComplete.BindLambda([Widget, TailCallback](FVector2D) mutable
	{
		Widget->OnCollapseComplete.Unbind();
		if (TailCallback.IsBound())
		{
			TailCallback.Execute();
		}
	});

	Widget->BroadcastCollapseComplete(FVector2D(12.f, 34.f));

	TestTrue(TEXT("Tail callback still runs when collapse delegate clears itself"), bTailCallbackRan);
	TestFalse(TEXT("Widget collapse delegate is consumed after broadcast"), Widget->OnCollapseComplete.IsBound());

	return true;
}

#endif
