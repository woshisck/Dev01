#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"  // 复用 EBFCompareOp
#include "BFNode_CompareInt.generated.h"

/**
 * 整数比较节点，专门用于比较 Int32 数据引脚（如 GetRuneInfo.StackCount）。
 * 用法与 CompareFloat 相同，但 A/B 均为 Int32，避免类型不兼容报错。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Int", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_CompareInt : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 左操作数 A — 可接受 GetRuneInfo.StackCount 等 Int32 数据引脚连线
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 A"))
	FFlowDataPinInputProperty_Int32 A;

	// 比较运算符 — 选择 A 和 B 的比较方式（>、>=、==、<=、<、!=）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "运算符"))
	EBFCompareOp Operator = EBFCompareOp::GreaterOrEqual;

	// 右操作数 B — 通常直接填写阈值，也可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 B"))
	FFlowDataPinInputProperty_Int32 B;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
