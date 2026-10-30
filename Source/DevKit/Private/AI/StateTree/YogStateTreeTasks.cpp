#include "AI/StateTree/YogStateTreeTasks.h"
#include "AI/StateTree/YogLuaSpawnLibrary.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/EnemyData.h"
#include "Data/EnemyWeaponDefinition.h"
#include "GameModes/YogGameMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Mob/MobSpawner.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"
#include "UnLua.h"
#include "UnLuaModule.h"

// ─── Activate Ability By Tag ────────────────────────────────────────────────

static bool SpawnMobStep(FStateTreeTask_SpawnMobInstanceData& InstanceData, AActor* OriginActor, APawn* InstigatorPawn)
{
	UWorld* World = OriginActor->GetWorld();

	FVector SpawnLocation = FVector::ZeroVector;
	if (!UYogLuaSpawnLibrary::FindReachableSpawnLocation(
			World,
			OriginActor->GetActorLocation(),
			InstanceData.SpawnRadius,
			InstanceData.MinSpawnDistance,
			InstanceData.MaxAttempts,
			SpawnLocation))
	{
		return false;
	}

	SpawnLocation.Z += InstanceData.SpawnZOffset;

	AEnemyCharacterBase* Spawned = UYogLuaSpawnLibrary::SpawnEnemyAtLocation(
		World, OriginActor, InstigatorPawn, InstanceData.EnemyClass, SpawnLocation);
	if (!Spawned)
	{
		return false;
	}

	Spawned->bCountsForLevelClear = InstanceData.bCountsForLevelClear;
	InstanceData.SpawnedEnemies.Add(Spawned);
	return true;
}

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

	// AbilityTags is the StateTree-authored selector. The equipped weapon's
	// AbilityData is used only to validate that the requested tag is configured;
	// never broaden the selector to every tag present on the weapon.
	TArray<FGameplayTag> ValidTags;
	const UAbilityData* WeaponAbilityData = nullptr;
	if (const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn))
	{
		if (const UEnemyWeaponDefinition* WeaponDefinition = Enemy->GetEquippedEnemyWeaponDefinition())
		{
			WeaponAbilityData = WeaponDefinition->AbilityData;
		}
	}

	if (WeaponAbilityData)
	{
		for (const FGameplayTag& Tag : InstanceData.AbilityTags)
		{
			if (Tag.IsValid() && WeaponAbilityData->HasAbility(Tag))
			{
				ValidTags.AddUnique(Tag);
			}
		}
	}
	else
	{
		// Legacy fallback for pawns that have no equipped weapon definition.
		if (const AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn))
		{
			if (const UCharacterDataComponent* DC = Char->GetCharacterDataComponent())
			{
				if (const UCharacterData* CD = DC->GetCharacterData())
				{
					if (const UAbilityData* AD = CD->AbilityData)
					{
						for (const FGameplayTag& Tag : InstanceData.AbilityTags)
						{
							if (Tag.IsValid() && AD->HasAbility(Tag))
							{
								ValidTags.AddUnique(Tag);
							}
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

	AYogCharacterBase* YogCharacter = Cast<AYogCharacterBase>(Pawn);

	// Hitboxes are narrow cones tested against actor yaw, and nothing else turns the pawn
	// once it stops moving, so a stale facing from the chase makes every swing whiff.
	const AActor* FaceTarget = InstanceData.AIController->GetFocusActor();
	if (!FaceTarget)
	{
		FaceTarget = UGameplayStatics::GetPlayerPawn(Pawn->GetWorld(), 0);
	}
	if (FaceTarget)
	{
		FVector ToTarget = FaceTarget->GetActorLocation() - Pawn->GetActorLocation();
		ToTarget.Z = 0.0f;
		if (!ToTarget.IsNearlyZero())
		{
			const FRotator FaceTargetRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);
			Pawn->SetActorRotation(FaceTargetRotation);
			InstanceData.AIController->SetControlRotation(FaceTargetRotation);
		}
	}

	if (InstanceData.bPreAttackFlash && YogCharacter)
	{
		YogCharacter->StartPreAttackFlash();
		InstanceData.FlashCharacter = YogCharacter;
	}

	// Clear the accumulator before activating so the outcome reflects only this
	// attack. GA_MeleeAttack also clears it, but montage-only GAs do not.
	if (InstanceData.bReportAttackOutcome && YogCharacter)
	{
		YogCharacter->bAttackHitConnected = false;
	}

	// Activate only an authored tag; never choose from weapon-wide data. Each
	// authored tag names one ability, so multiple tags mean "pick one at random".
	// Shuffle rather than a single random draw so a blocked/cooling-down pick
	// still falls through to the remaining candidates.
	for (int32 Index = ValidTags.Num() - 1; Index > 0; --Index)
	{
		ValidTags.Swap(Index, FMath::RandRange(0, Index));
	}

	FGameplayTag ActivatedTag;
	for (const FGameplayTag& Tag : ValidTags)
	{
		if (YogASC->TryActivateAbilityByExactTag(Tag, false))
		{
			ActivatedTag = Tag;
			break;
		}
	}

	if (!ActivatedTag.IsValid())
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
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->AbilityTags.HasTagExact(ActivatedTag))
		{
			bStillActive = true;
			break;
		}
	}

	AYogAIController* YogAI = Cast<AYogAIController>(InstanceData.AIController);
	const bool bReportOutcome = InstanceData.bReportAttackOutcome && YogAI && YogCharacter;

	if (!bStillActive)
	{
		if (bReportOutcome)
		{
			YogAI->NotifyAttackResolved(!YogCharacter->bAttackHitConnected);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	InstanceData.ActiveASC = ASC;
	InstanceData.EndHandle = ASC->OnAbilityEnded.AddLambda(
		[WeakContext = Context.MakeWeakExecutionContext(),
		 ActivatedTag,
		 bReportOutcome,
		 WeakYogAI = TWeakObjectPtr<AYogAIController>(YogAI),
		 WeakCharacter = TWeakObjectPtr<AYogCharacterBase>(YogCharacter)](const FAbilityEndedData& Data)
		{
			if (!Data.AbilityThatEnded || !Data.AbilityThatEnded->AbilityTags.HasTagExact(ActivatedTag))
			{
				return;
			}

			// A cancelled ability never got to resolve, so reporting it as a whiff
			// would punish interrupts (hit reactions, staggers) as if they missed.
			if (bReportOutcome && !Data.bWasCancelled && WeakYogAI.IsValid() && WeakCharacter.IsValid())
			{
				WeakYogAI->NotifyAttackResolved(!WeakCharacter->bAttackHitConnected);
			}

			WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
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

// ─── Enter Boss Phase ───────────────────────────────────────────────────────

FStateTreeTask_EnterBossPhase::FStateTreeTask_EnterBossPhase()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_EnterBossPhase::EnterState(
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

	if (ASC && InstanceData.PhaseEffect)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(Pawn);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(InstanceData.PhaseEffect, InstanceData.PhaseEffectLevel, EffectContext);
		if (Spec.IsValid())
		{
			InstanceData.AppliedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			InstanceData.AppliedASC = ASC;
		}
	}

	// Look change: swap mesh / material on the pawn's skeletal mesh component.
	if (const AYogCharacterBase* Character = Cast<AYogCharacterBase>(Pawn))
	{
		if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
		{
			if (InstanceData.PhaseMesh)
			{
				MeshComp->SetSkeletalMeshAsset(InstanceData.PhaseMesh);
			}
			if (InstanceData.PhaseMaterialOverride)
			{
				MeshComp->SetMaterial(0, InstanceData.PhaseMaterialOverride);
			}
		}
	}

	if (ASC && InstanceData.PhaseVfxCueTag.IsValid())
	{
		ASC->ExecuteGameplayCue(InstanceData.PhaseVfxCueTag);
	}

	// One-shot: the sibling attack task on this state drives ongoing combat.
	return EStateTreeRunStatus::Succeeded;
}

void FStateTreeTask_EnterBossPhase::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.bRemoveEffectOnExit
		&& InstanceData.AppliedASC.IsValid()
		&& InstanceData.AppliedEffectHandle.IsValid())
	{
		InstanceData.AppliedASC->RemoveActiveGameplayEffect(InstanceData.AppliedEffectHandle);
	}
	InstanceData.AppliedEffectHandle.Invalidate();
	InstanceData.AppliedASC.Reset();
}

// ─── Boss Dying Reaction ────────────────────────────────────────────────────

FStateTreeTask_BossDyingReaction::FStateTreeTask_BossDyingReaction()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_BossDyingReaction::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!Pawn || !InstanceData.EncounterPoint)
	{
		return EStateTreeRunStatus::Failed;
	}

	const UGameInstance* GameInstance = Pawn->GetGameInstance();
	UStoryEncounterRuntimeSubsystem* Story = GameInstance ? GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>() : nullptr;
	if (Story)
	{
		Story->TriggerEncounterPoint(InstanceData.EncounterPoint, Pawn);
	}

	return EStateTreeRunStatus::Succeeded;
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

	// Mirror the point onto the blackboard so a Move To Controller Target task
	// (which has no property binding) can read it back as its destination.
	if (UBlackboardComponent* Blackboard = InstanceData.AIController ? InstanceData.AIController->GetBlackboardComponent() : nullptr)
	{
		Blackboard->SetValueAsVector(TEXT("PatrolTargetLocation"), PatrolTarget);
	}

	return EStateTreeRunStatus::Succeeded;
}

// ─── Enemy Patrol Wait ──────────────────────────────────────────────────────

// Spawn Mob In Reachable NavMesh

FStateTreeTask_SpawnMobInReachableNavMesh::FStateTreeTask_SpawnMobInReachableNavMesh()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_SpawnMobInReachableNavMesh::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AActor* OriginActor = InstanceData.SpawnOriginActor ? InstanceData.SpawnOriginActor.Get() : nullptr;
	if (!OriginActor && InstanceData.AIController)
	{
		OriginActor = InstanceData.AIController->GetPawn();
	}

	if (!OriginActor || !InstanceData.EnemyClass)
	{
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = OriginActor->GetWorld();
	if (!World)
	{
		return EStateTreeRunStatus::Failed;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!UYogLuaSpawnLibrary::FindReachableSpawnLocation(
			World,
			OriginActor->GetActorLocation(),
			InstanceData.SpawnRadius,
			InstanceData.MinSpawnDistance,
			InstanceData.MaxAttempts,
			SpawnLocation))
	{
		return EStateTreeRunStatus::Failed;
	}

	SpawnLocation.Z += InstanceData.SpawnZOffset;

	AEnemyCharacterBase* SpawnedEnemy = UYogLuaSpawnLibrary::SpawnEnemyAtLocation(
		World,
		OriginActor,
		InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr,
		InstanceData.EnemyClass,
		SpawnLocation);
	if (!SpawnedEnemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	SpawnedEnemy->bCountsForLevelClear = InstanceData.bCountsForLevelClear;
	InstanceData.SpawnedEnemy = SpawnedEnemy;
	return EStateTreeRunStatus::Succeeded;
}

// ─── Spawn Mob ──────────────────────────────────────────────────────────────

static constexpr int32 SpawnMob_MaxConsecutiveFailures = 10;

FStateTreeTask_SpawnMob::FStateTreeTask_SpawnMob()
{
	bShouldCallTick = true;
}

void FStateTreeTask_SpawnMob::RollBatchTarget(FInstanceDataType& InstanceData)
{
	InstanceData.BatchTarget = FMath::RandRange(
		InstanceData.TotalCount,
		FMath::Max(InstanceData.TotalCount, InstanceData.TotalCountMax));
}

EStateTreeRunStatus FStateTreeTask_SpawnMob::AdvanceSpawn(FInstanceDataType& InstanceData) const
{
	AActor* OriginActor = InstanceData.SpawnOriginActor ? InstanceData.SpawnOriginActor.Get() : nullptr;
	if (!OriginActor && InstanceData.AIController)
	{
		OriginActor = InstanceData.AIController->GetPawn();
	}

	if (!OriginActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	APawn* InstigatorPawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!SpawnMobStep(InstanceData, OriginActor, InstigatorPawn))
	{
		if (InstanceData.bFailOnSpawnFailure)
		{
			return EStateTreeRunStatus::Failed;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("[SpawnMob] No reachable spawn point for %s around %s (Radius=%.0f Min=%.0f Attempts=%d); retrying"),
			*GetNameSafe(InstanceData.EnemyClass),
			*GetNameSafe(OriginActor),
			InstanceData.SpawnRadius,
			InstanceData.MinSpawnDistance,
			InstanceData.MaxAttempts);

		// A failed placement must not consume one of the batch, or the group
		// silently comes out short. But an origin that can never place anything
		// must not retry forever either.
		if (++InstanceData.ConsecutiveFailures >= SpawnMob_MaxConsecutiveFailures)
		{
			return EStateTreeRunStatus::Failed;
		}

		InstanceData.TimeUntilNextSpawn = InstanceData.SpawnInterval;
		return EStateTreeRunStatus::Running;
	}

	InstanceData.ConsecutiveFailures = 0;
	++InstanceData.SpawnedCount;
	if (InstanceData.SpawnedCount >= InstanceData.BatchTarget)
	{
		if (!InstanceData.bRepeatIndefinitely)
		{
			return EStateTreeRunStatus::Succeeded;
		}

		// Scoped to the current batch: a repeating task would otherwise keep hard
		// references to every mob it ever spawned, pinning dead ones against GC.
		InstanceData.SpawnedEnemies.Reset();
		InstanceData.SpawnedCount = 0;
		RollBatchTarget(InstanceData);
		InstanceData.TimeUntilNextSpawn = InstanceData.RepeatDelay;
		return EStateTreeRunStatus::Running;
	}

	InstanceData.TimeUntilNextSpawn = InstanceData.SpawnInterval;
	return EStateTreeRunStatus::Running;
}

// A zero SpawnInterval means "spawn the whole batch at once" -- Tick would
// otherwise stretch it over one frame per mob. The TimeUntilNextSpawn guard
// stops the drain at a batch boundary, where AdvanceSpawn has armed the
// RepeatDelay; without it a repeating task would spawn forever in one frame.
EStateTreeRunStatus FStateTreeTask_SpawnMob::AdvanceBatch(FInstanceDataType& InstanceData) const
{
	EStateTreeRunStatus Status = AdvanceSpawn(InstanceData);
	while (Status == EStateTreeRunStatus::Running
		&& InstanceData.SpawnInterval <= 0.f
		&& InstanceData.TimeUntilNextSpawn <= 0.f)
	{
		Status = AdvanceSpawn(InstanceData);
	}

	return Status;
}

EStateTreeRunStatus FStateTreeTask_SpawnMob::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.EnemyClass || InstanceData.TotalCount <= 0)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.SpawnedEnemies.Reset();
	InstanceData.SpawnedCount = 0;
	InstanceData.ConsecutiveFailures = 0;
	InstanceData.TimeUntilNextSpawn = 0.f;
	RollBatchTarget(InstanceData);

	return AdvanceBatch(InstanceData);
}

