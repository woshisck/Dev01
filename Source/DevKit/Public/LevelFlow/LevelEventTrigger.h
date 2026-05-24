#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelEventTrigger.generated.h"

class UBoxComponent;
class UFlowComponent;
class ULevelFlowAsset;
class UStoryEncounterGraph;
class APlayerCharacterBase;
struct FHitResult;

/**
 * Level trigger used by story encounter points. Place it in a level, set the
 * trigger volume, then bind an EncounterGraph and NodeId.
 */
UCLASS(Blueprintable)
class DEVKIT_API ALevelEventTrigger : public AActor
{
	GENERATED_BODY()

public:
	ALevelEventTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story Encounter|Runtime")
	TObjectPtr<UFlowComponent> LevelFlowComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	TObjectPtr<UStoryEncounterGraph> EncounterGraph = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Encounter")
	bool bTriggerOnce = true;

public:
	FSimpleMulticastDelegate OnPlayerExited;

	UBoxComponent* GetTriggerVolume() const { return TriggerVolume; }
	bool ShouldTriggerOnce() const { return bTriggerOnce; }
	bool RunLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow = true);

private:
	UPROPERTY(VisibleAnywhere, Category = "Story Encounter|Debug")
	bool bTriggered = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
