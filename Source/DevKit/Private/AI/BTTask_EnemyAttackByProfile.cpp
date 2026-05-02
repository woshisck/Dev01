#include "AI/BTTask_EnemyAttackByProfile.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBTTask_EnemyAttackByProfile::UBTTask_EnemyAttackByProfile()
{
	NodeName = TEXT("Enemy Attack By Profile");
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;

	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	DistanceToTargetKey.SelectedKeyName = TEXT("DistanceToTarget");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttackByProfile, TargetActorKey), AActor::StaticClass());
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttackByProfile, DistanceToTargetKey));
}

EBTNodeResult::Type UBTTask_EnemyAttackByProfile::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ClearActiveAbilityDelegate();
	StopFlash();
	ActiveAbilityTags.Reset();

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!AIC || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	if (AYogAIController* YogAI = Cast<AYogAIController>(AIC))
	{
		YogAI->EnterCombat(ResolveTargetActor(OwnerComp), false);
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (!YogASC)
	{
		return EBTNodeResult::Failed;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Character ? Character->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const UAbilityData* AbilityData = EnemyData ? EnemyData->AbilityData : nullptr;
	if (!EnemyData || !AbilityData || EnemyData->AttackProfile.Attacks.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	const float CurrentTime = Pawn->GetWorld() ? Pawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const float DistanceToTarget = ResolveDistanceToTarget(OwnerComp, *Pawn);
	AttackCooldownEndTimes.SetNum(EnemyData->AttackProfile.Attacks.Num());

	TArray<FAttackCandidate> Candidates;
	float TotalWeight = 0.0f;

	for (int32 AttackIndex = 0; AttackIndex < EnemyData->AttackProfile.Attacks.Num(); ++AttackIndex)
	{
		const FEnemyAIAttackOption& Attack = EnemyData->AttackProfile.Attacks[AttackIndex];
		if (Attack.Weight <= 0.0f || Attack.AbilityTags.IsEmpty())
		{
			continue;
		}

		const bool bInsideMinRange = DistanceToTarget >= Attack.MinRange;
		const bool bInsideMaxRange = Attack.MaxRange <= 0.0f || DistanceToTarget <= Attack.MaxRange;
		if (!bInsideMinRange || !bInsideMaxRange)
		{
			continue;
		}

		if (AttackCooldownEndTimes.IsValidIndex(AttackIndex) && CurrentTime < AttackCooldownEndTimes[AttackIndex])
		{
			continue;
		}

		FGameplayTagContainer ValidTags;
		for (const FGameplayTag& Tag : Attack.AbilityTags)
		{
			if (AbilityData->HasAbility(Tag))
			{
				ValidTags.AddTag(Tag);
			}
		}

		if (ValidTags.IsEmpty())
		{
			continue;
		}

		FAttackCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.AttackIndex = AttackIndex;
		Candidate.Weight = Attack.Weight;
		Candidate.ValidTags = MoveTemp(ValidTags);
		TotalWeight += Attack.Weight;
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return EBTNodeResult::Failed;
	}

	const FAttackCandidate* ChosenCandidate = nullptr;
	float Pick = FMath::FRandRange(0.0f, TotalWeight);
	for (const FAttackCandidate& Candidate : Candidates)
	{
		Pick -= Candidate.Weight;
		if (Pick <= 0.0f)
		{
			ChosenCandidate = &Candidate;
			break;
		}
	}

	if (!ChosenCandidate)
	{
		ChosenCandidate = &Candidates.Last();
	}

	const FEnemyAIAttackOption& ChosenAttack = EnemyData->AttackProfile.Attacks[ChosenCandidate->AttackIndex];
	if (ChosenAttack.bPreAttackFlash && Character)
	{
		Character->StartPreAttackFlash();
		FlashCharacter = Character;
	}

	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(ChosenCandidate->ValidTags, false);
	if (!bActivated)
	{
		StopFlash();
		return EBTNodeResult::Failed;
	}

	if (AttackCooldownEndTimes.IsValidIndex(ChosenCandidate->AttackIndex))
	{
		AttackCooldownEndTimes[ChosenCandidate->AttackIndex] = CurrentTime + ChosenAttack.Cooldown;
	}

	ActiveAbilityTags = ChosenCandidate->ValidTags;

	bool bStillActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->AbilityTags.HasAny(ActiveAbilityTags))
		{
			bStillActive = true;
			break;
		}
	}

	if (!bStillActive)
	{
		StopFlash();
		ActiveAbilityTags.Reset();
		return EBTNodeResult::Succeeded;
	}

	ActiveASC = ASC;
	TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner(&OwnerComp);
	ActiveEndHandle = ASC->OnAbilityEnded.AddLambda(
		[this, WeakOwner](const FAbilityEndedData& Data)
		{
			if (!WeakOwner.IsValid())
			{
				return;
			}

			if (Data.AbilityThatEnded && Data.AbilityThatEnded->AbilityTags.HasAny(ActiveAbilityTags))
			{
				FinishLatentTask(*WeakOwner.Get(), EBTNodeResult::Succeeded);
			}
		});

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttackByProfile::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	ClearActiveAbilityDelegate();
	StopFlash();
	ActiveAbilityTags.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_EnemyAttackByProfile::GetStaticDescription() const
{
	return FString::Printf(TEXT("Profile attack. Target: %s, Distance: %s"),
		*TargetActorKey.SelectedKeyName.ToString(),
		*DistanceToTargetKey.SelectedKeyName.ToString());
}

float UBTTask_EnemyAttackByProfile::ResolveDistanceToTarget(const UBehaviorTreeComponent& OwnerComp, const APawn& Pawn) const
{
	if (const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		if (!DistanceToTargetKey.SelectedKeyName.IsNone())
		{
			const float BlackboardDistance = Blackboard->GetValueAsFloat(DistanceToTargetKey.SelectedKeyName);
			if (BlackboardDistance > 0.0f)
			{
				return BlackboardDistance;
			}
		}
	}

	if (const AActor* TargetActor = ResolveTargetActor(OwnerComp))
	{
		return FVector::Dist2D(Pawn.GetActorLocation(), TargetActor->GetActorLocation());
	}

	return TNumericLimits<float>::Max();
}

AActor* UBTTask_EnemyAttackByProfile::ResolveTargetActor(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard)
	{
		if (!TargetActorKey.SelectedKeyName.IsNone())
		{
			if (AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)))
			{
				return TargetActor;
			}
		}

		if (AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))))
		{
			return TargetActor;
		}

		if (AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("FindPlayer"))))
		{
			return TargetActor;
		}
	}

	return UGameplayStatics::GetPlayerPawn(OwnerComp.GetWorld(), 0);
}

void UBTTask_EnemyAttackByProfile::ClearActiveAbilityDelegate()
{
	if (ActiveASC.IsValid() && ActiveEndHandle.IsValid())
	{
		ActiveASC->OnAbilityEnded.Remove(ActiveEndHandle);
	}

	ActiveASC.Reset();
	ActiveEndHandle.Reset();
}

void UBTTask_EnemyAttackByProfile::StopFlash()
{
	if (FlashCharacter.IsValid())
	{
		FlashCharacter->StopPreAttackFlash();
		FlashCharacter.Reset();
	}
}
