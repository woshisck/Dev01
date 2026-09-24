#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "YogBubbleSpeaker.generated.h"

UINTERFACE(MinimalAPI)
class UYogBubbleSpeaker : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lets an actor say a line without knowing anything about the UI.
 *
 * Every function below already works — the default bodies forward into UYogBubbleSubsystem, which
 * owns the widget, the queue and the lifetime. An implementer only decides *when* to speak, and
 * optionally overrides GetBubbleSpeakerName to label the bubble.
 *
 * Plain virtuals rather than BlueprintNativeEvent on purpose: a BlueprintNativeEvent would make
 * every implementer either chain to Super or re-write the forwarding call. Blueprint actors that
 * need to trigger a bubble should call UYogBubbleSubsystem directly, which is BlueprintCallable.
 */
class DEVKIT_API IYogBubbleSpeaker
{
	GENERATED_BODY()

public:
	/** Authored line(s) from the bubble DataTable. */
	virtual void ShowBubble(const FDataTableRowHandle& RowHandle, FGameplayTag DedupTag = FGameplayTag());

	/** Row lookup by name against UYogSettings::BubbleMessageTable. */
	virtual void ShowBubbleByName(FName RowName, FGameplayTag DedupTag = FGameplayTag());

	/** Dynamic text with no authored row — player names, counts, debug output. */
	virtual void ShowBubbleText(const FText& Line, float Duration = 3.f);

	virtual void DismissBubble();

	/** Shown above the line. Empty by default so the bubble is text-only. */
	virtual FText GetBubbleSpeakerName() const { return FText::GetEmpty(); }
};
