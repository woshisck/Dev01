#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryEventTypes.h"
#include "StoryEventRegistryDA.generated.h"

UCLASS(BlueprintType)
class DEVKIT_API UStoryEventRegistryDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StoryEvent", meta = (TitleProperty = "EventTag"))
	TArray<FStoryEventEntry> Entries;

	const FStoryEventEntry* FindEntry(FGameplayTag EventTag) const;
};
