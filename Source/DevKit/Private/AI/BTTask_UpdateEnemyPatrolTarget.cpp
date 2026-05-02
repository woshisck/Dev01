#include "AI/BTTask_UpdateEnemyPatrolTarget.h"

#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyData.h"
#include "NavigationSystem.h"

UBTTask_UpdateEnemyPatrolTarget::UBTTask_UpdateEnemyPatrolTarget()
{
	NodeName = TEXT("Update Enemy Patrol Target");

	PatrolOriginLocationKey.SelectedKeyName = TEXT("PatrolOriginLocation");
	PatrolTargetLocationKey.SelectedKeyName = TEXT("PatrolTargetLocation");

	PatrolOriginLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_UpdateEnemyPatrolTarget, PatrolOriginLocationKey));
	PatrolTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_UpdateEnemyPatrolTarget, PatrolTargetLocationKey));
}

EBTNodeResult::Type UBTTask_UpdateEnemyPatrolTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();

	FVector PatrolOrigin = Blackboard->GetValueAsVector(PatrolOriginLocationKey.SelectedKeyName);
	if (PatrolOrigin.IsNearlyZero())
	{
		PatrolOrigin = Pawn->GetActorLocation();
		Blackboard->SetValueAsVector(PatrolOriginLocationKey.SelectedKeyName, PatrolOrigin);
	}

	FVector PatrolTarget = PatrolOrigin;
	if (UWorld* World = Pawn->GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			if (NavSys->GetRandomReachablePointInRadius(PatrolOrigin, Tuning.PatrolRadius, NavLocation))
			{
				PatrolTarget = NavLocation.Location;
			}
		}
	}

	Blackboard->SetValueAsVector(PatrolTargetLocationKey.SelectedKeyName, PatrolTarget);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_UpdateEnemyPatrolTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Patrol target from %s -> %s"),
		*PatrolOriginLocationKey.SelectedKeyName.ToString(),
		*PatrolTargetLocationKey.SelectedKeyName.ToString());
}
