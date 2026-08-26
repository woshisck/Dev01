#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "YogStateTreeTasks.generated.h"

class AAIController;
class UAbilitySystemComponent;
class AActor;
class AEnemyCharacterBase;
class AYogCharacterBase;
class UGameplayEffect;
class USkeletalMesh;
class UMaterialInterface;
class UStoryEncounterPointDA;

// ─── Activate Ability By Tag ────────────────────────────────────────────────
// Filters the requested tags down to the
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
// Picks a random reachable point
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

USTRUCT()
struct FStateTreeTask_SpawnMobInReachableNavMeshInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> SpawnOriginActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	TSubclassOf<AEnemyCharacterBase> EnemyClass;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float SpawnRadius = 1000.0f;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float MinSpawnDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float SpawnZOffset = 96.0f;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "1"))
	int32 MaxAttempts = 24;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bCountsForLevelClear = true;

	UPROPERTY(EditAnywhere, Category = Output)
	TObjectPtr<AEnemyCharacterBase> SpawnedEnemy = nullptr;
};

USTRUCT(meta = (DisplayName = "Spawn Mob In Reachable NavMesh", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_SpawnMobInReachableNavMesh : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_SpawnMobInReachableNavMeshInstanceData;

	FStateTreeTask_SpawnMobInReachableNavMesh();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Enemy Patrol Wait ──────────────────────────────────────────────────────
// Waits a random duration within
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

// ─── Enter Boss Phase ───────────────────────────────────────────────────────
// One-shot state-enter task for a boss phase: applies a phase GameplayEffect
// (attribute changes) to self, and optionally swaps the look (mesh / material
// override) and fires a gameplay cue for a VFX burst. Succeeds immediately so a
// sibling attack task on the same state drives the actual combat. Set
// bRemoveEffectOnExit only for transient phases; phase changes are usually one-way.

USTRUCT()
struct FStateTreeTask_EnterBossPhaseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Attribute changes for this phase (e.g. +AttackPower, +MoveSpeed). */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TSubclassOf<UGameplayEffect> PhaseEffect;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float PhaseEffectLevel = 1.f;

	/** Optional: swap the pawn's skeletal mesh for the phase look change. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<USkeletalMesh> PhaseMesh = nullptr;

	/** Optional: override material slot 0 for the phase look change. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<UMaterialInterface> PhaseMaterialOverride = nullptr;

	/** Optional: gameplay cue executed on enter for a one-shot transformation VFX. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FGameplayTag PhaseVfxCueTag;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bRemoveEffectOnExit = false;

	// Runtime state (not reflected; persists for the active state's lifetime).
	FActiveGameplayEffectHandle AppliedEffectHandle;
	TWeakObjectPtr<UAbilitySystemComponent> AppliedASC;
};

USTRUCT(meta = (DisplayName = "Enter Boss Phase", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_EnterBossPhase : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_EnterBossPhaseInstanceData;

	FStateTreeTask_EnterBossPhase();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Boss Dying Reaction ────────────────────────────────────────────────────
// Fires an authored Story Encounter point (dialogue / info-hint) through the
// runtime subsystem, then succeeds immediately. Branch between different reactions
// at the graph level using the Player Health % Below condition on sibling states.

USTRUCT()
struct FStateTreeTask_BossDyingReactionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Encounter point whose actions (e.g. Dialogue) play when this state is entered. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<UStoryEncounterPointDA> EncounterPoint = nullptr;
};

USTRUCT(meta = (DisplayName = "Boss Dying Reaction", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_BossDyingReaction : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_BossDyingReactionInstanceData;

	FStateTreeTask_BossDyingReaction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Move To Controller Target ──────────────────────────────────────────────
// Issues a nav MoveTo toward a location read from the controller's blackboard
// (DestinationKey). Move-to for patrol / alert without needing
// StateTree property bindings: the destination is sourced from the controller,
// consistent with the other Yog StateTree tasks. Stays Running until the move
// request finishes, so it can run in parallel with the wait task on a state.

USTRUCT()
struct FStateTreeTask_MoveToControllerTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Blackboard vector key holding the destination (e.g. PatrolTargetLocation, LastKnownTargetLocation). */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FName DestinationKey = TEXT("MoveTargetLocation");

	UPROPERTY(EditAnywhere, Category = Parameter)
	float AcceptanceRadius = 50.f;

	// Runtime state (not reflected; persists for the active state's lifetime).
	FAIRequestID MoveRequestID;
	TWeakObjectPtr<AAIController> BoundController;
	FDelegateHandle RequestFinishedHandle;
};

USTRUCT(meta = (DisplayName = "Move To Controller Target", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_MoveToControllerTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_MoveToControllerTargetInstanceData;

	FStateTreeTask_MoveToControllerTarget();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ─── Enemy Combat Move ──────────────────────────────────────────────────────
// Chases the combat slot that the "Update Enemy
// Combat Move" evaluator writes to the blackboard, repathing as the slot drifts,
// and succeeds once the pawn is in attack range. Unlike Move To Controller Target
// this re-issues the move on a timer rather than waiting on a single path request,
// which is what lets an enemy track a moving player.

USTRUCT()
struct FStateTreeTask_EnemyCombatMoveInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	// Blackboard key names — must match the keys the combat move evaluator writes.
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

	/** Slot drift that forces an immediate repath instead of waiting for RepathInterval. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float TargetRefreshDistance = 80.0f;

	// Runtime state (not reflected; persists for the active state's lifetime).
	FVector LastMoveTarget = FVector::ZeroVector;
	float LastMoveRequestTime = -FLT_MAX;
	bool bHasMoveRequest = false;
};

USTRUCT(meta = (DisplayName = "Enemy Combat Move", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_EnemyCombatMove : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_EnemyCombatMoveInstanceData;

	FStateTreeTask_EnemyCombatMove();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	bool IssueMove(FInstanceDataType& InstanceData, bool bTargetMoved, float RepathInterval) const;
};

// ─── Debug Print ────────────────────────────────────────────────────────────
// Logs a message on state enter and succeeds immediately, so the owning state
// completes and its OnStateCompleted transition fires. Derives from the common
// task base rather than FStateTreeAIActionTaskBase so it stays selectable under
// StateTreeComponentSchema, which rejects FStateTreeAITaskBase structs.

USTRUCT()
struct FStateTreeTask_DebugPrintInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Parameter)
	FString Message = TEXT("Enter Idle state");

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bPrintToScreen = true;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float ScreenDuration = 2.0f;

	// Optional: bind to an evaluator output to see whether it actually resolved.
	// Appended to the message as "Actor=<name>", or "Actor=None" when unbound or null.
	UPROPERTY(EditAnywhere, Category = Input, meta = (Optional))
	TObjectPtr<AActor> ReferenceActor = nullptr;
};

USTRUCT(meta = (DisplayName = "Debug Print", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_DebugPrint : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_DebugPrintInstanceData;

	FStateTreeTask_DebugPrint();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
