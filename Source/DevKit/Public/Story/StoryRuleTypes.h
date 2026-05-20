#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "StoryRuleTypes.generated.h"

class AActor;
class APlayerController;
class ULevelFlowAsset;

UENUM(BlueprintType)
enum class EStoryFlagScope : uint8
{
	Save UMETA(DisplayName = "Save"),
	Run UMETA(DisplayName = "Run"),
	Session UMETA(DisplayName = "Session"),
};

UENUM(BlueprintType)
enum class EStoryRuleFirePolicy : uint8
{
	Always UMETA(DisplayName = "Always"),
	OncePerSave UMETA(DisplayName = "Once Per Save"),
	OncePerRun UMETA(DisplayName = "Once Per Run"),
	OncePerMap UMETA(DisplayName = "Once Per Map"),
};

UENUM(BlueprintType)
enum class EStoryConditionMatchPolicy : uint8
{
	All UMETA(DisplayName = "All"),
	Any UMETA(DisplayName = "Any"),
};

UENUM(BlueprintType)
enum class EStoryConditionType : uint8
{
	None UMETA(DisplayName = "None"),
	HasFlag UMETA(DisplayName = "Has Flag"),
	FeatureUnlocked UMETA(DisplayName = "Feature Unlocked"),
	TutorialStateEquals UMETA(DisplayName = "Tutorial State Equals"),
	RunCountAtLeast UMETA(DisplayName = "Run Count At Least"),
	EventTagEquals UMETA(DisplayName = "Event Tag Equals"),
	ContextTagMatches UMETA(DisplayName = "Context Tag Matches"),
};

UENUM(BlueprintType)
enum class EStoryActionType : uint8
{
	None UMETA(DisplayName = "None"),
	SetFlag UMETA(DisplayName = "Set Flag"),
	ClearFlag UMETA(DisplayName = "Clear Flag"),
	PlayLevelFlow UMETA(DisplayName = "Play Level Flow"),
	ShowTutorialPopup UMETA(DisplayName = "Show Tutorial Popup"),
	ShowInfoHint UMETA(DisplayName = "Show Info Hint"),
	SetQuestTask UMETA(DisplayName = "Set Quest Task"),
	CompleteQuestTask UMETA(DisplayName = "Complete Quest Task"),
	UnlockFeature UMETA(DisplayName = "Unlock Feature"),
	AddMetaCurrency UMETA(DisplayName = "Add Meta Currency"),
	TriggerStoryEvent UMETA(DisplayName = "Trigger Story Event"),
};

UENUM(BlueprintType)
enum class EStoryQuestTaskState : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed UMETA(DisplayName = "Failed"),
	Overwritten UMETA(DisplayName = "Overwritten"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEventContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag AreaTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag ContextTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 FloorIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 RunIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FName MapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FName SourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TObjectPtr<APlayerController> PlayerController = nullptr;

	static FStoryEventContext Make(FGameplayTag InEventTag)
	{
		FStoryEventContext Context;
		Context.EventTag = InEventTag;
		return Context;
	}
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryConditionType Type = EStoryConditionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bInvert = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryFlagScope FlagScope = EStoryFlagScope::Save;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag FlagTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	ETutorialState TutorialState = ETutorialState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (ClampMin = "0"))
	int32 RunCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag MatchTag;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryActionType Type = EStoryActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryFlagScope FlagScope = EStoryFlagScope::Save;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag FlagTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TObjectPtr<ULevelFlowAsset> LevelFlow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bStopExistingStoryFlow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FName TutorialEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bPauseGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FText HintTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
	FText HintText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (ClampMin = "0"))
	float HintDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag QuestTaskId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
	FText QuestTaskText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag QuestSourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag RelatedFlagTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag CurrencyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag EventTagToTrigger;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FName RuleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag TriggerEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryConditionMatchPolicy ConditionPolicy = EStoryConditionMatchPolicy::All;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TArray<FStoryCondition> Conditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TArray<FStoryAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryRuleFirePolicy FirePolicy = EStoryRuleFirePolicy::OncePerSave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bStopAfterMatch = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryQuestTaskData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag TaskId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EStoryQuestTaskState State = EStoryQuestTaskState::Inactive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (MultiLine = true))
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag SourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag RelatedFlagTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FGameplayTag RewardCurrencyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	int32 RewardCurrencyAmount = 0;
};
