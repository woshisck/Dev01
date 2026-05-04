#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRuneGroundPathEffect.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

UCLASS(DisplayName = "Spawn Rune Ground Path Effect")
class DEVKIT_API UBFNode_SpawnRuneGroundPathEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	ERuneGroundPathTargetPolicy TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Fallback facing mode when Spawn Location Override / Spawn Rotation Override are not linked. Prefer using Calc Rune Ground Path Transform for authored flows."))
	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Optional data pin. If linked, this exact world location is used instead of the internal position calculation."))
	FFlowDataPinInputProperty_Vector SpawnLocationOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Optional data pin. If linked, this exact world rotation is used instead of the internal facing calculation."))
	FFlowDataPinInputProperty_Rotator SpawnRotationOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
	bool bCenterOnPathLength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
	float RotationYawOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Length = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Width = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
	float Height = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1.0", ToolTip = "Decal projection depth in cm. Keep this shallow so the path decal stays on the floor instead of projecting up onto characters."))
	float DecalProjectionDepth = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ToolTip = "Rotates only the decal texture/mask on the floor. This does not rotate the damage/collision area. Try 0/90/180/270 if the decal visual direction does not match the yellow debug area."))
	float DecalPlaneRotationDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
	FVector NiagaraScale = FVector(0.5f, 0.5f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1", ClampMax = "12", ToolTip = "Number of Niagara instances distributed along the path. Fire paths use multiple small instances to read as a ground strip."))
	int32 NiagaraInstanceCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Tag 1 (SetByCaller)", ToolTip = "GameplayTag used by the GameplayEffect execution. Burn paths should use Data.Damage.Burn."))
	FGameplayTag SetByCallerTag1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Value 1 / Tick", ToolTip = "Designer-facing damage value passed to SetByCallerTag1. For UGE_RuneBurn this is the burn damage per periodic tick."))
	FFlowDataPinInputProperty_Float SetByCallerValue1 = FFlowDataPinInputProperty_Float(0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	FGameplayTag SetByCallerTag2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	FFlowDataPinInputProperty_Float SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (ClampMin = "1"))
	int32 ApplicationCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
	bool bApplyOncePerTarget = false;

	virtual void ExecuteInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