EStateTreeRunStatus FStateTreeTask_SpawnMob::Tick(
	FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.TimeUntilNextSpawn -= DeltaTime;
	if (InstanceData.TimeUntilNextSpawn > 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	return AdvanceBatch(InstanceData);
}

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

// ─── Random Walk Around Spawn ───────────────────────────────────────────────

static void YogStateTreeTasks_BeginWait(FStateTreeTask_RandomWalkAroundSpawn::FInstanceDataType& InstanceData)
{
	float MinWait = InstanceData.WaitMin;
	float MaxWait = InstanceData.WaitMax;
	if (InstanceData.bUseEnemyDataTuning)
	{
		const APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
		const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
		const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
		if (const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr)
		{
			MinWait = EnemyData->AwarenessTuning.PatrolWaitMin;
			MaxWait = EnemyData->AwarenessTuning.PatrolWaitMax;
		}
	}

	MinWait = FMath::Max(0.0f, MinWait);
	MaxWait = FMath::Max(MinWait, MaxWait);
	InstanceData.WaitRemaining = FMath::FRandRange(MinWait, MaxWait);
	InstanceData.bWalking = false;
}

// Returns false when no move could be started, in which case the caller should idle and retry.
static bool YogStateTreeTasks_BeginWalk(FStateTreeTask_RandomWalkAroundSpawn::FInstanceDataType& InstanceData)
{
	AAIController* AIController = InstanceData.AIController;
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	UNavigationSystemV1* NavSys = Pawn ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld()) : nullptr;
	if (!NavSys)
	{
		return false;
	}

	float Radius = InstanceData.WalkRadius;
	if (InstanceData.bUseEnemyDataTuning)
	{
		const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Pawn);
		const UCharacterDataComponent* DataComponent = Enemy ? Enemy->GetCharacterDataComponent() : nullptr;
		if (const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr)
		{
			Radius = EnemyData->AwarenessTuning.PatrolRadius;
		}
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	const float MinDistanceSq = FMath::Square(InstanceData.MinWalkDistance);

	bool bFoundPoint = false;
	FNavLocation NavLocation;
	for (int32 Attempt = 0; Attempt < InstanceData.MaxPickAttempts; ++Attempt)
	{
		if (NavSys->GetRandomReachablePointInRadius(InstanceData.Origin, Radius, NavLocation)
			&& FVector::DistSquared2D(NavLocation.Location, PawnLocation) >= MinDistanceSq)
		{
			bFoundPoint = true;
			break;
		}
	}
	if (!bFoundPoint)
	{
		return false;
	}

	InstanceData.CurrentWalkTarget = NavLocation.Location;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(NavLocation.Location);
	MoveRequest.SetAcceptanceRadius(InstanceData.AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetCanStrafe(true);

	const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveRequest);
	if (RequestResult.Code != EPathFollowingRequestResult::RequestSuccessful)
	{
		return false;
	}

	InstanceData.bWalking = true;
	return true;
}

