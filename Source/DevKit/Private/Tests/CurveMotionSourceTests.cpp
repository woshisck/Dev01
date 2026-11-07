#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/YogRootMotionSource_CurveMotion.h"
#include "Curves/CurveFloat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurveMotionFractionTest,
	"DevKit.Animation.CurveMotion.FractionDeltasSumToCurveEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurveMotionFractionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Null curve is linear"), FYogRootMotionSource_CurveMotion::EvaluateFraction(nullptr, 0.25f), 0.25f, 0.001f);
	TestEqual(TEXT("Alpha clamps above 1"), FYogRootMotionSource_CurveMotion::EvaluateFraction(nullptr, 1.5f), 1.f, 0.001f);

	UCurveFloat* EaseOut = NewObject<UCurveFloat>();
	EaseOut->FloatCurve.AddKey(0.f, 0.f);
	EaseOut->FloatCurve.AddKey(0.3f, 0.8f);
	EaseOut->FloatCurve.AddKey(1.f, 1.f);

	// Uneven ticks overshooting the window must still integrate to exactly the curve's end value,
	// otherwise server and client would disagree whenever their frame times differ.
	const float WindowDuration = 0.5f;
	const float Ticks[] = { 0.016f, 0.033f, 0.1f, 0.007f, 0.2f, 0.3f };
	float Time = 0.f;
	float Sum = 0.f;
	for (const float Tick : Ticks)
	{
		Sum += FYogRootMotionSource_CurveMotion::EvaluateFractionDelta(EaseOut, Time, Time + Tick, WindowDuration);
		Time += Tick;
	}
	TestEqual(TEXT("Deltas sum to the curve's end value"), Sum, 1.f, 0.001f);

	TestEqual(TEXT("Zero-length window applies everything on the first tick"),
		FYogRootMotionSource_CurveMotion::EvaluateFractionDelta(nullptr, 0.f, 0.016f, 0.f), 1.f, 0.001f);
	TestEqual(TEXT("Zero-length window applies nothing afterwards"),
		FYogRootMotionSource_CurveMotion::EvaluateFractionDelta(nullptr, 0.016f, 0.032f, 0.f), 0.f, 0.001f);
	return true;
}

#endif
