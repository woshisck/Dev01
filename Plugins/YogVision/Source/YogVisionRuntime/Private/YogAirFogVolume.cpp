#include "YogAirFogVolume.h"
#include "YogAirFogRenderBridge.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
float AirFogSafeFloat(float Value, float Default, float Min, float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Default, Min, Max);
}

FLinearColor AirFogSafeColor(const FLinearColor& Value, const FLinearColor& Default, float Maximum)
{
    return FLinearColor(AirFogSafeFloat(Value.R, Default.R, 0.f, Maximum),
        AirFogSafeFloat(Value.G, Default.G, 0.f, Maximum), AirFogSafeFloat(Value.B, Default.B, 0.f, Maximum), 1.f);
}

void AirFogRestoreVolumetricShadow(ULocalLightComponent* Light, bool bValue)
{
    Light->SetCastVolumetricShadow(bValue);
    // A user may have changed Mobility to Static after our acquisition. Public
    // setters reject that component now; restore only the field we still own.
    if (Light->bCastVolumetricShadow != bValue)
    {
        Light->bCastVolumetricShadow = bValue;
        Light->MarkRenderStateDirty();
    }
}

void AirFogRestoreScattering(ULocalLightComponent* Light, float Value)
{
    Light->SetVolumetricScatteringIntensity(Value);
    if (Light->VolumetricScatteringIntensity != Value)
    {
        Light->VolumetricScatteringIntensity = Value;
        Light->MarkRenderStateDirty();
    }
}

struct FAirFogConsoleValue
{
    FString Name;
    IConsoleVariable* Variable = nullptr;
    FString OriginalValue;
    EConsoleVariableFlags OriginalPriority = ECVF_SetByConstructor;
    FString AppliedValue;
    EConsoleVariableFlags AppliedPriority = ECVF_SetByTemp;
    bool bUsesConsoleSnapshot = false;
};

IConsoleVariable* AirFogFindConsoleVariable(const TCHAR* Name)
{
    // These six renderer/engine CVars live for the process lifetime. Slate polls status
    // every frame; repeated FindConsoleVariable calls trigger the engine's lookup warning.
    static TMap<FName, IConsoleVariable*> Cache;
    const FName Key(Name);
    if (IConsoleVariable** Found = Cache.Find(Key)) return *Found;
    IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
    Cache.Add(Key, Variable);
    return Variable;
}

// CVars are process-wide, not World properties. Shared leases prevent one World's teardown
// from undoing another World's active preview. Existing console history is never cleared.
struct FAirFogConsoleLease
{
    TSet<TWeakObjectPtr<AYogAirFogVolume>> Owners;
    TArray<FAirFogConsoleValue> Values;
    const FName Tag = TEXT("YogAirFog.NativeVolumeLease");

    void Restore()
    {
        for (const FAirFogConsoleValue& State : Values)
        {
            IConsoleVariable* Variable = State.Variable;
            if (!Variable) continue;
            const bool bStillOurs = Variable->GetString() == State.AppliedValue
                && (Variable->GetFlags() & ECVF_SetByMask) == State.AppliedPriority;
            if (State.bUsesConsoleSnapshot)
            {
                // Console is NOT an array priority in UE5.8. Restore its snapshot only when
                // neither the current value nor SetBy has changed outside this lease.
                if (bStillOurs) Variable->Set(*State.OriginalValue, State.OriginalPriority);
            }
            else Variable->Unset(ECVF_SetByTemp, Tag);
            // Temp is an array priority: remove only our tag, even if a newer user console
            // value currently covers it. This keeps the user's value and lower history,
            // without leaving a dormant preview overlay or SetByCode pollution.
        }
        Values.Reset();
        IConsoleManager::Get().CallAllConsoleVariableSinks();
    }

    void Prune()
    {
        for (auto It = Owners.CreateIterator(); It; ++It) if (!It->IsValid()) It.RemoveCurrent();
        if (Owners.IsEmpty() && !Values.IsEmpty()) Restore();
    }

    bool SetValue(const TCHAR* Name, const FString& Desired, FString& Error)
    {
        IConsoleVariable* Variable = AirFogFindConsoleVariable(Name);
        if (!Variable)
        {
            Error = FString::Printf(TEXT("当前引擎缺少空气雾设置：%s。"), Name);
            return false;
        }
        FAirFogConsoleValue State;
        State.Name = Name;
        State.Variable = Variable;
        State.OriginalValue = Variable->GetString();
        State.OriginalPriority = static_cast<EConsoleVariableFlags>(Variable->GetFlags() & ECVF_SetByMask);
        State.bUsesConsoleSnapshot = State.OriginalPriority >= ECVF_SetByConsole;
        if (State.bUsesConsoleSnapshot) Variable->Set(*Desired, ECVF_SetByConsole);
        else Variable->Set(*Desired, ECVF_SetByTemp, Tag);
        State.AppliedPriority = static_cast<EConsoleVariableFlags>(Variable->GetFlags() & ECVF_SetByMask);
        State.AppliedValue = Variable->GetString();
        Values.Add(MoveTemp(State));
        return true;
    }

    bool Acquire(AYogAirFogVolume* Owner, const FYogAirFogParameters& Parameters, FString& Error)
    {
        Prune();
        if (Owners.Contains(Owner)) return true;
        if (Owners.IsEmpty())
        {
            if (!SetValue(TEXT("r.Fog"), TEXT("1"), Error)
                || !SetValue(TEXT("r.VolumetricFog"), TEXT("1"), Error)
                || !SetValue(TEXT("r.VolumetricFog.GridPixelSize"), FString::FromInt(Parameters.GridPixelSize), Error)
                || !SetValue(TEXT("r.VolumetricFog.GridSizeZ"), FString::FromInt(Parameters.GridSizeZ), Error)
                || !SetValue(TEXT("r.VolumetricFog.HistoryWeight"), FString::SanitizeFloat(Parameters.HistoryWeight), Error)
                || !SetValue(TEXT("r.VolumetricFog.LightSoftFading"), FString::SanitizeFloat(Parameters.LightSoftFading), Error))
            {
                Restore();
                return false;
            }
            IConsoleManager::Get().CallAllConsoleVariableSinks();
        }
        IConsoleVariable* Fog = AirFogFindConsoleVariable(TEXT("r.Fog"));
        IConsoleVariable* VolumeFog = AirFogFindConsoleVariable(TEXT("r.VolumetricFog"));
        if (!Fog || !VolumeFog || Fog->GetInt() != 1 || VolumeFog->GetInt() != 1)
        {
            Error = TEXT("雾开关被外部设置覆盖，未接管场景。请检查当前控制台设置。");
            if (Owners.IsEmpty()) Restore();
            return false;
        }
        Owners.Add(Owner);
        return true;
    }

