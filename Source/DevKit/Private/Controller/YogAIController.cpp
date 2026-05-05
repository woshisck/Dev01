// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/YogAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/CombatItemComponent.h"
#include "Data/AbilityData.h"
#include "GameModes/YogGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemyMoveSmooth, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogEnemyAIState, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogEnemyMovementAttack, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarEnemyMoveSmoothLog(
		TEXT("DevKit.EnemyAI.MoveSmoothLog"),
		0,
		TEXT("0=off, 1=summary, 2=summary plus MoveTo requests for enemy movement smoothing diagnostics."));

	TAutoConsoleVariable<float> CVarEnemyMoveSmoothLogInterval(
		TEXT("DevKit.EnemyAI.MoveSmoothLogInterval"),
		0.35f,
		TEXT("Seconds between enemy movement smoothing diagnostic summary logs per AI."));

	TAutoConsoleVariable<float> CVarEnemyMoveSmoothWarnTargetJump(
		TEXT("DevKit.EnemyAI.MoveSmoothWarnTargetJump"),
		140.0f,
		TEXT("Move target delta that marks an enemy movement smoothing sample as unstable."));

	TAutoConsoleVariable<float> CVarEnemyMoveSmoothWarnYawDelta(
		TEXT("DevKit.EnemyAI.MoveSmoothWarnYawDelta"),
		75.0f,
		TEXT("Desired yaw delta that marks an enemy movement smoothing sample as unstable."));

	TAutoConsoleVariable<float> CVarEnemyMoveSmoothWarnYawRate(
		TEXT("DevKit.EnemyAI.MoveSmoothWarnYawRate"),
		540.0f,
		TEXT("Actor yaw rate that marks an enemy movement smoothing sample as unstable."));

	TAutoConsoleVariable<int32> CVarEnemyAIStateLog(
		TEXT("DevKit.EnemyAI.StateLog"),
		0,
		TEXT("0=off, 1=state transitions, 2=state transitions plus awareness samples."));

	TAutoConsoleVariable<float> CVarEnemyAIStateLogInterval(
		TEXT("DevKit.EnemyAI.StateLogInterval"),
		0.5f,
		TEXT("Seconds between enemy AI awareness sample logs per AI when DevKit.EnemyAI.StateLog >= 2."));

	TAutoConsoleVariable<int32> CVarEnemyMovementAttackLog(
		TEXT("DevKit.EnemyAI.MovementAttackLog"),
		0,
		TEXT("0=off, 1=log movement attack cooldown activation, reset, and range gating."));

	const TCHAR* EnemyAIStateToString(EEnemyAIState State)
	{
		switch (State)
		{
		case EEnemyAIState::Patrol:
			return TEXT("Patrol");
		case EEnemyAIState::Alert:
			return TEXT("Alert");
		case EEnemyAIState::Combat:
			return TEXT("Combat");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* PathFollowingStatusToString(EPathFollowingStatus::Type Status)
	{
		switch (Status)
		{
		case EPathFollowingStatus::Idle:
			return TEXT("Idle");
		case EPathFollowingStatus::Waiting:
			return TEXT("Waiting");
		case EPathFollowingStatus::Paused:
			return TEXT("Paused");
		case EPathFollowingStatus::Moving:
			return TEXT("Moving");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* MoveRequestResultToString(int32 ResultCode)
	{
		switch (static_cast<EPathFollowingRequestResult::Type>(ResultCode))
		{
		case EPathFollowingRequestResult::Failed:
			return TEXT("Failed");
		case EPathFollowingRequestResult::AlreadyAtGoal:
			return TEXT("AlreadyAtGoal");
		case EPathFollowingRequestResult::RequestSuccessful:
			return TEXT("RequestSuccessful");
		default:
			return TEXT("Unknown");
		}
	}

	FName GetMovementAttackCooldownKey(const FEnemyAIAttackOption& Attack)
	{
		if (!Attack.AttackName.IsNone())
		{
			return Attack.AttackName;
		}

		return FName(*Attack.AbilityTags.ToStringSimple());
	}

	FName GetAttackCooldownKey(const FEnemyAIAttackOption& Attack)
	{
		return GetMovementAttackCooldownKey(Attack);
	}

	bool HasAnyValidAbilityTag(const UAbilityData* AbilityData, const FEnemyAIAttackOption& Attack)
	{
		if (!AbilityData || Attack.AbilityTags.IsEmpty())
		{
			return false;
		}

		for (const FGameplayTag& Tag : Attack.AbilityTags)
		{
			if (AbilityData->HasAbility(Tag))
			{
				return true;
			}
		}

		return false;
	}
}


AYogAIController::AYogAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("YogBT"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("YogBB"));
}

bool AYogAIController::RunBTWithBlackboard(UBehaviorTree* BT, UBlackboardData* BB)
{
    if (!BT)
    {
        UE_LOG(LogTemp, Warning, TEXT("[YogAIController] RunBTWithBlackboard FAIL: BT is null"));
        return false;
    }
	bool bInitializedPatrol = false;
    // 1) 先绑黑板（绕开 BT 内部 BlackboardAsset 断链）
    if (BB && BlackboardComponent)
    {
        const bool bUsed = UseBlackboard(BB, BlackboardComponent);
        UE_LOG(LogTemp, Warning, TEXT("[YogAIController] UseBlackboard(%s) -> %d"),
            *BB->GetName(), bUsed ? 1 : 0);
		if (bUsed)
		{
			InitializePatrolState();
			bInitializedPatrol = true;
		}
    }
    // 2) 启动 BT（RunBehaviorTree 内部如果发现 BB 已绑会复用，不会覆盖）
    const bool bRan = RunBehaviorTree(BT);
    UE_LOG(LogTemp, Warning, TEXT("[YogAIController] RunBehaviorTree(%s) -> %d"),
        *BT->GetName(), bRan ? 1 : 0);
	if (!bInitializedPatrol && bRan)
	{
		InitializePatrolState();
	}
    return bRan;
}

void AYogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ApplyCrowdTuningFromEnemyData();

	if (UEnemyData* EnemyData = GetPossessedEnemyData())
	{
		if (EnemyData->BehaviorTree)
		{
			RunBTWithBlackboard(EnemyData->BehaviorTree, EnemyData->BehaviorTree->BlackboardAsset);
			return;
		}
	}

	// 兜底启动：BT 内部 BlackboardAsset 断链时由 C++ 强制 UseBlackboard + RunBehaviorTree
	if (bUseFallbackStartup && FallbackBehaviorTree)
	{
		RunBTWithBlackboard(FallbackBehaviorTree, FallbackBlackboard);
	}
}

UEnemyData* AYogAIController::GetPossessedEnemyData() const
{
	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetPawn());
	if (!Enemy)
	{
		return nullptr;
	}

	const UCharacterDataComponent* DataComponent = Enemy->GetCharacterDataComponent();
	return DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
}

UBlackboardComponent* AYogAIController::ResolveBlackboardComponent() const
{
	if (BlackboardComponent)
	{
		return BlackboardComponent;
	}

	return const_cast<UBlackboardComponent*>(GetBlackboardComponent());
}

