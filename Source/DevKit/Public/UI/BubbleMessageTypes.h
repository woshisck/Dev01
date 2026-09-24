#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "BubbleMessageTypes.generated.h"

/**
 * Tie-break order when several systems request a bubble at once. Higher wins.
 * Values are spaced so new categories can slot between them. Mirrors YogInteractPriority.
 */
namespace YogBubblePriority
{
	constexpr int32 Story   = 100;
	constexpr int32 Hint    = 60;
	constexpr int32 Ambient = 20;
}

UENUM(BlueprintType)
enum class EBubbleAnchorMode : uint8
{
	/** Above the speaker while it is on screen, falling back to the screen corner when it is not. */
	Auto        UMETA(DisplayName = "Auto (world, falls back to screen)"),

	/** Above the speaker only. Hidden entirely while the speaker is off screen. */
	WorldOnly   UMETA(DisplayName = "World only (hide when off screen)"),

	/** Fixed screen corner. The only valid mode when there is no speaker actor. */
	ScreenOnly  UMETA(DisplayName = "Screen corner only")
};

UENUM(BlueprintType)
enum class EBubbleScreenCorner : uint8
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBubbleLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble", meta = (MultiLine = "true"))
	FText Text;

	/** Ignored while the owning bubble is blocking — those advance on the interact key instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble", meta = (ClampMin = "0.1"))
	float Duration = 3.f;
};

/**
 * One authored bubble. Rows live in a UDataTable referenced from Project Settings
 * (UYogSettings::BubbleMessageTable); callers normally pass an FDataTableRowHandle.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FBubbleMessageRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	TArray<FBubbleLine> Lines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	EBubbleAnchorMode AnchorMode = EBubbleAnchorMode::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	EBubbleScreenCorner ScreenCorner = EBubbleScreenCorner::TopRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	int32 Priority = YogBubblePriority::Ambient;

	/**
	 * Holds the player in conversation mode for the whole sequence and advances on the interact
	 * key instead of a timer. Only one blocking bubble can run at a time; the rest queue.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	bool bBlocking = false;
};

/**
 * Runtime form of a bubble. Both the DataTable path and the raw-FText path collapse into this, so
 * the subsystem only ever deals with one representation.
 */
USTRUCT()
struct DEVKIT_API FBubbleRequest
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> Speaker;

	UPROPERTY()
	FText SpeakerName;

	UPROPERTY()
	TArray<FBubbleLine> Lines;

	UPROPERTY()
	EBubbleAnchorMode AnchorMode = EBubbleAnchorMode::Auto;

	UPROPERTY()
	EBubbleScreenCorner ScreenCorner = EBubbleScreenCorner::TopRight;

	UPROPERTY()
	int32 Priority = YogBubblePriority::Ambient;

	UPROPERTY()
	bool bBlocking = false;

	UPROPERTY()
	FGameplayTag DedupTag;

	bool IsValidRequest() const
	{
		return Lines.Num() > 0;
	}
};
