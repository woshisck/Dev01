#include "Tools/Workflow/DevKitCloudLightingFinishTrial.h"

#include "Components/DirectionalLightComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "StylizedLightingSettings.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"

namespace
{
    struct FCharacterFinishSnapshot
    {
        int32 ProfileIndex = INDEX_NONE;
        FName ProfileName;
        float SpecularIntensity = 1.f;
        float ReflectionIntensity = 1.f;
        float IndirectOcclusionStrength = 1.f;
    };

    struct FDirectionalFinishSnapshot
    {
        TWeakObjectPtr<UDirectionalLightComponent> Component;
        float ProjectedShadowStrength = 1.f;
    };

    struct FCloudFinishSnapshot
    {
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<UStylizedLightingSettings> Settings;
        float SceneSpecularIntensity = 1.f;
        float SceneSpecularOffset = 0.f;
        TArray<FCharacterFinishSnapshot> Profiles;
        TArray<FDirectionalFinishSnapshot> DirectionalLights;
        FDevKitCloudLightingFinishParameters Parameters;
        FDelegateHandle WorldCleanupHandle;
        FDelegateHandle PreSaveWorldHandle;
        FDelegateHandle PreSaveExternalActorsHandle;
    };

    TUniquePtr<FCloudFinishSnapshot> SessionSnapshot;

    float SafeValue(float Value, float Min, float Max, float Fallback)
    {
        return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Fallback;
    }

    // These profile fields have no authored ClampMax. Retain unusually strong
    // existing profiles instead of flattening all of them to a UI-only maximum.
    float ScaleNonNegative(float Original, float Scale)
    {
        const double Base = FMath::IsFinite(Original) ? FMath::Max(static_cast<double>(Original), 0.0) : 0.0;
        return static_cast<float>(FMath::Min(Base * static_cast<double>(Scale), static_cast<double>(TNumericLimits<float>::Max())));
    }

    FStylizedCharacterLightingProfile* FindOriginalProfile(UStylizedLightingSettings* Settings, const FCharacterFinishSnapshot& Original)
    {
        if (!Settings || !Settings->CharacterLightingProfiles.IsValidIndex(Original.ProfileIndex))
        {
            return nullptr;
        }
        FStylizedCharacterLightingProfile& Profile = Settings->CharacterLightingProfiles[Original.ProfileIndex];
        // Never apply an old snapshot to a different, reordered or renamed look.
        return Profile.ProfileName == Original.ProfileName ? &Profile : nullptr;
    }

    bool OwnsSavingWorld(UWorld* World)
    {
        if (!SessionSnapshot || !World)
        {
            return false;
        }
        if (SessionSnapshot->World.Get() == World)
        {
            return true;
        }
        // A directional component in a streamed level can render in the parent
        // editor world while being serialized as part of its sublevel package.
        for (const FDirectionalFinishSnapshot& Original : SessionSnapshot->DirectionalLights)
        {
            if (const UDirectionalLightComponent* Light = Original.Component.Get())
            {
                if (Light->GetOutermost() == World->GetOutermost())
                {
                    return true;
                }
            }
        }
        return false;
    }

