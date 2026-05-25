#pragma once

#include "CoreMinimal.h"
#include "Character/EnemyCharacterBase.h"
#include "TrainingDummyCharacter.generated.h"

/**
 * Training dummy that resets to full health instead of dying permanently.
 * Does not register with GameMode's kill count, so killing it won't trigger room completion.
 */
UCLASS()
class DEVKIT_API ATrainingDummyCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	ATrainingDummyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Keep legacy training-dummy behavior by default. Tutorial spawners can disable this
	// so the actor follows the normal death -> destroy lifecycle and respawns as a new mob.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training Dummy")
	bool bResetOnDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Training Dummy", meta = (ClampMin = "0.0"))
	float ResetDelay = 0.1f;

	virtual void BeginPlay() override;
	virtual void Die() override;
	virtual void FinishDying() override;

private:
	void ResetDummy();

	FTimerHandle ResetTimerHandle;
};
