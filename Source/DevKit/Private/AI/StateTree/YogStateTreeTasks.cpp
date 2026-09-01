#include "AI/StateTree/YogStateTreeTasks.h"

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

// ─── Activate Ability By Tag ────────────────────────────────────────────────

namespace
{
	bool FindReachableSpawnLocation(
		UWorld* World,
		const FVector& Origin,
		float SpawnRadius,
		float MinSpawnDistance,
		int32 MaxAttempts,
		FVector& OutLocation)
	{
		if (!World || SpawnRadius <= 0.0f)
		{
			return false;
		}

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSys)
		{
			return false;
		}

		const float SafeMinDistance = FMath::Clamp(MinSpawnDistance, 0.0f, SpawnRadius);
		const int32 SafeMaxAttempts = FMath::Max(MaxAttempts, 1);
		for (int32 Attempt = 0; Attempt < SafeMaxAttempts; ++Attempt)
		{
			FNavLocation NavLocation;
			if (!NavSys->GetRandomReachablePointInRadius(Origin, SpawnRadius, NavLocation))
			{
				continue;
			}

			if (SafeMinDistance > 0.0f
				&& FVector::DistSquared2D(Origin, NavLocation.Location) < FMath::Square(SafeMinDistance))
			{
				continue;
			}

			OutLocation = NavLocation.Location;
			return true;
		}

		return false;
	}

	AEnemyCharacterBase* SpawnEnemyAtLocation(
		UWorld* World,
		AActor* SpawnOwner,
		APawn* InstigatorPawn,
		TSubclassOf<AEnemyCharacterBase> EnemyClass,
		const FVector& SpawnLocation)
	{
		if (!World || !EnemyClass)
		{
			return nullptr;
		}

		if (AMobSpawner* MobSpawner = Cast<AMobSpawner>(SpawnOwner))
		{
			return MobSpawner->SpawnMobAtLocation(EnemyClass, SpawnLocation);
		}

		AEnemyCharacterBase* Spawned = World->SpawnActorDeferred<AEnemyCharacterBase>(
			EnemyClass,
			FTransform(FRotator::ZeroRotator, SpawnLocation),
			SpawnOwner,
			InstigatorPawn,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Spawned)
		{
			return nullptr;
		}

		Spawned->FinishSpawning(FTransform(FRotator::ZeroRotator, SpawnLocation));
		if (!Spawned->GetController())
		{
			Spawned->SpawnDefaultController();
		}
		if (AYogGameMode* GM = World->GetAuthGameMode<AYogGameMode>())
		{
			GM->RegisterEnemy(Spawned);
		}

		return Spawned;
	}
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
	if (!FindReachableSpawnLocation(
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

	AEnemyCharacterBase* SpawnedEnemy = SpawnEnemyAtLocation(
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

// ─── Move To Controller Target ──────────────────────────────────────────────

FStateTreeTask_HoldPosition::FStateTreeTask_HoldPosition()
{
	bShouldCallTick = false;
}

EStateTreeRunStatus FStateTreeTask_HoldPosition::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& /*Transition*/) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.AIController->StopMovement();
	return EStateTreeRunStatus::Running;
}

FStateTreeTask_MoveToControllerTarget::FStateTreeTask_MoveToControllerTarget()
{
	// Completion is delegate-driven off the path-following request; no tick needed.
	bShouldCallTick = false;
}

namespace
{
	// Slack on top of the move request's reach test before a weave leg counts as issuable. The reach
	// test uses the movement component's nav agent radius, which GetSimpleCollisionRadius only
	// approximates, so leave room for the two to disagree.
	constexpr float YogStateTree_SnakeLegMargin = 50.0f;

	// One leg of the weave: advance toward the player, offset sideways onto whichever side the
	// caller has already chosen in bSnakeToRight. SnakeRandomness jitters both distances here.
	FVector YogStateTree_ComputeSnakeWaypoint(
		const APawn& Pawn,
		const AActor& Player,
		const FStateTreeTask_ChasePlayerUntilDistanceInstanceData& InstanceData,
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
		const float ForwardRoom = FMath::Max(DistanceToPlayer - InstanceData.StopDistance, 0.0f);
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

	const float Distance = FVector::Dist2D(Pawn->GetActorLocation(), Player->GetActorLocation());
	if (Distance <= InstanceData.StopDistance)
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
		&& Distance > InstanceData.StopDistance + MinLegLength;

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

			const FVector Waypoint = YogStateTree_ComputeSnakeWaypoint(*Pawn, *Player, InstanceData, MinLegLength);
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
