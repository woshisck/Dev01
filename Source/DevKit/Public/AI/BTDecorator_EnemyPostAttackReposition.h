#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_EnemyPostAttackReposition.generated.h"

UCLASS()
class DEVKIT_API UBTDecorator_EnemyPostAttackReposition : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_EnemyPostAttackReposition();

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector bPostAttackRepositionKey;
};
