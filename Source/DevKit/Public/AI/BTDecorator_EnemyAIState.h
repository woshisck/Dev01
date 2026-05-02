#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Data/EnemyData.h"
#include "BTDecorator_EnemyAIState.generated.h"

UCLASS()
class DEVKIT_API UBTDecorator_EnemyAIState : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_EnemyAIState();

	virtual FString GetStaticDescription() const override;

	void SetRequiredState(EEnemyAIState InRequiredState) { RequiredState = InRequiredState; }

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector EnemyAIStateKey;

	UPROPERTY(EditAnywhere, Category = "Condition")
	EEnemyAIState RequiredState = EEnemyAIState::Patrol;
};
