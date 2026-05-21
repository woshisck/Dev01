#pragma once

#include "CoreMinimal.h"
#include "GenericGraphEdge.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterGraphEdge.generated.h"

/**
 * Directed transition between two story encounter graph nodes.
 */
UCLASS(BlueprintType, meta = (DisplayName = "剧情点连线"))
class DEVKIT_API UStoryEncounterGraphEdge : public UGenericGraphEdge
{
	GENERATED_BODY()

public:
	UStoryEncounterGraphEdge();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情连线")
	FText TransitionLabel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "剧情连线")
	FStoryEncounterCondition Condition;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle) override;
#endif
};
