#pragma once

#include "CoreMinimal.h"
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

UCLASS(NotBlueprintable, meta = (DisplayName = "浮点比较", Category = "BuffFlow|逻辑"))
class DEVKIT_API UBFNode_CompareFloat : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	float A = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFCompareOp Operator = EBFCompareOp::GreaterOrEqual;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	float B = 0.f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
