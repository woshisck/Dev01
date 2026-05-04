#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rune512FlipbookVFXActor.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;

UCLASS(BlueprintType)
class DEVKIT_API ARune512FlipbookVFXActor : public AActor
{
	GENERATED_BODY()

public:
	ARune512FlipbookVFXActor();

	void InitializeFlipbook(
		UTexture2D* InTexture,
		UMaterialInterface* InMaterial,
		UStaticMesh* InPlaneMesh,
		int32 InRows,
		int32 InColumns,
		float InDuration,
		float InLifetime,
		bool bInLoop,
		float InSize,
		bool bInFaceCamera,
		const FLinearColor& InEmissiveColor,
		float InAlphaScale);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlaneComponent;

private:
	void UpdateMaterialTime();
	void FaceCamera();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	float StartWorldTime = 0.f;
	float Duration = 0.45f;
	float Lifetime = 0.45f;
	bool bLoop = false;
	bool bFaceCamera = true;
};
