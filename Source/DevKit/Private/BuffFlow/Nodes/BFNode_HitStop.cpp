#include "BuffFlow/Nodes/BFNode_HitStop.h"
#include "Animation/HitStopManager.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"

namespace HitStopTags
{
	static const FGameplayTag Freeze = FGameplayTag::RequestGameplayTag("Buff.Status.HitStop.Freeze");
	static const FGameplayTag Slow   = FGameplayTag::RequestGameplayTag("Buff.Status.HitStop.Slow");
}

UBFNode_HitStop::UBFNode_HitStop(const FObjectInitializer& OI) : Super(OI)
{
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_HitStop::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();
	AYogCharacterBase* Owner = GetBuffOwner();
	if (!ASC || !Owner)
	{
		TriggerFirstOutput(true);
		return;
	}

	const bool bCanFreeze = ASC->HasMatchingGameplayTag(HitStopTags::Freeze);
	const bool bCanSlow   = ASC->HasMatchingGameplayTag(HitStopTags::Slow);

	if (!bCanFreeze && !bCanSlow)
	{
		TriggerFirstOutput(true);
		return;
	}

	UAnimInstance* AnimInst = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		TriggerFirstOutput(true);
		return;
	}

	// 消费 Tag（先消费再触发，防止同帧重入）
	if (bCanFreeze) ASC->RemoveLooseGameplayTag(HitStopTags::Freeze);
	if (bCanSlow)   ASC->RemoveLooseGameplayTag(HitStopTags::Slow);

	const float EffectiveFrozen = bCanFreeze ? FrozenDuration : 0.f;
	const float EffectiveSlow   = bCanSlow   ? SlowDuration   : 0.f;

	if (UHitStopManager* Mgr = Owner->GetWorld()->GetSubsystem<UHitStopManager>())
	{
		Mgr->RequestMontageHitStop(AnimInst, EffectiveFrozen, EffectiveSlow, SlowRate, CatchUpRate);
	}

	TriggerFirstOutput(true);
}
