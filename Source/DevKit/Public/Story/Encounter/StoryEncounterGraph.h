#pragma once

#include "CoreMinimal.h"
#include "GenericGraph.h"
#include "StoryEncounterGraph.generated.h"

class UStoryEncounterGraphNode;

/**
 * Visual story/tutorial flow graph.
 *
 * Designers use the GenericGraph editor to connect nodes, and each graph node can bind a StoryEncounterPoint DA.
 */
UCLASS(BlueprintType)
class DEVKIT_API UStoryEncounterGraph : public UGenericGraph
{
	GENERATED_BODY()

public:
	UStoryEncounterGraph();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Graph")
	FName EncounterId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Graph")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Graph", meta = (MultiLine = true))
	FText Description;

	UFUNCTION(BlueprintPure, Category = "Story Graph")
	UStoryEncounterGraphNode* FindNodeByStoryNodeId(FName NodeId) const;
};
