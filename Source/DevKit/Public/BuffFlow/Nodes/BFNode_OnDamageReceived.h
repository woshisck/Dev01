#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnDamageReceived.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 受到伤害时触发
 * 绑定 ASC 的 ReceivedDamage 委托
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "当受到伤害时", Category = "BuffFlow|触发器"))
class DEVKIT_API UBFNode_OnDamageReceived : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleDamageReceived(UYogAbilitySystemComponent* SourceASC, float Damage);

private:
	float CachedDamage = 0.f;
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
