#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryEncounterTrigger.generated.h"

class UBoxComponent;
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情触发")
	FName NodeId;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "剧情触发")
	TObjectPtr<UBoxComponent> TriggerVolume = nullptr;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
