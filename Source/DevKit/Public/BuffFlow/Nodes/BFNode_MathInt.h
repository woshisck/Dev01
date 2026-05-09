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

	// 操作数 A — 整数输入，可连接上游数据引脚（如 GetRuneInfo.StackCount）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 A"))
	FFlowDataPinInputProperty_Int32 A;

	// 运算符 — 选择 A 和 B 之间的数学运算（+、-、×、÷）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "运算符"))
	EBFMathOp Operator = EBFMathOp::Add;

	// 操作数 B — 整数输入，通常直接填写固定值，也可连接数据引脚
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 B"))
	FFlowDataPinInputProperty_Int32 B;

	// 运算结果（数据输出引脚）— 可连线到 CompareInt / GrantGA.AbilityLevel 等
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "结果（输出）"))
	FFlowDataPinOutputProperty_Int32 Result;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