FStateTreeTask_RandomWalkAroundSpawn::FStateTreeTask_RandomWalkAroundSpawn()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_RandomWalkAroundSpawn::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const APawn* Pawn = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// The blackboard origin is the spawn point; an all-zero value means it was never written.
	const UBlackboardComponent* Blackboard = InstanceData.AIController->GetBlackboardComponent();
	FVector Origin = (Blackboard && !InstanceData.OriginKey.IsNone())
		? Blackboard->GetValueAsVector(InstanceData.OriginKey)
		: FVector::ZeroVector;
	if (Origin.IsNearlyZero())
	{
		Origin = Pawn->GetActorLocation();
	}
	InstanceData.Origin = Origin;
	InstanceData.CurrentWalkTarget = Origin;

	if (InstanceData.bStartWithWait || !YogStateTreeTasks_BeginWalk(InstanceData))
	{
		YogStateTreeTasks_BeginWait(InstanceData);
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_RandomWalkAroundSpawn::Tick(
	FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = InstanceData.AIController;
	if (!AIController || !AIController->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bWalking)
	{
		// Arrival and abort are treated alike; either way the pawn idles, then picks a new point.
		if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			YogStateTreeTasks_BeginWait(InstanceData);
		}
		return EStateTreeRunStatus::Running;
	}

	InstanceData.WaitRemaining -= DeltaTime;
	if (InstanceData.WaitRemaining > 0.0f)
	{
		return EStateTreeRunStatus::Running;
	}

	if (!InstanceData.bLoop)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (!YogStateTreeTasks_BeginWalk(InstanceData))
	{
		YogStateTreeTasks_BeginWait(InstanceData);
	}
	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_RandomWalkAroundSpawn::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.bWalking && InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.bWalking = false;
}

