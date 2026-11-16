#include "YogVisionSourceComponent.h"
#include "YogVisionSubsystem.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UYogVisionSourceComponent::UYogVisionSourceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UYogVisionSourceComponent::BindLight(ULocalLightComponent* Light)
{
    BoundLight = Light;
    bHadExplicitLightBinding = Light != nullptr;
}

ULocalLightComponent* UYogVisionSourceComponent::ResolveBoundLight() const
{
    if (bHadExplicitLightBinding)
    {
        UPointLightComponent* Point = Cast<UPointLightComponent>(BoundLight.Get());
        // USpotLightComponent inherits UPointLightComponent, but its cone is not a radial vision source.
        return Point && !Point->IsA<USpotLightComponent>() ? Point : nullptr;
    }
    if (bBindOwnerLight && GetOwner())
    {
        TInlineComponentArray<UPointLightComponent*> Lights;
        GetOwner()->GetComponents(Lights);
        for (UPointLightComponent* Point : Lights) if (Point && !Point->IsA<USpotLightComponent>()) return Point;
    }
    return nullptr;
}

float UYogVisionSourceComponent::GetEffectiveStrength() const
{
    const AActor* Owner = GetOwner();
    if (!bVisionEnabled || !IsActive() || !Owner || Owner->IsActorBeingDestroyed() || Owner->IsHidden()) return 0.f;
    if (bHadExplicitLightBinding || bBindOwnerLight)
    {
        const ULocalLightComponent* Light = ResolveBoundLight();
        if (!Light || !Light->IsVisible() || Light->bHiddenInGame || !Light->bAffectsWorld || !FMath::IsFinite(Light->Intensity) || Light->Intensity <= KINDA_SMALL_NUMBER) return 0.f;
        if (Light->GetOwner() && Light->GetOwner()->IsHidden()) return 0.f;
    }
    return FMath::IsFinite(Strength) ? FMath::Clamp(Strength, 0.f, 1.f) : 0.f;
}

float UYogVisionSourceComponent::GetEffectiveRadius() const
{
    float Result = FMath::IsFinite(Radius) ? FMath::Clamp(Radius, 0.f, 20000.f) : 0.f;
    if (const ULocalLightComponent* Light = ResolveBoundLight())
    {
        Result = FMath::Min(Result, FMath::IsFinite(Light->AttenuationRadius) ? FMath::Max(0.f, Light->AttenuationRadius) : 0.f);
    }
    return GetEffectiveStrength() > 0.f ? Result : 0.f;
}

void UYogVisionSourceComponent::OnRegister()
{
    Super::OnRegister();
    if (UWorld* World = GetWorld()) if (UYogVisionSubsystem* System = World->GetSubsystem<UYogVisionSubsystem>()) System->RegisterSource(this);
}

void UYogVisionSourceComponent::OnUnregister()
{
    if (UWorld* World = GetWorld()) if (UYogVisionSubsystem* System = World->GetSubsystem<UYogVisionSubsystem>()) System->UnregisterSource(this);
    Super::OnUnregister();
}
