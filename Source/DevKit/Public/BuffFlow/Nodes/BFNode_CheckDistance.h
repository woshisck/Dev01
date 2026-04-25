#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_CheckDistance.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Check Distance", Category = "BuffFlow|Utility"))
class DEVKIT_API UBFNode_CheckDistance : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Distance")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	UPROPERTY(EditAnywhere, Category = "Distance")
	FFlowDataPinInputProperty_Float RequiredDistance;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	TWeakObjectPtr<AActor> TrackedActor;
	FVector SavedOrigin = FVector::ZeroVector;
};