// ─── Move To Controller Target ──────────────────────────────────────────────

FStateTreeTask_HoldPosition::FStateTreeTask_HoldPosition()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_HoldPosition::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.AIController->StopMovement();

	InstanceData.bSetFocus = false;
	if (InstanceData.bFaceTarget)
	{
		const UBlackboardComponent* Blackboard = InstanceData.AIController->GetBlackboardComponent();
		AActor* Target = (Blackboard && !InstanceData.TargetActorKey.IsNone())
			? Cast<AActor>(Blackboard->GetValueAsObject(InstanceData.TargetActorKey))
			: nullptr;
		if (Target)
		{
			InstanceData.AIController->SetFocus(Target);
			InstanceData.bSetFocus = true;
		}
	}
	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_HoldPosition::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.bSetFocus && InstanceData.AIController)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	InstanceData.bSetFocus = false;
}

FStateTreeTask_MoveToControllerTarget::FStateTreeTask_MoveToControllerTarget()
{
	// Completion is delegate-driven off the path-following request; no tick needed.
	bShouldCallTick = false;
}

// Slack on top of the move request's reach test before a weave leg counts as issuable. The reach
// test uses the movement component's nav agent radius, which GetSimpleCollisionRadius only
// approximates, so leave room for the two to disagree.
static constexpr float YogStateTree_SnakeLegMargin = 50.0f;

// Where the chase hands over to the attack. On AttackRangeAttribute the scale keeps the
// pawn from stopping exactly on the attack gate's boundary, where a small drift by the
// player would push it back out of range and restart the chase.
static float YogStateTree_ResolveChaseStopDistance(
	const FStateTreeTask_ChasePlayerUntilDistanceInstanceData& InstanceData,
	const AActor* Self)
{
	if (InstanceData.StopDistanceSource != EYogDistanceSource::AttackRangeAttribute)
	{
		return InstanceData.StopDistance;
	}

	const TOptional<float> AttackRange = YogStateTree::ResolveAttackRange(Self);
	if (!AttackRange.IsSet())
	{
		return InstanceData.StopDistance;
	}

	return AttackRange.GetValue() * FMath::Clamp(InstanceData.StopDistanceAttributeScale, 0.1f, 1.0f);
}

// One leg of the weave: advance toward the player, offset sideways onto whichever side the
// caller has already chosen in bSnakeToRight. SnakeRandomness jitters both distances here.
static FVector YogStateTree_ComputeSnakeWaypoint(
	const APawn& Pawn,
	const AActor& Player,
	const FStateTreeTask_ChasePlayerUntilDistanceInstanceData& InstanceData,
	float StopDistance,
	float MinLegLength)
{
	const FVector PawnLocation = Pawn.GetActorLocation();
	FVector ToPlayer = Player.GetActorLocation() - PawnLocation;
	ToPlayer.Z = 0.0f;

	const float DistanceToPlayer = ToPlayer.Size();
	if (DistanceToPlayer <= KINDA_SMALL_NUMBER)
	{
		return PawnLocation;
	}
	ToPlayer /= DistanceToPlayer;

	const float Randomness = FMath::Clamp(InstanceData.SnakeRandomness, 0.0f, 1.0f);

	// Never place a leg past the stop distance, otherwise the weave carries the pawn through
	// the player.
	const float ForwardRoom = FMath::Max(DistanceToPlayer - StopDistance, 0.0f);
	const float LegScale = 1.0f + FMath::FRandRange(-0.6f, 0.6f) * Randomness;

	// A leg shorter than the move request's reach test is answered with AlreadyAtGoal, which
	// finishes the request without moving the pawn -- so randomness must not be able to shrink a
	// leg below it. ForwardRoom still wins: overshooting it would weave the pawn past the player.
	const float LegLength = FMath::Min(
		FMath::Max(InstanceData.SnakeSegmentLength * LegScale, MinLegLength),
		ForwardRoom);

	// Randomness only ever narrows the offset, never widens it, so the 45-degree cap below
	// still holds at full randomness.
	const float AmplitudeScale = 1.0f - FMath::FRandRange(0.0f, 0.7f) * Randomness;

	// Clamping the sideways offset to the forward progress caps the weave at 45 degrees, which
	// is what keeps the pawn closing rather than orbiting. It also lets the weave taper off on
	// its own as the gap shrinks, so a short re-approach still weaves instead of being cut off.
	const float Amplitude = FMath::Min(InstanceData.SnakeAmplitude * AmplitudeScale, LegLength);

	const FVector Lateral = FVector::CrossProduct(FVector::UpVector, ToPlayer)
		* (InstanceData.bSnakeToRight ? Amplitude : -Amplitude);

	return PawnLocation + ToPlayer * LegLength + Lateral;
}

FStateTreeTask_ChasePlayerUntilDistance::FStateTreeTask_ChasePlayerUntilDistance()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_ChasePlayerUntilDistance::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.LastRequestTime = -FLT_MAX;
	InstanceData.bHasSnakeWaypoint = false;
	return Tick(Context, 0.0f);
}

