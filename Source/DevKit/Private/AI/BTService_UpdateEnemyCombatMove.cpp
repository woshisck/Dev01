// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_UpdateEnemyCombatMove.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Controller/YogAIController.h"

UBTService_UpdateEnemyCombatMove::UBTService_UpdateEnemyCombatMove()
{
	NodeName = TEXT("Update Enemy Combat Move");
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	MoveTargetLocationKey.SelectedKeyName = TEXT("MoveTargetLocation");
	DistanceToTargetKey.SelectedKeyName = TEXT("DistanceToTarget");
	bInAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");
	AcceptanceRadiusKey.SelectedKeyName = TEXT("AcceptanceRadius");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyCombatMove, TargetActorKey), AActor::StaticClass());
	MoveTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyCombatMove, MoveTargetLocationKey));
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyCombatMove, DistanceToTargetKey));
	bInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyCombatMove, bInAttackRangeKey));
	AcceptanceRadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyCombatMove, AcceptanceRadiusKey));
}

void UBTService_UpdateEnemyCombatMove::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AYogAIController* YogAI = Cast<AYogAIController>(OwnerComp.GetAIOwner());
	if (!YogAI)
	{
		return;
	}

	YogAI->UpdateCombatMoveBlackboard(
		OwnerComp.GetBlackboardComponent(),
		TargetActorKey.SelectedKeyName,
		MoveTargetLocationKey.SelectedKeyName,
		DistanceToTargetKey.SelectedKeyName,
		bInAttackRangeKey.SelectedKeyName,
		AcceptanceRadiusKey.SelectedKeyName);
}
