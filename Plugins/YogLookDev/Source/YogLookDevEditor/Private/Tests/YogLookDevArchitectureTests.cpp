#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/Level.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PluginDescriptor.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"
#include "Tools/CharacterLookDev/DevKitLookDevLightingBuilder.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLookDevPluginBoundaryTest,
    "YogLookDev.Architecture.PluginBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogLookDevPluginBoundaryTest::RunTest(const FString& Parameters)
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("YogLookDev"));
    if (!TestTrue(TEXT("LookDev is a mounted plugin"), Plugin.IsValid())) return false;
    TestTrue(TEXT("Plugin has content"), Plugin->CanContainContent());
    TestTrue(TEXT("Plugin mount resolves to its own Content directory"),
        FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPackageName::LongPackageNameToFilename(TEXT("/YogLookDev/LookDevMain")))),
            FPaths::ConvertRelativePathToFull(Plugin->GetContentDir())));
    const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
    TestEqual(TEXT("LookDev code remains editor-only"), Descriptor.Modules.Num(), 1);
    if (!Descriptor.Modules.IsEmpty()) TestEqual(TEXT("No runtime tool module is introduced"), Descriptor.Modules[0].Type, EHostType::Editor);
    for (const TCHAR* Provider : { TEXT("YogArt"), TEXT("YogArt_Material"), TEXT("YogArt_VFX"),
        TEXT("YogAnimSource"), TEXT("YogSFX"), TEXT("YogFrontLib") })
    {
        TestTrue(*FString::Printf(TEXT("LookDev explicitly depends on %s"), Provider),
            Descriptor.Plugins.ContainsByPredicate([Provider](const FPluginReferenceDescriptor& Reference)
            { return Reference.Name == Provider && Reference.bEnabled; }));
        const TSharedPtr<IPlugin> ProviderPlugin = IPluginManager::Get().FindPlugin(Provider);
        if (!TestTrue(*FString::Printf(TEXT("Content provider exists: %s"), Provider), ProviderPlugin.IsValid())) continue;
        TestTrue(TEXT("Provider can supply asset references"), ProviderPlugin->CanContainContent());
        TestFalse(TEXT("Content provider does not depend back on the review workbench"),
            ProviderPlugin->GetDescriptor().Plugins.ContainsByPredicate([](const FPluginReferenceDescriptor& Reference)
            { return Reference.Name == TEXT("YogLookDev") && Reference.bEnabled; }));
    }
    TestFalse(TEXT("LookDev does not activate retired combo graph content"),
        Descriptor.Plugins.ContainsByPredicate([](const FPluginReferenceDescriptor& Reference)
        { return Reference.Name == TEXT("YogComboGraph") && Reference.bEnabled; }));
    TestEqual(TEXT("Canonical main map uses plugin package namespace"),
        FDevKitLookDevLevelService::MainPackagePath(), FString(TEXT("/YogLookDev/LookDevMain")));
    for (const auto& Entry : FDevKitLookDevLevelService::GetReviewEntries())
        if (!Entry.bStandalone) TestTrue(TEXT("Managed review maps stay inside plugin"), Entry.PackagePath.StartsWith(TEXT("/YogLookDev/ReviewLevel/")));
    for (const auto& Entry : FDevKitLookDevLevelService::GetLightingEntries())
        TestTrue(TEXT("Managed light maps stay inside plugin"), Entry.PackagePath.StartsWith(TEXT("/YogLookDev/LightDev/")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLookDevLightingBuilderIsolationTest,
    "YogLookDev.Architecture.LightingBuilderIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogLookDevLightingBuilderIsolationTest::RunTest(const FString& Parameters)
{
    for (const auto& Preset : FDevKitCharacterLookDevPresets::GetPresets())
    {
        const FString PackagePath = TEXT("/YogLookDev/LightDev/__Automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
        UPackage* Package = CreatePackage(*PackagePath);
        Package->SetFlags(RF_Transient);
        UWorld::InitializationValues Init;
        Init.CreatePhysicsScene(false).ShouldSimulatePhysics(false).CreateNavigation(false).CreateAISystem(false).CreateWorldPartition(false);
        TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Inactive, false,
            FName(*FPackageName::GetLongPackageAssetName(PackagePath)), Package, false, ERHIFeatureLevel::Num, &Init));
        if (!TestNotNull(TEXT("Isolated in-memory test world"), World.Get())) return false;
        FText Message;
        TestTrue(*FString::Printf(TEXT("Serializable light rig: %s"), *Preset.Id), YogLookDev::BuildLightingLevel(World.Get(), Preset.Id, Message));
        int32 LightCount = 0;
        int32 PostCount = 0;
        for (AActor* Actor : World->PersistentLevel->Actors)
        {
            if (!Actor) continue;
            if (ALight* Light = Cast<ALight>(Actor))
            {
                ++LightCount;
                TestFalse(TEXT("Light actors are not transient"), Actor->HasAnyFlags(RF_Transient));
                TestFalse(TEXT("Light components survive map serialization"), Light->GetLightComponent()->HasAnyFlags(RF_Transient));
                TestEqual(TEXT("Diffuse input is preserved"), Light->GetLightComponent()->DiffuseScale, 1.f);
                TestEqual(TEXT("Specular input is preserved"), Light->GetLightComponent()->SpecularScale, 1.f);
                if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(Light->GetLightComponent()))
                    TestEqual(TEXT("Local light uses physical lumens"), Local->IntensityUnits, ELightUnits::Lumens);
            }
            if (const APostProcessVolume* Post = Cast<APostProcessVolume>(Actor))
            {
                ++PostCount;
                TestEqual(TEXT("Exposure stays manual"), Post->Settings.AutoExposureMethod.GetValue(), AEM_Manual);
                TestEqual(TEXT("Exposure compensation stays zero"), Post->Settings.AutoExposureBias, 0.f);
                TestTrue(TEXT("Preset exposure survives module extraction"),
                    FMath::IsNearlyEqual(Post->Settings.CameraShutterSpeed, FMath::Pow(2.f, Preset.EV100) / 16.f));
            }
        }
        TestEqual(TEXT("Exactly one key and one fill"), LightCount, 2);
        TestEqual(TEXT("Exactly one fixed exposure volume"), PostCount, 1);
        const int32 ActorCount = World->PersistentLevel->Actors.Num();
        TestFalse(TEXT("An existing authored rig is never regenerated"), YogLookDev::BuildLightingLevel(World.Get(), Preset.Id, Message));
        TestEqual(TEXT("Duplicate request preserves authored actor collection"), World->PersistentLevel->Actors.Num(), ActorCount);
        TestFalse(TEXT("Test never creates a package file"), FPackageName::DoesPackageExist(PackagePath));
        Package->SetDirtyFlag(false);
        World->DestroyWorld(false);
        World->MarkObjectsPendingKill();
    }
    return true;
}

#endif
