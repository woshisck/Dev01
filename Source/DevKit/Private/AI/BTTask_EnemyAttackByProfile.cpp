#include "AI/BTTask_EnemyAttackByProfile.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyAIAttackDecision, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarEnemyAIAttackDecisionLog(
		TEXT("DevKit.EnemyAI.AttackDecisionLog"),
		0,
		TEXT("0=off, 1=attack task failure and selected attack logs."));

	const TCHAR* AttackMovementModeToString(EEnemyAIAttackMovementMode Mode)
	{
		switch (Mode)
		{
		case EEnemyAIAttackMovementMode::None:
			return TEXT("None");
		case EEnemyAIAttackMovementMode::RadialLunge:
			return TEXT("RadialLunge");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* AttackRoleToString(EEnemyAIAttackRole Role)
	{
		switch (Role)
		{
		case EEnemyAIAttackRole::CloseMelee:
			return TEXT("CloseMelee");
		case EEnemyAIAttackRole::SpecialMovement:
			return TEXT("SpecialMovement");
		case EEnemyAIAttackRole::Skill:
			return TEXT("Skill");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* AttackRoleSelectedResult(EEnemyAIAttackRole Role)
	{
		switch (Role)
		{
		case EEnemyAIAttackRole::CloseMelee:
			return TEXT("CloseMeleeSelected");
		case EEnemyAIAttackRole::SpecialMovement:
			return TEXT("SpecialMovementSelected");
		case EEnemyAIAttackRole::Skill:
			return TEXT("SkillSelected");
		default:
			return TEXT("Selected");
		}
	}

	const TCHAR* AttackRoleBlockedResult(EEnemyAIAttackRole Role)
	{
		switch (Role)
		{
		case EEnemyAIAttackRole::CloseMelee:
			return TEXT("CloseMeleeBlocked");
		case EEnemyAIAttackRole::SpecialMovement:
			return TEXT("SpecialMovementBlocked");
		case EEnemyAIAttackRole::Skill:
			return TEXT("SkillBlocked");
		default:
			return TEXT("NoCandidate");
		}
	}

	bool FacePawnTowardsTarget(APawn& Pawn, const AActor* TargetActor, float* OutYawDelta = nullptr)
	{
		if (!TargetActor)
		{
			return false;
		}

		FVector Direction = TargetActor->GetActorLocation() - Pawn.GetActorLocation();
		Direction.Z = 0.0f;
		if (Direction.IsNearlyZero())
		{
			return false;
		}

		const float PreviousYaw = Pawn.GetActorRotation().Yaw;
		const FRotator FaceTargetRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
		Pawn.SetActorRotation(FaceTargetRotation);
		if (AController* Controller = Pawn.GetController())
		{
			Controller->SetControlRotation(FaceTargetRotation);
		}

		if (OutYawDelta)
		{
			*OutYawDelta = FMath::FindDeltaAngleDegrees(PreviousYaw, FaceTargetRotation.Yaw);
		}
		return true;
	}

	bool TargetHasSmokeAttackBlock(const AActor* TargetActor)
	{
		const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);
		const UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
		const FGameplayTag InSmokeTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.InSmoke"), false);
		return TargetASC && InSmokeTag.IsValid() && TargetASC->HasMatchingGameplayTag(InSmokeTag);
	}
}

UBTTask_EnemyAttackByProfile::UBTTask_EnemyAttackByProfile()
{
	NodeName = TEXT("Enemy Attack By Profile");
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;

	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	DistanceToTargetKey.SelectedKeyName = TEXT("DistanceToTarget");
	bInAttackRangeKey.SelectedKeyName = TEXT("bInAttackRange");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttackByProfile, TargetActorKey), AActor::StaticClass());
	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttackByProfile, DistanceToTargetKey));
	bInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyAttackByProfile, bInAttackRangeKey));
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

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard)
	{
		if (!bInAttackRangeKey.SelectedKeyName.IsNone()
			&& Blackboard->GetKeyID(bInAttackRangeKey.SelectedKeyName) != FBlackboard::InvalidKey
			&& !Blackboard->GetValueAsBool(bInAttackRangeKey.SelectedKeyName))
		{
			if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogEnemyAIAttackDecision, Log,
					TEXT("[EnemyAIAttack] Enemy=%s Result=%s Reason=BlackboardOutOfAttackRange Role=%s Target=%s Distance=%.1f"),
					*GetNameSafe(Pawn),
					AttackRoleBlockedResult(RequiredAttackRole),
					AttackRoleToString(RequiredAttackRole),
					*GetNameSafe(ResolveTargetActor(OwnerComp)),
					ResolveDistanceToTarget(OwnerComp, *Pawn));
			}
			return EBTNodeResult::Failed;
		}
	}

	AActor* TargetActorForBlock = ResolveTargetActor(OwnerComp);
	if (TargetHasSmokeAttackBlock(TargetActorForBlock))
	{
		if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyAIAttackDecision, Log,
				TEXT("[EnemyAIAttack] Enemy=%s Result=%s Reason=TargetInSmoke Role=%s Target=%s Distance=%.1f"),
				*GetNameSafe(Pawn),
				AttackRoleBlockedResult(RequiredAttackRole),
				AttackRoleToString(RequiredAttackRole),
				*GetNameSafe(TargetActorForBlock),
				ResolveDistanceToTarget(OwnerComp, *Pawn));
		}
		return EBTNodeResult::Failed;
	}

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (!YogASC)
	{
		if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyAIAttackDecision, Log,
				TEXT("[EnemyAIAttack] Enemy=%s Result=Fail Reason=MissingYogASC"),
				*GetNameSafe(Pawn));
		}
		return EBTNodeResult::Failed;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Character ? Character->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const UAbilityData* AbilityData = EnemyData ? EnemyData->AbilityData : nullptr;
	if (!EnemyData || !AbilityData || EnemyData->AttackProfile.Attacks.IsEmpty())
	{
		if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyAIAttackDecision, Log,
				TEXT("[EnemyAIAttack] Enemy=%s Result=Fail Reason=MissingEnemyDataOrAttackProfile EnemyData=%s AbilityData=%s AttackCount=%d"),
				*GetNameSafe(Pawn),
				*GetNameSafe(EnemyData),
				*GetNameSafe(AbilityData),
				EnemyData ? EnemyData->AttackProfile.Attacks.Num() : 0);
		}
		return EBTNodeResult::Failed;
	}

	AYogAIController* YogAI = Cast<AYogAIController>(AIC);
	const float CurrentTime = Pawn->GetWorld() ? Pawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const float DistanceToTarget = ResolveDistanceToTarget(OwnerComp, *Pawn);
	if (YogAI)
	{
		YogAI->RefreshMovementAttackCooldownReset(DistanceToTarget);
	}
	AttackCooldownEndTimes.SetNum(EnemyData->AttackProfile.Attacks.Num());

	TArray<FAttackCandidate> Candidates;
	float TotalWeight = 0.0f;
	int32 RejectedNoWeightOrTags = 0;
	int32 RejectedMinRange = 0;
	int32 RejectedMaxRange = 0;
	int32 RejectedCooldown = 0;
	int32 RejectedMovementAttackCooldown = 0;
	int32 RejectedNoValidAbilityTag = 0;
	int32 RejectedWrongRole = 0;
	int32 PenalizedRecentRepeat = 0;
	float CloseCombatRange = EnemyData->MovementTuning.AttackRange;
	for (const FEnemyAIAttackOption& Attack : EnemyData->AttackProfile.Attacks)
	{
		if (Attack.AttackRole == EEnemyAIAttackRole::CloseMelee && Attack.MaxRange > 0.0f)
		{
			CloseCombatRange = FMath::Max(CloseCombatRange, Attack.MaxRange);
		}
	}

	for (int32 AttackIndex = 0; AttackIndex < EnemyData->AttackProfile.Attacks.Num(); ++AttackIndex)
	{
		const FEnemyAIAttackOption& Attack = EnemyData->AttackProfile.Attacks[AttackIndex];
		if (Attack.AttackRole != RequiredAttackRole)
		{
			++RejectedWrongRole;
			continue;
		}

		if (Attack.Weight <= 0.0f || Attack.AbilityTags.IsEmpty())
		{
			++RejectedNoWeightOrTags;
			continue;
		}

		float RequiredMinRange = Attack.MinRange;
		if (Attack.AttackRole == EEnemyAIAttackRole::SpecialMovement)
		{
			RequiredMinRange = FMath::Max(FMath::Max(Attack.MinRange, Attack.LungeStartRange), CloseCombatRange);
		}

		const bool bMovementAttackWindow = Attack.AttackMovementMode != EEnemyAIAttackMovementMode::None
			&& DistanceToTarget >= RequiredMinRange;
		if (Attack.AttackMovementMode != EEnemyAIAttackMovementMode::None && !bMovementAttackWindow)
		{
			++RejectedMinRange;
			continue;
		}

		const float EffectiveMaxRange = bMovementAttackWindow && YogAI
			? YogAI->GetMovementAttackRange(Attack)
			: Attack.MaxRange;
		const bool bInsideMinRange = DistanceToTarget >= RequiredMinRange;
		const bool bInsideMaxRange = EffectiveMaxRange <= 0.0f || DistanceToTarget <= EffectiveMaxRange;
		if (!bInsideMinRange || !bInsideMaxRange)
		{
			if (!bInsideMinRange)
			{
				++RejectedMinRange;
			}
			if (!bInsideMaxRange)
			{
				++RejectedMaxRange;
			}
			continue;
		}

		if (bMovementAttackWindow && YogAI)
		{
			float RemainingMovementAttackCooldown = 0.0f;
			if (!YogAI->CanUseMovementAttack(Attack, DistanceToTarget, &RemainingMovementAttackCooldown))
			{
				++RejectedMovementAttackCooldown;
				if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
				{
					UE_LOG(LogEnemyAIAttackDecision, Log,
						TEXT("[EnemyAIAttack] Enemy=%s Result=Skip Attack=%s Reason=MovementAttackCooldown Distance=%.1f MoveRange=%.1f Remaining=%.2f"),
						*GetNameSafe(Pawn),
						*Attack.AttackName.ToString(),
						DistanceToTarget,
						YogAI->GetMovementAttackRange(Attack),
						RemainingMovementAttackCooldown);
				}
				continue;
			}
		}

		if (AttackCooldownEndTimes.IsValidIndex(AttackIndex) && CurrentTime < AttackCooldownEndTimes[AttackIndex])
		{
			++RejectedCooldown;
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
			++RejectedNoValidAbilityTag;
			if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogEnemyAIAttackDecision, Log,
					TEXT("[EnemyAIAttack] Enemy=%s Result=Skip Attack=%s Reason=NoValidAbilityTag ConfiguredTags=%s AbilityData=%s Distance=%.1f MoveMode=%s"),
					*GetNameSafe(Pawn),
					*Attack.AttackName.ToString(),
					*Attack.AbilityTags.ToStringSimple(),
					*GetNameSafe(AbilityData),
					DistanceToTarget,
					AttackMovementModeToString(Attack.AttackMovementMode));
			}
			continue;
		}

		FAttackCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.AttackIndex = AttackIndex;
		Candidate.Weight = Attack.Weight;
		Candidate.ValidTags = MoveTemp(ValidTags);
		TotalWeight += Attack.Weight;
	}

	if (YogAI
		&& Candidates.Num() > 1
		&& EnemyData->AttackProfile.RecentAttackMemoryDuration > 0.0f
		&& EnemyData->AttackProfile.RepeatAttackWeightMultiplier < 1.0f)
	{
		TotalWeight = 0.0f;
		for (FAttackCandidate& Candidate : Candidates)
		{
			const FEnemyAIAttackOption& CandidateAttack = EnemyData->AttackProfile.Attacks[Candidate.AttackIndex];
			float RepeatAge = 0.0f;
			if (YogAI->IsRecentAttackRepeat(CandidateAttack, EnemyData->AttackProfile.RecentAttackMemoryDuration, &RepeatAge))
			{
				Candidate.Weight *= FMath::Max(EnemyData->AttackProfile.RepeatAttackWeightMultiplier, 0.0f);
				++PenalizedRecentRepeat;
			}
			TotalWeight += Candidate.Weight;
		}
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		const bool bMayClearInRange = RequiredAttackRole == EEnemyAIAttackRole::CloseMelee
			&& Blackboard
			&& !bInAttackRangeKey.SelectedKeyName.IsNone()
			&& DistanceToTarget > CloseCombatRange;
		if (bMayClearInRange)
		{
			Blackboard->SetValueAsBool(bInAttackRangeKey.SelectedKeyName, false);
		}
		if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyAIAttackDecision, Log,
				TEXT("[EnemyAIAttack] Enemy=%s Result=%s Reason=NoCandidate Role=%s Target=%s Distance=%.1f CloseCombatRange=%.1f SetInRangeFalse=%d Rejected{WrongRole=%d,NoWeightOrTags=%d,MinRange=%d,MaxRange=%d,Cooldown=%d,MovementCooldown=%d,NoValidAbilityTag=%d,RecentRepeatPenalty=%d}"),
				*GetNameSafe(Pawn),
				AttackRoleBlockedResult(RequiredAttackRole),
				AttackRoleToString(RequiredAttackRole),
				*GetNameSafe(ResolveTargetActor(OwnerComp)),
				DistanceToTarget,
				CloseCombatRange,
				bMayClearInRange ? 1 : 0,
				RejectedWrongRole,
				RejectedNoWeightOrTags,
				RejectedMinRange,
				RejectedMaxRange,
				RejectedCooldown,
				RejectedMovementAttackCooldown,
				RejectedNoValidAbilityTag,
				PenalizedRecentRepeat);
		}
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
	AActor* TargetActor = ResolveTargetActor(OwnerComp);
	if (YogAI)
	{
		YogAI->EnterCombat(TargetActor, false);
	}
	AIC->StopMovement();

	float AttackFaceYawDelta = 0.0f;
	const bool bFacedTarget = FacePawnTowardsTarget(*Pawn, TargetActor, &AttackFaceYawDelta);

	AEnemyCharacterBase* EnemyCharacter = Cast<AEnemyCharacterBase>(Pawn);
	if (EnemyCharacter)
	{
		EnemyCharacter->SetAIAttackRuntimeContext(ChosenAttack, TargetActor, DistanceToTarget);
	}
	if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogEnemyAIAttackDecision, Log,
			TEXT("[EnemyAIAttack] Enemy=%s Result=%s Role=%s Attack=%s Tags=%s Distance=%.1f Min=%.1f Max=%.1f Weight=%.2f TotalWeight=%.2f Cooldown=%.2f MoveMode=%s LungeStart=%.1f FacedTarget=%d FaceYawDelta=%.1f RecentRepeatPenalty=%d"),
			*GetNameSafe(Pawn),
			AttackRoleSelectedResult(RequiredAttackRole),
			AttackRoleToString(RequiredAttackRole),
			*ChosenAttack.AttackName.ToString(),
			*ChosenCandidate->ValidTags.ToStringSimple(),
			DistanceToTarget,
			ChosenAttack.MinRange,
			ChosenAttack.MaxRange,
			ChosenCandidate->Weight,
			TotalWeight,
			ChosenAttack.Cooldown,
			AttackMovementModeToString(ChosenAttack.AttackMovementMode),
			ChosenAttack.LungeStartRange,
			bFacedTarget ? 1 : 0,
			AttackFaceYawDelta,
			PenalizedRecentRepeat);
	}

	if (YogAI)
	{
		YogAI->SetCombatAttackInProgress(true);
	}

	if (ChosenAttack.bPreAttackFlash && Character)
	{
		Character->StartPreAttackFlash();
		FlashCharacter = Character;
	}

	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(ChosenCandidate->ValidTags, false);
	if (!bActivated)
	{
		if (Blackboard && !bInAttackRangeKey.SelectedKeyName.IsNone() && DistanceToTarget > CloseCombatRange)
		{
			Blackboard->SetValueAsBool(bInAttackRangeKey.SelectedKeyName, false);
		}
		if (CVarEnemyAIAttackDecisionLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyAIAttackDecision, Log,
				TEXT("[EnemyAIAttack] Enemy=%s Result=Fail Reason=ActivateAbilityFailed Attack=%s Tags=%s Distance=%.1f SetInRangeFalse=%d"),
				*GetNameSafe(Pawn),
				*ChosenAttack.AttackName.ToString(),
				*ChosenCandidate->ValidTags.ToStringSimple(),
				DistanceToTarget,
				Blackboard && !bInAttackRangeKey.SelectedKeyName.IsNone() && DistanceToTarget > CloseCombatRange ? 1 : 0);
		}
		if (EnemyCharacter)
		{
			EnemyCharacter->ClearAIAttackRuntimeContext();
		}
		if (YogAI)
		{
			YogAI->SetCombatAttackInProgress(false);
			YogAI->ResetCombatMoveSmoothingAfterAttack();
		}
		StopFlash();
		return EBTNodeResult::Failed;
	}

	if (AttackCooldownEndTimes.IsValidIndex(ChosenCandidate->AttackIndex))
	{
		AttackCooldownEndTimes[ChosenCandidate->AttackIndex] = CurrentTime + ChosenAttack.Cooldown;
	}
	if (YogAI)
	{
		YogAI->NotifyAttackActivated(ChosenAttack);
	}
	if (YogAI
		&& ChosenAttack.AttackMovementMode != EEnemyAIAttackMovementMode::None
		&& DistanceToTarget >= ChosenAttack.LungeStartRange)
	{
		YogAI->NotifyMovementAttackActivated(ChosenAttack);
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
		if (YogAI)
		{
			YogAI->SetCombatAttackInProgress(false);
		}
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
				if (AYogAIController* EndYogAI = Cast<AYogAIController>(WeakOwner->GetAIOwner()))
				{
					EndYogAI->SetCombatAttackInProgress(false);
					EndYogAI->ResetCombatMoveSmoothingAfterAttack();
				}
				FinishLatentTask(*WeakOwner.Get(), EBTNodeResult::Succeeded);
			}
		});

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyAttackByProfile::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AYogAIController* YogAI = Cast<AYogAIController>(OwnerComp.GetAIOwner()))
	{
		YogAI->SetCombatAttackInProgress(false);
	}
	ClearActiveAbilityDelegate();
	StopFlash();
	ActiveAbilityTags.Reset();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_EnemyAttackByProfile::GetStaticDescription() const
{
	return FString::Printf(TEXT("Profile attack. Role: %s, Target: %s, Distance: %s, InRange: %s"),
		AttackRoleToString(RequiredAttackRole),
		*TargetActorKey.SelectedKeyName.ToString(),
		*DistanceToTargetKey.SelectedKeyName.ToString(),
		*bInAttackRangeKey.SelectedKeyName.ToString());
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
