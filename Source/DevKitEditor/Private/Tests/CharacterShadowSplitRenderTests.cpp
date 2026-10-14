#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "ShaderCompiler.h"
#include "RenderUtils.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"

namespace
{
class FCharacterShadowSplitCapture final : public IAutomationLatentCommand
{
public:
    explicit FCharacterShadowSplitCapture(FAutomationTestBase* InTest) : Test(InTest), Start(FPlatformTime::Seconds()) {}
    ~FCharacterShadowSplitCapture() override
    {
        for (const auto& Item : Before)
        {
            Item.Key->ClearFlags(ECVF_SetByMask);
            Item.Key->Set(*Item.Value.Key, static_cast<EConsoleVariableFlags>(Item.Value.Value & ECVF_SetByMask));
            Item.Key->SetFlags(Item.Value.Value);
        }
    }
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Start > 1200.) return Fail(TEXT("Character split capture timed out (including initial shader compilation)."));
        if (State == 0)
        {
            FText Message;
            if (!FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message)) return Fail(Message.ToString());
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/YogArt/Character/Enemy/Elite/CellCaptain/SM_CellCaptain_01.SM_CellCaptain_01"));
            if (!Mesh || !FDevKitCharacterLookDevService::SetSubject(Mesh, Message)
                || !FDevKitCharacterLookDevService::SetCamera(false, Message)) return Fail(TEXT("Cannot create actual CellCaptain render fixture: ") + Message.ToString());
            bool bCharacterMaterial = false;
            for (const auto& Slot : Mesh->GetStaticMaterials())
                bCharacterMaterial |= Slot.MaterialInterface && Slot.MaterialInterface->GetShadingModels().HasShadingModel(MSM_StylizedCharacterLit);
            if (!bCharacterMaterial) return Fail(TEXT("Fixture has no StylizedCharacterLit material; cannot test character splitting."));
            for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Subject"))) Subject = *It;
            if (!Subject.IsValid() || !GCurrentLevelEditingViewportClient) return Fail(TEXT("Fixture subject or perspective viewport is unavailable."));
            NearLocation = GCurrentLevelEditingViewportClient->GetViewLocation();
            NearRotation = GCurrentLevelEditingViewportClient->GetViewRotation();
            // Make the native non-far-caster cutoff observable in the far-camera
            // pair without changing any saved light/map asset.
            for (TActorIterator<ADirectionalLight> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                CastChecked<UDirectionalLightComponent>(It->GetLightComponent())->SetDynamicShadowDistanceMovableLight(1000.f);
            for (const TCHAR* Name : {TEXT("r.StylizedCharacter.Shadow.Split.Enable"), TEXT("r.StylizedCharacter.Shadow.Split.Self"),
                TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"), TEXT("r.Shadow.Virtual.Character.PreserveCasters"), TEXT("r.Shadow.Virtual.Enable"),
                TEXT("r.Shadow.Virtual.UseFarShadowCulling"), TEXT("r.StylizedCharacter.Shadow.Split.Dump"), TEXT("r.Shadow.Virtual.Cache"),
                TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"), TEXT("r.Shadow.RadiusThreshold"),
                TEXT("r.Shadow.Virtual.Cache.ForceInvalidateDirectional"), TEXT("r.Shadow.Virtual.Visualize.LightName"),
                TEXT("r.DistanceFieldShadowing"), TEXT("r.ContactShadows")})
            {
                IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
                if (!Variable) return Fail(FString(TEXT("Missing shadow CVar: ")) + Name);
                Before.Add(Variable, {Variable->GetString(), Variable->GetFlags()});
                Variable->ClearFlags(ECVF_SetByMask);
            }
            InitialVSMCache = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Cache"))->GetInt();
            InitialUseRadiusThreshold = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"))->GetInt();
            InitialRadiusThreshold = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.RadiusThreshold"))->GetFloat();
            InitialForceInvalidate = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Cache.ForceInvalidateDirectional"))->GetInt();
            InitialDistanceFieldShadowing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DistanceFieldShadowing"))->GetInt();
            InitialContactShadows = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ContactShadows"))->GetInt();
            InitialVisualizeLightName = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Visualize.LightName"))->GetString();
            if (!SetIntChecked(TEXT("r.Shadow.Virtual.Enable"), 1)
                || !SetIntChecked(TEXT("r.Shadow.Virtual.Character.PreserveCasters"), 1)
                || !SetIntChecked(TEXT("r.Shadow.Virtual.UseFarShadowCulling"), 1)) return true;
            if (!UseVirtualShadowMaps(GMaxRHIShaderPlatform))
                return Fail(TEXT("VSM is requested but the real platform path is unavailable. Check Nanite project support, DX12/SM6 and hardware atomics; CVar=1 alone is not acceptance."));
            Test->AddInfo(TEXT("Actual UseVirtualShadowMaps(platform) is true; this run is not a classic-shadow-map fallback."));
            if (!SetIntChecked(TEXT("r.StylizedCharacter.Shadow.Split.Dump"), 0)) return true;
            Test->TestFalse(TEXT("Diagnostics fixture is not restricted LookDevMain"), FDevKitLookDevLevelService::IsMainWorld());
            if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_VisualizeVirtualShadowMap, FName(TEXT("mask")), Message)) return Fail(Message.ToString());
            if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_VisualizeLumen, FName(TEXT("LumenScene")), Message)) return Fail(Message.ToString());
            if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_Lit, NAME_None, Message)) return Fail(Message.ToString());
            Directory = FPaths::ProjectSavedDir() / TEXT("CharacterShadowSplit") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
            IFileManager::Get().MakeDirectory(*Directory, true);
            State = 1;
        }
        if (State == 1)
        {
            if (!Subject.IsValid() || !GCurrentLevelEditingViewportClient) return Fail(TEXT("Capture fixture was lost between variants."));
            const bool bNative = Variant == 0 || Variant == 5 || Variant == 6 || Variant == 8 || Variant >= 10;
            if (!SetIntChecked(TEXT("r.StylizedCharacter.Shadow.Split.Enable"), bNative ? 0 : 1)
                || !SetIntChecked(TEXT("r.StylizedCharacter.Shadow.Split.Self"), Variant == 3 ? 0 : 1)
                || !SetIntChecked(TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"), Variant >= 2 ? 1 : 0)
                || !SetIntChecked(TEXT("r.Shadow.Virtual.Character.PreserveCasters"), Variant == 10 || Variant == 14 || Variant == 16 ? 0 : 1)
                || !SetIntChecked(TEXT("r.Shadow.Virtual.Enable"), 1)
                || !SetIntChecked(TEXT("r.Shadow.Virtual.UseFarShadowCulling"), 1)) return true;
            FText Message;
            // Rebuild both preservation A/B variants without prior cached pages.
            if (!SetIntChecked(TEXT("r.Shadow.Virtual.Cache"), IsPreservationPair() ? (Variant >= 16 ? 1 : 0) : InitialVSMCache)) return true;
            // Use the same deliberately strict native small-caster threshold in
            // both far variants; otherwise this fixture may never be culled and
            // equal screenshots cannot establish preservation causality.
            const bool bPreservationPair = IsPreservationPair();
            if (!SetIntChecked(TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"), bPreservationPair ? 1 : InitialUseRadiusThreshold)) return true;
            // Cache=0 already makes the entry uncached. ForceInvalidateDirectional
            // also changes clipmap depth range, so keep it off in this A/B pair.
            if (!SetIntChecked(TEXT("r.Shadow.Virtual.Cache.ForceInvalidateDirectional"), bPreservationPair ? 0 : InitialForceInvalidate)
                || !SetIntChecked(TEXT("r.DistanceFieldShadowing"), bPreservationPair ? 0 : InitialDistanceFieldShadowing)
                || !SetIntChecked(TEXT("r.ContactShadows"), bPreservationPair ? 0 : InitialContactShadows)) return true;
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Visualize.LightName"))->Set(
                bPreservationPair ? TEXT("LookDev Candidate Key (lux)") : *InitialVisualizeLightName, ECVF_SetByCode);
            IConsoleVariable* RadiusThreshold = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.RadiusThreshold"));
            RadiusThreshold->Set(bPreservationPair ? .1f : InitialRadiusThreshold, ECVF_SetByCode);
            if (!FMath::IsNearlyEqual(RadiusThreshold->GetFloat(), bPreservationPair ? .1f : InitialRadiusThreshold))
                return Fail(TEXT("Native small-caster threshold did not apply to the capture fixture."));
            ExpectedViewMode = Variant == 12 || Variant >= 14 ? VMI_VisualizeVirtualShadowMap : Variant == 13 ? VMI_VisualizeLumen : VMI_Lit;
            const FName DiagnosticName = ExpectedViewMode == VMI_VisualizeVirtualShadowMap ? FName(TEXT("mask")) : Variant == 13 ? FName(TEXT("LumenScene")) : NAME_None;
            if (!FDevKitLookDevLevelService::SetPreviewViewMode(ExpectedViewMode, DiagnosticName, Message)) return Fail(Message.ToString());
            if (Variant < 4 || Variant == 6 || Variant == 7 || Variant == 12 || Variant == 13)
            {
                GCurrentLevelEditingViewportClient->SetViewLocation(NearLocation);
                GCurrentLevelEditingViewportClient->SetViewRotation(NearRotation);
            }
            if (Variant == 4 || Variant == 5 || IsPreservationPair())
            {
                auto* Viewport = GCurrentLevelEditingViewportClient;
                Viewport->SetViewLocation(NearLocation - NearRotation.Vector() * 2500.f);
                Viewport->SetViewRotation(NearRotation);
            }
            if (Variant == 10)
            {
                // The preservation pair must differ only in PreserveCasters.
                // Remove the two earlier transient scenario additions first.
                if (EnvironmentOccluder.IsValid()) EnvironmentOccluder->Destroy();
                if (SecondSubject.IsValid()) SecondSubject->Destroy();
                EnvironmentOccluder.Reset();
                SecondSubject.Reset();
                UStaticMeshComponent* Component = Subject->GetStaticMeshComponent();
                Component->bCastFarShadow = false;
                Component->MarkRenderStateDirty();
                // Only the directional key may cast a shadow in the distance
                // preservation fixture. Broad Fill lighting remains unchanged.
                for (TActorIterator<ALight> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                    if (!It->IsA<ADirectionalLight>()) It->GetLightComponent()->SetCastShadows(false);
            }
            if (Variant == 6)
            {
                auto* Viewport = GCurrentLevelEditingViewportClient;
                Viewport->SetViewLocation(NearLocation);
                Viewport->SetViewRotation(NearRotation);
                UWorld* World = GEditor->GetEditorWorldContext().World();
                UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
                FVector Center, Extent;
                Subject->GetActorBounds(false, Center, Extent);
                FVector ToLight = FVector(-1, -1, 1).GetSafeNormal();
                for (TActorIterator<ADirectionalLight> It(World); It; ++It) { ToLight = -It->GetActorForwardVector(); break; }
                FActorSpawnParameters Spawn;
                Spawn.ObjectFlags = RF_Transient;
                AStaticMeshActor* Occluder = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Center + ToLight * (Extent.Z + 100.f), FRotator::ZeroRotator, Spawn);
                if (!Cube || !Occluder) return Fail(TEXT("Cannot create environment-to-character occluder."));
                EnvironmentOccluder = Occluder;
                auto* Component = Occluder->GetStaticMeshComponent();
                Component->SetMobility(EComponentMobility::Movable);
                Component->SetStaticMesh(Cube);
                Component->SetCastShadow(true);
                Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Occluder->SetActorScale3D(FVector(FMath::Max(0.25f, Extent.X / 100.f), FMath::Max(0.4f, Extent.Y / 50.f), 0.2f));
                Occluder->SetActorLabel(TEXT("ShadowSplit Environment Occluder - transient"));
            }
            if (Variant == 8)
            {
                UWorld* World = GEditor->GetEditorWorldContext().World();
                FVector Center, Extent;
                Subject->GetActorBounds(false, Center, Extent);
                const FVector Offset(0, FMath::Max(150.f, Extent.Y * 2.f + 50.f), 0);
                FActorSpawnParameters Spawn;
                Spawn.ObjectFlags = RF_Transient;
                AStaticMeshActor* Second = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Subject->GetActorLocation() + Offset, Subject->GetActorRotation(), Spawn);
                if (!Second) return Fail(TEXT("Cannot create second character receiver."));
                SecondSubject = Second;
                auto* Component = Second->GetStaticMeshComponent();
                Component->SetMobility(EComponentMobility::Movable);
                Component->SetStaticMesh(Subject->GetStaticMeshComponent()->GetStaticMesh());
                Component->SetCastShadow(true);
                Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Second->SetActorLabel(TEXT("ShadowSplit Second Character - transient"));
                auto* Viewport = GCurrentLevelEditingViewportClient;
                const FVector Target = Center + Offset * 0.5f;
                const FVector Location = Target + (NearLocation - Center) * 1.7f;
                Viewport->SetViewLocation(Location);
                Viewport->SetViewRotation((Target - Location).Rotation());
            }
            if (!SetIntChecked(TEXT("r.StylizedCharacter.Shadow.Split.Dump"), Variant + 1)) return true;
            ReadyAt = FPlatformTime::Seconds() + 3.;
            State = 2;
            return false;
        }
        if (State == 2)
        {
            if ((GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) || FPlatformTime::Seconds() < ReadyAt) return false;
            if (!VerifyConfiguration()) return true;
            if (Variant >= 14)
            {
                TArray<FColor> Pixels;
                FViewport* Viewport = GCurrentLevelEditingViewportClient->Viewport;
                if (!Viewport || !Viewport->ReadPixels(Pixels)) return Fail(TEXT("Cannot read settled VSM mask pixels."));
                const FIntPoint Size = Viewport->GetSizeXY();
                if (Pixels.Num() != Size.X * Size.Y) return Fail(TEXT("Unexpected VSM mask readback size."));
                int32 ShadowPixels = 0;
                for (int32 Y = 0; Y < Size.Y; ++Y)
                    for (int32 X = 0; X < Size.X; ++X)
                    {
                        // Exclude the blue swatch in the diagnostic legend.
                        if (X < 500 && Y > Size.Y - 180) continue;
                        const FColor Pixel = Pixels[Y * Size.X + X];
                        ShadowPixels += static_cast<int32>(Pixel.B) > static_cast<int32>(Pixel.R) + 32
                            && static_cast<int32>(Pixel.B) > static_cast<int32>(Pixel.G) + 32;
                    }
                Test->AddInfo(FString::Printf(TEXT("CHARACTER_VSM_PRESERVE_MASK cache=%d preserve=%d blueShadowPixels=%d"), Variant >= 16 ? 1 : 0, Variant % 2, ShadowPixels));
                if (Variant % 2 == 0) UnprotectedShadowPixels = ShadowPixels;
                else if (ShadowPixels <= UnprotectedShadowPixels + 32)
                    return Fail(TEXT("Protected far character did not add VSM shadow pixels for the explicitly selected directional key. Screenshot existence is not sufficient."));
            }
            const TCHAR* Names[] = {TEXT("NativeVSM"), TEXT("SplitLightVector"), TEXT("SplitHalfVector"), TEXT("SplitWithoutSelf"), TEXT("FarHalfVector"),
                TEXT("FarNativeVSM"), TEXT("EnvironmentNativeVSM"), TEXT("EnvironmentSplitHalfVector"), TEXT("MultiCharacterNativeVSM"), TEXT("MultiCharacterSplitHalfVector"),
                TEXT("FarNativePreserveOff"), TEXT("FarNativePreserveOn"), TEXT("DiagnosticVSMMask"), TEXT("DiagnosticLumenScene"),
                TEXT("FarVSMMaskPreserveOff"), TEXT("FarVSMMaskPreserveOn"),
                TEXT("FarVSMMaskCachedPreserveOff"), TEXT("FarVSMMaskCachedPreserveOn")};
            Filename = FPaths::ConvertRelativePathToFull(Directory / (FString(Names[Variant]) + TEXT(".png")));
            LogFixtureContext(Names[Variant]);
            FScreenshotRequest::RequestScreenshot(Filename, false, false);
            if (GEditor) GEditor->RedrawLevelEditingViewports();
            Test->AddInfo(FString::Printf(TEXT("CHARACTER_SHADOW_CAPTURE variant=%s file=%s"), Names[Variant], *Filename));
            ReadyAt = FPlatformTime::Seconds() + 1.;
            State = 3;
            return false;
        }
        if (State == 3)
        {
            if (FPlatformTime::Seconds() < ReadyAt || IFileManager::Get().FileSize(*Filename) <= 32) return false;
            if (++Variant == 18)
            {
                FText Message;
                if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_Lit, NAME_None, Message)) return Fail(Message.ToString());
                Test->AddInfo(TEXT("Eighteen real-RHI captures completed. Original ten split/scenario captures are retained; added same-camera native VSM PreserveCasters=0/1 pairs in Lit and VSM mask with far culling enabled, identical radius threshold=0.1, uncached and cached directional pairs, plus settled near VSM mask/LumenScene captures. CVar readback, view mode and fixture context were checked. Explicit directional shadow-pixel growth is asserted in each mask A/B pair. Visual comparison is still required for receiver separation, shadow shape and GPU budget."));
                return true;
            }
            State = 1;
        }
        return false;
    }
