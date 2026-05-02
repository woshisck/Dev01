#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "GameplayTagContainer.h"
#include "BTTask_EnemyAttackByProfile.generated.h"

class AYogCharacterBase;
class UAbilitySystemComponent;

UCLASS()
class DEVKIT_API UBTTask_EnemyAttackByProfile : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyAttackByProfile();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetKey;

private:
	struct FAttackCandidate
	{
		int32 AttackIndex = INDEX_NONE;
		float Weight = 0.0f;
		FGameplayTagContainer ValidTags;
	};

	TArray<float> AttackCooldownEndTimes;

	TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;

	FDelegateHandle ActiveEndHandle;

	TWeakObjectPtr<AYogCharacterBase> FlashCharacter;

	FGameplayTagContainer ActiveAbilityTags;

	float ResolveDistanceToTarget(const UBehaviorTreeComponent& OwnerComp, const APawn& Pawn) const;
	AActor* ResolveTargetActor(const UBehaviorTreeComponent& OwnerComp) const;
	void ClearActiveAbilityDelegate();
	void StopFlash();
};
