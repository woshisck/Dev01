#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_Fork.generated.h"

/**
 * Internal legacy fork node.
 *
 * Yog Rune exposes the FlowGraph native Sequence node for new fork authoring.
 * This class stays loadable for any already-created fork nodes.
 */
UCLASS(Abstract, NotBlueprintable, meta = (DisplayName = "Fork Internal", Category = "Internal"))
class DEVKIT_API UBFNode_Fork : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
