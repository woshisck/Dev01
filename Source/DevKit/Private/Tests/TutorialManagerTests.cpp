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

#endif
