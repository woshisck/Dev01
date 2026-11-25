#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogVisionMotionProbe.generated.h"

/** An explicit review fixture; it is never spawned by the vision gameplay subsystem. */
UCLASS()
class YOGVISIONRUNTIME_API AYogVisionMotionProbe : public AActor
{
    GENERATED_BODY()
public:
    AYogVisionMotionProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    UPROPERTY(VisibleAnywhere, Category="Review") TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, Category="Review") TObjectPtr<class UYogVisionTargetComponent> VisionTarget;
    UPROPERTY(EditAnywhere, Category="Review") float TravelDistance = 180.f;
    UPROPERTY(EditAnywhere, Category="Review") float TravelSpeed = .9f;
private:
    FVector Anchor;
    float Phase = 0.f;
    bool bHasAnchor = false;
};
