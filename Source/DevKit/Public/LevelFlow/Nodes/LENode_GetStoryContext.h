#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "Types/FlowDataPinResults.h"
#include "LENode_GetStoryContext.generated.h"

class AStoryFlowProxy;

/**
 * Reads context captured by AStoryFlowProxy for story-owned LevelFlow assets.
 */
UCLASS(meta = (DisplayName = "Get Story Context"))
class DEVKIT_API ULENode_GetStoryContext : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool CanSupplyDataPinValues_Implementation() const override;
	virtual FFlowDataPinResult_Object TrySupplyDataPinAsObject_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Transform TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const override;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	const AStoryFlowProxy* GetStoryFlowProxyOwner() const;
};
