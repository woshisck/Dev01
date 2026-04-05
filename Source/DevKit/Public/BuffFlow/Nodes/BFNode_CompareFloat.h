#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_CompareFloat.generated.h"

UENUM(BlueprintType)
enum class EBFCompareOp : uint8
{
	GreaterThan      UMETA(DisplayName = ">"),
	GreaterOrEqual   UMETA(DisplayName = ">="),
	Equal            UMETA(DisplayName = "=="),
	LessOrEqual      UMETA(DisplayName = "<="),
	LessThan         UMETA(DisplayName = "<"),
	NotEqual         UMETA(DisplayName = "!="),
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Float", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_CompareFloat : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 左操作数，可接受来自数据输出引脚（如 GetGEInfo.StackCount）的连接 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Float A;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFCompareOp Operator = EBFCompareOp::GreaterOrEqual;

	/** 右操作数，通常在节点上直接填写阈值 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Float B;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
