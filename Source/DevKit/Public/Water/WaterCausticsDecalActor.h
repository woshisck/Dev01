#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterCausticsDecalActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable)
class DEVKIT_API AWaterCausticsDecalActor : public AActor
{
	GENERATED_BODY()

public:
	AWaterCausticsDecalActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Water")
	void SetCausticsIntensity(float NewIntensity);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UDecalComponent> CausticsDecal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UMaterialInterface> CausticsMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float CausticsIntensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water")
	FVector2D CausticsScrollSpeed = FVector2D(0.04f, 0.025f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water", meta = (ClampMin = "0.0"))
	float CausticsScale = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CausticsMID;

	float ElapsedTime = 0.0f;
};
