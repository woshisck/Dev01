#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/PlayerLocomotionMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionMoveAngleTest,
	"DevKit.Locomotion.MoveAngleIsRelativeToFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionMoveAngleTest::RunTest(const FString& Parameters)
{
	const FRotator FacingNorth(0.f, 0.f, 0.f);
	TestEqual(TEXT("Moving along facing is 0"), YogLocomotion::CalculateMoveAngle(FVector(1.f, 0.f, 0.f), FacingNorth), 0.f, 0.01f);
	TestEqual(TEXT("Moving to the right of facing is +90"), YogLocomotion::CalculateMoveAngle(FVector(0.f, 1.f, 0.f), FacingNorth), 90.f, 0.01f);
	TestEqual(TEXT("Moving to the left of facing is -90"), YogLocomotion::CalculateMoveAngle(FVector(0.f, -1.f, 0.f), FacingNorth), -90.f, 0.01f);
	TestEqual(TEXT("Moving backwards is 180"), FMath::Abs(YogLocomotion::CalculateMoveAngle(FVector(-1.f, 0.f, 0.f), FacingNorth)), 180.f, 0.01f);

	const FRotator FacingEast(0.f, 90.f, 0.f);
	TestEqual(TEXT("Facing +Y and moving +Y is 0"), YogLocomotion::CalculateMoveAngle(FVector(0.f, 1.f, 0.f), FacingEast), 0.f, 0.01f);
	TestEqual(TEXT("Facing +Y and moving +X is -90"), YogLocomotion::CalculateMoveAngle(FVector(1.f, 0.f, 0.f), FacingEast), -90.f, 0.01f);
	TestEqual(TEXT("Pitch and roll are ignored"), YogLocomotion::CalculateMoveAngle(FVector(0.f, 1.f, 0.f), FRotator(30.f, 0.f, 20.f)), 90.f, 0.01f);
	TestEqual(TEXT("Zero direction yields 0"), YogLocomotion::CalculateMoveAngle(FVector::ZeroVector, FacingNorth), 0.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionCardinalBandsTest,
	"DevKit.Locomotion.CardinalBandsWithoutHysteresis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionCardinalBandsTest::RunTest(const FString& Parameters)
{
	// Previous is chosen far from the angle under test so the dead zone never holds it.
	auto Resolve = [](float Angle)
	{
		const ELocoCardinal Far = FMath::Abs(FMath::UnwindDegrees(Angle)) < 90.f ? ELocoCardinal::Back : ELocoCardinal::Front;
		return YogLocomotion::AngleToCardinal(Angle, Far, 0.f);
	};

	TestTrue(TEXT("0 is Front"), Resolve(0.f) == ELocoCardinal::Front);
	TestTrue(TEXT("40 is Front"), Resolve(40.f) == ELocoCardinal::Front);
	TestTrue(TEXT("-40 is Front"), Resolve(-40.f) == ELocoCardinal::Front);
	TestTrue(TEXT("90 is Right"), Resolve(90.f) == ELocoCardinal::Right);
	TestTrue(TEXT("-90 is Left"), Resolve(-90.f) == ELocoCardinal::Left);
	TestTrue(TEXT("180 is Back"), Resolve(180.f) == ELocoCardinal::Back);
	TestTrue(TEXT("-180 is Back"), Resolve(-180.f) == ELocoCardinal::Back);
	TestTrue(TEXT("150 is Back"), Resolve(150.f) == ELocoCardinal::Back);
	TestTrue(TEXT("-150 is Back"), Resolve(-150.f) == ELocoCardinal::Back);
	TestTrue(TEXT("540 unwinds to Back"), Resolve(540.f) == ELocoCardinal::Back);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionCardinalHysteresisTest,
	"DevKit.Locomotion.CardinalHysteresisHoldsPreviousInDeadZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionCardinalHysteresisTest::RunTest(const FString& Parameters)
{
	constexpr float DeadZone = 15.f;

	TestTrue(TEXT("Front holds just past the 45 edge"),
		YogLocomotion::AngleToCardinal(50.f, ELocoCardinal::Front, DeadZone) == ELocoCardinal::Front);
	TestTrue(TEXT("Front holds at the edge of the dead zone"),
		YogLocomotion::AngleToCardinal(59.f, ELocoCardinal::Front, DeadZone) == ELocoCardinal::Front);
	TestTrue(TEXT("Front releases once the dead zone is passed"),
		YogLocomotion::AngleToCardinal(65.f, ELocoCardinal::Front, DeadZone) == ELocoCardinal::Right);

	TestTrue(TEXT("Right holds just under its 45 edge"),
		YogLocomotion::AngleToCardinal(40.f, ELocoCardinal::Right, DeadZone) == ELocoCardinal::Right);
	TestTrue(TEXT("Right releases once the dead zone is passed"),
		YogLocomotion::AngleToCardinal(20.f, ELocoCardinal::Right, DeadZone) == ELocoCardinal::Front);

	TestTrue(TEXT("Back holds across the +-180 seam"),
		YogLocomotion::AngleToCardinal(-170.f, ELocoCardinal::Back, DeadZone) == ELocoCardinal::Back);
	TestTrue(TEXT("Back holds just short of its band on the negative side"),
		YogLocomotion::AngleToCardinal(-125.f, ELocoCardinal::Back, DeadZone) == ELocoCardinal::Back);
	TestTrue(TEXT("Back releases to Left past the dead zone"),
		YogLocomotion::AngleToCardinal(-110.f, ELocoCardinal::Back, DeadZone) == ELocoCardinal::Left);

	TestTrue(TEXT("Oversized dead zone is clamped and still resolves a cardinal"),
		YogLocomotion::AngleToCardinal(180.f, ELocoCardinal::Front, 500.f) == ELocoCardinal::Back);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionReversalTest,
	"DevKit.Locomotion.ReversalDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionReversalTest::RunTest(const FString& Parameters)
{
	const FVector Velocity(600.f, 0.f, 0.f);

	TestTrue(TEXT("Exactly opposite input is a reversal"), YogLocomotion::IsReversal(Velocity, FVector(-1.f, 0.f, 0.f), 30.f));
	TestTrue(TEXT("Input 25 degrees off opposite is a reversal"),
		YogLocomotion::IsReversal(Velocity, FVector(-FMath::Cos(FMath::DegreesToRadians(25.f)), FMath::Sin(FMath::DegreesToRadians(25.f)), 0.f), 30.f));
	TestFalse(TEXT("Input 40 degrees off opposite is not a reversal"),
		YogLocomotion::IsReversal(Velocity, FVector(-FMath::Cos(FMath::DegreesToRadians(40.f)), FMath::Sin(FMath::DegreesToRadians(40.f)), 0.f), 30.f));
	TestFalse(TEXT("Perpendicular input is not a reversal"), YogLocomotion::IsReversal(Velocity, FVector(0.f, 1.f, 0.f), 30.f));
	TestFalse(TEXT("Same direction is not a reversal"), YogLocomotion::IsReversal(Velocity, FVector(1.f, 0.f, 0.f), 30.f));
	TestFalse(TEXT("Zero velocity never reverses"), YogLocomotion::IsReversal(FVector::ZeroVector, FVector(-1.f, 0.f, 0.f), 30.f));
	TestFalse(TEXT("Zero input never reverses"), YogLocomotion::IsReversal(Velocity, FVector::ZeroVector, 30.f));
	TestTrue(TEXT("Vertical components are ignored"), YogLocomotion::IsReversal(FVector(600.f, 0.f, 200.f), FVector(-1.f, 0.f, 5.f), 30.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionPivotTypeTest,
	"DevKit.Locomotion.PivotTypeSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionPivotTypeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Front"), YogLocomotion::ChoosePivotType(ELocoCardinal::Front) == ELocoPivotType::Front_To_Back);
	TestTrue(TEXT("Back"), YogLocomotion::ChoosePivotType(ELocoCardinal::Back) == ELocoPivotType::Back_To_Front);
	TestTrue(TEXT("Left"), YogLocomotion::ChoosePivotType(ELocoCardinal::Left) == ELocoPivotType::Left_To_Right);
	TestTrue(TEXT("Right"), YogLocomotion::ChoosePivotType(ELocoCardinal::Right) == ELocoPivotType::Right_To_Left);

	// Free mode end to end: a -45 view yaw (the isometric camera) and the input rotation used by the controller.
	// Facing is irrelevant, so this holds no matter how far orient-to-movement has turned the body.
	const FRotator View(0.f, -45.f, 0.f);
	auto ScreenToWorld = [](float InputX, float InputY)
	{
		const FVector2D Rotated = FVector2D(InputX, InputY).GetRotated(-45.f);
		return FVector(Rotated.X, Rotated.Y, 0.f);
	};
	auto FreePivotFrom = [&View](const FVector& WorldVelocity)
	{
		const float Angle = YogLocomotion::CalculateMoveAngle(WorldVelocity, View);
		return YogLocomotion::ChoosePivotType(YogLocomotion::AngleToCardinal(Angle, ELocoCardinal::Front, 0.f));
	};
	TestTrue(TEXT("Free, screen up then down"), FreePivotFrom(ScreenToWorld(1.f, 0.f)) == ELocoPivotType::Front_To_Back);
	TestTrue(TEXT("Free, screen down then up"), FreePivotFrom(ScreenToWorld(-1.f, 0.f)) == ELocoPivotType::Back_To_Front);
	TestTrue(TEXT("Free, screen right then left"), FreePivotFrom(ScreenToWorld(0.f, 1.f)) == ELocoPivotType::Right_To_Left);
	TestTrue(TEXT("Free, screen left then right"), FreePivotFrom(ScreenToWorld(0.f, -1.f)) == ELocoPivotType::Left_To_Right);

	TestTrue(TEXT("Opposite of Front"), YogLocomotion::OppositeCardinal(ELocoCardinal::Front) == ELocoCardinal::Back);
	TestTrue(TEXT("Opposite of Back"), YogLocomotion::OppositeCardinal(ELocoCardinal::Back) == ELocoCardinal::Front);
	TestTrue(TEXT("Opposite of Left"), YogLocomotion::OppositeCardinal(ELocoCardinal::Left) == ELocoCardinal::Right);
	TestTrue(TEXT("Opposite of Right"), YogLocomotion::OppositeCardinal(ELocoCardinal::Right) == ELocoCardinal::Left);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionMotionStateTest,
	"DevKit.Locomotion.MotionStateFromVelocityAndAcceleration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionMotionStateTest::RunTest(const FString& Parameters)
{
	using namespace YogLocomotion;
	const FLocoMotionTuning Tuning;
	constexpr float MaxSpeed = 600.f;
	const FVector Fwd(1.f, 0.f, 0.f);
	const FVector Accel = Fwd * 2048.f;

	TestTrue(TEXT("At rest, no input"), NextMotionState(ELocoMotionState::Moving, FVector::ZeroVector, FVector::ZeroVector, MaxSpeed, true, Tuning) == ELocoMotionState::Idle);
	TestTrue(TEXT("Sliding, no input"), NextMotionState(ELocoMotionState::Moving, Fwd * 300.f, FVector::ZeroVector, MaxSpeed, true, Tuning) == ELocoMotionState::Stopping);
	TestTrue(TEXT("Input from rest"), NextMotionState(ELocoMotionState::Idle, FVector::ZeroVector, Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Starting);
	TestTrue(TEXT("Still accelerating"), NextMotionState(ELocoMotionState::Starting, Fwd * 300.f, Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Starting);
	TestTrue(TEXT("Reached loop speed"), NextMotionState(ELocoMotionState::Starting, Fwd * 580.f, Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Moving);
	TestTrue(TEXT("Moving holds through a speed dip"), NextMotionState(ELocoMotionState::Moving, Fwd * 400.f, Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Moving);

	TestTrue(TEXT("Opposite input at speed pivots"), NextMotionState(ELocoMotionState::Moving, Fwd * 600.f, -Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Pivoting);
	TestTrue(TEXT("Opposite input while slow does not pivot"), NextMotionState(ELocoMotionState::Starting, Fwd * 100.f, -Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Starting);
	TestTrue(TEXT("Perpendicular input does not pivot"), NextMotionState(ELocoMotionState::Moving, Fwd * 600.f, FVector(0.f, 2048.f, 0.f), MaxSpeed, true, Tuning) == ELocoMotionState::Moving);
	TestTrue(TEXT("No pivot when disallowed"), NextMotionState(ELocoMotionState::Moving, Fwd * 600.f, -Accel, MaxSpeed, false, Tuning) == ELocoMotionState::Moving);

	TestTrue(TEXT("Pivot holds while braking"), NextMotionState(ELocoMotionState::Pivoting, Fwd * 200.f, -Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Pivoting);
	TestTrue(TEXT("Pivot ends at zero velocity"), NextMotionState(ELocoMotionState::Pivoting, FVector::ZeroVector, -Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Starting);
	TestTrue(TEXT("Pivot ends when velocity flips past zero"), NextMotionState(ELocoMotionState::Pivoting, Fwd * -50.f, -Accel, MaxSpeed, true, Tuning) == ELocoMotionState::Starting);
	TestTrue(TEXT("Releasing input mid-pivot stops"), NextMotionState(ELocoMotionState::Pivoting, Fwd * 200.f, FVector::ZeroVector, MaxSpeed, true, Tuning) == ELocoMotionState::Stopping);
	TestTrue(TEXT("Ability taking over mid-pivot leaves it"), NextMotionState(ELocoMotionState::Pivoting, Fwd * 200.f, -Accel, MaxSpeed, false, Tuning) == ELocoMotionState::Starting);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionViewYawTest,
	"DevKit.Locomotion.ViewYawFromCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLocomotionViewYawTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Tilted camera uses its yaw"), YogLocomotion::ViewYawFromCameraRotation(FRotator(-50.f, -45.f, 0.f)), -45.f, 0.01f);
	TestEqual(TEXT("Straight-down camera falls back to its up axis"),
		YogLocomotion::ViewYawFromCameraRotation(FRotator(-90.f, 30.f, 0.f)), 30.f, 0.01f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