void AYogAIController::SetEnemyAIState(EEnemyAIState NewState)
{
	if (UBlackboardComponent* BB = ResolveBlackboardComponent())
	{
		const EEnemyAIState PreviousState = static_cast<EEnemyAIState>(BB->GetValueAsEnum(TEXT("EnemyAIState")));
		BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(NewState));
		if (CVarEnemyAIStateLog.GetValueOnGameThread() > 0 && PreviousState != NewState)
		{
			UE_LOG(LogEnemyAIState, Log,
				TEXT("[EnemyAIState] Enemy=%s %s -> %s Reason=ManualSet Target=%s"),
				*GetNameSafe(GetPawn()),
				EnemyAIStateToString(PreviousState),
				EnemyAIStateToString(NewState),
				*GetNameSafe(Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")))));
		}
	}
}

void AYogAIController::InitializePatrolState()
{
	UBlackboardComponent* BB = ResolveBlackboardComponent();
	APawn* ControlledPawn = GetPawn();
	if (!BB || !ControlledPawn)
	{
		return;
	}

	const FVector SpawnLocation = ControlledPawn->GetActorLocation();
	const EEnemyAIState PreviousState = static_cast<EEnemyAIState>(BB->GetValueAsEnum(TEXT("EnemyAIState")));
	BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(EEnemyAIState::Patrol));
	BB->ClearValue(TEXT("TargetActor"));
	BB->SetValueAsVector(TEXT("PatrolOriginLocation"), SpawnLocation);
	BB->SetValueAsVector(TEXT("PatrolTargetLocation"), SpawnLocation);
	BB->SetValueAsVector(TEXT("MoveTargetLocation"), SpawnLocation);
	BB->SetValueAsVector(TEXT("LastKnownTargetLocation"), SpawnLocation);
	BB->SetValueAsFloat(TEXT("DistanceToTarget"), 0.0f);
	BB->SetValueAsBool(TEXT("bInAttackRange"), false);
	BB->SetValueAsFloat(TEXT("AlertExpireTime"), 0.0f);
	BB->SetValueAsFloat(TEXT("LastSeenTargetTime"), 0.0f);

	if (const UEnemyData* EnemyData = GetPossessedEnemyData())
	{
		BB->SetValueAsFloat(TEXT("AcceptanceRadius"), EnemyData->MovementTuning.AcceptanceRadius);
	}

	ResetCombatMoveSmoothing();
	if (CVarEnemyAIStateLog.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogEnemyAIState, Log,
			TEXT("[EnemyAIState] Enemy=%s %s -> Patrol Reason=InitializePatrol Spawn=%s"),
			*GetNameSafe(ControlledPawn),
			EnemyAIStateToString(PreviousState),
			*SpawnLocation.ToCompactString());
	}
}

EEnemyAIState AYogAIController::GetEnemyAIState() const
{
	if (const UBlackboardComponent* BB = ResolveBlackboardComponent())
	{
		return static_cast<EEnemyAIState>(BB->GetValueAsEnum(TEXT("EnemyAIState")));
	}

	return EEnemyAIState::Patrol;
}

void AYogAIController::EnterCombat(AActor* TargetActor, bool bBroadcastAlert)
{
	UBlackboardComponent* BB = ResolveBlackboardComponent();
	if (!BB)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FVector TargetLocation = TargetActor ? TargetActor->GetActorLocation() : GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const EEnemyAIState PreviousState = static_cast<EEnemyAIState>(BB->GetValueAsEnum(TEXT("EnemyAIState")));

	BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(EEnemyAIState::Combat));
	BB->SetValueAsObject(TEXT("TargetActor"), TargetActor);
	BB->SetValueAsVector(TEXT("LastKnownTargetLocation"), TargetLocation);
	BB->SetValueAsFloat(TEXT("LastSeenTargetTime"), CurrentTime);
	BB->SetValueAsFloat(TEXT("AlertExpireTime"), 0.0f);

	if (bBroadcastAlert)
	{
		BroadcastAlert(TargetActor, TargetLocation);
	}
	if (CVarEnemyAIStateLog.GetValueOnGameThread() > 0
		&& (PreviousState != EEnemyAIState::Combat || CVarEnemyAIStateLog.GetValueOnGameThread() >= 2))
	{
		UE_LOG(LogEnemyAIState, Log,
			TEXT("[EnemyAIState] Enemy=%s %s -> Combat Reason=EnterCombat Target=%s TargetLoc=%s Broadcast=%d"),
			*GetNameSafe(GetPawn()),
			EnemyAIStateToString(PreviousState),
			*GetNameSafe(TargetActor),
			*TargetLocation.ToCompactString(),
			bBroadcastAlert ? 1 : 0);
	}
}

void AYogAIController::EnterAlert(AActor* TargetActor, FVector AlertLocation, bool bBroadcastAlert)
{
	UBlackboardComponent* BB = ResolveBlackboardComponent();
	if (!BB)
	{
		return;
	}

	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const EEnemyAIState PreviousState = static_cast<EEnemyAIState>(BB->GetValueAsEnum(TEXT("EnemyAIState")));

	if (AlertLocation.IsNearlyZero() && TargetActor)
	{
		AlertLocation = TargetActor->GetActorLocation();
	}

	BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(EEnemyAIState::Alert));
	BB->SetValueAsObject(TEXT("TargetActor"), TargetActor);
	BB->SetValueAsVector(TEXT("LastKnownTargetLocation"), AlertLocation);
	BB->SetValueAsFloat(TEXT("AlertExpireTime"), CurrentTime + Tuning.AlertDuration);

	if (bBroadcastAlert)
	{
		BroadcastAlert(TargetActor, AlertLocation);
	}
	if (CVarEnemyAIStateLog.GetValueOnGameThread() > 0
		&& (PreviousState != EEnemyAIState::Alert || CVarEnemyAIStateLog.GetValueOnGameThread() >= 2))
	{
		UE_LOG(LogEnemyAIState, Log,
			TEXT("[EnemyAIState] Enemy=%s %s -> Alert Reason=EnterAlert Target=%s AlertLoc=%s Expire=%.2f Broadcast=%d"),
			*GetNameSafe(GetPawn()),
			EnemyAIStateToString(PreviousState),
			*GetNameSafe(TargetActor),
			*AlertLocation.ToCompactString(),
			CurrentTime + Tuning.AlertDuration,
			bBroadcastAlert ? 1 : 0);
	}
}

void AYogAIController::BroadcastAlert(AActor* TargetActor, FVector AlertLocation) const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();
	AYogGameMode* GameMode = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World));
	if (!GameMode)
	{
		return;
	}

	const FVector Origin = ControlledPawn->GetActorLocation();
	for (AEnemyCharacterBase* NearbyEnemy : GameMode->GetNearbyEnemies(Origin, Tuning.AlertBroadcastRadius))
	{
		if (!NearbyEnemy || NearbyEnemy == ControlledPawn)
		{
			continue;
		}

		AYogAIController* NearbyAI = Cast<AYogAIController>(NearbyEnemy->GetController());
		if (!NearbyAI || NearbyAI->GetEnemyAIState() == EEnemyAIState::Combat)
		{
			continue;
		}

		NearbyAI->EnterAlert(TargetActor, AlertLocation, false);
	}
}

