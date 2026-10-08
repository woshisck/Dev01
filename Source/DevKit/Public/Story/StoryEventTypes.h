#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StoryEventTypes.generated.h"

class URoomDataAsset;
class ULevelFlowAsset;

UENUM(BlueprintType)
enum class EStoryEventActionType : uint8
{
	None UMETA(DisplayName = "None"),
	BroadcastOnly UMETA(DisplayName = "Broadcast Only"),
	// Slot retained after the tutorial system was removed, to keep the serialized
	// uint8 index of LevelFlow stable in existing story event registry assets.
	Deprecated_TutorialPopup UMETA(Hidden),
	LevelFlow UMETA(DisplayName = "Level Flow"),
};

UENUM(BlueprintType)
enum class EStoryEventDispatchResult : uint8
{
	Unconfigured UMETA(DisplayName = "Unconfigured"),
	Triggered UMETA(DisplayName = "Triggered"),
	SkippedAlreadyFired UMETA(DisplayName = "Skipped Already Fired"),
	Deprecated_SkippedTutorialCompleted UMETA(Hidden),
	Failed UMETA(DisplayName = "Failed"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEventEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	EStoryEventActionType ActionType = EStoryEventActionType::BroadcastOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent", meta = (EditCondition = "ActionType == EStoryEventActionType::LevelFlow"))
	TObjectPtr<ULevelFlowAsset> LevelFlow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent", meta = (EditCondition = "ActionType == EStoryEventActionType::LevelFlow"))
	bool bStopExistingStoryFlow = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	bool bFireOncePerRun = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	FText DesignerNote;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEventRuntimeContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	int32 FloorIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	FGameplayTag StageTag;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	FGameplayTag EventTag;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	TObjectPtr<URoomDataAsset> RoomData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	EStoryEventActionType ActionType = EStoryEventActionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	EStoryEventDispatchResult Result = EStoryEventDispatchResult::Unconfigured;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	FName ResolvedTutorialEventID;

	UPROPERTY(BlueprintReadOnly, Category = "StoryEvent")
	TObjectPtr<ULevelFlowAsset> ResolvedLevelFlow = nullptr;
};
