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

	void ApplyCrowdTuningFromEnemyData();

	UEnemyData* GetPossessedEnemyData() const;

	UFUNCTION(BlueprintCallable, Category = "AI|State")
	void SetEnemyAIState(EEnemyAIState NewState);

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
	FVector ComputeCombatMoveTarget(const AActor& TargetActor, const FEnemyAIMovementTuning& Tuning) const;

	UBlackboardComponent* ResolveBlackboardComponent() const;
};
