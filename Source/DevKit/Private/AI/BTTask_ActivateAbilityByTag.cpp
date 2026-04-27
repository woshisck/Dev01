#include "AI/BTTask_ActivateAbilityByTag.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

UBTTask_ActivateAbilityByTag::UBTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("Activate Ability By Tag");
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UBTTask_ActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] Enter — Pawn=%s RequestedTags=%s"),
		Pawn ? *Pawn->GetName() : TEXT("null"),
		*AbilityTags.ToStringSimple());

	if (!AIC) { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: no AIController")); return EBTNodeResult::Failed; }
	if (!Pawn) { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: no Pawn")); return EBTNodeResult::Failed; }

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
	if (!ASC) { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: no ASC")); return EBTNodeResult::Failed; }

	UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
	if (!YogASC) { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: ASC not YogASC")); return EBTNodeResult::Failed; }

	// 过滤掉没有蒙太奇数据的 Tag，只在敌人实际配置了蒙太奇的攻击中随机选
	FGameplayTagContainer ValidTags;
	if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn))
	{
		if (const UCharacterDataComponent* DC = Char->GetCharacterDataComponent())
		{
			if (const UCharacterData* CD = DC->GetCharacterData())
			{
				if (const UAbilityData* AD = CD->AbilityData)
				{
					for (const FGameplayTag& Tag : AbilityTags)
					{
						const bool bHas = AD->HasAbility(Tag);
						UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag]   Tag=%s | AD=%s HasAbility=%d"),
							*Tag.ToString(), *AD->GetName(), bHas);
						if (bHas) ValidTags.AddTag(Tag);
					}
				}
				else { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: CD->AbilityData is null on %s"), *CD->GetName()); }
			}
			else { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: DC->GetCharacterData is null")); }
		}
		else { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: Char->GetCharacterDataComponent is null")); }
	}
	else { UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: Pawn not YogCharacterBase")); }

	if (ValidTags.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BTT_ActivateAbilityByTag] FAIL: ValidTags empty (no matching ability for requested tags)"));
		return EBTNodeResult::Failed;
	}

	// 攻击前摇红光（勾选 bPreAttackFlash 时，与技能同步开始）
	auto* Memory = CastInstanceNodeMemory<FActivateAbilityMemory>(NodeMemory);
	if (bPreAttackFlash)
	{
		if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn))
		{
			Char->StartPreAttackFlash();
			Memory->FlashCharacter = Char;
		}
	}

	// 尝试激活匹配 Tag 的 GA（只从有效 Tag 中随机选）
	const bool bActivated = YogASC->TryActivateRandomAbilitiesByTag(ValidTags, false);
	if (!bActivated)
	{
		if (Memory->FlashCharacter.IsValid())
		{
			Memory->FlashCharacter->StopPreAttackFlash();
			Memory->FlashCharacter.Reset();
		}
		return EBTNodeResult::Failed;
	}

	// 检查 GA 是否已同步结束（如无蒙太奇时会在 ActivateAbility 内直接 EndAbility）
	// 若此时没有匹配 Tag 的 Spec 仍处于 Active 状态，说明 GA 已立即结束，直接返回 Succeeded。
	// 否则注册 OnAbilityEnded 回调，等待异步结束（BT Task 保持 InProgress）。
	bool bStillActive = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.IsActive() && Spec.Ability && Spec.Ability->AbilityTags.HasAny(AbilityTags))
		{
			bStillActive = true;
			break;
		}
	}

	if (!bStillActive)
	{
		// GA 在激活期间已同步结束，无需等待回调
		return EBTNodeResult::Succeeded;
	}

	// GA 仍在运行，注册结束回调等待完成
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

	// 停止红光（技能正常结束或被打断均会走这里）
	if (Memory->FlashCharacter.IsValid())
	{
		Memory->FlashCharacter->StopPreAttackFlash();
		Memory->FlashCharacter.Reset();
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_ActivateAbilityByTag::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate: %s"), *AbilityTags.ToStringSimple());
}
