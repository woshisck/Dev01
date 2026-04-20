#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_HasAbilityWithTag.generated.h"

/**
 * 检查 Pawn 的 ASC 中是否存在匹配 AbilityTags 的可激活 GA。
 * 挂在 BTTask_ActivateAbilityByTag 节点上，没有对应 GA 时直接跳过该节点。
 */
UCLASS()
class DEVKIT_API UBTDecorator_HasAbilityWithTag : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasAbilityWithTag();

    UPROPERTY(EditAnywhere, Category = "Ability")
    FGameplayTagContainer AbilityTags;

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
    virtual FString GetStaticDescription() const override;
};