void AYogAIController::ApplyCrowdTuningFromEnemyData()
{
	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();

	if (UCrowdFollowingComponent* CrowdFollowing = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdFollowing->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
		CrowdFollowing->SetCrowdAnticipateTurns(true);
		CrowdFollowing->SetCrowdObstacleAvoidance(true);
		CrowdFollowing->SetCrowdSeparation(true);
		CrowdFollowing->SetCrowdSeparationWeight(Tuning.CrowdSeparationWeight);
		CrowdFollowing->SetCrowdOptimizeVisibility(true);
		CrowdFollowing->SetCrowdOptimizeTopology(true);
		CrowdFollowing->SetCrowdPathOffset(true);
		CrowdFollowing->SetCrowdSlowdownAtGoal(true);
		CrowdFollowing->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
		CrowdFollowing->SetCrowdCollisionQueryRange(FMath::Max(300.f, Tuning.PreferredRange + Tuning.FlankDistance + 200.f));
		CrowdFollowing->SetCrowdPathOptimizationRange(FMath::Max(600.f, Tuning.PreferredRange + 300.f));
	}

	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn()))
	{
		ControlledCharacter->bUseControllerRotationYaw = false;
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			// Detour Crowd drives local avoidance for AI; do not run RVO avoidance on top of it.
			Movement->bUseRVOAvoidance = false;
			Movement->bUseControllerDesiredRotation = false;
			Movement->bOrientRotationToMovement = Tuning.bUseForwardSteering;
			if (Tuning.MaxTurnYawSpeed > 0.0f)
			{
				Movement->RotationRate.Yaw = Tuning.MaxTurnYawSpeed * UCombatItemComponent::GetStickyOilTurnSpeedMultiplier(ControlledCharacter);
			}
			if (Tuning.MaxWalkSpeedOverride > 0.0f)
			{
				Movement->MaxWalkSpeed = Tuning.MaxWalkSpeedOverride;
			}
		}
	}
}

FVector AYogAIController::ComputeCombatMoveTarget(const AActor& TargetActor, const FEnemyAIMovementTuning& Tuning, float EffectiveAttackRange)
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return TargetActor.GetActorLocation();
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = TargetActor.GetActorLocation();
	const float DistanceToTarget = FVector::Dist2D(PawnLocation, TargetLocation);

	FVector DesiredLocation = TargetLocation;
	const float ValidAttackRange = FMath::Max(EffectiveAttackRange > 0.0f ? EffectiveAttackRange : Tuning.AttackRange, 1.0f);
	const float AcceptanceBuffer = FMath::Max(Tuning.AcceptanceRadius, 0.0f);
	const float MaxGoalRange = FMath::Max(40.0f, ValidAttackRange - AcceptanceBuffer);
	const float PreferredRange = Tuning.PreferredRange > 0.0f
		? FMath::Clamp(Tuning.PreferredRange, 40.0f, MaxGoalRange)
		: MaxGoalRange;

	if (DistanceToTarget <= ValidAttackRange)
	{
		if (Tuning.ApproachStyle != EEnemyAIApproachStyle::BruiserHold || DistanceToTarget > PreferredRange * 0.65f)
		{
			return PawnLocation;
		}
	}

	const FVector FromTargetToPawn = (PawnLocation - TargetLocation).GetSafeNormal2D();
	const FVector TargetForward = TargetActor.GetActorForwardVector().GetSafeNormal2D();
	const FVector TargetRight = TargetActor.GetActorRightVector().GetSafeNormal2D();

	switch (Tuning.ApproachStyle)
	{
	case EEnemyAIApproachStyle::SwarmFlank:
	{
		const uint32 StableId = ControlledPawn->GetUniqueID();
		const float SideSign = (StableId % 2 == 0) ? 1.f : -1.f;
		const bool bUseFlank = Tuning.StrafeChance > 0.f;
		FVector FlankDirection = (-TargetForward * 0.65f) + (TargetRight * SideSign * (bUseFlank ? 0.75f : 0.25f));
		if (FlankDirection.IsNearlyZero())
		{
			FlankDirection = FromTargetToPawn.IsNearlyZero() ? FVector::ForwardVector : FromTargetToPawn;
		}
		DesiredLocation = TargetLocation + FlankDirection.GetSafeNormal2D() * PreferredRange;
		DesiredLocation += TargetRight * SideSign * Tuning.FlankDistance;
		break;
	}
	case EEnemyAIApproachStyle::BruiserHold:
	{
		FVector HoldDirection = FromTargetToPawn;
		if (HoldDirection.IsNearlyZero())
		{
			HoldDirection = -TargetForward;
		}
		DesiredLocation = TargetLocation + HoldDirection.GetSafeNormal2D() * PreferredRange;
		break;
	}
	case EEnemyAIApproachStyle::Direct:
	default:
	{
		FVector ApproachDirection = FromTargetToPawn;
		if (ApproachDirection.IsNearlyZero())
		{
			ApproachDirection = -TargetForward;
		}
		DesiredLocation = TargetLocation + ApproachDirection.GetSafeNormal2D() * PreferredRange;
		break;
	}
	}

	FVector DesiredOffset = DesiredLocation - TargetLocation;
	DesiredOffset.Z = 0.0f;
	const float DesiredRange = DesiredOffset.Size2D();
	if (DesiredRange > PreferredRange && DesiredRange > KINDA_SMALL_NUMBER)
	{
		DesiredLocation = TargetLocation + DesiredOffset.GetSafeNormal2D() * PreferredRange;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Tuning.ApproachStyle == EEnemyAIApproachStyle::SwarmFlank && Tuning.CombatSlotLockDuration > 0.0f)
	{
		const float LockedSlotMaxDistance = PreferredRange + FMath::Max(Tuning.FlankDistance, 0.0f) + 260.0f;
		const bool bLockedSlotStillUseful = bHasLockedCombatSlot
			&& CurrentTime < LockedCombatSlotExpireTime
			&& FVector::Dist2D(LockedCombatSlotLocation, TargetLocation) <= LockedSlotMaxDistance;
		if (bLockedSlotStillUseful)
		{
			DesiredLocation = LockedCombatSlotLocation;
		}
		else
		{
			LockedCombatSlotLocation = DesiredLocation;
			LockedCombatSlotExpireTime = CurrentTime + Tuning.CombatSlotLockDuration;
			bHasLockedCombatSlot = true;
		}
	}

	DesiredLocation.Z = PawnLocation.Z;

	// Keep forward-steering diagnostics warm, but use the stable combat slot as the MoveTo destination.
	(void)ApplyForwardSteeringToMoveTarget(DesiredLocation, Tuning);

	auto MakeProgressTarget = [&]()
	{
		FVector DirectionToTarget = TargetLocation - PawnLocation;
		DirectionToTarget.Z = 0.0f;
		if (DirectionToTarget.IsNearlyZero())
		{
			DirectionToTarget = ControlledPawn->GetActorForwardVector();
		}

		const float ProgressDistance = FMath::Clamp(
			DistanceToTarget - ValidAttackRange * 0.5f,
			FMath::Max(Tuning.AcceptanceRadius + 120.0f, 160.0f),
			FMath::Max(Tuning.ForwardTurnLeadDistance, 220.0f));
		return PawnLocation + DirectionToTarget.GetSafeNormal2D() * ProgressDistance;
	};

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(DesiredLocation, NavLocation, FVector(220.f, 220.f, 300.f)))
			{
				if (DistanceToTarget > ValidAttackRange + Tuning.AttackRangeExitBuffer
					&& FVector::Dist2D(PawnLocation, NavLocation.Location) <= FMath::Max(Tuning.AcceptanceRadius + 40.0f, 90.0f))
				{
					FNavLocation ProgressNavLocation;
					const FVector ProgressTarget = MakeProgressTarget();
					if (NavSys->ProjectPointToNavigation(ProgressTarget, ProgressNavLocation, FVector(220.f, 220.f, 300.f)))
					{
						return ProgressNavLocation.Location;
					}
					return ProgressTarget;
				}
				return NavLocation.Location;
			}
		}
	}

	if (DistanceToTarget > ValidAttackRange + Tuning.AttackRangeExitBuffer
		&& FVector::Dist2D(PawnLocation, DesiredLocation) <= FMath::Max(Tuning.AcceptanceRadius + 40.0f, 90.0f))
	{
		return MakeProgressTarget();
	}

	return DesiredLocation;
}

