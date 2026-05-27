#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_Base::UBFNode_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// 在普通 FA 中可见，排除 LevelFlowAsset（关卡事件）和 UStoryFlowAsset（导演系统）
	DeniedAssetClasses = { ULevelFlowAsset::StaticClass(), UStoryFlowAsset::StaticClass() };
#endif
}

void UBFNode_Base::ExecuteInput(const FName& PinName)
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->RecordTrace(
			this,
			nullptr,
			nullptr,
			EBuffFlowTraceResult::Success,
			FString::Printf(TEXT("In:%s"), *PinName.ToString()),
			FString());
	}

	ExecuteBuffFlowInput(PinName);
}

void UBFNode_Base::ExecuteBuffFlowInput(const FName& PinName)
{
	// Default: route to UFlowNode's ExecuteInput so Blueprint subclasses fire
	// their K2_ExecuteInput event. C++ subclasses override this method instead.
	Super::ExecuteInput(PinName);
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
	case EBFTargetSelector::LifecycleTarget:
		return BFC->GetLifecycleTarget();
	case EBFTargetSelector::AllHitTargets:
		return BFC->LastEventContext.DamageReceiver.Get();
	default:
		return BFC->GetBuffOwner();
	}
}

TArray<AActor*> UBFNode_Base::ResolveAllTargets(EBFTargetSelector Selector) const
{
	TArray<AActor*> Result;

	if (Selector != EBFTargetSelector::AllHitTargets)
	{
		if (AActor* Single = ResolveTarget(Selector))
		{
			Result.Add(Single);
		}
		return Result;
	}

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		return Result;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : BFC->LastEventContext.DamageReceivers)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Result.Add(Actor);
		}
	}
	return Result;
}