    void Update(AYogAirFogVolume* Owner, const FYogAirFogParameters& Parameters)
    {
        Prune();
        if (Owners.Num() != 1 || !Owners.Contains(Owner)) return;
        for (FAirFogConsoleValue& State : Values)
        {
            FString Desired;
            if (State.Name == TEXT("r.VolumetricFog.GridPixelSize")) Desired = FString::FromInt(Parameters.GridPixelSize);
            else if (State.Name == TEXT("r.VolumetricFog.GridSizeZ")) Desired = FString::FromInt(Parameters.GridSizeZ);
            else if (State.Name == TEXT("r.VolumetricFog.HistoryWeight")) Desired = FString::SanitizeFloat(Parameters.HistoryWeight);
            else if (State.Name == TEXT("r.VolumetricFog.LightSoftFading")) Desired = FString::SanitizeFloat(Parameters.LightSoftFading);
            else continue;
            IConsoleVariable* Variable = State.Variable;
            if (!Variable || Variable->GetString() != State.AppliedValue
                || (Variable->GetFlags() & ECVF_SetByMask) != State.AppliedPriority) continue;
            if (State.bUsesConsoleSnapshot) Variable->Set(*Desired, ECVF_SetByConsole);
            else Variable->Set(*Desired, ECVF_SetByTemp, Tag);
            State.AppliedPriority = static_cast<EConsoleVariableFlags>(Variable->GetFlags() & ECVF_SetByMask);
            State.AppliedValue = Variable->GetString();
        }
        IConsoleManager::Get().CallAllConsoleVariableSinks();
    }

    void Release(AYogAirFogVolume* Owner)
    {
        Owners.Remove(Owner);
        Prune();
    }

    bool HasExternalOverride() const
    {
        for (const FAirFogConsoleValue& State : Values)
        {
            IConsoleVariable* Variable = State.Variable;
            if (!Variable || Variable->GetString() != State.AppliedValue
                || (Variable->GetFlags() & ECVF_SetByMask) != State.AppliedPriority) return true;
        }
        return false;
    }
};

FAirFogConsoleLease& AirFogConsoleLease()
{
    static FAirFogConsoleLease Lease;
    return Lease;
}
}

void FYogAirFogParameters::Sanitize()
{
    const FYogAirFogParameters Default;
    if (FogBankCenter.ContainsNaN()) FogBankCenter = Default.FogBankCenter;
    if (FogBankExtent.ContainsNaN()) FogBankExtent = Default.FogBankExtent;
    FogBankExtent = FogBankExtent.GetAbs().ComponentMax(FVector(50.f)).ComponentMin(FVector(100000.f));
    FogBankStrength = AirFogSafeFloat(FogBankStrength, Default.FogBankStrength, 0.f, 1.f);
    WorldVisionRadius = AirFogSafeFloat(WorldVisionRadius, Default.WorldVisionRadius, 0.f, 20000.f);
    WorldVisionFeather = AirFogSafeFloat(WorldVisionFeather, Default.WorldVisionFeather, 1.f, 20000.f);
    VisionRadius = AirFogSafeFloat(VisionRadius, Default.VisionRadius, .02f, 2.f);
    VisionFeather = AirFogSafeFloat(VisionFeather, Default.VisionFeather, .01f, 1.f);
    LightClearStrength = AirFogSafeFloat(LightClearStrength, Default.LightClearStrength, 0.f, 1.f);
    LightBoundaryStrength = AirFogSafeFloat(LightBoundaryStrength, Default.LightBoundaryStrength, 0.f, 2.f);
    OuterVolumeStrength = AirFogSafeFloat(OuterVolumeStrength, Default.OuterVolumeStrength, 0.f, .25f);
    FlowMapScale = AirFogSafeFloat(FlowMapScale, Default.FlowMapScale, .1f, 8.f);
    if (StrokeTexture.IsNull()) StrokeTexture = Default.StrokeTexture;
    if (FlowTexture.IsNull()) FlowTexture = Default.FlowTexture;
    Density = AirFogSafeFloat(Density, Default.Density, 0.f, 5.f);
    NearDensity = AirFogSafeFloat(NearDensity, Default.NearDensity, 0.f, Density);
    ClearMinDensity = AirFogSafeFloat(ClearMinDensity, Default.ClearMinDensity, 0.f, NearDensity);
    NearOffset = AirFogSafeFloat(NearOffset, Default.NearOffset, 0.f, 50000.f);
    DepthRamp = AirFogSafeFloat(DepthRamp, Default.DepthRamp, 1.f, 50000.f);
    DepthPower = AirFogSafeFloat(DepthPower, Default.DepthPower, .1f, 8.f);
    ClearRadius = AirFogSafeFloat(ClearRadius, Default.ClearRadius, 0.f, 20000.f);
    ClearFeather = AirFogSafeFloat(ClearFeather, Default.ClearFeather, 1.f, 20000.f);
    ClearHeight = AirFogSafeFloat(ClearHeight, Default.ClearHeight, 10.f, 50000.f);
    CameraHeightOffset = AirFogSafeFloat(CameraHeightOffset, Default.CameraHeightOffset, -20000.f, 50000.f);
    HeightFalloff = AirFogSafeFloat(HeightFalloff, Default.HeightFalloff, 10.f, 50000.f);
    NoiseAmount = AirFogSafeFloat(NoiseAmount, Default.NoiseAmount, 0.f, 1.f);
    NoiseScale = AirFogSafeFloat(NoiseScale, Default.NoiseScale, .00001f, 1.f);
    WindSpeed = AirFogSafeFloat(WindSpeed, Default.WindSpeed, -2000.f, 2000.f);
    BoundaryNoise = AirFogSafeFloat(BoundaryNoise, Default.BoundaryNoise, 0.f, 5000.f);
    AmbientIntensity = AirFogSafeFloat(AmbientIntensity, Default.AmbientIntensity, 0.f, 5.f);
    PaletteDistance = AirFogSafeFloat(PaletteDistance, Default.PaletteDistance, 1.f, 100000.f);
    PaletteStrength = AirFogSafeFloat(PaletteStrength, Default.PaletteStrength, 0.f, 5.f);
    FarStart = AirFogSafeFloat(FarStart, Default.FarStart, 0.f, 50000.f);
    FarEnd = FMath::Max(FarStart + 1.f, AirFogSafeFloat(FarEnd, Default.FarEnd, 1.f, 100000.f));
    FarFlatten = AirFogSafeFloat(FarFlatten, Default.FarFlatten, 0.f, 1.f);
    StructureStrength = AirFogSafeFloat(StructureStrength, Default.StructureStrength, 0.f, 2.f);
    DetailStrength = AirFogSafeFloat(DetailStrength, Default.DetailStrength, 0.f, 1.f);
    EdgeWidth = AirFogSafeFloat(EdgeWidth, Default.EdgeWidth, .25f, 8.f);
    FormLightYaw = AirFogSafeFloat(FormLightYaw, Default.FormLightYaw, -180.f, 180.f);
    FormLightPitch = AirFogSafeFloat(FormLightPitch, Default.FormLightPitch, -89.f, 89.f);
    FormLightStrength = AirFogSafeFloat(FormLightStrength, Default.FormLightStrength, 0.f, 2.f);
    FormShadowStrength = AirFogSafeFloat(FormShadowStrength, Default.FormShadowStrength, 0.f, 1.f);
    FormLightColor = AirFogSafeColor(FormLightColor, Default.FormLightColor, 10.f);
    StrokeStrength = AirFogSafeFloat(StrokeStrength, Default.StrokeStrength, 0.f, 1.f);
    StrokeScale = AirFogSafeFloat(StrokeScale, Default.StrokeScale, .25f, 16.f);
    StrokeFlowSpeed = AirFogSafeFloat(StrokeFlowSpeed, Default.StrokeFlowSpeed, -5.f, 5.f);
    StrokeWarp = AirFogSafeFloat(StrokeWarp, Default.StrokeWarp, 0.f, 1.f);
    BoundaryDetail = AirFogSafeFloat(BoundaryDetail, Default.BoundaryDetail, 0.f, 1.f);
    BoundaryDetailScale = AirFogSafeFloat(BoundaryDetailScale, Default.BoundaryDetailScale, .0001f, 1.f);
    VolumeFormStrength = AirFogSafeFloat(VolumeFormStrength, Default.VolumeFormStrength, 0.f, 2.f);
    FogAlbedo = AirFogSafeColor(FogAlbedo, Default.FogAlbedo, 1.f);
    AmbientColor = AirFogSafeColor(AmbientColor, Default.AmbientColor, 10.f);
    FarColor = AirFogSafeColor(FarColor, Default.FarColor, 10.f);
    EdgeColor = AirFogSafeColor(EdgeColor, Default.EdgeColor, 2.f);
    ViewDistance = AirFogSafeFloat(ViewDistance, Default.ViewDistance, 5000.f, 100000.f);
    GridPixelSize = FMath::Clamp(GridPixelSize, 4, 64);
    GridSizeZ = FMath::Clamp(GridSizeZ, 16, 256);
    HistoryWeight = AirFogSafeFloat(HistoryWeight, Default.HistoryWeight, 0.f, .99f);
    LightSoftFading = AirFogSafeFloat(LightSoftFading, Default.LightSoftFading, 0.f, 4.f);
}

