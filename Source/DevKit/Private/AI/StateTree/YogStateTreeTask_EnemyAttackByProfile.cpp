#include "AI/StateTree/YogStateTreeTask_EnemyAttackByProfile.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"

namespace
{
	// Local candidate mirrors UBTTask_EnemyAttackByProfile::FAttackCandidate.
	struct FAttackCandidate
	{
		int32 AttackIndex = INDEX_NONE;
		float Weight = 0.f;
		FGameplayTagContainer ValidTags;
	};

	bool StateTreeFacePawnTowardsTarget(APawn& Pawn, const AActor* TargetActor)
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

		const FRotator FaceTargetRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
		Pawn.SetActorRotation(FaceTargetRotation);
		if (AController* Controller = Pawn.GetController())
		{
			Controller->SetControlRotation(FaceTargetRotation);
		}
		return true;
	}

	bool StateTreeTargetHasSmokeAttackBlock(const AActor* TargetActor)
	{
		const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);
		const UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
		const FGameplayTag InSmokeTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.InSmoke"), false);
		return TargetASC && InSmokeTag.IsValid() && TargetASC->HasMatchingGameplayTag(InSmokeTag);
	}

	float StateTreeResolveHealthPercent(const UAbilitySystemComponent* ASC)
	{
		if (!ASC
			|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute())
			|| !ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute()))
		{
			return 1.0f;
		}

		const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth <= KINDA_SMALL_NUMBER)
		{
			return 1.0f;
		}

		const float Health = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		return FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
	}
}

