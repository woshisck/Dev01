#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnDamageDealt.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 对别人造成伤害时触发
 * 绑定 ASC 的 DealtDamage 委托，每次命中都触发输出
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Damage Dealt", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnDamageDealt : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 开启后：同一帧内多次命中只触发一次（适合AOE攻击只算一次热度）
	 *  关闭后：每次命中独立触发（默认，命中几个敌人算几次） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow",
		meta = (DisplayName = "Once Per Swing"))
	bool bOncePerSwing = false;

	/** 本次伤害量（数据输出引脚，可连线到 Math Float / Compare Float 等节点） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow",
		meta = (DisplayName = "Last Damage Amount"))
	FFlowDataPinOutputProperty_Float LastDamageOutput;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage);

private:
	float CachedDamage = 0.f;
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
	uint64 LastTriggeredFrame = 0;
};
