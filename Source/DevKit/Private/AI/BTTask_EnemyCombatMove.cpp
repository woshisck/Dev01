#include "AI/BTTask_EnemyCombatMove.h"

#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Controller/YogAIController.h"
#include "Data/EnemyData.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	struct FEnemyCombatMoveMemory
	{
		FVector LastMoveTarget = FVector::ZeroVector;
		float LastMoveRequestTime = -FLT_MAX;
		bool bHasMoveRequest = false;
	};
}

UBTTask_EnemyCombatMove::UBTTask_EnemyCombatMove()
{
	NodeName = TEXT("Enemy Combat Move");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	MoveTargetLocationKey.SelectedKeyName = TEXT("MoveTargetLocation");
	DistanceToTargetKey.SelectedKeyName = TEXT("DistanceToTarget");
	bInAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");
	AcceptanceRadiusKey.SelectedKeyName = TEXT("AcceptanceRadius");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyCombatMove, TargetActorKey), AActor::StaticClass());
	MoveTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyCombatMove, MoveTargetLocationKey));
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyCombatMove, DistanceToTargetKey));
	bInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyCombatMove, bInAttackRangeKey));
	AcceptanceRadiusKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyCombatMove, AcceptanceRadiusKey));
}

EBTNodeResult::Type UBTTask_EnemyCombatMove::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	if (Blackboard->GetValueAsBool(bInAttackRangeKey.SelectedKeyName))
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			AIC->StopMovement();
		}
		return EBTNodeResult::Succeeded;
	}

	return IssueMove(OwnerComp, NodeMemory, true, 0.0f) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

void UBTTask_EnemyCombatMove::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Blackboard->GetValueAsBool(bInAttackRangeKey.SelectedKeyName))
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			AIC->StopMovement();
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FEnemyCombatMoveMemory* Memory = reinterpret_cast<FEnemyCombatMoveMemory*>(NodeMemory);
	const FVector MoveTarget = Blackboard->GetValueAsVector(MoveTargetLocationKey.SelectedKeyName);
	const float CurrentTime = OwnerComp.GetWorld() ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;

	float RepathInterval = 0.25f;
	if (const AYogAIController* YogAI = Cast<AYogAIController>(OwnerComp.GetAIOwner()))
	{
		if (const UEnemyData* EnemyData = YogAI->GetPossessedEnemyData())
		{
			RepathInterval = FMath::Max(EnemyData->MovementTuning.RepathInterval, 0.05f);
		}
	}

	const bool bTargetMoved = !Memory->bHasMoveRequest
		|| FVector::DistSquared2D(Memory->LastMoveTarget, MoveTarget) >= FMath::Square(TargetRefreshDistance);
	if (bTargetMoved || CurrentTime - Memory->LastMoveRequestTime >= RepathInterval)
	{
		if (!IssueMove(OwnerComp, NodeMemory, bTargetMoved, RepathInterval))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}

void UBTTask_EnemyCombatMove::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (TaskResult == EBTNodeResult::Aborted)
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			AIC->StopMovement();
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

uint16 UBTTask_EnemyCombatMove::GetInstanceMemorySize() const
{
	return sizeof(FEnemyCombatMoveMemory);
}

FString UBTTask_EnemyCombatMove::GetStaticDescription() const
{
	return FString::Printf(TEXT("Move to %s until %s is true"),
		*MoveTargetLocationKey.SelectedKeyName.ToString(),
		*bInAttackRangeKey.SelectedKeyName.ToString());
}

