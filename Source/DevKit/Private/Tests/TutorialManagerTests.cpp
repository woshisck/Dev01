#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Tutorial/TutorialManager.h"
#include "UI/DialogContentDA.h"
#include "UI/TutorialRegistryDA.h"

namespace TutorialManagerTests
{
UDialogContentDA* MakeTutorialContent(UObject* Outer, int32 PageCount)
{
	UDialogContentDA* Content = NewObject<UDialogContentDA>(Outer);
	for (int32 Index = 0; Index < PageCount; ++Index)
	{
		FTutorialPage& Page = Content->Pages.AddDefaulted_GetRef();
		Page.Title = FText::FromString(FString::Printf(TEXT("Page %d"), Index + 1));
		Page.Body = FText::FromString(TEXT("Body"));
	}
	return Content;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerPrefersMoonlightLinkTutorialTest,
	"DevKit.TutorialManager.PrefersMoonlightLinkTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerPrefersMoonlightLinkTutorialTest::RunTest(const FString& Parameters)
{
	UTutorialRegistryDA* Registry = NewObject<UTutorialRegistryDA>();
	Registry->Entries.Add(
		FName(TEXT("tutorial_card_link")),
		TutorialManagerTests::MakeTutorialContent(Registry, 1));
	Registry->Entries.Add(
		FName(TEXT("tutorial_card_link_moonlight")),
		TutorialManagerTests::MakeTutorialContent(Registry, 3));

	TestEqual(
		TEXT("Moonlight link tutorial is preferred when configured"),
		UTutorialManager::ResolveLinkCardTutorialEventIdForTest(Registry).ToString(),
		FString(TEXT("tutorial_card_link_moonlight")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerFallsBackToGenericLinkTutorialTest,
	"DevKit.TutorialManager.FallsBackToGenericLinkTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerFallsBackToGenericLinkTutorialTest::RunTest(const FString& Parameters)
{
	UTutorialRegistryDA* Registry = NewObject<UTutorialRegistryDA>();
	Registry->Entries.Add(
		FName(TEXT("tutorial_card_link")),
		TutorialManagerTests::MakeTutorialContent(Registry, 1));

	TestEqual(
		TEXT("Generic link tutorial is used when moonlight tutorial is not configured"),
		UTutorialManager::ResolveLinkCardTutorialEventIdForTest(Registry).ToString(),
		FString(TEXT("tutorial_card_link")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerFallsBackWhenMoonlightTutorialHasNoPagesTest,
	"DevKit.TutorialManager.FallsBackWhenMoonlightTutorialHasNoPages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerFallsBackWhenMoonlightTutorialHasNoPagesTest::RunTest(const FString& Parameters)
{
	UTutorialRegistryDA* Registry = NewObject<UTutorialRegistryDA>();
	Registry->Entries.Add(
		FName(TEXT("tutorial_card_link")),
		TutorialManagerTests::MakeTutorialContent(Registry, 1));
	Registry->Entries.Add(
		FName(TEXT("tutorial_card_link_moonlight")),
		TutorialManagerTests::MakeTutorialContent(Registry, 0));

	TestEqual(
		TEXT("Empty moonlight tutorial does not block generic fallback"),
		UTutorialManager::ResolveLinkCardTutorialEventIdForTest(Registry).ToString(),
		FString(TEXT("tutorial_card_link")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerMoonlightLinkContentConfiguredTest,
	"DevKit.TutorialManager.MoonlightLinkContentConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerMoonlightLinkContentConfiguredTest::RunTest(const FString& Parameters)
{
	const UTutorialRegistryDA* Registry = LoadObject<UTutorialRegistryDA>(
		nullptr,
		TEXT("/Game/Docs/UI/Tutorial/DA_TutorialRegistry.DA_TutorialRegistry"));
	if (!TestNotNull(TEXT("Tutorial registry asset loads"), Registry))
	{
		return false;
	}

	const TArray<FTutorialPage>* Pages = Registry->FindPages(FName(TEXT("tutorial_card_link_moonlight")));
	if (!TestNotNull(TEXT("Moonlight link tutorial is registered"), Pages))
	{
		return false;
	}

	TestEqual(TEXT("Moonlight link tutorial has three pages"), Pages->Num(), 3);
	for (int32 Index = 0; Index < Pages->Num(); ++Index)
	{
		TestNotNull(
			FString::Printf(TEXT("Moonlight link page %d has an illustration"), Index + 1),
			(*Pages)[Index].Illustration.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerCoreTutorialIllustrationsConfiguredTest,
	"DevKit.TutorialManager.CoreTutorialIllustrationsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerCoreTutorialIllustrationsConfiguredTest::RunTest(const FString& Parameters)
{
	const UTutorialRegistryDA* Registry = LoadObject<UTutorialRegistryDA>(
		nullptr,
		TEXT("/Game/Docs/UI/Tutorial/DA_TutorialRegistry.DA_TutorialRegistry"));
	if (!TestNotNull(TEXT("Tutorial registry asset loads"), Registry))
	{
		return false;
	}

	const FName RequiredEventIDs[] = {
		FName(TEXT("tutorial_weapon_pickup")),
		FName(TEXT("tutorial_first_rune")),
		FName(TEXT("tutorial_backpack")),
		FName(TEXT("tutorial_card_link")),
		FName(TEXT("tutorial_card_link_moonlight")),
		FName(TEXT("tutorial_heavy_card")),
		FName(TEXT("tutorial_finisher")),
		FName(TEXT("tutorial_shuffle_hint")),
	};

	bool bAllConfigured = true;
	for (const FName EventID : RequiredEventIDs)
	{
		const TArray<FTutorialPage>* Pages = Registry->FindPages(EventID);
		bAllConfigured &= TestNotNull(
			FString::Printf(TEXT("%s tutorial is registered"), *EventID.ToString()),
			Pages);
		if (!Pages)
		{
			continue;
		}

		bAllConfigured &= TestTrue(
			FString::Printf(TEXT("%s has pages"), *EventID.ToString()),
			Pages->Num() > 0);
		for (int32 Index = 0; Index < Pages->Num(); ++Index)
		{
			bAllConfigured &= TestNotNull(
				FString::Printf(TEXT("%s page %d has an illustration"), *EventID.ToString(), Index + 1),
				(*Pages)[Index].Illustration.Get());
		}
	}

	return bAllConfigured;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerLoadsDefaultRegistryWhenHudRegistryIsUnsetTest,
	"DevKit.TutorialManager.LoadsDefaultRegistryWhenHudRegistryIsUnset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerLoadsDefaultRegistryWhenHudRegistryIsUnsetTest::RunTest(const FString& Parameters)
{
	const UTutorialRegistryDA* Registry = UTutorialManager::ResolveTutorialRegistryForTest(nullptr);
	if (!TestNotNull(TEXT("Default tutorial registry loads when HUD registry is unset"), Registry))
	{
		return false;
	}

	const TArray<FTutorialPage>* Pages = Registry->FindPages(FName(TEXT("tutorial_weapon_pickup")));
	if (!TestNotNull(TEXT("Default registry contains weapon pickup tutorial"), Pages))
	{
		return false;
	}

	TestTrue(TEXT("Weapon pickup tutorial has pages"), Pages->Num() > 0);
	if (Pages->Num() > 0)
	{
		TestNotNull(TEXT("Weapon pickup first page has an illustration"), (*Pages)[0].Illustration.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialManagerSuppressesDirectWeaponPickupEventTest,
	"DevKit.TutorialManager.SuppressesDirectWeaponPickupEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialManagerSuppressesDirectWeaponPickupEventTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("Weapon pickup tutorial cannot be shown by direct story event"),
		UTutorialManager::IsDirectEventTutorialAllowedForTest(FName(TEXT("tutorial_weapon_pickup"))));

	TestTrue(
		TEXT("Other tutorial events can still be shown directly"),
		UTutorialManager::IsDirectEventTutorialAllowedForTest(FName(TEXT("tutorial_first_rune"))));

	return true;
}

#endif