private:
    bool IsPreservationPair() const { return Variant == 10 || Variant == 11 || Variant >= 14; }
    bool VerifyInt(const TCHAR* Name, int32 Expected)
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
        if (!Variable || Variable->GetInt() != Expected)
        {
            Test->AddError(FString::Printf(TEXT("Capture CVar mismatch: %s expected=%d actual=%s flags=0x%08x"), Name, Expected,
                Variable ? *Variable->GetString() : TEXT("missing"), Variable ? static_cast<uint32>(Variable->GetFlags()) : 0u));
            return false;
        }
        return true;
    }
    bool SetIntChecked(const TCHAR* Name, int32 Value)
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
        if (Variable) Variable->Set(Value, ECVF_SetByCode);
        return VerifyInt(Name, Value);
    }
    bool VerifyConfiguration()
    {
        if (!UseVirtualShadowMaps(GMaxRHIShaderPlatform))
        {
            Test->AddError(TEXT("Actual VSM platform path became unavailable before capture."));
            return false;
        }
        const bool bNative = Variant == 0 || Variant == 5 || Variant == 6 || Variant == 8 || Variant >= 10;
        if (!VerifyInt(TEXT("r.StylizedCharacter.Shadow.Split.Enable"), bNative ? 0 : 1)
            || !VerifyInt(TEXT("r.StylizedCharacter.Shadow.Split.Self"), Variant == 3 ? 0 : 1)
            || !VerifyInt(TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"), Variant >= 2 ? 1 : 0)
            || !VerifyInt(TEXT("r.Shadow.Virtual.Character.PreserveCasters"), Variant == 10 || Variant == 14 || Variant == 16 ? 0 : 1)
            || !VerifyInt(TEXT("r.Shadow.Virtual.Enable"), 1)
            || !VerifyInt(TEXT("r.Shadow.Virtual.Cache"), IsPreservationPair() ? (Variant >= 16 ? 1 : 0) : InitialVSMCache)
            || !VerifyInt(TEXT("r.Shadow.Virtual.Cache.ForceInvalidateDirectional"), IsPreservationPair() ? 0 : InitialForceInvalidate)
            || !VerifyInt(TEXT("r.DistanceFieldShadowing"), IsPreservationPair() ? 0 : InitialDistanceFieldShadowing)
            || !VerifyInt(TEXT("r.ContactShadows"), IsPreservationPair() ? 0 : InitialContactShadows)
            || !VerifyInt(TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"), IsPreservationPair() ? 1 : InitialUseRadiusThreshold)
            || !VerifyInt(TEXT("r.Shadow.Virtual.UseFarShadowCulling"), 1)) return false;
        if (!FMath::IsNearlyEqual(IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.RadiusThreshold"))->GetFloat(),
            IsPreservationPair() ? .1f : InitialRadiusThreshold))
        {
            Test->AddError(TEXT("Capture native radius threshold changed before screenshot."));
            return false;
        }
        if (!Subject.IsValid() || !GCurrentLevelEditingViewportClient || GCurrentLevelEditingViewportClient->GetViewMode() != ExpectedViewMode)
        {
            Test->AddError(TEXT("Capture subject/viewport/view mode no longer matches the configured variant."));
            return false;
        }
        if (Variant >= 10 && (EnvironmentOccluder.IsValid() || SecondSubject.IsValid()
            || Subject->GetStaticMeshComponent()->bCastFarShadow))
        {
            Test->AddError(TEXT("Preservation/diagnostic fixture must contain only the original subject, with CastFarShadow disabled."));
            return false;
        }
        if (IsPreservationPair()
            && (!GCurrentLevelEditingViewportClient->GetViewLocation().Equals(NearLocation - NearRotation.Vector() * 2500.f, 0.01f)
                || !GCurrentLevelEditingViewportClient->GetViewRotation().Equals(NearRotation, 0.01f)))
        {
            Test->AddError(TEXT("Far PreserveCasters A/B camera changed before capture."));
            return false;
        }
        if (IsPreservationPair())
        {
            if (IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Visualize.LightName"))->GetString() != TEXT("LookDev Candidate Key (lux)"))
            {
                Test->AddError(TEXT("Distance preservation diagnostic is not locked to the directional key."));
                return false;
            }
            for (TActorIterator<ALight> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                if (!It->IsA<ADirectionalLight>() && It->GetLightComponent()->CastShadows)
                {
                    Test->AddError(TEXT("Distance preservation fixture has an additional shadow-casting fill light."));
                    return false;
                }
        }
        return true;
    }
    void LogFixtureContext(const TCHAR* Name)
    {
        UWorld* World = GEditor->GetEditorWorldContext().World();
        int32 ActorCount = 0, StaticMeshCount = 0, CharacterMaterialActorCount = 0, DirectionalLightCount = 0;
        for (TActorIterator<AActor> It(World); It; ++It) ++ActorCount;
        for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
        {
            ++StaticMeshCount;
            bool bCharacter = false;
            UStaticMeshComponent* Component = It->GetStaticMeshComponent();
            for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
                if (UMaterialInterface* Material = Component->GetMaterial(Index))
                    bCharacter |= Material->GetShadingModels().HasShadingModel(MSM_StylizedCharacterLit);
            CharacterMaterialActorCount += bCharacter ? 1 : 0;
        }
        for (TActorIterator<ADirectionalLight> It(World); It; ++It)
        {
            ++DirectionalLightCount;
            const auto* Component = CastChecked<UDirectionalLightComponent>(It->GetLightComponent());
            Test->AddInfo(FString::Printf(TEXT("CHARACTER_SHADOW_LIGHT variant=%s name=%s mobility=%d dynamicShadowDistanceMovable=%.1f"),
                Name, *It->GetName(), static_cast<int32>(Component->Mobility), Component->DynamicShadowDistanceMovableLight));
        }
        const auto* Viewport = GCurrentLevelEditingViewportClient;
        const UStaticMeshComponent* Component = Subject->GetStaticMeshComponent();
        const UStaticMesh* Mesh = Component->GetStaticMesh();
        Test->AddInfo(FString::Printf(TEXT("CHARACTER_SHADOW_CONTEXT variant=%s actors=%d staticMeshActors=%d characterMaterialActors=%d directionalLights=%d occluder=%d secondSubject=%d subjectCastShadow=%d subjectCastFarShadow=%d subjectNaniteAssetEnabled=%d viewMode=%d camera=%s rotation=%s split=%d self=%d halfVector=%d vsm=%d preserve=%d farCulling=%d"),
            Name, ActorCount, StaticMeshCount, CharacterMaterialActorCount, DirectionalLightCount, EnvironmentOccluder.IsValid() ? 1 : 0, SecondSubject.IsValid() ? 1 : 0,
            Component->CastShadow ? 1 : 0, Component->bCastFarShadow ? 1 : 0, Mesh && Mesh->GetNaniteSettings().bEnabled ? 1 : 0,
            static_cast<int32>(Viewport->GetViewMode()), *Viewport->GetViewLocation().ToString(), *Viewport->GetViewRotation().ToString(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.Enable"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.Self"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Character.PreserveCasters"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.UseFarShadowCulling"))->GetInt()));
        Test->AddInfo(TEXT("Context counts are game-thread fixture observations, not GPU draw, map-allocation or split-pass counters. If native and split images match, a renderer fallback remains possible."));
        Test->AddInfo(FString::Printf(TEXT("CHARACTER_SHADOW_CULL_CONTEXT variant=%s useRadiusThreshold=%d radiusThreshold=%g forceInvalidateDirectional=%d"),
            Name, IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"))->GetInt(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.RadiusThreshold"))->GetFloat(),
            IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Cache.ForceInvalidateDirectional"))->GetInt()));
    }
    bool Fail(const FString& Message) { Test->AddError(Message); return true; }
    FAutomationTestBase* Test;
    double Start, ReadyAt = 0.;
    int32 State = 0, Variant = 0;
    int32 InitialVSMCache = 1;
    int32 UnprotectedShadowPixels = 0;
    int32 InitialUseRadiusThreshold = 1;
    int32 InitialForceInvalidate = 0;
    int32 InitialDistanceFieldShadowing = 0, InitialContactShadows = 0;
    FString InitialVisualizeLightName;
    float InitialRadiusThreshold = 0.f;
    FString Directory, Filename;
    TWeakObjectPtr<AStaticMeshActor> Subject;
    TWeakObjectPtr<AStaticMeshActor> EnvironmentOccluder, SecondSubject;
    EViewModeIndex ExpectedViewMode = VMI_Lit;
    FVector NearLocation;
    FRotator NearRotation;
    TMap<IConsoleVariable*, TPair<FString, EConsoleVariableFlags>> Before;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterShadowSplitRenderTest, "DevKitEditor.CharacterShadowSplit.RenderSmoke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCharacterShadowSplitRenderTest::RunTest(const FString&)
{
    if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("Dev01CharacterShadowSplitRender")))
    {
        AddInfo(TEXT("Skipped: requires a dedicated unattended editor with -Dev01CharacterShadowSplitRender; creates only an unsaved stage."));
        return true;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("NullRHI"))) { AddError(TEXT("Real RHI is required.")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FCharacterShadowSplitCapture(this));
    return true;
}
#endif
