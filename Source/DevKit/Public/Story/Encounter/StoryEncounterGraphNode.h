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
UCLASS(BlueprintType)
class DEVKIT_API UStoryEncounterGraphNode : public UGenericGraphNode
{
	GENERATED_BODY()

public:
	UStoryEncounterGraphNode();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Point")
	TObjectPtr<UStoryEncounterPointDA> Point = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Point|Fallback")
	FName FallbackEncounterId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Point|Fallback")
	FName FallbackNodeId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Point|Fallback")
	FText FallbackTitle;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	UStoryEncounterPointDA* GetPoint() const;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	FName GetEncounterId() const;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	FName GetStoryNodeId() const;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	FStoryEncounterNode ToEncounterNode() const;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle) override;
	virtual bool CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes,
		FText& ErrorMessage) override;
#endif
};
