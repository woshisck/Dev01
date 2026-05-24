#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryEncounterTrigger.generated.h"

class UBoxComponent;
class UFlowComponent;
class ULevelFlowAsset;
class UStoryEncounterGraph;
class UStoryEncounterMap;
class UStoryEncounterPointDA;

UCLASS(Blueprintable)
class DEVKIT_API AStoryEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	AStoryEncounterTrigger();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情触发")
	TObjectPtr<UStoryEncounterPointDA> EncounterPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情触发")
	TObjectPtr<UStoryEncounterMap> EncounterMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UStoryEncounterGraph> EncounterGraph = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情触发")
	FName NodeId;

	FSimpleMulticastDelegate OnPlayerExited;

	UBoxComponent* GetTriggerVolume() const { return TriggerVolume; }
	bool RunLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow = true);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "剧情触发")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "剧情触发")
	TObjectPtr<UFlowComponent> LevelFlowComp = nullptr;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool ShouldBlockRepeatTrigger() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "剧情触发|Debug")
	bool bTriggered = false;
};
