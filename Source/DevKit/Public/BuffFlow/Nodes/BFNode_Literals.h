#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_Literals.generated.h"

/**
 * 字面量 Float —— 纯数据节点，无执行引脚。
 *
 * 用途：将固定浮点值作为数据引脚输出，供下游节点（如 ApplyAttributeModifier.Value）连线读取。
 * 等价于 Blueprint 的 "Make Literal Float"。
 *
 * 使用方式：
 *   1. 放在 FA 图里（不需要连接执行流）
 *   2. 将 Value 输出引脚连接到下游节点的 Float 输入引脚
 *   3. 在 Details 面板里填写固定数值
 *
 * ⚠️ 注意：此节点没有执行引脚，不参与执行流。纯数据驱动。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Literal Float", Category = "BuffFlow|Utility"))
class DEVKIT_API UBFNode_LiteralFloat : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	/** 输出的浮点字面量（其他节点的 Float 数据引脚可连线到此） */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (DisplayName = "Value"))
	FFlowDataPinOutputProperty_Float Value;
};

/**
 * 字面量 Int —— 纯数据节点，无执行引脚。
 * 等价于 Blueprint 的 "Make Literal Int"。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Literal Int", Category = "BuffFlow|Utility"))
class DEVKIT_API UBFNode_LiteralInt : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	/** 输出的整数字面量 */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (DisplayName = "Value"))
	FFlowDataPinOutputProperty_Int32 Value;
};

/**
 * 字面量 Bool —— 纯数据节点，无执行引脚。
 * 等价于 Blueprint 的 "Make Literal Bool"。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Literal Bool", Category = "BuffFlow|Utility"))
class DEVKIT_API UBFNode_LiteralBool : public UFlowNode
{
	GENERATED_UCLASS_BODY()

	/** 输出的布尔字面量 */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (DisplayName = "Value"))
	FFlowDataPinOutputProperty_Bool bValue;
};
