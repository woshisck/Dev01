#include "Misc/AutomationTest.h"

#include "CelesLightEncoder.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesLightEncoderBuildsMaterialRowsTest,
	"CelesLight.Encoding.BuildsMaterialRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesLightEncoderBuildsMaterialRowsTest::RunTest(const FString& Parameters)
{
	FCelesLightSourceData Source;
	Source.WorldPosition = FVector(10.0, 20.0, 30.0);
	Source.Radius = 400.0f;
	Source.Intensity = 7.5f;
	Source.Color = FLinearColor(0.25f, 0.5f, 0.75f, 1.0f);
	Source.bFillLight = true;
	Source.SmoothStepMin = 0.2f;
	Source.SmoothStepMax = 0.8f;
	Source.SpecularOffset = 0.35f;
	Source.EffectType = 3;

	const TArray<FLinearColor> Pixels = FCelesLightEncoder::EncodeLight(Source);

	TestEqual(TEXT("One light encodes to four texels"), Pixels.Num(), 4);
	TestEqual(TEXT("Data0 stores world position and valid flag"), Pixels[0], FLinearColor(10.0f, 20.0f, 30.0f, 1.0f));
	TestEqual(TEXT("Data1 stores radius, intensity, and fill flag"), Pixels[1], FLinearColor(400.0f, 7.5f, 1.0f, 0.0f));
	TestEqual(TEXT("Data2 stores color"), Pixels[2], FLinearColor(0.25f, 0.5f, 0.75f, 1.0f));
	TestEqual(TEXT("Data3 stores smooth range and extensions"), Pixels[3], FLinearColor(0.2f, 0.8f, 0.35f, 3.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesLightDefaultsKeepLightInfoSmallTest,
	"CelesLight.Encoding.DefaultsKeepLightInfoSmall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesLightDefaultsKeepLightInfoSmallTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The default light-info RT stores four lights"), CelesLight::DefaultLightInfoCount, 4);
	TestEqual(TEXT("The largest supported light-info RT stores sixteen lights"), CelesLight::MaxLightInfoCount, 16);
	return true;
}

#endif