FVector AYogAIController::ApplyForwardSteeringToMoveTarget(const FVector& DesiredLocation, const FEnemyAIMovementTuning& Tuning)
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !Tuning.bUseForwardSteering)
	{
		LastCombatMoveDesiredYawDelta = 0.0f;
		LastCombatMoveAppliedYawDelta = 0.0f;
		LastCombatMoveLeadDistance = 0.0f;
		LastCombatMoveSmoothAlpha = 1.0f;
		return DesiredLocation;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector DesiredDirection = DesiredLocation - PawnLocation;
	DesiredDirection.Z = 0.0f;
	const float DistanceToDesired = DesiredDirection.Size2D();
	if (DistanceToDesired <= FMath::Max(Tuning.AcceptanceRadius, 20.0f))
	{
		LastCombatMoveDesiredYawDelta = 0.0f;
		LastCombatMoveAppliedYawDelta = 0.0f;
		LastCombatMoveLeadDistance = DistanceToDesired;
		LastCombatMoveSmoothAlpha = 1.0f;
		return DesiredLocation;
	}

	DesiredDirection = DesiredDirection.GetSafeNormal2D();
	FVector CurrentDirection = bHasSmoothedCombatMove
		? SmoothedCombatMoveDirection
		: ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	if (CurrentDirection.IsNearlyZero())
	{
		CurrentDirection = DesiredDirection;
	}

	const float DeltaSeconds = GetWorld() ? FMath::Clamp(GetWorld()->GetDeltaSeconds(), 1.0f / 120.0f, 1.0f / 20.0f) : 1.0f / 60.0f;
	const float CurrentYaw = CurrentDirection.Rotation().Yaw;
	const float DesiredYaw = DesiredDirection.Rotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);
	const float StickyTurnMultiplier = UCombatItemComponent::GetStickyOilTurnSpeedMultiplier(ControlledPawn);
	const float MaxYawStep = FMath::Max(Tuning.MaxTurnYawSpeed * StickyTurnMultiplier, 1.0f) * DeltaSeconds;
	const float AppliedYawDelta = FMath::Clamp(DeltaYaw, -MaxYawStep, MaxYawStep);
	const float NewYaw = CurrentYaw + AppliedYawDelta;

	SmoothedCombatMoveDirection = FRotator(0.0f, NewYaw, 0.0f).Vector().GetSafeNormal2D();
	const float SharpTurnMultiplier = FMath::Abs(DeltaYaw) >= Tuning.SharpTurnAngle ? 1.35f : 1.0f;
	const float LeadDistance = FMath::Clamp(
		DistanceToDesired,
		FMath::Max(Tuning.AcceptanceRadius, 60.0f),
		FMath::Max(Tuning.ForwardTurnLeadDistance * SharpTurnMultiplier, 80.0f));
	const FVector ForwardTarget = PawnLocation + SmoothedCombatMoveDirection * LeadDistance;

	const float SmoothSpeed = FMath::Max(Tuning.MoveTargetSmoothingSpeed, 0.0f);
	const float Alpha = SmoothSpeed <= 0.0f ? 1.0f : 1.0f - FMath::Exp(-SmoothSpeed * DeltaSeconds);
	SmoothedCombatMoveTarget = bHasSmoothedCombatMove
		? FMath::Lerp(SmoothedCombatMoveTarget, ForwardTarget, Alpha)
		: ForwardTarget;
	bHasSmoothedCombatMove = true;

	LastCombatMoveDesiredYawDelta = DeltaYaw;
	LastCombatMoveAppliedYawDelta = AppliedYawDelta;
	LastCombatMoveLeadDistance = LeadDistance;
	LastCombatMoveSmoothAlpha = Alpha;

	return SmoothedCombatMoveTarget;
}

void AYogAIController::ResetCombatMoveSmoothingAfterAttack()
{
	ResetCombatMoveSmoothing(false);
}

void AYogAIController::SetCombatAttackInProgress(bool bInProgress)
{
	if (bInProgress)
	{
		StopMovement();
		ResetCombatMoveSmoothing(false);
	}
	bCombatAttackInProgress = bInProgress;
}

void AYogAIController::ResetCombatMoveSmoothing(bool bResetCooldowns)
{
	const APawn* ControlledPawn = GetPawn();
	SmoothedCombatMoveDirection = ControlledPawn ? ControlledPawn->GetActorForwardVector().GetSafeNormal2D() : FVector::ZeroVector;
	SmoothedCombatMoveTarget = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	bHasSmoothedCombatMove = false;
	LockedCombatSlotLocation = FVector::ZeroVector;
	LockedCombatSlotExpireTime = -FLT_MAX;
	bHasLockedCombatSlot = false;
	LastCombatMoveDebugPawnLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
	LastCombatMoveDebugMoveTarget = SmoothedCombatMoveTarget;
	LastCombatMoveDebugTime = -FLT_MAX;
	LastCombatMoveDebugActorYaw = ControlledPawn ? ControlledPawn->GetActorRotation().Yaw : 0.0f;
	LastCombatMoveDesiredYawDelta = 0.0f;
	LastCombatMoveAppliedYawDelta = 0.0f;
	LastCombatMoveLeadDistance = 0.0f;
	LastCombatMoveSmoothAlpha = 1.0f;
	CombatMoveRequestsSinceLastDebugLog = 0;
	LastCombatMoveRequestTarget = FVector::ZeroVector;
	LastCombatMoveRequestTime = -FLT_MAX;
	LastCombatMoveRequestInterval = 0.0f;
	LastCombatMoveRequestTargetDelta = 0.0f;
	LastCombatMoveRequestAcceptanceRadius = 0.0f;
	LastCombatMoveRequestResultCode = static_cast<int32>(EPathFollowingRequestResult::Failed);
	bLastCombatMoveRequestTargetMoved = false;
	LastAwarenessDebugLogTime = -FLT_MAX;
	if (bResetCooldowns)
	{
		MovementAttackCooldownEndTimes.Reset();
		AttackCooldownEndTimes.Reset();
		InvalidMovementAttackAbilityLogTimes.Reset();
		LastSelectedAttackKey = NAME_None;
		LastSelectedAttackTime = -FLT_MAX;
	}
	bCombatAttackInProgress = false;
}

