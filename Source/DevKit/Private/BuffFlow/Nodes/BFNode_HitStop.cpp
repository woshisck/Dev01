#include "BuffFlow/Nodes/BFNode_HitStop.h"
#include "Animation/HitStopManager.h"
#include "Animation/AN_MeleeDamage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"

namespace HitStopTags
{
	static FGameplayTag Freeze()
	{
		return FGameplayTag::RequestGameplayTag("Buff.Status.HitStop.Freeze");
	}

	static FGameplayTag Slow()
	{
		return FGameplayTag::RequestGameplayTag("Buff.Status.HitStop.Slow");
	}
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

	auto& Override = Owner->PendingHitStopOverride;

	const FGameplayTag FreezeTag = HitStopTags::Freeze();
	const FGameplayTag SlowTag = HitStopTags::Slow();
	bool bCanFreeze = ASC->HasMatchingGameplayTag(FreezeTag);
	bool bCanSlow   = ASC->HasMatchingGameplayTag(SlowTag);

	// AN 配置的 HitStop 模式直接激活对应阶段（不依赖 FA 写 Tag）
	if (Override.bActive)
	{
		if (Override.Mode == EHitStopMode::Freeze) bCanFreeze = true;
		else if (Override.Mode == EHitStopMode::Slow) bCanSlow = true;
	}

	if (!bCanFreeze && !bCanSlow)
	{
		Override.bActive = false;
		TriggerFirstOutput(true);
		return;
	}

	UAnimInstance* AnimInst = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst)
	{
		Override.bActive = false;
		TriggerFirstOutput(true);
		return;
	}

	// 消费 Tag（先消费再触发，防止同帧重入）
	if (bCanFreeze) ASC->RemoveLooseGameplayTag(FreezeTag);
	if (bCanSlow)   ASC->RemoveLooseGameplayTag(SlowTag);

	// AN 覆盖参数只作用于 AN 指定的模式，Tag 触发的模式使用节点默认值
	const bool bOverrideFreeze = Override.bActive && Override.Mode == EHitStopMode::Freeze;
	const bool bOverrideSlow   = Override.bActive && Override.Mode == EHitStopMode::Slow;

	const float UseFrozen   = bCanFreeze ? (bOverrideFreeze ? Override.FrozenDuration : FrozenDuration) : 0.f;
	const float UseSlow     = bCanSlow   ? (bOverrideSlow   ? Override.SlowDuration   : SlowDuration)   : 0.f;
	const float UseSlowRate = bOverrideSlow ? Override.SlowRate    : SlowRate;
	const float UseCatchUp  = bOverrideSlow ? Override.CatchUpRate : CatchUpRate;
	Override.bActive = false;

	if (UHitStopManager* Mgr = Owner->GetWorld()->GetSubsystem<UHitStopManager>())
	{
		Mgr->RequestMontageHitStop(AnimInst, UseFrozen, UseSlow, UseSlowRate, UseCatchUp);
	}

	TriggerFirstOutput(true);
}
