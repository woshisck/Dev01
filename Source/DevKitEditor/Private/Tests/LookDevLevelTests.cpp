#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Character/PlayerCharacterBase.h"
#include "Camera/YogSpringArmComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Brush.h"
#include "Engine/Level.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "LookDev/YogLookDevLightingRig.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"
#include "UObject/Package.h"
#include <limits>

namespace
{
bool IsIsolatedLookDevTest()
{
	return FApp::IsUnattended() && FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevLevelsSmoke"));
}

UWorld* LoadMapAsset(const FString& PackagePath)
{
	return LoadObject<UWorld>(nullptr, *(PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath)));
}

FString HashMap(const FString& PackagePath)
{
	TArray<uint8> Bytes;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetMapPackageExtension());
	if (!FFileHelper::LoadFileToArray(Bytes, *Filename)) return FString();
	FSHAHash Hash;
	FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
	return Hash.ToString();
}

int32 VisibleCount(UWorld* World, const TArray<FDevKitLookDevLevelEntry>& Entries)
{
	int32 Count = 0;
	for (const auto& Entry : Entries)
	{
		if (Entry.bStandalone) continue;
		for (ULevelStreaming* Streaming : World->GetStreamingLevels())
		{
			if (Streaming && Streaming->GetWorldAssetPackageName() == Entry.PackagePath && Streaming->GetShouldBeVisibleInEditor()) ++Count;
		}
	}
	return Count;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevLevelCatalogTest,
	"DevKitEditor.LookDevLevels.Catalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevLevelCatalogTest::RunTest(const FString& Parameters)
{
	const FString Root = TEXT("/YogLookDev");
	TestEqual(TEXT("Main map has requested persistent package path"), FDevKitLookDevLevelService::MainPackagePath(), Root / TEXT("LookDevMain"));
	const auto Reviews = FDevKitLookDevLevelService::GetReviewEntries();
	const auto Lights = FDevKitLookDevLevelService::GetLightingEntries();
	TestTrue(TEXT("Four requested reviews are available"), Reviews.Num() >= 4);
	TestEqual(TEXT("Neutral plus one standard environment per game level"), Lights.Num(), 4);
	const FString LightingIds[] = { TEXT("Neutral"), TEXT("L1_Torch"), TEXT("L2_Fire"), TEXT("L3_Sanctuary") };
	for (const FString& Id : LightingIds)
	{
		TestTrue(*FString::Printf(TEXT("Expected standard preset: %s"), *Id), Lights.ContainsByPredicate([&Id](const auto& Entry) { return Entry.Id == Id; }));
	}
	for (const FString& RetiredPath : FDevKitLookDevLevelService::GetRetiredLightingPackagePaths())
	{
		TestFalse(*FString::Printf(TEXT("Retired light is no longer selectable: %s"), *RetiredPath), Lights.ContainsByPredicate([&RetiredPath](const auto& Entry) { return Entry.PackagePath == RetiredPath; }));
	}
	const FString ReviewNames[] = { TEXT("Review"), TEXT("Character"), TEXT("Building"), TEXT("Prop") };
	for (const FString& Name : ReviewNames)
	{
		const FString Expected = Root / TEXT("ReviewLevel") / (TEXT("LookDev_") + Name);
		TestTrue(*Expected, Reviews.ContainsByPredicate([&Expected](const auto& Entry) { return Entry.PackagePath == Expected && !Entry.bStandalone; }));
	}
	TSet<FString> Paths;
	for (const auto& Entry : Reviews)
	{
		TestFalse(TEXT("Review paths are unique"), Paths.Contains(Entry.PackagePath));
		Paths.Add(Entry.PackagePath);
		TestTrue(TEXT("Every review path is a full package name"), FPackageName::IsValidLongPackageName(Entry.PackagePath));
	}
	for (const auto& Entry : Lights)
	{
		TestFalse(TEXT("Lighting paths do not overlap other entries"), Paths.Contains(Entry.PackagePath));
		Paths.Add(Entry.PackagePath);
		TestTrue(TEXT("Lights remain in LightDev folder"), Entry.PackagePath.StartsWith(Root + TEXT("/LightDev/")));
		TestFalse(TEXT("A light never opens a separate editor world"), Entry.bStandalone);
		TestFalse(TEXT("Lighting selection describes intent and basis"), Entry.Description.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevLevelAssetContractTest,
	"DevKitEditor.LookDevLevels.SavedAssetContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevLevelAssetContractTest::RunTest(const FString& Parameters)
{
	if (!IsIsolatedLookDevTest())
	{
		AddInfo(TEXT("Saved asset validation needs an isolated -Unattended -Dev01LookDevLevelsSmoke process after LookDev setup."));
		return true;
	}
	UWorld* Main = LoadMapAsset(FDevKitLookDevLevelService::MainPackagePath());
	if (!TestNotNull(TEXT("Saved main loads"), Main)) return false;
	TestFalse(TEXT("Main supports classic streaming rather than World Partition"), Main->IsPartitionedWorld());
	const auto Reviews = FDevKitLookDevLevelService::GetReviewEntries();
	const auto Lights = FDevKitLookDevLevelService::GetLightingEntries();
	TSet<FString> RequiredSublevels;
	for (const auto& Review : Reviews) if (!Review.bStandalone) RequiredSublevels.Add(Review.PackagePath);
	for (const auto& Light : Lights) RequiredSublevels.Add(Light.PackagePath);
	// Artists may add positioned comparison levels (for example MaterialMatrix).
	// Validate the required workbench levels without rejecting authored additions.
	for (const FString& RequiredPath : RequiredSublevels)
	{
		int32 ReferenceCount = 0;
		for (const ULevelStreaming* Streaming : Main->GetStreamingLevels())
		{
			if (Streaming && Streaming->GetWorldAssetPackageName() == RequiredPath) ++ReferenceCount;
		}
		TestEqual(*FString::Printf(TEXT("Required sublevel is referenced exactly once: %s"), *RequiredPath), ReferenceCount, 1);
	}
	for (const FString& RetiredPath : FDevKitLookDevLevelService::GetRetiredLightingPackagePaths())
	{
		TestFalse(*FString::Printf(TEXT("Retired light reference is removed from Main: %s"), *RetiredPath),
			Main->GetStreamingLevels().ContainsByPredicate([&RetiredPath](const ULevelStreaming* Streaming)
			{
				return Streaming && Streaming->GetWorldAssetPackageName() == RetiredPath;
			}));
	}
	TestEqual(TEXT("Default main has one review visible"), VisibleCount(Main, Reviews), 1);
	TestEqual(TEXT("Default main has one light visible"), VisibleCount(Main, Lights), 1);
	for (ULevelStreaming* Streaming : Main->GetStreamingLevels())
	{
		TestTrue(TEXT("Saved main uses dynamic classic streaming levels"), Streaming && Streaming->IsA<ULevelStreamingDynamic>());
		if (Streaming && RequiredSublevels.Contains(Streaming->GetWorldAssetPackageName()))
		{
			TestTrue(TEXT("Required workbench sublevel preserves its authored origin"), Streaming->LevelTransform.Equals(FTransform::Identity));
		}
	}
	for (const auto& Review : Reviews)
	{
		if (Review.bStandalone) continue;
		UWorld* World = LoadMapAsset(Review.PackagePath);
		if (!TestNotNull(*Review.PackagePath, World)) return false;
		TestFalse(TEXT("Review is a classic non-partitioned level"), World->IsPartitionedWorld());
		// Run this bootstrap-only assertion before artists populate their maps.
		if (FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevBootstrapEmpty")))
		{
			int32 AuthoredActors = 0;
			for (AActor* Actor : World->PersistentLevel->Actors)
			{
				if (Actor && !Actor->IsA<AWorldSettings>() && !Actor->IsA<ABrush>()) ++AuthoredActors;
			}
			TestEqual(TEXT("Bootstrap review contains no authored models, lights or postprocess actors"), AuthoredActors, 0);
		}
	}
	for (const auto& Entry : Lights)
	{
		const auto* Preset = FDevKitCharacterLookDevPresets::Find(Entry.Id);
		if (!Preset) continue; // Custom lighting maps stay artist-authored.
		UWorld* World = LoadMapAsset(Entry.PackagePath);
		if (!TestNotNull(*Entry.PackagePath, World)) return false;
		TestFalse(TEXT("Light is a classic non-partitioned level"), World->IsPartitionedWorld());
		if (Entry.Id == TEXT("Neutral"))
		{
			// Neutral preserves the migrated AssetsPreview SCS. It is not the
			// two-ALight/APostProcessVolume fixture used by the three game scenes.
			AYogLookDevLightingRig* Rig = nullptr;
			int32 RigCount = 0;
			for (AActor* Actor : World->PersistentLevel->Actors)
			{
				if (!Actor) continue;
				TestFalse(TEXT("Neutral authored actors survive serialization"), Actor->HasAnyFlags(RF_Transient));
				if (AYogLookDevLightingRig* Candidate = Cast<AYogLookDevLightingRig>(Actor))
				{
					++RigCount;
					Rig = Candidate;
				}
			}
			TestEqual(TEXT("Neutral contains exactly one migrated native lighting rig"), RigCount, 1);
			if (TestNotNull(TEXT("Neutral AssetsPreview derives from YogLookDevLightingRig"), Rig))
			{
				TestEqual(TEXT("Neutral scenario defaults to screenshot MidContrast"), Rig->LightingScenario, EYogLookDevLightingScenario::MidContrast);
				TestEqual(TEXT("Neutral background compensation defaults to zero"), Rig->BackgroundCompensation, 0.f);
				TestEqual(TEXT("Neutral exposure compensation defaults to zero"), Rig->ExposureCompensation, 0.f);
				TestTrue(TEXT("Neutral ground is enabled"), Rig->bGrounded);
				TestTrue(TEXT("Neutral calibrator is enabled"), Rig->bCalibrator);
				TestEqual(TEXT("Neutral persisted directional intensity"), Rig->DirectionalSettings.Intensity, 2.3f);
				TestEqual(TEXT("Neutral persisted sky intensity"), Rig->SkySettings.Intensity, 1.2f);
				UDirectionalLightComponent* Directional = Rig->GetActiveDirectionalLight();
				if (TestNotNull(TEXT("Neutral active SCS directional component exists"), Directional))
				{
					TestEqual(TEXT("Neutral activates the authored Mid component"), Directional->GetFName(), FName(TEXT("DirectionalLight_Mid")));
					TestTrue(TEXT("Neutral active directional is visible"), Directional->IsVisible());
					TestEqual(TEXT("Neutral rendered directional intensity follows saved defaults"), Directional->Intensity, 2.3f);
					TestEqual(TEXT("Neutral directional color is white"), Directional->LightColor, FColor::White);
					TestEqual(TEXT("Neutral directional indirect multiplier is one"), Directional->IndirectLightingIntensity, 1.f);
					TestEqual(TEXT("Neutral directional volumetric multiplier is one"), Directional->VolumetricScatteringIntensity, 1.f);
				}
				TInlineComponentArray<UDirectionalLightComponent*> Directionals;
				Rig->GetComponents(Directionals);
				int32 VisibleDirectionals = 0;
				for (const UDirectionalLightComponent* Light : Directionals) if (Light && Light->IsVisible()) ++VisibleDirectionals;
				TestEqual(TEXT("Neutral preserves all three authored directional calibrations"), Directionals.Num(), 3);
				TestEqual(TEXT("Neutral enables only one directional calibration"), VisibleDirectionals, 1);
				USkyLightComponent* Sky = Rig->GetSkyLight();
				if (TestNotNull(TEXT("Neutral SCS skylight exists"), Sky))
				{
					TestTrue(TEXT("Neutral sky is visible"), Sky->IsVisible());
					TestEqual(TEXT("Neutral rendered sky intensity follows saved defaults"), Sky->Intensity, 1.2f);
					TestEqual(TEXT("Neutral sky color is white"), Sky->LightColor, FColor::White);
					TestEqual(TEXT("Neutral sky indirect multiplier is one"), Sky->IndirectLightingIntensity, 1.f);
					TestEqual(TEXT("Neutral sky volumetric multiplier is one"), Sky->VolumetricScatteringIntensity, 1.f);
					TestEqual(TEXT("Neutral sky occlusion maximum distance"), Sky->OcclusionMaxDistance, 1000.f);
					TestEqual(TEXT("Neutral sky occlusion exponent"), Sky->OcclusionExponent, 1.f);
					TestEqual(TEXT("Neutral sky minimum occlusion"), Sky->MinOcclusion, 0.f);
					TestEqual(TEXT("Neutral sky occlusion tint"), Sky->OcclusionTint, FColor::Black);
				}
				TInlineComponentArray<UPostProcessComponent*> PostProcesses;
				Rig->GetComponents(PostProcesses);
				UPostProcessComponent* ExposurePost = nullptr;
				int32 ExposurePostCount = 0;
				for (UPostProcessComponent* Post : PostProcesses)
				{
					if (Post && Post->GetFName() == TEXT("PostProcess"))
					{
						ExposurePost = Post;
						++ExposurePostCount;
					}
				}
				TestEqual(TEXT("Neutral has one authored exposure postprocess component"), ExposurePostCount, 1);
				if (TestNotNull(TEXT("Neutral exposure postprocess exists"), ExposurePost))
				{
					TestTrue(TEXT("Neutral exposure postprocess is enabled"), ExposurePost->bEnabled);
					TestTrue(TEXT("Neutral explicitly overrides exposure method"), ExposurePost->Settings.bOverride_AutoExposureMethod);
					TestEqual(TEXT("Neutral exposure is manual"), ExposurePost->Settings.AutoExposureMethod.GetValue(), AEM_Manual);
					TestTrue(TEXT("Neutral explicitly overrides exposure compensation"), ExposurePost->Settings.bOverride_AutoExposureBias);
					TestEqual(TEXT("Neutral rendered exposure compensation is zero"), ExposurePost->Settings.AutoExposureBias, 0.f);
				}
			}
			continue;
		}
		int32 LightCount = 0;
		int32 PostCount = 0;
		int32 SkyCount = 0;
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (!Actor) continue;
			TestFalse(TEXT("Saved authored lighting actors survive serialization"), Actor->HasAnyFlags(RF_Transient));
			if (Actor->IsA<ASkyLight>()) ++SkyCount;
			if (ALight* Light = Cast<ALight>(Actor))
			{
				++LightCount;
				ULightComponent* Component = Light->GetLightComponent();
				TestEqual(TEXT("Native diffuse scale is conserved"), Component->DiffuseScale, 1.f);
				TestEqual(TEXT("Native specular scale is conserved"), Component->SpecularScale, 1.f);
				TestEqual(TEXT("Indirect lighting has no artificial multiplier"), Component->IndirectLightingIntensity, 1.f);
				if (auto* Local = Cast<ULocalLightComponent>(Component))
				{
					TestEqual(TEXT("Local light uses physical lumens"), Local->IntensityUnits, ELightUnits::Lumens);
					TestEqual(TEXT("Exposure does not alter emitted light power"), Local->InverseExposureBlend, 0.f);
				}
				if (auto* Point = Cast<UPointLightComponent>(Component)) TestTrue(TEXT("Point light retains inverse-square attenuation"), Point->bUseInverseSquaredFalloff);
			}
			if (auto* Post = Cast<APostProcessVolume>(Actor))
			{
				++PostCount;
				TestEqual(TEXT("Exposure is manual"), Post->Settings.AutoExposureMethod.GetValue(), AEM_Manual);
				TestTrue(TEXT("Shutter gives stated EV100"), FMath::IsNearlyEqual(Post->Settings.CameraShutterSpeed, FMath::Pow(2.f, Preset->EV100) / 16.f));
				TestFalse(TEXT("Color temperature does not drift between environments"), Post->Settings.bOverride_WhiteTemp);
				TestFalse(TEXT("White tint does not drift between environments"), Post->Settings.bOverride_WhiteTint);
			}
		}
		TestEqual(TEXT("One key and one physically identified fill"), LightCount, 2);
		TestEqual(TEXT("One fixed exposure volume"), PostCount, 1);
		TestEqual(TEXT("No uncalibrated environment cubemap is added"), SkyCount, 0);
	}
	TMap<FString, FString> Before;
	for (const auto& Review : Reviews) if (!Review.bStandalone) Before.Add(Review.PackagePath, HashMap(Review.PackagePath));
	for (const auto& Light : Lights) Before.Add(Light.PackagePath, HashMap(Light.PackagePath));
	Before.Add(FDevKitLookDevLevelService::MainPackagePath(), HashMap(FDevKitLookDevLevelService::MainPackagePath()));
	TArray<FString> Created;
	FText Message;
	TestTrue(TEXT("Repeating setup succeeds without altering authored levels"), FDevKitLookDevLevelService::CreateMissingAssets(Message, &Created));
	TestEqual(TEXT("Repeating setup creates no duplicate maps"), Created.Num(), 0);
	for (const auto& Pair : Before) TestEqual(*FString::Printf(TEXT("Preserve bytes: %s"), *Pair.Key), HashMap(Pair.Key), Pair.Value);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevLevelSwitchTest,
	"DevKitEditor.LookDevLevels.ExclusiveSwitching", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevLevelSwitchTest::RunTest(const FString& Parameters)
{
	if (!IsIsolatedLookDevTest())
	{
		AddInfo(TEXT("Map-switch tests need an isolated -Unattended -Dev01LookDevLevelsSmoke process; interactive maps are preserved."));
		return true;
	}
	FText Message;
	if (!TestTrue(TEXT("Open persistent LookDev main"), FDevKitLookDevLevelService::OpenMain(Message)))
	{
		AddError(Message.ToString());
		return false;
	}
	const auto Reviews = FDevKitLookDevLevelService::GetReviewEntries();
	const auto Lights = FDevKitLookDevLevelService::GetLightingEntries();
	UWorld* World = GEditor->GetEditorWorldContext().World();
	TestEqual(TEXT("Main remains the editor's loaded world"), World->GetPackage()->GetName(), FDevKitLookDevLevelService::MainPackagePath());
	TMap<UPackage*, bool> DirtyBefore;
	DirtyBefore.Add(World->GetPackage(), World->GetPackage()->IsDirty());
	ULevel* DirtyReviewLevel = nullptr;
	for (ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (!Streaming || !Streaming->GetLoadedLevel()) continue;
		ULevel* Level = Streaming->GetLoadedLevel();
		DirtyBefore.Add(Level->GetPackage(), Level->GetPackage()->IsDirty());
		if (Streaming->GetWorldAssetPackageName() == Reviews[0].PackagePath) DirtyReviewLevel = Level;
	}
	if (!TestNotNull(TEXT("First review is loaded for unsaved-authoring preservation"), DirtyReviewLevel)) return false;
	UPackage* DirtyReviewPackage = DirtyReviewLevel->GetPackage();
	const bool bOriginalReviewDirty = DirtyReviewPackage->IsDirty();
	AWorldSettings* DirtySettings = DirtyReviewLevel->GetWorldSettings();
	const FName PendingTag(TEXT("Dev01.LookDevTest.UnsavedArtistEdit"));
	DirtySettings->Tags.Add(PendingTag);
	DirtyReviewPackage->SetDirtyFlag(true);
	DirtyBefore[DirtyReviewPackage] = true;
	ON_SCOPE_EXIT
	{
		DirtySettings->Tags.Remove(PendingTag);
		DirtyReviewPackage->SetDirtyFlag(bOriginalReviewDirty);
		for (const auto& Pair : DirtyBefore)
		{
			if (Pair.Key != DirtyReviewPackage) Pair.Key->SetDirtyFlag(Pair.Value);
		}
	};
	TestFalse(TEXT("Unknown review cannot touch the current world"), FDevKitLookDevLevelService::LoadReview(TEXT("/Game/MissingReview"), Message));
	TestFalse(TEXT("Unknown lighting cannot touch the current world"), FDevKitLookDevLevelService::SwitchLighting(TEXT("/Game/MissingLight"), Message));
	for (const auto& Review : Reviews)
	{
		if (Review.bStandalone) continue;
		TestTrue(*FString::Printf(TEXT("Load %s"), *Review.PackagePath), FDevKitLookDevLevelService::LoadReview(Review.PackagePath, Message));
		TestEqual(TEXT("Only one review is visible"), VisibleCount(World, Reviews), 1);
		TestEqual(TEXT("Displayed active review reflects actual streaming state"), FDevKitLookDevLevelService::GetCurrentReviewPath(), Review.PackagePath);
		TestEqual(TEXT("Artist placement targets selected review rather than persistent or light level"), World->GetCurrentLevel()->GetPackage()->GetName(), Review.PackagePath);
		for (const auto& Light : Lights)
		{
			TestTrue(*FString::Printf(TEXT("Switch %s"), *Light.PackagePath), FDevKitLookDevLevelService::SwitchLighting(Light.PackagePath, Message));
			TestEqual(TEXT("Only one lighting rig is visible"), VisibleCount(World, Lights), 1);
			TestEqual(TEXT("Lighting switching preserves review selection"), FDevKitLookDevLevelService::GetCurrentReviewPath(), Review.PackagePath);
			TestEqual(TEXT("Active lighting reflects loaded sublevel state"), FDevKitLookDevLevelService::GetCurrentLightingPath(), Light.PackagePath);
			TestEqual(TEXT("Light clicks keep geometry placement in the selected review"), World->GetCurrentLevel()->GetPackage()->GetName(), Review.PackagePath);
			for (const auto& Pair : DirtyBefore)
			{
				TestEqual(*FString::Printf(TEXT("Preview switches preserve dirty state: %s"), *Pair.Key->GetName()), Pair.Key->IsDirty(), Pair.Value);
			}
			bool bDirtyReviewStillLoaded = false;
			for (ULevelStreaming* Streaming : World->GetStreamingLevels())
			{
				if (Streaming && Streaming->GetWorldAssetPackageName() == Reviews[0].PackagePath)
				{
					bDirtyReviewStillLoaded = Streaming->GetLoadedLevel() == DirtyReviewLevel;
				}
			}
			TestTrue(TEXT("Hidden dirty review retains the same loaded authored level"), bDirtyReviewStillLoaded);
			TestTrue(TEXT("Unsaved artist edits survive every Review/light choice"), DirtySettings->ActorHasTag(PendingTag));
		}
	}
	// Exercise discovery's AddLevelToWorld path without creating or writing another asset:
	// drop only an in-memory reference to an existing hidden lighting map, then let it be repaired.
	ULevelStreaming* MissingReference = nullptr;
	for (ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (Streaming && Streaming->GetWorldAssetPackageName() == Lights[0].PackagePath) MissingReference = Streaming;
	}
	if (TestNotNull(TEXT("Existing hidden light reference is available for repair fixture"), MissingReference))
	{
		ULevel* ActiveReviewBeforeAttach = World->GetCurrentLevel();
		const FString MainHashBeforeRepair = HashMap(FDevKitLookDevLevelService::MainPackagePath());
		World->RemoveStreamingLevel(MissingReference);
		TestTrue(TEXT("Missing in-memory light reference is attached on next selection"), FDevKitLookDevLevelService::SwitchLighting(Lights[0].PackagePath, Message));
		TestEqual(TEXT("Attaching a newly discovered child restores the original Review placement target"), World->GetCurrentLevel(), ActiveReviewBeforeAttach);
		TestEqual(TEXT("Repair and preview never save main implicitly"), HashMap(FDevKitLookDevLevelService::MainPackagePath()), MainHashBeforeRepair);
		TestEqual(TEXT("Repair retains a single visible lighting environment"), VisibleCount(World, Lights), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevCameraReferenceTest,
	"DevKitEditor.LookDevLevels.CameraReferenceSafety", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevCameraReferenceTest::RunTest(const FString& Parameters)
{
	FText Message;
	FDevKitLookDevCameraReference Camera;
	TestFalse(TEXT("Empty session reference is not an actual game camera"), Camera.IsValid());
	TestFalse(TEXT("Invalid reference never moves a viewport"), FDevKitLookDevLevelService::ApplyCameraReference(Camera, Message));
	TestTrue(TEXT("Manual reference can be made without an editor-world write"), FDevKitLookDevLevelService::MakeManualCamera(2300.f, 50.f, -47.f, 45.f, Camera, Message));
	TestTrue(TEXT("Manual camera is valid"), Camera.IsValid());
	TestFalse(TEXT("Manual values are never claimed to be captured PIE values"), Camera.bFromPIE);
	TestTrue(TEXT("Manual source explicitly says it is not an actual game camera"), Camera.Source.ToString().Contains(TEXT("不是实际游戏镜头")));
	TestTrue(TEXT("Requested distance is preserved"), FMath::IsNearlyEqual(Camera.TargetToCameraOffset.Size(), 2300.0, 0.01));
	TestEqual(TEXT("Manual FOV is preserved"), Camera.FieldOfView, 50.f);
	TestEqual(TEXT("Manual pitch is preserved"), Camera.Rotation.Pitch, -47.0);
	const FDevKitLookDevCameraReference Before = Camera;
	TestFalse(TEXT("Zero distance is rejected"), FDevKitLookDevLevelService::MakeManualCamera(0.f, 50.f, -47.f, 45.f, Camera, Message));
	TestFalse(TEXT("Unusable FOV is rejected"), FDevKitLookDevLevelService::MakeManualCamera(2300.f, 180.f, -47.f, 45.f, Camera, Message));
	TestFalse(TEXT("Nonfinite distance is rejected"), FDevKitLookDevLevelService::MakeManualCamera(std::numeric_limits<float>::infinity(), 50.f, -47.f, 45.f, Camera, Message));
	TestFalse(TEXT("Nonfinite angle is rejected"), FDevKitLookDevLevelService::MakeManualCamera(2300.f, 50.f, std::numeric_limits<float>::quiet_NaN(), 45.f, Camera, Message));
	TestFalse(TEXT("Infinite yaw is rejected before rotator math"), FDevKitLookDevLevelService::MakeManualCamera(2300.f, 50.f, -47.f, std::numeric_limits<float>::infinity(), Camera, Message));
	TestFalse(TEXT("Nonfinite FOV is rejected"), FDevKitLookDevLevelService::MakeManualCamera(2300.f, std::numeric_limits<float>::quiet_NaN(), -47.f, 45.f, Camera, Message));
	FDevKitLookDevCameraReference InvalidRotation = Before;
	InvalidRotation.Rotation.Pitch = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Reference validation rejects NaN pitch without rotator math"), InvalidRotation.IsValid());
	InvalidRotation.Rotation = Before.Rotation;
	InvalidRotation.Rotation.Yaw = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("Reference validation rejects infinite yaw without rotator math"), InvalidRotation.IsValid());
	InvalidRotation.Rotation = Before.Rotation;
	InvalidRotation.Rotation.Roll = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Reference validation rejects NaN roll without rotator math"), InvalidRotation.IsValid());
	TestTrue(TEXT("Failed inputs preserve the previous valid reference"), Camera.TargetToCameraOffset.Equals(Before.TargetToCameraOffset) && Camera.Rotation.Equals(Before.Rotation) && Camera.FieldOfView == Before.FieldOfView);
	TestFalse(TEXT("No implicit player-BP selection/fallback"), FDevKitLookDevLevelService::ReadPlayerCameraBlueprint(FString(), Camera, Message));
	if (!GEditor || !GEditor->PlayWorld)
	{
		TestFalse(TEXT("No synthetic PIE camera when PIE is not running"), FDevKitLookDevLevelService::CapturePIECamera(Camera, Message));
		TestFalse(TEXT("Failed capture preserves the manual source"), Camera.bFromPIE);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevCameraDefaultsAndDiagnosticsTest,
	"DevKitEditor.LookDevLevels.CameraDefaultsAndReadOnlyDiagnostics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevCameraDefaultsAndDiagnosticsTest::RunTest(const FString& Parameters)
{
	const APlayerCharacterBase* Defaults = GetDefault<APlayerCharacterBase>();
	if (!TestNotNull(TEXT("Native player camera defaults exist"), Defaults) || !TestNotNull(TEXT("Native camera arm exists"), Defaults->GetCameraBoom())) return false;
	UPackage* Package = Defaults->GetPackage();
	const bool bDirtyBefore = Package->IsDirty();
	const FRotator RotationBefore = Defaults->GetCameraBoom()->GetRelativeRotation();
	FDevKitLookDevCameraReference Camera;
	FText Message;
	TestTrue(TEXT("Player class camera properties can be read without instancing a player"), FDevKitLookDevLevelService::ReadPlayerCameraClass(APlayerCharacterBase::StaticClass(), Camera, Message));
	TestEqual(TEXT("Uses the FOV which BeginPlay applies"), Camera.FieldOfView, Defaults->DefaultCameraFOV);
	TestFalse(TEXT("Class defaults are not labelled as live PIE"), Camera.bFromPIE);
	TestTrue(TEXT("Class source warns about runtime overrides"), Camera.Source.ToString().Contains(TEXT("不包含运行时")));
	TestTrue(TEXT("Reading CDO does not change its camera rotation"), RotationBefore.Equals(Defaults->GetCameraBoom()->GetRelativeRotation()));
	TestEqual(TEXT("Reading CDO does not dirty an asset"), Package->IsDirty(), bDirtyBefore);
	const TCHAR* Names[] = { TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.DiffuseIndirect.Allow"), TEXT("r.ReflectionMethod"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.Shadow.Virtual.Enable"), TEXT("sg.ShadowQuality"), TEXT("r.ScreenPercentage") };
	TMap<FString, FString> Before;
	for (const TCHAR* Name : Names) if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name)) Before.Add(Name, CVar->GetString());
	const FText Diagnostics = FDevKitLookDevLevelService::GetRenderingDiagnostics();
	TestTrue(TEXT("Diagnosis explicitly labels read-only state"), Diagnostics.ToString().Contains(TEXT("只读状态")));
	TestTrue(TEXT("VSM state is visible"), Diagnostics.ToString().Contains(TEXT("VSM 请求=")));
	TestTrue(TEXT("Selected material diagnosis is part of the read-only status"), Diagnostics.ToString().Contains(TEXT("已加载材质状态（只读，不迁移资产）")));
	for (const auto& Pair : Before)
	{
		const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Pair.Key);
		TestEqual(*FString::Printf(TEXT("Diagnosis preserves %s"), *Pair.Key), CVar->GetString(), Pair.Value);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevSelectedMaterialDiagnosticsTest,
	"DevKitEditor.LookDevLevels.SelectedMaterialDiagnostics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevSelectedMaterialDiagnosticsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	USelection* Selection = GEditor ? GEditor->GetSelectedActors() : nullptr;
	if (!FApp::IsUnattended() || !World || !Selection)
	{
		AddInfo(TEXT("Transient material/selection fixtures require an isolated unattended editor test process."));
		return true;
	}
	TArray<TWeakObjectPtr<UObject>> PreviousSelection;
	for (FSelectionIterator It(*Selection); It; ++It) PreviousSelection.Add(*It);
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags = RF_Transient;
	AActor* Subject = World->SpawnActor<AActor>(SpawnParams);
	if (!TestNotNull(TEXT("Transient subject can be created without loading a Review asset"), Subject)) return false;
	ON_SCOPE_EXIT
	{
		Selection->BeginBatchSelectOperation();
		Selection->DeselectAll();
		for (const TWeakObjectPtr<UObject>& Previous : PreviousSelection) if (Previous.IsValid()) Selection->Select(Previous.Get());
		Selection->EndBatchSelectOperation(false);
		World->DestroyActor(Subject, false, false);
	};
	Selection->BeginBatchSelectOperation();
	Selection->DeselectAll();
	Selection->EndBatchSelectOperation(false);
	TestTrue(TEXT("Empty selection explains how to inspect actual materials"), FDevKitLookDevLevelService::GetRenderingDiagnostics().ToString().Contains(TEXT("请在 Review 关卡中选择一个角色或物件")));
	UMaterial* Unlit = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* DefaultLit = NewObject<UMaterial>(GetTransientPackage());
	UMaterial* StylizedLit = NewObject<UMaterial>(GetTransientPackage());
	Unlit->SetShadingModel(MSM_Unlit);
	DefaultLit->SetShadingModel(MSM_DefaultLit);
	StylizedLit->SetShadingModel(MSM_StylizedCharacterLit);
	UMaterialInterface* Materials[] = { Unlit, DefaultLit, StylizedLit };
	UStaticMesh* MeshAsset = NewObject<UStaticMesh>(GetTransientPackage());
	for (int32 Slot = 0; Slot < 9; ++Slot) MeshAsset->GetStaticMaterials().Add(FStaticMaterial(Materials[Slot % 3]));
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Subject);
	Subject->AddInstanceComponent(Mesh);
	Mesh->SetStaticMesh(MeshAsset);
	Selection->BeginBatchSelectOperation();
	Selection->Select(Subject);
	Selection->EndBatchSelectOperation(false);
	const bool bWorldDirtyBeforeRead = World->GetPackage()->IsDirty();
	const FString Status = FDevKitLookDevLevelService::GetRenderingDiagnostics().ToString();
	TestTrue(TEXT("Unlit cannot be confused with a failed lighting parameter"), Status.Contains(TEXT("Unlit → 不参加真实灯光")));
	TestTrue(TEXT("Default Lit explains the global-only art controls"), Status.Contains(TEXT("DefaultLit → 原生受光，只参加全局调色")));
	TestTrue(TEXT("Stylized character lit explains its supported response"), Status.Contains(TEXT("StylizedCharacterLit → 支持角色连续受光")));
	TestTrue(TEXT("First eight slots include slot seven"), Status.Contains(TEXT("槽 7：")));
	TestFalse(TEXT("Ninth slot is not expanded"), Status.Contains(TEXT("槽 8：")));
	TestTrue(TEXT("Additional slot count is explicit"), Status.Contains(TEXT("另有 1 个材质槽未展开")));
	TestEqual(TEXT("Diagnosis does not dirty the world package"), World->GetPackage()->IsDirty(), bWorldDirtyBeforeRead);
	for (int32 Slot = 0; Slot < 9; ++Slot) TestEqual(TEXT("Diagnosis leaves all material assignments unchanged"), Mesh->GetMaterial(Slot), Materials[Slot % 3]);
	return true;
}

#endif
