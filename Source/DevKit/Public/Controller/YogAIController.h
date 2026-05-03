// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
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


	// The component that will run the Behavior Tree
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComponent;

	// The component that holds the Blackboard data
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBlackboardComponent* BlackboardComponent;

	// ── BT/BB 兜底配置（用于 BT 内部 BlackboardAsset 引用断链时的 C++ 接管启动） ──
	// 在子类 BP（如 BP_AIControllerBase）的 Class Defaults 里赋值。OnPossess 优先用这两个启动；
	// 若任一为空则回退到原本的 BP 启动逻辑（不接管）。

	/** 默认行为树（BT 资产，作为 BP RunBehaviorTree 的 fallback） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Fallback")
	TObjectPtr<class UBehaviorTree> FallbackBehaviorTree;

	/** 默认黑板数据（BT 内部 BlackboardAsset 断链时手动绑） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Fallback")
	TObjectPtr<class UBlackboardData> FallbackBlackboard;

	/** 是否启用 fallback 启动（true = OnPossess 强制走 UseBlackboard + RunBehaviorTree） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Fallback")
	bool bUseFallbackStartup = false;

	/** 蓝图可调：手动启动 BT，使用指定 BB 兜底（不依赖 BT 内置 BlackboardAsset） */
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool RunBTWithBlackboard(class UBehaviorTree* BT, class UBlackboardData* BB);

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

	float GetMovementAttackRange(const FEnemyAIAttackOption& Attack) const;
	void NotifyAttackActivated(const FEnemyAIAttackOption& Attack);
	bool IsAttackCooldownReadyForAI(const FEnemyAIAttackOption& Attack, float* OutRemainingCooldown = nullptr) const;
	bool IsRecentAttackRepeat(const FEnemyAIAttackOption& Attack, float MemoryDuration, float* OutRepeatAge = nullptr) const;
	bool CanUseMovementAttack(const FEnemyAIAttackOption& Attack, float DistanceToTarget, float* OutRemainingCooldown = nullptr) const;
	void NotifyMovementAttackActivated(const FEnemyAIAttackOption& Attack);
	void RefreshMovementAttackCooldownReset(float DistanceToTarget);
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

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void EnterCombat(AActor* TargetActor, bool bBroadcastAlert = true);

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void EnterAlert(AActor* TargetActor, FVector AlertLocation, bool bBroadcastAlert = false);

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void BroadcastAlert(AActor* TargetActor, FVector AlertLocation) const;

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
	TMap<FName, float> MovementAttackCooldownEndTimes;
	TMap<FName, float> AttackCooldownEndTimes;
	TMap<FName, float> InvalidMovementAttackAbilityLogTimes;
	FName LastSelectedAttackKey = NAME_None;
	float LastSelectedAttackTime = -FLT_MAX;
	bool bCombatAttackInProgress = false;
};
