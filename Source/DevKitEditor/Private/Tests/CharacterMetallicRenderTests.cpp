#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Math/RotationMatrix.h"
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ReflectionCapture.h"
#include "Engine/Light.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Character/PlayerCharacterBase.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionStylizedCharacterLightingOutput.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"
#include "PrimitiveSceneProxy.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"
#include "StylizedLightingSettings.h"
#include "StylizedCharacterLighting.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"

namespace
{
class FCharacterMetallicCapture final : public IAutomationLatentCommand
{
public:
    explicit FCharacterMetallicCapture(FAutomationTestBase* InTest)
        : Test(InTest), Started(FPlatformTime::Seconds()), Settings(GetMutableDefault<UStylizedLightingSettings>())
    {
        bReflectionsBefore = Settings->bEnableEnvironmentReflections;
        NonMetalAlphaBefore = Settings->NonMetalReflectionAlpha;
        MetalAlphaBefore = Settings->MetalReflectionAlpha;
        bMixBefore = Settings->bEnableMixMapSpecularControl;
        bDirectLightingBefore = Settings->bEnableCharacterDirectLighting;
        CpuProfilesBefore = Settings->CharacterLightingProfiles;
        GetStylizedCharacterLightingProfiles(ProfilesBefore, ProfileCountBefore);
        SceneStyle = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"));
        if (SceneStyle) { SceneStyleBefore = SceneStyle->GetString(); SceneStyleFlags = SceneStyle->GetFlags(); }
        MetalStyle = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedCharacterLighting.MetalStyle"));
        if (MetalStyle) { MetalStyleBefore = MetalStyle->GetString(); MetalStyleFlags = MetalStyle->GetFlags(); }
        if (FParse::Param(FCommandLine::Get(), TEXT("Dev01NprMetalOnly"))) Variant = 30;
        if (FParse::Param(FCommandLine::Get(), TEXT("Dev01MetalMaterialOverridesOnly"))) Variant = 52;
        if (FParse::Param(FCommandLine::Get(), TEXT("Dev01MetalF0Only"))) Variant = 63;
    }
    ~FCharacterMetallicCapture() override
    {
        if (MetalColorShowFlagsBefore.IsSet() && GCurrentLevelEditingViewportClient)
        {
            GCurrentLevelEditingViewportClient->EngineShowFlags = MetalColorShowFlagsBefore.GetValue();
            GCurrentLevelEditingViewportClient->Invalidate();
        }
        RestoreCvars(ZeroLightCvars);
        RestoreCvars(MaterialDebugCvars);
        Settings->bEnableEnvironmentReflections = bReflectionsBefore;
        Settings->NonMetalReflectionAlpha = NonMetalAlphaBefore;
        Settings->MetalReflectionAlpha = MetalAlphaBefore;
        Settings->bEnableMixMapSpecularControl = bMixBefore;
        Settings->bEnableCharacterDirectLighting = bDirectLightingBefore;
        Settings->CharacterLightingProfiles = CpuProfilesBefore;
        SetStylizedCharacterLightingProfiles(MakeArrayView(ProfilesBefore.GetData(), ProfilesBefore.Num()), ProfileCountBefore);
        if (SceneStyle) { SceneStyle->ClearFlags(ECVF_SetByMask); SceneStyle->Set(*SceneStyleBefore, ECVF_SetByCode); SceneStyle->ClearFlags(ECVF_SetByMask); SceneStyle->SetFlags(static_cast<EConsoleVariableFlags>(SceneStyleFlags & ECVF_SetByMask)); }
        if (MetalStyle) { MetalStyle->ClearFlags(ECVF_SetByMask); MetalStyle->Set(*MetalStyleBefore, ECVF_SetByCode); MetalStyle->ClearFlags(ECVF_SetByMask); MetalStyle->SetFlags(static_cast<EConsoleVariableFlags>(MetalStyleFlags & ECVF_SetByMask)); }
    }
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 1200.) return Fail(TEXT("Metallic render test timed out, including shader compilation."));
        if (LastLoggedState != State || LastLoggedVariant != Variant)
        {
            UE_LOG(LogTemp, Display, TEXT("CHARACTER_METALLIC_PROGRESS variant=%d state=%d file=%s bytes=%lld"), Variant, State, *Filename, IFileManager::Get().FileSize(*Filename));
            LastLoggedState = State;
            LastLoggedVariant = Variant;
        }
        FText Message;
        if (State == 0)
        {
            if (!MetalStyle) return Fail(TEXT("Character MetalStyle CVar is missing; the loaded engine does not expose the tested feature."));
            if (!FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message)) return Fail(Message.ToString());
            // Lit alone does not reset persisted material/debug flags or the
            // developer diffuse remapping CVars inherited from another test.
            SetScopedCvar(MaterialDebugCvars, TEXT("r.DiffuseColor.Min"), 0.f);
            SetScopedCvar(MaterialDebugCvars, TEXT("r.DiffuseColor.Max"), 1.f);
            SetScopedCvar(MaterialDebugCvars, TEXT("r.Roughness.Min"), 0.f);
            SetScopedCvar(MaterialDebugCvars, TEXT("r.Roughness.Max"), 1.f);
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/YogArt/Character/Enemy/Elite/CellCaptain/SM_CellCaptain_01.SM_CellCaptain_01"));
            if (!Mesh || !FDevKitCharacterLookDevService::SetSubject(Mesh, Message)
                || !FDevKitCharacterLookDevService::SetCamera(false, Message)) return Fail(TEXT("Cannot build actual character fixture: ") + Message.ToString());
            for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Subject"))) Subject = *It;
            if (!Subject.IsValid()) return Fail(TEXT("Metallic fixture has no subject."));
            bool bHasCharacterMaterial = false;
            for (int32 Slot = 0; Slot < Subject->GetStaticMeshComponent()->GetNumMaterials(); ++Slot)
            {
                UMaterialInterface* Bound = Subject->GetStaticMeshComponent()->GetMaterial(Slot);
                const bool bCharacter = Bound && Bound->GetShadingModels().HasShadingModel(MSM_StylizedCharacterLit);
                bHasCharacterMaterial |= bCharacter;
                UE_LOG(LogTemp, Display, TEXT("CHARACTER_METALLIC_BOUND_MATERIAL slot=%d material=%s base=%s stylized=%d"), Slot,
                    *GetPathNameSafe(Bound), Bound ? *GetPathNameSafe(Bound->GetMaterial()) : TEXT("None"), bCharacter ? 1 : 0);
                AuditMetallicTexture(Bound);
                AuditMaterialParameters(Bound);
            }
            if (!bHasCharacterMaterial) return Fail(TEXT("Actual CellCaptain fixture has no StylizedCharacterLit material slot."));
            Settings->bEnableEnvironmentReflections = true;
            // Native BRDF parity cases compare full reflection, not artist attenuation.
            Settings->NonMetalReflectionAlpha = Variant < 30 ? 1.f : .3f;
            Settings->MetalReflectionAlpha = Variant < 30 ? 1.f : .7f;
            Settings->bEnableMixMapSpecularControl = true;
            Settings->bEnableCharacterDirectLighting = true;
            Settings->ApplyShadowTransitionParameters();
            // Compare against actual native DefaultLit, not the scene two-band path.
            if (SceneStyle) { SceneStyle->ClearFlags(ECVF_SetByMask); SceneStyle->Set(0, ECVF_SetByCode); }
            Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CharacterMetallic") / FGuid::NewGuid().ToString(EGuidFormats::Digits));
            IFileManager::Get().MakeDirectory(*Directory, true);
            State = 1;
        }
        if (State == 1)
        {
            if (Variant == 1 || Variant == 10 || Variant == 16 || Variant == 24 || Variant == 34 || Variant == 46 || Variant == 48 || Variant == 52 || Variant == 63)
            {
                UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
                if (!Sphere || !FDevKitCharacterLookDevService::SetSubject(Sphere, Message)
                    || !FDevKitCharacterLookDevService::SetCamera(false, Message)) return Fail(Message.ToString());
                // SetSubject replaces the transient actor, not just its mesh.
                Subject.Reset();
                for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                    if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Subject"))) Subject = *It;
                if (!Subject.IsValid()) return Fail(TEXT("Transient sphere actor was not found."));
                UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
                UMaterial* Material = NewObject<UMaterial>(Owner, NAME_None, RF_Transient);
                Material->SetShadingModel(MSM_StylizedCharacterLit);
                auto* Data = Material->GetEditorOnlyData();
                auto* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
                Color->ParameterName = TEXT("TestBaseColor");
                Color->DefaultValue = FLinearColor(.65f, .32f, .09f);
                Data->ExpressionCollection.Expressions.Add(Color);
                Data->BaseColor.Connect(0, Color);
                auto Parameter = [Material, Data](const TCHAR* Name, float Value)
                {
                    auto* Node = NewObject<UMaterialExpressionScalarParameter>(Material);
                    Node->ParameterName = FName(Name);
                    Node->DefaultValue = Value;
                    Data->ExpressionCollection.Expressions.Add(Node);
                    return Node;
                };
                Data->Metallic.Connect(0, Parameter(TEXT("TestMetallic"), 1.f));
                Data->Roughness.Connect(0, Parameter(TEXT("TestRoughness"), .2f));
                Data->Specular.Connect(0, Parameter(TEXT("TestSpecular"), .5f));
                if (Variant == 52)
                {
                    auto* Output = NewObject<UMaterialExpressionStylizedCharacterLightingOutput>(Material);
                    Data->ExpressionCollection.Expressions.Add(Output);
                    Output->MetalHighlightWidth.Connect(0, Parameter(TEXT("TestHighlightWidth"), 0.f));
                    auto* HighlightColor = NewObject<UMaterialExpressionVectorParameter>(Material);
                    HighlightColor->ParameterName = TEXT("TestHighlightColor");
                    HighlightColor->DefaultValue = FLinearColor(-1.f, -1.f, -1.f);
                    Data->ExpressionCollection.Expressions.Add(HighlightColor);
                    const FColor Colors[] = { FColor::Red, FColor::Green, FColor::Blue, FColor::White };
                    UTexture2D* Palette = UTexture2D::CreateTransient(2, 2, PF_B8G8R8A8);
                    Palette->SRGB = true; Palette->Filter = TF_Nearest;
                    Palette->AddressX = TA_Clamp; Palette->AddressY = TA_Clamp;
                    void* Texels = Palette->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
                    FMemory::Memcpy(Texels, Colors, sizeof(Colors));
                    Palette->GetPlatformData()->Mips[0].BulkData.Unlock(); Palette->UpdateResource();
                    auto* Texture = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
                    Texture->ParameterName = TEXT("TestHighlightPalette"); Texture->Texture = Palette;
                    Texture->SamplerType = SAMPLERTYPE_Color;
                    Data->ExpressionCollection.Expressions.Add(Texture);
                    auto* Source = NewObject<UMaterialExpressionLinearInterpolate>(Material);
                    Source->A.Connect(0, HighlightColor); Source->B.Connect(0, Texture);
                    Source->Alpha.Connect(0, Parameter(TEXT("TestUseHighlightTexture"), 0.f));
                    Data->ExpressionCollection.Expressions.Add(Source);
                    Output->MetalHighlightColor.Connect(0, Source);
                }
                Material->PostEditChange();
                MaterialInstance = UMaterialInstanceDynamic::Create(Material, Owner);
                Owner->SetMaterial(0, MaterialInstance.Get());
                Test->TestTrue(TEXT("Transient sphere uses the requested shading model"), MaterialInstance->GetShadingModels().HasShadingModel(MSM_StylizedCharacterLit));
            }
            if (Variant > 0 && Variant < 6)
            {
                if (!MaterialInstance.IsValid()) return Fail(TEXT("Transient metallic material was lost."));
                MaterialInstance->SetScalarParameterValue(TEXT("TestMetallic"), Variant == 4 ? 0.f : 1.f);
                MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), Variant == 3 ? .65f : .2f);
                MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), Variant == 1 ? 0.f : Variant == 2 ? 1.f : .5f);
                Settings->bEnableEnvironmentReflections = Variant != 5;
                Settings->ApplyShadowTransitionParameters();
            }
            if (Variant == 6 || Variant == 7)
            {
                Settings->bEnableEnvironmentReflections = true;
                Settings->ApplyShadowTransitionParameters();
                UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
                if (Variant == 6)
                {
                    MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), .5f);
                    Owner->SetMaterial(0, MaterialInstance.Get());
                }
                else
                {
                    Owner->SetMaterial(0, MakeNativeMaterial(MaterialInstance->GetMaterial(), Owner));
                }
            }
            if (Variant == 8 || Variant == 14 || Variant == 20 || Variant == 30 || Variant == 44 || Variant == 61)
            {
                UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/YogArt/Character/Enemy/Elite/CellCaptain/SM_CellCaptain_01.SM_CellCaptain_01"));
                if (!Mesh || !FDevKitCharacterLookDevService::SetSubject(Mesh, Message)) return Fail(Message.ToString());
                Subject.Reset();
                for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
                    if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Subject"))) Subject = *It;
                if (!Subject.IsValid()) return Fail(TEXT("Actual character was lost."));
                Subject->AddActorWorldRotation(FRotator(0, 70, 0));
                if (!FDevKitCharacterLookDevService::SetCamera(false, Message)) return Fail(Message.ToString());
            }
            if (Variant == 62)
            {
                UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
                for (int32 Slot = 0; Slot < Owner->GetNumMaterials(); ++Slot)
                {
                    // Exercise the real artist-facing master and its static switches,
                    // inheriting each CellCaptain instance's maps without saving it.
                    auto* Override = NewObject<UMaterialInstanceConstant>(Owner, NAME_None, RF_Transient);
                    Override->SetParentEditorOnly(Owner->GetMaterial(Slot));
                    Override->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Override Metal Highlight Width")), true);
                    Override->SetStaticSwitchParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Override Metal Highlight Color")), true);
                    Override->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Metal Highlight Width")), 2.5f);
                    Override->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Metal Highlight Color")), FLinearColor(.02f, .8f, 1.f));
                    Override->UpdateStaticPermutation();
                    Owner->SetMaterial(Slot, Override);
                }
            }
            if (Variant == 9)
            {
                UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
                for (int32 Slot = 0; Slot < Owner->GetNumMaterials(); ++Slot)
                    Owner->SetMaterial(Slot, MakeNativeMaterial(Owner->GetMaterial(Slot), Owner));
            }
            if ((Variant >= 10 && Variant <= 13) || (Variant >= 16 && Variant < 20))
            {
                if (!MaterialInstance.IsValid()) return Fail(TEXT("Transient styled-metal material was lost."));
                MaterialInstance->SetScalarParameterValue(TEXT("TestMetallic"), Variant == 12 || Variant == 13 ? 0.f : 1.f);
                MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), .45f);
                MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), .5f);
                Settings->bEnableEnvironmentReflections = Variant != 16 && Variant != 17;
                Settings->bEnableCharacterDirectLighting = Variant < 18;
                Settings->ApplyShadowTransitionParameters();
            }
            if (Variant >= 20 && Variant < 30 && !ConfigureTopDownVariant(Message)) return Fail(Message.ToString());
            if (Variant >= 30 && !ConfigureNprVariant(Message)) return Fail(Message.ToString());
            // Apply the feature toggle last: settings uploads must not silently
            // change the baseline under a reference or paired comparison.
            const bool bStyledVariant = Variant >= 10 && (Variant & 1) != 0;
            // New pairs compare the previously authored look (1) against the
            // top-down look (2), not against unrelated native shading (0).
            const float StyleValue = Variant >= 30 ? (IsNprBaseline(Variant) ? 2.f : 3.f)
                : Variant >= 20 ? (Variant <= 25 && (Variant & 1) == 0 ? 1.f : 2.f)
                : Variant == 13 ? 2.f : bStyledVariant ? 1.f : 0.f;
            MetalStyle->ClearFlags(ECVF_SetByMask);
            MetalStyle->Set(StyleValue, ECVF_SetByCode);
            Test->TestEqual(TEXT("The requested MetalStyle state reaches the live engine"), MetalStyle->GetFloat(), StyleValue);
            if (!SetFixtureLit(Message)) return Fail(Message.ToString());
            ReadyAt = 0.;
            State = 2;
            if (Variant == 30 || Variant == 32 || Variant == 44 || Variant == 61)
            {
                // Read the real GBuffer Metallic diagnostic for this exact
                // camera. Actual-character metrics exclude leather, cloth,
                // and background instead of averaging the entire image.
                if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_VisualizeBuffer,
                    Variant == 30 ? TEXT("BaseColor") : TEXT("Metallic"), Message)) return Fail(Message.ToString());
                State = Variant == 30 ? 5 : 4;
            }
        }
        if (State == 5)
        {
            if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { ReadyAt = 0.; return false; }
            if (ReadyAt == 0.) ReadyAt = FPlatformTime::Seconds() + 4.;
            if (FPlatformTime::Seconds() < ReadyAt) return false;
            TArray<FColor> BaseColorPixels;
            if (!GCurrentLevelEditingViewportClient || !GCurrentLevelEditingViewportClient->Viewport
                || !GCurrentLevelEditingViewportClient->Viewport->ReadPixels(BaseColorPixels)) return Fail(TEXT("CellCaptain BaseColor diagnostic readback failed."));
            const FIntPoint Size = GCurrentLevelEditingViewportClient->Viewport->GetSizeXY();
            if (BaseColorPixels.Num() != Size.X * Size.Y) return Fail(TEXT("CellCaptain BaseColor diagnostic dimensions mismatch."));
            const FString ColorFilename = Directory / TEXT("30_CellCaptain_BaseColor.png");
            TArray64<uint8> Png;
            FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(BaseColorPixels.GetData(), BaseColorPixels.Num()), Png);
            if (Png.IsEmpty() || !FFileHelper::SaveArrayToFile(Png, *ColorFilename)) return Fail(TEXT("Cannot save CellCaptain BaseColor diagnostic."));
            Test->AddInfo(FString::Printf(TEXT("NPR_BASECOLOR_REVIEW file=%s; confirm black leather and colored metal before accepting actual-character visual comparisons"), *ColorFilename));
            if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_VisualizeBuffer, TEXT("Metallic"), Message)) return Fail(Message.ToString());
            ReadyAt = 0.;
            State = 4;
        }
        if (State == 4)
        {
            if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { ReadyAt = 0.; return false; }
            if (ReadyAt == 0.) ReadyAt = FPlatformTime::Seconds() + 4.;
            if (FPlatformTime::Seconds() < ReadyAt) return false;
            if (!GCurrentLevelEditingViewportClient || !GCurrentLevelEditingViewportClient->Viewport
                || !GCurrentLevelEditingViewportClient->Viewport->ReadPixels(NprMetalMaskPixels)) return Fail(TEXT("NPR Metallic diagnostic readback failed."));
            const FIntPoint Size = GCurrentLevelEditingViewportClient->Viewport->GetSizeXY();
            if (NprMetalMaskPixels.Num() != Size.X * Size.Y) return Fail(TEXT("NPR Metallic diagnostic dimensions do not match the live viewport."));
            const FString MaskFilename = Directory / FString::Printf(TEXT("%02d_CellCaptain_MetallicMask.png"), Variant);
            TArray64<uint8> MaskPng;
            FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(NprMetalMaskPixels.GetData(), NprMetalMaskPixels.Num()), MaskPng);
            if (MaskPng.IsEmpty() || !FFileHelper::SaveArrayToFile(MaskPng, *MaskFilename)) return Fail(TEXT("Cannot save NPR Metallic diagnostic."));
            Test->AddInfo(FString::Printf(TEXT("NPR_METAL_MASK file=%s; selected inside projected subject bounds with RGB8 Metallic diagnostic >=160"), *MaskFilename));
            if (!SetFixtureLit(Message)) return Fail(Message.ToString());
            ReadyAt = 0.;
            State = 2;
        }
        if (State == 2)
        {
            if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) { ReadyAt = 0.; return false; }
            if (ReadyAt == 0. && Variant == 31)
            {
                for (UMaterialInterface* Bound : Subject->GetStaticMeshComponent()->GetMaterials())
                {
                    const FMaterialResource* Resource = Bound ? Bound->GetMaterial()->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
                    if (Resource)
                    {
                        Test->AddInfo(FString::Printf(TEXT("METAL_DEFAULT_AUX_REQUEST enabled=%d material=%s"), Resource->MaterialUsesAnisotropy_GameThread(), *Bound->GetMaterial()->GetPathName()));
                        Test->TestFalse(TEXT("Default character material with static metal overrides off requests no auxiliary pass"), Resource->MaterialUsesAnisotropy_GameThread());
                    }
                }
            }
            if (ReadyAt == 0. && Variant == 52)
            {
                const FMaterialResource* Resource = MaterialInstance->GetMaterial()->GetMaterialResource(GMaxRHIShaderPlatform);
                Test->TestTrue(TEXT("Dynamic material override inputs request the auxiliary data pass"), Resource && Resource->MaterialUsesAnisotropy_GameThread());
                Test->TestTrue(TEXT("Material relevance schedules the auxiliary pass for the custom shading model"),
                    MaterialInstance->GetRelevance_Concurrent(GMaxRHIShaderPlatform).bUsesAnisotropy);
            }
            if (ReadyAt == 0. && (Variant == 61 || Variant == 62))
            {
                UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
                Test->AddInfo(FString::Printf(TEXT("METAL_ACTUAL_CHARACTER_PATH variant=%d naniteAsset=%d naniteProxy=%d"),
                    Variant, Owner->GetStaticMesh()->IsNaniteEnabled(),
                    Owner->GetSceneProxy() && Owner->GetSceneProxy()->IsNaniteMesh()));
                for (UMaterialInterface* Bound : Owner->GetMaterials())
                    Test->TestEqual(TEXT("Actual CellCaptain material switches schedule only the opted-in auxiliary pass"),
                        Bound->GetRelevance_Concurrent(GMaxRHIShaderPlatform).bUsesAnisotropy, Variant == 62);
            }
            if (ReadyAt == 0.) ReadyAt = FPlatformTime::Seconds() + (Variant == 41 || Variant == 60 || Variant == 82 ? 8. : 4.);
            if (FPlatformTime::Seconds() < ReadyAt) return false;
            if (!GCurrentLevelEditingViewportClient || !GCurrentLevelEditingViewportClient->Viewport) return Fail(TEXT("No real RHI viewport."));
            TArray<FColor> Pixels;
            if (!GCurrentLevelEditingViewportClient->Viewport->ReadPixels(Pixels) || Pixels.IsEmpty()) return Fail(TEXT("Metallic readback failed."));
            const FIntPoint CurrentSize = GCurrentLevelEditingViewportClient->Viewport->GetSizeXY();
            if (Variant == 1 || (Variant >= 30 && RenderSize == FIntPoint::ZeroValue)) { MetalLowSpecPixels = Pixels; RenderSize = CurrentSize; }
            if (Variant >= 2 && (CurrentSize != RenderSize || Pixels.Num() != MetalLowSpecPixels.Num()))
                return Fail(TEXT("Metallic comparison viewport size changed; images are not comparable."));
            if (Variant == 2)
            {
                const double Difference = MeanDifference(MetalLowSpecPixels, Pixels, RenderSize);
                Test->AddInfo(FString::Printf(TEXT("METALLIC_SPECULAR_INVARIANCE subjectROI_RGB8Delta=%g (temporal tolerance 2.0)"), Difference));
                Test->TestTrue(TEXT("Full metal ignores dielectric Specular/MixMap suppression and lobe broadening"), Difference <= 2.0);
                MetalReferencePixels = Pixels;
            }
            if (Variant == 3 || Variant == 4 || Variant == 5)
            {
                const double Difference = MeanDifference(MetalReferencePixels, Pixels, RenderSize);
                Test->AddInfo(FString::Printf(TEXT("METALLIC_RESPONSE variant=%d subjectROI_RGB8Delta=%g"), Variant, Difference));
                Test->TestTrue(TEXT("Roughness, metallic and environment-reflection variants visibly affect real shader output"), Difference > .1);
            }
            if (Variant == 6 || Variant == 8) NativeComparisonPixels = Pixels;
            if (Variant == 7 || Variant == 9)
            {
                const double Difference = MeanDifference(NativeComparisonPixels, Pixels, RenderSize);
                Test->AddInfo(FString::Printf(TEXT("METALLIC_NATIVE_COMPARISON variant=%d subjectROI_RGB8Delta=%g"), Variant, Difference));
                if (Variant == 7 && !FParse::Param(FCommandLine::Get(), TEXT("MetallicBaseline")))
                    Test->TestTrue(TEXT("Full-metal character shader matches native DefaultLit within temporal tolerance"), Difference < 3.);
            }
            if (Variant >= 10 && Variant < 20)
            {
                if ((Variant & 1) == 0)
                {
                    StyleComparisonPixels = Pixels;
                }
                else
                {
                    const double Difference = MeanDifference(StyleComparisonPixels, Pixels, RenderSize);
                    Test->AddInfo(FString::Printf(TEXT("METALLIC_STYLE_RESPONSE variant=%d subjectROI_RGB8Delta=%g"), Variant, Difference));
                    if (Variant == 11)
                        Test->TestTrue(TEXT("Art-directed metal visibly changes the rough-metal sphere"), Difference > 1.);
                    if (Variant == 13)
                        Test->TestTrue(TEXT("MetalStyle leaves nonmetal lighting unchanged within temporal tolerance"), Difference < 2.);
                    // Actual-character metal coverage and available indirect
                    // radiance vary. Retain paired captures without asserting
                    // arbitrary whole-character/reflection brightness gains.
                }
            }
            if (Variant >= 20 && Variant < 30)
            {
                const float PixelRadius = TopDownPixelRadius(CurrentSize);
                if (Variant <= 25 && (Variant & 1) == 0) StyleComparisonPixels = Pixels;
                if (Variant <= 25 && (Variant & 1) != 0)
                {
                    const double Difference = TopDownDifference(StyleComparisonPixels, Pixels, CurrentSize, PixelRadius);
                    Test->AddInfo(FString::Printf(TEXT("METALLIC_TOPDOWN_RESPONSE variant=%d distanceCm=%g fov=%g subjectRadiusPixels=%g RGB8Delta=%g"),
                        Variant, TopDownDistance, TopDownFOV, PixelRadius, Difference));
                    if (Variant == 25)
                        Test->TestTrue(TEXT("Top-down metal remains visibly active on a distant small sphere"), Difference > .25);
                    // Real-character metal coverage is sparse; its near/far
                    // pairs are retained without a whole-body brightness gate.
                }
                if (Variant == 25) SweepComparisonPixels = Pixels;
                if (Variant == 26)
                {
                    const double Difference = TopDownDifference(SweepComparisonPixels, Pixels, CurrentSize, PixelRadius);
                    Test->AddInfo(FString::Printf(TEXT("METALLIC_TOPDOWN_LIGHT_SWEEP RGB8Delta=%g; only key-light direction changed"), Difference));
                    Test->TestTrue(TEXT("Distant top-down metal responds to real moving light rather than a static painted highlight"), Difference > .25);
                }
                if (Variant == 27) BloomComparisonPixels = Pixels;
                if (Variant == 28)
                {
                    const double HaloGain = TopDownBloomHaloGain(BloomComparisonPixels, Pixels, CurrentSize, PixelRadius);
                    Test->AddInfo(FString::Printf(TEXT("METALLIC_TOPDOWN_BLOOM positiveHaloRGB8Gain=%g; same 20000-lux key, native bloom only"), HaloGain));
                    Test->TestTrue(TEXT("Bright real metal radiance reaches native bloom outside the mesh silhouette"), HaloGain > .01);
                }
                if (Variant == 29)
                {
                    const double DarkMean = TopDownInnerMean(Pixels, CurrentSize, PixelRadius);
                    Test->AddInfo(FString::Printf(TEXT("METALLIC_TOPDOWN_DARK metalInnerRGB8Mean=%g; direct/reflection features disabled, bloom enabled"), DarkMean));
                    Test->TestTrue(TEXT("Top-down metal does not introduce an unlit/emissive brightness floor"), DarkMean < 3.);
                }
            }
            if (Variant >= 30 && Variant < 48) ValidateNprCapture(Pixels, CurrentSize);
            if (Variant >= 48 && Variant < 52)
            {
                if (Variant == 48 || Variant == 50) ArtistComparisonPixels = Pixels;
                else
                {
                    const double Difference = MeanDifference(ArtistComparisonPixels, Pixels, CurrentSize);
                    Test->AddInfo(FString::Printf(TEXT("METAL_ART_CONTROLS variant=%d meanRGB8Difference=%g"), Variant, Difference));
                    if (Variant == 49 || Variant == 51)
                        Test->TestTrue(TEXT("Artist metal shape and reflection alpha alter actual DX12 pixels"), Difference > .05);
                }
            }
            if (Variant == 52) MaterialOverrideBasePixels = Pixels;
            if (Variant == 53 || Variant == 55 || Variant == 56)
            {
                const double Difference = TopDownDifference(MaterialOverrideBasePixels, Pixels, CurrentSize, TopDownPixelRadius(CurrentSize));
                Test->AddInfo(FString::Printf(TEXT("METAL_MATERIAL_OVERRIDE variant=%d meanRGB8Difference=%g"), Variant, Difference));
                Test->TestTrue(TEXT("Material width, RGB and texture overrides visibly change deferred pixels beyond temporal noise"), Difference > 3.);
            }
            if (Variant == 53) MaterialOverrideWidePixels = Pixels;
            if (Variant == 54)
                Test->TestTrue(TEXT("Narrow and wide material highlight controls produce different pixels"),
                    TopDownDifference(MaterialOverrideWidePixels, Pixels, CurrentSize, TopDownPixelRadius(CurrentSize)) > 3.);
            if (Variant == 57) MaterialOverrideSmoothPixels = Pixels;
            if (Variant == 58)
            {
                Test->TestTrue(TEXT("Material roughness changes highlight edges without global softness"),
                    TopDownDifference(MaterialOverrideSmoothPixels, Pixels, CurrentSize, TopDownPixelRadius(CurrentSize)) > 2.);
                MaterialOverrideLightMean = TopDownInnerMean(Pixels, CurrentSize, TopDownPixelRadius(CurrentSize));
            }
            if (Variant == 59)
            {
                const double Mean = TopDownInnerMean(Pixels, CurrentSize, TopDownPixelRadius(CurrentSize));
                Test->AddInfo(FString::Printf(TEXT("METAL_MATERIAL_LIGHT_RESPONSE full=%g low=%g"), MaterialOverrideLightMean, Mean));
                Test->TestTrue(TEXT("Material-overridden highlight/core follow actual light intensity"), Mean < MaterialOverrideLightMean);
            }
            if (Variant == 60)
                Test->TestTrue(TEXT("Material override adds no light-independent glow"), TopDownInnerMean(Pixels, CurrentSize, TopDownPixelRadius(CurrentSize)) < 3.);
            if (Variant == 61) ActualMaterialOverridePixels = Pixels;
            if (Variant == 62)
            {
                double Sum = 0.; int32 Count = 0;
                for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
                    for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                        if (IsNprSample(X, Y, CurrentSize))
                        {
                            const int32 I = Y * CurrentSize.X + X;
                            const FColor A = ActualMaterialOverridePixels[I], B = Pixels[I];
                            Sum += FMath::Abs(int32(A.R) - B.R) + FMath::Abs(int32(A.G) - B.G) + FMath::Abs(int32(A.B) - B.B);
                            ++Count;
                        }
                const double Difference = Sum / FMath::Max(3 * Count, 1);
                Test->AddInfo(FString::Printf(TEXT("METAL_ACTUAL_MI_OVERRIDE samples=%d meanRGB8Difference=%g"), Count, Difference));
                Test->TestTrue(TEXT("Actual CellCaptain material-instance width/color overrides visibly alter metallic pixels"), Count > 30 && Difference > 3.);
            }
            if (Variant >= 63 && Variant <= 67) ValidateMetalF0Capture(Pixels, CurrentSize);
            if ((Variant >= 68 && Variant <= 70) || (Variant >= 85 && Variant <= 87)) ValidateDefaultRoughnessResponse(Pixels, CurrentSize);
            if (Variant >= 71 && Variant <= 73) NativeGgxNeutralPixels[Variant - 71] = Pixels;
            if (Variant >= 74 && Variant <= 76)
            {
                const double Difference = NprDifference(NativeGgxNeutralPixels[Variant - 74], Pixels, CurrentSize);
                Test->AddInfo(FString::Printf(TEXT("METAL_NATIVE_GGX_PARITY roughness=%g maskedRGB8Delta=%g; neutral artist gains versus independent DefaultLit material, identical real light/camera"), TestRoughnessForVariant(Variant), Difference));
                Test->TestTrue(TEXT("Neutral metal controls reproduce native DefaultLit GGX rather than an invented roughness lobe"), Difference < 1.);
            }
            if (Variant >= 77 && Variant <= 84) ValidateMetalModeBlend(Pixels, CurrentSize);
            if (Variant >= 88) ValidateFarBlockControls(Pixels, CurrentSize);
            static const TCHAR* Names[] = {TEXT("CellCaptain_OriginalMaterials"), TEXT("Metal_Specular0"), TEXT("Metal_Specular1"),
                TEXT("Metal_Roughness065"), TEXT("NonMetal"), TEXT("Metal_EnvironmentReflectionOff"),
                TEXT("Metal_CharacterPipeline"), TEXT("Metal_NativeDefaultLit"), TEXT("CellCaptain_CharacterPipeline"), TEXT("CellCaptain_NativeDefaultLit"),
                TEXT("Metal_Roughness045_StyleOff"), TEXT("Metal_Roughness045_StyleOn"),
                TEXT("NonMetal_StyleOff"), TEXT("NonMetal_TopDownOn"),
                TEXT("CellCaptain_StyleOff"), TEXT("CellCaptain_StyleOn"),
                TEXT("Metal_DirectOnly_StyleOff"), TEXT("Metal_DirectOnly_StyleOn"),
                TEXT("Metal_IndirectOnly_StyleOff"), TEXT("Metal_IndirectOnly_StyleOn"),
                TEXT("CellCaptain_TopDownNear_Prior"), TEXT("CellCaptain_TopDownNear_Enhanced"),
                TEXT("CellCaptain_TopDownFar_Prior"), TEXT("CellCaptain_TopDownFar_Enhanced"),
                TEXT("Metal_TopDownFar_Prior"), TEXT("Metal_TopDownFar_Enhanced"),
                TEXT("Metal_TopDownFar_LightSweep"), TEXT("Metal_TopDownBright_BloomOff"),
                TEXT("Metal_TopDownBright_BloomOn"), TEXT("Metal_TopDown_NoLighting"),
                TEXT("CellCaptain_300px_Mode2"), TEXT("CellCaptain_300px_NPR_Mode3"),
                TEXT("CellCaptain_150px_Mode2"), TEXT("CellCaptain_150px_NPR_Mode3"),
                TEXT("Metal_150px_Mode2"), TEXT("Metal_150px_NPR_Mode3"),
                TEXT("Metal_150px_NPR_LightSweep"), TEXT("Metal_150px_LowLight_Mode2"),
                TEXT("Metal_150px_LowLight_NPR_Mode3"), TEXT("Metal_150px_NPR_BloomOff"),
                TEXT("Metal_150px_NPR_BloomOn"), TEXT("Metal_150px_NPR_ZeroPhysicalLight"),
                TEXT("NonMetal_150px_Mode2"), TEXT("NonMetal_150px_NPR_Mode3"),
                TEXT("CellCaptain_150px_LowLight_Mode2"), TEXT("CellCaptain_150px_LowLight_NPR_Mode3"),
                TEXT("Metal_150px_ProfileOptOut_Mode2"), TEXT("Metal_150px_ProfileOptOut_Mode3"),
                TEXT("ArtistMetal_Default"), TEXT("ArtistMetal_WideBrightCore"), TEXT("ArtistMetal_ReflectionAlpha0"), TEXT("ArtistMetal_ReflectionAlpha1"),
                TEXT("MaterialMetal_Inherit"), TEXT("MaterialMetal_Wide"), TEXT("MaterialMetal_Narrow"), TEXT("MaterialMetal_Cyan"),
                TEXT("MaterialMetal_TexturePalette"), TEXT("MaterialMetal_Smooth"), TEXT("MaterialMetal_Rough"), TEXT("MaterialMetal_LowLight"), TEXT("MaterialMetal_ZeroLight"),
                TEXT("CellCaptain_MI_Inherit"), TEXT("CellCaptain_MI_WidthColorOverride"),
                TEXT("MetalF0_Gold_300px"), TEXT("MetalF0_Gold_150px"), TEXT("MetalF0_Copper_150px"),
                TEXT("MetalF0_Silver_150px"), TEXT("MetalF0_Black_150px"),
                TEXT("MetalDefaultWidth_Roughness005"), TEXT("MetalDefaultWidth_Roughness050"), TEXT("MetalDefaultWidth_Roughness095"),
                TEXT("MetalGGX_Neutral_Roughness005"), TEXT("MetalGGX_Neutral_Roughness050"), TEXT("MetalGGX_Neutral_Roughness095"),
                TEXT("MetalGGX_Native_Roughness005"), TEXT("MetalGGX_Native_Roughness050"), TEXT("MetalGGX_Native_Roughness095"),
                TEXT("MetalMode_PBR"), TEXT("MetalMode_Stylized_Blend0"), TEXT("MetalMode_Stylized_Blend1"),
                TEXT("MetalMode_Stylized_Blend050"), TEXT("MetalMode_Stylized_Blend025"), TEXT("MetalMode_Stylized_ZeroLight"),
                TEXT("MetalMode_NonMetal_PBR"), TEXT("MetalMode_NonMetal_Stylized"),
                TEXT("MetalMode_Stylized_Roughness005"), TEXT("MetalMode_Stylized_Roughness050"), TEXT("MetalMode_Stylized_Roughness095"),
                TEXT("MetalBlocks_Base0_150px"), TEXT("MetalBlocks_Base030_150px"), TEXT("MetalBlocks_Base060_150px"),
                TEXT("MetalBlocks_Dark080_150px"), TEXT("MetalBlocks_Base030_300px"), TEXT("MetalBlocks_Base030_Far150px"),
                TEXT("MetalBlocks_Spec0"), TEXT("MetalBlocks_Spec1")};
            Filename = Directory / FString::Printf(TEXT("%02d_%s.png"), Variant, Names[Variant]);
            // Persist the exact viewport readback used by the assertions. The
            // global deferred screenshot request can be consumed by another
            // editor viewport, leaving no evidence for an otherwise valid frame.
            TArray64<uint8> Png;
            FImageUtils::PNGCompressImageArray(CurrentSize.X, CurrentSize.Y,
                TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
            if (Png.IsEmpty() || !FFileHelper::SaveArrayToFile(Png, *Filename))
                return Fail(TEXT("Cannot save metallic viewport readback: ") + Filename);
            Test->AddInfo(FString::Printf(TEXT("CHARACTER_METALLIC_CAPTURE variant=%s file=%s; transient fixture, no asset save."), Names[Variant], *Filename));
            ReadyAt = FPlatformTime::Seconds() + 1.;
            State = 3;
        }
        if (State == 3 && FPlatformTime::Seconds() > ReadyAt + 30.)
            return Fail(TEXT("Timed out waiting for metallic screenshot: ") + Filename);
        if (State == 3 && FPlatformTime::Seconds() >= ReadyAt && IFileManager::Get().FileSize(*Filename) > 32)
        {
            if (++Variant == 96)
            {
                Test->AddInfo(TEXT("NPR_METAL_VISUAL_REVIEW_REQUIRED: numeric gates verify routing, visibility, motion, bloom and isolation only; inspect paired 150/300px character captures, material identity, block shapes and temporal stability before visual acceptance."));
                return true;
            }
            State = 1;
        }
        return false;
    }
