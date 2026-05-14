#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_Probability.generated.h"

/**
 * 概率节点：按设定概率分发执行流。
 *
 * Chance 为 0.0～1.0（0.75 = 75%），可接受数据引脚连线动态驱动。
 * 例如：有护甲时将 Chance 从 0.75 乘以 0.75 得到 0.5625，连入此节点。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Probability", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_Probability : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 触发概率 0.0~1.0（0.75 = 75%），可接数据引脚驱动 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow",
		meta = (ClampMin = "0.0", ClampMax = "1.0", DisplayName = "Chance (0~1)"))
	FFlowDataPinInputProperty_Float Chance;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
