#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_CompareBool.generated.h"

UENUM(BlueprintType)
enum class EBFBoolCompareOp : uint8
{
	Equal UMETA(DisplayName = "=="),
	NotEqual UMETA(DisplayName = "!="),
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Bool", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_CompareBool : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "A"))
	FFlowDataPinInputProperty_Bool A;

	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "Operator"))
	EBFBoolCompareOp Operator = EBFBoolCompareOp::Equal;

	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "B"))
	FFlowDataPinInputProperty_Bool B;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
