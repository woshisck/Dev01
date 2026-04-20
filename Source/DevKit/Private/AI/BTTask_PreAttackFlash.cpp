#include "AI/BTTask_PreAttackFlash.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/YogCharacterBase.h"

UBTTask_PreAttackFlash::UBTTask_PreAttackFlash()
{
    NodeName = TEXT("Pre Attack Flash");
    bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_PreAttackFlash::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
    AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn);
    if (!Char)
        return EBTNodeResult::Failed;

    auto* Mem = CastInstanceNodeMemory<FBTPreAttackFlashMemory>(NodeMemory);
    Mem->Character = Char;

    Char->StartPreAttackFlash();

    // FlashDuration 后自动结束任务，期间被打断走 OnTaskFinished
    TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner(&OwnerComp);
    OwnerComp.GetWorld()->GetTimerManager().SetTimer(
        Mem->TimerHandle,
        [WeakOwner, NodeMemory, this]()
        {
            if (WeakOwner.IsValid())
                OnFlashTimerExpired(WeakOwner.Get(), NodeMemory);
        },
        FlashDuration, false);

    return EBTNodeResult::InProgress;
}

void UBTTask_PreAttackFlash::OnFlashTimerExpired(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
    auto* Mem = CastInstanceNodeMemory<FBTPreAttackFlashMemory>(NodeMemory);
    if (Mem->Character.IsValid())
        Mem->Character->StopPreAttackFlash();
    Mem->Character.Reset();

    FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

void UBTTask_PreAttackFlash::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // 被打断时：清 Timer + 停闪光；正常结束时 Timer 已触发，StopFlash 已在 Expired 里调用
    auto* Mem = CastInstanceNodeMemory<FBTPreAttackFlashMemory>(NodeMemory);
    OwnerComp.GetWorld()->GetTimerManager().ClearTimer(Mem->TimerHandle);
    if (Mem->Character.IsValid())
        Mem->Character->StopPreAttackFlash();

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_PreAttackFlash::GetStaticDescription() const
{
    return FString::Printf(TEXT("Pre Attack Flash\nDuration: %.2fs"), FlashDuration);
}