AYogAirFogVolume::AYogAirFogVolume()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Root->SetMobility(EComponentMobility::Movable);
    VolumeBounds = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VolumeBounds"));
    VolumeBounds->SetupAttachment(Root);
    VolumeBounds->SetMobility(EComponentMobility::Movable);
    VolumeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VolumeBounds->SetGenerateOverlapEvents(false);
    VolumeBounds->SetCanEverAffectNavigation(false);
    VolumeBounds->SetCastShadow(false);
    VolumeBounds->SetVisibility(false, false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) VolumeBounds->SetStaticMesh(Cube.Object);
    VolumeBounds->SetRelativeScale3D(VolumeSize / 100.f);

    Carrier = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Carrier"));
    Carrier->SetupAttachment(Root);
    Carrier->SetMobility(EComponentMobility::Movable);
    Carrier->FogDensity = .001f;
    Carrier->SecondFogData.FogDensity = 0.f;
    Carrier->FogMaxOpacity = .001f;
    Carrier->bEnableVolumetricFog = true;
    Carrier->VolumetricFogExtinctionScale = 0.f;
    Carrier->VolumetricFogEmissive = FLinearColor::Black;
    Carrier->VolumetricFogDistance = 20000.f;
    Carrier->SetVisibility(false, false);
}

void AYogAirFogVolume::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoActivate && GetNetMode() != NM_DedicatedServer) SetAirFogEnabled(true);
}

void AYogAirFogVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bDesiredEnabled = false;
    DeactivateAirFog();
    Super::EndPlay(EndPlayReason);
}

void AYogAirFogVolume::Destroyed()
{
    bDesiredEnabled = false;
    DeactivateAirFog();
    Super::Destroyed();
}

void AYogAirFogVolume::BeginDestroy()
{
    bDesiredEnabled = false;
    DeactivateAirFog();
    Super::BeginDestroy();
}

bool AYogAirFogVolume::SetAirFogEnabled(bool bEnabled)
{
    bDesiredEnabled = bEnabled;
    RetryAccumulator = 0.f;
    if (!bEnabled)
    {
        DeactivateAirFog();
        LocalLightOptOuts.Reset();
        LastError.Reset();
        return true;
    }
    if (SaveSuspensionDepth > 0) return true;
    return bActive || ActivateAirFog();
}

