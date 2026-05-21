#pragma once

#include "CoreMinimal.h"
#include "GenericGraphNode.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterGraphNode.generated.h"

class UStoryEncounterPointDA;

/**
 * Canvas node for a story encounter graph.
 *
 * The node is intentionally thin: designers bind a StoryEncounterPoint DA here, and that DA is what level triggers run.
 */
UCLASS(BlueprintType, meta = (DisplayName = "剧情点节点"))
class DEVKIT_API UStoryEncounterGraphNode : public UGenericGraphNode
{
	GENERATED_BODY()

public:
	UStoryEncounterGraphNode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情点")
	TObjectPtr<UStoryEncounterPointDA> Point = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情点|兜底")
	FName FallbackEncounterId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情点|兜底")
	FName FallbackNodeId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情点|兜底")
	FText FallbackTitle;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	UStoryEncounterPointDA* GetPoint() const;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	FName GetEncounterId() const;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	FName GetStoryNodeId() const;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	FStoryEncounterNode ToEncounterNode() const;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle) override;
	virtual bool CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes,
		FText& ErrorMessage) override;
#endif
};