FStateTreeTask_EnemyAttackByProfile::FStateTreeTask_EnemyAttackByProfile()
{
	// Completion is delegate-driven; no per-frame tick needed.
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_EnemyAttackByProfile::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIC = InstanceData.AIController;
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!AIC || !Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Resolve target: bound Input, fall back to player pawn.
	AActor* TargetActor = InstanceData.TargetActor;
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(Pawn->GetWorld(), 0);
	}

	// Range gate — every role except Reposition requires being in attack range.
	if (InstanceData.RequiredAttackRole != EEnemyAIAttackRole::Reposition && !InstanceData.bInAttackRange)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (StateTreeTargetHasSmokeAttackBlock(TargetActor))
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

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(Pawn);
	const UCharacterDataComponent* DataComponent = Character ? Character->GetCharacterDataComponent() : nullptr;
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	const UAbilityData* AbilityData = EnemyData ? EnemyData->AbilityData : nullptr;
	if (!EnemyData || !AbilityData || EnemyData->AttackProfile.Attacks.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	AYogAIController* YogAI = Cast<AYogAIController>(AIC);
	const float CurrentTime = Pawn->GetWorld() ? Pawn->GetWorld()->GetTimeSeconds() : 0.0f;

	// Distance: prefer the bound Input, else compute against the target.
	float DistanceToTarget = InstanceData.DistanceToTarget;
	if (DistanceToTarget <= 0.0f)
	{
		DistanceToTarget = TargetActor
			? FVector::Dist2D(Pawn->GetActorLocation(), TargetActor->GetActorLocation())
			: TNumericLimits<float>::Max();
	}

	if (YogAI)
	{
		YogAI->RefreshMovementAttackCooldownReset(DistanceToTarget);
	}
	InstanceData.AttackCooldownEndTimes.SetNum(EnemyData->AttackProfile.Attacks.Num());

	const float HealthPercent = StateTreeResolveHealthPercent(ASC);
	float CloseCombatRange = EnemyData->MovementTuning.AttackRange;
	for (const FEnemyAIAttackOption& Attack : EnemyData->AttackProfile.Attacks)
	{
		if (Attack.AttackRole == EEnemyAIAttackRole::CloseMelee && Attack.MaxRange > 0.0f)
		{
			CloseCombatRange = FMath::Max(CloseCombatRange, Attack.MaxRange);
		}
	}

	TArray<FAttackCandidate> Candidates;
	float TotalWeight = 0.0f;
	for (int32 AttackIndex = 0; AttackIndex < EnemyData->AttackProfile.Attacks.Num(); ++AttackIndex)
	{
		const FEnemyAIAttackOption& Attack = EnemyData->AttackProfile.Attacks[AttackIndex];
		if (Attack.AttackRole != InstanceData.RequiredAttackRole)
		{
			continue;
		}

		if (Attack.Weight <= 0.0f || Attack.AbilityTags.IsEmpty())
		{
			continue;
		}

		const float MinHealthPercent = FMath::Clamp(Attack.MinHealthPercent, 0.0f, 1.0f);
		const float MaxHealthPercent = FMath::Clamp(Attack.MaxHealthPercent, 0.0f, 1.0f);
		if (HealthPercent < MinHealthPercent || HealthPercent > MaxHealthPercent)
		{
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
			continue;
		}

		const float EffectiveMaxRange = bMovementAttackWindow && YogAI
			? YogAI->GetMovementAttackRange(Attack)
			: Attack.MaxRange;
		const bool bInsideMinRange = DistanceToTarget >= RequiredMinRange;
		const bool bInsideMaxRange = EffectiveMaxRange <= 0.0f || DistanceToTarget <= EffectiveMaxRange;
		if (!bInsideMinRange || !bInsideMaxRange)
		{
			continue;
		}

		if (bMovementAttackWindow && YogAI)
		{
			float RemainingMovementAttackCooldown = 0.0f;
			if (!YogAI->CanUseMovementAttack(Attack, DistanceToTarget, &RemainingMovementAttackCooldown))
			{
				continue;
			}
		}

		if (InstanceData.AttackCooldownEndTimes.IsValidIndex(AttackIndex)
			&& CurrentTime < InstanceData.AttackCooldownEndTimes[AttackIndex])
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

	// Recent-repeat penalty (only meaningful with >1 candidate).
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
			}
			TotalWeight += Candidate.Weight;
		}
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return EStateTreeRunStatus::Failed;
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
	if (YogAI)
	{
		YogAI->EnterCombat(TargetActor, false);
	}
	AIC->StopMovement();

	StateTreeFacePawnTowardsTarget(*Pawn, TargetActor);

	AEnemyCharacterBase* EnemyCharacter = Cast<AEnemyCharacterBase>(Pawn);
	if (EnemyCharacter)
	{
		EnemyCharacter->SetAIAttackRuntimeContext(ChosenAttack, TargetActor, DistanceToTarget);
	}

	if (YogAI)
	{
		YogAI->SetCombatAttackInProgress(true);
	}

	if (ChosenAttack.bPreAttackFlash && Character)
	{
		Character->StartPreAttackFlash();
		InstanceData.FlashCharacter = Character;
	}

	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(ChosenCandidate->ValidTags, false);
	if (!bActivated)
	{
		if (EnemyCharacter)
		{
			EnemyCharacter->ClearAIAttackRuntimeContext();
		}
		if (YogAI)
		{
			YogAI->SetCombatAttackInProgress(false);
			YogAI->ResetCombatMoveSmoothingAfterAttack();
		}
		if (InstanceData.FlashCharacter.IsValid())
		{
			InstanceData.FlashCharacter->StopPreAttackFlash();
			InstanceData.FlashCharacter.Reset();
		}
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.AttackCooldownEndTimes.IsValidIndex(ChosenCandidate->AttackIndex))
	{
		InstanceData.AttackCooldownEndTimes[ChosenCandidate->AttackIndex] = CurrentTime + ChosenAttack.Cooldown;
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

	InstanceData.ActiveAbilityTags = ChosenCandidate->ValidTags;

	// If no matching GA is still active, it ended synchronously (e.g. no montage).
	bool bStillActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->AbilityTags.HasAny(InstanceData.ActiveAbilityTags))
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
		if (InstanceData.FlashCharacter.IsValid())
		{
			InstanceData.FlashCharacter->StopPreAttackFlash();
			InstanceData.FlashCharacter.Reset();
		}
		InstanceData.ActiveAbilityTags.Reset();
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.ActiveASC = ASC;
	const FGameplayTagContainer ActiveTags = InstanceData.ActiveAbilityTags;
	TWeakObjectPtr<AYogAIController> WeakYogAI(YogAI);
	InstanceData.EndHandle = ASC->OnAbilityEnded.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(), ActiveTags, WeakYogAI](const FAbilityEndedData& Data)
		{
			if (Data.AbilityThatEnded && Data.AbilityThatEnded->AbilityTags.HasAny(ActiveTags))
			{
				if (WeakYogAI.IsValid())
				{
					WeakYogAI->SetCombatAttackInProgress(false);
					WeakYogAI->ResetCombatMoveSmoothingAfterAttack();
				}
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		});

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_EnemyAttackByProfile::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController))
	{
		YogAI->SetCombatAttackInProgress(false);
	}

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

	InstanceData.ActiveAbilityTags.Reset();
}
