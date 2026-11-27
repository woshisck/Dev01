#include "YogNinjaFogEnvironmentActor.h"
#include "YogNinjaFogMaskCache.h"
#include "YogFogSimulationComponent.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#include "ScopedTransaction.h"
#endif

namespace YogNinjaEnvironment
{
constexpr int32 MaxLights = 4;
constexpr double LightScanInterval = .5;
constexpr float LightRadiusScale = .45f;
const FName SmokeTag(TEXT("YogNinjaSmokeExact"));
const FName EnabledParameter(TEXT("SpatialEnvironmentEnabled"));
const FName LightNames[MaxLights] = {
    FName(TEXT("EnvLight0")), FName(TEXT("EnvLight1")),
    FName(TEXT("EnvLight2")), FName(TEXT("EnvLight3"))
};
const FName DirectionNames[MaxLights] = {
    FName(TEXT("EnvLightDirection0")), FName(TEXT("EnvLightDirection1")),
    FName(TEXT("EnvLightDirection2")), FName(TEXT("EnvLightDirection3"))
};

float Safe(float Value, float Default, float Min, float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Default, Min, Max);
}

bool FinitePosition(const FVector& Position)
{
    return FMath::IsFinite(Position.X) && FMath::IsFinite(Position.Y) && FMath::IsFinite(Position.Z);
}

FVector OnGroundPlane(const FVector& Origin, const FVector& Direction, float Height)
{
    if (FinitePosition(Direction) && FMath::Abs(Direction.Z) > .0001)
    {
        const double Distance = (Height - Origin.Z) / Direction.Z;
        if (FMath::IsFinite(Distance) && Distance >= 0.0 && Distance <= 200000.0)
            return Origin + Direction * Distance;
    }
    // A horizontal/upward view cannot intersect a ground plane in front of
    // the camera. Use its vertical ground projection instead of a stale hole.
    return FVector(Origin.X, Origin.Y, Height);
}
}

AYogNinjaFogEnvironmentActor::AYogNinjaFogEnvironmentActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    bAllowTickBeforeBeginPlay = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    Tags.AddUnique(TEXT("YogNinjaFogEnvironment"));
}

float AYogNinjaFogEnvironmentActor::GroundHeight() const
{
    return YogNinjaEnvironment::Safe(ManualGroundHeight, 0.f, -1000000.f, 1000000.f);
}

UMaterialInstanceDynamic* AYogNinjaFogEnvironmentActor::ResolveOutputMaterial() const
{
    if (!IsValid(FogCarrier) || FogCarrier->IsActorBeingDestroyed() || FogCarrier->GetWorld() != GetWorld()
        || !FogCarrier->ActorHasTag(YogNinjaEnvironment::SmokeTag)) return nullptr;
    // Read the original Blueprint's current MID, not its output MIC asset.
    // Construction/PIE can replace it, so resolve this small property each
    // update rather than keeping a stale material pointer indefinitely.
    const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(
        FogCarrier->GetClass(), TEXT("DynamicMaterialInstance"));
    UMaterialInstanceDynamic* Material = Property
        ? Cast<UMaterialInstanceDynamic>(Property->GetObjectPropertyValue_InContainer(FogCarrier.Get())) : nullptr;
    return IsValid(Material) ? Material : nullptr;
}

bool AYogNinjaFogEnvironmentActor::ResolveObserverPosition(FVector& Position, FString& Source)
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (World->IsGameWorld())
    {
        APlayerController* Player = CachedLocalPlayer.Get();
        if (!IsValid(Player) || Player->GetWorld() != World || !Player->IsLocalController())
        {
            CachedLocalPlayer.Reset();
            Player = nullptr;
            // Iterates the local world controller list, never the actor set.
            for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
            {
                APlayerController* Candidate = It->Get();
                if (IsValid(Candidate) && Candidate->IsLocalController())
                {
                    CachedLocalPlayer = Candidate;
                    Player = Candidate;
                    break;
                }
            }
        }
        if (IsValid(Player))
        {
            if (APawn* Pawn = Player->GetPawn(); IsValid(Pawn)
                && YogNinjaEnvironment::FinitePosition(Pawn->GetActorLocation()))
            {
                Position = Pawn->GetActorLocation();
                Source = TEXT("本地玩家");
                return true;
            }
            FVector Camera;
            FRotator Rotation;
            Player->GetPlayerViewPoint(Camera, Rotation);
            if (YogNinjaEnvironment::FinitePosition(Camera) && !Rotation.ContainsNaN())
            {
                Position = YogNinjaEnvironment::OnGroundPlane(Camera, Rotation.Vector(), GroundHeight());
                Source = TEXT("无玩家：镜头地面交点");
                return YogNinjaEnvironment::FinitePosition(Position);
            }
        }
        Source = TEXT("等待本地玩家或镜头");
        return false;
    }
