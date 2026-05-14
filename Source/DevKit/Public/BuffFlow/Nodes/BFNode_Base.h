#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_Base.generated.h"

class UBuffFlowComponent;
class UYogAbilitySystemComponent;
class AYogCharacterBase;

/**
 * 所有 BuffFlow 节点的基类
 * 提供访问 BuffFlowComponent / ASC / BuffOwner 的便捷方法
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API UBFNode_Base : public UFlowNode
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Sealed wrapper around UFlowNode::ExecuteInput.
	 * Records a BuffFlow trace entry on every activation (so the editor debug
	 * tool always shows the FA invocation), then dispatches to the subclass
	 * via ExecuteBuffFlowInput. Subclasses MUST override ExecuteBuffFlowInput
	 * instead of ExecuteInput — overriding ExecuteInput would silently bypass
	 * the trace, which is the exact bug this wrapper fixes.
	 */
	virtual void ExecuteInput(const FName& PinName) override final;

protected:
	/**
	 * Subclass override point. Default implementation routes to UFlowNode's
	 * ExecuteInput so Blueprint-derived nodes (UBFNode_BlueprintBase) still
	 * fire their K2_ExecuteInput event.
	 */
	virtual void ExecuteBuffFlowInput(const FName& PinName);

	/** 获取挂在角色上的 BuffFlowComponent */
	UBuffFlowComponent* GetBuffFlowComponent() const;

	/** 获取 Buff 拥有者的 ASC */
	UYogAbilitySystemComponent* GetOwnerASC() const;

	/** 获取 Buff 拥有者角色 */
	AYogCharacterBase* GetBuffOwner() const;

	/** 根据 Target 选择器解析目标 Actor */
	AActor* ResolveTarget(EBFTargetSelector Selector) const;

private:
	/** 缓存，避免每次调用都查找 */
	mutable TWeakObjectPtr<UBuffFlowComponent> CachedComponent;
};