bool UBTTask_EnemyCombatMove::IssueMove(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	bool bTargetMoved,
	float RepathInterval) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIC || !ControlledPawn || !Blackboard)
	{
		return false;
	}

	FVector MoveTarget = Blackboard->GetValueAsVector(MoveTargetLocationKey.SelectedKeyName);
	const float AcceptanceRadius = FMath::Max(Blackboard->GetValueAsFloat(AcceptanceRadiusKey.SelectedKeyName), 25.0f);
	const float DistanceToTarget = Blackboard->GetValueAsFloat(DistanceToTargetKey.SelectedKeyName);
	const UEnemyData* EnemyData = nullptr;
	if (const AYogAIController* YogAI = Cast<AYogAIController>(AIC))
	{
		EnemyData = YogAI->GetPossessedEnemyData();
	}
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();
	const float ExitRange = Tuning.AttackRange + FMath::Max(Tuning.AttackRangeExitBuffer, 0.0f);
	if (!Blackboard->GetValueAsBool(bInAttackRangeKey.SelectedKeyName)
		&& DistanceToTarget > ExitRange
		&& FVector::Dist2D(ControlledPawn->GetActorLocation(), MoveTarget) <= AcceptanceRadius + 10.0f)
	{
		AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
		if (TargetActor)
		{
			FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
			DirectionToTarget.Z = 0.0f;
			if (!DirectionToTarget.IsNearlyZero())
			{
				const float ProgressDistance = FMath::Max(Tuning.AcceptanceRadius + 140.0f, 180.0f);
				MoveTarget = ControlledPawn->GetActorLocation() + DirectionToTarget.GetSafeNormal2D() * ProgressDistance;
				if (UWorld* World = OwnerComp.GetWorld())
				{
					if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
					{
						FNavLocation NavLocation;
						if (NavSys->ProjectPointToNavigation(MoveTarget, NavLocation, FVector(220.0f, 220.0f, 300.0f)))
						{
							MoveTarget = NavLocation.Location;
						}
					}
				}
				Blackboard->SetValueAsVector(MoveTargetLocationKey.SelectedKeyName, MoveTarget);
			}
		}
	}

	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (UWorld* World = OwnerComp.GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			const FVector ProjectionExtent(500.0f, 500.0f, 800.0f);
			if (NavSys->ProjectPointToNavigation(MoveTarget, NavLocation, ProjectionExtent))
			{
				MoveTarget = NavLocation.Location;
				Blackboard->SetValueAsVector(MoveTargetLocationKey.SelectedKeyName, MoveTarget);
			}
			else if (TargetActor && DistanceToTarget > ExitRange)
			{
				FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
				DirectionToTarget.Z = 0.0f;
				if (!DirectionToTarget.IsNearlyZero())
				{
					const float ProgressDistance = FMath::Clamp(
						DistanceToTarget - ExitRange * 0.5f,
						FMath::Max(Tuning.AcceptanceRadius + 140.0f, 180.0f),
						FMath::Max(Tuning.ForwardTurnLeadDistance, 240.0f));
					const FVector ProgressTarget = ControlledPawn->GetActorLocation() + DirectionToTarget.GetSafeNormal2D() * ProgressDistance;
					if (NavSys->ProjectPointToNavigation(ProgressTarget, NavLocation, ProjectionExtent))
					{
						MoveTarget = NavLocation.Location;
					}
					else
					{
						MoveTarget = ProgressTarget;
						MoveTarget.Z = ControlledPawn->GetActorLocation().Z;
					}
					Blackboard->SetValueAsVector(MoveTargetLocationKey.SelectedKeyName, MoveTarget);
				}
			}
			else
			{
				MoveTarget.Z = ControlledPawn->GetActorLocation().Z;
				Blackboard->SetValueAsVector(MoveTargetLocationKey.SelectedKeyName, MoveTarget);
			}
		}
	}

	const EPathFollowingRequestResult::Type MoveResult = AIC->MoveToLocation(
		MoveTarget,
		AcceptanceRadius,
		false,
		true,
		true,
		false,
		nullptr,
		true);

	if (AYogAIController* YogAI = Cast<AYogAIController>(AIC))
	{
		YogAI->RecordCombatMoveRequestForDebug(
			MoveTarget,
			static_cast<int32>(MoveResult),
			bTargetMoved,
			RepathInterval,
			AcceptanceRadius);
	}

	FEnemyCombatMoveMemory* Memory = reinterpret_cast<FEnemyCombatMoveMemory*>(NodeMemory);
	Memory->LastMoveTarget = MoveTarget;
	Memory->LastMoveRequestTime = OwnerComp.GetWorld() ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;
	Memory->bHasMoveRequest = true;

	// A failed MoveTo request should not fail the behavior tree task. If it does, the combat selector
	// immediately restarts all attack branches and can starve movement entirely while spamming logs.
	return true;
}
