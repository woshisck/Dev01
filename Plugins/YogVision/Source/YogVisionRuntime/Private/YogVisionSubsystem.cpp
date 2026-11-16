#include "YogVisionSubsystem.h"
#include "YogVisionObserverComponent.h"
#include "YogVisionSourceComponent.h"
#include "YogVisionTargetComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "UObject/UObjectIterator.h"
#include "Interfaces/Interface_PostProcessVolume.h"

namespace
{
constexpr int32 MaxSources = 4;
constexpr int32 AngularSamples = 64;
constexpr int32 MaxEchoes = 8;
constexpr float UpdateInterval = .1f;
float SafeHeight(float Height) { return FMath::IsFinite(Height) ? FMath::Clamp(Height, 0.f, 500.f) : 70.f; }
void IgnoreActorTree(FCollisionQueryParams& Query, AActor* Actor)
{
    if (!Actor) return;
    Query.AddIgnoredActor(Actor);
    TArray<AActor*> Children;
    Actor->GetAllChildActors(Children, true);
    TArray<AActor*> Attached;
    Actor->GetAttachedActors(Attached, true, true);
    for (AActor* Child : Attached)
        if (IsValid(Child) && Child->IsOwnedBy(Actor)) Children.AddUnique(Child);
    Query.AddIgnoredActors(Children);
}
}

void UYogVisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    const UYogVisionSettings* Settings = GetDefault<UYogVisionSettings>();
    Parameters = Settings->Parameters;
    Parameters.Sanitize();
    bEnabled = GetWorld()->IsGameWorld() && Settings->bEnabled;
    for (TObjectIterator<UYogVisionSourceComponent> It; It; ++It)
        if (It->GetWorld() == GetWorld() && It->IsRegistered()) RegisterSource(*It);
    for (TObjectIterator<UYogVisionTargetComponent> It; It; ++It)
        if (It->GetWorld() == GetWorld() && It->IsRegistered()) RegisterTarget(*It);
}

bool UYogVisionSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

void UYogVisionSubsystem::Deinitialize()
{
    bEnabled = false;
    RemoveRenderObjects();
    Sources.Reset();
    Targets.Reset();
    SmoothedSourceRadii.Reset();
    PreviousTargetPositions.Reset();
    PreviewObserver.Reset();
    bHasPreviewSnapshot = false;
    Super::Deinitialize();
}

TStatId UYogVisionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UYogVisionSubsystem, STATGROUP_Tickables);
}

void UYogVisionSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!bEnabled || !World || World->bIsTearingDown) return;
    if (World->GetNetMode() != NM_DedicatedServer && !MaterialInstance) CreateRenderObjects();
    AActor* Observer = ResolveObserver();
    if (World->GetNetMode() != NM_DedicatedServer)
    {
        const UYogVisionObserverComponent* ObserverComponent = IsValid(Observer) ? Observer->FindComponentByClass<UYogVisionObserverComponent>() : nullptr;
        const bool bPresentVision = IsPresentationActive() && IsValid(Observer) && !Observer->IsHidden() && (!ObserverComponent || ObserverComponent->bVisionEnabled);
        // Enforce cached presentation each frame: independently ticking health bars/VFX cannot leak identity between perception updates.
        for (const auto& Target : Targets) if (Target.IsValid()) Target->UpdatePresentation(bPresentVision, Target->IsDirectlyVisible());
    }
    if (MaterialInstance)
    {
        MaterialInstance->SetScalarParameterValue(TEXT("TimeSeconds"), World->GetTimeSeconds());
        float SceneFade = 1.f;
        for (const IInterface_PostProcessVolume& PostProcess : World->GetPostProcessVolumeIterator())
        {
            if (&PostProcess == static_cast<IInterface_PostProcessVolume*>(Volume.Get())) continue;
            const FPostProcessVolumeProperties Properties = PostProcess.GetProperties();
            if (!Properties.bIsEnabled || !Properties.bIsUnbound || !Properties.Settings || !Properties.Settings->bOverride_ColorGain) continue;
            const FVector4 Gain = Properties.Settings->ColorGain;
            const float MaximumChannel = FMath::Max3(Gain.X, Gain.Y, Gain.Z) * Gain.W;
            if (FMath::IsFinite(MaximumChannel))
                SceneFade = FMath::Min(SceneFade, FMath::Lerp(1.f, FMath::Clamp(MaximumChannel, 0.f, 1.f), FMath::Clamp(Properties.BlendWeight, 0.f, 1.f)));
        }
        MaterialInstance->SetScalarParameterValue(TEXT("PresentationFade"), SceneFade * PresentationFade);
    }
    UpdateAccumulator += DeltaTime;
    if (UpdateAccumulator < UpdateInterval) return;
    const float UpdateDelta = FMath::Min(UpdateAccumulator, .5f);
    UpdateAccumulator = 0.f;
    UpdatePerception(Observer, UpdateDelta);
}

