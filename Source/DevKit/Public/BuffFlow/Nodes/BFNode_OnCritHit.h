#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnCritHit.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 触发暴击时激活
 * 绑定 ASC->OnCritHit 委托
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "当触发暴击时", Category = "BuffFlow|触发器"))
class DEVKIT_API UBFNode_OnCritHit : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleCritHit(UYogAbilitySystemComponent* TargetASC, float Damage);

private:
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
