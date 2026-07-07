#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "GameplayTagContainer.h"
#include "YogStateTreeTasks.generated.h"

class AAIController;
class UAbilitySystemComponent;
class AYogCharacterBase;

// ─── Activate Ability By Tag ────────────────────────────────────────────────
// Mirrors UBTTask_ActivateAbilityByTag. Filters the requested tags down to the
// abilities the pawn's AbilityData actually owns, activates a random matching
// GA, and stays Running until that GA ends (or completes immediately when the
// GA has no montage). Optionally drives the pre-attack flash for its duration.

USTRUCT()
struct FStateTreeTask_ActivateAbilityByTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FGameplayTagContainer AbilityTags;

	/** Trigger the pre-attack red flash while the ability runs. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bPreAttackFlash = true;

	// Runtime state (not reflected; persists for the active state's lifetime).
	TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;
	FDelegateHandle EndHandle;
	TWeakObjectPtr<AYogCharacterBase> FlashCharacter;
};

USTRUCT(meta = (DisplayName = "Activate Ability By Tag", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_ActivateAbilityByTag : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_ActivateAbilityByTagInstanceData;

	FStateTreeTask_ActivateAbilityByTag();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Play Dead ──────────────────────────────────────────────────────────────
// Terminal state task. Stops movement and stays Running so the pawn remains in
// the Dead state (the death montage / GA is driven elsewhere by GA_Dead).

USTRUCT()
struct FStateTreeTask_PlayDeadInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "Play Dead", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_PlayDead : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_PlayDeadInstanceData;

	FStateTreeTask_PlayDead();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Update Enemy Patrol Target ─────────────────────────────────────────────
// Mirrors UBTTask_UpdateEnemyPatrolTarget. Picks a random reachable point
// within EnemyData.AwarenessTuning.PatrolRadius of the patrol origin and writes
// it to the PatrolTargetLocation output (bind the Move To task to it).

USTRUCT()
struct FStateTreeTask_UpdateEnemyPatrolTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Output)
	FVector PatrolTargetLocation = FVector::ZeroVector;

	// Latched patrol origin (first evaluation captures the spawn location).
	UPROPERTY()
	FVector PatrolOrigin = FVector::ZeroVector;

	UPROPERTY()
	bool bHasOrigin = false;
};

USTRUCT(meta = (DisplayName = "Update Enemy Patrol Target", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_UpdateEnemyPatrolTarget : public FStateTreeAITaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_UpdateEnemyPatrolTargetInstanceData;

	FStateTreeTask_UpdateEnemyPatrolTarget();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Enemy Patrol Wait ──────────────────────────────────────────────────────
// Mirrors UBTTask_EnemyPatrolWait. Waits a random duration within
// EnemyData.AwarenessTuning PatrolWaitMin/PatrolWaitMax, then succeeds.

USTRUCT()
struct FStateTreeTask_EnemyPatrolWaitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	float RemainingTime = 0.f;
};

USTRUCT(meta = (DisplayName = "Enemy Patrol Wait", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_EnemyPatrolWait : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_EnemyPatrolWaitInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