    void RegisterLifecycleDelegates(FCloudFinishSnapshot& Snapshot)
    {
        if (!Snapshot.World.IsValid())
        {
            return;
        }
        Snapshot.WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* World, bool, bool)
        {
            if (SessionSnapshot && SessionSnapshot->World.Get() == World)
            {
                FDevKitCloudLightingFinishTrial::Restore();
            }
        });
        Snapshot.PreSaveWorldHandle = FEditorDelegates::PreSaveWorldWithContext.AddLambda([](UWorld* World, FObjectPreSaveContext)
        {
            if (OwnsSavingWorld(World))
            {
                FDevKitCloudLightingFinishTrial::Restore();
            }
        });
        Snapshot.PreSaveExternalActorsHandle = FEditorDelegates::PreSaveExternalActors.AddLambda([](UWorld* World)
        {
            if (OwnsSavingWorld(World))
            {
                FDevKitCloudLightingFinishTrial::Restore();
            }
        });
    }

    void RemoveLifecycleDelegates(FCloudFinishSnapshot& Snapshot)
    {
        if (Snapshot.WorldCleanupHandle.IsValid())
        {
            FWorldDelegates::OnWorldCleanup.Remove(Snapshot.WorldCleanupHandle);
            Snapshot.WorldCleanupHandle.Reset();
        }
        if (Snapshot.PreSaveWorldHandle.IsValid())
        {
            FEditorDelegates::PreSaveWorldWithContext.Remove(Snapshot.PreSaveWorldHandle);
            Snapshot.PreSaveWorldHandle.Reset();
        }
        if (Snapshot.PreSaveExternalActorsHandle.IsValid())
        {
            FEditorDelegates::PreSaveExternalActors.Remove(Snapshot.PreSaveExternalActorsHandle);
            Snapshot.PreSaveExternalActorsHandle.Reset();
        }
    }

    void Capture(UWorld* World, UStylizedLightingSettings* Settings)
    {
        SessionSnapshot = MakeUnique<FCloudFinishSnapshot>();
        FCloudFinishSnapshot& Snapshot = *SessionSnapshot;
        Snapshot.World = World;
        Snapshot.Settings = Settings;
        Snapshot.SceneSpecularIntensity = Settings->SpecularIntensity;
        Snapshot.SceneSpecularOffset = Settings->SpecularOffset;

        for (int32 Index = 0; Index < Settings->CharacterLightingProfiles.Num(); ++Index)
        {
            const FStylizedCharacterLightingProfile& Profile = Settings->CharacterLightingProfiles[Index];
            FCharacterFinishSnapshot& Original = Snapshot.Profiles.AddDefaulted_GetRef();
            Original.ProfileIndex = Index;
            Original.ProfileName = Profile.ProfileName;
            Original.SpecularIntensity = Profile.SpecularIntensity;
            Original.ReflectionIntensity = Profile.ReflectionIntensity;
            Original.IndirectOcclusionStrength = Profile.IndirectOcclusionStrength;
        }

        if (World)
        {
            // Component iteration also finds directional lights nested in a BP,
            // unlike iteration over ADirectionalLight actors alone.
            for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
            {
                UDirectionalLightComponent* Light = *It;
                if (!IsValid(Light) || Light->IsTemplate() || Light->GetWorld() != World || !Light->CastShadows)
                {
                    continue;
                }
                FDirectionalFinishSnapshot& Original = Snapshot.DirectionalLights.AddDefaulted_GetRef();
                Original.Component = Light;
                Original.ProjectedShadowStrength = Light->StylizedCharacterShadowStrength;
            }
        }
        RegisterLifecycleDelegates(Snapshot);
    }

    void RefreshPreview(UStylizedLightingSettings* Settings)
    {
        if (Settings)
        {
            Settings->ApplyToConsoleVariables();
        }
        if (GEditor)
        {
            GEditor->RedrawLevelEditingViewports();
        }
    }
}

