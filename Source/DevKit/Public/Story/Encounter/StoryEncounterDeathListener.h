#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryEncounterDeathListener.generated.h"

class AYogCharacterBase;
class UStoryEncounterGraph;
class UStoryEncounterMap;
class UStoryEncounterPointDA;

/**
 * Arms story encounter nodes from character death without requiring an overlap trigger.
 * Place one in the level, set a target actor name/tag, then bind an encounter point or graph node.
 */
UCLASS(Blueprintable)
class DEVKIT_API AStoryEncounterDeathListener : public AActor
{
	GENERATED_BODY()

public:
	AStoryEncounterDeathListener();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UStoryEncounterPointDA> EncounterPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UStoryEncounterMap> EncounterMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UStoryEncounterGraph> EncounterGraph = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter|Target")
	FName TargetActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter|Target")
	FName TargetActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	bool bTriggerOnce = true;

	UFUNCTION(BlueprintCallable, Category = "Story Encounter")
	int32 BindMatchingTargets();

	UFUNCTION(BlueprintPure, Category = "Story Encounter")
	bool HasTriggered() const { return bTriggered; }

	bool IsBoundToTarget(const AYogCharacterBase* Character) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleTargetDied(AYogCharacterBase* Character);

	bool DoesTargetMatch(const AActor* Actor) const;
	bool BindTarget(AYogCharacterBase* Character);
	void UnbindAllTargets();
	bool TriggerEncounter(AYogCharacterBase* Character);

	UPROPERTY(VisibleAnywhere, Category = "Story Encounter|Debug")
	bool bTriggered = false;

	struct FBoundDeathTarget
	{
		TWeakObjectPtr<AYogCharacterBase> Character;
		FDelegateHandle DelegateHandle;
	};

	TArray<FBoundDeathTarget> BoundTargets;
};
