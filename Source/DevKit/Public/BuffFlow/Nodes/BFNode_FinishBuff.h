#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_FinishBuff.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "结束当前Buff", Category = "BuffFlow|增益"))
class DEVKIT_API UBFNode_FinishBuff : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
