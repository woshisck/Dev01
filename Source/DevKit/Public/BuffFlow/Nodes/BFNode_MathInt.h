#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_MathInt.generated.h"

/**
 * 整数数学运算节点（+  -  ×  ÷）
 *
 * A 和 B 均为数据引脚，可接受上游节点连线，也可直接填写固定值。
 * Result 为数据输出引脚，可连接到 CompareInt / GrantGA.AbilityLevel 等。
 * 除法时若 B == 0，Result 输出 0。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Math Int", Category = "BuffFlow|Math"))
class DEVKIT_API UBFNode_MathInt : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Int32 A;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFMathOp Operator = EBFMathOp::Add;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Int32 B;

	/** 运算结果（数据输出引脚） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Int32 Result;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