bool AYogAirFogVolume::PrepareResources()
{
    if (!VolumeBounds || !VolumeBounds->GetStaticMesh())
    {
        LastError = TEXT("缺少原生体积覆盖Cube，场景与控制台未被接管。");
        return false;
    }
    UMaterialInterface* Density = LoadObject<UMaterialInterface>(nullptr, TEXT("/YogVision/AirFog/M_AirFogDensity.M_AirFogDensity"));
    UMaterialInterface* Finish = bFinishEnabled ? LoadObject<UMaterialInterface>(nullptr, TEXT("/YogVision/AirFog/M_AirFogFinish.M_AirFogFinish")) : nullptr;
    if (!Density || !Density->GetMaterial() || Density->GetMaterial()->MaterialDomain != MD_Volume)
    {
        LastError = TEXT("空气密度材质尚未准备好，需要M_AirFogDensity体积材质；现有场景保持原样。");
        return false;
    }
    if (!Density->GetMaterial()->GetShadingModels().HasShadingModel(MSM_DefaultLit))
    {
        LastError = TEXT("空气密度材质缺少原生消光支持，请重新准备空气雾资源。");
        return false;
    }
    if (bFinishEnabled && (!Finish || !Finish->GetMaterial() || Finish->GetMaterial()->MaterialDomain != MD_PostProcess))
    {
        LastError = TEXT("远景结构材质尚未准备好，需要M_AirFogFinish后处理材质；现有场景保持原样。");
        return false;
    }
    if (!DensityMID || DensityMID->GetOuter() != this || DensityMID->Parent != Density)
    {
        DensityMID = UMaterialInstanceDynamic::Create(Density, this);
        if (DensityMID) DensityMID->SetFlags(RF_Transient);
    }
    if (bFinishEnabled && (!FinishMID || FinishMID->GetOuter() != this || FinishMID->Parent != Finish))
    {
        FinishMID = UMaterialInstanceDynamic::Create(Finish, this);
        if (FinishMID) FinishMID->SetFlags(RF_Transient);
    }
    if (!DensityMID || (bFinishEnabled && !FinishMID))
    {
        LastError = TEXT("无法创建空气雾临时材质，场景保持原样。");
        return false;
    }
    // Do not expose the default opaque Cube before a valid Volume-domain MID exists.
    VolumeBounds->SetMaterial(0, DensityMID);
    UpdateMaterialParameters();
    return true;
}

bool AYogAirFogVolume::ActivateAirFog()
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown || GetNetMode() == NM_DedicatedServer || SaveSuspensionDepth > 0) return false;
    if (World->WorldType != EWorldType::Editor && !World->IsGameWorld()) return false;
    for (TActorIterator<AYogAirFogVolume> It(World); It; ++It)
    {
        if (*It != this && It->bActive)
        {
            LastError = TEXT("当前世界已有另一个空气雾正在启用，请先关闭它。");
            return false;
        }
    }
    Parameters.Sanitize();
    if (!PrepareResources()) return false;

    if (!AirFogConsoleLease().Acquire(this, Parameters, LastError))
    {
        DeactivateAirFog();
        return false;
    }
    bHasConsoleLease = true;
    SuppressOtherHeightFog();
    UpdateNativeParameters();
    SetActorLocation(ResolveCameraAnchor());
    Carrier->SetVisibility(true, false);
    VolumeBounds->SetVisibility(true, false);
    bActive = true;
    ManageLocalLightScattering();
    UpdateFinishRegistration();
    LastError.Reset();
    return true;
}

void AYogAirFogVolume::DeactivateAirFog()
{
    YogAirFogRenderBridge::UnregisterVolume(this);
    bFinishRegistered = false;
    bActive = false;
    if (VolumeBounds) VolumeBounds->SetVisibility(false, false);
    if (Carrier) Carrier->SetVisibility(false, false);
    RestoreLocalLightScattering();
    RestoreOtherHeightFog();
    if (bHasConsoleLease)
    {
        AirFogConsoleLease().Release(this);
        bHasConsoleLease = false;
    }
}

void AYogAirFogVolume::ApplyParameters(FYogAirFogParameters NewParameters)
{
    NewParameters.Sanitize();
    Parameters = NewParameters;
    UpdateNativeParameters();
    UpdateMaterialParameters();
    if (bActive)
    {
        AirFogConsoleLease().Update(this, Parameters);
        UpdateFinishRegistration();
    }
}

UMaterialInstanceDynamic* AYogAirFogVolume::GetFinishMID() const
{
    return bActive && bFinishEnabled && IsValid(FinishMID) ? FinishMID.Get() : nullptr;
}

void AYogAirFogVolume::UpdateFinishRegistration()
{
    if (!bActive || !bDesiredEnabled || SaveSuspensionDepth > 0 || !bFinishEnabled)
    {
        if (bFinishRegistered) YogAirFogRenderBridge::UnregisterVolume(this);
        bFinishRegistered = false;
        return;
    }
    // Enabling the finish after activating density-only needs its template once.
    // It does not restart the carrier, light scopes, or the global quality lease.
    if (!IsValid(FinishMID) && !PrepareResources()) return;
    if (!bFinishRegistered)
    {
        YogAirFogRenderBridge::RegisterVolume(this);
        bFinishRegistered = true;
    }
}

void AYogAirFogVolume::SetFocusActor(AActor* Actor)
{
    FocusActor = IsValid(Actor) && Actor->GetWorld() == GetWorld() ? Actor : nullptr;
    bHasExplicitFocusPosition = false;
    UpdateMaterialParameters();
}

void AYogAirFogVolume::SetFocusPosition(FVector Position)
{
    FocusActor.Reset();
    FocusPosition = Position.ContainsNaN() ? GetActorLocation() : Position;
    bHasExplicitFocusPosition = true;
    UpdateMaterialParameters();
}

void AYogAirFogVolume::SetPaletteMaterial(UMaterialInterface* Material)
{
    PaletteMaterial = IsValid(Material) ? Material : nullptr;
    bTriedDefaultPalette = false;
    UpdateMaterialParameters();
}

FVector AYogAirFogVolume::ResolveCameraAnchor() const
{
    const UWorld* World = GetWorld();
#if WITH_EDITOR
    if (World && World->WorldType == EWorldType::Editor && EditorCameraAnchor.IsSet())
        return EditorCameraAnchor.GetValue();
#endif
    if (World && World->IsGameWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            const APlayerController* PC = It->Get();
            // Before the first camera update GetPlayerViewPoint falls back to the
            // ViewTarget's actor origin, which can be a Pawn rather than a camera.
            if (!PC || !PC->IsLocalController() || !IsValid(PC->PlayerCameraManager)
                || PC->PlayerCameraManager->GetCameraCacheTime() <= 0.f) continue;
            FVector Location;
            FRotator Rotation;
            PC->GetPlayerViewPoint(Location, Rotation);
            if (!Location.ContainsNaN()) return Location;
        }
    }
    if (World)
    {
        for (const FVector& Location : World->ViewLocationsRenderedLastFrame)
            if (!Location.ContainsNaN()) return Location;
    }
    // No rendered view yet: retain the last bounds position until a camera exists.
    // Legacy FocusActor / FocusPosition are deliberately not consulted.
    return GetActorLocation();
}

