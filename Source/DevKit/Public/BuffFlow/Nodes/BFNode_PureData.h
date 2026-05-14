#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PureData.generated.h"

/**
 * Pure data node base — no execution pins.
 * Subclasses supply data values via IFlowDataPinValueSupplierInterface.
 * Use OutputPins with non-Exec PinType to declare data output pins.
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API UBFNode_PureData : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override {}
};
