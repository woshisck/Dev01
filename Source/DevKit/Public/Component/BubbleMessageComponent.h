#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BubbleMessageComponent.generated.h"

struct FBubbleLine;
class UBubbleMessageWidget;
class UWidgetComponent;

/**
 * Owns the world-anchored speech bubble for one speaker.
 *
 * UYogBubbleSubsystem creates this on demand the first time an actor speaks and keeps it pooled on
 * the actor afterwards, so it does not need to be authored on anything. Actors can still add it in
 * the editor to tune BubbleHeight.
 */
UCLASS(ClassGroup = (Bubble), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UBubbleMessageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBubbleMessageComponent();

	/** Height above the owner's root that the bubble floats at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bubble")
	float BubbleHeight = 190.f;

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void ShowLine(const FText& SpeakerName, const FText& Body);

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void Hide();

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	bool IsShowing() const;

	UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }

protected:
	virtual void OnRegister() override;

private:
	/** Spawns the widget component on the owning actor. Must own it, not this component, to register. */
	void EnsureWidgetComponent();

	/** Creates the widget once, binds it to the local player, and starts it collapsed. */
	void EnsureWidget();

	UBubbleMessageWidget* GetBubbleWidget() const;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> WidgetComponent;
};
