// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/YogAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "GameModes/YogGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"


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
    // 1) 先绑黑板（绕开 BT 内部 BlackboardAsset 断链）
    if (BB && BlackboardComponent)
    {
        const bool bUsed = UseBlackboard(BB, BlackboardComponent);
        UE_LOG(LogTemp, Warning, TEXT("[YogAIController] UseBlackboard(%s) -> %d"),
            *BB->GetName(), bUsed ? 1 : 0);
    }
    // 2) 启动 BT（RunBehaviorTree 内部如果发现 BB 已绑会复用，不会覆盖）
    const bool bRan = RunBehaviorTree(BT);
    UE_LOG(LogTemp, Warning, TEXT("[YogAIController] RunBehaviorTree(%s) -> %d"),
        *BT->GetName(), bRan ? 1 : 0);
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
			SetEnemyAIState(EEnemyAIState::Patrol);
			return;
		}
	}

	// 兜底启动：BT 内部 BlackboardAsset 断链时由 C++ 强制 UseBlackboard + RunBehaviorTree
	if (bUseFallbackStartup && FallbackBehaviorTree)
	{
		RunBTWithBlackboard(FallbackBehaviorTree, FallbackBlackboard);
		SetEnemyAIState(EEnemyAIState::Patrol);
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
		BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(NewState));
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

	BB->SetValueAsEnum(TEXT("EnemyAIState"), static_cast<uint8>(EEnemyAIState::Combat));
	BB->SetValueAsObject(TEXT("TargetActor"), TargetActor);
	BB->SetValueAsVector(TEXT("LastKnownTargetLocation"), TargetLocation);
	BB->SetValueAsFloat(TEXT("LastSeenTargetTime"), CurrentTime);
	BB->SetValueAsFloat(TEXT("AlertExpireTime"), 0.0f);

	if (bBroadcastAlert)
	{
		BroadcastAlert(TargetActor, TargetLocation);
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
		if (UCharacterMovementComponent* Movement = ControlledCharacter->GetCharacterMovement())
		{
			// Detour Crowd drives local avoidance for AI; do not run RVO avoidance on top of it.
			Movement->bUseRVOAvoidance = false;
		}
	}
}

FVector AYogAIController::ComputeCombatMoveTarget(const AActor& TargetActor, const FEnemyAIMovementTuning& Tuning) const
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
	const float PreferredRange = FMath::Max(Tuning.PreferredRange, Tuning.AttackRange);

	if (DistanceToTarget <= Tuning.AttackRange)
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
		DesiredLocation = TargetLocation + ApproachDirection.GetSafeNormal2D() * Tuning.AttackRange;
		break;
	}
	}

	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(DesiredLocation, NavLocation, FVector(220.f, 220.f, 300.f)))
			{
				return NavLocation.Location;
			}
		}
	}

	return DesiredLocation;
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

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (PlayerPawn)
	{
		const float DistanceToPlayer = FVector::Dist2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
		const bool bCanSeePlayer = DistanceToPlayer <= Tuning.DetectionRadius && LineOfSightTo(PlayerPawn);
		if (bCanSeePlayer)
		{
			InBlackboard->SetValueAsObject(TargetActorKeyName, PlayerPawn);
			InBlackboard->SetValueAsVector(LastKnownTargetLocationKeyName, PlayerPawn->GetActorLocation());
			InBlackboard->SetValueAsFloat(LastSeenTargetTimeKeyName, CurrentTime);

			if (DistanceToPlayer <= Tuning.CombatEnterRadius)
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

	const EEnemyAIState CurrentState = static_cast<EEnemyAIState>(InBlackboard->GetValueAsEnum(EnemyAIStateKeyName));
	if (CurrentState == EEnemyAIState::Combat)
	{
		const float LastSeenTargetTime = InBlackboard->GetValueAsFloat(LastSeenTargetTimeKeyName);
		if (CurrentTime - LastSeenTargetTime >= Tuning.LoseTargetDelay)
		{
			AActor* TargetActor = Cast<AActor>(InBlackboard->GetValueAsObject(TargetActorKeyName));
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

	const UEnemyData* EnemyData = GetPossessedEnemyData();
	const FEnemyAIMovementTuning Tuning = EnemyData ? EnemyData->MovementTuning : FEnemyAIMovementTuning();
	const float DistanceToTarget = FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const bool bInAttackRange = DistanceToTarget <= Tuning.AttackRange;
	const FVector MoveTargetLocation = ComputeCombatMoveTarget(*TargetActor, Tuning);

	InBlackboard->SetValueAsVector(MoveTargetLocationKeyName, MoveTargetLocation);
	InBlackboard->SetValueAsFloat(DistanceToTargetKeyName, DistanceToTarget);
	InBlackboard->SetValueAsBool(bInAttackRangeKeyName, bInAttackRange);
	InBlackboard->SetValueAsFloat(AcceptanceRadiusKeyName, Tuning.AcceptanceRadius);

	return true;
}