void AYogAirFogVolume::ResolveArtistPalette(FLinearColor& NearColor, FLinearColor& FarColor, float& FogMix)
{
    const FLinearColor DefaultNear(.112495f, .678705f, .744792f, 1.f);
    const FLinearColor DefaultFar(.1106f, .1505f, .35f, 1.f);
    NearColor = DefaultNear;
    FarColor = DefaultFar;
    FogMix = .07f;
    if (!IsValid(PaletteMaterial) && !bTriedDefaultPalette)
    {
        bTriedDefaultPalette = true;
        PaletteMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/YogVision/AirFog/MI_AirFogArtistCompat.MI_AirFogArtistCompat"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    FLinearColor ReadNear;
    FLinearColor ReadFar;
    float ReadMix = 0.f;
    const bool bNearValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), ReadNear)
        && FMath::IsFinite(ReadNear.R) && FMath::IsFinite(ReadNear.G) && FMath::IsFinite(ReadNear.B);
    const bool bFarValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), ReadFar)
        && FMath::IsFinite(ReadFar.R) && FMath::IsFinite(ReadFar.G) && FMath::IsFinite(ReadFar.B);
    const bool bMixValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), ReadMix)
        && FMath::IsFinite(ReadMix);
    if (bNearValid) NearColor = AirFogSafeColor(ReadNear, DefaultNear, 10.f);
    if (bFarValid) FarColor = AirFogSafeColor(ReadFar, DefaultFar, 10.f);
    if (bMixValid) FogMix = FMath::Clamp(ReadMix, 0.f, 1.f);
    bPaletteFallback = !bNearValid || !bFarValid || !bMixValid;
}

void AYogAirFogVolume::ResolveDirectionalFormLight(float& Yaw, float& Pitch, FLinearColor& Color)
{
    Yaw = Parameters.FormLightYaw;
    Pitch = Parameters.FormLightPitch;
    Color = Parameters.FormLightColor;
    FormLightStatus = Parameters.bUseSceneDirectionalLight
        ? TEXT("自动方向光：未找到有效场景灯，使用手动方向与颜色。")
        : TEXT("手动方向光：使用美术水平角、俯仰角与颜色。");
    UWorld* World = GetWorld();
    if (!Parameters.bUseSceneDirectionalLight || !World) return;

    UDirectionalLightComponent* Selected = nullptr;
    int32 SelectedSunIndex = MAX_int32;
    double SelectedBrightness = -1.;
    FString SelectedPath;
    FLinearColor SelectedColor = FLinearColor::Black;
    float SelectedIntensity = 0.f;
    for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
    {
        UDirectionalLightComponent* Light = *It;
        AActor* LightOwner = Light->GetOwner();
        if (!IsValid(Light) || Light->IsTemplate() || Light->GetWorld() != World || !Light->IsRegistered()
            || !Light->IsVisible() || !Light->bAffectsWorld || !IsValid(LightOwner) || LightOwner->IsHidden()) continue;
#if WITH_EDITOR
        if (!World->IsGameWorld() && LightOwner->IsHiddenEd()) continue;
#endif
        const float Intensity = Light->ComputeLightBrightness();
        if (!FMath::IsFinite(Intensity) || Intensity <= 0.f) continue;
        FLinearColor SourceColor = Light->GetLightColor();
        if (Light->bUseTemperature)
        {
            if (!FMath::IsFinite(Light->Temperature)) continue;
            SourceColor *= Light->GetColorTemperature();
        }
        if (!FMath::IsFinite(SourceColor.R) || !FMath::IsFinite(SourceColor.G) || !FMath::IsFinite(SourceColor.B)) continue;
        SourceColor = AirFogSafeColor(SourceColor, FLinearColor::Black, 10.f);
        const double Brightness = static_cast<double>(Intensity) * SourceColor.GetLuminance();
        if (Brightness <= 0. || !FMath::IsFinite(Brightness) || Light->GetForwardVector().ContainsNaN()) continue;
        const int32 SunIndex = Light->bAtmosphereSunLight ? FMath::Max(0, Light->AtmosphereSunLightIndex) : MAX_int32;
        const FString Path = Light->GetPathName();
        if (!Selected || SunIndex < SelectedSunIndex
            || (SunIndex == SelectedSunIndex && (Brightness > SelectedBrightness
                || (Brightness == SelectedBrightness && Path.Compare(SelectedPath, ESearchCase::CaseSensitive) < 0))))
        {
            Selected = Light;
            SelectedSunIndex = SunIndex;
            SelectedBrightness = Brightness;
            SelectedPath = Path;
            SelectedColor = SourceColor;
            SelectedIntensity = Intensity;
        }
    }
    if (!Selected) return;

    // The shader's key vector points from the shaded point toward the light.
    const FRotator TowardLight = (-Selected->GetForwardVector()).Rotation();
    Yaw = TowardLight.Yaw;
    Pitch = TowardLight.Pitch;
    // Directional intensity is illuminance, not a direct emissive multiplier.
    // Bound it smoothly, preserving source tint and artist light/shadow strengths.
    const float BoundedIntensity = static_cast<float>(static_cast<double>(SelectedIntensity)
        / (static_cast<double>(SelectedIntensity) + 10.));
    const float MaxChannel = FMath::Max3(SelectedColor.R, SelectedColor.G, SelectedColor.B);
    Color = SelectedColor * (BoundedIntensity / FMath::Max(1.f, MaxChannel));
    Color.A = 1.f;
    const FString Name = Selected->GetOwner()->GetActorNameOrLabel();
    FormLightStatus = Selected->bAtmosphereSunLight
        ? FString::Printf(TEXT("自动方向光：%s（大气太阳索引%d）。"), *Name, SelectedSunIndex)
        : FString::Printf(TEXT("自动方向光：%s。"), *Name);
}

FString AYogAirFogVolume::GetFormLightStatusText() const
{
    if (!FormLightStatus.IsEmpty()) return FormLightStatus;
    return Parameters.bUseSceneDirectionalLight
        ? TEXT("自动方向光：等待解析当前场景灯。") : TEXT("手动方向光：使用美术方向与颜色。");
}

