#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/NotifyFlowAsset.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_Base::UBFNode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// 只在 NotifyFlowAsset（符文/Buff Flow）编辑器中出现，不显示在 LevelFlowAsset 里
	AllowedAssetClasses = { UNotifyFlowAsset::StaticClass() };
#endif
}

UBuffFlowComponent* UBFNode_Base::GetBuffFlowComponent() const
{
	if (CachedComponent.IsValid())
	{
		return CachedComponent.Get();
	}

	UFlowAsset* FlowAsset = GetFlowAsset();
	if (!FlowAsset)
	{
		return nullptr;
	}

	// FlowAsset 的 Owner 就是启动 Flow 的 UBuffFlowComponent
	AActor* OwnerActor = FlowAsset->TryFindActorOwner();
	if (OwnerActor)
	{
		UBuffFlowComponent* Comp = OwnerActor->FindComponentByClass<UBuffFlowComponent>();
		CachedComponent = Comp;
		return Comp;
	}

	return nullptr;
}

UYogAbilitySystemComponent* UBFNode_Base::GetOwnerASC() const
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	return BFC ? BFC->GetASC() : nullptr;
}

AYogCharacterBase* UBFNode_Base::GetBuffOwner() const
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	return BFC ? BFC->GetBuffOwner() : nullptr;
}

AActor* UBFNode_Base::ResolveTarget(EBFTargetSelector Selector) const
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		return nullptr;
	}

	switch (Selector)
	{
	case EBFTargetSelector::BuffOwner:
		return BFC->GetBuffOwner();
	case EBFTargetSelector::BuffGiver:
		return BFC->GetBuffGiver();
	case EBFTargetSelector::LastDamageTarget:
		return BFC->LastEventContext.DamageReceiver.Get();
	case EBFTargetSelector::DamageCauser:
		return BFC->LastEventContext.DamageCauser.Get();
	default:
		return BFC->GetBuffOwner();
	}
}
