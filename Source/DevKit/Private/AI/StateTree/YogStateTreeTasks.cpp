#include "AI/StateTree/YogStateTreeTasks.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/EnemyData.h"
#include "NavigationSystem.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

// ─── Activate Ability By Tag ────────────────────────────────────────────────

FStateTreeTask_ActivateAbilityByTag::FStateTreeTask_ActivateAbilityByTag()
{
	// Completion is delegate-driven; no per-frame tick needed.
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_ActivateAbilityByTag::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (!YogASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Only keep tags the pawn's AbilityData actually owns a montage/GA for.
	FGameplayTagContainer ValidTags;
	if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn))
	{
		if (const UCharacterDataComponent* DC = Char->GetCharacterDataComponent())
		{
			if (const UCharacterData* CD = DC->GetCharacterData())
			{
				if (const UAbilityData* AD = CD->AbilityData)
				{
					for (const FGameplayTag& Tag : InstanceData.AbilityTags)
					{
						if (AD->HasAbility(Tag))
						{
							ValidTags.AddTag(Tag);
						}
					}
				}
			}
		}
	}

	if (ValidTags.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bPreAttackFlash)
	{
		if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn))
		{
			Char->StartPreAttackFlash();
			InstanceData.FlashCharacter = Char;
		}
	}

	if (!YogASC->TryActivateRandomAbilitiesByTag(ValidTags, false))
	{
		if (InstanceData.FlashCharacter.IsValid())
		{
			InstanceData.FlashCharacter->StopPreAttackFlash();
			InstanceData.FlashCharacter.Reset();
		}
		return EStateTreeRunStatus::Failed;
	}

	// If no matching GA is still active, it ended synchronously (e.g. no montage).
	bool bStillActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->AbilityTags.HasAny(ValidTags))
		{
			bStillActive = true;
			break;
		}
	}

	if (!bStillActive)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.ActiveASC = ASC;
	InstanceData.EndHandle = ASC->OnAbilityEnded.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(), ValidTags](const FAbilityEndedData& Data)
		{
			if (Data.AbilityThatEnded && Data.AbilityThatEnded->AbilityTags.HasAny(ValidTags))
			{
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		});

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_ActivateAbilityByTag::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.ActiveASC.IsValid() && InstanceData.EndHandle.IsValid())
	{
		InstanceData.ActiveASC->OnAbilityEnded.Remove(InstanceData.EndHandle);
	}
	InstanceData.ActiveASC.Reset();
	InstanceData.EndHandle.Reset();

	if (InstanceData.FlashCharacter.IsValid())
	{
		InstanceData.FlashCharacter->StopPreAttackFlash();
		InstanceData.FlashCharacter.Reset();
	}
}

// ─── Play Dead ──────────────────────────────────────────────────────────────

FStateTreeTask_PlayDead::FStateTreeTask_PlayDead()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_PlayDead::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	// Terminal state: stay Running so the pawn holds the dead pose.
	return EStateTreeRunStatus::Running;
}

// ─── Update Enemy Patrol Target ─────────────────────────────────────────────

FStateTreeTask_UpdateEnemyPatrolTarget::FStateTreeTask_UpdateEnemyPatrolTarget()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_UpdateEnemyPatrolTarget::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();

	if (!InstanceData.bHasOrigin)
	{
		InstanceData.PatrolOrigin = Pawn->GetActorLocation();
		InstanceData.bHasOrigin = true;
	}

	FVector PatrolTarget = InstanceData.PatrolOrigin;
	if (UWorld* World = Pawn->GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			if (NavSys->GetRandomReachablePointInRadius(InstanceData.PatrolOrigin, Tuning.PatrolRadius, NavLocation))
			{
				PatrolTarget = NavLocation.Location;
			}
		}
	}

	InstanceData.PatrolTargetLocation = PatrolTarget;
	return EStateTreeRunStatus::Succeeded;
}

// ─── Enemy Patrol Wait ──────────────────────────────────────────────────────

EStateTreeRunStatus FStateTreeTask_EnemyPatrolWait::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();

	const float MinWait = FMath::Max(0.0f, Tuning.PatrolWaitMin);
	const float MaxWait = FMath::Max(MinWait, Tuning.PatrolWaitMax);
	const float WaitTime = FMath::FRandRange(MinWait, MaxWait);
	if (WaitTime <= 0.0f)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.RemainingTime = WaitTime;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_EnemyPatrolWait::Tick(
	FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.RemainingTime -= DeltaTime;
	return InstanceData.RemainingTime <= 0.0f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
