#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnDamageDealt.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 对别人造成伤害时触发
 * 绑定 ASC 的 DealtDamage 委托，每次命中都触发输出
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "当造成伤害时", Category = "BuffFlow|触发器"))
class DEVKIT_API UBFNode_OnDamageDealt : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage);

private:
	float CachedDamage = 0.f;
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
