#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Story/StoryRuleTypes.h"
#include "UI/GameDialogWidget.h"
#include "StoryEncounterTypes.generated.h"

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
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情条件")
	EStoryEncounterConditionKind Kind = EStoryEncounterConditionKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情条件")
	FName ProgressKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情条件")
	FText ProgressLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情条件")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情条件", meta = (ClampMin = "0"))
	int32 RunCount = 0;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	EStoryEncounterActionKind Kind = EStoryEncounterActionKind::WeakHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FName ActionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FName ReuseKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作", meta = (MultiLine = true))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	bool bUseInputTextVariants = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作", meta = (MultiLine = true))
	FText KeyboardMouseBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作", meta = (MultiLine = true))
	FText GamepadBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FName TutorialEventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Action")
	TArray<FTutorialPage> TutorialPages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	bool bPauseGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FName ProgressKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FText ProgressLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FGameplayTag FeatureTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FGameplayTag QuestTaskTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	FName TargetNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作|关卡对象")
	FName TargetActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作|关卡对象")
	FName TargetActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作|关卡对象")
	bool bActorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情动作")
	TObjectPtr<ULevelFlowAsset> LevelFlow = nullptr;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryEncounterNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	EStoryEncounterNodeKind Kind = EStoryEncounterNodeKind::Area;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点", meta = (MultiLine = true))
	FText PlayerFacingEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	EStoryEncounterFirePolicy FirePolicy = EStoryEncounterFirePolicy::Once;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	FStoryEncounterCondition Condition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	TArray<FStoryEncounterAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	FName NextNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点")
	FVector2D EditorPosition = FVector2D::ZeroVector;
};
