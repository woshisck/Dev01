#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StoryEventTypes.generated.h"

class URoomDataAsset;

UENUM(BlueprintType)
enum class EStoryEventActionType : uint8
{
	None UMETA(DisplayName = "None"),
	TutorialPopup UMETA(DisplayName = "Tutorial Popup"),
};

UENUM(BlueprintType)
enum class EStoryEventDispatchResult : uint8
{
	Unconfigured UMETA(DisplayName = "Unconfigured"),
	Triggered UMETA(DisplayName = "Triggered"),
	SkippedAlreadyFired UMETA(DisplayName = "Skipped Already Fired"),
	SkippedTutorialCompleted UMETA(DisplayName = "Skipped Tutorial Completed"),
	Failed UMETA(DisplayName = "Failed"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEventEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	EStoryEventActionType ActionType = EStoryEventActionType::TutorialPopup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent", meta = (EditCondition = "ActionType == EStoryEventActionType::TutorialPopup"))
	FName TutorialEventID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent", meta = (EditCondition = "ActionType == EStoryEventActionType::TutorialPopup"))
	bool bPauseGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StoryEvent")
	bool bOnlyWhenTutorialIncomplete = false;

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
};