bool UYogVisionSubsystem::CreateRenderObjects()
{
    if (bMaterialLoadAttempted) return false;
    bMaterialLoadAttempted = true;
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_DedicatedServer) return false;
    UMaterialInterface* Material = Cast<UMaterialInterface>(GetDefault<UYogVisionSettings>()->PostProcessMaterial.TryLoad());
    if (!Material || Material->GetMaterial()->MaterialDomain != MD_PostProcess) return false;
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
#if WITH_EDITOR
    Spawn.bTemporaryEditorActor = true;
    Spawn.bHideFromSceneOutliner = true;
    Spawn.bCreateActorPackage = false;
#endif
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Volume = World->SpawnActor<APostProcessVolume>(Spawn);
    if (!Volume) return false;
    Volume->bUnbound = true;
    Volume->Priority = 50.f;
    Volume->BlendWeight = 0.f;
#if WITH_EDITOR
    Volume->SetActorLabel(TEXT("YOG 视野迷雾（临时预览）"), false);
#endif
    MaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
    VisibilityTexture = UTexture2D::CreateTransient(AngularSamples, MaxSources, PF_B8G8R8A8);
    if (!MaterialInstance || !VisibilityTexture)
    {
        RemoveRenderObjects();
        return false;
    }
    MaterialInstance->SetFlags(RF_Transient);
    VisibilityTexture->SRGB = false;
    VisibilityTexture->Filter = TF_Nearest;
    VisibilityTexture->AddressX = TA_Wrap;
    VisibilityTexture->AddressY = TA_Clamp;
    VisibilityTexture->NeverStream = true;
    VisibilityTexture->UpdateResource();
    MaterialInstance->SetTextureParameterValue(TEXT("VisibilityTexture"), VisibilityTexture);
    Volume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, MaterialInstance));
    PushParameters(Parameters);
    return true;
}

void UYogVisionSubsystem::RemoveRenderObjects()
{
    if (IsValid(Volume))
    {
        Volume->Settings.WeightedBlendables.Array.Reset();
        Volume->Destroy();
    }
    Volume = nullptr;
    MaterialInstance = nullptr;
    VisibilityTexture = nullptr;
    ActiveSourceCount = 0;
    ActiveEchoCount = 0;
    for (const auto& Target : Targets) if (Target.IsValid())
    {
        Target->SetPerception(false, false);
        Target->RestorePresentation();
    }
}

void UYogVisionSubsystem::SetEnabled(bool bNewEnabled)
{
    if (bEnabled == bNewEnabled) return;
    bEnabled = bNewEnabled;
    UpdateAccumulator = UpdateInterval;
    bMaterialLoadAttempted = false;
    if (!bEnabled) RemoveRenderObjects();
}

void UYogVisionSubsystem::SetPresentationFade(float Fade)
{
    PresentationFade = FMath::IsFinite(Fade) ? FMath::Clamp(Fade, 0.f, 1.f) : 1.f;
}

