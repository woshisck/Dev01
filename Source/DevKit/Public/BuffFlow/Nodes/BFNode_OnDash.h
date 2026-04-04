#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnDash.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 执行冲刺时触发
 * 绑定 ASC->OnDashExecuted 委托
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Dash", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnDash : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleDash();

private:
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
