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

	// 追踪目标 — 监测哪个 Actor 的移动距离
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (DisplayName = "追踪目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// 每次触发距离 — 目标累计移动此距离后触发 Out 引脚（cm），可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (DisplayName = "每次触发距离（cm）"))
	FFlowDataPinInputProperty_Float DistancePerTrigger;

	// 检测间隔（秒）— 每隔多少秒检测一次目标位移
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.05", DisplayName = "检测间隔（秒）"))
	float TickInterval = 0.2f;

	// 静止超时（秒）— 目标静止超过此时间后触发 Timeout 引脚（0 = 不启用）
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0", DisplayName = "静止超时（秒）"))
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
