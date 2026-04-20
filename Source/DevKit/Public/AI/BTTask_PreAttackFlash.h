#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PreAttackFlash.generated.h"

class AYogCharacterBase;

struct FBTPreAttackFlashMemory
{
    TWeakObjectPtr<AYogCharacterBase> Character;
    FTimerHandle TimerHandle;
};

/**
 * 行为树任务：攻击前摇红光预警
 *
 * 用法：在 BT 中放在攻击任务（BTTask_ActivateAbilityByTag）之前
 *   Sequence
 *   ├── BTTask_PreAttackFlash   (FlashDuration = 前摇时长)
 *   └── BTTask_ActivateAbilityByTag
 *
 * 执行时立即对敌人角色调用 StartPreAttackFlash()，
 * FlashDuration 秒后调用 StopPreAttackFlash() 并返回 Succeeded。
 * 被打断时（Abort）立即停止闪光。
 */
UCLASS()
class DEVKIT_API UBTTask_PreAttackFlash : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_PreAttackFlash();

    /** 红光持续时间（秒），建议与攻击前摇时长匹配，默认 0.6s */
    UPROPERTY(EditAnywhere, Category = "Flash", meta = (ClampMin = "0.05", ClampMax = "5.0"))
    float FlashDuration = 0.6f;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
    virtual FString GetStaticDescription() const override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTPreAttackFlashMemory); }

private:
    void OnFlashTimerExpired(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory);
};
