#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StylizedEmissivePresetEditProxy.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCelesLightEmissiveColorHDRRoundTripTest,
	"CelesLightEditor.EmissiveLibrary.LinearColorHDRRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesLightEmissiveColorHDRRoundTripTest::RunTest(const FString& Parameters)
{
	const FStructProperty* ColorProperty = FindFProperty<FStructProperty>(
		UStylizedEmissivePresetEditProxy::StaticClass(), GET_MEMBER_NAME_CHECKED(UStylizedEmissivePresetEditProxy, LightColor));
	if (!TestNotNull(TEXT("Light color is exposed to the native property editor"), ColorProperty)) return false;
	TestEqual(TEXT("Native color swatch uses linear HDR color"), ColorProperty->Struct->GetFName(), NAME_LinearColor);
	TestFalse(TEXT("Light color has no 0..1 clamp metadata"), ColorProperty->HasMetaData(TEXT("ClampMax")));

	TStrongObjectPtr<UStylizedEmissivePresetEditProxy> Proxy(NewObject<UStylizedEmissivePresetEditProxy>(GetTransientPackage()));
	FStylizedEmissiveModelEntry Entry;
	Entry.LightColor = FLinearColor(4.5f, 2.0f, 0.5f, 0.75f);
	Proxy->LoadFromEntry(Entry);
	TestTrue(TEXT("Loading a preset preserves HDR channels and alpha"), Proxy->LightColor == Entry.LightColor);
	Proxy->LightColor = FLinearColor(8.0f, 3.0f, 1.0f, 0.25f);
	Proxy->WriteToEntry(Entry);
	TestTrue(TEXT("Writing a preset preserves HDR channels and alpha"), Entry.LightColor == Proxy->LightColor);
	return true;
}

#endif
