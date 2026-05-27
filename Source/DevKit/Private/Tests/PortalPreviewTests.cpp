#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/Portal.h"
#include "UI/YogHUD.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewRestoresLegacyAnchorPositionTest,
	"DevKit.Portal.PreviewRestoresLegacyAnchorPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewRestoresLegacyAnchorPositionTest::RunTest(const FString& Parameters)
{
	const FVector2D ViewportSize(1920.f, 1080.f);
	const FVector2D ProjectedDoorPosition(1896.f, 1068.f);

	const FVector2D AnchorPosition = AYogHUD::ResolvePortalPreviewAnchorPosition(
		ProjectedDoorPosition,
		ViewportSize,
		32.f,
		24.f);

	TestEqual(TEXT("Portal preview keeps the legacy door-following horizontal anchor"),
		AnchorPosition.X,
		1864.0,
		0.001);
	TestEqual(TEXT("Portal preview clamps only the anchor Y inside the viewport"),
		AnchorPosition.Y,
		1056.0,
		0.001);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewAlignsBesideDoorProjectionTest,
	"DevKit.Portal.PreviewAlignsBesideDoorProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewAlignsBesideDoorProjectionTest::RunTest(const FString& Parameters)
{
	const FVector2D ViewportSize(1920.f, 1080.f);

	const FVector2D RightDoorAlignment = AYogHUD::ResolvePortalPreviewAlignment(
		FVector2D(1700.f, 540.f),
		ViewportSize);
	TestEqual(TEXT("Door on the right side anchors the preview by its right edge"),
		RightDoorAlignment.X,
		1.0,
		0.001);
	TestEqual(TEXT("Preview stays vertically bottom-aligned near the door"),
		RightDoorAlignment.Y,
		1.0,
		0.001);

	const FVector2D LeftDoorAlignment = AYogHUD::ResolvePortalPreviewAlignment(
		FVector2D(220.f, 540.f),
		ViewportSize);
	TestEqual(TEXT("Door on the left side anchors the preview by its left edge"),
		LeftDoorAlignment.X,
		0.0,
		0.001);
	TestEqual(TEXT("Preview stays vertically bottom-aligned near the door"),
		LeftDoorAlignment.Y,
		1.0,
		0.001);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewUsesFixedRewardTypesTest,
	"DevKit.Portal.PreviewUsesFixedRewardTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewUsesFixedRewardTypesTest::RunTest(const FString& Parameters)
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

	FLootOption DuplicateGoldOption;
	DuplicateGoldOption.LootType = ELootType::Gold;
	DuplicateGoldOption.Amount = 75;

	FLootOption RuneOption;
	RuneOption.LootType = ELootType::Rune;
	RuneOption.RuneAsset = NewObject<URuneDataAsset>(Room);

	FLootOption MaterialOption;
	MaterialOption.LootType = ELootType::Material;
	MaterialOption.Amount = 1;

	Room->FixedRewardOptions = { GoldOption, DuplicateGoldOption, RuneOption, MaterialOption };
	Portal->Open(TEXT("TutorialGoldRoom"), Room, {});

	bool bValid = true;
	bValid &= TestEqual(TEXT("Portal preview carries one option per fixed reward type"),
		Portal->CachedPreviewInfo.RewardPreviewOptions.Num(),
		3);
	if (Portal->CachedPreviewInfo.RewardPreviewOptions.Num() == 3)
	{
		bValid &= TestEqual(TEXT("First reward preview option is gold"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[0].LootType,
			ELootType::Gold);
		bValid &= TestEqual(TEXT("Second reward preview option is card"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[1].LootType,
			ELootType::Rune);
		bValid &= TestNull(TEXT("Card reward preview is type-level and does not expose a concrete rune"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[1].RuneAsset.Get());
		bValid &= TestEqual(TEXT("Third reward preview option is material"),
			Portal->CachedPreviewInfo.RewardPreviewOptions[2].LootType,
			ELootType::Material);
	}

	Portal->Destroy();
	return bValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewSummarizesLootPoolAsCardRewardTypeTest,
	"DevKit.Portal.PreviewSummarizesLootPoolAsCardRewardType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewSummarizesLootPoolAsCardRewardTypeTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	APortal* Portal = World->SpawnActor<APortal>();
	URoomDataAsset* Room = NewObject<URoomDataAsset>(Portal);
	Room->RoomName = TEXT("TutorialCardRoom");
	Room->LootPool = {
		NewObject<URuneDataAsset>(Room),
		NewObject<URuneDataAsset>(Room),
		NewObject<URuneDataAsset>(Room)
	};

	Portal->Open(TEXT("TutorialCardRoom"), Room, {});

	bool bValid = true;
	bValid &= TestEqual(TEXT("Portal preview summarizes any rune loot pool as one card reward type"),
		Portal->CachedPreviewInfo.RewardPreviewOptions.Num(),
		1);
	if (Portal->CachedPreviewInfo.RewardPreviewOptions.Num() == 1)
	{
		const FLootOption& PreviewOption = Portal->CachedPreviewInfo.RewardPreviewOptions[0];
		bValid &= TestEqual(TEXT("Loot pool preview reward type is card/rune"),
			PreviewOption.LootType,
			ELootType::Rune);
		bValid &= TestNull(TEXT("Loot pool preview does not reveal the concrete rune asset"),
			PreviewOption.RuneAsset.Get());
		bValid &= TestNull(TEXT("Loot pool preview does not carry a concrete rune icon"),
			PreviewOption.Icon.Get());
	}

	Portal->Destroy();
	return bValid;
}

#endif