void UYogVisionSubsystem::ApplyParameters(FYogVisionParameters NewParameters)
{
    NewParameters.Sanitize();
    Parameters = NewParameters;
    PushParameters(Parameters);
    UpdateAccumulator = UpdateInterval;
    // Assets may have been generated while the workbench was open.
    if (!MaterialInstance) bMaterialLoadAttempted = false;
}

void UYogVisionSubsystem::SetPreviewObserver(AActor* Observer)
{
    if (!bHasPreviewSnapshot)
    {
        SavedPreviewParameters = Parameters;
        SavedPreviewObserver = PreviewObserver;
        bSavedEnabled = bEnabled;
        bHasPreviewSnapshot = true;
    }
    PreviewObserver = Observer;
    UpdateAccumulator = UpdateInterval;
}

void UYogVisionSubsystem::ClearPreview()
{
    if (!bHasPreviewSnapshot) return;
    PreviewObserver = SavedPreviewObserver;
    Parameters = SavedPreviewParameters;
    const bool bRestoreEnabled = bSavedEnabled;
    bHasPreviewSnapshot = false;
    SavedPreviewObserver.Reset();
    SetEnabled(false);
    ApplyParameters(Parameters);
    SetEnabled(bRestoreEnabled);
}

AActor* UYogVisionSubsystem::ResolveObserver() const
{
    if (AActor* Actor = PreviewObserver.Get()) return Actor;
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return nullptr;
    // First local player only. Split-screen needs separate view-family resources.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        if (APlayerController* PC = It->Get(); PC && PC->IsLocalController()) return PC->GetPawn();
    return nullptr;
}

void UYogVisionSubsystem::RegisterSource(UYogVisionSourceComponent* Source)
{
    if (IsValid(Source)) Sources.Add(Source);
}

void UYogVisionSubsystem::UnregisterSource(UYogVisionSourceComponent* Source)
{
    Sources.Remove(Source);
    SmoothedSourceRadii.Remove(Source);
}

void UYogVisionSubsystem::RegisterTarget(UYogVisionTargetComponent* Target)
{
    if (IsValid(Target)) Targets.Add(Target);
}

void UYogVisionSubsystem::UnregisterTarget(UYogVisionTargetComponent* Target)
{
    Targets.Remove(Target);
    PreviousTargetPositions.Remove(Target);
}

bool UYogVisionSubsystem::HasLineOfSight(const FVisionSource& Source, const FVector& Target, AActor* TargetActor) const
{
    if (Source.Strength <= 0.f || Source.Radius <= 0.f || FVector::DistSquared2D(Source.Center, Target) > FMath::Square(Source.Radius)) return false;
    if (FMath::Abs(Source.Center.Z - Target.Z) >= 600.f) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(YogVisionTargetLOS), true);
    IgnoreActorTree(Query, Source.Owner.Get());
    IgnoreActorTree(Query, ResolveObserver());
    // Match the fan: bodies and carried equipment are perception targets, not walls.
    for (const auto& Target : Targets) if (Target.IsValid()) IgnoreActorTree(Query, Target->GetOwner());
    if (TargetActor)
    {
        TArray<AActor*> Children;
        TargetActor->GetAllChildActors(Children, true);
        Query.AddIgnoredActors(Children);
    }
    FHitResult Hit;
    const FVector Start = Source.Center + FVector(0, 0, Source.TraceHeight);
    const FVector End = Target + FVector(0, 0, Source.TraceHeight);
    return !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query) || Hit.GetActor() == TargetActor;
}