EStateTreeRunStatus FStateTreeTask_ChasePlayerUntilDistance::Tick(
	FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAIController* Controller = InstanceData.AIController;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Controller || !Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// EnterState runs from OnPossess, which for preplaced pawns happens before the player
	// pawn exists. Keep waiting instead of failing: ST_Debugger has no failure transition,
	// so a single Failed halts the whole tree for this pawn permanently.
	AActor* Player = UGameplayStatics::GetPlayerPawn(Pawn, 0);
	if (!Player)
	{
		return EStateTreeRunStatus::Running;
	}

	// Re-resolved every tick so a range buff or weapon swap mid-chase moves the handover
	// point immediately instead of only on the next EnterState.
	const float StopDistance = YogStateTree_ResolveChaseStopDistance(InstanceData, Pawn);

	const float Distance = FVector::Dist2D(Pawn->GetActorLocation(), Player->GetActorLocation());
	if (Distance <= StopDistance)
	{
		Controller->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	const UWorld* World = Controller->GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;

	// Legs are clamped to the room left in front of the player, so they shrink as the pawn closes in.
	// MoveToLocation is issued with bStopOnOverlap, making its reach test AcceptanceRadius plus the
	// agent radius, and a leg inside that is answered with AlreadyAtGoal: the request finishes
	// immediately without moving the pawn, the status returns to Idle, and the next tick issues an
	// equally short leg on the flipped side. That never resolves, so the pawn holds short of
	// StopDistance and the task never succeeds. Weaving must stop while the legs are still longer.
	const float MinLegLength = InstanceData.AcceptanceRadius
		+ Pawn->GetSimpleCollisionRadius()
		+ YogStateTree_SnakeLegMargin;

	// Weaving has to abandon MoveToActor: goal-actor observation re-solves the path straight at the
	// player every frame, which would erase the lateral offset.
	const bool bSnake = InstanceData.SnakeAmplitude > KINDA_SMALL_NUMBER
		&& Distance > InstanceData.SnakeStraightenDistance
		&& Distance > StopDistance + MinLegLength;

	// Straightening out mid-leg leaves the pawn walking at a lateral corner with no snake waypoint
	// to hand over from, and the Idle gate below would not re-issue until it brakes there. Treat the
	// switch as a handoff so the pawn turns onto the player immediately.
	const bool bStraightenedThisTick = !bSnake && InstanceData.bHasSnakeWaypoint;
	if (!bSnake)
	{
		InstanceData.bHasSnakeWaypoint = false;
	}

	// Hand the next leg over before the pawn reaches the corner. Letting the move run to completion
	// makes path following brake onto the corner, and the pawn then stands there until the tree
	// ticks again — that pause, not the turn angle, is what reads as a shuttle run.
	//
	// Capping the trigger at half the leg matters near the player: the forward-room clamp shortens
	// legs there, and a leg that starts shorter than SnakeCornerLookAhead would satisfy the handoff
	// the moment it is issued. The move then re-issues every RepathInterval, and because the side
	// flips each time the commanded direction oscillates faster than the pawn can act on it, so the
	// lateral components cancel and it crawls forward. Requiring half a leg of travel first commits
	// each direction long enough to cover ground.
	const float HandoffDistance = FMath::Min(
		InstanceData.SnakeCornerLookAhead,
		InstanceData.SnakeLegStartDistance * 0.5f);
	const bool bCornerHandoff = InstanceData.bHasSnakeWaypoint
		&& FVector::Dist2D(Pawn->GetActorLocation(), InstanceData.SnakeWaypoint) <= HandoffDistance;

	// The straight chase keeps its original Idle gate: MoveToActor installs goal-actor observation
	// (AIController::FindPathForMoveRequest) so it repaths itself as the player drifts, and
	// re-issuing it on a timer flips the chosen corridor at corners and wiggles into geometry.
	const bool bWantsRequest = Controller->GetMoveStatus() == EPathFollowingStatus::Idle
		|| (bSnake && !InstanceData.bHasSnakeWaypoint);

	// A corner handoff is a planned leg change rather than a retry, so RepathInterval must not gate
	// it. Throttling it would let the pawn run onto the corner and brake, which is what we are
	// avoiding. The half-leg cap above is what bounds how often it can happen.
	if (bCornerHandoff || bStraightenedThisTick
		|| (bWantsRequest && Now - InstanceData.LastRequestTime >= InstanceData.RepathInterval))
	{
		InstanceData.LastRequestTime = Now;

		EPathFollowingRequestResult::Type Result;
		if (bSnake)
		{
			// Strict alternation is what makes the weave read as a pattern, so let randomness
			// occasionally hold the same side for a second leg and break the rhythm.
			const float RepeatSideChance = 0.35f * FMath::Clamp(InstanceData.SnakeRandomness, 0.0f, 1.0f);
			if (FMath::FRand() >= RepeatSideChance)
			{
				InstanceData.bSnakeToRight = !InstanceData.bSnakeToRight;
			}

			const FVector Waypoint = YogStateTree_ComputeSnakeWaypoint(*Pawn, *Player, InstanceData, StopDistance, MinLegLength);
			InstanceData.SnakeWaypoint = Waypoint;
			InstanceData.bHasSnakeWaypoint = true;
			InstanceData.SnakeLegStartDistance = FVector::Dist2D(Pawn->GetActorLocation(), Waypoint);
			Result = Controller->MoveToLocation(
				Waypoint, InstanceData.AcceptanceRadius, true, true, true, true, nullptr, true);
		}
		else
		{
			Result = Controller->MoveToActor(
				Player, InstanceData.AcceptanceRadius, true, true, true, nullptr, true);
		}

		if (Result == EPathFollowingRequestResult::Failed)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ChasePlayer] %s REJECTED for %s at %s (distance %.0f) - retrying"),
				bSnake ? TEXT("MoveToLocation") : TEXT("MoveToActor"),
				*GetNameSafe(Pawn), *Pawn->GetActorLocation().ToCompactString(), Distance);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_ChasePlayerUntilDistance::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
}

// ─── Escape From Player ─────────────────────────────────────────────────────

// A direction that gains less than this fraction of a leg over the current distance counts as blocked.
static constexpr float YogStateTree_EscapeMinGainFraction = 0.35f;

// Picks the reachable point farthest from the player among a fan of directions around "straight away".
// False when even the best one gains almost nothing, i.e. the pawn is cornered.
static bool YogStateTree_FindEscapePoint(const APawn& Pawn, const AActor& Player, float StepDistance, FVector& OutPoint)
{
	static const float CandidateAngles[] = { 0.0f, 30.0f, -30.0f, 60.0f, -60.0f, 90.0f, -90.0f, 120.0f, -120.0f };

	const FVector PawnLocation = Pawn.GetActorLocation();
	const FVector PlayerLocation = Player.GetActorLocation();

	FVector Away = (PawnLocation - PlayerLocation).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = Pawn.GetActorForwardVector().GetSafeNormal2D();
	}

	UWorld* World = Pawn.GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;

	float BestDistance = -1.0f;
	for (const float Angle : CandidateAngles)
	{
		FVector Candidate = PawnLocation + Away.RotateAngleAxis(Angle, FVector::UpVector) * StepDistance;
		if (NavSys)
		{
			FNavLocation Projected;
			if (!NavSys->ProjectPointToNavigation(Candidate, Projected, FVector(150.0f, 150.0f, 300.0f)))
			{
				continue;
			}
			Candidate = Projected.Location;
		}

		const float CandidateDistance = FVector::Dist2D(Candidate, PlayerLocation);
		if (CandidateDistance > BestDistance)
		{
			BestDistance = CandidateDistance;
			OutPoint = Candidate;
		}
	}

	const float CurrentDistance = FVector::Dist2D(PawnLocation, PlayerLocation);
	return BestDistance >= CurrentDistance + StepDistance * YogStateTree_EscapeMinGainFraction;
}