void AYogAIController::RecordCombatMoveRequestForDebug(
	const FVector& MoveTarget,
	int32 MoveResultCode,
	bool bTargetMoved,
	float RepathInterval,
	float AcceptanceRadius)
{
	if (CVarEnemyMoveSmoothLog.GetValueOnGameThread() <= 0)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastCombatMoveRequestInterval = LastCombatMoveRequestTime > -FLT_MAX * 0.5f
		? CurrentTime - LastCombatMoveRequestTime
		: 0.0f;
	LastCombatMoveRequestTargetDelta = LastCombatMoveRequestTime > -FLT_MAX * 0.5f
		? FVector::Dist2D(LastCombatMoveRequestTarget, MoveTarget)
		: 0.0f;
	LastCombatMoveRequestTime = CurrentTime;
	LastCombatMoveRequestTarget = MoveTarget;
	LastCombatMoveRequestAcceptanceRadius = AcceptanceRadius;
	LastCombatMoveRequestResultCode = MoveResultCode;
	bLastCombatMoveRequestTargetMoved = bTargetMoved;
	++CombatMoveRequestsSinceLastDebugLog;

	if (CVarEnemyMoveSmoothLog.GetValueOnGameThread() >= 2)
	{
		UE_LOG(LogEnemyMoveSmooth, Log,
			TEXT("[EnemyMoveSmooth:MoveTo] Enemy=%s Result=%s TargetMoved=%d TargetDelta=%.1f RequestInterval=%.3f RepathInterval=%.3f Acceptance=%.1f Target=%s"),
			*GetNameSafe(GetPawn()),
			MoveRequestResultToString(MoveResultCode),
			bTargetMoved ? 1 : 0,
			LastCombatMoveRequestTargetDelta,
			LastCombatMoveRequestInterval,
			RepathInterval,
			AcceptanceRadius,
			*MoveTarget.ToCompactString());
	}
}

float AYogAIController::GetMovementAttackRange(const FEnemyAIAttackOption& Attack) const
{
	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();
	const float MultipliedRange = FMath::Max(Tuning.AttackRange, 1.0f) * FMath::Max(Attack.MovementAttackRangeMultiplier, 0.0f);
	return FMath::Max(Attack.MaxRange, MultipliedRange);
}

void AYogAIController::NotifyAttackActivated(const FEnemyAIAttackOption& Attack)
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastSelectedAttackKey = GetAttackCooldownKey(Attack);
	LastSelectedAttackTime = CurrentTime;

	if (Attack.Cooldown <= 0.0f)
	{
		return;
	}

	AttackCooldownEndTimes.FindOrAdd(GetAttackCooldownKey(Attack)) = CurrentTime + Attack.Cooldown;
}

bool AYogAIController::IsAttackCooldownReadyForAI(const FEnemyAIAttackOption& Attack, float* OutRemainingCooldown) const
{
	if (OutRemainingCooldown)
	{
		*OutRemainingCooldown = 0.0f;
	}

	const float* CooldownEndTime = AttackCooldownEndTimes.Find(GetAttackCooldownKey(Attack));
	if (!CooldownEndTime)
	{
		return true;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime >= *CooldownEndTime)
	{
		return true;
	}

	if (OutRemainingCooldown)
	{
		*OutRemainingCooldown = *CooldownEndTime - CurrentTime;
	}
	return false;
}

bool AYogAIController::IsRecentAttackRepeat(const FEnemyAIAttackOption& Attack, float MemoryDuration, float* OutRepeatAge) const
{
	if (OutRepeatAge)
	{
		*OutRepeatAge = 0.0f;
	}

	if (MemoryDuration <= 0.0f || LastSelectedAttackKey.IsNone() || LastSelectedAttackTime <= -FLT_MAX * 0.5f)
	{
		return false;
	}

	if (GetAttackCooldownKey(Attack) != LastSelectedAttackKey)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float RepeatAge = CurrentTime - LastSelectedAttackTime;
	if (OutRepeatAge)
	{
		*OutRepeatAge = RepeatAge;
	}

	return RepeatAge >= 0.0f && RepeatAge <= MemoryDuration;
}

bool AYogAIController::CanUseMovementAttack(const FEnemyAIAttackOption& Attack, float DistanceToTarget, float* OutRemainingCooldown) const
{
	if (OutRemainingCooldown)
	{
		*OutRemainingCooldown = 0.0f;
	}

	if (Attack.AttackMovementMode == EEnemyAIAttackMovementMode::None)
	{
		return false;
	}

	const float MovementAttackRange = GetMovementAttackRange(Attack);
	if (DistanceToTarget > MovementAttackRange)
	{
		return false;
	}

	if (Attack.LungeStartRange > 0.0f && DistanceToTarget < Attack.LungeStartRange)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FName CooldownKey = GetMovementAttackCooldownKey(Attack);
	const float* CooldownEndTime = MovementAttackCooldownEndTimes.Find(CooldownKey);
	if (CooldownEndTime && CurrentTime < *CooldownEndTime)
	{
		if (OutRemainingCooldown)
		{
			*OutRemainingCooldown = *CooldownEndTime - CurrentTime;
		}
		return false;
	}

	return true;
}

void AYogAIController::NotifyMovementAttackActivated(const FEnemyAIAttackOption& Attack)
{
	if (Attack.AttackMovementMode == EEnemyAIAttackMovementMode::None || Attack.MovementAttackCooldown <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FName CooldownKey = GetMovementAttackCooldownKey(Attack);
	const float CooldownEndTime = CurrentTime + Attack.MovementAttackCooldown;
	MovementAttackCooldownEndTimes.FindOrAdd(CooldownKey) = CooldownEndTime;

	if (CVarEnemyMovementAttackLog.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogEnemyMovementAttack, Log,
			TEXT("[EnemyMovementAttack] Enemy=%s Attack=%s Event=Activated Cooldown=%.2f CooldownEnd=%.2f Range=%.1f"),
			*GetNameSafe(GetPawn()),
			*Attack.AttackName.ToString(),
			Attack.MovementAttackCooldown,
			CooldownEndTime,
			GetMovementAttackRange(Attack));
	}
}

void AYogAIController::RefreshMovementAttackCooldownReset(float DistanceToTarget)
{
	const UEnemyData* EnemyData = GetPossessedEnemyData();
	if (!EnemyData || (MovementAttackCooldownEndTimes.IsEmpty() && AttackCooldownEndTimes.IsEmpty()))
	{
		return;
	}

	for (const FEnemyAIAttackOption& Attack : EnemyData->AttackProfile.Attacks)
	{
		if (Attack.AttackMovementMode == EEnemyAIAttackMovementMode::None)
		{
			continue;
		}

		const float MovementAttackRange = GetMovementAttackRange(Attack);
		if (DistanceToTarget <= MovementAttackRange)
		{
			continue;
		}

		const FName CooldownKey = GetMovementAttackCooldownKey(Attack);
		if (MovementAttackCooldownEndTimes.Remove(CooldownKey) > 0 && CVarEnemyMovementAttackLog.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogEnemyMovementAttack, Log,
				TEXT("[EnemyMovementAttack] Enemy=%s Attack=%s Event=ResetCooldown Reason=TargetLeftMovementAttackRange Dist=%.1f Range=%.1f"),
				*GetNameSafe(GetPawn()),
				*Attack.AttackName.ToString(),
				DistanceToTarget,
				MovementAttackRange);
		}
	}
}