#if WITH_EDITOR
    if (World->WorldType == EWorldType::Editor && GEditor)
    {
        FLevelEditorViewportClient* Viewport = nullptr;
        FViewport* Active = GEditor->GetActiveViewport();
        for (FLevelEditorViewportClient* Candidate : GEditor->GetLevelViewportClients())
        {
            if (Candidate && Candidate->GetWorld() == World && Candidate->Viewport == Active)
            {
                Viewport = Candidate;
                break;
            }
        }
        if (!Viewport && GCurrentLevelEditingViewportClient && GCurrentLevelEditingViewportClient->GetWorld() == World)
            Viewport = GCurrentLevelEditingViewportClient;
        if (Viewport && YogNinjaEnvironment::FinitePosition(Viewport->GetViewLocation()))
        {
            Position = YogNinjaEnvironment::OnGroundPlane(Viewport->GetViewLocation(),
                Viewport->GetViewRotation().Vector(), GroundHeight());
            Source = TEXT("编辑视口中心地面交点");
            return YogNinjaEnvironment::FinitePosition(Position);
        }
    }
#endif
    Source = TEXT("没有当前场景的有效视口");
    return false;
}

bool AYogNinjaFogEnvironmentActor::IsUsableLight(const ULocalLightComponent* Light) const
{
    const UWorld* World = GetWorld();
    const AActor* LightOwner = IsValid(Light) ? Light->GetOwner() : nullptr;
    if (!World || !IsValid(Light) || Light->IsTemplate() || Light->GetWorld() != World || !Light->IsRegistered()
        || !Light->IsVisible() || Light->bHiddenInGame || !Light->bAffectsWorld
        || !IsValid(LightOwner) || LightOwner->IsHidden() || LightOwner->IsActorBeingDestroyed()
        || !YogNinjaEnvironment::FinitePosition(Light->GetComponentLocation())
        || !YogNinjaEnvironment::FinitePosition(Light->GetForwardVector())
        || !FMath::IsFinite(Light->AttenuationRadius) || Light->AttenuationRadius <= 0.f) return false;
#if WITH_EDITOR
    if (!World->IsGameWorld() && LightOwner->IsHiddenEd()) return false;
#endif
    const float Brightness = Light->ComputeLightBrightness();
    return FMath::IsFinite(Brightness) && Brightness > KINDA_SMALL_NUMBER;
}

void AYogNinjaFogEnvironmentActor::RefreshLightCache(const FVector& ObserverPosition)
{
    struct FCandidate
    {
        ULocalLightComponent* Light;
        double DistanceSquared;
        FString Path;
    };
    TArray<FCandidate, TInlineAllocator<YogNinjaEnvironment::MaxLights>> Candidates;
    // This is the only light discovery scan. It is throttled to twice per
    // second; only four weak references survive into the frame update path.
    for (TObjectIterator<ULocalLightComponent> It; It; ++It)
    {
        ULocalLightComponent* Light = *It;
        if (!IsUsableLight(Light)) continue;
        const double Distance = FVector::DistSquared(ObserverPosition, Light->GetComponentLocation());
        if (!FMath::IsFinite(Distance)) continue;
        const FString Path = Light->GetPathName();
        int32 Index = 0;
        while (Index < Candidates.Num() && (Candidates[Index].DistanceSquared < Distance
            || (Candidates[Index].DistanceSquared == Distance && Candidates[Index].Path < Path))) ++Index;
        if (Index >= YogNinjaEnvironment::MaxLights) continue;
        if (Candidates.Num() == YogNinjaEnvironment::MaxLights)
            Candidates.RemoveAt(YogNinjaEnvironment::MaxLights - 1, 1, EAllowShrinking::No);
        Candidates.Insert(FCandidate{Light, Distance, Path}, Index);
    }
    CachedLights.Reset();
    for (const FCandidate& Candidate : Candidates) CachedLights.Add(Candidate.Light);
    NextLightScanTime = FPlatformTime::Seconds() + YogNinjaEnvironment::LightScanInterval;
}

