#include "AI/StateTree/YogStateTreeTask_EnemyAttackByProfile.h"

#include "AI/StateTree/YogStateTreeShared.h"
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

static bool StateTreeFacePawnTowardsTarget(APawn& Pawn, const AActor* TargetActor)
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

static bool StateTreeTargetHasSmokeAttackBlock(const AActor* TargetActor)
{
	const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	const FGameplayTag InSmokeTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.InSmoke"), false);
	return TargetASC && InSmokeTag.IsValid() && TargetASC->HasMatchingGameplayTag(InSmokeTag);
}

static float StateTreeResolveHealthPercent(const UAbilitySystemComponent* ASC)
{
	return YogStateTree::ResolveHealthPercent(ASC);
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
	if (!EnemyData || !AbilityData || InstanceData.AbilityTags.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	AYogAIController* YogAI = Cast<AYogAIController>(AIC);

	// Distance: prefer the bound Input, else compute against the target.
	float DistanceToTarget = InstanceData.DistanceToTarget;
	if (DistanceToTarget <= 0.0f)
	{
		DistanceToTarget = TargetActor
			? FVector::Dist2D(Pawn->GetActorLocation(), TargetActor->GetActorLocation())
			: TNumericLimits<float>::Max();
	}

	// Range band is authored on the state itself rather than derived from the chase, so a
	// retreat state can declare a wide band and still fire point-blank.
	const bool bInsideMinRange = DistanceToTarget >= InstanceData.MinRange;
	const bool bInsideMaxRange = InstanceData.MaxRange <= 0.f || DistanceToTarget <= InstanceData.MaxRange;
	if (!bInsideMinRange || !bInsideMaxRange)
	{
		return EStateTreeRunStatus::Failed;
	}

	const float HealthPercent = StateTreeResolveHealthPercent(ASC);
	const float MinHealthPercent = FMath::Clamp(InstanceData.MinHealthPercent, 0.f, 1.f);
	const float MaxHealthPercent = FMath::Clamp(InstanceData.MaxHealthPercent, 0.f, 1.f);
	if (HealthPercent < MinHealthPercent || HealthPercent > MaxHealthPercent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Cooldowns are tracked per ability tag on the controller, so an attack shared by two states
	// stays on cooldown across both.
	TArray<FGameplayTag> ReadyTags;
	for (const FGameplayTag& Tag : InstanceData.AbilityTags)
	{
		if (!AbilityData->HasAbility(Tag))
		{
			continue;
		}

		if (YogAI && !YogAI->IsAttackCooldownReadyForAI(Tag))
		{
			continue;
		}

		ReadyTags.Add(Tag);
	}

	if (ReadyTags.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	// Pick here rather than letting the ASC choose, so the cooldown is stamped on the tag that
	// actually fired.
	const FGameplayTag ChosenTag = ReadyTags[FMath::RandHelper(ReadyTags.Num())];
	const FGameplayTagContainer ChosenTags(ChosenTag);

	if (YogAI)
	{
		YogAI->EnterCombat(TargetActor, false);
	}
	AIC->StopMovement();

	StateTreeFacePawnTowardsTarget(*Pawn, TargetActor);

	AEnemyCharacterBase* EnemyCharacter = Cast<AEnemyCharacterBase>(Pawn);
	if (EnemyCharacter)
	{
		EnemyCharacter->SetAIAttackRuntimeContext(
			InstanceData.bIsReposition,
			InstanceData.bRequestRepositionOnResolve,
			TargetActor,
			DistanceToTarget);
	}

	if (YogAI)
	{
		YogAI->SetCombatAttackInProgress(true);
	}

	if (InstanceData.bPreAttackFlash && Character)
	{
		Character->StartPreAttackFlash();
		InstanceData.FlashCharacter = Character;
	}

	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(ChosenTags, false);
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

	if (YogAI)
	{
		YogAI->NotifyAttackActivated(ChosenTag, InstanceData.Cooldown);
	}

	InstanceData.ActiveAbilityTags = ChosenTags;

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