void AYogAIController::LogCombatMoveSmoothSample(
	const AActor& TargetActor,
	const FVector& MoveTargetLocation,
	float DistanceToTarget,
	float EffectiveAttackRange,
	bool bInAttackRange,
	const FEnemyAIMovementTuning& Tuning)
{
	if (CVarEnemyMoveSmoothLog.GetValueOnGameThread() <= 0)
	{
		return;
	}

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float LogInterval = FMath::Max(CVarEnemyMoveSmoothLogInterval.GetValueOnGameThread(), 0.05f);
	if (LastCombatMoveDebugTime > -FLT_MAX * 0.5f && CurrentTime - LastCombatMoveDebugTime < LogInterval)
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const float DeltaSeconds = LastCombatMoveDebugTime > -FLT_MAX * 0.5f
		? FMath::Max(CurrentTime - LastCombatMoveDebugTime, KINDA_SMALL_NUMBER)
		: 0.0f;
	const float ActualSpeed = DeltaSeconds > 0.0f
		? FVector::Dist2D(LastCombatMoveDebugPawnLocation, PawnLocation) / DeltaSeconds
		: 0.0f;
	const float MoveTargetDelta = LastCombatMoveDebugTime > -FLT_MAX * 0.5f
		? FVector::Dist2D(LastCombatMoveDebugMoveTarget, MoveTargetLocation)
		: 0.0f;

	const FVector ToMoveTarget = MoveTargetLocation - PawnLocation;
	const float ActorYaw = ControlledPawn->GetActorRotation().Yaw;
	const float MoveTargetYaw = ToMoveTarget.IsNearlyZero()
		? ActorYaw
		: ToMoveTarget.Rotation().Yaw;
	const float ActorToMoveTargetYawDelta = FMath::FindDeltaAngleDegrees(ActorYaw, MoveTargetYaw);
	const float ActorYawDelta = LastCombatMoveDebugTime > -FLT_MAX * 0.5f
		? FMath::FindDeltaAngleDegrees(LastCombatMoveDebugActorYaw, ActorYaw)
		: 0.0f;
	const float ActorYawRate = DeltaSeconds > 0.0f ? FMath::Abs(ActorYawDelta) / DeltaSeconds : 0.0f;

	const UPathFollowingComponent* PathFollowing = GetPathFollowingComponent();
	const EPathFollowingStatus::Type PathStatus = PathFollowing ? PathFollowing->GetStatus() : EPathFollowingStatus::Idle;
	float CurrentCombatMoveAcceptanceRadius = Tuning.AcceptanceRadius;
	if (const UBlackboardComponent* BB = ResolveBlackboardComponent())
	{
		CurrentCombatMoveAcceptanceRadius = BB->GetValueAsFloat(TEXT("AcceptanceRadius"));
	}
	bool bHasMovementAttack = false;
	bool bMovementAttackAbilityValid = false;
	bool bMovementAttackReady = false;
	float MovementAttackRange = 0.0f;
	float MovementAttackCooldownRemaining = 0.0f;
	float AttackCooldownRemaining = 0.0f;
	if (const UEnemyData* EnemyData = GetPossessedEnemyData())
	{
		const UAbilityData* AbilityData = EnemyData->AbilityData;
		for (const FEnemyAIAttackOption& Attack : EnemyData->AttackProfile.Attacks)
		{
			if (Attack.AttackMovementMode == EEnemyAIAttackMovementMode::None)
			{
				continue;
			}

			bHasMovementAttack = true;
			MovementAttackRange = GetMovementAttackRange(Attack);
			bMovementAttackAbilityValid = HasAnyValidAbilityTag(AbilityData, Attack);
			const bool bAttackCooldownReady = IsAttackCooldownReadyForAI(Attack, &AttackCooldownRemaining);
			bMovementAttackReady = bMovementAttackAbilityValid
				&& bAttackCooldownReady
				&& CanUseMovementAttack(Attack, DistanceToTarget, &MovementAttackCooldownRemaining);
			break;
		}
	}
	const bool bEvaluateMoveStability = !bInAttackRange && !bCombatAttackInProgress;
	const bool bTargetJump = bEvaluateMoveStability && MoveTargetDelta >= CVarEnemyMoveSmoothWarnTargetJump.GetValueOnGameThread();
	const bool bYawJump = bEvaluateMoveStability && FMath::Abs(LastCombatMoveDesiredYawDelta) >= CVarEnemyMoveSmoothWarnYawDelta.GetValueOnGameThread();
	const bool bYawRateHigh = bEvaluateMoveStability && ActorYawRate >= CVarEnemyMoveSmoothWarnYawRate.GetValueOnGameThread();
	const bool bLikelyUnstable = bTargetJump || bYawJump || bYawRateHigh;

	UE_LOG(LogEnemyMoveSmooth, Log,
		TEXT("[EnemyMoveSmooth:%s] Enemy=%s Target=%s Move=MoveToCombatSlot Path=%s Dist=%.1f AttackRange=%.1f InRange=%d MoveAttack{Has=%d,ValidAbility=%d,Ready=%d,Range=%.1f,Cooldown=%.2f,AttackCooldown=%.2f} Speed=%.1f MoveTargetDelta=%.1f DesiredYawDelta=%.1f AppliedYawDelta=%.1f ActorToMoveYaw=%.1f ActorYawRate=%.1f Lead=%.1f SmoothAlpha=%.2f MoveReq=%d LastMoveResult=%s LastReqInterval=%.3f LastReqTargetMoved=%d LastReqTargetDelta=%.1f Acceptance=%.1f BBAcc=%.1f Tuning{Acceptance=%.1f,Lead=%.1f,YawSpeed=%.1f,Smooth=%.1f,Repath=%.2f} MoveTarget=%s"),
		bLikelyUnstable ? TEXT("Unstable") : TEXT("OK"),
		*GetNameSafe(ControlledPawn),
		*GetNameSafe(&TargetActor),
		PathFollowingStatusToString(PathStatus),
		DistanceToTarget,
		EffectiveAttackRange,
		bInAttackRange ? 1 : 0,
		bHasMovementAttack ? 1 : 0,
		bMovementAttackAbilityValid ? 1 : 0,
		bMovementAttackReady ? 1 : 0,
		MovementAttackRange,
		MovementAttackCooldownRemaining,
		AttackCooldownRemaining,
		ActualSpeed,
		MoveTargetDelta,
		LastCombatMoveDesiredYawDelta,
		LastCombatMoveAppliedYawDelta,
		ActorToMoveTargetYawDelta,
		ActorYawRate,
		LastCombatMoveLeadDistance,
		LastCombatMoveSmoothAlpha,
		CombatMoveRequestsSinceLastDebugLog,
		MoveRequestResultToString(LastCombatMoveRequestResultCode),
		LastCombatMoveRequestInterval,
		bLastCombatMoveRequestTargetMoved ? 1 : 0,
		LastCombatMoveRequestTargetDelta,
		LastCombatMoveRequestAcceptanceRadius,
		CurrentCombatMoveAcceptanceRadius,
		Tuning.AcceptanceRadius,
		Tuning.ForwardTurnLeadDistance,
		Tuning.MaxTurnYawSpeed,
		Tuning.MoveTargetSmoothingSpeed,
		Tuning.RepathInterval,
		*MoveTargetLocation.ToCompactString());

	LastCombatMoveDebugPawnLocation = PawnLocation;
	LastCombatMoveDebugMoveTarget = MoveTargetLocation;
	LastCombatMoveDebugTime = CurrentTime;
	LastCombatMoveDebugActorYaw = ActorYaw;
	CombatMoveRequestsSinceLastDebugLog = 0;
}