void AYogNinjaFogEnvironmentActor::PushEnvironment(bool bForceLightRefresh)
{
    if (IsTemplate()) return;
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer) return;
#if WITH_EDITOR
    // The editor object must never push into or compete with the PIE copy.
    if (World->WorldType == EWorldType::Editor && GEditor
        && (GEditor->PlayWorld || GEditor->bIsSimulatingInEditor)) return;
#endif
    UMaterialInstanceDynamic* Material = ResolveOutputMaterial();
    if (LastMaterial.Get() != Material)
    {
        if (UMaterialInstanceDynamic* Previous = LastMaterial.Get())
        {
            Previous->SetScalarParameterValue(YogNinjaEnvironment::EnabledParameter, 0.f);
            Previous->SetVectorParameterValue(TEXT("HybridMaskLayout"), FLinearColor(128,16,4,0));
        }
        LastMaterial = Material;
    }
    if (!Material)
    {
        ReleaseMaskCache();
        ActiveLightCount = 0;
        BindingStatus = TEXT("请绑定当前场景的 Ninja 雾体，等待动态材质就绪");
        return;
    }
    Material->SetScalarParameterValue(YogNinjaEnvironment::EnabledParameter, bEnvironmentEnabled ? 1.f : 0.f);
    BindingStatus = TEXT("仅更新绑定雾体的动态材质");
    if (!bEnvironmentEnabled)
    {
        Material->SetVectorParameterValue(TEXT("HybridMaskLayout"), FLinearColor(128,16,4,0));
        ReleaseMaskCache();
        ActiveLightCount = 0;
        return;
    }
    FVector ObserverPosition = FVector::ZeroVector;
    const bool bHaveObserver = ResolveObserverPosition(ObserverPosition, ObserverStatus);
    if (!bHaveObserver)
        ObserverPosition = IsValid(FogCarrier) ? FogCarrier->GetActorLocation() : GetActorLocation();
    if (!YogNinjaEnvironment::FinitePosition(ObserverPosition)) ObserverPosition = FVector::ZeroVector;
    if (bForceLightRefresh || FPlatformTime::Seconds() >= NextLightScanTime)
        RefreshLightCache(ObserverPosition);

    const float Height = GroundHeight();
    // These are independent world-space material inputs. In particular, never
    // overwrite Ninja's local-box LightPosition or invoke its RefreshLighting.
    // SpatialProfileEnabled stays under authoring control at zero; it is not
    // the activation switch for this new environment layer.
    Material->SetScalarParameterValue(TEXT("SpatialGroundHeight"), Height);
    Material->SetScalarParameterValue(TEXT("EnvGroundHeight"), Height);
    Material->SetScalarParameterValue(TEXT("EnvGroundBodyHeight"), YogNinjaEnvironment::Safe(GroundBodyHeight, 180.f, 1.f, 10000.f));
    Material->SetScalarParameterValue(TEXT("EnvGroundAmount"), YogNinjaEnvironment::Safe(GroundAmount, 2.5f, 0.f, 20.f));
    Material->SetScalarParameterValue(TEXT("EnvUpperAmount"), YogNinjaEnvironment::Safe(UpperAmount, .55f, 0.f, 20.f));
    Material->SetScalarParameterValue(TEXT("EnvSurfaceFade"), YogNinjaEnvironment::Safe(SurfaceFade, 40.f, 1.f, 5000.f));
    Material->SetScalarParameterValue(TEXT("EnvSurfaceBand"), YogNinjaEnvironment::Safe(SurfaceBand, 250.f, 1.f, 10000.f));
    Material->SetScalarParameterValue(TEXT("EnvContactBoost"), YogNinjaEnvironment::Safe(ContactBoost, .35f, 0.f, 10.f));
    Material->SetScalarParameterValue(TEXT("EnvPlayerStrength"), YogNinjaEnvironment::Safe(PlayerStrength, .85f, 0.f, 1.f));
    Material->SetScalarParameterValue(TEXT("EnvPlayerFeather"), YogNinjaEnvironment::Safe(PlayerFeather, 400.f, 1.f, 10000.f));
    Material->SetScalarParameterValue(TEXT("EnvPlayerHeight"), YogNinjaEnvironment::Safe(PlayerHeight, 350.f, 1.f, 10000.f));
    Material->SetScalarParameterValue(TEXT("EnvLightStrength"), YogNinjaEnvironment::Safe(LightStrength, .8f, 0.f, 1.f));
    Material->SetScalarParameterValue(TEXT("EnvLightFeather"), YogNinjaEnvironment::Safe(LightFeather, 450.f, 1.f, 10000.f));
    Material->SetVectorParameterValue(TEXT("EnvObserver"), FLinearColor(
        ObserverPosition.X, ObserverPosition.Y, ObserverPosition.Z,
        bHaveObserver ? YogNinjaEnvironment::Safe(PlayerRadius, 450.f, 0.f, 50000.f) : 0.f));

    ActiveLightCount = 0;
    for (int32 Index = 0; Index < YogNinjaEnvironment::MaxLights; ++Index)
    {
        FLinearColor Position = FLinearColor::Transparent;
        FLinearColor Direction(0.f, 0.f, 0.f, -1.f);
        ULocalLightComponent* Light = CachedLights.IsValidIndex(Index) ? CachedLights[Index].Get() : nullptr;
        if (IsUsableLight(Light))
        {
            const FVector WorldPosition = Light->GetComponentLocation();
            const FVector WorldDirection = Light->GetForwardVector();
            const float ClearRadius = FMath::Min(Light->AttenuationRadius * YogNinjaEnvironment::LightRadiusScale,
                YogNinjaEnvironment::Safe(LightRadiusLimit, 800.f, 1.f, 50000.f));
            Position = FLinearColor(WorldPosition.X, WorldPosition.Y, WorldPosition.Z,
                ClearRadius);
            Direction = FLinearColor(WorldDirection.X, WorldDirection.Y, WorldDirection.Z, -1.f);
            if (const USpotLightComponent* Spot = Cast<USpotLightComponent>(Light))
                Direction.A = FMath::Cos(FMath::DegreesToRadians(
                    YogNinjaEnvironment::Safe(Spot->OuterConeAngle, 45.f, .1f, 89.f)));
            ++ActiveLightCount;
        }
        // Disabled/destroyed lights are cleared immediately, not after the
        // next discovery scan. Rect lights use the radial point approximation.
        Material->SetVectorParameterValue(YogNinjaEnvironment::LightNames[Index], Position);
        Material->SetVectorParameterValue(YogNinjaEnvironment::DirectionNames[Index], Direction);
    }
    UpdateMaskCache(Material, ObserverPosition, bHaveObserver, bForceLightRefresh);
}

