#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_DoOnce.generated.h"

/**
 * Allows execution to pass through only once.
 * Subsequent triggers are ignored until the "Reset" input is called.
 * Mirrors Unreal's Blueprint "Do Once" node.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Do Once", Category = "BuffFlow|FlowControl"))
class DEVKIT_API UBFNode_DoOnce : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** If true, the node starts already triggered and won't fire until reset. */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	bool bStartClosed = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	bool bHasTriggered = false;
};
