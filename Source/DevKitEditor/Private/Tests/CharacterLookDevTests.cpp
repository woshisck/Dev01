#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Engine/RectLight.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/WorldSettings.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"
#include "Tools/MapCreator/DevKitMapCreatorService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevPresetContractTest,
	"DevKitEditor.CharacterLookDev.PresetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevPresetContractTest::RunTest(const FString& Parameters)
{
	TSet<FString> Ids;
	TMap<FString, float> ExposureByLayer;
	for (const auto& P : FDevKitCharacterLookDevPresets::GetPresets())
	{
		TestFalse(TEXT("Preset IDs are unique"), Ids.Contains(P.Id));
		Ids.Add(P.Id);
		TestTrue(TEXT("Finite exposure"), FMath::IsFinite(P.EV100));
		TestTrue(TEXT("Nonnegative key light"), FMath::IsFinite(P.KeyIntensity) && P.KeyIntensity >= 0);
		TestTrue(TEXT("Nonnegative fill"), FMath::IsFinite(P.FillLumens) && P.FillLumens >= 0);
		TestTrue(TEXT("Fog density is bounded"), P.FogDensity >= 0 && P.FogDensity <= 0.1f);
		if (!P.Layer.IsEmpty())
		{
			const FString Template = FDevKitCharacterLookDevPresets::GetLightTemplate(P.Layer);
			TestFalse(TEXT("Level variants have a production template reference"), Template.IsEmpty());
			TestEqual(TEXT("Level generator and LookDev share the same source"),
				FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Light"), P.Layer), Template);
			if (const float* EV = ExposureByLayer.Find(P.Layer))
			{
				TestEqual(TEXT("Bright/shadow comparisons never auto-correct exposure"), P.EV100, *EV);
			}
			ExposureByLayer.Add(P.Layer, P.EV100);
		}
	}
	TestTrue(TEXT("Neutral baseline exists"), Ids.Contains(TEXT("Neutral")));
	TestEqual(TEXT("Neutral plus exactly one standard per major level"), Ids.Num(), 4);
	TestTrue(TEXT("L1 standard exists"), Ids.Contains(TEXT("L1_Torch")));
	TestTrue(TEXT("L2 standard exists"), Ids.Contains(TEXT("L2_Fire")));
	TestTrue(TEXT("L3 standard exists"), Ids.Contains(TEXT("L3_Sanctuary")));
	for (const TPair<FString, FString>& Expected : TArray<TPair<FString, FString>>{
		{ TEXT("Neutral"), TEXT("中性环境，灯光基准") },
		{ TEXT("L1_Torch"), TEXT("L1-标准") },
		{ TEXT("L2_Fire"), TEXT("L2-标准") },
		{ TEXT("L3_Sanctuary"), TEXT("L3-标准") } })
	{
		const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(Expected.Key);
		if (TestNotNull(*FString::Printf(TEXT("Display preset exists: %s"), *Expected.Key), Preset))
		{
			TestEqual(*FString::Printf(TEXT("Display label for %s"), *Expected.Key), Preset->Label, Expected.Value);
		}
	}
	TestNull(TEXT("Unknown preset cannot silently select another environment"), FDevKitCharacterLookDevPresets::Find(TEXT("NotAPreset")));
	TestTrue(TEXT("Neutral does not pretend to have a production template"), FDevKitCharacterLookDevPresets::GetLightTemplate(TEXT("")).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevStageSmokeTest,
	"DevKitEditor.CharacterLookDev.StageSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevStageSmokeTest::RunTest(const FString& Parameters)
{
	// This test switches disposable maps. Never run it in an artist's interactive editor.
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevSmoke")))
	{
		AddInfo(TEXT("Stage smoke requires -Unattended -Dev01LookDevSmoke in a dedicated test process."));
		return true;
	}
	// Tests share an editor process. RenderSmoke may have left a disposable stage open.
	// Reset only inside this explicitly opted-in unattended test, before regular-map assertions.
	if (!TestNotNull(TEXT("Fresh regular map for isolated guard checks"), GEditor->NewMap())) return false;
	FText Message;
	TestTrue(TEXT("LookDev tab is registered"), FGlobalTabmanager::Get()->HasTabSpawner(TEXT("DevKitCharacterLookDev")));
	TestFalse(TEXT("Cannot mutate a regular editor map"), FDevKitCharacterLookDevService::ApplyPreset(TEXT("Neutral"), Message));
	GEditor->GetEditorWorldContext().World()->GetWorldSettings()->Tags.Add(TEXT("Dev01.CharacterLookDev"));
	TestFalse(TEXT("A persisted tag alone cannot claim a regular map"), FDevKitCharacterLookDevService::IsStageWorld());
	GEditor->GetEditorWorldContext().World()->GetWorldSettings()->Tags.Remove(TEXT("Dev01.CharacterLookDev"));
	UPackage* OriginalPackage = GEditor->GetEditorWorldContext().World()->GetOutermost();
	const bool bWasDirty = OriginalPackage->IsDirty();
	OriginalPackage->SetDirtyFlag(true);
	TestFalse(TEXT("Dirty regular map prevents destructive stage creation"), FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message));
	OriginalPackage->SetDirtyFlag(bWasDirty);
	TestFalse(TEXT("Invalid preset cannot create a map"), FDevKitCharacterLookDevService::CreateStage(TEXT("Missing"), false, Message));
	if (!TestTrue(TEXT("Create an isolated neutral stage"), FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message)))
	{
		AddError(Message.ToString());
		return false;
	}
	TestTrue(TEXT("Created world is identified as a LookDev stage"), FDevKitCharacterLookDevService::IsStageWorld());
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Reference mesh loads"), Mesh)) return false;
	TestTrue(TEXT("Subject loaded with original materials"), FDevKitCharacterLookDevService::SetSubject(Mesh, Message));
	bool bCheckedDirectionalKey = false;
	bool bCheckedRectKey = false;
	for (const auto& P : FDevKitCharacterLookDevPresets::GetPresets())
	{
		TestTrue(*FString::Printf(TEXT("Apply %s"), *P.Id), FDevKitCharacterLookDevService::ApplyPreset(P.Id, Message));
		bool bFoundKey = false;
		bool bFoundFill = false;
		for (TActorIterator<ALight> It(GEditor->GetEditorWorldContext().World()); It; ++It)
		{
			if (It->GetActorLabel().StartsWith(TEXT("LookDev Candidate Key")))
			{
				bFoundKey = true;
				bCheckedDirectionalKey |= It->IsA<ADirectionalLight>();
				bCheckedRectKey |= It->IsA<ARectLight>();
				TestTrue(*FString::Printf(TEXT("%s key location matches the absolute preset"), *P.Id),
					It->GetActorLocation().Equals(P.KeyLocation, 0.01f));
				TestTrue(*FString::Printf(TEXT("%s key rotation excludes the native CDO offset"), *P.Id),
					It->GetActorQuat().Equals(P.KeyRotation.Quaternion(), 0.0001f));
				TestTrue(*FString::Printf(TEXT("%s key scale excludes the native CDO scale"), *P.Id),
					It->GetActorScale3D().Equals(FVector::OneVector, 0.0001f));
				TestTrue(TEXT("Linear color survives the native light storage roundtrip"), It->GetLightComponent()->GetLightColor().Equals(P.KeyColor, 0.01f));
			}
			else if (It->GetActorLabel().StartsWith(TEXT("LookDev Candidate Broad Fill")))
			{
				bFoundFill = true;
				const FVector ExpectedPosition(-200, 200, 240);
				const FQuat ExpectedRotation = (FVector(0,0,100) - ExpectedPosition).Rotation().Quaternion();
				TestTrue(TEXT("Broad fill remains a Rect light"), It->IsA<ARectLight>());
				TestTrue(TEXT("Broad fill location remains absolute"), It->GetActorLocation().Equals(ExpectedPosition, 0.01f));
				TestTrue(TEXT("Broad fill faces the subject"), It->GetActorQuat().Equals(ExpectedRotation, 0.0001f));
				TestTrue(TEXT("Broad fill has unit scale"), It->GetActorScale3D().Equals(FVector::OneVector, 0.0001f));
			}
		}
		TestTrue(TEXT("Each preset creates its key light"), bFoundKey);
		TestTrue(TEXT("Each preset creates its broad fill"), bFoundFill);
		int32 Actors = 0;
		for (TActorIterator<AActor> It(GEditor->GetEditorWorldContext().World()); It; ++It) ++Actors;
		TestTrue(TEXT("Preset swaps do not accumulate lights or fog"), Actors < 32);
	}
	TestTrue(TEXT("Transform checks cover a Directional key"), bCheckedDirectionalKey);
	TestTrue(TEXT("Transform checks cover a Rect key"), bCheckedRectKey);
	TestTrue(TEXT("Turntable rotation works"), FDevKitCharacterLookDevService::RotateSubject(45, Message));
	TestTrue(TEXT("Record exports without a rendered viewport"), FDevKitCharacterLookDevService::ExportReport(Message));
	TestTrue(TEXT("Recreating a disposable stage leaves no world leak"), FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message));
	for (const FString& Id : { FString(TEXT("L1_Torch")), FString(TEXT("L2_Fire")), FString(TEXT("L3_Sanctuary")) })
	{
		if (!TestTrue(*FString::Printf(TEXT("Copy real light template for %s"), *Id), FDevKitCharacterLookDevService::CreateStage(Id, true, Message)))
		{
			AddError(Message.ToString());
			return false;
		}
		TestFalse(TEXT("A template copy cannot be overwritten by candidate values"), FDevKitCharacterLookDevService::ApplyPreset(TEXT("Neutral"), Message));
	}
	TestTrue(TEXT("Return from template comparison to candidate without world leak"), FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message));
	return true;
}

#endif
