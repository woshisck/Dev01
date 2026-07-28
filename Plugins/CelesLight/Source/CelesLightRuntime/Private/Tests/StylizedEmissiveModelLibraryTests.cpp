#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StylizedEmissiveModelLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCelesStylizedEmissiveModelLibraryTest,
	"CelesLight.StylizedEmissive.ModelLibrary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesStylizedEmissiveModelLibraryTest::RunTest(const FString& Parameters)
{
	UStylizedEmissiveModelLibrary* Library = NewObject<UStylizedEmissiveModelLibrary>();
	TestNotNull(TEXT("Model library can be constructed"), Library);
	if (!Library)
	{
		return false;
	}

	FStylizedEmissiveModelEntry Bulb;
	Bulb.ModelId = TEXT("Bulb");
	Library->Models.Add(Bulb);

	FStylizedEmissiveModelEntry DuplicateBulb;
	DuplicateBulb.ModelId = TEXT("Bulb");
	Library->Models.Add(DuplicateBulb);

	FStylizedEmissiveModelEntry NeonPanel;
	NeonPanel.ModelId = TEXT("NeonPanel");
	Library->Models.Add(NeonPanel);

	TestNotNull(TEXT("Library resolves an entry by stable id"), Library->FindModel(TEXT("Bulb")));
	TestNull(TEXT("Library rejects unknown ids"), Library->FindModel(TEXT("Unknown")));

	const TArray<FString> Options = Library->GetModelOptions();
	TestEqual(TEXT("Picker removes duplicate ids"), Options.Num(), 2);
	TestTrue(TEXT("Picker contains Bulb"), Options.Contains(TEXT("Bulb")));
	TestTrue(TEXT("Picker contains NeonPanel"), Options.Contains(TEXT("NeonPanel")));

	return true;
}

#endif