bool AYogAIController::UpdateAwarenessBlackboard(
	UBlackboardComponent* InBlackboard,
	FName EnemyAIStateKeyName,
	FName TargetActorKeyName,
	FName LastKnownTargetLocationKeyName,
	FName AlertExpireTimeKeyName,
	FName LastSeenTargetTimeKeyName)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !InBlackboard)
	{
		return false;
	}

	if (EnemyAIStateKeyName.IsNone())
	{
		EnemyAIStateKeyName = TEXT("EnemyAIState");
	}
	if (TargetActorKeyName.IsNone())
	{
		TargetActorKeyName = TEXT("TargetActor");
	}
	if (LastKnownTargetLocationKeyName.IsNone())
	{
		LastKnownTargetLocationKeyName = TEXT("LastKnownTargetLocation");
	}
	if (AlertExpireTimeKeyName.IsNone())
	{
		AlertExpireTimeKeyName = TEXT("AlertExpireTime");
	}
	if (LastSeenTargetTimeKeyName.IsNone())
	{
		LastSeenTargetTimeKeyName = TEXT("LastSeenTargetTime");
	}

	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIAwarenessTuning Tuning = EnemyData ? EnemyData->AwarenessTuning : FEnemyAIAwarenessTuning();
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const EEnemyAIState CurrentState = static_cast<EEnemyAIState>(InBlackboard->GetValueAsEnum(EnemyAIStateKeyName));
	const float CombatExitRadius = Tuning.CombatExitRadius > 0.0f
		? Tuning.CombatExitRadius
		: Tuning.DetectionRadius + 300.0f;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		const float DistanceToPlayer = FVector::Dist2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
		const bool bCanSeePlayer = DistanceToPlayer <= Tuning.DetectionRadius && LineOfSightTo(PlayerPawn);
		if (CVarEnemyAIStateLog.GetValueOnGameThread() >= 2)
		{
			const float LogInterval = FMath::Max(CVarEnemyAIStateLogInterval.GetValueOnGameThread(), 0.05f);
			if (LastAwarenessDebugLogTime <= -FLT_MAX * 0.5f || CurrentTime - LastAwarenessDebugLogTime >= LogInterval)
			{
				UE_LOG(LogEnemyAIState, Log,
					TEXT("[EnemyAIState:Awareness] Enemy=%s State=%s Player=%s Dist=%.1f CanSee=%d Detection=%.1f CombatEnter=%.1f LastSeenAge=%.2f Target=%s"),
					*GetNameSafe(ControlledPawn),
					EnemyAIStateToString(static_cast<EEnemyAIState>(InBlackboard->GetValueAsEnum(EnemyAIStateKeyName))),
					*GetNameSafe(PlayerPawn),
					DistanceToPlayer,
					bCanSeePlayer ? 1 : 0,
					Tuning.DetectionRadius,
					Tuning.CombatEnterRadius,
					CurrentTime - InBlackboard->GetValueAsFloat(LastSeenTargetTimeKeyName),
					*GetNameSafe(Cast<AActor>(InBlackboard->GetValueAsObject(TargetActorKeyName))));
				LastAwarenessDebugLogTime = CurrentTime;
			}
		}
		if (bCanSeePlayer)
		{
			InBlackboard->SetValueAsObject(TargetActorKeyName, PlayerPawn);
			InBlackboard->SetValueAsVector(LastKnownTargetLocationKeyName, PlayerPawn->GetActorLocation());
			InBlackboard->SetValueAsFloat(LastSeenTargetTimeKeyName, CurrentTime);

			if (CurrentState == EEnemyAIState::Combat)
			{
				if (DistanceToPlayer <= CombatExitRadius || bCombatAttackInProgress)
				{
					EnterCombat(PlayerPawn, false);
				}
				else
				{
					EnterAlert(PlayerPawn, PlayerPawn->GetActorLocation(), true);
				}
			}
			else if (DistanceToPlayer <= Tuning.CombatEnterRadius)
			{
				EnterCombat(PlayerPawn, true);
			}
			else
			{
				EnterAlert(PlayerPawn, PlayerPawn->GetActorLocation(), true);
			}

			return true;
		}
	}

	if (CurrentState == EEnemyAIState::Combat)
	{
		const float LastSeenTargetTime = InBlackboard->GetValueAsFloat(LastSeenTargetTimeKeyName);
		AActor* TargetActor = Cast<AActor>(InBlackboard->GetValueAsObject(TargetActorKeyName));
		const float DistanceToTarget = TargetActor
			? FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation())
			: TNumericLimits<float>::Max();
		const bool bPastExitRadius = DistanceToTarget > CombatExitRadius;
		const float StickyLoseDelay = Tuning.LoseTargetDelay + Tuning.AlertDuration;
		const float LastSeenAge = CurrentTime - LastSeenTargetTime;
		if (!bCombatAttackInProgress
			&& LastSeenAge >= Tuning.LoseTargetDelay
			&& (bPastExitRadius || LastSeenAge >= StickyLoseDelay))
		{
			const FVector LastKnownTargetLocation = InBlackboard->GetValueAsVector(LastKnownTargetLocationKeyName);
			EnterAlert(TargetActor, LastKnownTargetLocation, false);
		}
	}
	else if (CurrentState == EEnemyAIState::Alert)
	{
		const float AlertExpireTime = InBlackboard->GetValueAsFloat(AlertExpireTimeKeyName);
		if (AlertExpireTime > 0.0f && CurrentTime >= AlertExpireTime)
		{
			InBlackboard->SetValueAsEnum(EnemyAIStateKeyName, static_cast<uint8>(EEnemyAIState::Patrol));
			InBlackboard->ClearValue(TargetActorKeyName);
			InBlackboard->SetValueAsFloat(AlertExpireTimeKeyName, 0.0f);
			if (CVarEnemyAIStateLog.GetValueOnGameThread() > 0)
			{
				UE_LOG(LogEnemyAIState, Log,
					TEXT("[EnemyAIState] Enemy=%s Alert -> Patrol Reason=AlertExpired AlertExpireTime=%.2f CurrentTime=%.2f"),
					*GetNameSafe(ControlledPawn),
					AlertExpireTime,
					CurrentTime);
			}
		}
	}

	return true;
}

