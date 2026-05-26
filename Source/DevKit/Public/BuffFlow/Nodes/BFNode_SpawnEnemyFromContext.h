#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SpawnEnemyFromContext.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Enemy From Context", Category = "BuffFlow|Lifecycle"))
class DEVKIT_API UBFNode_SpawnEnemyFromContext : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
