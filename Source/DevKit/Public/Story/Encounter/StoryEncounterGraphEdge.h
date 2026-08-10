#pragma once

#include "CoreMinimal.h"
#include "GenericGraphEdge.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterGraphEdge.generated.h"

/**
 * Directed transition between two story encounter graph nodes.
 */
UCLASS(BlueprintType)
class DEVKIT_API UStoryEncounterGraphEdge : public UGenericGraphEdge
{
	GENERATED_BODY()

public:
	UStoryEncounterGraphEdge();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Edge")
	FText TransitionLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Story Edge")
	FStoryEncounterCondition Condition;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle) override;
#endif
};
