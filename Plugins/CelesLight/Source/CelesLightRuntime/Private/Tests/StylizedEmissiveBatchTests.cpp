#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StylizedEmissiveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesStylizedEmissivePerInstanceDataTest,
	"CelesLight.StylizedEmissive.PerInstanceData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesStylizedEmissivePerInstanceDataTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Per-instance material contract contains RGB and intensity"),
		FStylizedEmissivePerInstanceData::NumCustomDataFloats,
		4);
	TestEqual(
		TEXT("Color starts at custom data index zero"),
		FStylizedEmissivePerInstanceData::ColorDataIndex,
		0);
	TestEqual(
		TEXT("Intensity uses custom data index three"),
		FStylizedEmissivePerInstanceData::IntensityDataIndex,
		3);

	const FStylizedEmissivePerInstanceData DefaultData;
	TestEqual(
		TEXT("Default visible emissive color is white"),
		DefaultData.EmissiveColor,
		FLinearColor::White);
	TestEqual(
		TEXT("Default visible emissive strength matches the material"),
		DefaultData.EmissiveIntensity,
		20.0f);
	return true;
}

#endif
