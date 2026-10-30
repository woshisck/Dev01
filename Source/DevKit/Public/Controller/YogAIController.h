// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Data/EnemyData.h"
#include "YogAIController.generated.h"

/**
 * 
 */

UCLASS()
class DEVKIT_API AYogAIController : public AModularAIController
{
	GENERATED_BODY()
	
public:
	AYogAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;


	// The component that holds the Blackboard data
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBlackboardComponent* BlackboardComponent;

	UPROPERTY(BlueprintReadWrite, Category = "AI")
	TObjectPtr<class UStateTreeAIComponent> StateTreeComponent;

	// Used when the possessed EnemyData supplies no StateTreeBlackboard of its own.
	// Assign in the subclass BP's Class Defaults (e.g. BP_AIControllerBase).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Fallback")
	TObjectPtr<class UBlackboardData> FallbackBlackboard;

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool RunStateTreeWithBlackboard(class UStateTree* StateTree, class UBlackboardData* BB);

	bool UpdateCombatMoveBlackboard(
		UBlackboardComponent* InBlackboard,
		FName TargetActorKeyName,
		FName MoveTargetLocationKeyName,
		FName DistanceToTargetKeyName,
		FName bInAttackRangeKeyName,
		FName AcceptanceRadiusKeyName);

	void RecordCombatMoveRequestForDebug(
		const FVector& MoveTarget,
		int32 MoveResultCode,
		bool bTargetMoved,
		float RepathInterval,
		float AcceptanceRadius);

	// Attack cooldowns are owned here rather than by the StateTree task: task instance data is
	// reallocated on every state entry, so a deadline stored there would never survive.
	void NotifyAttackActivated(const FGameplayTag& AbilityTag, float Cooldown);
	bool IsAttackCooldownReadyForAI(const FGameplayTag& AbilityTag, float* OutRemainingCooldown = nullptr) const;

	// Records the outcome of the most recently completed enemy profile attack on the
	// blackboard: bLastAttackWhiffed reflects whether it missed, and LastWhiffTime is
	// stamped on a whiff so the StateTree can gate a reposition response. Connecting clears the flag.
	void NotifyAttackResolved(bool bWhiffed, bool bForceReposition = false);
	void ResetCombatMoveSmoothingAfterAttack();
	void SetCombatAttackInProgress(bool bInProgress);
	bool IsCombatAttackInProgress() const { return bCombatAttackInProgress; }

	void ApplyCrowdTuningFromEnemyData();

	UEnemyData* GetPossessedEnemyData() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void SetEnemyAIState(EEnemyAIState NewState);

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void InitializePatrolState();

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	EEnemyAIState GetEnemyAIState() const;

	// World-time (seconds) when this controller last transitioned into Combat from a
	// non-Combat state. Returns -FLT_MAX while never-in-combat. Drives the StateTree
	// time-in-combat phase gate; re-aggro while already fighting does not reset it.
	float GetCombatStartTime() const { return CombatStartTime; }

	// World-time (seconds) of the last non-Alert -> Alert transition. Re-sighting the player
	// while already alarmed does not reset it, so it measures how long the notice has run.
	float GetAlertStartTime() const { return AlertStartTime; }

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void EnterCombat(AActor* TargetActor, bool bBroadcastAlert = true);

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void EnterAlert(AActor* TargetActor, FVector AlertLocation, bool bBroadcastAlert = false);

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void BroadcastAlert(AActor* TargetActor, FVector AlertLocation) const;

	// Forwards a gameplay-tag event to the running StateTree. No-op while the tree is not running,
	// since the engine drops events sent before StartLogic.
	void SendStateTreeEvent(FGameplayTag EventTag);

	// Conversation hold: pause/resume the StateTree brain so no new
	// attack or move decisions fire, and stop in-flight pathing. Animations and VFX
	// keep playing because the mesh ticks independently of the AI brain. Driven by
	// the player controller's conversation-mode wrapper.
	void SetConversationHold(bool bHold);

	bool UpdateAwarenessBlackboard(
		UBlackboardComponent* InBlackboard,
		FName EnemyAIStateKeyName,
		FName TargetActorKeyName,
		FName LastKnownTargetLocationKeyName,
		FName AlertExpireTimeKeyName,
		FName LastSeenTargetTimeKeyName);

private:
	FVector ComputeCombatMoveTarget(const AActor& TargetActor, const FEnemyAIMovementTuning& Tuning, float EffectiveAttackRange);
	FVector ApplyForwardSteeringToMoveTarget(const FVector& DesiredLocation, const FEnemyAIMovementTuning& Tuning);
	void ResetCombatMoveSmoothing(bool bResetCooldowns = true);
	void LogCombatMoveSmoothSample(
		const AActor& TargetActor,
		const FVector& MoveTargetLocation,
		float DistanceToTarget,
		float EffectiveAttackRange,
		bool bInAttackRange,
		const FEnemyAIMovementTuning& Tuning);

	UBlackboardComponent* ResolveBlackboardComponent() const;

	FVector SmoothedCombatMoveDirection = FVector::ZeroVector;
	FVector SmoothedCombatMoveTarget = FVector::ZeroVector;
	bool bHasSmoothedCombatMove = false;
	FVector LockedCombatSlotLocation = FVector::ZeroVector;
	float LockedCombatSlotExpireTime = -FLT_MAX;
	bool bHasLockedCombatSlot = false;

	FVector LastCombatMoveDebugPawnLocation = FVector::ZeroVector;
	FVector LastCombatMoveDebugMoveTarget = FVector::ZeroVector;
	float LastCombatMoveDebugTime = -FLT_MAX;
	float LastCombatMoveDebugActorYaw = 0.0f;
	float LastCombatMoveDesiredYawDelta = 0.0f;
	float LastCombatMoveAppliedYawDelta = 0.0f;
	float LastCombatMoveLeadDistance = 0.0f;
	float LastCombatMoveSmoothAlpha = 1.0f;
	int32 CombatMoveRequestsSinceLastDebugLog = 0;
	FVector LastCombatMoveRequestTarget = FVector::ZeroVector;
	float LastCombatMoveRequestTime = -FLT_MAX;
	float LastCombatMoveRequestInterval = 0.0f;
	float LastCombatMoveRequestTargetDelta = 0.0f;
	float LastCombatMoveRequestAcceptanceRadius = 0.0f;
	int32 LastCombatMoveRequestResultCode = 0;
	bool bLastCombatMoveRequestTargetMoved = false;
	float LastAwarenessDebugLogTime = -FLT_MAX;
	TMap<FGameplayTag, float> AttackCooldownEndTimes;
	bool bCombatAttackInProgress = false;
	float CombatStartTime = -FLT_MAX;
	float AlertStartTime = -FLT_MAX;
};