void UYogVisionSubsystem::UpdatePerception(AActor* Observer, float UpdateDelta)
{
    if (!IsValid(Observer) || Observer->IsActorBeingDestroyed() || Observer->IsHidden())
    {
        ActiveSourceCount = ActiveEchoCount = 0;
        if (MaterialInstance)
        {
            MaterialInstance->SetScalarParameterValue(TEXT("SourceCount"), 0.f);
            MaterialInstance->SetScalarParameterValue(TEXT("EchoCount"), 0.f);
            // A game without a valid local pawn should never be covered by a stale full-screen mask.
            Volume->BlendWeight = 0.f;
        }
        for (const auto& Target : Targets) if (Target.IsValid())
        {
            Target->SetPerception(false, false);
            Target->RestorePresentation();
        }
        return;
    }
    FYogVisionParameters Effective = Parameters;
    float ObserverHeight = 70.f;
    if (const UYogVisionObserverComponent* ObserverComponent = Observer->FindComponentByClass<UYogVisionObserverComponent>())
    {
        if (!ObserverComponent->bVisionEnabled)
        {
            if (Volume) Volume->BlendWeight = 0.f;
            for (const auto& Target : Targets) if (Target.IsValid())
            {
                Target->SetPerception(false, false);
                Target->RestorePresentation();
            }
            return;
        }
        Effective = ObserverComponent->ResolveParameters(Parameters);
        ObserverHeight = SafeHeight(ObserverComponent->EyeHeight);
    }
    if (Volume) Volume->BlendWeight = 1.f;
    SmoothedSightRadius = FMath::FInterpTo(SmoothedSightRadius, Effective.BaseSightRadius, UpdateDelta, 8.f);
    SmoothedSpiritStrength = FMath::FInterpTo(SmoothedSpiritStrength, Effective.SpiritStrength, UpdateDelta, 8.f);
    Effective.BaseSightRadius = SmoothedSightRadius;
    Effective.SpiritStrength = SmoothedSpiritStrength;
    PushParameters(Effective);
    if (MaterialInstance) MaterialInstance->SetVectorParameterValue(TEXT("Observer"), FLinearColor(Observer->GetActorLocation().X, Observer->GetActorLocation().Y, Observer->GetActorLocation().Z, Effective.BaseSightRadius));

    TArray<FVisionSource> FrameSources;
    FVisionSource PlayerSource;
    PlayerSource.Center = Observer->GetActorLocation();
    PlayerSource.Radius = Effective.BaseSightRadius;
    PlayerSource.TraceHeight = ObserverHeight;
    PlayerSource.Owner = Observer;
    FrameSources.Add(PlayerSource);

    TArray<UYogVisionSourceComponent*> Candidates;
    for (auto It = Sources.CreateIterator(); It; ++It)
    {
        UYogVisionSourceComponent* Source = It->Get();
        if (!Source) { It.RemoveCurrent(); continue; }
        if (Source->GetEffectiveRadius() > 0.f) Candidates.Add(Source);
    }
    Candidates.Sort([Observer](const UYogVisionSourceComponent& A, const UYogVisionSourceComponent& B)
    {
        return FVector::DistSquared2D(A.GetComponentLocation(), Observer->GetActorLocation()) < FVector::DistSquared2D(B.GetComponentLocation(), Observer->GetActorLocation());
    });
    for (UYogVisionSourceComponent* Component : Candidates)
    {
        if (FrameSources.Num() >= MaxSources) break;
        const float DesiredRadius = Component->GetEffectiveRadius();
        float& Radius = SmoothedSourceRadii.FindOrAdd(Component, DesiredRadius);
        Radius = FMath::FInterpTo(Radius, DesiredRadius, UpdateDelta, 8.f);
        FVisionSource Source;
        Source.Center = Component->GetComponentLocation();
        Source.Radius = Radius;
        Source.Strength = Component->GetEffectiveStrength();
        Source.TraceHeight = SafeHeight(Component->TraceHeight);
        Source.Owner = Component->GetOwner();
        FrameSources.Add(Source);
    }
    ActiveSourceCount = FrameSources.Num();
    TArray<FColor> Pixels;
    Pixels.Init(FColor::Black, AngularSamples * MaxSources);
    for (int32 SourceIndex = 0; SourceIndex < MaxSources; ++SourceIndex)
    {
        if (MaterialInstance)
        {
            const FVisionSource Source = FrameSources.IsValidIndex(SourceIndex) ? FrameSources[SourceIndex] : FVisionSource();
            MaterialInstance->SetVectorParameterValue(FName(*FString::Printf(TEXT("Source%d"), SourceIndex)), FLinearColor(Source.Center.X, Source.Center.Y, Source.Center.Z, Source.Radius));
            MaterialInstance->SetScalarParameterValue(FName(*FString::Printf(TEXT("SourceStrength%d"), SourceIndex)), Source.Strength);
        }
        if (!FrameSources.IsValidIndex(SourceIndex)) continue;
        const FVisionSource& Source = FrameSources[SourceIndex];
        FCollisionQueryParams Query(SCENE_QUERY_STAT(YogVisionAngularLOS), true);
        IgnoreActorTree(Query, Source.Owner.Get());
        IgnoreActorTree(Query, Observer);
        // Bodies are perception targets, not walls. Their own capsule must not cut sight at their center.
        for (const auto& Target : Targets) if (Target.IsValid()) IgnoreActorTree(Query, Target->GetOwner());
        const FVector Start = Source.Center + FVector(0, 0, Source.TraceHeight);
        for (int32 Angle = 0; Angle < AngularSamples; ++Angle)
        {
            const float Theta = 2.f * PI * Angle / AngularSamples;
            const FVector End = Start + FVector(FMath::Cos(Theta), FMath::Sin(Theta), 0) * Source.Radius;
            FHitResult Hit;
            const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query);
            const float Ratio = bHit ? FMath::Clamp((Hit.Distance - 3.f) / FMath::Max(Source.Radius, 1.f), 0.f, 1.f) : 1.f;
            const uint8 Value = static_cast<uint8>(FMath::RoundToInt(Ratio * 255.f));
            Pixels[SourceIndex * AngularSamples + Angle] = FColor(Value, Value, Value, 255);
        }
    }
    UploadVisibility(Pixels);

    TArray<UYogVisionTargetComponent*> SpiritTargets;
    for (auto It = Targets.CreateIterator(); It; ++It)
    {
        UYogVisionTargetComponent* Target = It->Get();
        if (!Target) { It.RemoveCurrent(); continue; }
        AActor* Actor = Target->GetOwner();
        if (!IsValid(Actor) || !Target->IsPresentationEligible() || !Target->IsActive())
        {
            Target->SetPerception(false, false);
            Target->RestorePresentation();
            PreviousTargetPositions.Remove(Target);
            continue;
        }
        bool bDirect = false;
        for (int32 SourceIndex = 0; SourceIndex < FrameSources.Num(); ++SourceIndex)
        {
            const FVisionSource& Source = FrameSources[SourceIndex];
            const FVector Difference = Actor->GetActorLocation() - Source.Center;
            float Angle = FMath::Atan2(Difference.Y, Difference.X);
            if (Angle < 0.f) Angle += 2.f * PI;
            const int32 Sample = FMath::FloorToInt(Angle / (2.f * PI) * AngularSamples) % AngularSamples;
            const int32 Next = (Sample + 1) % AngularSamples;
            const float WallCut = FMath::Min(Pixels[SourceIndex * AngularSamples + Sample].R, Pixels[SourceIndex * AngularSamples + Next].R) / 255.f;
            // Direct sight requires at least 50% reveal under the worst shader retreat.
            // Animated noise never grants gameplay authorization. Height and smoothstep match VisionFog.hlsl.
            const float Width = FMath::Max(Effective.BoundaryWidth, 1.f);
            const float Transition = FMath::Clamp((Source.Radius * WallCut - Difference.Size2D() - Width * Effective.BoundaryNoise) / Width, 0.f, 1.f);
            const float HeightTransition = FMath::Clamp((FMath::Abs(Difference.Z) - 260.f) / 340.f, 0.f, 1.f);
            const float RevealLowerBound = Transition * Transition * (3.f - 2.f * Transition)
                * (1.f - HeightTransition * HeightTransition * (3.f - 2.f * HeightTransition)) * Source.Strength;
            if (RevealLowerBound >= .5f && HasLineOfSight(Source, Actor->GetActorLocation(), Actor))
            {
                bDirect = true;
                break;
            }
        }
        const bool bSpirit = !bDirect && Effective.SpiritStrength > 0.f && Target->SpiritIntensity > 0.f
            && FVector::DistSquared2D(Observer->GetActorLocation(), Actor->GetActorLocation()) <= FMath::Square(Effective.SpiritRadius);
        Target->SetPerception(bDirect, bSpirit);
        if (GetWorld()->GetNetMode() != NM_DedicatedServer) Target->UpdatePresentation(IsPresentationActive(), bDirect);
        if (bSpirit) SpiritTargets.Add(Target);
    }
    SpiritTargets.Sort([Observer](const UYogVisionTargetComponent& A, const UYogVisionTargetComponent& B)
    {
        return FVector::DistSquared2D(A.GetOwner()->GetActorLocation(), Observer->GetActorLocation()) < FVector::DistSquared2D(B.GetOwner()->GetActorLocation(), Observer->GetActorLocation());
    });
    ActiveEchoCount = FMath::Min(MaxEchoes, SpiritTargets.Num());
    for (int32 EchoIndex = 0; EchoIndex < MaxEchoes; ++EchoIndex)
    {
        FLinearColor Echo = FLinearColor::Black;
        FLinearColor Motion = FLinearColor::Black;
        if (SpiritTargets.IsValidIndex(EchoIndex))
        {
            UYogVisionTargetComponent* Target = SpiritTargets[EchoIndex];
            AActor* Actor = Target->GetOwner();
            const FVector Position = Actor->GetActorLocation();
            FVector Velocity = Actor->GetVelocity();
            if (const FVector* Previous = PreviousTargetPositions.Find(Target))
                if (Velocity.IsNearlyZero()) Velocity = (Position - *Previous) / FMath::Max(UpdateDelta, .01f);
            PreviousTargetPositions.Add(Target, Position);
            const float Radius = FMath::IsFinite(Target->SpiritEchoRadius) ? FMath::Clamp(Target->SpiritEchoRadius, 1.f, 1000.f) : 90.f;
            const float Intensity = FMath::IsFinite(Target->SpiritIntensity) ? FMath::Clamp(Target->SpiritIntensity, 0.f, 2.f) : 0.f;
            Echo = FLinearColor(Position.X, Position.Y, Position.Z, Effective.SpiritStrength * Intensity);
            Motion = FLinearColor(Velocity.X, Velocity.Y, Radius, FMath::Clamp(Velocity.Size2D() / 600.f, 0.f, 1.f));
        }
        if (MaterialInstance)
        {
            MaterialInstance->SetVectorParameterValue(FName(*FString::Printf(TEXT("Echo%d"), EchoIndex)), Echo);
            MaterialInstance->SetVectorParameterValue(FName(*FString::Printf(TEXT("EchoVelocity%d"), EchoIndex)), Motion);
        }
    }
    if (MaterialInstance)
    {
        MaterialInstance->SetScalarParameterValue(TEXT("SourceCount"), ActiveSourceCount);
        MaterialInstance->SetScalarParameterValue(TEXT("EchoCount"), ActiveEchoCount);
    }
}

