#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;

struct FActivateAbilityMemory
{
    TWeakObjectPtr<UAbilitySystemComponent> ASC;
    FDelegateHandle EndHandle;
};

UCLASS()
class DEVKIT_API UBTTask_ActivateAbilityByTag : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ActivateAbilityByTag();

    UPROPERTY(EditAnywhere, Category = "Ability")
    FGameplayTagContainer AbilityTags;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    virtual FString GetStaticDescription() const override;

    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FActivateAbilityMemory); }
};