bool AYogAIController::UpdateCombatMoveBlackboard(
	UBlackboardComponent* InBlackboard,
	FName TargetActorKeyName,
	FName MoveTargetLocationKeyName,
	FName DistanceToTargetKeyName,
	FName bInAttackRangeKeyName,
	FName AcceptanceRadiusKeyName)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !InBlackboard)
	{
		return false;
	}

	if (TargetActorKeyName.IsNone())
	{
		TargetActorKeyName = TEXT("TargetActor");
	}
	if (MoveTargetLocationKeyName.IsNone())
	{
		MoveTargetLocationKeyName = TEXT("MoveTargetLocation");
	}
	if (DistanceToTargetKeyName.IsNone())
	{
		DistanceToTargetKeyName = TEXT("DistanceToTarget");
	}
	if (bInAttackRangeKeyName.IsNone())
	{
		bInAttackRangeKeyName = TEXT("bInAttackRange");
	}
	if (AcceptanceRadiusKeyName.IsNone())
	{
		AcceptanceRadiusKeyName = TEXT("AcceptanceRadius");
	}

	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();
	if (static_cast<EEnemyAIState>(InBlackboard->GetValueAsEnum(TEXT("EnemyAIState"))) != EEnemyAIState::Combat)
	{
		InBlackboard->SetValueAsBool(bInAttackRangeKeyName, false);
		InBlackboard->SetValueAsFloat(AcceptanceRadiusKeyName, Tuning.AcceptanceRadius);
		return true;
	}

	AActor* TargetActor = Cast<AActor>(InBlackboard->GetValueAsObject(TargetActorKeyName));
	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
		if (TargetActor)
		{
			InBlackboard->SetValueAsObject(TargetActorKeyName, TargetActor);
		}
	}
	if (!TargetActor)
	{
		return false;
	}

	const float DistanceToTarget = FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const float CombatMoveAcceptanceRadius = Tuning.bUseForwardSteering
		? FMath::Clamp(Tuning.AcceptanceRadius * 0.45f, 30.0f, 55.0f)
		: Tuning.AcceptanceRadius;

	if (bCombatAttackInProgress)
	{
		StopMovement();
		const FVector FrozenMoveTarget = ControlledPawn->GetActorLocation();
		const float FrozenAttackRange = FMath::Max(Tuning.AttackRange, DistanceToTarget);
		InBlackboard->SetValueAsVector(MoveTargetLocationKeyName, FrozenMoveTarget);
		InBlackboard->SetValueAsFloat(DistanceToTargetKeyName, DistanceToTarget);
		InBlackboard->SetValueAsBool(bInAttackRangeKeyName, true);
		InBlackboard->SetValueAsFloat(AcceptanceRadiusKeyName, CombatMoveAcceptanceRadius);
		LogCombatMoveSmoothSample(*TargetActor, FrozenMoveTarget, DistanceToTarget, FrozenAttackRange, true, Tuning);
		return true;
	}

	RefreshMovementAttackCooldownReset(DistanceToTarget);

	float EffectiveAttackRange = Tuning.AttackRange;
	if (EnemyData)
	{
		const UAbilityData* AbilityData = EnemyData->AbilityData;
		for (const FEnemyAIAttackOption& Attack : EnemyData->AttackProfile.Attacks)
		{
			if (Attack.Weight <= 0.0f || Attack.AbilityTags.IsEmpty() || Attack.MaxRange <= 0.0f)
			{
				continue;
			}

			if (!HasAnyValidAbilityTag(AbilityData, Attack))
			{
				if (Attack.AttackMovementMode != EEnemyAIAttackMovementMode::None
					&& CVarEnemyMovementAttackLog.GetValueOnGameThread() > 0)
				{
					const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
					const FName LogKey = GetMovementAttackCooldownKey(Attack);
					float& LastLogTime = InvalidMovementAttackAbilityLogTimes.FindOrAdd(LogKey, -FLT_MAX);
					if (CurrentTime - LastLogTime >= 1.0f)
					{
						LastLogTime = CurrentTime;
						UE_LOG(LogEnemyMovementAttack, Log,
							TEXT("[EnemyMovementAttack] Enemy=%s Attack=%s Event=RangeIgnored Reason=NoValidAbilityTag Tags=%s AbilityData=%s"),
							*GetNameSafe(ControlledPawn),
							*Attack.AttackName.ToString(),
							*Attack.AbilityTags.ToStringSimple(),
							*GetNameSafe(AbilityData));
					}
				}
				continue;
			}

			if (Attack.AttackRole == EEnemyAIAttackRole::SpecialMovement)
			{
				float RemainingAttackCooldown = 0.0f;
				if (!IsAttackCooldownReadyForAI(Attack, &RemainingAttackCooldown))
				{
					if (CVarEnemyMovementAttackLog.GetValueOnGameThread() > 0)
					{
						UE_LOG(LogEnemyMovementAttack, Verbose,
							TEXT("[EnemyMovementAttack] Enemy=%s Attack=%s Event=RangeBlockedByAttackCooldown Dist=%.1f Remaining=%.2f MoveRange=%.1f CloseRange=%.1f"),
							*GetNameSafe(ControlledPawn),
							*Attack.AttackName.ToString(),
							DistanceToTarget,
							RemainingAttackCooldown,
							GetMovementAttackRange(Attack),
							Tuning.AttackRange);
					}
					continue;
				}

				float RemainingCooldown = 0.0f;
				const float SpecialStartRange = FMath::Max(Attack.LungeStartRange, Tuning.AttackRange);
				if (DistanceToTarget >= SpecialStartRange && CanUseMovementAttack(Attack, DistanceToTarget, &RemainingCooldown))
				{
					EffectiveAttackRange = FMath::Max(EffectiveAttackRange, GetMovementAttackRange(Attack));
				}
				else if (CVarEnemyMovementAttackLog.GetValueOnGameThread() > 0 && RemainingCooldown > 0.0f)
				{
					UE_LOG(LogEnemyMovementAttack, Verbose,
						TEXT("[EnemyMovementAttack] Enemy=%s Attack=%s Event=RangeBlockedByCooldown Dist=%.1f Remaining=%.2f MoveRange=%.1f CloseRange=%.1f"),
						*GetNameSafe(ControlledPawn),
						*Attack.AttackName.ToString(),
						DistanceToTarget,
						RemainingCooldown,
						GetMovementAttackRange(Attack),
						Tuning.AttackRange);
				}
				continue;
			}

			if (Attack.AttackRole == EEnemyAIAttackRole::Skill)
			{
				if (IsAttackCooldownReadyForAI(Attack) && Attack.MaxRange > 0.0f)
				{
					EffectiveAttackRange = FMath::Max(EffectiveAttackRange, Attack.MaxRange);
				}
				continue;
			}

			if (Attack.AttackRole == EEnemyAIAttackRole::CloseMelee)
			{
				EffectiveAttackRange = FMath::Max(EffectiveAttackRange, Attack.MaxRange);
			}
		}
	}

	const bool bWasInAttackRange = InBlackboard->GetValueAsBool(bInAttackRangeKeyName);
	const float ExitBuffer = bWasInAttackRange ? FMath::Max(Tuning.AttackRangeExitBuffer, 0.0f) : 0.0f;
	const bool bInAttackRange = DistanceToTarget <= EffectiveAttackRange + ExitBuffer;
	const FVector MoveTargetLocation = bInAttackRange
		? ControlledPawn->GetActorLocation()
		: ComputeCombatMoveTarget(*TargetActor, Tuning, EffectiveAttackRange);
	if (bInAttackRange)
	{
		StopMovement();
	}

	InBlackboard->SetValueAsVector(MoveTargetLocationKeyName, MoveTargetLocation);
	InBlackboard->SetValueAsFloat(DistanceToTargetKeyName, DistanceToTarget);
	InBlackboard->SetValueAsBool(bInAttackRangeKeyName, bInAttackRange);
	InBlackboard->SetValueAsFloat(AcceptanceRadiusKeyName, CombatMoveAcceptanceRadius);
	LogCombatMoveSmoothSample(*TargetActor, MoveTargetLocation, DistanceToTarget, EffectiveAttackRange, bInAttackRange, Tuning);

	return true;
}
