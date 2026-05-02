#include "AI/BTTask_EnemyPatrolWait.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyData.h"

namespace
{
struct FEnemyPatrolWaitMemory
{
	float RemainingTime = 0.0f;
};
}

UBTTask_EnemyPatrolWait::UBTTask_EnemyPatrolWait()
{
	NodeName = TEXT("Enemy Patrol Wait");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyPatrolWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIC = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();

	const float MinWait = FMath::Max(0.0f, Tuning.PatrolWaitMin);
	const float MaxWait = FMath::Max(MinWait, Tuning.PatrolWaitMax);
	const float WaitTime = FMath::FRandRange(MinWait, MaxWait);
	if (WaitTime <= 0.0f)
	{
		return EBTNodeResult::Succeeded;
	}

	FEnemyPatrolWaitMemory* Memory = reinterpret_cast<FEnemyPatrolWaitMemory*>(NodeMemory);
	Memory->RemainingTime = WaitTime;
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyPatrolWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FEnemyPatrolWaitMemory* Memory = reinterpret_cast<FEnemyPatrolWaitMemory*>(NodeMemory);
	Memory->RemainingTime -= DeltaSeconds;
	if (Memory->RemainingTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_EnemyPatrolWait::GetInstanceMemorySize() const
{
	return sizeof(FEnemyPatrolWaitMemory);
}

FString UBTTask_EnemyPatrolWait::GetStaticDescription() const
{
	return TEXT("Wait using EnemyData.AwarenessTuning PatrolWaitMin/PatrolWaitMax");
}
