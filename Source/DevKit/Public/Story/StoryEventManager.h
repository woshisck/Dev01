#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/StoryEventTypes.h"
#include "StoryEventManager.generated.h"

class APlayerController;
class URoomDataAsset;
class UStoryEventRegistryDA;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryEventDispatched, FStoryEventRuntimeContext, Context);

UCLASS()
class DEVKIT_API UStoryEventManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "StoryEvent")
	void SetRegistry(UStoryEventRegistryDA* InRegistry);

	UFUNCTION(BlueprintCallable, Category = "StoryEvent")
	void ProcessCampaignStage(int32 FloorIndex, FGameplayTag StageTag, FGameplayTagContainer EventTags,
		URoomDataAsset* RoomData, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "StoryEvent")
	void ResetRunEvents();

	UFUNCTION(BlueprintPure, Category = "StoryEvent")
	bool HasFiredEvent(FGameplayTag EventTag) const;

	UPROPERTY(BlueprintAssignable, Category = "StoryEvent")
	FOnStoryEventDispatched OnStoryEventDispatched;

	UPROPERTY(BlueprintAssignable, Category = "StoryEvent")
	FOnStoryEventDispatched OnStoryEventSkipped;

private:
	UPROPERTY()
	TObjectPtr<UStoryEventRegistryDA> Registry;

	UPROPERTY()
	FGameplayTagContainer FiredRunEventTags;

	FStoryEventRuntimeContext BuildContext(int32 FloorIndex, FGameplayTag StageTag, FGameplayTag EventTag,
		URoomDataAsset* RoomData) const;
	bool ShouldSkipForTutorialState(const FStoryEventEntry& Entry) const;
	bool DispatchTutorialPopup(const FStoryEventEntry& Entry, FStoryEventRuntimeContext& Context,
		APlayerController* PlayerController);
};
