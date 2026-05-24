#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterRuntimeSubsystem.generated.h"

class UStoryEncounterMap;
class UStoryEncounterGraph;
class UStoryEncounterPointDA;
class ULevelInfoPopupDA;
class AYogCharacterBase;

UCLASS()
class DEVKIT_API UStoryEncounterRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Story Encounter")
	bool TriggerEncounterNode(UStoryEncounterMap* EncounterMap, FName NodeId, AActor* SourceActor);

	UFUNCTION(BlueprintCallable, Category = "Story Encounter")
	bool TriggerEncounterGraphNode(UStoryEncounterGraph* EncounterGraph, FName NodeId, AActor* SourceActor);

	UFUNCTION(BlueprintCallable, Category = "Story Encounter")
	bool TriggerEncounterPoint(UStoryEncounterPointDA* EncounterPoint, AActor* SourceActor);

	static FString MakeProgressTagName(FName EncounterId, FName ProgressKey);
	static FGameplayTag MakeProgressTag(FName EncounterId, FName ProgressKey);
	static bool ConvertEncounterActionForTest(FName EncounterId, const FStoryEncounterAction& EncounterAction,
		FStoryAction& OutStoryAction);

private:
	bool CanTriggerNode(FName EncounterId, const FStoryEncounterNode& Node,
		const FStoryEventContext& Context) const;
	bool ExecuteActorEnabledAction(const FStoryEncounterAction& Action, const FStoryEventContext& Context) const;
	bool ExecuteTutorialAreaHintAction(const FStoryEncounterAction& Action, const FStoryEventContext& Context);
	bool ExecuteSpawnRewardPickupAction(const FStoryEncounterAction& Action, const FStoryEventContext& Context);
	void ExecuteEncounterAction(FName EncounterId, const FStoryEncounterAction& Action, const FStoryEventContext& Context);

	UFUNCTION()
	void HandleRewardDropCharacterDied(AYogCharacterBase* Character);

	UPROPERTY()
	TArray<TObjectPtr<ULevelInfoPopupDA>> TransientInfoPopups;

	TMap<TObjectKey<AYogCharacterBase>, TArray<FStoryEncounterAction>> PendingRewardDropActions;
	TSet<TObjectKey<AYogCharacterBase>> BoundRewardDropTargets;
};