void AYogNinjaFogEnvironmentActor::UpdateMaskCache(UMaterialInstanceDynamic* Material,
    const FVector& ObserverPosition, bool bHaveObserver, bool bForce)
{
    float HybridSamples = 0.f;
    if (!bUseCachedEnvironment || !Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("HybridShadowSamples")),HybridSamples))
    {
        Material->SetVectorParameterValue(TEXT("HybridMaskLayout"),FLinearColor(128,16,4,0));
        ReleaseMaskCache();
        return;
    }
    const int32 Resolution=FMath::Clamp(MaskResolution,32,256);
    const int32 Slices=FMath::Clamp(MaskHeightSlices,4,32);
    const int32 Columns=4;
    const int32 Rows=FMath::DivideAndRoundUp(Slices,Columns);
    if (!EnvironmentMask || EnvironmentMask->SizeX!=Resolution*Columns || EnvironmentMask->SizeY!=Resolution*Rows)
    {
        ReleaseMaskCache();
        EnvironmentMask=NewObject<UTextureRenderTarget2D>(this,NAME_None,RF_Transient);
        EnvironmentMask->bCanCreateUAV=true;
        EnvironmentMask->ClearColor=FLinearColor(1,1,0,1);
        EnvironmentMask->Filter=TF_Bilinear;
        EnvironmentMask->InitCustomFormat(Resolution*Columns,Resolution*Rows,PF_FloatRGBA,true);
        EnvironmentMask->UpdateResourceImmediate(true);
    }
    if (!EnvironmentMaskVolume || EnvironmentMaskVolume->SizeX!=Resolution
        || EnvironmentMaskVolume->SizeY!=Resolution || EnvironmentMaskVolume->SizeZ!=Slices)
    {
        ReleaseMaskCache();
        EnvironmentMaskVolume=NewObject<UTextureRenderTargetVolume>(this,NAME_None,RF_Transient);
        EnvironmentMaskVolume->bSupportsUAV=true;
        EnvironmentMaskVolume->bForceLinearGamma=true;
        EnvironmentMaskVolume->SRGB=false;
        EnvironmentMaskVolume->ClearColor=FLinearColor(1,1,0,1);
        EnvironmentMaskVolume->Filter=TF_Bilinear;
        EnvironmentMaskVolume->Init(Resolution,Resolution,Slices,PF_FloatRGBA);
        // Consume the one-shot deferred clear before the shared producer runs.
        EnvironmentMaskVolume->UpdateResourceImmediate(true);
    }
    if (!MaskRenderState.IsValid())
    {
        MaskRenderState=MakeShared<YogNinjaFogMaskCache::FState,ESPMode::ThreadSafe>();
        MaskRevision=0;MaskDomainRevision=1;NextMaskUpdateTime=0.0;
    }
    FTextureResource* Resource=EnvironmentMask->GetResource();
    FTextureResource* VolumeResource=EnvironmentMaskVolume->GetResource();
    if (!Resource || !Resource->TextureRHI.IsValid()
        || !VolumeResource || !VolumeResource->TextureRHI.IsValid()) return;
    FVector Minimum, Size;
    if (!ResolveMaskDomain(Material, ObserverPosition, Minimum, Size))
    {
        Material->SetVectorParameterValue(TEXT("HybridMaskLayout"),FLinearColor(Resolution,Slices,Columns,0));
        ReleaseMaskCache();
        return;
    }
    const bool bDomainChanged=!Minimum.Equals(CachedMaskMin,.01)||!Size.Equals(CachedMaskSize,.01);
    if (bDomainChanged)
    {
        CachedMaskMin=Minimum;CachedMaskSize=Size;NextMaskUpdateTime=0.0;
        MaskDomainRevision=MaskRevision+1;
    }
    MaskBakeCount=MaskRenderState->Executed.GetValue();
    const bool bReady=MaskBakeCount>0 && MaskRenderState->CompletedRevision.GetValue()>=static_cast<int32>(MaskDomainRevision);
    float WakeStrength = YogNinjaEnvironment::Safe(PlayerWakeStrength, 0.f, 0.f, 4.f);
    if (WakeStrength > 0.f)
    {
        float BoundaryMotion;
        if (Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("HybridBoundaryMotion")), BoundaryMotion))
            WakeStrength *= YogNinjaEnvironment::Safe(BoundaryMotion, 1.f, 0.f, 1.f);
    }
    UpdatePlayerWake(Minimum, Size, WakeStrength);
    Material->SetTextureParameterValue(TEXT("HybridEnvironmentMask"),EnvironmentMask);
    Material->SetTextureParameterValue(TEXT("HybridEnvironmentVolume"),EnvironmentMaskVolume);
    Material->SetVectorParameterValue(TEXT("HybridMaskMin"),FLinearColor(Minimum.X,Minimum.Y,Minimum.Z,0));
    Material->SetVectorParameterValue(TEXT("HybridMaskInvSize"),FLinearColor(1./Size.X,1./Size.Y,1./Size.Z,0));
    Material->SetVectorParameterValue(TEXT("HybridMaskLayout"),FLinearColor(Resolution,Slices,Columns,bReady?1.f:0.f));
    const double Now=FPlatformTime::Seconds();
    if (!bForce && Now<NextMaskUpdateTime) return;
    NextMaskUpdateTime=Now+1.0/YogNinjaEnvironment::Safe(MaskUpdatesPerSecond,10.f,1.f,30.f);
    YogNinjaFogMaskCache::FInput Input;
    Input.Scene=GetWorld()->Scene;Input.Target=Resource->TextureRHI;
    Input.VolumeTarget=VolumeResource->TextureRHI;
    Input.Min=FVector3f(Minimum);Input.Size=FVector3f(Size);
    Input.Resolution=Resolution;Input.Slices=Slices;Input.Columns=Columns;
    Input.GroundHeight=GroundHeight();Input.BodyHeight=YogNinjaEnvironment::Safe(GroundBodyHeight,180.f,1.f,10000.f);
    Input.GroundAmount=YogNinjaEnvironment::Safe(GroundAmount,2.5f,0.f,20.f);
    Input.UpperAmount=YogNinjaEnvironment::Safe(UpperAmount,.55f,0.f,20.f);
    Input.SurfaceFade=YogNinjaEnvironment::Safe(SurfaceFade,40.f,1.f,5000.f);
    Input.SurfaceBand=YogNinjaEnvironment::Safe(SurfaceBand,250.f,1.f,10000.f);
    Input.ContactBoost=YogNinjaEnvironment::Safe(ContactBoost,.35f,0.f,10.f);
    Input.HeightEnabled=bSpatialHeightEnabled?1u:0u;
    Input.NearHeight=YogNinjaEnvironment::Safe(NearFogHeight,35.f,1.f,10000.f);
    Input.FarHeight=FMath::Max(Input.NearHeight,YogNinjaEnvironment::Safe(FarFogHeight,350.f,1.f,10000.f));
    Input.WallHeight=FMath::Max(Input.FarHeight,YogNinjaEnvironment::Safe(WallFogHeight,500.f,1.f,10000.f));
    Input.HeightFeather=YogNinjaEnvironment::Safe(HeightFeather,100.f,1.f,5000.f);
    Input.WallRange=YogNinjaEnvironment::Safe(WallRange,150.f,1.f,1000.f);
    Input.PlayerStrength=YogNinjaEnvironment::Safe(PlayerStrength,.85f,0.f,1.f);
    Input.PlayerFeather=YogNinjaEnvironment::Safe(PlayerFeather,400.f,1.f,10000.f);
    Input.PlayerHeight=YogNinjaEnvironment::Safe(PlayerHeight,350.f,1.f,10000.f);
    Input.LightStrength=YogNinjaEnvironment::Safe(LightStrength,.8f,0.f,1.f);
    Input.LightFeather=YogNinjaEnvironment::Safe(LightFeather,450.f,1.f,10000.f);
    Input.Observer=FVector4f(ObserverPosition.X,ObserverPosition.Y,ObserverPosition.Z,bHaveObserver?YogNinjaEnvironment::Safe(PlayerRadius,450.f,0.f,50000.f):0.f);
    for (int32 Index=0;Index<4;Index++)
    {
        const FLinearColor Light=Material->K2_GetVectorParameterValue(YogNinjaEnvironment::LightNames[Index]);
        const FLinearColor Direction=Material->K2_GetVectorParameterValue(YogNinjaEnvironment::DirectionNames[Index]);
        Input.Lights[Index]=FVector4f(Light.R,Light.G,Light.B,Light.A);
        Input.Directions[Index]=FVector4f(Direction.R,Direction.G,Direction.B,Direction.A);
    }
    Input.Revision=++MaskRevision;
    FTextureRenderTargetResource* WakeResource = nullptr;
    if (IsValid(PlayerWake) && PlayerWake->DensityField && PlayerWake->SubmittedStepCount > 0)
    {
        const FVector WakeSize = PlayerWake->ActualDomainExtent * 2.;
        Input.WakeMin = FVector3f(PlayerWake->ActualDomainCenter - PlayerWake->ActualDomainExtent);
        Input.WakeInvSize = FVector3f(FVector::OneVector / WakeSize);
        // The solver's G decays exponentially. Fade its very last residual before retiring the grid.
        const float Idle = float(GetWorld()->GetTimeSeconds() - LastWakeMotion);
        Input.WakeStrength = WakeStrength * (1.f - FMath::SmoothStep(6.f, 8.f, Idle));
        WakeResource = PlayerWake->DensityField->GameThread_GetRenderTargetResource();
    }
    YogNinjaFogMaskCache::Submit(GetUniqueID(),Input,MaskRenderState.ToSharedRef(),WakeResource);
}

