#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayEffectTypes.h"
#include "BFNode_OnHealthChanged.generated.h"

class UAbilitySystemComponent;

/**
 * 触发节点：当 BuffOwner 的 HP 属性发生任意变化时触发（包括受伤和回血）。
 * 适用于"痛苦契约"类符文：持续监听血量，每次变化都重算攻速修改器。
 *
 * 使用方式：
 *   In  → 开始监听（绑定属性变化委托）
 *   Stop → 停止监听（解绑）
 *   OnHealthChanged → 每次血量变化时触发（不结束节点，持续监听）
 *   NewHP（数据输出引脚）→ 变化后的血量值，可连线到 GetAttr(MaxHP) / MathFloat 等节点
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Health Changed", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnHealthChanged : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 变化后的生命值（数据输出引脚） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow",
		meta = (DisplayName = "New HP"))
	FFlowDataPinOutputProperty_Float NewHealthOutput;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle HealthChangedHandle;
};
