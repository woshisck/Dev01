#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_TrackMovement.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Track Movement", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_TrackMovement : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Movement")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	UPROPERTY(EditAnywhere, Category = "Movement")
	FFlowDataPinInputProperty_Float DistancePerTrigger;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.05"))
	float TickInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0"))
	float StationaryTimeout = 0.f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void OnTick();

	TWeakObjectPtr<AActor> TrackedActor;
	FVector LastPosition = FVector::ZeroVector;
	float AccumulatedDistance = 0.f;
	float StationaryTimer = 0.f;
	FTimerHandle TickTimerHandle;
};
