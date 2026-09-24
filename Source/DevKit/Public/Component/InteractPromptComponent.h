#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractPromptComponent.generated.h"

class UInteractPromptWidget;
class UWidgetComponent;

/**
 * Owns the hold-to-interact prompt for one interactable.
 *
 * Add this to any actor implementing IYogInteractable; the interface's default
 * SetInteractHoldProgress / ShowInteractPrompt find it automatically, so the actor only decides
 * *when* the prompt should be up (its own gating) and never touches the widget.
 */
UCLASS(ClassGroup = (Interact), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UInteractPromptComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractPromptComponent();

	/** Height above the owner's root that the prompt floats at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	float PromptHeight = 150.f;

	UFUNCTION(BlueprintCallable, Category = "Interact Prompt")
	void SetPromptVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Interact Prompt")
	bool IsPromptShowing() const;

	UFUNCTION(BlueprintCallable, Category = "Interact Prompt")
	void SetHoldProgress(float Normalized);

	UWidgetComponent* GetWidgetComponent() const { return WidgetComponent; }

protected:
	virtual void OnRegister() override;

private:
	/** Spawns the widget component on the owning actor. Must own it, not this component, to register. */
	void EnsureWidgetComponent();

	/** Creates the widget once, binds it to the local player, and starts it collapsed. */
	void EnsureWidget();

	UInteractPromptWidget* GetPromptWidget() const;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> WidgetComponent;
};
