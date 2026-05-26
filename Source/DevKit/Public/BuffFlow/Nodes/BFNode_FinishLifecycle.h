#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_FinishLifecycle.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Finish Lifecycle", Category = "BuffFlow|Lifecycle"))
class DEVKIT_API UBFNode_FinishLifecycle : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Lifecycle")
	bool bFinishDyingOnDeathLifecycle = true;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
