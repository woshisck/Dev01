#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PlayFlipbookVFX.generated.h"

class ARune512FlipbookVFXActor;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Flipbook VFX", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayFlipbookVFX : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TObjectPtr<UTexture2D> Texture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TObjectPtr<UStaticMesh> PlaneMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1"))
	int32 Rows = 4;

	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1"))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.01"))
	float Duration = 0.45f;

	/** Actor lifetime. 0 uses Duration. Use a longer lifetime with bLoop for status effects. */
	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.0"))
	float Lifetime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "1.0"))
	float Size = 80.f;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	FName Socket = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	TArray<FName> SocketFallbackNames;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	FVector Offset = FVector::ZeroVector;

	/** Moves the spawned sprite from the socket/bone center toward the camera-facing surface of the target. */
	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface")
	bool bProjectToVisibleSurface = false;

	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "0.0"))
	float SurfaceOffset = 6.f;

	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "0.0"))
	float SurfaceFallbackRadiusScale = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Flipbook|Surface", meta = (ClampMin = "1.0"))
	float SurfaceTraceExtraDistance = 120.f;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	bool bFaceCamera = true;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	bool bDestroyWithFlow = false;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	FLinearColor EmissiveColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Flipbook", meta = (ClampMin = "0.0"))
	float AlphaScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "Flipbook")
	FName EffectName = NAME_None;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FVector ProjectToVisibleSurface(AActor* TargetActor, USceneComponent* AttachComp, const FVector& BaseLocation) const;

	UPROPERTY()
	TArray<TObjectPtr<ARune512FlipbookVFXActor>> ActiveActors;
};
