#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "Data/EnemyData.h"
#include "YogStateTreeEvaluators.generated.h"

class AAIController;
class AActor;

// ─── Enemy Awareness ────────────────────────────────────────────────────────
// Mirrors UBTService_UpdateEnemyAwareness. Each tick it drives
// AYogAIController::UpdateAwarenessBlackboard (perception, alert timers, state
// transitions) using the controller's own BlackboardComponent, then re-exposes
// the resulting state/target as StateTree output properties so conditions and
// transitions can bind to them.

USTRUCT()
struct FStateTreeEvaluator_EnemyAwarenessInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	// Blackboard key names — must match the keys on the controller's blackboard.
	UPROPERTY(EditAnywhere, Category = Parameter)
	FName EnemyAIStateKey = TEXT("EnemyAIState");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName LastKnownTargetLocationKey = TEXT("LastKnownTargetLocation");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName AlertExpireTimeKey = TEXT("AlertExpireTime");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName LastSeenTargetTimeKey = TEXT("LastSeenTargetTime");

	// Outputs (bind conditions / transitions / tasks to these).
	UPROPERTY(EditAnywhere, Category = Output)
	EEnemyAIState EnemyAIState = EEnemyAIState::Patrol;

	UPROPERTY(EditAnywhere, Category = Output)
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Output)
	FVector LastKnownTargetLocation = FVector::ZeroVector;
};

USTRUCT(meta = (DisplayName = "Update Enemy Awareness", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeEvaluator_EnemyAwareness : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_EnemyAwarenessInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void Refresh(FStateTreeExecutionContext& Context) const;
};

// ─── Enemy Combat Move ──────────────────────────────────────────────────────
// Mirrors UBTService_UpdateEnemyCombatMove. Drives
// AYogAIController::UpdateCombatMoveBlackboard and re-exposes the move target,
// distance, in-range flag and acceptance radius as outputs. Bind the built-in
// "Move To" task's Destination to MoveTargetLocation.

USTRUCT()
struct FStateTreeEvaluator_EnemyCombatMoveInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName MoveTargetLocationKey = TEXT("MoveTargetLocation");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName DistanceToTargetKey = TEXT("DistanceToTarget");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName bInAttackRangeKey = TEXT("bInAttackRange");

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName AcceptanceRadiusKey = TEXT("AcceptanceRadius");

	// Outputs.
	UPROPERTY(EditAnywhere, Category = Output)
	FVector MoveTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Output)
	float DistanceToTarget = 0.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bInAttackRange = false;

	UPROPERTY(EditAnywhere, Category = Output)
	float AcceptanceRadius = 0.f;
};

USTRUCT(meta = (DisplayName = "Update Enemy Combat Move", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeEvaluator_EnemyCombatMove : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEvaluator_EnemyCombatMoveInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
