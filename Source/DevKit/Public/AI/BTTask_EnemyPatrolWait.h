#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyPatrolWait.generated.h"

UCLASS()
class DEVKIT_API UBTTask_EnemyPatrolWait : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyPatrolWait();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;
};
