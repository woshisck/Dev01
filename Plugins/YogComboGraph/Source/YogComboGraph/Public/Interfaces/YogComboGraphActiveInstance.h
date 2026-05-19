#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "YogComboGraphActiveInstance.generated.h"

class UGameplayAbilityComboGraph;

UINTERFACE(MinimalAPI, BlueprintType)
class UYogComboGraphActiveInstance : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by runtime objects (typically the project's combo runtime component)
 * to expose which ComboGraph asset and which node-id are currently active. The
 * plugin's asset editor uses this to drive PIE node-highlight in the graph view
 * without depending on any project-specific class.
 */
class YOGCOMBOGRAPH_API IYogComboGraphActiveInstance
{
	GENERATED_BODY()

public:
	virtual const UGameplayAbilityComboGraph* GetActiveComboGraph() const = 0;
	virtual FName GetActiveComboNodeId() const = 0;
};
