#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;
class AYogCharacterBase;

struct FActivateAbilityMemory
{
    TWeakObjectPtr<UAbilitySystemComponent> ASC;
    FDelegateHandle EndHandle;
    TWeakObjectPtr<AYogCharacterBase> FlashCharacter;
};

UCLASS()
class DEVKIT_API UBTTask_ActivateAbilityByTag : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ActivateAbilityByTag();

    UPROPERTY(EditAnywhere, Category = "Ability")
    FGameplayTagContainer AbilityTags;

    /** 激活技能的同时触发攻击前摇红光；技能结束时自动停止。勾选后在动画里留出前摇时间即可。 */
    UPROPERTY(EditAnywhere, Category = "Ability")
    bool bPreAttackFlash = true;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    virtual FString GetStaticDescription() const override;

    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FActivateAbilityMemory); }
};
