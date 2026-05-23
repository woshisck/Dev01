#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Water/WaterInteractionTypes.h"
#include "CheapWaterSurfaceActor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class DEVKIT_API ACheapWaterSurfaceActor : public AActor
{
	GENERATED_BODY()

public:
	ACheapWaterSurfaceActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Water")
	void ApplyWaterVisualTuning();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UStaticMeshComponent> WaterSurface;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	FWaterVisualTuning VisualTuning;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;

	void EnsureMaterialInstance();
};