void UYogVisionSubsystem::UploadVisibility(const TArray<FColor>& Pixels)
{
    if (!VisibilityTexture || !VisibilityTexture->GetResource()) return;
    uint8* Data = static_cast<uint8*>(FMemory::Malloc(Pixels.Num() * sizeof(FColor)));
    FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, AngularSamples, MaxSources);
    VisibilityTexture->UpdateTextureRegions(0, 1, Region, AngularSamples * sizeof(FColor), sizeof(FColor), Data,
        [](uint8* Bytes, const FUpdateTextureRegion2D* UpdateRegion) { FMemory::Free(Bytes); delete UpdateRegion; });
}

void UYogVisionSubsystem::PushParameters(const FYogVisionParameters& Effective)
{
    if (!MaterialInstance) return;
#define YOG_VISION_FLOAT(Name) MaterialInstance->SetScalarParameterValue(TEXT(#Name), Effective.Name)
    YOG_VISION_FLOAT(BaseSightRadius);
    YOG_VISION_FLOAT(SpiritRadius);
    YOG_VISION_FLOAT(SpiritStrength);
    YOG_VISION_FLOAT(FogDensity);
    YOG_VISION_FLOAT(FogContrast);
    YOG_VISION_FLOAT(FogBrightness);
    YOG_VISION_FLOAT(DistanceFogStart);
    YOG_VISION_FLOAT(DistanceFogEnd);
    YOG_VISION_FLOAT(DistanceFogDensity);
    YOG_VISION_FLOAT(SpiritDisturbance);
    YOG_VISION_FLOAT(SpiritVolumeHeight);
    YOG_VISION_FLOAT(SpiritTrailLength);
    YOG_VISION_FLOAT(DarkThreshold);
    YOG_VISION_FLOAT(DarkFogStrength);
    YOG_VISION_FLOAT(EdgeStrength);
    YOG_VISION_FLOAT(StrokeStrength);
    YOG_VISION_FLOAT(StrokeScale);
    YOG_VISION_FLOAT(FlowSpeed);
    YOG_VISION_FLOAT(BoundaryWidth);
    YOG_VISION_FLOAT(BoundaryNoise);
    YOG_VISION_FLOAT(PlayerLift);
#undef YOG_VISION_FLOAT
    MaterialInstance->SetVectorParameterValue(TEXT("FogColor"), Effective.FogColor);
    MaterialInstance->SetVectorParameterValue(TEXT("SpiritColor"), Effective.SpiritColor);
}

