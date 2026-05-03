#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_EnemyCombatMove.generated.h"

UCLASS()
class DEVKIT_API UBTTask_EnemyCombatMove : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyCombatMove();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveTargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector bInAttackRangeKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AcceptanceRadiusKey;

	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.0"))
	float TargetRefreshDistance = 80.0f;

private:
	bool IssueMove(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		bool bTargetMoved,
		float RepathInterval) const;
};
