#include "Misc/AutomationTest.h"

#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesLightCaptureBoxExposesRenderTargetWorkflowTest,
	"CelesLight.CaptureBox.ExposesRenderTargetWorkflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesLightCaptureBoxExposesRenderTargetWorkflowTest::RunTest(const FString& Parameters)
{
	UClass* CaptureBoxClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Script/CelesLightRuntime.CelesLightCaptureBox"));

	TestNotNull(TEXT("A placeable capture box actor is available"), CaptureBoxClass);
	if (!CaptureBoxClass)
	{
		return false;
	}

	TestNotNull(TEXT("Capture box exposes a user-assignable RT"), FindFProperty<FObjectPropertyBase>(CaptureBoxClass, TEXT("LightInfoRenderTarget")));
	TestNotNull(TEXT("Capture box exposes a per-box light limit"), FindFProperty<FIntProperty>(CaptureBoxClass, TEXT("MaxLightCount")));
	TestNotNull(TEXT("Capture box exposes an editor tick toggle"), FindFProperty<FBoolProperty>(CaptureBoxClass, TEXT("bUpdateInEditor")));
	TestNotNull(TEXT("Capture box exposes a light selection mode"), FindFProperty<FEnumProperty>(CaptureBoxClass, TEXT("LightSelectionMode")));
	TestNotNull(TEXT("Capture box exposes encoded light count status"), FindFProperty<FIntProperty>(CaptureBoxClass, TEXT("LastEncodedLightCount")));
	TestNotNull(TEXT("Capture box exposes overflow light count status"), FindFProperty<FIntProperty>(CaptureBoxClass, TEXT("LastOverflowLightCount")));
	TestNotNull(TEXT("Capture box exposes overflow light list status"), FindFProperty<FArrayProperty>(CaptureBoxClass, TEXT("LastOverflowLights")));
	TestNotNull(TEXT("Capture box exposes a manual update action"), CaptureBoxClass->FindFunctionByName(TEXT("UpdateCelesLight")));

	const UObject* Defaults = CaptureBoxClass->GetDefaultObject();
	const FIntProperty* MaxLightCountProperty = FindFProperty<FIntProperty>(CaptureBoxClass, TEXT("MaxLightCount"));
	if (Defaults && MaxLightCountProperty)
	{
		TestEqual(TEXT("Capture boxes default to four lights"), MaxLightCountProperty->GetPropertyValue_InContainer(Defaults), 4);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesLightPointLightExposesEditorBillboardTest,
	"CelesLight.PointLight.ExposesEditorBillboard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesLightPointLightExposesEditorBillboardTest::RunTest(const FString& Parameters)
{
	const UClass* PointLightClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Script/CelesLightRuntime.CelesPointLight"));

	TestNotNull(TEXT("Celes point light actor is available"), PointLightClass);
	if (!PointLightClass)
	{
		return false;
	}

	TestNotNull(TEXT("Celes point light exposes its own editor billboard"), FindFProperty<FObjectPropertyBase>(PointLightClass, TEXT("Billboard")));

	if (const UClass* EditorLibraryClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/CelesLightEditor.CelesLightEditorLibrary")))
	{
		TestNotNull(TEXT("Editor library exposes Celes point light creation"), EditorLibraryClass->FindFunctionByName(TEXT("CreateCelesPointLight")));
	}

	return true;
}

#endif