void AYogAirFogVolume::Tick(float DeltaSeconds)
{
#if WITH_EDITOR
    LastUpdateFrame = GFrameCounter;
#endif
    Super::Tick(DeltaSeconds);
    if (!bDesiredEnabled || SaveSuspensionDepth > 0 || !GetWorld() || GetWorld()->bIsTearingDown) return;
    if (!bActive)
    {
        RetryAccumulator += DeltaSeconds;
        if (RetryAccumulator >= 1.f) { RetryAccumulator = 0.f; ActivateAirFog(); }
        return;
    }
    UpdateFinishRegistration();
    if (VolumeBounds && DensityMID && VolumeBounds->GetMaterial(0) != DensityMID) VolumeBounds->SetMaterial(0, DensityMID);
    const FVector Focus = ResolveCameraAnchor();
    if (!Focus.ContainsNaN() && !GetActorLocation().Equals(Focus, .1f)) SetActorLocation(Focus);
    UpdateMaterialParameters();
    OtherFogScanAccumulator += DeltaSeconds;
    if (OtherFogScanAccumulator >= 1.f)
    {
        OtherFogScanAccumulator = 0.f;
        SuppressOtherHeightFog();
        ManageLocalLightScattering();
    }
    else if (!bManageLocalLightScattering && !LocalLightStates.IsEmpty()) RestoreLocalLightScattering();
}

void AYogAirFogVolume::UpdateNativeParameters()
{
    Parameters.Sanitize();
    if (Carrier)
    {
        Carrier->SetFogDensity(.001f);
        Carrier->SetSecondFogDensity(0.f);
        Carrier->SetFogMaxOpacity(.001f);
        Carrier->SetVolumetricFog(true);
        Carrier->SetVolumetricFogExtinctionScale(0.f);
        Carrier->SetVolumetricFogEmissive(FLinearColor::Black);
        Carrier->SetVolumetricFogDistance(Parameters.ViewDistance);
        Carrier->SetVolumetricFogStartDistance(0.f);
    }
    if (VolumeBounds)
    {
        const FVector SafeSize(AirFogSafeFloat(VolumeSize.X, 60000.f, 1000.f, 1000000.f),
            AirFogSafeFloat(VolumeSize.Y, 60000.f, 1000.f, 1000000.f), AirFogSafeFloat(VolumeSize.Z, 24000.f, 1000.f, 1000000.f));
        VolumeBounds->SetRelativeScale3D(SafeSize / 100.f);
    }
}

void AYogAirFogVolume::UpdateMaterialParameters()
{
    const FVector Focus = ResolveCameraAnchor();
    float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
#if WITH_EDITOR
    if (GetWorld() && !GetWorld()->IsGameWorld())
    {
        // Editor worlds can leave their gameplay clock paused between edits.
        // Keep slow paint advection alive independently of that gameplay clock.
        static const double EditorClockOrigin = FPlatformTime::Seconds();
        WorldTime = static_cast<float>(FPlatformTime::Seconds() - EditorClockOrigin);
    }
#endif
    FLinearColor ArtistNearColor;
    FLinearColor ArtistFarColor;
    float ArtistFogMix = 0.f;
    ResolveArtistPalette(ArtistNearColor, ArtistFarColor, ArtistFogMix);
    float ResolvedYaw = Parameters.FormLightYaw;
    float ResolvedPitch = Parameters.FormLightPitch;
    FLinearColor ResolvedColor = Parameters.FormLightColor;
    ResolveDirectionalFormLight(ResolvedYaw, ResolvedPitch, ResolvedColor);
    // Editor selection never supplies a vision centre. In game use the explicit
    // observer, or the first local possessed pawn; no pawn means screen centre.
    ScreenVisionSource = FLinearColor::Transparent;
    if (GetWorld() && GetWorld()->IsGameWorld())
    {
        AActor* Observer = FocusActor.Get();
        if (!Observer)
            for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
                if (APlayerController* PC = It->Get(); PC && PC->IsLocalController() && PC->GetPawn())
                { Observer = PC->GetPawn(); break; }
        const FVector Location = Observer ? Observer->GetActorLocation() : FocusPosition;
        if ((Observer || bHasExplicitFocusPosition) && !Location.ContainsNaN())
            ScreenVisionSource = FLinearColor(Location.X, Location.Y, Location.Z, 1.f);
    }
    struct FScreenLight
    {
        ULocalLightComponent* Light = nullptr;
        double Score = 0;
        FString Path;
    };
    TArray<FScreenLight> Lights;
    for (TObjectIterator<ULocalLightComponent> It; It; ++It)
    {
        ULocalLightComponent* Light = *It;
        AActor* LocalLightOwner = Light->GetOwner();
        if (Light->IsTemplate() || Light->GetWorld() != GetWorld() || !Light->IsRegistered()
            || !Light->IsVisible() || !Light->bAffectsWorld || !LocalLightOwner || LocalLightOwner->IsHidden()
            || !FMath::IsFinite(Light->AttenuationRadius) || Light->AttenuationRadius <= 0
            || !FMath::IsFinite(Light->ComputeLightBrightness()) || Light->ComputeLightBrightness() <= 0
            || !FMath::IsFinite(Light->VolumetricScatteringIntensity) || Light->VolumetricScatteringIntensity <= 0
            || Light->GetComponentLocation().ContainsNaN() || Light->GetForwardVector().ContainsNaN()) continue;
#if WITH_EDITOR
        if (!GetWorld()->IsGameWorld() && LocalLightOwner->IsHiddenEd()) continue;
#endif
        const double Distance = FVector::Distance(Focus, Light->GetComponentLocation());
        Lights.Add({Light, Distance / FMath::Max(1.f, Light->AttenuationRadius), Light->GetPathName()});
    }
    Lights.Sort([](const FScreenLight& A, const FScreenLight& B)
    {
        return A.Score == B.Score ? A.Path < B.Path : A.Score < B.Score;
    });
    ScreenLightCount = FMath::Min(4, Lights.Num());
    UTexture2D* Brush = Parameters.StrokeTexture.LoadSynchronous();
    UTexture2D* FlowMap = Parameters.FlowTexture.LoadSynchronous();
    for (UMaterialInstanceDynamic* MID : { DensityMID.Get(), FinishMID.Get() })
    {
        if (!MID) continue;
        for (TFieldIterator<FFloatProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
            MID->SetScalarParameterValue(It->GetFName(), It->GetPropertyValue_InContainer(&Parameters));
        for (TFieldIterator<FIntProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
            MID->SetScalarParameterValue(It->GetFName(), It->GetPropertyValue_InContainer(&Parameters));
        for (TFieldIterator<FStructProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
            if (It->Struct == TBaseStructure<FLinearColor>::Get())
                MID->SetVectorParameterValue(It->GetFName(), *It->ContainerPtrToValuePtr<FLinearColor>(&Parameters));
            else if (It->Struct == TBaseStructure<FVector>::Get())
            {
                const FVector V = *It->ContainerPtrToValuePtr<FVector>(&Parameters);
                MID->SetVectorParameterValue(It->GetFName(), FLinearColor(V.X, V.Y, V.Z, 1.f));
            }
        MID->SetVectorParameterValue(TEXT("Focus"), FLinearColor(Focus.X, Focus.Y, Focus.Z, 1.f));
        MID->SetVectorParameterValue(TEXT("VisionSource"), ScreenVisionSource);
        MID->SetScalarParameterValue(TEXT("UseScreenVision"), Parameters.bUseScreenVision ? 1.f : 0.f);
        if (Brush) MID->SetTextureParameterValue(TEXT("StrokeTexture"), Brush);
        if (FlowMap) MID->SetTextureParameterValue(TEXT("FlowTexture"), FlowMap);
        for (int32 Index = 0; Index < 4; ++Index)
        {
            FLinearColor Position = FLinearColor::Transparent, Direction(0,0,0,-2), Colour = FLinearColor::Transparent;
            if (Index < ScreenLightCount)
            {
                ULocalLightComponent* Light = Lights[Index].Light;
                const FVector P = Light->GetComponentLocation(), D = Light->GetForwardVector();
                Position = FLinearColor(P.X, P.Y, P.Z, Light->AttenuationRadius);
                Direction = FLinearColor(D.X, D.Y, D.Z, -2.f);
                if (USpotLightComponent* Spot = Cast<USpotLightComponent>(Light))
                    Direction.A = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Spot->OuterConeAngle, .1f, 89.f)));
                Colour = Light->GetLightColor();
                if (Light->bUseTemperature) Colour *= Light->GetColorTemperature();
                Colour = AirFogSafeColor(Colour, FLinearColor::Black, 10.f);
                // Range is geometric; intensity fades only negligible lights.
                const double Brightness = Light->ComputeLightBrightness();
                Colour.A = FMath::Clamp(static_cast<float>(Brightness / (Brightness + 1.)) * Light->VolumetricScatteringIntensity, 0.f, 1.f);
            }
            MID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightPosition%d"), Index)), Position);
            MID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightDirection%d"), Index)), Direction);
            MID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightColour%d"), Index)), Colour);
        }
        MID->SetScalarParameterValue(TEXT("TimeSeconds"), WorldTime);
        MID->SetScalarParameterValue(TEXT("UseArtistPalette"), Parameters.bUseArtistPalette ? 1.f : 0.f);
        MID->SetVectorParameterValue(TEXT("ArtistNearColor"), ArtistNearColor);
        MID->SetVectorParameterValue(TEXT("ArtistFarColor"), ArtistFarColor);
        MID->SetScalarParameterValue(TEXT("ArtistFogMix"), ArtistFogMix);
        MID->SetScalarParameterValue(TEXT("FormLightYaw"), ResolvedYaw);
        MID->SetScalarParameterValue(TEXT("FormLightPitch"), ResolvedPitch);
        MID->SetVectorParameterValue(TEXT("FormLightColor"), ResolvedColor);
    }
}

