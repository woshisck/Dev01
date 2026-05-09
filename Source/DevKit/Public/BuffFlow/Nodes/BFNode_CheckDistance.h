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

	// 目标 — 检测 BuffOwner 到哪个 Actor 的距离
	UPROPERTY(EditAnywhere, Category = "Distance", meta = (DisplayName = "目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// 所需距离（cm）— 达到此距离时触发 Out 引脚；可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "Distance", meta = (DisplayName = "所需距离（cm）"))
	FFlowDataPinInputProperty_Float RequiredDistance;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	TWeakObjectPtr<AActor> TrackedActor;
	FVector SavedOrigin = FVector::ZeroVector;
};