void AYogNinjaFogEnvironmentActor::ReleasePlayerWake()
{
    if (IsValid(PlayerWake))
    {
        PlayerWake->DestroyComponent();
        NextMaskUpdateTime = 0.;
    }
    PlayerWake = nullptr;
    WakePawn.Reset();
    LastWakeSample = LastWakeMotion = -1.;
}

void AYogNinjaFogEnvironmentActor::UpdatePlayerWake(const FVector& MaskMin, const FVector& MaskSize, float Strength)
{
    UWorld* World = GetWorld();
    if (Strength <= 0.f || !World || !World->IsGameWorld()
        || !HasActorBegunPlay() || !IsValid(FogCarrier) || !FApp::CanEverRender())
    {
        if (PlayerWake || LastWakeSample >= 0.) ReleasePlayerWake();
        return;
    }
    APlayerController* Controller = CachedLocalPlayer.Get();
    APawn* Pawn = Controller && Controller->IsLocalController() ? Controller->GetPawn() : nullptr;
    if (!IsValid(Pawn) || Pawn->GetWorld() != World || !YogNinjaEnvironment::FinitePosition(Pawn->GetActorLocation()))
    {
        ReleasePlayerWake();
        return;
    }

    const FVector Position = Pawn->GetActorLocation();
    const double Now = World->GetTimeSeconds();
    const double Step = Now - LastWakeSample;
    const FVector Travel = Position - LastWakePosition;
    // A respawn, teleport or long discontinuity starts a new trail without joining positions.
    if (WakePawn.Get() != Pawn || Step < 0. || Step > .5 || Travel.SizeSquared() > FMath::Square(350.))
    {
        ReleasePlayerWake();
        WakePawn = Pawn;
        LastWakePosition = Position;
        LastWakeSample = Now;
        return;
    }
    if (Step < UE_SMALL_NUMBER) return;
    const bool bHadBody = LastWakeMotion == LastWakeSample;
    LastWakePosition = Position;
    LastWakeSample = Now;

    // A fixed grid in carrier space covers the low bank. Never recenter on the player or camera.
    constexpr double Voxel = 36.;
    const double Bottom = FMath::Max(MaskMin.Z, double(GroundHeight()) - Voxel);
    const double Top = FMath::Min(MaskMin.Z + MaskSize.Z, double(GroundHeight()) + 10. * Voxel);
    if (Top <= Bottom + 1.)
    {
        ReleasePlayerWake();
        return;
    }
    const FVector Size(FMath::Min(MaskSize.X, 128. * Voxel), FMath::Min(MaskSize.Y, 128. * Voxel), Top - Bottom);
    const FVector Center(MaskMin.X + MaskSize.X * .5, MaskMin.Y + MaskSize.Y * .5, (Top + Bottom) * .5);
    const FVector Extent = Size * .5;
    if (PlayerWake && (!PlayerWake->ActualDomainCenter.Equals(Center, .1) || !PlayerWake->ActualDomainExtent.Equals(Extent, .1)))
    {
        ReleasePlayerWake();
        return;
    }

    const FVector MeasuredVelocity = Travel / Step;
    const bool bMoved = FBox(Center - Extent, Center + Extent).IsInsideOrOn(Position)
        && MeasuredVelocity.SizeSquared() > FMath::Square(8.);
    if (bMoved)
    {
        if (!PlayerWake)
        {
            PlayerWake = NewObject<UYogFogSimulationComponent>(this, NAME_None, RF_Transient);
            PlayerWake->bSimulateInEditor = false;
            PlayerWake->GridResolution = FIntVector(FMath::Clamp(FMath::RoundToInt(Size.X / Voxel), 8, 128),
                FMath::Clamp(FMath::RoundToInt(Size.Y / Voxel), 8, 128),
                FMath::Clamp(FMath::RoundToInt(Size.Z / Voxel), 8, 128));
            PlayerWake->SetDomain(Center, Extent);
            PlayerWake->AddTickPrerequisiteActor(this);
            PlayerWake->RegisterComponent();
        }
        FYogFogSimBody Body;
        Body.Center = Position;
        const FVector MovementVelocity = Pawn->GetVelocity();
        Body.Velocity = (!MovementVelocity.ContainsNaN() && MovementVelocity.SizeSquared() > FMath::Square(8.)
            ? MovementVelocity : MeasuredVelocity).GetClampedToMaxSize(1500.);
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Pawn->GetRootComponent());
        Body.Radius = FMath::Max(float(2. * Voxel), Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f);
        Body.HalfHeight = FMath::Max(Body.Radius, Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f);
        PlayerWake->SetBodies({Body});
        LastWakeMotion = Now;
    }
    else if (PlayerWake)
    {
        if (Now - LastWakeMotion >= 8.) ReleasePlayerWake();
        else if (bHadBody) PlayerWake->SetBodies({});
    }
}

