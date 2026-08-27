#include "AI/StateTree/YogStateTreeConditions.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AI/StateTree/YogStateTreeShared.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/YogCharacterBase.h"
#include "Controller/YogAIController.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

namespace
{
	const UAbilitySystemComponent* YogStateTreeGetASC(const AActor* Actor)
	{
		const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Actor);
		return ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	}
}

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

bool FStateTreeCondition_SelfHealthPercentBelow::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAIController* AIC = InstanceData.AIController;
	const APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	return YogStateTree::ResolveHealthPercent(YogStateTreeGetASC(Pawn)) <= InstanceData.Threshold;
}

bool FStateTreeCondition_TimeInCombatAtLeast::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController);
	if (!YogAI)
	{
		return false;
	}

	const float CombatStartTime = YogAI->GetCombatStartTime();
	if (CombatStartTime <= -FLT_MAX * 0.5f)
	{
		return false;
	}

	const UWorld* World = YogAI->GetWorld();
	if (!World)
	{
		return false;
	}

	return (World->GetTimeSeconds() - CombatStartTime) >= InstanceData.Seconds;
}

bool FStateTreeCondition_PlayerHealthPercentBelow::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAIController* AIC = InstanceData.AIController;
	if (!AIC)
	{
		return false;
	}

	AActor* TargetActor = nullptr;
	if (const UBlackboardComponent* BB = AIC->GetBlackboardComponent())
	{
		TargetActor = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
	}
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(AIC->GetWorld(), 0);
	}
	if (!TargetActor)
	{
		return false;
	}

	return YogStateTree::ResolveHealthPercent(YogStateTreeGetASC(TargetActor)) <= InstanceData.Threshold;
}

bool FStateTreeCondition_EnemyPostAttackReposition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const AAIController* AIC = InstanceData.AIController;
	const UBlackboardComponent* Blackboard = AIC ? AIC->GetBlackboardComponent() : nullptr;
	return Blackboard && Blackboard->GetValueAsBool(InstanceData.bPostAttackRepositionKey);
}

bool FStateTreeCondition_TargetWithin2DDistance::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Actor || !InstanceData.Target)
	{
		return false;
	}

	// Under StateTreeComponentSchema the Actor context resolves to the AIController,
	// whose own location never follows its pawn. Step down to the pawn so the
	// measurement is taken from the body rather than the controller's spawn point.
	const AActor* Self = InstanceData.Actor;
	if (const AController* SelfController = Cast<AController>(Self))
	{
		Self = SelfController->GetPawn();
	}
	if (!Self)
	{
		return false;
	}

	const float Distance2D = FVector::Dist2D(Self->GetActorLocation(), InstanceData.Target->GetActorLocation());
	return Distance2D <= InstanceData.Distance;
}

bool FStateTreeCondition_TargetBeyond2DDistance::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Actor || !InstanceData.Target)
	{
		return false;
	}

	const AActor* Self = InstanceData.Actor;
	if (const AController* SelfController = Cast<AController>(Self))
	{
		Self = SelfController->GetPawn();
	}
	if (!Self)
	{
		return false;
	}

	return FVector::Dist2D(Self->GetActorLocation(), InstanceData.Target->GetActorLocation()) > InstanceData.Distance;
}
