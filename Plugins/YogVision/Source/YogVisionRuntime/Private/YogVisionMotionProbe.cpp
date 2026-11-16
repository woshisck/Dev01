#include "YogVisionMotionProbe.h"
#include "YogVisionTargetComponent.h"
#include "Components/StaticMeshComponent.h"

AYogVisionMotionProbe::AYogVisionMotionProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReviewMesh")); SetRootComponent(Mesh);
    Mesh->SetMobility(EComponentMobility::Movable);
    VisionTarget = CreateDefaultSubobject<UYogVisionTargetComponent>(TEXT("VisionTarget"));
}
void AYogVisionMotionProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bHasAnchor) { Anchor = GetActorLocation(); bHasAnchor = true; }
    Phase += DeltaSeconds * TravelSpeed;
    FVector Before = GetActorLocation();
    FVector After = Anchor + FVector(0, FMath::Sin(Phase) * TravelDistance, 0);
    SetActorLocation(After);
    Mesh->ComponentVelocity = DeltaSeconds > KINDA_SMALL_NUMBER ? (After - Before) / DeltaSeconds : FVector::ZeroVector;
}
