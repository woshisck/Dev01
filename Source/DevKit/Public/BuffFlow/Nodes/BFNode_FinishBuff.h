#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_FinishBuff.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Finish Buff", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_FinishBuff : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
