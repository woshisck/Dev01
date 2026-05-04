#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_CalcRuneGroundPathTransform.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Calc Rune Ground Path Transform", Category = "BuffFlow|Area"))
class DEVKIT_API UBFNode_CalcRuneGroundPathTransform : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Path length used only for center calculation. Link this to the same length as the spawn node when possible."))
	FFlowDataPinInputProperty_Float Length = FFlowDataPinInputProperty_Float(520.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Local offset from source actor. X is forward, Y is right, Z is up."))
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "When true, output location is the center of the path area. When false, output location is the path start/apex."))
	bool bCenterOnPathLength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
	float RotationYawOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground")
	bool bProjectToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground", meta = (ClampMin = "0.0"))
	float GroundTraceUp = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Ground", meta = (ClampMin = "0.0"))
	float GroundTraceDown = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data")
	FFlowDataPinOutputProperty_Vector SpawnLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data")
	FFlowDataPinOutputProperty_Rotator SpawnRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output|Data")
	FFlowDataPinOutputProperty_Vector ForwardVector;

	virtual void ExecuteInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
