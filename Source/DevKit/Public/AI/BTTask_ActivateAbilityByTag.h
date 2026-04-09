#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;

/** BTTask 节点内存：存储 ASC 弱引用和委托句柄，供回调清理用 */
struct FActivateAbilityMemory
{
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FDelegateHandle EndHandle;
};

/**
 * 行为树任务：通过 GameplayTag 激活 GA，等待 GA 结束后返回 Success
 * - 无需在蓝图里手写 GetCurrentAbilityInstance / Listen for Ability End
 * - AbilityTags 填写 GA 的 AbilityTags（与 AbilityData 里的 Key 一致）
 * - TryActivateRandomAbilitiesByTag 失败（无匹配 GA / 被 Block）时返回 Failed
 */
UCLASS()
class DEVKIT_API UBTTask_ActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ActivateAbilityByTag();

	/** 要激活的 GA AbilityTags，随机选一个执行 */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTagContainer AbilityTags;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FActivateAbilityMemory); }
	virtual FString GetStaticDescription() const override;
};
