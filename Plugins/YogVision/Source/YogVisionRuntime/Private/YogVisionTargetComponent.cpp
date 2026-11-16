#include "YogVisionTargetComponent.h"
#include "YogVisionSubsystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

namespace
{
void CollectPresentationComponents(const AActor* Owner, TArray<UPrimitiveComponent*>& Components)
{
    Owner->GetComponents(Components);
    TArray<AActor*> Children;
    Owner->GetAllChildActors(Children, true);
    TArray<AActor*> Attached;
    Owner->GetAttachedActors(Attached, true, true);
    for (AActor* Actor : Attached)
        if (IsValid(Actor) && Actor->IsOwnedBy(Owner)) Children.AddUnique(Actor);
    for (const AActor* Child : Children)
    {
        if (!IsValid(Child)) continue;
        TArray<UPrimitiveComponent*> ChildComponents;
        Child->GetComponents(ChildComponents);
        Components.Append(ChildComponents);
    }
}
}

UYogVisionTargetComponent::UYogVisionTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UYogVisionTargetComponent::SetTargetEnabled(bool bEnabled)
{
    bTargetEnabled = bEnabled;
    if (!bEnabled)
    {
        SetPerception(false, false);
        RestorePresentation();
    }
}

void UYogVisionTargetComponent::SetSpiritAppearance(float Radius, float Intensity)
{
    SpiritEchoRadius = FMath::IsFinite(Radius) ? FMath::Clamp(Radius, 1.f, 1000.f) : 90.f;
    SpiritIntensity = FMath::IsFinite(Intensity) ? FMath::Clamp(Intensity, 0.f, 2.f) : 0.f;
}

void UYogVisionTargetComponent::SetPerception(bool bDirect, bool bSpirit)
{
    bDirectlyVisible = bDirect;
    bSpiritSensed = bSpirit;
}

bool UYogVisionTargetComponent::IsPresentationEligible() const
{
    const AActor* Owner = GetOwner();
    if (!Owner || Owner->IsHidden() || Owner->IsActorBeingDestroyed() || !bTargetEnabled) return false;
    TArray<UPrimitiveComponent*> Meshes;
    CollectPresentationComponents(Owner, Meshes);
    // Non-mesh targets remain useful for designer-authored perception proxies.
    if (Meshes.IsEmpty()) return true;
    for (UPrimitiveComponent* Mesh : Meshes)
    {
        if (!IsValid(Mesh) || Mesh->bHiddenInGame) continue;
        const bool* Original = VisibilitySnapshot.Find(Mesh);
        if (Original ? *Original : Mesh->GetVisibleFlag()) return true;
    }
    return false;
}

void UYogVisionTargetComponent::UpdatePresentation(bool bVisionEnabled, bool bDirect)
{
    if (!bVisionEnabled || !bTargetEnabled || !bHideBodyOutsideSight || bDirect)
    {
        RestorePresentation();
        return;
    }
    AActor* Owner = GetOwner();
    if (!Owner || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    bPresentationHidden = true;
    TArray<UPrimitiveComponent*> Meshes;
    CollectPresentationComponents(Owner, Meshes);
    // Includes meshes, Niagara, particle effects and world-space widgets. Collision stays unchanged.
    for (auto It = VisibilitySnapshot.CreateIterator(); It; ++It)
    {
        UPrimitiveComponent* Component = It->Key.Get();
        if (!Component || !Meshes.Contains(Component))
        {
            if (Component) Component->SetVisibility(It->Value, false);
            It.RemoveCurrent();
        }
    }
    for (UPrimitiveComponent* Mesh : Meshes)
    {
        if (!IsValid(Mesh)) continue;
        if (!VisibilitySnapshot.Contains(Mesh)) VisibilitySnapshot.Add(Mesh, Mesh->GetVisibleFlag());
        Mesh->SetVisibility(false, false);
    }
}

void UYogVisionTargetComponent::RestorePresentation()
{
    for (const auto& Pair : VisibilitySnapshot)
        if (UPrimitiveComponent* Mesh = Pair.Key.Get()) Mesh->SetVisibility(Pair.Value, false);
    VisibilitySnapshot.Reset();
    bPresentationHidden = false;
}

void UYogVisionTargetComponent::ApplyRequestedVisibility(UPrimitiveComponent* Component, bool bRequestedVisible)
{
    if (!IsValid(Component)) return;
    if (bPresentationHidden)
    {
        VisibilitySnapshot.Add(Component, bRequestedVisible);
        Component->SetVisibility(false, false);
    }
    else Component->SetVisibility(bRequestedVisible, false);
}

void UYogVisionTargetComponent::OnRegister()
{
    Super::OnRegister();
    if (UWorld* World = GetWorld()) if (UYogVisionSubsystem* System = World->GetSubsystem<UYogVisionSubsystem>()) System->RegisterTarget(this);
}

void UYogVisionTargetComponent::OnUnregister()
{
    SetPerception(false, false);
    RestorePresentation();
    if (UWorld* World = GetWorld()) if (UYogVisionSubsystem* System = World->GetSubsystem<UYogVisionSubsystem>()) System->UnregisterTarget(this);
    Super::OnUnregister();
}
