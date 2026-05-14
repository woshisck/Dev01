#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_MathFloat.generated.h"

/**
 * 浮点数学运算节点（+  -  ×  ÷）
 *
 * A 和 B 均为数据引脚，可接受上游节点连线，也可直接填写固定值。
 * Result 为数据输出引脚，可连接到 CompareFloat / ApplyEffect.Level 等。
 * 除法时若 B == 0，Result 输出 0。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Math Float", Category = "BuffFlow|Math"))
class DEVKIT_API UBFNode_MathFloat : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 操作数 A — 浮点输入，可连接上游数据引脚（如 GetAttribute.CachedValue）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 A"))
	FFlowDataPinInputProperty_Float A;

	// 运算符 — 选择 A 和 B 之间的数学运算（+、-、×、÷）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "运算符"))
	EBFMathOp Operator = EBFMathOp::Add;

	// 操作数 B — 浮点输入，通常直接填写固定值，也可连接数据引脚
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 B"))
	FFlowDataPinInputProperty_Float B;

	// 运算结果（数据输出引脚）— 可连线到 CompareFloat / ApplyAttributeModifier.Value 等
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "结果（输出）"))
	FFlowDataPinOutputProperty_Float Result;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
