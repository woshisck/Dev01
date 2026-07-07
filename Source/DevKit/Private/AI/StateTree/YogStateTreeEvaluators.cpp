#include "AI/StateTree/YogStateTreeEvaluators.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Controller/YogAIController.h"
#include "StateTreeExecutionContext.h"

void FStateTreeEvaluator_EnemyAwareness::TreeStart(FStateTreeExecutionContext& Context) const
{
	Refresh(Context);
}

void FStateTreeEvaluator_EnemyAwareness::Tick(FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	Refresh(Context);
}

void FStateTreeEvaluator_EnemyAwareness::Refresh(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController);
	if (!YogAI)
	{
		return;
	}

	UBlackboardComponent* Blackboard = YogAI->BlackboardComponent;
	if (Blackboard)
	{
		YogAI->UpdateAwarenessBlackboard(
			Blackboard,
			InstanceData.EnemyAIStateKey,
			InstanceData.TargetActorKey,
			InstanceData.LastKnownTargetLocationKey,
			InstanceData.AlertExpireTimeKey,
			InstanceData.LastSeenTargetTimeKey);

		InstanceData.TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(InstanceData.TargetActorKey));
		InstanceData.LastKnownTargetLocation = Blackboard->GetValueAsVector(InstanceData.LastKnownTargetLocationKey);
	}

	InstanceData.EnemyAIState = YogAI->GetEnemyAIState();
}

void FStateTreeEvaluator_EnemyCombatMove::Tick(FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController);
	if (!YogAI)
	{
		return;
	}

	UBlackboardComponent* Blackboard = YogAI->BlackboardComponent;
	if (!Blackboard)
	{
		return;
	}

	YogAI->UpdateCombatMoveBlackboard(
		Blackboard,
		InstanceData.TargetActorKey,
		InstanceData.MoveTargetLocationKey,
		InstanceData.DistanceToTargetKey,
		InstanceData.bInAttackRangeKey,
		InstanceData.AcceptanceRadiusKey);

	InstanceData.MoveTargetLocation = Blackboard->GetValueAsVector(InstanceData.MoveTargetLocationKey);
	InstanceData.DistanceToTarget = Blackboard->GetValueAsFloat(InstanceData.DistanceToTargetKey);
	InstanceData.bInAttackRange = Blackboard->GetValueAsBool(InstanceData.bInAttackRangeKey);
	InstanceData.AcceptanceRadius = Blackboard->GetValueAsFloat(InstanceData.AcceptanceRadiusKey);
}
