#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "UI/PortalPreviewWidget.h"

namespace PortalPreviewWidgetTests
{
FLootOption MakeLootOption(ELootType LootType)
{
	FLootOption Option;
	Option.LootType = LootType;
	return Option;
}

FLootOption MakeRuneOptionWithConcreteIcon()
{
	FLootOption Option = MakeLootOption(ELootType::Rune);
	Option.Icon = NewObject<UTexture2D>();
	Option.DisplayName = NSLOCTEXT("PortalPreviewWidgetTests", "ConcreteRune", "Concrete Rune");
	return Option;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewWidgetCollapsesRuneRewardsTest,
	"DevKit.PortalPreviewWidget.CollapsesRuneRewardsToSingleCardIcon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewWidgetCollapsesRuneRewardsTest::RunTest(const FString& Parameters)
{
	const TArray<FLootOption> Aggregated = UPortalPreviewWidget::BuildAggregatedRewardPreviewOptions({
		PortalPreviewWidgetTests::MakeRuneOptionWithConcreteIcon(),
		PortalPreviewWidgetTests::MakeRuneOptionWithConcreteIcon(),
		PortalPreviewWidgetTests::MakeRuneOptionWithConcreteIcon()
	});

	TestEqual(TEXT("Three card rewards collapse to one portal preview icon"), Aggregated.Num(), 1);
	if (Aggregated.Num() == 1)
	{
		TestEqual(TEXT("Collapsed preview still represents card loot"), Aggregated[0].LootType, ELootType::Rune);
		TestNull(TEXT("Collapsed card preview does not expose a concrete card icon"), Aggregated[0].Icon.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPortalPreviewWidgetPreservesCurrencyMaterialWithCardTest,
	"DevKit.PortalPreviewWidget.PreservesCurrencyMaterialWithSingleCardIcon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalPreviewWidgetPreservesCurrencyMaterialWithCardTest::RunTest(const FString& Parameters)
{
	const TArray<FLootOption> Aggregated = UPortalPreviewWidget::BuildAggregatedRewardPreviewOptions({
		PortalPreviewWidgetTests::MakeLootOption(ELootType::Gold),
		PortalPreviewWidgetTests::MakeRuneOptionWithConcreteIcon(),
		PortalPreviewWidgetTests::MakeLootOption(ELootType::Material),
		PortalPreviewWidgetTests::MakeRuneOptionWithConcreteIcon()
	});

	int32 GoldCount = 0;
	int32 MaterialCount = 0;
	int32 RuneCount = 0;
	for (const FLootOption& Option : Aggregated)
	{
		if (Option.LootType == ELootType::Gold)
		{
			++GoldCount;
		}
		else if (Option.LootType == ELootType::Material)
		{
			++MaterialCount;
		}
		else if (Option.LootType == ELootType::Rune)
		{
			++RuneCount;
			TestNull(TEXT("Mixed reward card preview does not expose a concrete card icon"), Option.Icon.Get());
		}
	}

	TestEqual(TEXT("Mixed rewards keep gold, material, and one card preview"), Aggregated.Num(), 3);
	TestEqual(TEXT("Gold preview count"), GoldCount, 1);
	TestEqual(TEXT("Material preview count"), MaterialCount, 1);
	TestEqual(TEXT("Collapsed card preview count"), RuneCount, 1);

	return true;
}

#endif
