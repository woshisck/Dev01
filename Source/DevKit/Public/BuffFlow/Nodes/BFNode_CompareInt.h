#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"  // 复用 EBFCompareOp
#include "BFNode_CompareInt.generated.h"

/**
 * 整数比较节点，专门用于比较 Int32 数据引脚（如 GetRuneInfo.StackCount）。
 * 用法与 CompareFloat 相同，但 A/B 均为 Int32，避免类型不兼容报错。
 *
 * 典型用法（判断 StackCount >= 5）：
 *   GetRuneInfo → StackCount → CompareInt.A
 *                              CompareInt.B = 5, Op = >=
 *                              → True → AddEffect(...)
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Int", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_CompareInt : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 左操作数，可接受 GetRuneInfo.StackCount 等 Int32 数据引脚 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Int32 A;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFCompareOp Operator = EBFCompareOp::GreaterOrEqual;

	/** 右操作数，通常直接填写阈值 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Int32 B;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};