void AYogAirFogVolume::SuppressOtherHeightFog()
{
    UWorld* World = GetWorld();
    if (!World) return;
    LocalFogVolumeCount = 0;
    for (TObjectIterator<UExponentialHeightFogComponent> It; It; ++It)
    {
        if (*It == Carrier || It->GetWorld() != World || !It->IsRegistered()) continue;
        if (!OtherFogVisibility.Contains(*It))
        {
            OtherFogVisibility.Add(*It, It->GetVisibleFlag());
            It->SetVisibility(false, false);
        }
    }
    for (TObjectIterator<ULocalFogVolumeComponent> It; It; ++It)
        if (It->GetWorld() == World && It->IsRegistered() && It->IsVisible()) ++LocalFogVolumeCount;
}

void AYogAirFogVolume::RestoreOtherHeightFog()
{
    for (const auto& Pair : OtherFogVisibility)
    {
        UExponentialHeightFogComponent* Fog = Pair.Key.Get();
        if (Fog && Fog->GetWorld() == GetWorld() && !Fog->GetVisibleFlag()) Fog->SetVisibility(Pair.Value, false);
    }
    OtherFogVisibility.Reset();
    LocalFogVolumeCount = 0;
}

void AYogAirFogVolume::ManageLocalLightScattering()
{
    if (!bActive || !bManageLocalLightScattering || !GetWorld())
    {
        RestoreLocalLightScattering();
        return;
    }
    TSet<TWeakObjectPtr<ULocalLightComponent>> Eligible;
    for (TObjectIterator<ULocalLightComponent> It; It; ++It)
    {
        ULocalLightComponent* Light = *It;
        if (Light->GetWorld() != GetWorld() || !Light->IsRegistered()
            || Light->Mobility == EComponentMobility::Static || Light->IsTemplate()) continue;
        const TWeakObjectPtr<ULocalLightComponent> Key(Light);
        Eligible.Add(Key);
        FLocalLightState* Existing = LocalLightStates.Find(Key);
        if (!Existing)
        {
            FLocalLightState Initial;
            Initial.bOriginalVolumetricShadow = Light->bCastVolumetricShadow;
            Initial.OriginalScattering = Light->VolumetricScatteringIntensity;
            Initial.bLastVolumetricShadow = Light->bCastVolumetricShadow;
            Initial.LastScattering = Light->VolumetricScatteringIntensity;
            if (const uint8* OptOut = LocalLightOptOuts.Find(Key))
            {
                Initial.bExternalVolumetricShadow = (*OptOut & 1) != 0;
                Initial.bExternalScattering = (*OptOut & 2) != 0;
            }
            Existing = &LocalLightStates.Add(Key, Initial);
        }
        FLocalLightState& State = *Existing;
        // Respect later edits during the preview as well as at restore. A changed
        // field stays opted out for this activation; another field may still be ours.
        if (Light->bCastVolumetricShadow != State.bLastVolumetricShadow)
        {
            State.bExternalVolumetricShadow = true;
            State.bOwnsVolumetricShadow = false;
        }
        if (Light->VolumetricScatteringIntensity != State.LastScattering)
        {
            State.bExternalScattering = true;
            State.bOwnsScattering = false;
        }
        if (Light->CastShadows && Light->CastDynamicShadows)
        {
            if (State.bOwnsScattering)
            {
                AirFogRestoreScattering(Light, State.OriginalScattering);
                State.bOwnsScattering = false;
            }
            if (!State.bExternalVolumetricShadow && !Light->bCastVolumetricShadow)
            {
                Light->SetCastVolumetricShadow(true);
                State.bOwnsVolumetricShadow = true;
            }
        }
        else
        {
            // An unshadowed artistic fill must not illuminate fog through a wall.
            // Do not turn on its ordinary shadows or alter its surface-light intensity.
            if (State.bOwnsVolumetricShadow)
            {
                AirFogRestoreVolumetricShadow(Light, State.bOriginalVolumetricShadow);
                State.bOwnsVolumetricShadow = false;
            }
            if (!State.bExternalScattering && Light->VolumetricScatteringIntensity != 0.f)
            {
                Light->SetVolumetricScatteringIntensity(0.f);
                State.bOwnsScattering = true;
            }
        }
        State.bLastVolumetricShadow = Light->bCastVolumetricShadow;
        State.LastScattering = Light->VolumetricScatteringIntensity;
    }
    ManagedLocalLightCount = 0;
    for (auto It = LocalLightOptOuts.CreateIterator(); It; ++It)
        if (!It.Key().IsValid()) It.RemoveCurrent();
    for (auto It = LocalLightStates.CreateIterator(); It; ++It)
    {
        ULocalLightComponent* Light = It.Key().Get();
        FLocalLightState& State = It.Value();
        if (!Eligible.Contains(It.Key()))
        {
            if (Light && Light->GetWorld() == GetWorld())
            {
                if (State.bOwnsVolumetricShadow && Light->bCastVolumetricShadow == State.bLastVolumetricShadow)
                    AirFogRestoreVolumetricShadow(Light, State.bOriginalVolumetricShadow);
                if (State.bOwnsScattering && Light->VolumetricScatteringIntensity == State.LastScattering)
                    AirFogRestoreScattering(Light, State.OriginalScattering);
            }
            It.RemoveCurrent();
        }
        else if (State.bOwnsVolumetricShadow || State.bOwnsScattering) ++ManagedLocalLightCount;
    }
}