bool UYogVisionSubsystem::IsDirectlyVisible(AActor* Actor) const
{
    const UYogVisionTargetComponent* Target = IsValid(Actor) ? Actor->FindComponentByClass<UYogVisionTargetComponent>() : nullptr;
    return bEnabled && Target && Target->bTargetEnabled && !Actor->IsHidden() && !Actor->IsActorBeingDestroyed() && Target->IsDirectlyVisible();
}

bool UYogVisionSubsystem::IsPresentationActive() const
{
    return bEnabled && IsValid(MaterialInstance) && IsValid(Volume) && Volume->bEnabled && Volume->BlendWeight > 0.f;
}

bool UYogVisionSubsystem::IsSpiritSensed(AActor* Actor) const
{
    const UYogVisionTargetComponent* Target = IsValid(Actor) ? Actor->FindComponentByClass<UYogVisionTargetComponent>() : nullptr;
    return bEnabled && Target && Target->bTargetEnabled && !Actor->IsHidden() && !Actor->IsActorBeingDestroyed() && Target->IsSpiritSensed();
}

FString UYogVisionSubsystem::GetStatusText() const
{
    if (!bEnabled) return TEXT("视野迷雾已关闭；现有场景后处理保持原样。");
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer) return TEXT("专用服务器：不创建后处理渲染对象。");
    if (bMaterialLoadAttempted && !MaterialInstance) return TEXT("缺少有效的视野后处理材质，请先生成基础资产。");
    if (!ResolveObserver()) return TEXT("已启用，等待本地玩家或工作台预览观察者。");
    return FString::Printf(TEXT("视野迷雾运行中｜视野源 %d/4｜灵体 %d/8"), ActiveSourceCount, ActiveEchoCount);
}
