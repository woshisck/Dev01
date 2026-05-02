#include "AI/BTDecorator_EnemyAIState.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_EnemyAIState::UBTDecorator_EnemyAIState()
{
	NodeName = TEXT("Enemy AI State");
	FlowAbortMode = EBTFlowAbortMode::Both;

	EnemyAIStateKey.SelectedKeyName = TEXT("EnemyAIState");
	EnemyAIStateKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_EnemyAIState, EnemyAIStateKey), StaticEnum<EEnemyAIState>());
}

bool UBTDecorator_EnemyAIState::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}

	return static_cast<EEnemyAIState>(Blackboard->GetValueAsEnum(EnemyAIStateKey.SelectedKeyName)) == RequiredState;
}

FString UBTDecorator_EnemyAIState::GetStaticDescription() const
{
	const UEnum* Enum = StaticEnum<EEnemyAIState>();
	const FString StateName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(RequiredState)) : TEXT("Unknown");
	return FString::Printf(TEXT("%s == %s"), *EnemyAIStateKey.SelectedKeyName.ToString(), *StateName);
}
