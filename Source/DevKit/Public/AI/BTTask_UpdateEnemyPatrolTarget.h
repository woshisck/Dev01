#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_UpdateEnemyPatrolTarget.generated.h"

UCLASS()
class DEVKIT_API UBTTask_UpdateEnemyPatrolTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UpdateEnemyPatrolTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolOriginLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolTargetLocationKey;
};
