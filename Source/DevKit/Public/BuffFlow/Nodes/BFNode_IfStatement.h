#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_IfStatement.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "If Statement", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_IfStatement : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 条件（数据引脚）— 连接布尔类型数据引脚；true 走 True 引脚，false 走 False 引脚
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "条件"))
	FFlowDataPinInputProperty_Bool Condition;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
