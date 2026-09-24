#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/BubbleMessageTypes.h"
#include "YogBubbleSubsystem.generated.h"

class AYogPlayerControllerBase;
class UBubbleMessageComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogYogBubble, Log, All);

/**
 * One bubble in flight. Ambient bubbles keep one of these per speaker; the blocking bubble keeps
 * a single one for the whole world.
 */
USTRUCT()
struct FActiveBubble
{
	GENERATED_BODY()

	UPROPERTY()
	FBubbleRequest Request;

	UPROPERTY()
	int32 LineIndex = 0;

	/** Seconds left on the current line. Unused while blocking — those advance on the interact key. */
	UPROPERTY()
	float TimeRemaining = 0.f;

	/** True while the line is being drawn by AYogHUD instead of the speaker's widget component. */
	UPROPERTY()
	bool bUsingScreenFallback = false;

	const FBubbleLine* GetCurrentLine() const
	{
		return Request.Lines.IsValidIndex(LineIndex) ? &Request.Lines[LineIndex] : nullptr;
	}
};

/**
 * Central owner of every bubble message: queueing, dedup, component pooling and lifetime.
 *
 * Actors normally reach this through IYogBubbleSpeaker. Systems with no speaker actor (tutorials,
 * room events) call RequestBubbleText directly with a null speaker.
 *
 * World-scoped on purpose. Every dependency — speaker actors, timers, AYogHUD, the controller that
 * owns conversation mode — dies with the world, so a level transition cannot leave a pooled
 * component dangling or the player frozen in a half-finished blocking conversation.
 */
UCLASS(BlueprintType)
class DEVKIT_API UYogBubbleSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/** Speaker may be null only when the row anchors to a screen corner. */
	UFUNCTION(BlueprintCallable, Category = "Bubble", meta = (AutoCreateRefTerm = "DedupTag"))
	void RequestBubbleFromRow(AActor* Speaker, const FDataTableRowHandle& RowHandle, const FGameplayTag& DedupTag);

	/** Row lookup against UYogSettings::BubbleMessageTable, for callers holding only a row name. */
	UFUNCTION(BlueprintCallable, Category = "Bubble", meta = (AutoCreateRefTerm = "DedupTag"))
	void RequestBubbleByName(AActor* Speaker, FName RowName, const FGameplayTag& DedupTag);

	/** Dynamic/debug text that has no authored row. A null Speaker forces the screen-corner anchor. */
	UFUNCTION(BlueprintCallable, Category = "Bubble", meta = (AutoCreateRefTerm = "DedupTag"))
	void RequestBubbleText(
		AActor* Speaker,
		const FText& SpeakerName,
		const FText& Line,
		float Duration,
		EBubbleAnchorMode AnchorMode,
		int32 Priority,
		const FGameplayTag& DedupTag);

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void DismissBubbleFor(AActor* Speaker);

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void DismissAll();

	UFUNCTION(BlueprintPure, Category = "Bubble")
	bool IsBlockingBubbleActive() const { return ActiveBlocking.IsSet(); }

	/** Bound to the controller's raw interact press. Inert unless a blocking bubble is running. */
	void AdvanceBlockingBubble();

	/**
	 * True when the projected speaker position is unusable and the line should move to a screen
	 * corner instead. Pure so it can be unit tested without a world.
	 *
	 * bProjectionValid carries ProjectWorldLocationToScreen's return value — a point behind the
	 * camera still yields finite coordinates, so the bounds test alone would pass it.
	 */
	static bool ResolveShouldFallbackToScreen(
		const FVector2D& ScreenPos,
		const FVector2D& ViewportSize,
		bool bProjectionValid,
		float EdgeMargin);

	/** Higher priority first, insertion order preserved within a priority. Pure, for testability. */
	static void SortBlockingQueue(TArray<FBubbleRequest>& Queue);

private:
	void SubmitRequest(FBubbleRequest&& Request);
	void StartAmbient(FBubbleRequest&& Request);
	void StartBlocking(FBubbleRequest&& Request);
	void FinishBlocking();

	void TickAmbient(float DeltaTime);
	void RefreshAnchor(FActiveBubble& Bubble);
	void PresentLine(FActiveBubble& Bubble);
	void ClearPresentation(FActiveBubble& Bubble);

	bool ConsumeDedup(const FGameplayTag& DedupTag);
	void TickDedup(float DeltaTime);

	UBubbleMessageComponent* GetOrCreateComponent(AActor* Speaker);

	AYogPlayerControllerBase* GetYogPlayerController() const;
	class AYogHUD* GetYogHUD() const;

	/**
	 * One line per speaker, all running concurrently. A null speaker collapses to the default key,
	 * so systems without an actor share a single slot rather than stacking toasts on each other.
	 *
	 * Not a UPROPERTY: TObjectKey is not a reflected key type, and nothing in here is a strong
	 * UObject reference that would need GC tracking.
	 */
	TMap<TObjectKey<AActor>, FActiveBubble> ActiveAmbient;

	/** Weak by design — the component is registered on its actor, which owns its lifetime. */
	TMap<TObjectKey<AActor>, TWeakObjectPtr<UBubbleMessageComponent>> ComponentPool;

	/** Only one blocking bubble can hold conversation mode, so the rest wait here. */
	TOptional<FActiveBubble> ActiveBlocking;

	TArray<FBubbleRequest> BlockingQueue;

	/** Tag -> seconds until the same bubble may fire again. */
	TMap<FGameplayTag, float> DedupCooldowns;

	/** Which speaker currently owns the shared screen-corner slot, so it is handed back cleanly. */
	TObjectKey<AActor> ScreenSlotOwner;
	bool bScreenSlotInUse = false;

	FDelegateHandle InteractPressedHandle;
	TWeakObjectPtr<AYogPlayerControllerBase> BoundController;
};
