#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "System/YogBubbleSubsystem.h"
#include "UI/BubbleMessageTypes.h"
#include "UI/YogHUD.h"

namespace BubbleMessageTests
{
	const FVector2D Viewport(1920.f, 1080.f);
	constexpr float EdgeMargin = 48.f;

	FBubbleRequest MakeRequest(int32 Priority, const TCHAR* Body)
	{
		FBubbleRequest Request;
		Request.Priority = Priority;
		Request.Lines.Add(FBubbleLine{ FText::FromString(Body), 3.f });
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBubbleFallbackResolutionTest,
	"DevKit.BubbleMessage.ResolvesScreenFallbackFromProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBubbleFallbackResolutionTest::RunTest(const FString& Parameters)
{
	using namespace BubbleMessageTests;

	TestFalse(TEXT("A speaker near the middle of the screen stays world-anchored"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(960.f, 540.f), Viewport, true, EdgeMargin));

	TestTrue(TEXT("A speaker past the left edge falls back to the corner"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(-120.f, 540.f), Viewport, true, EdgeMargin));

	TestTrue(TEXT("A speaker past the bottom edge falls back to the corner"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(960.f, 1200.f), Viewport, true, EdgeMargin));

	TestTrue(TEXT("A speaker inside the edge margin falls back before it visually clips"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(10.f, 540.f), Viewport, true, EdgeMargin));

	// The regression this guards: a point behind the camera still projects to in-bounds
	// coordinates, so the bounds test alone would keep the bubble world-anchored.
	TestTrue(TEXT("A failed projection falls back even when the coordinates look in-bounds"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(960.f, 540.f), Viewport, false, EdgeMargin));

	TestTrue(TEXT("A degenerate viewport falls back rather than dividing into nothing"),
		UYogBubbleSubsystem::ResolveShouldFallbackToScreen(FVector2D(960.f, 540.f), FVector2D::ZeroVector, true, EdgeMargin));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBubbleQueuePriorityTest,
	"DevKit.BubbleMessage.SortsBlockingQueueByPriorityThenOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBubbleQueuePriorityTest::RunTest(const FString& Parameters)
{
	using namespace BubbleMessageTests;

	TArray<FBubbleRequest> Queue;
	Queue.Add(MakeRequest(YogBubblePriority::Ambient, TEXT("ambient")));
	Queue.Add(MakeRequest(YogBubblePriority::Story, TEXT("story")));
	Queue.Add(MakeRequest(YogBubblePriority::Hint, TEXT("hint-first")));
	Queue.Add(MakeRequest(YogBubblePriority::Hint, TEXT("hint-second")));

	UYogBubbleSubsystem::SortBlockingQueue(Queue);

	TestEqual(TEXT("Highest priority runs first"), Queue[0].Lines[0].Text.ToString(), FString(TEXT("story")));
	TestEqual(TEXT("Equal priorities keep request order"), Queue[1].Lines[0].Text.ToString(), FString(TEXT("hint-first")));
	TestEqual(TEXT("Equal priorities keep request order"), Queue[2].Lines[0].Text.ToString(), FString(TEXT("hint-second")));
	TestEqual(TEXT("Lowest priority runs last"), Queue[3].Lines[0].Text.ToString(), FString(TEXT("ambient")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBubbleCornerPlacementTest,
	"DevKit.BubbleMessage.PlacesCornerBubbleInsideViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBubbleCornerPlacementTest::RunTest(const FString& Parameters)
{
	using namespace BubbleMessageTests;

	constexpr float Padding = 64.f;
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Alignment = FVector2D::ZeroVector;

	AYogHUD::ResolveBubbleCornerPlacement(EBubbleScreenCorner::TopLeft, Viewport, Padding, Position, Alignment);
	TestEqual(TEXT("Top-left sits at the padding offset"), Position, FVector2D(Padding, Padding));
	TestEqual(TEXT("Top-left grows down-right"), Alignment, FVector2D(0.f, 0.f));

	AYogHUD::ResolveBubbleCornerPlacement(EBubbleScreenCorner::BottomRight, Viewport, Padding, Position, Alignment);
	TestEqual(TEXT("Bottom-right insets from the far corner"),
		Position, FVector2D(Viewport.X - Padding, Viewport.Y - Padding));
	TestEqual(TEXT("Bottom-right grows up-left"), Alignment, FVector2D(1.f, 1.f));

	AYogHUD::ResolveBubbleCornerPlacement(EBubbleScreenCorner::TopRight, Viewport, Padding, Position, Alignment);
	TestEqual(TEXT("Top-right anchors to the right edge"), Alignment, FVector2D(1.f, 0.f));

	AYogHUD::ResolveBubbleCornerPlacement(EBubbleScreenCorner::BottomLeft, Viewport, Padding, Position, Alignment);
	TestEqual(TEXT("Bottom-left anchors to the bottom edge"), Alignment, FVector2D(0.f, 1.f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
