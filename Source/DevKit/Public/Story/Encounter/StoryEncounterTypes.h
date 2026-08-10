#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameModes/LevelFlowTypes.h"
#include "Story/StoryRuleTypes.h"
#include "Story/StoryRewardOverrideTypes.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "UI/GameDialogWidget.h"
#include "StoryEncounterTypes.generated.h"

class ARewardPickup;
class UFlowAsset;
class ULevelFlowAsset;

UENUM(BlueprintType)
enum class EStoryEncounterNodeKind : uint8
{
	Area UMETA(DisplayName = "区域"),
	Object UMETA(DisplayName = "物件"),
	NPC UMETA(DisplayName = "NPC"),
	System UMETA(DisplayName = "系统"),
	Death UMETA(DisplayName = "死亡"),
	Feature UMETA(DisplayName = "功能"),
};

UENUM(BlueprintType)
enum class EStoryEncounterFirePolicy : uint8
{
	Once UMETA(DisplayName = "只触发一次"),
	Repeat UMETA(DisplayName = "可重复触发"),
	OncePerRun UMETA(DisplayName = "每局一次"),
};

UENUM(BlueprintType)
enum class EStoryEncounterConditionKind : uint8
{
	None UMETA(DisplayName = "无条件"),
	ProgressMissing UMETA(DisplayName = "还没有发生过"),
	ProgressCompleted UMETA(DisplayName = "已经发生过"),
	RunCountAtLeast UMETA(DisplayName = "第 N 局之后"),
	FeatureUnlocked UMETA(DisplayName = "功能已解锁"),
};

UENUM(BlueprintType)
enum class EStoryEncounterActionKind : uint8
{
	WeakHint UMETA(DisplayName = "底部操作提示条"),
	Dialogue UMETA(DisplayName = "对话"),
	RecordProgress UMETA(DisplayName = "记录进度"),
	UnlockFeature UMETA(DisplayName = "解锁功能"),
	SetQuestObjective UMETA(DisplayName = "设置目标"),
	TeleportToNode UMETA(DisplayName = "跳到节点"),
	PlayLevelFlow UMETA(DisplayName = "播放流程"),
	SetActorEnabled UMETA(DisplayName = "设置关卡对象启用"),
	TutorialPopup UMETA(DisplayName = "教程弹窗"),
	TutorialAreaHint UMETA(DisplayName = "区域教程提示"),
	SpawnRewardPickup UMETA(DisplayName = "Spawn Reward Pickup"),
	SetRoomRewardOverride UMETA(DisplayName = "Set Room Reward Override"),
	SetPortalOverride UMETA(DisplayName = "Set Portal Override"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Condition")
	EStoryEncounterConditionKind Kind = EStoryEncounterConditionKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Condition")
	FName ProgressKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Condition")
	FText ProgressLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Condition")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Condition", meta = (ClampMin = "0"))
	int32 RunCount = 0;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	EStoryEncounterActionKind Kind = EStoryEncounterActionKind::WeakHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FName ReuseKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action", meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	bool bUseInputTextVariants = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action", meta = (MultiLine = true))
	FText KeyboardMouseBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action", meta = (MultiLine = true))
	FText GamepadBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FName TutorialEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	TArray<FTutorialPage> TutorialPages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	bool bPauseGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FName ProgressKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FText ProgressLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FGameplayTag QuestTaskTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	FName TargetNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Level Objects")
	FName TargetActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Level Objects")
	FName TargetActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Level Objects")
	bool bActorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	TObjectPtr<ULevelFlowAsset> LevelFlow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	bool bStopExistingStoryFlow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	TSubclassOf<ARewardPickup> RewardPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	TArray<FLootOption> RewardLootOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup", meta = (ClampMin = "1"))
	int32 RewardPickupCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	FVector RewardSpawnOffset = FVector(120.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	bool bSpawnRewardOnTargetDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	bool bRewardPickupAllowedOutsideArrangement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Pickup")
	bool bPlayRewardPickupFocusCue = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Override")
	bool bClearRoomRewardOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Reward Override")
	EStoryRewardOverrideTarget RewardOverrideTarget = EStoryRewardOverrideTarget::CurrentRoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Portal Override")
	int32 ForcedPortalIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action|Portal Override")
	bool bClearPortalOverride = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	EStoryEncounterNodeKind Kind = EStoryEncounterNodeKind::Area;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node", meta = (MultiLine = true))
	FText PlayerFacingEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	EStoryEncounterFirePolicy FirePolicy = EStoryEncounterFirePolicy::Once;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	FStoryEncounterCondition Condition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	TArray<FStoryEncounterAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node|Flow")
	TObjectPtr<UStoryFlowAsset> NodeEventFlow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	FName NextNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Node")
	FVector2D EditorPosition = FVector2D::ZeroVector;
};
