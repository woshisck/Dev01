#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/Portal.h"
#include "UI/YogHUD.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewPositionClampsInsideViewportTest,
	"DevKit.Portal.PreviewPositionClampsInsideViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewPositionClampsInsideViewportTest::RunTest(const FString& Parameters)
{
	const FVector2D ViewportSize(1920.f, 1080.f);
	const FVector2D WidgetSize(430.f, 260.f);
	const FVector2D Alignment(0.5f, 1.0f);
	const FVector2D ProjectedDoorPosition(1896.f, 1068.f);

	const FVector2D ClampedPosition = AYogHUD::ClampPortalPreviewPositionForViewport(
		ProjectedDoorPosition,
		ViewportSize,
		WidgetSize,
		Alignment,
		24.f);

	TestTrue(TEXT("Right edge remains inside viewport"),
		ClampedPosition.X + WidgetSize.X * (1.0f - Alignment.X) <= ViewportSize.X - 24.f);
	TestTrue(TEXT("Bottom edge remains inside viewport"),
		ClampedPosition.Y + WidgetSize.Y * (1.0f - Alignment.Y) <= ViewportSize.Y - 24.f);
	TestTrue(TEXT("Left edge remains inside viewport"),
		ClampedPosition.X - WidgetSize.X * Alignment.X >= 24.f);
	TestTrue(TEXT("Top edge remains inside viewport"),
		ClampedPosition.Y - WidgetSize.Y * Alignment.Y >= 24.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewUsesFixedRoomRewardOptionsTest,
	"DevKit.Portal.PreviewUsesFixedRoomRewardOptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewUsesFixedRoomRewardOptionsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	APortal* Portal = World->SpawnActor<APortal>();
	URoomDataAsset* Room = NewObject<URoomDataAsset>(Portal);
	Room->RoomName = TEXT("TutorialGoldRoom");
	Room->bUseFixedRewardOptions = true;

	FLootOption GoldOption;
	GoldOption.LootType = ELootType::Gold;
	GoldOption.Amount = 50;

	FLootOption MaterialOption;
	MaterialOption.LootType = ELootType::Material;
	MaterialOption.Amount = 1;

	Room->FixedRewardOptions = { GoldOption, MaterialOption };
	Portal->Open(TEXT("TutorialGoldRoom"), Room, {});

	bool bValid = true;
	bValid &= TestEqual(TEXT("Portal preview carries fixed room reward options"),
		Portal->CachedPreviewInfo.RewardPreviewOptions.Num(),
		2);
	if (Portal->CachedPreviewInfo.RewardPreviewOptions.Num() == 2)
	{
		bValid &= TestEqual(TEXT("First reward preview option is gold"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[0].LootType,
			ELootType::Gold);
		bValid &= TestEqual(TEXT("Second reward preview option is material"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[1].LootType,
			ELootType::Material);
	}

	Portal->Destroy();
	return bValid;
}

#endif