FStateTreeTask_EscapeFromPlayer::FStateTreeTask_EscapeFromPlayer()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_EscapeFromPlayer::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.LastRequestTime = -FLT_MAX;

	const UWorld* World = InstanceData.AIController ? InstanceData.AIController->GetWorld() : nullptr;
	InstanceData.StartTime = World ? World->GetTimeSeconds() : 0.0f;
	return Tick(Context, 0.0f);
}

EStateTreeRunStatus FStateTreeTask_EscapeFromPlayer::Tick(
	FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AAIController* Controller = InstanceData.AIController;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Controller || !Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Same as Chase: EnterState can run before the player pawn exists, and a Failed here would halt a
	// tree that has no failure transition, so wait instead.
	AActor* Player = UGameplayStatics::GetPlayerPawn(Pawn, 0);
	if (!Player)
	{
		return EStateTreeRunStatus::Running;
	}

	const UWorld* World = Controller->GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;

	const float Distance = FVector::Dist2D(Pawn->GetActorLocation(), Player->GetActorLocation());
	const bool bTimedOut = InstanceData.MaxDuration > 0.0f && Now - InstanceData.StartTime >= InstanceData.MaxDuration;
	if (Distance >= InstanceData.DesiredDistance || bTimedOut)
	{
		Controller->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	if (Now - InstanceData.LastRequestTime < InstanceData.RepathInterval)
	{
		return EStateTreeRunStatus::Running;
	}
	InstanceData.LastRequestTime = Now;

	// A leg inside the nav reach test (acceptance radius plus agent radius) is answered with AlreadyAtGoal
	// and never moves the pawn, so legs are never allowed below that. See the matching note in Chase.
	const float MinStep = InstanceData.AcceptanceRadius
		+ Pawn->GetSimpleCollisionRadius()
		+ YogStateTree_SnakeLegMargin;
	const float StepDistance = FMath::Max(InstanceData.FleeStepDistance, MinStep);

	FVector Destination = FVector::ZeroVector;
	if (!YogStateTree_FindEscapePoint(*Pawn, *Player, StepDistance, Destination))
	{
		Controller->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	const EPathFollowingRequestResult::Type Result = Controller->MoveToLocation(
		Destination, InstanceData.AcceptanceRadius, true, true, true, true, nullptr, true);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[EscapePlayer] MoveToLocation REJECTED for %s at %s (distance %.0f) - retrying"),
			*GetNameSafe(Pawn), *Pawn->GetActorLocation().ToCompactString(), Distance);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_EscapeFromPlayer::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
}

EStateTreeRunStatus FStateTreeTask_MoveToControllerTarget::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = InstanceData.AIController;
	if (!AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	const UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard || InstanceData.DestinationKey.IsNone())
	{
		return EStateTreeRunStatus::Failed;
	}

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Blackboard->GetValueAsVector(InstanceData.DestinationKey));
	MoveRequest.SetAcceptanceRadius(InstanceData.AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(true);
	MoveRequest.SetCanStrafe(true);

	const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveRequest);
	if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	if (RequestResult.Code == EPathFollowingRequestResult::Failed)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.MoveRequestID = RequestResult.MoveId;
	InstanceData.BoundController = AIController;
	if (UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
	{
		InstanceData.RequestFinishedHandle = PathFollowing->OnRequestFinished.AddLambda(
			[WeakContext = Context.MakeWeakExecutionContext(), RequestID = RequestResult.MoveId](FAIRequestID FinishedID, const FPathFollowingResult& /*Result*/)
			{
				if (FinishedID == RequestID)
				{
					// Patrol / alert do not distinguish arrival from abort; either way the
					// state proceeds (loops or re-selects) once the move settles.
					WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
				}
			});
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_MoveToControllerTarget::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.BoundController.IsValid() && InstanceData.RequestFinishedHandle.IsValid())
	{
		if (UPathFollowingComponent* PathFollowing = InstanceData.BoundController->GetPathFollowingComponent())
		{
			PathFollowing->OnRequestFinished.Remove(InstanceData.RequestFinishedHandle);
		}
	}
	InstanceData.RequestFinishedHandle.Reset();
	InstanceData.BoundController.Reset();
	InstanceData.MoveRequestID = FAIRequestID::InvalidRequest;
}

// ─── Enemy Combat Move ──────────────────────────────────────────────────────

