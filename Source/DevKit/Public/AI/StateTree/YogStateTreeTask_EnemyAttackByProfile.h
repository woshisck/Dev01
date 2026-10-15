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
// The state IS the attack definition: it names the ability tags to fire plus the
// range / health / cooldown gates that guard them. Picks uniformly at random
// among the tags that are granted and off cooldown, activates the chosen GA, and
// stays Running until it ends.
//
// Cooldowns deliberately live on AYogAIController, never here: StateTree
// reallocates task instance data on every state entry, and this state is
// re-entered after each attack, so a deadline stored here would always read as
// expired and the attack would never go on cooldown.
//
// TargetActor / DistanceToTarget can be bound to the "Update Enemy Combat Move"
// evaluator outputs. If omitted, EnterState uses runtime fallbacks so generated
// template trees remain directly compilable.

USTRUCT()
struct FStateTreeTask_EnemyAttackByProfileInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float MinRange = 0.f;

	// 0 means no upper range limit.
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float MaxRange = 300.f;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float Cooldown = 1.f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bPreAttackFlash = true;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinHealthPercent = 0.f;

	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxHealthPercent = 1.f;

	// No-damage movement option (e.g. a whiff reposition feint): never reports a
	// whiff and clears the whiff flag when used.
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bIsReposition = false;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bRequestRepositionOnResolve = false;

	UPROPERTY(EditAnywhere, Category = Input, meta = (Optional))
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Input, meta = (Optional))
	float DistanceToTarget = 0.f;

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
