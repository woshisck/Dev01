#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryFlowProxy.generated.h"

class UFlowAsset;
class UFlowComponent;

/**
 * Lightweight runtime owner for story-driven Flow assets.
 * Stores the triggering context so LevelFlow nodes can read it without coupling
 * the source actor class to story-specific behavior.
 */
UCLASS(NotBlueprintable)
class DEVKIT_API AStoryFlowProxy : public AActor
{
	GENERATED_BODY()

public:
	AStoryFlowProxy();

	void RunFlow(UFlowAsset* FlowAsset);
	void StopFlow();
	bool IsRunningFlow(const UFlowAsset* FlowAsset) const;

	UFUNCTION(BlueprintPure, Category = "Story|Context")
	AActor* GetContextSourceActor() const { return ContextSourceActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Story|Context")
	FTransform GetContextTransform() const { return ContextTransform; }

	UFUNCTION(BlueprintPure, Category = "Story|Context")
	APlayerController* GetContextPlayerController() const { return ContextPlayerController.Get(); }

	UPROPERTY()
	TWeakObjectPtr<AActor> ContextSourceActor;

	UPROPERTY()
	FTransform ContextTransform = FTransform::Identity;

	UPROPERTY()
	TWeakObjectPtr<APlayerController> ContextPlayerController;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFlowComponent> FlowComp;

	FTimerHandle PollTimer;

	void PollFlowCompletion();
};