private:
    struct FCvarState
    {
        IConsoleVariable* Variable = nullptr;
        FString Value;
        EConsoleVariableFlags Flags = ECVF_Default;
    };
    static void SetScopedCvar(TArray<FCvarState>& States, const TCHAR* Name, float Value)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
        {
            if (!States.ContainsByPredicate([Variable](const FCvarState& State) { return State.Variable == Variable; }))
            {
                FCvarState& State = States.AddDefaulted_GetRef();
                State.Variable = Variable;
                State.Value = Variable->GetString();
                State.Flags = Variable->GetFlags();
                UE_LOG(LogTemp, Display, TEXT("CHARACTER_METALLIC_SCOPED_CVAR name=%s before=%s test=%g"), Name, *State.Value, Value);
            }
            Variable->ClearFlags(ECVF_SetByMask);
            Variable->Set(Value, ECVF_SetByCode);
        }
    }
    static void RestoreCvars(TArray<FCvarState>& States)
    {
        for (const FCvarState& State : States)
        {
            State.Variable->ClearFlags(ECVF_SetByMask);
            State.Variable->Set(*State.Value, ECVF_SetByCode);
            State.Variable->ClearFlags(ECVF_SetByMask);
            State.Variable->SetFlags(static_cast<EConsoleVariableFlags>(State.Flags & ECVF_SetByMask));
        }
        States.Reset();
    }
    bool SetFixtureLit(FText& Message)
    {
        if (!GCurrentLevelEditingViewportClient) return false;
        FLevelEditorViewportClient* Client = GCurrentLevelEditingViewportClient;
        UE_LOG(LogTemp, Display, TEXT("CHARACTER_METALLIC_VIEW_BEFORE variant=%d view=%d materials=%d diffuse=%d specular=%d normal=%d lightingOnly=%d detailLighting=%d reflections=%d buffer=%d"),
            Variant, int32(Client->GetViewMode()), Client->EngineShowFlags.Materials, Client->EngineShowFlags.Diffuse,
            Client->EngineShowFlags.Specular, Client->EngineShowFlags.MaterialNormal, Client->EngineShowFlags.LightingOnlyOverride,
            Client->EngineShowFlags.OverrideDiffuseAndSpecular, Client->EngineShowFlags.ReflectionOverride, Client->EngineShowFlags.VisualizeBuffer);
        Client->ChangeBufferVisualizationMode(NAME_None);
        if (!FDevKitLookDevLevelService::SetPreviewViewMode(VMI_Lit, NAME_None, Message)) return false;
        Client->EngineShowFlags.SetMaterials(true);
        Client->EngineShowFlags.SetDiffuse(true);
        Client->EngineShowFlags.SetSpecular(true);
        Client->EngineShowFlags.SetMaterialNormal(true);
        Client->EngineShowFlags.SetLightingOnlyOverride(false);
        Client->EngineShowFlags.SetOverrideDiffuseAndSpecular(false);
        Client->EngineShowFlags.SetReflectionOverride(false);
        Client->EngineShowFlags.SetVisualizeBuffer(false);
        Client->Invalidate();
        return true;
    }
    struct FRigLightState
    {
        TWeakObjectPtr<ALight> Light;
        float Intensity = 0.f;
    };
    struct FNprMetrics
    {
        int32 Samples = 0;
        float P10 = 0.f, P90 = 0.f, Peak = 0.f;
        double Mean = 0., BrightFraction = 0., SharpEdgeFraction = 0.;
        FVector2D HighlightCentroid = FVector2D::ZeroVector;
    };
    static bool IsNprBaseline(int32 Index)
    {
        return Index == 30 || Index == 32 || Index == 34 || Index == 37 || Index == 42 || Index == 44 || Index == 46;
    }
    static FLinearColor MetalF0TestColor(int32 Index)
    {
        if (Index == 65) return FLinearColor(.95f, .30f, .15f);
        if (Index == 66) return FLinearColor(.8f, .8f, .8f);
        if (Index == 67) return FLinearColor::Black;
        return FLinearColor(.65f, .32f, .09f);
    }
    void ValidateMetalF0Capture(const TArray<FColor>& Pixels, FIntPoint Size)
    {
        TArray<FColor> Samples;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X, Y, Size)) Samples.Add(Pixels[Y * Size.X + X]);
        Samples.Sort([](FColor A, FColor B) { return NprLuma(A) > NprLuma(B); });
        const int32 Count = FMath::Max(1, Samples.Num() / 10);
        FLinearColor Mean(0.f, 0.f, 0.f, 0.f);
        int32 Clipped = 0;
        for (FColor Color : Samples) Clipped += Color.R >= 250 || Color.G >= 250 || Color.B >= 250;
        for (int32 I = 0; I < Count && I < Samples.Num(); ++I) Mean += FLinearColor(Samples[I]);
        Mean /= Count;
        Mean.A = 0.f; // GetMax below measures RGB radiance, not opaque alpha.
        const FLinearColor F0 = MetalF0TestColor(Variant);
        Test->AddInfo(FString::Printf(TEXT("METAL_F0_COLOR variant=%d heightPx=%g samples=%d highlightLinear=(%g,%g,%g) F0=(%g,%g,%g); white direct light, native Fresnel, no film/GI/reflection override"),
            Variant, NprProjectedHeight, Samples.Num(), Mean.R, Mean.G, Mean.B, F0.R, F0.G, F0.B));
        Test->TestTrue(TEXT("F0 capture has a real 150/300-pixel metal silhouette"), Samples.Num() > 100 && FMath::Abs(NprProjectedHeight - NprTargetHeight) < 2.f);
        Test->TestEqual(TEXT("F0 chromaticity fixture does not clip any metal RGB channel"), Clipped, 0);
        if (Variant == 67)
        {
            Test->TestTrue(TEXT("Black F0 does not acquire a white block/core pedestal"), Mean.GetMax() < .001f);
            return;
        }
        Test->TestTrue(TEXT("F0 highlight has measurable radiance without channel clipping"), Mean.R > .005f && Mean.GetMax() < .95f);
        Test->TestTrue(TEXT("Metallic block and core keep native F0 green-to-red chromaticity"),
            FMath::Abs(Mean.G / FMath::Max(Mean.R, 1.e-6f) - F0.G / F0.R) < .08f);
        Test->TestTrue(TEXT("Metallic block and core keep native F0 blue-to-red chromaticity instead of fixed white mixing"),
            FMath::Abs(Mean.B / FMath::Max(Mean.R, 1.e-6f) - F0.B / F0.R) < .06f);
    }
    static float TestRoughnessForVariant(int32 Index)
    {
        if (Index >= 88) return .5f;
        if (Index >= 77 && Index < 85) return .5f;
        const int32 Step = Index >= 85 ? Index - 85 : Index - 68;
        return Index >= 68 ? Step % 3 == 0 ? .05f : Step % 3 == 1 ? .5f : .95f : .5f;
    }
    void ValidateFarBlockControls(const TArray<FColor>& Pixels, FIntPoint Size)
    {
        TArray<float> Values;
        float Peak = 0.f;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X,Y,Size))
                {
                    const float Value = FLinearColor(Pixels[Y * Size.X + X]).GetLuminance();
                    Values.Add(Value); Peak = FMath::Max(Peak, Value);
                }
        int32 Bright = 0;
        for (float Value : Values) Bright += Value > Peak * .25f;
        const double Coverage = double(Bright) / FMath::Max(Values.Num(), 1);
        Test->AddInfo(FString::Printf(TEXT("METAL_FAR_BLOCK variant=%d heightPx=%g peakLinear=%g relativeCoverage=%g"), Variant, NprProjectedHeight, Peak, Coverage));
        Test->TestTrue(TEXT("New block fixture remains a visible finite colored metal lobe"), Values.Num() > 100 && Peak > .005f && Peak < .95f);
        if (Variant == 88) BaseBlockCoverage = Coverage;
        if (Variant == 89)
        {
            Test->TestTrue(TEXT("Base block size broadens actual distant highlight coverage"), Coverage > BaseBlockCoverage * 1.1);
            BaseBlockCoverage = Coverage; FarBlockReferencePixels = Pixels;
        }
        if (Variant == 90)
            Test->TestTrue(TEXT("Larger base size further broadens the same far highlight"), Coverage > BaseBlockCoverage * 1.1);
        if (Variant == 91)
        {
            double DarkBefore = 0., DarkAfter = 0., BrightError = 0.; int32 DarkCount = 0, BrightCount = 0;
            for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
                for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                    if (IsNprSample(X,Y,Size))
                    {
                        const int32 I = Y * Size.X + X;
                        const float A = FLinearColor(FarBlockReferencePixels[I]).GetLuminance();
                        const float B = FLinearColor(Pixels[I]).GetLuminance();
                        // The colored metal's measurable off-block tail is
                        // below 15% of the peak. A 2% gate can contain no SDR
                        // samples at all and would not actually test darkening.
                        if (A > .0001f && A < Peak * .15f) { DarkBefore += A; DarkAfter += B; ++DarkCount; }
                        if (A > Peak * .5f) { BrightError += FMath::Abs(B - A); ++BrightCount; }
                    }
            Test->AddInfo(FString::Printf(TEXT("METAL_DARK_BLOCK samples=%d before=%g after=%g brightMeanError=%g"), DarkCount, DarkBefore/FMath::Max(DarkCount,1), DarkAfter/FMath::Max(DarkCount,1), BrightError/FMath::Max(BrightCount,1)));
            Test->TestTrue(TEXT("Dark strengthening lowers only the real low-luminance specular tail"), DarkCount > 10 && DarkAfter < DarkBefore * .7);
            Test->TestTrue(TEXT("Dark strengthening leaves bright block/core intact"), BrightCount > 10 && BrightError/FMath::Max(BrightCount,1) < .003);
        }
        if (Variant == 92) NearBlockCoverage = Coverage;
        if (Variant == 93)
            Test->TestTrue(TEXT("Normalized block coverage survives camera pullback from 300 to 150 pixels"), FMath::Abs(Coverage - NearBlockCoverage) < .06 && Coverage > .05);
        if (Variant == 94) FarBlockReferencePixels = Pixels;
        if (Variant == 95)
        {
            const double Difference = NprDifference(FarBlockReferencePixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("METAL_BLOCK_SPEC_INVARIANCE maskedRGB8Delta=%g"), Difference));
            Test->TestTrue(TEXT("Stylized block metal ignores legacy dielectric Specular control"), Difference < 1.);
        }
    }
    void ValidateMetalModeBlend(const TArray<FColor>& Pixels, FIntPoint Size)
    {
        if (Variant == 77) MetalModePbrPixels = Pixels;
        if (Variant == 78)
        {
            const double Difference = NprDifference(MetalModePbrPixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("METAL_MODE_BLEND zeroBlendRGB8Delta=%g"), Difference));
            Test->TestTrue(TEXT("Stylized mode at zero blend exactly preserves the PBR endpoint"), Difference < 1.);
        }
        if (Variant == 79)
        {
            MetalModeStylizedPixels = Pixels;
            const double Difference = NprDifference(MetalModePbrPixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("METAL_MODE_BLEND fullStyleRGB8Delta=%g"), Difference));
            Test->TestTrue(TEXT("Full stylization visibly reshapes the actual metal lobe"), Difference > .25);
        }
        if (Variant == 80 || Variant == 81)
        {
            const float Alpha = Variant == 80 ? .5f : .25f;
            double Error = 0.;
            int32 Samples = 0;
            for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
                for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                    if (IsNprSample(X, Y, Size))
                    {
                        const int32 I = Y * Size.X + X;
                        const FLinearColor Expected = FMath::Lerp(FLinearColor(MetalModePbrPixels[I]), FLinearColor(MetalModeStylizedPixels[I]), Alpha);
                        const FLinearColor Actual(Pixels[I]);
                        Error += FMath::Abs(Expected.R - Actual.R) + FMath::Abs(Expected.G - Actual.G) + FMath::Abs(Expected.B - Actual.B);
                        ++Samples;
                    }
            Error /= FMath::Max(3 * Samples, 1);
            Test->AddInfo(FString::Printf(TEXT("METAL_MODE_BLEND alpha=%g linearRGBError=%g samples=%d"), Alpha, Error, Samples));
            Test->TestTrue(TEXT("Blend interpolates reflected radiance rather than display-space colors"), Samples > 100 && Error < .006);
            Test->TestTrue(TEXT("Intermediate blend remains distinguishable from both endpoints"),
                NprDifference(MetalModePbrPixels, Pixels, Size) > .1 && NprDifference(MetalModeStylizedPixels, Pixels, Size) > .1);
        }
        if (Variant == 82)
            Test->TestTrue(TEXT("Full stylization creates no highlight without actual illumination"), TopDownInnerMean(Pixels, Size, TopDownPixelRadius(Size)) < 3.);
        if (Variant == 83) MetalModeNonMetalPixels = Pixels;
        if (Variant == 84)
        {
            const double Difference = NprDifference(MetalModeNonMetalPixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("METAL_MODE_BLEND nonMetalRGB8Delta=%g"), Difference));
            Test->TestTrue(TEXT("Metal mode changes do not modify nonmetal shading"), Difference < 1.);
        }
    }
    void ValidateDefaultRoughnessResponse(const TArray<FColor>& Pixels, FIntPoint Size)
    {
        TArray<float> Radiance;
        float Peak = 0.f;
        float MaxGradient = 0.f;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X, Y, Size))
                {
                    const FLinearColor Color(Pixels[Y * Size.X + X]);
                    const float Value = Color.GetLuminance();
                    Radiance.Add(Value);
                    Peak = FMath::Max(Peak, Value);
                    for (FIntPoint Step : {FIntPoint(1, 0), FIntPoint(0, 1)})
                        if (IsNprSample(X + Step.X, Y + Step.Y, Size))
                        {
                            const float Neighbor = FLinearColor(Pixels[(Y + Step.Y) * Size.X + X + Step.X]).GetLuminance();
                            MaxGradient = FMath::Max(MaxGradient, FMath::Abs(Neighbor - Value));
                        }
                }
        int32 Bright = 0;
        for (float Value : Radiance) if (Value > Peak * .25f) ++Bright;
        const int32 Index = Variant >= 85 ? Variant - 85 : Variant - 68;
        RoughnessHighlightFraction[Index] = double(Bright) / FMath::Max(Radiance.Num(), 1);
        RoughnessPeak[Index] = Peak;
        RoughnessNormalizedGradient[Index] = MaxGradient / FMath::Max(Peak, 1.e-6f);
        Test->AddInfo(FString::Printf(TEXT("METAL_ROUGHNESS_RESPONSE variant=%d roughness=%g heightPx=%g samples=%d peakLinear=%g highlightFraction=%g normalizedMaxGradient=%g; inherited material width, isolated specular lobe, same real light, no GI/film/Bloom"),
            Variant, Index == 0 ? .05f : Index == 1 ? .5f : .95f, NprProjectedHeight,
            Radiance.Num(), Peak, RoughnessHighlightFraction[Index], RoughnessNormalizedGradient[Index]));
        Test->TestTrue(TEXT("Roughness-response fixture has a visible 150-pixel metal silhouette"),
            Radiance.Num() > 100 && Peak > .001f && FMath::Abs(NprProjectedHeight - 150.f) < 2.f);
        Test->TestTrue(TEXT("Roughness-response peak stays within the measurable SDR readback range"), Peak < .95f);
        if (Index == 2)
        {
            Test->TestTrue(TEXT("Polished metal has a narrower inherited highlight than medium roughness"),
                RoughnessHighlightFraction[0] * 1.2 < RoughnessHighlightFraction[1]);
            Test->TestTrue(TEXT("Rough metal has a wider inherited highlight than medium roughness"),
                RoughnessHighlightFraction[1] * 1.2 < RoughnessHighlightFraction[2]);
            Test->TestTrue(TEXT("Medium roughness lowers highlight peak instead of enlarging a constant-height block"),
                RoughnessPeak[1] < RoughnessPeak[0] * .4);
            Test->TestTrue(TEXT("High roughness further attenuates the highlight peak"),
                RoughnessPeak[2] < RoughnessPeak[1] * .6);
            Test->TestTrue(TEXT("Medium roughness softens the normalized highlight gradient, not just its brightness"),
                RoughnessNormalizedGradient[1] < RoughnessNormalizedGradient[0] * .65);
            Test->TestTrue(TEXT("High roughness keeps a diffuse falloff without a hard bright core"),
                RoughnessNormalizedGradient[2] < RoughnessNormalizedGradient[0] * .65);
        }
    }
    void SaveRigLights(UWorld* World)
    {
        if (!RigLightsBefore.IsEmpty()) return;
        for (TActorIterator<ALight> It(World); It; ++It)
            if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig")))
            {
                FRigLightState& Light = RigLightsBefore.AddDefaulted_GetRef();
                Light.Light = *It;
                Light.Intensity = It->GetLightComponent()->Intensity;
            }
    }
    bool ConfigureNprVariant(FText& Message)
    {
        if (!Subject.IsValid() || !GCurrentLevelEditingViewportClient || !GCurrentLevelEditingViewportClient->Viewport)
        {
            Message = FText::FromString(TEXT("NPR fixture has no subject or live viewport."));
            return false;
        }
        UWorld* World = GEditor->GetEditorWorldContext().World();
        RestoreCvars(ZeroLightCvars);
        SaveRigLights(World);
        // The metallic reference sphere overlaps the projected character box
        // from above. Hide only transient reference balls, retaining the floor,
        // so the real GBuffer mask cannot classify that ball as character metal.
        for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
            if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Reference"))
                && It->GetStaticMeshComponent()->GetStaticMesh()
                && It->GetStaticMeshComponent()->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Sphere.Sphere"))
                It->GetStaticMeshComponent()->SetVisibility(false, true);
        if (!KeyLight.IsValid())
        {
            for (TActorIterator<ADirectionalLight> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig"))) KeyLight = *It;
            for (TActorIterator<APostProcessVolume> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig"))) TestPost = *It;
            if (KeyLight.IsValid()) KeyRotation = KeyLight->GetActorRotation();
        }
        if (!KeyLight.IsValid() || !TestPost.IsValid())
        {
            Message = FText::FromString(TEXT("NPR fixture key light/post process was not found."));
            return false;
        }
        bNprActualCharacter = Variant < 34 || Variant == 44 || Variant == 45 || Variant == 61 || Variant == 62;
        if (!bNprActualCharacter)
        {
            if (!MaterialInstance.IsValid()) return false;
            MaterialInstance->SetScalarParameterValue(TEXT("TestMetallic"), Variant == 42 || Variant == 43 || Variant == 83 || Variant == 84 ? 0.f : 1.f);
            // Sharp-block, sweep and bloom contracts use polished metal. The
            // isolated 68..70 sequence separately requires physical GGX spread/
            // peak decay; 71..76 compare neutral gains to independent DefaultLit.
            // Actual character roughness maps stay intact.
            MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), Variant >= 34 && Variant <= 40 ? .25f : .45f);
            MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), .5f);
            if (Variant >= 52 && Variant <= 60)
            {
                MaterialInstance->SetScalarParameterValue(TEXT("TestHighlightWidth"), Variant == 52 ? 0.f : Variant == 53 ? 2.5f : Variant == 54 ? .2f : 1.f);
                MaterialInstance->SetVectorParameterValue(TEXT("TestHighlightColor"), Variant == 55 || Variant >= 57 ? FLinearColor(.02f, .8f, 1.f) : FLinearColor(-1.f, -1.f, -1.f));
                MaterialInstance->SetScalarParameterValue(TEXT("TestUseHighlightTexture"), Variant == 56 ? 1.f : 0.f);
                MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), Variant == 57 ? 0.f : Variant >= 58 ? .85f : .45f);
            }
            if (Variant >= 63)
            {
                MaterialInstance->SetVectorParameterValue(TEXT("TestBaseColor"), MetalF0TestColor(Variant));
                MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), TestRoughnessForVariant(Variant));
                MaterialInstance->SetScalarParameterValue(TEXT("TestHighlightWidth"), 0.f);
                MaterialInstance->SetVectorParameterValue(TEXT("TestHighlightColor"), FLinearColor(-1.f, -1.f, -1.f));
                MaterialInstance->SetScalarParameterValue(TEXT("TestUseHighlightTexture"), 0.f);
            }
        }
        // The material's own profile is the opt-in. No global profile override
        // can turn this material choice into a project-wide NPR replacement.
        if (Settings->CharacterLightingProfiles.IsEmpty()) Settings->CharacterLightingProfiles.AddDefaulted();
        for (FStylizedCharacterLightingProfile& Profile : Settings->CharacterLightingProfiles)
            Profile.bEnableNPRMetal = Variant != 46 && Variant != 47;
        Settings->bEnableEnvironmentReflections = Variant < 63;
        Settings->bEnableCharacterDirectLighting = true;
        Settings->NonMetalReflectionAlpha = .3f;
        Settings->MetalReflectionAlpha = Variant == 50 ? 0.f : Variant == 51 ? 1.f : .7f;
        for (auto& Profile : Settings->CharacterLightingProfiles)
        {
            Profile.bOverrideMetalArt = Variant == 49 || Variant >= 63;
            if (Variant == 49)
            {
                Profile.MetalArt = Settings->MetalArt;
                Profile.MetalArt.HighlightWidth = 3.f;
                Profile.MetalArt.CoreStrength = 7.f;
            }
            if (Variant >= 63)
            {
                // Neutral face gains isolate specular shape, peak decay and
                // blend interpolation from all other artist controls.
                Profile.MetalArt = Settings->MetalArt;
                Profile.MetalArt.LightFace = 0.f;
                Profile.MetalArt.DarkFace = 0.f;
                Profile.MetalArt.HighlightWidth = 1.f;
                Profile.MetalArt.BaseBlockSize = 0.f;
                Profile.MetalArt.DarkSideStrength = 0.f;
                Profile.MetalArt.HighlightMode = Variant <= 77 || Variant == 83
                    ? EStylizedMetalHighlightMode::PBR : EStylizedMetalHighlightMode::Stylized;
                Profile.MetalArt.Stylization = Variant == 77 ? .73f : Variant == 78 ? 0.f : Variant == 80 ? .5f : Variant == 81 ? .25f : 1.f;
                if (Variant >= 71 && Variant <= 76)
                {
                    Profile.MetalArt.HighlightStrength = 1.f;
                    Profile.MetalArt.CoreStrength = 0.f;
                    Profile.MetalArt.HighlightTint = FLinearColor::White;
                }
                if (Variant >= 88)
                {
                    Profile.MetalArt.BaseBlockSize = Variant == 88 ? 0.f : Variant == 90 ? .6f : .3f;
                    Profile.MetalArt.DarkSideStrength = Variant == 91 ? .8f : 0.f;
                    Profile.MetalArt.Stylization = 1.f;
                }
            }
        }
        Settings->ApplyShadowTransitionParameters();
        if (Variant >= 94)
            MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), Variant == 94 ? 0.f : 1.f);
        TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> UploadedProfiles;
        uint32 UploadedCount = 0;
        GetStylizedCharacterLightingProfiles(UploadedProfiles, UploadedCount);
        for (uint32 Index = 0; Index < UploadedCount; ++Index)
            Test->TestEqual(TEXT("Material-profile NPR opt-in reaches GPU table slot 11.w"),
                UploadedProfiles[Index * StylizedCharacterLighting::Float4sPerProfile + 11].W,
                Variant == 46 || Variant == 47 ? 0.f : 1.f);
        if (Variant >= 74 && Variant <= 76)
        {
            UStaticMeshComponent* Owner = Subject->GetStaticMeshComponent();
            if (!NativeRoughnessInstance.IsValid())
                NativeRoughnessInstance = UMaterialInstanceDynamic::Create(MakeNativeMaterial(MaterialInstance->GetMaterial(), Owner), Owner);
            NativeRoughnessInstance->SetVectorParameterValue(TEXT("TestBaseColor"), MetalF0TestColor(Variant));
            NativeRoughnessInstance->SetScalarParameterValue(TEXT("TestMetallic"), 1.f);
            NativeRoughnessInstance->SetScalarParameterValue(TEXT("TestSpecular"), .5f);
            NativeRoughnessInstance->SetScalarParameterValue(TEXT("TestRoughness"), TestRoughnessForVariant(Variant));
            Owner->SetMaterial(0, NativeRoughnessInstance.Get());
        }
        if (Variant == 77) Subject->GetStaticMeshComponent()->SetMaterial(0, MaterialInstance.Get());
        const bool bLowLight = Variant == 37 || Variant == 38 || Variant == 44 || Variant == 45 || Variant == 59;
        for (const FRigLightState& Light : RigLightsBefore)
            if (Light.Light.IsValid())
            {
                Light.Light->GetLightComponent()->SetIntensity(Variant == 41 || Variant == 60 || Variant == 82 ? 0.f : Light.Intensity * (bLowLight ? .1f : 1.f));
                Light.Light->GetLightComponent()->MarkRenderStateDirty();
            }
        KeyLight->SetActorRotation(KeyRotation + FRotator(0, Variant == 36 ? 55.f : 0.f, 0));
        if (Variant == 39 || Variant == 40) KeyLight->GetLightComponent()->SetIntensity(20000.f);
        if (Variant >= 63)
        {
            // Calibrate a finite physical area source so the 8-bit SDR readback
            // can measure both polished peaks and rough tails without clipping.
            // Native and enhanced/reference captures use the exact same light.
            // Only this transient fixture changes; no project setting is saved.
            for (const FRigLightState& Light : RigLightsBefore)
                if (Light.Light.IsValid())
                {
                    Light.Light->GetLightComponent()->SetIntensity(Variant != 82 && Light.Light.Get() == KeyLight.Get() ? 10.f : 0.f);
                    Light.Light->GetLightComponent()->MarkRenderStateDirty();
                }
            KeyLight->GetLightComponent()->SetLightColor(FLinearColor::White);
            if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(KeyLight->GetLightComponent()))
                Directional->SetLightSourceAngle(30.f);
            KeyLight->SetActorRotation((-FVector(-1, -1, 1.5).GetSafeNormal()).Rotation());
        }
        KeyLight->GetLightComponent()->MarkRenderStateDirty();
        FPostProcessSettings& Post = TestPost->Settings;
        Post.bOverride_BloomIntensity = true;
        Post.BloomIntensity = Variant == 40 ? 1.f : 0.f;
        Post.bOverride_BloomThreshold = true;
        Post.BloomThreshold = 1.f;
        if (Variant == 41 || Variant == 60 || Variant == 82)
        {
            // Neutral creates no skylight or captures. Assert that assumption
            // rather than silently forgetting ASkyLight (which is not ALight).
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                TArray<USkyLightComponent*> SkyLights;
                It->GetComponents(SkyLights);
                for (USkyLightComponent* Sky : SkyLights)
                    Test->TestTrue(TEXT("Zero-light fixture has no contributing sky light"), !Sky->IsVisible() || Sky->Intensity == 0.f);
                Test->TestFalse(TEXT("Zero-light fixture has no baked reflection capture source"), It->IsA<AReflectionCapture>());
            }
            for (TActorIterator<ALight> It(World); It; ++It)
                Test->TestTrue(TEXT("All fixture light components have zero intensity"), It->GetLightComponent()->Intensity == 0.f);
            // The preceding 20,000-lux bloom variant leaves bright Lumen cards
            // and temporal/radiance history. Rebuild physical illumination, not
            // the shader: GI, environment reflections, NPR and direct-light
            // feature gates all remain enabled for the no-emissive assertion.
            SetScopedCvar(ZeroLightCvars, TEXT("r.LumenScene.SurfaceCache.Reset"), 1.f);
            SetScopedCvar(ZeroLightCvars, TEXT("r.LumenScene.Lighting.ForceLightingUpdate"), 1.f);
            SetScopedCvar(ZeroLightCvars, TEXT("r.Lumen.RadianceCache.ForceFullUpdate"), 1.f);
            SetScopedCvar(ZeroLightCvars, TEXT("r.Lumen.ScreenProbeGather.Temporal.ClearHistoryEveryFrame"), 1.f);
            SetScopedCvar(ZeroLightCvars, TEXT("r.Lumen.Reflections.Temporal"), 0.f);
        }
        FLevelEditorViewportClient* Client = GCurrentLevelEditingViewportClient;
        if (Variant >= 63 && !MetalColorShowFlagsBefore.IsSet()) MetalColorShowFlagsBefore = Client->EngineShowFlags;
        Client->EngineShowFlags.SetBloom(Variant < 63);
        Client->EngineShowFlags.SetTonemapper(Variant < 63);
        Client->EngineShowFlags.SetGlobalIllumination(Variant < 63);
        const FIntPoint Size = Client->Viewport->GetSizeXY();
        FVector Target, Extent;
        Subject->GetActorBounds(false, Target, Extent);
        TopDownFOV = GetDefault<APlayerCharacterBase>()->DefaultCameraFOV;
        NprTargetHeight = Variant == 30 || Variant == 31 || Variant == 61 || Variant == 62 || Variant == 63 || Variant == 92 ? 300.f : 150.f;
        const float FocalLength = Size.X * .5f / FMath::Tan(FMath::DegreesToRadians(TopDownFOV * .5f));
        const FVector ViewDirection = FVector(-1, -1, 1.5).GetSafeNormal();
        const FRotator ViewRotation = (-ViewDirection).Rotation();
        const FVector Up = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Z);
        const FVector Right = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Y);
        auto ProjectBounds = [&](float Distance, FVector2D& Min, FVector2D& Max)
        {
            Min = FVector2D(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
            Max = -Min;
            for (int32 Corner = 0; Corner < 8; ++Corner)
            {
                const FVector Offset(Extent.X * ((Corner & 1) ? 1 : -1), Extent.Y * ((Corner & 2) ? 1 : -1), Extent.Z * ((Corner & 4) ? 1 : -1));
                const float Depth = FMath::Max(1., Distance - FVector::DotProduct(Offset, ViewDirection));
                const FVector2D Pixel(Size.X * .5f + FocalLength * FVector::DotProduct(Offset, Right) / Depth,
                    Size.Y * .5f - FocalLength * FVector::DotProduct(Offset, Up) / Depth);
                Min.X = FMath::Min(Min.X, Pixel.X); Min.Y = FMath::Min(Min.Y, Pixel.Y);
                Max.X = FMath::Max(Max.X, Pixel.X); Max.Y = FMath::Max(Max.Y, Pixel.Y);
            }
        };
        FVector2D Min, Max;
        if (bNprActualCharacter)
        {
            float Lower = Extent.Size() * 1.1f, Upper = FMath::Max(10000.f, Lower * 100.f);
            for (int32 Iteration = 0; Iteration < 24; ++Iteration)
            {
                TopDownDistance = (Lower + Upper) * .5f;
                ProjectBounds(TopDownDistance, Min, Max);
                if (Max.Y - Min.Y > NprTargetHeight) Lower = TopDownDistance; else Upper = TopDownDistance;
            }
        }
        else
        {
            // Fit a sphere's silhouette, not its enclosing cube. The actual
            // material sample remains 150 pixels across at any viewport size.
            TopDownDistance = Extent.GetMin() * FMath::Sqrt(1.f + FMath::Square(2.f * FocalLength / NprTargetHeight));
        }
        ProjectBounds(TopDownDistance, Min, Max);
        NprBounds = FIntRect(FMath::Clamp(FMath::FloorToInt(Min.X), 0, Size.X - 1), FMath::Clamp(FMath::FloorToInt(Min.Y), 0, Size.Y - 1),
            FMath::Clamp(FMath::CeilToInt(Max.X), 1, Size.X), FMath::Clamp(FMath::CeilToInt(Max.Y), 1, Size.Y));
        NprProjectedHeight = bNprActualCharacter ? Max.Y - Min.Y : NprTargetHeight;
        TopDownSubjectRadius = bNprActualCharacter ? Extent.Size() : Extent.GetMin();
        const FVector Location = Target + ViewDirection * TopDownDistance;
        Client->SetViewLocation(Location);
        Client->SetLookAtLocation(Target);
        Client->SetViewRotation(ViewRotation);
        Client->ViewFOV = TopDownFOV;
        Client->FOVAngle = TopDownFOV;
        Client->Invalidate();
        return true;
    }
    bool IsNprSample(int32 X, int32 Y, FIntPoint Size) const
    {
        if (X < NprBounds.Min.X || X >= NprBounds.Max.X || Y < NprBounds.Min.Y || Y >= NprBounds.Max.Y) return false;
        if (bNprActualCharacter)
            return NprMetalMaskPixels.Num() == Size.X * Size.Y && NprMetalMaskPixels[Y * Size.X + X].R >= 160;
        return FMath::Square(X - Size.X * .5f) + FMath::Square(Y - Size.Y * .5f) < FMath::Square(NprTargetHeight * .45f);
    }
    static float NprLuma(FColor Color) { return .2126f * Color.R + .7152f * Color.G + .0722f * Color.B; }
    FNprMetrics MeasureNpr(const TArray<FColor>& Pixels, FIntPoint Size) const
    {
        FNprMetrics Result;
        TArray<float> Values;
        int32 Bright = 0, Sharp = 0, Edges = 0;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X, Y, Size))
                {
                    const float Luma = NprLuma(Pixels[Y * Size.X + X]);
                    Values.Add(Luma); Result.Mean += Luma; Result.Peak = FMath::Max(Result.Peak, Luma); Bright += Luma >= 200.f;
                    for (FIntPoint Step : {FIntPoint(1, 0), FIntPoint(0, 1)})
                        if (IsNprSample(X + Step.X, Y + Step.Y, Size))
                        {
                            ++Edges;
                            Sharp += FMath::Abs(NprLuma(Pixels[(Y + Step.Y) * Size.X + X + Step.X]) - Luma) >= 20.f;
                        }
                }
        Result.Samples = Values.Num();
        if (Values.IsEmpty()) return Result;
        Values.Sort();
        Result.P10 = Values[(Values.Num() - 1) / 10];
        Result.P90 = Values[(Values.Num() - 1) * 9 / 10];
        Result.Mean /= Values.Num();
        Result.BrightFraction = double(Bright) / Values.Num();
        Result.SharpEdgeFraction = double(Sharp) / FMath::Max(Edges, 1);
        const float HighlightThreshold = Values[(Values.Num() - 1) * 3 / 4];
        double WeightSum = 0.;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X, Y, Size))
                {
                    const float Weight = FMath::Max(NprLuma(Pixels[Y * Size.X + X]) - HighlightThreshold, 0.f);
                    Result.HighlightCentroid += FVector2D(X, Y) * Weight;
                    WeightSum += Weight;
                }
        Result.HighlightCentroid = WeightSum > 1.e-6 ? Result.HighlightCentroid / WeightSum : FVector2D(Size.X * .5, Size.Y * .5);
        return Result;
    }
    double NprDifference(const TArray<FColor>& A, const TArray<FColor>& B, FIntPoint Size) const
    {
        double Sum = 0.; int32 Count = 0;
        for (int32 Y = NprBounds.Min.Y; Y < NprBounds.Max.Y; ++Y)
            for (int32 X = NprBounds.Min.X; X < NprBounds.Max.X; ++X)
                if (IsNprSample(X, Y, Size))
                {
                    const int32 Index = Y * Size.X + X;
                    Sum += FMath::Abs(int32(A[Index].R) - int32(B[Index].R)) + FMath::Abs(int32(A[Index].G) - int32(B[Index].G)) + FMath::Abs(int32(A[Index].B) - int32(B[Index].B));
                    ++Count;
                }
        return Sum / FMath::Max(3 * Count, 1);
    }
    void ValidateNprCapture(const TArray<FColor>& Pixels, FIntPoint Size)
    {
        const FNprMetrics Metrics = MeasureNpr(Pixels, Size);
        Test->TestTrue(TEXT("NPR capture matches the requested 150/300-pixel projection"), FMath::Abs(NprProjectedHeight - NprTargetHeight) < 2.f);
        Test->TestTrue(TEXT("NPR subject mask contains meaningful visible material samples"), Metrics.Samples >= (bNprActualCharacter ? 4 : 100));
        Test->AddInfo(FString::Printf(TEXT("NPR_METAL_METRICS variant=%d mode=%g distanceCm=%g projectedHeight=%g targetHeight=%g sampleType=%s samples=%d mean=%g p10=%g p90=%g peak=%g brightFraction200=%g sharpEdgeFraction20=%g centroid=(%g,%g); diagnostic, not visual approval"),
            Variant, MetalStyle->GetFloat(), TopDownDistance, NprProjectedHeight, NprTargetHeight,
            bNprActualCharacter ? TEXT("GBufferMetallic>=160") : TEXT("sphereInterior"), Metrics.Samples,
            Metrics.Mean, Metrics.P10, Metrics.P90, Metrics.Peak, Metrics.BrightFraction, Metrics.SharpEdgeFraction,
            Metrics.HighlightCentroid.X, Metrics.HighlightCentroid.Y));
        if (IsNprBaseline(Variant)) { NprComparisonPixels = Pixels; NprBaselineMetrics = Metrics; }
        if (Variant == 31 || Variant == 33 || Variant == 35 || Variant == 38 || Variant == 43 || Variant == 45 || Variant == 47)
        {
            const double Difference = NprDifference(NprComparisonPixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("NPR_METAL_PAIR variant=%d maskedRGB8Delta=%g contrastDelta=%g peakDelta=%g brightCoverageDelta=%g sharpEdgeDelta=%g"),
                Variant, Difference, (Metrics.P90 - Metrics.P10) - (NprBaselineMetrics.P90 - NprBaselineMetrics.P10),
                Metrics.Peak - NprBaselineMetrics.Peak, Metrics.BrightFraction - NprBaselineMetrics.BrightFraction,
                Metrics.SharpEdgeFraction - NprBaselineMetrics.SharpEdgeFraction));
            if (Variant == 43 || Variant == 47)
                Test->TestTrue(TEXT("NPR preserves nonmetal and profile-opt-out material output within temporal tolerance"), Difference < 2.);
            if (Variant == 35)
            {
                Test->TestTrue(TEXT("NPR routes distinct metal shading at gameplay pixel scale"), Difference > .25);
                Test->TestTrue(TEXT("NPR small metal sphere retains a readable light/dark range"), Metrics.P90 - Metrics.P10 > 12.f);
                Test->TestTrue(TEXT("NPR small metal sphere retains sharp interior highlight edges"), Metrics.SharpEdgeFraction > .001);
            }
            if (Variant == 38)
                Test->TestTrue(TEXT("NPR low-light metal remains responsive without requiring bright daylight"), Metrics.Peak > 10.f && Metrics.Peak - Metrics.P10 > 3.f);
        }
        if (Variant == 35) { NprSweepPixels = Pixels; NprSweepMetrics = Metrics; }
        if (Variant == 36)
        {
            const double Shift = FVector2D::Distance(Metrics.HighlightCentroid, NprSweepMetrics.HighlightCentroid);
            const double Difference = NprDifference(NprSweepPixels, Pixels, Size);
            Test->AddInfo(FString::Printf(TEXT("NPR_METAL_SWEEP highlightCentroidShiftPixels=%g maskedRGB8Delta=%g; key yaw +55 degrees, geometry/camera unchanged"), Shift, Difference));
            Test->TestTrue(TEXT("NPR highlight moves across the structure when the real key light moves"), Shift > 1. && Difference > .25);
        }
        if (Variant == 39) NprBloomPixels = Pixels;
        if (Variant == 40)
        {
            const double HaloGain = TopDownBloomHaloGain(NprBloomPixels, Pixels, Size, NprTargetHeight * .5f);
            Test->AddInfo(FString::Printf(TEXT("NPR_METAL_BLOOM positiveHaloRGB8Gain=%g; native bloom, same real 20000-lux key"), HaloGain));
            Test->TestTrue(TEXT("NPR bright metal radiance supports native bloom outside the silhouette"), HaloGain > .01);
        }
        if (Variant == 41)
            Test->TestTrue(TEXT("NPR produces no emissive floor with zero physical light while all character lighting features remain enabled"), Metrics.Mean < 3. && Metrics.Peak < 8.f);
    }
    bool ConfigureTopDownVariant(FText& Message)
    {
        if (!Subject.IsValid() || !GCurrentLevelEditingViewportClient) return false;
        UWorld* World = GEditor->GetEditorWorldContext().World();
        if (Variant == 20)
        {
            for (TActorIterator<ADirectionalLight> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig"))) KeyLight = *It;
            for (TActorIterator<APostProcessVolume> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig"))) TestPost = *It;
            if (!KeyLight.IsValid() || !TestPost.IsValid())
            {
                Message = FText::FromString(TEXT("Top-down fixture key light/post process volume was not found."));
                return false;
            }
            KeyRotation = KeyLight->GetActorRotation();
            KeyIntensity = KeyLight->GetLightComponent()->Intensity;
            SaveRigLights(World);
        }
        if (Variant >= 24)
        {
            if (!MaterialInstance.IsValid()) return false;
            MaterialInstance->SetScalarParameterValue(TEXT("TestMetallic"), 1.f);
            MaterialInstance->SetScalarParameterValue(TEXT("TestRoughness"), .45f);
            MaterialInstance->SetScalarParameterValue(TEXT("TestSpecular"), .5f);
        }
        Settings->bEnableEnvironmentReflections = Variant != 29;
        Settings->bEnableCharacterDirectLighting = Variant != 29;
        Settings->ApplyShadowTransitionParameters();
        KeyLight->SetActorRotation(KeyRotation + FRotator(0, Variant >= 26 ? 55.f : 0.f, 0));
        KeyLight->GetLightComponent()->SetIntensity(Variant >= 27 ? 20000.f : KeyIntensity);
        KeyLight->GetLightComponent()->MarkRenderStateDirty();
        if (Variant == 29)
        {
            // Turn off the physical rig as well as character feature gates:
            // other bright test objects must not leak a bloom halo into this
            // black-floor check. The four-second warmup clears prior history.
            for (TActorIterator<ALight> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("Dev01.CharacterLookDev.Rig")))
                {
                    It->GetLightComponent()->SetIntensity(0.f);
                    It->GetLightComponent()->MarkRenderStateDirty();
                }
        }
        FPostProcessSettings& Post = TestPost->Settings;
        Post.bOverride_BloomIntensity = true;
        Post.BloomIntensity = Variant >= 28 ? 1.f : 0.f;
        Post.bOverride_BloomThreshold = true;
        Post.BloomThreshold = 1.f;
        // Keep exposure and grading unchanged. This only enables the native
        // bloom stage in the disposable viewport, never project configuration.
        FLevelEditorViewportClient* Client = GCurrentLevelEditingViewportClient;
        Client->EngineShowFlags.SetBloom(true);
        FVector Target, Extent;
        Subject->GetActorBounds(false, Target, Extent);
        const APlayerCharacterBase* Defaults = GetDefault<APlayerCharacterBase>();
        TopDownFOV = Defaults->DefaultCameraFOV;
        TopDownDistance = Defaults->DefaultCameraBoomLength * (Variant == 22 || Variant == 23 ? 1.f : .5f);
        TopDownSubjectRadius = Variant >= 24 ? Extent.GetMin() : Extent.Size();
        const FVector Location = Target + FVector(-1, -1, 1.5).GetSafeNormal() * TopDownDistance;
        Client->SetViewLocation(Location);
        Client->SetLookAtLocation(Target);
        Client->SetViewRotation((Target - Location).Rotation());
        Client->ViewFOV = TopDownFOV;
        Client->FOVAngle = TopDownFOV;
        Client->Invalidate();
        return true;
    }
    float TopDownPixelRadius(FIntPoint Size) const
    {
        return FMath::Max(2.f, .5f * Size.X * TopDownSubjectRadius / (TopDownDistance * FMath::Tan(FMath::DegreesToRadians(TopDownFOV * .5f))));
    }
    static double TopDownDifference(const TArray<FColor>& A, const TArray<FColor>& B, FIntPoint Size, float Radius)
    {
        double Sum = 0.;
        int32 Count = 0;
        for (int32 Y = FMath::Max(0, FMath::FloorToInt(Size.Y * .5f - Radius)); Y < FMath::Min(Size.Y, FMath::CeilToInt(Size.Y * .5f + Radius)); ++Y)
            for (int32 X = FMath::Max(0, FMath::FloorToInt(Size.X * .5f - Radius)); X < FMath::Min(Size.X, FMath::CeilToInt(Size.X * .5f + Radius)); ++X)
            {
                if (FMath::Square(X - Size.X * .5f) + FMath::Square(Y - Size.Y * .5f) > Radius * Radius) continue;
                const int32 Index = Y * Size.X + X;
                Sum += FMath::Abs(int32(A[Index].R) - int32(B[Index].R)) + FMath::Abs(int32(A[Index].G) - int32(B[Index].G)) + FMath::Abs(int32(A[Index].B) - int32(B[Index].B));
                ++Count;
            }
        return Sum / FMath::Max(3 * Count, 1);
    }
    static double TopDownBloomHaloGain(const TArray<FColor>& A, const TArray<FColor>& B, FIntPoint Size, float Radius)
    {
        double Sum = 0.;
        int32 Count = 0;
        for (int32 Y = 0; Y < Size.Y; ++Y)
            for (int32 X = 0; X < Size.X; ++X)
            {
                const float DistanceSq = FMath::Square(X - Size.X * .5f) + FMath::Square(Y - Size.Y * .5f);
                if (DistanceSq < FMath::Square(Radius * 1.15f) || DistanceSq > FMath::Square(Radius * 2.f)) continue;
                const int32 Index = Y * Size.X + X;
                Sum += FMath::Max(int32(B[Index].R) - int32(A[Index].R), 0) + FMath::Max(int32(B[Index].G) - int32(A[Index].G), 0) + FMath::Max(int32(B[Index].B) - int32(A[Index].B), 0);
                ++Count;
            }
        return Sum / FMath::Max(3 * Count, 1);
    }
    static double TopDownInnerMean(const TArray<FColor>& Pixels, FIntPoint Size, float Radius)
    {
        double Sum = 0.;
        int32 Count = 0;
        // Interior only: exclude the antialiased silhouette and other rig geometry.
        for (int32 Y = 0; Y < Size.Y; ++Y)
            for (int32 X = 0; X < Size.X; ++X)
            {
                if (FMath::Square(X - Size.X * .5f) + FMath::Square(Y - Size.Y * .5f) > FMath::Square(Radius * .6f)) continue;
                const FColor& Pixel = Pixels[Y * Size.X + X];
                Sum += Pixel.R + Pixel.G + Pixel.B;
                ++Count;
            }
        return Sum / FMath::Max(3 * Count, 1);
    }
    static UMaterialInstanceConstant* MakeNativeMaterial(UMaterialInterface* Source, UObject* Owner)
    {
        UMaterialInstanceConstant* Native = NewObject<UMaterialInstanceConstant>(Owner, NAME_None, RF_Transient);
        Native->SetParentEditorOnly(Source);
        Native->BasePropertyOverrides.bOverride_ShadingModel = true;
        Native->BasePropertyOverrides.ShadingModel = MSM_DefaultLit;
        Native->PostEditChange();
        return Native;
    }
    static void AuditMetallicTexture(UMaterialInterface* Material)
    {
        UTexture* Texture = nullptr;
        if (!Material || !Material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("T_MixMap")), Texture)) return;
        UTexture2D* Mix = Cast<UTexture2D>(Texture);
        if (!Mix || Mix->Source.GetFormat() != TSF_BGRA8) return;
        TArray64<uint8> Raw;
        if (!Mix->Source.GetMipData(Raw, 0)) return;
        double RoughnessSum = 0.;
        int64 MetalCount = 0;
        for (int64 I = 0; I + 3 < Raw.Num(); I += 4)
            if (Raw[I] >= 204) { RoughnessSum += Raw[I + 1] / 255.; ++MetalCount; }
        UE_LOG(LogTemp, Display, TEXT("METALLIC_TEXTURE_AUDIT material=%s texture=%s metalFraction=%g metalMeanRoughness=%g sRGB=%d"),
            *Material->GetPathName(), *Mix->GetPathName(), MetalCount / double(FMath::Max<int64>(Raw.Num() / 4, 1)),
            RoughnessSum / FMath::Max<int64>(MetalCount, 1), Mix->SRGB ? 1 : 0);
    }
    static void AuditMaterialParameters(UMaterialInterface* Material)
    {
        if (!Material) return;
        TArray<FMaterialParameterInfo> Parameters;
        TArray<FGuid> Guids;
        Material->GetAllTextureParameterInfo(Parameters, Guids);
        for (const FMaterialParameterInfo& Parameter : Parameters)
        {
            UTexture* Texture = nullptr;
            Material->GetTextureParameterValue(Parameter, Texture);
            UE_LOG(LogTemp, Display, TEXT("CHARACTER_MATERIAL_TEXTURE material=%s parameter=%s texture=%s"),
                *Material->GetPathName(), *Parameter.Name.ToString(), *GetPathNameSafe(Texture));
        }
        Parameters.Reset(); Guids.Reset();
        Material->GetAllVectorParameterInfo(Parameters, Guids);
        for (const FMaterialParameterInfo& Parameter : Parameters)
        {
            FLinearColor Color;
            if (Material->GetVectorParameterValue(Parameter, Color))
                UE_LOG(LogTemp, Display, TEXT("CHARACTER_MATERIAL_VECTOR material=%s parameter=%s value=%s"),
                    *Material->GetPathName(), *Parameter.Name.ToString(), *Color.ToString());
        }
    }
    static double MeanDifference(const TArray<FColor>& A, const TArray<FColor>& B, FIntPoint Size)
    {
        if (A.Num() != B.Num() || A.IsEmpty() || A.Num() != Size.X * Size.Y) return TNumericLimits<double>::Max();
        double Sum = 0.;
        int32 Samples = 0;
        // Camera is fitted to the centered sphere. Compare the subject, not temporal background pixels.
        for (int32 Y = Size.Y * 3 / 10; Y < Size.Y * 7 / 10; ++Y)
            for (int32 X = Size.X * 35 / 100; X < Size.X * 65 / 100; ++X)
            {
                const int32 Index = Y * Size.X + X;
                Sum += FMath::Abs(int32(A[Index].R) - int32(B[Index].R)) + FMath::Abs(int32(A[Index].G) - int32(B[Index].G)) + FMath::Abs(int32(A[Index].B) - int32(B[Index].B));
                ++Samples;
            }
        return Sum / (3. * Samples);
    }
    bool Fail(const FString& Message) { Test->AddError(Message); return true; }
    FAutomationTestBase* Test;
    double Started, ReadyAt = 0.;
    int32 State = 0, Variant = 0;
    int32 LastLoggedState = INDEX_NONE, LastLoggedVariant = INDEX_NONE;
    uint32 ProfileCountBefore = 0;
    FString Directory, Filename;
    FIntPoint RenderSize = FIntPoint::ZeroValue;
    UStylizedLightingSettings* Settings;
    bool bReflectionsBefore, bMixBefore, bDirectLightingBefore;
    float NonMetalAlphaBefore = .3f, MetalAlphaBefore = .7f;
    TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> ProfilesBefore;
    TArray<FStylizedCharacterLightingProfile> CpuProfilesBefore;
    TArray<FRigLightState> RigLightsBefore;
    TArray<FCvarState> ZeroLightCvars, MaterialDebugCvars;
    TArray<FColor> NprMetalMaskPixels, NprComparisonPixels, NprSweepPixels, NprBloomPixels;
    TArray<FColor> ArtistComparisonPixels;
    TArray<FColor> MaterialOverrideBasePixels, MaterialOverrideWidePixels, MaterialOverrideSmoothPixels;
    TArray<FColor> ActualMaterialOverridePixels;
    TArray<FColor> NativeGgxNeutralPixels[3];
    TArray<FColor> MetalModePbrPixels, MetalModeStylizedPixels, MetalModeNonMetalPixels;
    TArray<FColor> FarBlockReferencePixels;
    double BaseBlockCoverage = 0., NearBlockCoverage = 0.;
    TWeakObjectPtr<UMaterialInstanceDynamic> NativeRoughnessInstance;
    TOptional<FEngineShowFlags> MetalColorShowFlagsBefore;
    double MaterialOverrideLightMean = 0.;
    double RoughnessHighlightFraction[3] = {};
    double RoughnessPeak[3] = {};
    double RoughnessNormalizedGradient[3] = {};
    FNprMetrics NprBaselineMetrics, NprSweepMetrics;
    FIntRect NprBounds;
    bool bNprActualCharacter = false;
    float NprTargetHeight = 150.f, NprProjectedHeight = 0.f;
    TArray<FColor> MetalLowSpecPixels, MetalReferencePixels, NativeComparisonPixels, StyleComparisonPixels, SweepComparisonPixels, BloomComparisonPixels;
    IConsoleVariable* SceneStyle = nullptr;
    FString SceneStyleBefore;
    EConsoleVariableFlags SceneStyleFlags = ECVF_Default;
    IConsoleVariable* MetalStyle = nullptr;
    FString MetalStyleBefore;
    EConsoleVariableFlags MetalStyleFlags = ECVF_Default;
    TWeakObjectPtr<AStaticMeshActor> Subject;
    TWeakObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
    TWeakObjectPtr<ADirectionalLight> KeyLight;
    TWeakObjectPtr<APostProcessVolume> TestPost;
    FRotator KeyRotation = FRotator::ZeroRotator;
    float KeyIntensity = 0.f, TopDownDistance = 1.f, TopDownFOV = 50.f, TopDownSubjectRadius = 1.f;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterMetallicRenderTest, "DevKitEditor.CharacterMaterial.MetallicRender", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCharacterMetallicRenderTest::RunTest(const FString&)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("Dev01CharacterMetallicRender")))
    {
        AddInfo(TEXT("Requires a disposable real-RHI editor with -Dev01CharacterMetallicRender; not run in the user's open editor."));
        return true;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FCharacterMetallicCapture(this));
    return true;
}
#endif
