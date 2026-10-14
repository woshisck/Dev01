#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LookDev/YogLookDevLightingRig.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
template <typename T>
T* FindMigratedRigComponent(const AActor* Actor, FName Name)
{
    TInlineComponentArray<T*> Components;
    Actor->GetComponents(Components);
    for (T* Component : Components)
        if (Component && Component->GetFName() == Name) return Component;
    return nullptr;
}

bool IsLookDevContentPackage(const UPackage* Package)
{
    const FString Name = Package->GetName();
    return Name.StartsWith(TEXT("/YogLookDev/")) || Name.StartsWith(TEXT("/Game/Art/Map/LookDev/"))
        || Name.StartsWith(TEXT("/Game/LookDev/"));
}

bool ReadAssetFingerprint(const FString& Filename, FSHAHash& Hash)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Filename)) return false;
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
    return true;
}

// Some compiler versions recreate an unconnected UCS entry after removing all
// graphs. No calls, event logic, variable writes or even connected entry pins
// are allowed to survive the migration.
bool IsEmptyConstructionGraph(const UEdGraph* Graph)
{
    if (!Graph || Graph->GetFName() != TEXT("UserConstructionScript")) return false;
    for (const UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Node->GetClass()->GetFName() != TEXT("K2Node_FunctionEntry")) return false;
        for (const UEdGraphPin* Pin : Node->Pins)
            if (Pin && !Pin->LinkedTo.IsEmpty()) return false;
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLookDevMigratedAssetAssemblyTest,
    "YogLookDev.MigratedAssets.BlueprintLightingAssembly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogLookDevMigratedAssetAssemblyTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevLevelsSmoke")))
    {
        AddInfo(TEXT("Actual migrated content test is opt-in: -Dev01LookDevLevelsSmoke. No assets were loaded."));
        return true;
    }

    // Remember pre-existing dirty state before loading any authored assets.
    // This test never saves, compiles, reparents or changes the active map.
    TMap<TWeakObjectPtr<UPackage>, bool> InitialDirtyStates;
    for (TObjectIterator<UPackage> It; It; ++It)
        if (IsLookDevContentPackage(*It)) InitialDirtyStates.Add(*It, It->IsDirty());
    TMap<FString, FSHAHash> FileFingerprints;
    for (const TCHAR* PackageName : {
        TEXT("/YogLookDev/Assets/AssetsPreview_BP"),
        TEXT("/YogLookDev/LightDev/LookDev_Light_Neutral"),
        TEXT("/Game/Art/Map/LookDev/Assets/AssetsPreview_BP"),
        TEXT("/Game/Art/Map/LookDev/LightDev/LookDev_Light_Neutral") })
    {
        FString Filename;
        if (FPackageName::DoesPackageExist(PackageName, &Filename))
        {
            FSHAHash Hash;
            if (TestTrue(*FString::Printf(TEXT("Can fingerprint %s"), PackageName), ReadAssetFingerprint(Filename, Hash)))
                FileFingerprints.Add(Filename, Hash);
        }
    }
    ON_SCOPE_EXIT
    {
        for (TObjectIterator<UPackage> It; It; ++It)
        {
            if (!IsLookDevContentPackage(*It)) continue;
            const bool* Initial = InitialDirtyStates.Find(TWeakObjectPtr<UPackage>(*It));
            TestEqual(*FString::Printf(TEXT("Authored package dirty state is preserved: %s"), *It->GetName()),
                It->IsDirty(), Initial ? *Initial : false);
        }
        for (const auto& Pair : FileFingerprints)
        {
            FSHAHash Current;
            const bool bRead = ReadAssetFingerprint(Pair.Key, Current);
            TestTrue(*FString::Printf(TEXT("Authored package bytes are unchanged: %s"), *Pair.Key), bRead && Current == Pair.Value);
        }
    };

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr,
        TEXT("/YogLookDev/Assets/AssetsPreview_BP.AssetsPreview_BP"));
    if (!TestNotNull(TEXT("Migrated real AssetsPreview_BP loads"), Blueprint)) return false;
    TestTrue(TEXT("The original Blueprint now directly inherits the native lighting behavior"),
        Blueprint->ParentClass == AYogLookDevLightingRig::StaticClass());
    TestEqual(TEXT("Legacy Blueprint variables are removed, not merely hidden"), Blueprint->NewVariables.Num(), 0);
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    for (const UEdGraph* Graph : Graphs)
        TestTrue(*FString::Printf(TEXT("No legacy graph execution: %s"), *GetNameSafe(Graph)), IsEmptyConstructionGraph(Graph));
    if (!TestNotNull(TEXT("Migrated Blueprint has a generated class"), Blueprint->GeneratedClass.Get())) return false;
    if (!TestTrue(TEXT("Generated class is a native rig subclass"), Blueprint->GeneratedClass->IsChildOf(AYogLookDevLightingRig::StaticClass()))) return false;
    if (!TestNotNull(TEXT("Authored SCS is retained"), Blueprint->SimpleConstructionScript.Get())) return false;
    for (TFieldIterator<FProperty> It(Blueprint->GeneratedClass); It; ++It)
    {
        if (!It->HasAnyPropertyFlags(CPF_Edit)) continue;
        FString Label = It->GetDisplayNameText().ToString().ToLower();
        Label.ReplaceInline(TEXT(" "), TEXT(""));
        Label.ReplaceInline(TEXT("_"), TEXT(""));
        for (const TCHAR* Removed : { TEXT("raytracingshadow"), TEXT("enabletransmission"),
            TEXT("usehardwareraytracing"), TEXT("enablechecker"), TEXT("turnlighting") })
            TestFalse(*FString::Printf(TEXT("Retired editable control does not survive in actual BP: %s"), Removed), Label == Removed);
    }

    // Check both templates and the actual assembled actor: the old UCS applied
    // these transforms at construction time, so keeping only old templates is
    // a silent migration regression even when the Blueprint compiles.
    auto CheckPreservedPostProcess = [this](const UPostProcessComponent* Post, const FString& Context)
    {
        if (!TestNotNull(*(Context + TEXT(": authored PostProcess component exists")), Post)) return;
        TestTrue(*(Context + TEXT(": vignette override is retained")), Post->Settings.bOverride_VignetteIntensity);
        TestTrue(*(Context + TEXT(": saved Neutral vignette is 0.3, not the former SCS 0.4")),
            FMath::IsNearlyEqual(Post->Settings.VignetteIntensity, 0.3f));
        TestTrue(*(Context + TEXT(": physical camera exposure override is retained")),
            Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure);
        TestFalse(*(Context + TEXT(": physical camera exposure remains disabled")),
            Post->Settings.AutoExposureApplyPhysicalCameraExposure);
    };
    bool bFoundDomeTemplate = false;
    bool bFoundCalibrationTemplate = false;
    bool bFoundPostProcessTemplate = false;
    bool bFoundSkyTemplate = false;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (!Node) continue;
        const USceneComponent* Template = Cast<USceneComponent>(Node->ComponentTemplate);
        if (!Template) continue;
        if (Node->GetVariableName() == TEXT("Skydome"))
        {
            bFoundDomeTemplate = true;
            TestTrue(TEXT("SCS freezes the authored constructed dome size"),
                Template->GetRelativeScale3D().Equals(FVector(240.104501, 240.104501, 120.052250), 0.001));
        }
        if (Node->GetVariableName() == TEXT("SpawnLocation"))
        {
            bFoundCalibrationTemplate = true;
            // Authoritative pre-migration serialized state: Saved/LookDevMigration/
            // 20261011-093050-2CB3F6C9/neutral-component-snapshot.json. The earlier
            // live audit had executed the old trace/UCS and therefore is not this baseline.
            TestTrue(TEXT("SCS freezes the visible calibrator group location"), Template->GetRelativeLocation().Equals(FVector(-70.160645, 0, 15), 0.001));
        }
        if (Node->GetVariableName() == TEXT("PostProcess"))
        {
            bFoundPostProcessTemplate = true;
            CheckPreservedPostProcess(Cast<UPostProcessComponent>(Template), TEXT("Migrated SCS"));
        }
        if (Node->GetVariableName() == TEXT("Skylight"))
        {
            bFoundSkyTemplate = true;
            const auto* SkyTemplate = Cast<USkyLightComponent>(Template);
            if (TestNotNull(TEXT("SCS skylight has the expected component class"), SkyTemplate))
                TestEqual(TEXT("SCS retains the saved Neutral HDRI angle, not the former template angle"), SkyTemplate->SourceCubemapAngle, -120.f);
        }
    }
    TestTrue(TEXT("Skydome template exists"), bFoundDomeTemplate);
    TestTrue(TEXT("Calibration group template exists"), bFoundCalibrationTemplate);
    TestTrue(TEXT("PostProcess template exists"), bFoundPostProcessTemplate);
    TestTrue(TEXT("Skylight template exists"), bFoundSkyTemplate);

    UWorld* Neutral = LoadObject<UWorld>(nullptr, TEXT("/YogLookDev/LightDev/LookDev_Light_Neutral.LookDev_Light_Neutral"));
    if (TestNotNull(TEXT("Migrated Neutral map loads without activating it"), Neutral) && Neutral->PersistentLevel)
    {
        int32 RigCount = 0;
        for (const AActor* Actor : Neutral->PersistentLevel->Actors)
            if (const AYogLookDevLightingRig* SavedRig = Cast<AYogLookDevLightingRig>(Actor))
            {
                ++RigCount;
                TestTrue(TEXT("Neutral uses the actual migrated Blueprint"), SavedRig->GetClass() == Blueprint->GeneratedClass);
                TestEqual(TEXT("Saved Neutral scenario is MidContrast"), SavedRig->LightingScenario, EYogLookDevLightingScenario::MidContrast);
                TestEqual(TEXT("Saved Neutral background compensation is zero"), SavedRig->BackgroundCompensation, 0.f);
                TestEqual(TEXT("Saved Neutral exposure compensation is zero"), SavedRig->ExposureCompensation, 0.f);
                TestTrue(TEXT("Saved Neutral uses requested Grounded and Calibrator defaults"), SavedRig->bGrounded && SavedRig->bCalibrator);
            }
        TestEqual(TEXT("Exactly one native comparison rig is serialized in Neutral"), RigCount, 1);
    }

    const FString PackageName = TEXT("/Temp/YogLookDevMigratedTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    UPackage* TestPackage = CreatePackage(*PackageName);
    TestPackage->SetFlags(RF_Transient);
    UWorld::InitializationValues Init;
    Init.CreatePhysicsScene(false).ShouldSimulatePhysics(false).CreateNavigation(false).CreateAISystem(false).CreateWorldPartition(false);
    TStrongObjectPtr<UWorld> TestWorld(UWorld::CreateWorld(EWorldType::Inactive, false,
        FName(TEXT("MigratedLookDevTest")), TestPackage, false, ERHIFeatureLevel::Num, &Init));
    if (!TestNotNull(TEXT("Independent transient test world"), TestWorld.Get())) return false;
    ON_SCOPE_EXIT
    {
        TestPackage->SetDirtyFlag(false);
        TestWorld->DestroyWorld(false);
        TestWorld->MarkObjectsPendingKill();
    };
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    Spawn.OverrideLevel = TestWorld->PersistentLevel;
    AYogLookDevLightingRig* Rig = TestWorld->SpawnActor<AYogLookDevLightingRig>(Blueprint->GeneratedClass, FTransform::Identity, Spawn);
    if (!TestNotNull(TEXT("Actual migrated Blueprint assembles successfully"), Rig)) return false;
    Rig->RefreshLighting();
    auto* Dome = FindMigratedRigComponent<UStaticMeshComponent>(Rig, TEXT("Skydome"));
    auto* Calibration = FindMigratedRigComponent<USceneComponent>(Rig, TEXT("SpawnLocation"));
    auto* Character = FindMigratedRigComponent<USceneComponent>(Rig, TEXT("Character_STM"));
    auto* Sky = Rig->GetSkyLight();
    auto* Key = Rig->GetActiveDirectionalLight();
    if (!TestNotNull(TEXT("Assembled dome"), Dome) || !TestNotNull(TEXT("Assembled calibration group"), Calibration)
        || !TestNotNull(TEXT("Assembled character calibrator"), Character) || !TestNotNull(TEXT("Assembled skylight"), Sky)
        || !TestNotNull(TEXT("Assembled active directional light"), Key)) return false;
    TestTrue(TEXT("Actual dome mesh survives migration"), Dome->GetStaticMesh() != nullptr);
    TestTrue(TEXT("Actual dome retains full constructed scale"), Dome->GetRelativeScale3D().Equals(FVector(240.104501, 240.104501, 120.052250), 0.001));
    // Same serialized baseline as the SCS check above (20261011-093050-2CB3F6C9).
    TestTrue(TEXT("Actual calibration position is unchanged"), Calibration->GetRelativeLocation().Equals(FVector(-70.160645, 0, 15), 0.001));
    TestTrue(TEXT("Character calibration offset is unchanged"), Character->GetRelativeLocation().Equals(FVector(-245, 0, -10), 0.001));
    TestTrue(TEXT("Grayscale HDRI is valid and migrated"), Sky->Cubemap && Sky->Cubemap->GetPathName()
        == TEXT("/YogLookDev/Assets/HDRI/EpicQuadPanorama_Gray.EpicQuadPanorama_Gray"));
    TestEqual(TEXT("Skylight still samples the specified grayscale cubemap"), Sky->SourceType.GetValue(), SLS_SpecifiedCubemap);
    TestEqual(TEXT("Mid key intensity comes from native defaults"), Key->Intensity, 2.3f);
    TestEqual(TEXT("Mid sky intensity comes from native defaults"), Sky->Intensity, 1.2f);
    TestEqual(TEXT("Assembled skylight preserves saved Neutral HDRI rotation"), Sky->SourceCubemapAngle, -120.f);
    CheckPreservedPostProcess(FindMigratedRigComponent<UPostProcessComponent>(Rig, TEXT("PostProcess")), TEXT("Spawned Blueprint"));
    if (const auto* Checker = FindMigratedRigComponent<UPostProcessComponent>(Rig, TEXT("PostProcess_PBRChecker")))
        TestFalse(TEXT("Retained checker SCS component cannot affect the preview"), Checker->bEnabled || Checker->IsVisible());
    auto CheckBackdrop = [this, Rig](float FarValue, float MaskRadius, float PDO)
    {
        const auto* Mesh = FindMigratedRigComponent<UStaticMeshComponent>(Rig, TEXT("Skydome"));
        const auto* Material = Mesh ? Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)) : nullptr;
        if (!TestNotNull(TEXT("Native construction creates the backdrop MID"), Material)) return;
        for (const auto& Pair : TArray<TPair<FName, float>> { { TEXT("Far Value"), FarValue }, { TEXT("Mask Radius"), MaskRadius }, { TEXT("PDO"), PDO } })
        {
            float Value = -1.f;
            const bool bFound = Material->GetScalarParameterValue(FMaterialParameterInfo(Pair.Key), Value);
            TestTrue(*FString::Printf(TEXT("Backdrop %s matches native state"), *Pair.Key.ToString()), bFound && FMath::IsNearlyEqual(Value, Pair.Value));
        }
    };
    CheckBackdrop(0.18f, 10000.f, 0.f);
    const FRotator InitialKeyRotation = Key->GetRelativeRotation();
    const float InitialCubemapAngle = Sky->SourceCubemapAngle;
    const FVector InitialDomeScale = Dome->GetRelativeScale3D();
    Rig->DirectionalSettings.Intensity = 3.1f;
    Rig->SkySettings.Intensity = 1.7f;
    Rig->BackgroundCompensation = 1.f;
    Rig->ExposureCompensation = 0.5f;
    for (int32 Pass = 0; Pass < 3; ++Pass)
    {
        Rig->RerunConstructionScripts();
        Key = Rig->GetActiveDirectionalLight();
        Sky = Rig->GetSkyLight();
        Dome = FindMigratedRigComponent<UStaticMeshComponent>(Rig, TEXT("Skydome"));
        if (!TestNotNull(TEXT("Key survives real SCS reconstruction"), Key) || !TestNotNull(TEXT("Sky survives real SCS reconstruction"), Sky)
            || !TestNotNull(TEXT("Dome survives real SCS reconstruction"), Dome)) return false;
        TestEqual(TEXT("Panel key edits survive SCS reconstruction"), Key->Intensity, 3.1f);
        TestEqual(TEXT("Panel sky edits survive SCS reconstruction"), Sky->Intensity, 1.7f);
        TestTrue(TEXT("SCS reconstruction does not accumulate directional rotation"), Key->GetRelativeRotation().Equals(InitialKeyRotation, 0.001));
        TestEqual(TEXT("SCS reconstruction does not accumulate HDRI rotation"), Sky->SourceCubemapAngle, InitialCubemapAngle);
        TestTrue(TEXT("SCS reconstruction does not accumulate dome scale"), Dome->GetRelativeScale3D().Equals(InitialDomeScale, 0.001));
        CheckBackdrop(0.36f, 10000.f, 0.f);
        const auto* Post = FindMigratedRigComponent<UPostProcessComponent>(Rig, TEXT("PostProcess"));
        CheckPreservedPostProcess(Post, FString::Printf(TEXT("SCS reconstruction %d"), Pass));
        if (TestNotNull(TEXT("Actual authored exposure component is present"), Post))
        {
            TestEqual(TEXT("Exposure mode is manual"), Post->Settings.AutoExposureMethod.GetValue(), AEM_Manual);
            TestEqual(TEXT("Panel exposure edit survives SCS reconstruction"), Post->Settings.AutoExposureBias, 0.5f);
        }
    }
    Rig->bGrounded = false;
    Rig->RefreshLighting();
    CheckBackdrop(0.36f, 0.1f, 3000.f);
    Rig->bGrounded = true;
    Rig->RefreshLighting();
    CheckBackdrop(0.36f, 10000.f, 0.f);
    Rig->SetLightingScenario(EYogLookDevLightingScenario::HighContrast);
    CheckBackdrop(0.2f, 10000.f, 0.f);
    Rig->SetLightingScenario(EYogLookDevLightingScenario::MidContrast);
    Rig->RerunConstructionScripts();
    Key = Rig->GetActiveDirectionalLight();
    if (TestNotNull(TEXT("Mid light survives scenario roundtrip"), Key))
        TestTrue(TEXT("Scenario roundtrip restores the calibrated rotation exactly"), Key->GetRelativeRotation().Equals(InitialKeyRotation, 0.001));
    TestFalse(TEXT("Transient test world is never saved"), FPackageName::DoesPackageExist(PackageName));
    return true;
}

#endif
