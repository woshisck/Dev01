#include "AI/BTDecorator_EnemyPostAttackReposition.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_EnemyPostAttackReposition::UBTDecorator_EnemyPostAttackReposition()
{
	NodeName = TEXT("Enemy Post Attack Reposition");
	FlowAbortMode = EBTFlowAbortMode::Both;

	bPostAttackRepositionKey.SelectedKeyName = TEXT("bPostAttackReposition");
	bPostAttackRepositionKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTDecorator_EnemyPostAttackReposition, bPostAttackRepositionKey));
}

bool UBTDecorator_EnemyPostAttackReposition::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard && Blackboard->GetValueAsBool(bPostAttackRepositionKey.SelectedKeyName);
}

FString UBTDecorator_EnemyPostAttackReposition::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s == true"), *bPostAttackRepositionKey.SelectedKeyName.ToString());
}
