#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "TagReactionDataAsset.generated.h"

class UFlowAsset;
class UGameplayAbility;
class UYogGameplayEffect;

UENUM(BlueprintType)
enum class ETagReactionType : uint8
{
	StartBuffFlow		UMETA(DisplayName = "启动 BuffFlow"),
	ApplyGameplayEffect	UMETA(DisplayName = "施加 GameplayEffect"),
	ActivateAbility		UMETA(DisplayName = "激活 GameplayAbility")
};

UENUM(BlueprintType)
enum class ETagReactionUndo : uint8
{
	Auto	UMETA(DisplayName = "自动回收"),
	Persist	UMETA(DisplayName = "不回收")
};

/**
 * One tag reaction rule: run ReactionType when TriggerTag appears on the ASC,
 * tear it down again when the tag leaves (subject to UndoPolicy).
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FTagReactionRule
{
	GENERATED_BODY()

	// Fires on the 0 -> non-zero edge, undone on the non-zero -> 0 edge.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "触发 Tag"))
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "反应类型"))
	ETagReactionType ReactionType = ETagReactionType::StartBuffFlow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "回收策略"))
	ETagReactionUndo UndoPolicy = ETagReactionUndo::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "Flow 资产",
		EditCondition = "ReactionType == ETagReactionType::StartBuffFlow", EditConditionHides))
	TObjectPtr<UFlowAsset> FlowAsset = nullptr;

	// Auto undo assumes an infinite-duration GE; a finite one expires before the tag does.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "GameplayEffect",
		EditCondition = "ReactionType == ETagReactionType::ApplyGameplayEffect", EditConditionHides))
	TSubclassOf<UYogGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "GameplayAbility",
		EditCondition = "ReactionType == ETagReactionType::ActivateAbility", EditConditionHides))
	TSubclassOf<UGameplayAbility> AbilityClass;

	// Execution order when one tag carries several rules; higher runs first.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction", meta = (DisplayName = "优先级"))
	int32 Priority = 0;
};

/**
 * Sibling of UStateConflictDataAsset, not a replacement.
 *   StateConflict — what is suppressed while a tag is present (movement / AI / ability blocking)
 *   TagReaction   — what fires when a tag flips (start flow / apply GE / activate GA)
 *
 * Both dispatch from UYogAbilitySystemComponent::OnTagUpdated; the conflict phase runs first so a
 * reaction can never activate an ability the same tag is meant to block.
 */
UCLASS(BlueprintType)
class DEVKIT_API UTagReactionDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagReaction")
	TArray<FTagReactionRule> Reactions;
};
