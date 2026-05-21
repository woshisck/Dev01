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
UCLASS(BlueprintType, meta = (DisplayName = "剧情教学流程图"))
class DEVKIT_API UStoryEncounterGraph : public UGenericGraph
{
	GENERATED_BODY()

public:
	UStoryEncounterGraph();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情流程图")
	FName EncounterId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情流程图")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情流程图", meta = (MultiLine = true))
	FText Description;

	UFUNCTION(BlueprintPure, Category = "剧情流程图")
	UStoryEncounterGraphNode* FindNodeByStoryNodeId(FName NodeId) const;
};
