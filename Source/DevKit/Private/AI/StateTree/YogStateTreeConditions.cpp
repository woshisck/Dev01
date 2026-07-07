#include "AI/StateTree/YogStateTreeConditions.h"

#include "AIController.h"
#include "Character/YogCharacterBase.h"
#include "Controller/YogAIController.h"
#include "StateTreeExecutionContext.h"

bool FStateTreeCondition_EnemyAIState::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController);
	if (!YogAI)
	{
		return false;
	}

	return YogAI->GetEnemyAIState() == InstanceData.RequiredState;
}

bool FStateTreeCondition_IsDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAIController* AIC = InstanceData.AIController;
	const AYogCharacterBase* Char = AIC ? Cast<AYogCharacterBase>(AIC->GetPawn()) : nullptr;
	return Char && Char->bIsDead;
}
