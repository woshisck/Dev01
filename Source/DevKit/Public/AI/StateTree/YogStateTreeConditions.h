#pragma once

#include "CoreMinimal.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "Data/EnemyData.h"
#include "YogStateTreeConditions.generated.h"

class AAIController;

// ─── Enemy AI State ─────────────────────────────────────────────────────────
// Mirrors UBTDecorator_EnemyAIState: passes when the controller's current
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