void AYogAirFogVolume::RestoreLocalLightScattering()
{
    for (const auto& Pair : LocalLightStates)
    {
        ULocalLightComponent* Light = Pair.Key.Get();
        const FLocalLightState& State = Pair.Value;
        if (!Light || Light->GetWorld() != GetWorld()) continue;
        // Save suspension restores the authored scene, then re-acquires it. Keep
        // user edits opted out across that internal boundary, including edits made
        // since the last scan. Explicit SetAirFogEnabled(false) clears this cache.
        uint8 OptOut = 0;
        if (State.bExternalVolumetricShadow || Light->bCastVolumetricShadow != State.bLastVolumetricShadow) OptOut |= 1;
        if (State.bExternalScattering || Light->VolumetricScatteringIntensity != State.LastScattering) OptOut |= 2;
        if (OptOut != 0) LocalLightOptOuts.FindOrAdd(Pair.Key) |= OptOut;
        if (State.bOwnsVolumetricShadow && Light->bCastVolumetricShadow == State.bLastVolumetricShadow)
            AirFogRestoreVolumetricShadow(Light, State.bOriginalVolumetricShadow);
        if (State.bOwnsScattering && Light->VolumetricScatteringIntensity == State.LastScattering)
            AirFogRestoreScattering(Light, State.OriginalScattering);
    }
    LocalLightStates.Reset();
    ManagedLocalLightCount = 0;
}

void AYogAirFogVolume::SuspendForSave(bool bSuspend)
{
    if (bSuspend)
    {
        ++SaveSuspensionDepth;
        if (SaveSuspensionDepth == 1) DeactivateAirFog();
    }
    else
    {
        SaveSuspensionDepth = FMath::Max(0, SaveSuspensionDepth - 1);
        if (SaveSuspensionDepth == 0 && bDesiredEnabled && !bActive) ActivateAirFog();
    }
}

#if WITH_EDITOR
void AYogAirFogVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AYogAirFogVolume, FocusPosition))
    {
        FocusActor.Reset();
        bHasExplicitFocusPosition = true;
    }
    ApplyParameters(Parameters);
}
#endif

FString AYogAirFogVolume::GetStatusText() const
{
    if (!bDesiredEnabled) return TEXT("空气雾已关闭，已还原场景雾与临时质量设置。");
    if (SaveSuspensionDepth > 0) return TEXT("保存期间暂停空气雾，保存后继续当前镜头与参数。");
    if (!LastError.IsEmpty()) return LastError;
    if (!bActive) return TEXT("空气雾等待激活。");
    FString Status = FString::Printf(TEXT("世界空间体积雾；清雾中心：%s；读取%d盏局部灯。笔刷直接参与三维消光与原生受光。"),
        ScreenVisionSource.A > .5f ? TEXT("游戏观察者") : TEXT("镜头位置"), ScreenLightCount);
    Status += TEXT(" ") + GetFormLightStatusText();
    if (Parameters.bUseArtistPalette && bPaletteFallback)
        Status += TEXT(" 配色材质缺少有效颜色或混合参数，对应项暂用兼容配色。");
    if (LocalFogVolumeCount > 0)
        Status += FString::Printf(TEXT(" 当前场景另有%d个局部雾组件，可能叠加密度。"), LocalFogVolumeCount);
    if (AirFogConsoleLease().Owners.Num() > 1)
        Status += TEXT(" 多个世界共享当前体积质量，单个世界关闭不会撤销其他预览。");
    if (AirFogConsoleLease().HasExternalOverride())
        Status += TEXT(" 检测到控制台外部调整，保留其当前设置。");
    return Status;
}