bool AYogNinjaFogEnvironmentActor::ResolveMaskDomain(UMaterialInstanceDynamic* Material,
    const FVector& ObserverPosition, FVector& Minimum, FVector& Size) const
{
    if (!IsValid(FogCarrier)) return false;
    // Ninja marches an analytic 100 cm local box. SM_CameraPlane deliberately
    // has 600 cm dummy bounds to keep the raster proxy visible; using those
    // bounds wastes six times the XY cache span and blurs local interaction.
    FTransform FieldTransform=FogCarrier->GetActorTransform();
    TInlineComponentArray<USceneComponent*> Components(FogCarrier);
    for (const USceneComponent* Component:Components)
        if (Component && Component->GetFName()==TEXT("DepthSlicer"))
        {
            FieldTransform.SetRotation(Component->GetComponentQuat());
            FieldTransform.SetScale3D(Component->GetComponentScale());
            break;
        }
    const FLinearColor PositionOffset=Material->K2_GetVectorParameterValue(TEXT("PositionOffset"));
    FieldTransform.SetTranslation(FogCarrier->GetActorLocation()+FVector(PositionOffset.R,PositionOffset.G,PositionOffset.B));
    const FBox Bounds=FBox(FVector(-50),FVector(50)).TransformBy(FieldTransform);
    Minimum=Bounds.Min;
    Minimum.Z=FMath::Max(Minimum.Z,static_cast<double>(GroundHeight()-100.f));
    Size=(Bounds.Max-Minimum).ComponentMax(FVector(1));
    return true;
}

