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

	virtual void BeginPlay() override;
	virtual void Die() override;
	virtual void FinishDying() override;

private:
	void ResetDummy();

	FTimerHandle ResetTimerHandle;
};
