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

	// 左操作数 A — 可接受数据输出引脚（如 GetGEInfo.StackCount）连线
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 A"))
	FFlowDataPinInputProperty_Float A;

	// 比较运算符 — 选择 A 和 B 的比较方式（>、>=、==、<=、<、!=）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "运算符"))
	EBFCompareOp Operator = EBFCompareOp::GreaterOrEqual;

	// 右操作数 B — 通常在节点上直接填写阈值，也可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "操作数 B"))
	FFlowDataPinInputProperty_Float B;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
