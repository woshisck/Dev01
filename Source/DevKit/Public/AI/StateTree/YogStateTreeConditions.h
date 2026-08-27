#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "Data/EnemyData.h"
#include "YogStateTreeConditions.generated.h"

class AAIController;
class AActor;

// ─── Enemy AI State ─────────────────────────────────────────────────────────
// Passes when the controller's current
// EEnemyAIState matches RequiredState. Use as an Enter Condition on the
// Combat / Patrol states.

USTRUCT()
struct FStateTreeCondition_EnemyAIStateInstanceData
{
	GENERATED_BODY()

	/** Bound to the schema's AIController context. */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	EEnemyAIState RequiredState = EEnemyAIState::Combat;
};

USTRUCT(meta = (DisplayName = "Enemy AI State Is", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_EnemyAIState : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_EnemyAIStateInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Is Dead ────────────────────────────────────────────────────────────────
// Mirrors the IsDead blackboard decorator: passes when the controlled pawn is
// flagged dead. Use as the Enter Condition on the terminal Dead state.

USTRUCT()
struct FStateTreeCondition_IsDeadInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(meta = (DisplayName = "Enemy Is Dead", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_IsDead : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_IsDeadInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Self Health % Below ────────────────────────────────────────────────────
// Passes when the controlled pawn's Health/MaxHealth <= Threshold. Use as the
// Enter Condition on lower-HP boss phases (order high-threshold phases last so
// the selector enters the deepest matching phase first).

USTRUCT()
struct FStateTreeCondition_SelfHealthPercentBelowInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** Fraction of max HP (0-1). Condition passes at or below this. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Threshold = 0.5f;
};

USTRUCT(meta = (DisplayName = "Self Health % Below", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_SelfHealthPercentBelow : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_SelfHealthPercentBelowInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Time In Combat At Least ────────────────────────────────────────────────
// Passes once the controller has been in Combat for at least Seconds (measured
// from AYogAIController::GetCombatStartTime). Pair with an OR next to the HP gate
// to enter a phase on "took too long" as well as "took enough damage".

USTRUCT()
struct FStateTreeCondition_TimeInCombatAtLeastInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float Seconds = 30.0f;
};

USTRUCT(meta = (DisplayName = "Time In Combat At Least", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_TimeInCombatAtLeast : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_TimeInCombatAtLeastInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Player Health % Below ──────────────────────────────────────────────────
// Passes when the current target's (player's) Health/MaxHealth <= Threshold.
// Resolves the target from the controller's TargetActor blackboard key, falling
// back to player pawn 0. Used to branch the dying reaction on the player's state.

USTRUCT()
struct FStateTreeCondition_PlayerHealthPercentBelowInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Threshold = 0.3f;
};

USTRUCT(meta = (DisplayName = "Player Health % Below", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_PlayerHealthPercentBelow : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_PlayerHealthPercentBelowInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Enemy Post Attack Reposition ───────────────────────────────────────────
// Passes while the controller has
// flagged a reposition request on its blackboard. AYogAIController::NotifyAttackResolved
// sets the flag on a whiff and clears it on a connect. Use as the Enter Condition on
// the Reposition attack state so it is only attempted after a miss.

USTRUCT()
struct FStateTreeCondition_EnemyPostAttackRepositionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName bPostAttackRepositionKey = TEXT("bPostAttackReposition");
};

USTRUCT(meta = (DisplayName = "Enemy Post Attack Reposition", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_EnemyPostAttackReposition : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_EnemyPostAttackRepositionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ─── Target Within 2D Distance ──────────────────────────────────────────────
// Passes when Target is within Distance of the tree owner, measured on XY only
// so height difference is ignored. Put it on an OnTick transition to leave a
// state the moment the player closes in. Derives from the common condition base
// so it also works under StateTreeComponentSchema.

USTRUCT()
struct FStateTreeCondition_TargetWithin2DDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> Actor = nullptr;

	/** Bind to an evaluator output, e.g. Player Reference -> PlayerPawn. */
	UPROPERTY(EditAnywhere, Category = Input, meta = (Optional))
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float Distance = 100.0f;
};

USTRUCT(meta = (DisplayName = "Target Within 2D Distance", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_TargetWithin2DDistance : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_TargetWithin2DDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT()
struct FStateTreeCondition_TargetBeyond2DDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = Input, meta = (Optional))
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float Distance = 100.0f;
};

USTRUCT(meta = (DisplayName = "Target Beyond 2D Distance", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeCondition_TargetBeyond2DDistance : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCondition_TargetBeyond2DDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