FStateTreeTask_EnemyCombatMove::FStateTreeTask_EnemyCombatMove()
{
	// The slot the evaluator publishes moves with the player, so completion is a
	// per-frame range check plus a repath timer rather than a single path request.
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_EnemyCombatMove::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = InstanceData.AIController;
	const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	if (!Blackboard)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Blackboard->GetValueAsBool(InstanceData.bInAttackRangeKey))
	{
		AIController->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	return IssueMove(InstanceData, true, 0.0f) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeTask_EnemyCombatMove::Tick(
	FStateTreeExecutionContext& Context, const float /*DeltaTime*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = InstanceData.AIController;
	const UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	if (!Blackboard)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Blackboard->GetValueAsBool(InstanceData.bInAttackRangeKey))
	{
		AIController->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	const UWorld* World = AIController->GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const FVector MoveTarget = Blackboard->GetValueAsVector(InstanceData.MoveTargetLocationKey);

	float RepathInterval = 0.25f;
	if (const AYogAIController* YogAI = Cast<AYogAIController>(AIController))
	{
		if (const UEnemyData* EnemyData = YogAI->GetPossessedEnemyData())
		{
			RepathInterval = FMath::Max(EnemyData->MovementTuning.RepathInterval, 0.05f);
		}
	}

	const bool bTargetMoved = !InstanceData.bHasMoveRequest
		|| FVector::DistSquared2D(InstanceData.LastMoveTarget, MoveTarget) >= FMath::Square(InstanceData.TargetRefreshDistance);
	if (bTargetMoved || CurrentTime - InstanceData.LastMoveRequestTime >= RepathInterval)
	{
		if (!IssueMove(InstanceData, bTargetMoved, RepathInterval))
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_EnemyCombatMove::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.bHasMoveRequest = false;
	InstanceData.LastMoveRequestTime = -FLT_MAX;
}

bool FStateTreeTask_EnemyCombatMove::IssueMove(
	FInstanceDataType& InstanceData, bool bTargetMoved, float RepathInterval) const
{
	AAIController* AIController = InstanceData.AIController;
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = AIController ? AIController->GetBlackboardComponent() : nullptr;
	if (!AIController || !ControlledPawn || !Blackboard)
	{
		return false;
	}

	FVector MoveTarget = Blackboard->GetValueAsVector(InstanceData.MoveTargetLocationKey);
	const float AcceptanceRadius = FMath::Max(Blackboard->GetValueAsFloat(InstanceData.AcceptanceRadiusKey), 25.0f);
	const float DistanceToTarget = Blackboard->GetValueAsFloat(InstanceData.DistanceToTargetKey);
	const UEnemyData* EnemyData = nullptr;
	if (const AYogAIController* YogAI = Cast<AYogAIController>(AIController))
	{
		EnemyData = YogAI->GetPossessedEnemyData();
	}
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();
	const float ExitRange = Tuning.AttackRange + FMath::Max(Tuning.AttackRangeExitBuffer, 0.0f);

	// Standing on the slot but still out of attack range means the slot itself is
	// short; push the goal toward the target so the pawn keeps making progress.
	if (!Blackboard->GetValueAsBool(InstanceData.bInAttackRangeKey)
		&& DistanceToTarget > ExitRange
		&& FVector::Dist2D(ControlledPawn->GetActorLocation(), MoveTarget) <= AcceptanceRadius + 10.0f)
	{
		if (const AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(InstanceData.TargetActorKey)))
		{
			FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
			DirectionToTarget.Z = 0.0f;
			if (!DirectionToTarget.IsNearlyZero())
			{
				const float ProgressDistance = FMath::Max(Tuning.AcceptanceRadius + 140.0f, 180.0f);
				MoveTarget = ControlledPawn->GetActorLocation() + DirectionToTarget.GetSafeNormal2D() * ProgressDistance;
				if (UWorld* World = AIController->GetWorld())
				{
					if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
					{
						FNavLocation NavLocation;
						if (NavSys->ProjectPointToNavigation(MoveTarget, NavLocation, FVector(220.0f, 220.0f, 300.0f)))
						{
							MoveTarget = NavLocation.Location;
						}
					}
				}
				Blackboard->SetValueAsVector(InstanceData.MoveTargetLocationKey, MoveTarget);
			}
		}
	}

	const AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(InstanceData.TargetActorKey));
	if (UWorld* World = AIController->GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			const FVector ProjectionExtent(500.0f, 500.0f, 800.0f);
			if (NavSys->ProjectPointToNavigation(MoveTarget, NavLocation, ProjectionExtent))
			{
				MoveTarget = NavLocation.Location;
				Blackboard->SetValueAsVector(InstanceData.MoveTargetLocationKey, MoveTarget);
			}
			else if (TargetActor && DistanceToTarget > ExitRange)
			{
				FVector DirectionToTarget = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
				DirectionToTarget.Z = 0.0f;
				if (!DirectionToTarget.IsNearlyZero())
				{
					const float ProgressDistance = FMath::Clamp(
						DistanceToTarget - ExitRange * 0.5f,
						FMath::Max(Tuning.AcceptanceRadius + 140.0f, 180.0f),
						FMath::Max(Tuning.ForwardTurnLeadDistance, 240.0f));
					const FVector ProgressTarget = ControlledPawn->GetActorLocation() + DirectionToTarget.GetSafeNormal2D() * ProgressDistance;
					if (NavSys->ProjectPointToNavigation(ProgressTarget, NavLocation, ProjectionExtent))
					{
						MoveTarget = NavLocation.Location;
					}
					else
					{
						MoveTarget = ProgressTarget;
						MoveTarget.Z = ControlledPawn->GetActorLocation().Z;
					}
					Blackboard->SetValueAsVector(InstanceData.MoveTargetLocationKey, MoveTarget);
				}
			}
			else
			{
				MoveTarget.Z = ControlledPawn->GetActorLocation().Z;
				Blackboard->SetValueAsVector(InstanceData.MoveTargetLocationKey, MoveTarget);
			}
		}
	}

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		MoveTarget,
		AcceptanceRadius,
		false,
		true,
		true,
		false,
		nullptr,
		true);

	if (AYogAIController* YogAI = Cast<AYogAIController>(AIController))
	{
		YogAI->RecordCombatMoveRequestForDebug(
			MoveTarget,
			static_cast<int32>(MoveResult),
			bTargetMoved,
			RepathInterval,
			AcceptanceRadius);
	}

	const UWorld* World = AIController->GetWorld();
	InstanceData.LastMoveTarget = MoveTarget;
	InstanceData.LastMoveRequestTime = World ? World->GetTimeSeconds() : 0.0f;
	InstanceData.bHasMoveRequest = true;

	// A failed MoveTo request must not fail the task. If it does, the combat state
	// re-selects every attack branch and can starve movement entirely while spamming logs.
	return true;
}

// ─── Debug Print ────────────────────────────────────────────────────────────

FStateTreeTask_DebugPrint::FStateTreeTask_DebugPrint()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_DebugPrint::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const FString Line = FString::Printf(
		TEXT("%s | Actor=%s"),
		*InstanceData.Message,
		*GetNameSafe(InstanceData.ReferenceActor));

	UE_LOG(LogTemp, Log, TEXT("[StateTree] %s"), *Line);

	if (InstanceData.bPrintToScreen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, InstanceData.ScreenDuration, FColor::Cyan, Line);
	}

	return EStateTreeRunStatus::Succeeded;
}

// ─── Call Lua ───────────────────────────────────────────────────────────────

namespace
{
	// Requires LuaModule and leaves it on top of the stack. Pushes the message handler
	// below it so callers can lua_pcall with MsgHandlerIdx without a second lookup.
	// Returns false (stack already unwound to Top) on any failure.
	bool StateTree_RequireLuaModule(lua_State* L, int32 Top, const FString& LuaModule, int32& OutMsgHandlerIdx)
	{
		lua_pushcfunction(L, UnLua::ReportLuaCallError);
		OutMsgHandlerIdx = lua_gettop(L);

		lua_getglobal(L, "require");
		lua_pushstring(L, TCHAR_TO_UTF8(*LuaModule));
		if (lua_pcall(L, 1, 1, OutMsgHandlerIdx) != LUA_OK || !lua_istable(L, -1))
		{
			UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] could not require '%s'."), *LuaModule);
			lua_settop(L, Top);
			return false;
		}

		return true;
	}

	// Looks up FieldName on the module left on top of the stack by StateTree_RequireLuaModule.
	// Pops the module either way, leaving only FieldName (if a function) above the message handler.
	bool StateTree_GetModuleFunction(lua_State* L, int32 Top, const char* FieldName)
	{
		const bool bIsFunction = lua_getfield(L, -1, FieldName) == LUA_TFUNCTION;
		lua_remove(L, -2); // drop the module table, keep the field
		if (!bIsFunction)
		{
			lua_settop(L, Top);
		}
		return bIsFunction;
	}
}

FStateTreeTask_CallLua::FStateTreeTask_CallLua()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_CallLua::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bCheckedForTick = false;
	InstanceData.bHasTick = false;

	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] Call Lua task has no Owner or LuaModule; failing."));
		return EStateTreeRunStatus::Failed;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s has no Lua env; '%s' skipped."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		return EStateTreeRunStatus::Failed;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!StateTree_GetModuleFunction(L, Top, "OnEnter"))
	{
		// OnEnter is optional: a module that only implements OnTick is valid.
		return EStateTreeRunStatus::Running;
	}

	UnLua::PushUObject(L, InstanceData.Owner);
	if (lua_pcall(L, 1, 1, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnEnter errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		lua_settop(L, Top);
		return EStateTreeRunStatus::Failed;
	}

	// Explicit false fails the task; nil/true/anything else keeps it Running.
	const bool bFailed = lua_isboolean(L, -1) && !lua_toboolean(L, -1);
	lua_settop(L, Top);
	return bFailed ? EStateTreeRunStatus::Failed : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_CallLua::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	// Module confirmed on a prior tick to have no OnTick: skip the require + field lookup
	// every frame rather than repeating it for a module that will never answer.
	if (InstanceData.bCheckedForTick && !InstanceData.bHasTick)
	{
		return EStateTreeRunStatus::Running;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		return EStateTreeRunStatus::Failed;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!StateTree_GetModuleFunction(L, Top, "OnTick"))
	{
		// Checked once per EnterState rather than once per Tick to avoid a require + field
		// lookup every frame for the (common) case of a module with no OnTick at all.
		InstanceData.bCheckedForTick = true;
		InstanceData.bHasTick = false;
		return EStateTreeRunStatus::Running;
	}
	InstanceData.bCheckedForTick = true;
	InstanceData.bHasTick = true;

	UnLua::PushUObject(L, InstanceData.Owner);
	lua_pushnumber(L, DeltaTime);
	if (lua_pcall(L, 2, 1, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnTick errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		lua_settop(L, Top);
		return EStateTreeRunStatus::Failed;
	}

	EStateTreeRunStatus Result = EStateTreeRunStatus::Running;
	if (lua_isboolean(L, -1))
	{
		Result = lua_toboolean(L, -1) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
	}

	lua_settop(L, Top);
	return Result;
}

void FStateTreeTask_CallLua::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		return;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		return;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return;
	}

	if (!StateTree_GetModuleFunction(L, Top, "OnExit"))
	{
		return;
	}

	UnLua::PushUObject(L, InstanceData.Owner);
	if (lua_pcall(L, 1, 0, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnExit errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
	}

	lua_settop(L, Top);
}

// ─── Call Lua Spawn ─────────────────────────────────────────────────────────

static void StateTree_PushSpawnParams(lua_State* L, const FStateTreeTask_CallLuaSpawnInstanceData& InstanceData)
{
	lua_createtable(L, 0, 3);

	if (InstanceData.EnemyData)
	{
		UnLua::PushUObject(L, InstanceData.EnemyData);
	}
	else
	{
		lua_pushnil(L);
	}
	lua_setfield(L, -2, "EnemyData");

	lua_pushnumber(L, InstanceData.SpawnRadius);
	lua_setfield(L, -2, "Radius");

	lua_pushinteger(L, InstanceData.SpawnCount);
	lua_setfield(L, -2, "Count");
}

// true -> Succeeded, false -> Failed, anything else (including nil) -> Running.
static EStateTreeRunStatus StateTree_StatusFromLuaResult(lua_State* L)
{
	if (!lua_isboolean(L, -1))
	{
		return EStateTreeRunStatus::Running;
	}

	return lua_toboolean(L, -1) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

FStateTreeTask_CallLuaSpawn::FStateTreeTask_CallLuaSpawn()
{
	bShouldCallTick = true;
}

EStateTreeRunStatus FStateTreeTask_CallLuaSpawn::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bCheckedForTick = false;
	InstanceData.bHasTick = false;

	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] Call Lua Spawn task has no Owner or LuaModule; failing."));
		return EStateTreeRunStatus::Failed;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s has no Lua env; '%s' skipped."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		return EStateTreeRunStatus::Failed;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!StateTree_GetModuleFunction(L, Top, "OnEnter"))
	{
		return EStateTreeRunStatus::Running;
	}

	UnLua::PushUObject(L, InstanceData.Owner);
	StateTree_PushSpawnParams(L, InstanceData);
	if (lua_pcall(L, 2, 1, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnEnter errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		lua_settop(L, Top);
		return EStateTreeRunStatus::Failed;
	}

	const EStateTreeRunStatus Result = StateTree_StatusFromLuaResult(L);
	lua_settop(L, Top);
	return Result;
}

EStateTreeRunStatus FStateTreeTask_CallLuaSpawn::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bCheckedForTick && !InstanceData.bHasTick)
	{
		return EStateTreeRunStatus::Running;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		return EStateTreeRunStatus::Failed;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bCheckedForTick = true;
	InstanceData.bHasTick = StateTree_GetModuleFunction(L, Top, "OnTick");
	if (!InstanceData.bHasTick)
	{
		return EStateTreeRunStatus::Running;
	}

	UnLua::PushUObject(L, InstanceData.Owner);
	lua_pushnumber(L, DeltaTime);
	StateTree_PushSpawnParams(L, InstanceData);
	if (lua_pcall(L, 3, 1, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnTick errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
		lua_settop(L, Top);
		return EStateTreeRunStatus::Failed;
	}

	const EStateTreeRunStatus Result = StateTree_StatusFromLuaResult(L);
	lua_settop(L, Top);
	return Result;
}

void FStateTreeTask_CallLuaSpawn::ExitState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Owner || InstanceData.LuaModule.IsEmpty())
	{
		return;
	}

	UnLua::FLuaEnv* Env = IUnLuaModule::Get().GetEnv(InstanceData.Owner);
	lua_State* L = Env ? Env->GetMainState() : nullptr;
	if (!L)
	{
		return;
	}

	const int32 Top = lua_gettop(L);
	int32 MsgHandlerIdx;
	if (!StateTree_RequireLuaModule(L, Top, InstanceData.LuaModule, MsgHandlerIdx))
	{
		return;
	}

	if (!StateTree_GetModuleFunction(L, Top, "OnExit"))
	{
		return;
	}

	UnLua::PushUObject(L, InstanceData.Owner);
	StateTree_PushSpawnParams(L, InstanceData);
	if (lua_pcall(L, 2, 0, MsgHandlerIdx) != LUA_OK)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StateTree][Lua] %s '%s'.OnExit errored."),
			*GetNameSafe(InstanceData.Owner),
			*InstanceData.LuaModule);
	}

	lua_settop(L, Top);
}