void AYogNinjaFogEnvironmentActor::ReleaseMaskCache()
{
    ReleasePlayerWake();
    if (MaskRenderState.IsValid()) YogNinjaFogMaskCache::Remove(GetUniqueID());
    MaskRenderState.Reset();MaskBakeCount=0;NextMaskUpdateTime=0.0;
}

void AYogNinjaFogEnvironmentActor::DisableEnvironmentOverride()
{
    UMaterialInstanceDynamic* Previous = LastMaterial.Get();
    if (Previous) Previous->SetScalarParameterValue(YogNinjaEnvironment::EnabledParameter, 0.f);
    if (Previous) Previous->SetVectorParameterValue(TEXT("HybridMaskLayout"),FLinearColor(128,16,4,0));
    if (UMaterialInstanceDynamic* Current = ResolveOutputMaterial(); Current && Current != Previous)
    {
        Current->SetScalarParameterValue(YogNinjaEnvironment::EnabledParameter, 0.f);
        Current->SetVectorParameterValue(TEXT("HybridMaskLayout"),FLinearColor(128,16,4,0));
    }
    LastMaterial.Reset();
    CachedLights.Reset();
    CachedLocalPlayer.Reset();
    ActiveLightCount = 0;
    NextLightScanTime = 0.0;
    ReleaseMaskCache();
}

void AYogNinjaFogEnvironmentActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PushEnvironment(false);
}

void AYogNinjaFogEnvironmentActor::RefreshEnvironment()
{
    PushEnvironment(true);
}

void AYogNinjaFogEnvironmentActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    PushEnvironment(false);
}

void AYogNinjaFogEnvironmentActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshEnvironment();
}

void AYogNinjaFogEnvironmentActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisableEnvironmentOverride();
    Super::EndPlay(EndPlayReason);
}

void AYogNinjaFogEnvironmentActor::Destroyed()
{
    DisableEnvironmentOverride();
    Super::Destroyed();
}

void AYogNinjaFogEnvironmentActor::BeginDestroy()
{
    ReleaseMaskCache();
    Super::BeginDestroy();
}

void AYogNinjaFogEnvironmentActor::EnableEditorPreview()
{
#if WITH_EDITOR
    if (!GEditor || !GetWorld() || GetWorld()->IsGameWorld()) return;
    for (FLevelEditorViewportClient* Client:GEditor->GetLevelViewportClients())
        if (Client && Client->GetWorld()==GetWorld()) { Client->SetRealtime(true);Client->Invalidate(); }
#endif
}

#if WITH_EDITOR
void AYogNinjaFogEnvironmentActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    // A dragged artist control must not turn the half-second discovery scan
    // into a per-frame scan. The explicit refresh button still forces one.
    PushEnvironment(false);
}
#endif

void AYogNinjaFogEnvironmentActor::SampleGroundHeight()
{
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    FVector Position;
    FString Source;
    if (!ResolveObserverPosition(Position, Source)) Position = GetActorLocation();
    if (!YogNinjaEnvironment::FinitePosition(Position)) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(YogNinjaFogSampleGround), true);
    Query.AddIgnoredActor(this);
    if (IsValid(FogCarrier)) Query.AddIgnoredActor(FogCarrier);
    if (APlayerController* Player = CachedLocalPlayer.Get())
        if (APawn* Pawn = Player->GetPawn()) Query.AddIgnoredActor(Pawn);
    const FVector Start = Position + FVector(0, 0, 500);
    const FVector End = Position - FVector(0, 0, 100000);
    FHitResult Hit;
    if (!World->LineTraceSingleByObjectType(Hit, Start, End,
        FCollisionObjectQueryParams(ECC_WorldStatic), Query)
        || !YogNinjaEnvironment::FinitePosition(Hit.ImpactPoint))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ninja fog ground sampling found no WorldStatic surface; keep height %.1f."), ManualGroundHeight);
        return;
    }
#if WITH_EDITOR
    const FScopedTransaction Transaction(NSLOCTEXT("YogNinjaFogEnvironment", "SampleGround", "拾取雾体地面高度"));
    Modify();
#endif
    ManualGroundHeight = Hit.ImpactPoint.Z;
    RefreshEnvironment();
}
