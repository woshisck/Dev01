#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_IfStatement.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "If Statement", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_IfStatement : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Bool Condition;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
