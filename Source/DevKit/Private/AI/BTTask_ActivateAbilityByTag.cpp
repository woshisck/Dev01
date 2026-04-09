#include "AI/BTTask_ActivateAbilityByTag.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBTTask_ActivateAbilityByTag::UBTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("Activate Ability By Tag");
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_ActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return EBTNodeResult::Failed;

	UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (!YogASC) return EBTNodeResult::Failed;

	// 尝试激活匹配 Tag 的 GA（随机选一个）
	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(AbilityTags, false);
	if (!bActivated)
	{
		return EBTNodeResult::Failed;
	}

	// 保存 ASC 引用，注册 GA 结束回调
	auto* Memory = CastInstanceNodeMemory<FActivateAbilityMemory>(NodeMemory);
	Memory->ASC = ASC;

	TWeakObjectPtr<UBehaviorTreeComponent> WeakOwner(&OwnerComp);

	Memory->EndHandle = ASC->OnAbilityEnded.AddLambda(
		[this, WeakOwner](const FAbilityEndedData& Data)
		{
			if (!WeakOwner.IsValid()) return;

			// 仅响应 Tag 匹配的 GA 结束
			if (Data.AbilityThatEnded && Data.AbilityThatEnded->AbilityTags.HasAny(AbilityTags))
			{
				FinishLatentTask(*WeakOwner.Get(), EBTNodeResult::Succeeded);
			}
		});

	return EBTNodeResult::InProgress;
}

void UBTTask_ActivateAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// 清理委托，防止 GA 结束后重复触发
	auto* Memory = CastInstanceNodeMemory<FActivateAbilityMemory>(NodeMemory);
	if (Memory->ASC.IsValid() && Memory->EndHandle.IsValid())
	{
		Memory->ASC->OnAbilityEnded.Remove(Memory->EndHandle);
		Memory->EndHandle.Reset();
	}
	Memory->ASC = nullptr;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_ActivateAbilityByTag::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate: %s"), *AbilityTags.ToStringSimple());
}
