#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "GameplayTagContainer.h"
#include "Data/EnemyData.h"
#include "YogStateTreeTask_EnemyAttackByProfile.generated.h"

class AAIController;
class AActor;
class UAbilitySystemComponent;
class AYogCharacterBase;

// ─── Enemy Attack By Profile ────────────────────────────────────────────────
// Port of UBTTask_EnemyAttackByProfile. Weighted-random-selects an attack from
// EnemyData.AttackProfile matching RequiredAttackRole, respecting range / health
// / cooldown / movement-attack gates, activates the chosen GA, and stays Running
// until it ends. Reuses AYogAIController's cooldown / combat bookkeeping.
//
// Bind TargetActor / DistanceToTarget / bInAttackRange to the
// "Update Enemy Combat Move" evaluator outputs.

USTRUCT()
struct FStateTreeTask_EnemyAttackByProfileInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	EEnemyAIAttackRole RequiredAttackRole = EEnemyAIAttackRole::CloseMelee;

	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Input)
	float DistanceToTarget = 0.f;

	UPROPERTY(EditAnywhere, Category = Input)
	bool bInAttackRange = false;

	// Per-attack cooldown end times, indexed alongside AttackProfile.Attacks.
	TArray<float> AttackCooldownEndTimes;
	FGameplayTagContainer ActiveAbilityTags;
	TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;
	FDelegateHandle EndHandle;
	TWeakObjectPtr<AYogCharacterBase> FlashCharacter;
};

USTRUCT(meta = (DisplayName = "Enemy Attack By Profile", Category = "Yog|AI"))
struct DEVKIT_API FStateTreeTask_EnemyAttackByProfile : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTask_EnemyAttackByProfileInstanceData;

	FStateTreeTask_EnemyAttackByProfile();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