void FDevKitCloudLightingFinishTrial::Apply(UWorld* World, const FDevKitCloudLightingFinishParameters& Parameters)
{
    if (!ensureMsgf(IsInGameThread(), TEXT("Cloud lighting finish preview must run on the editor game thread.")))
    {
        return;
    }

    UStylizedLightingSettings* Settings = GetMutableDefault<UStylizedLightingSettings>();
    if (SessionSnapshot && (SessionSnapshot->World.Get() != World || SessionSnapshot->Settings.Get() != Settings))
    {
        // One shared trial owns one world. Switching worlds releases the previous
        // preview before taking a fresh snapshot of the new context.
        Restore();
    }
    if (!SessionSnapshot)
    {
        Capture(World, Settings);
    }

    FCloudFinishSnapshot& Snapshot = *SessionSnapshot;
    const FDevKitCloudLightingFinishParameters Defaults;
    const float SceneScale = SafeValue(Parameters.SceneSpecularScale, 0.f, 2.f, Defaults.SceneSpecularScale);
    const float SceneOffset = SafeValue(Parameters.SceneSpecularOffsetDelta, -1.f, 1.f, Defaults.SceneSpecularOffsetDelta);
    const float CharacterSpecularScale = SafeValue(Parameters.CharacterSpecularScale, 0.f, 2.f, Defaults.CharacterSpecularScale);
    const float CharacterReflectionScale = SafeValue(Parameters.CharacterReflectionScale, 0.f, 2.f, Defaults.CharacterReflectionScale);
    const float CharacterOcclusionScale = SafeValue(Parameters.CharacterOcclusionScale, 0.f, 1.f, Defaults.CharacterOcclusionScale);
    const float ProjectedShadowScale = SafeValue(Parameters.CharacterProjectedShadowScale, 0.f, 1.f, Defaults.CharacterProjectedShadowScale);
    Snapshot.Parameters.SceneSpecularScale = SceneScale;
    Snapshot.Parameters.SceneSpecularOffsetDelta = SceneOffset;
    Snapshot.Parameters.CharacterSpecularScale = CharacterSpecularScale;
    Snapshot.Parameters.CharacterReflectionScale = CharacterReflectionScale;
    Snapshot.Parameters.CharacterOcclusionScale = CharacterOcclusionScale;
    Snapshot.Parameters.CharacterProjectedShadowScale = ProjectedShadowScale;

    Settings->SpecularIntensity = SafeValue(ScaleNonNegative(Snapshot.SceneSpecularIntensity, SceneScale), 0.f, 4.f, 0.f);
    Settings->SpecularOffset = SafeValue(Snapshot.SceneSpecularOffset + SceneOffset, -1.f, 1.f, 0.f);
    for (const FCharacterFinishSnapshot& Original : Snapshot.Profiles)
    {
        if (FStylizedCharacterLightingProfile* Profile = FindOriginalProfile(Settings, Original))
        {
            Profile->SpecularIntensity = ScaleNonNegative(Original.SpecularIntensity, CharacterSpecularScale);
            Profile->ReflectionIntensity = ScaleNonNegative(Original.ReflectionIntensity, CharacterReflectionScale);
            Profile->IndirectOcclusionStrength = SafeValue(Original.IndirectOcclusionStrength * CharacterOcclusionScale, 0.f, 1.f, 0.f);
        }
    }
    for (const FDirectionalFinishSnapshot& Original : Snapshot.DirectionalLights)
    {
        if (UDirectionalLightComponent* Light = Original.Component.Get())
        {
            Light->StylizedCharacterShadowStrength = SafeValue(Original.ProjectedShadowStrength * ProjectedShadowScale, 0.f, 1.f, 0.f);
            Light->MarkRenderStateDirty();
        }
    }
    RefreshPreview(Settings);
}

void FDevKitCloudLightingFinishTrial::Restore()
{
    if (!ensureMsgf(IsInGameThread(), TEXT("Cloud lighting finish preview must run on the editor game thread.")) || !SessionSnapshot)
    {
        return;
    }

    // Clear shared ownership before refresh callbacks can observe the service.
    TUniquePtr<FCloudFinishSnapshot> Snapshot = MoveTemp(SessionSnapshot);
    RemoveLifecycleDelegates(*Snapshot);
    UStylizedLightingSettings* Settings = Snapshot->Settings.Get();
    if (Settings)
    {
        Settings->SpecularIntensity = Snapshot->SceneSpecularIntensity;
        Settings->SpecularOffset = Snapshot->SceneSpecularOffset;
        for (const FCharacterFinishSnapshot& Original : Snapshot->Profiles)
        {
            if (FStylizedCharacterLightingProfile* Profile = FindOriginalProfile(Settings, Original))
            {
                Profile->SpecularIntensity = Original.SpecularIntensity;
                Profile->ReflectionIntensity = Original.ReflectionIntensity;
                Profile->IndirectOcclusionStrength = Original.IndirectOcclusionStrength;
            }
        }
    }
    for (const FDirectionalFinishSnapshot& Original : Snapshot->DirectionalLights)
    {
        if (UDirectionalLightComponent* Light = Original.Component.Get())
        {
            Light->StylizedCharacterShadowStrength = Original.ProjectedShadowStrength;
            Light->MarkRenderStateDirty();
        }
    }
    RefreshPreview(Settings);
}

bool FDevKitCloudLightingFinishTrial::IsActive()
{
    return SessionSnapshot.IsValid();
}

FDevKitCloudLightingFinishParameters FDevKitCloudLightingFinishTrial::GetParameters()
{
    return SessionSnapshot ? SessionSnapshot->Parameters : FDevKitCloudLightingFinishParameters();
}
