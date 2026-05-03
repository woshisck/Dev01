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
	UPROPERTY()
	TArray<TObjectPtr<ARune512FlipbookVFXActor>> ActiveActors;
};
