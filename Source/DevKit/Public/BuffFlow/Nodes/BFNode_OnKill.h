#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnKill.generated.h"

class UYogAbilitySystemComponent;

/**
 * 延时节点：当 BuffOwner 击杀敌人时触发
 * 绑定 ASC->OnKilledTarget 委托
 * 自动将击杀位置写入 BFC->LastKillLocation 和被击杀 Actor 写入 LastEventContext
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "当击杀敌人时", Category = "BuffFlow|触发器"))
class DEVKIT_API UBFNode_OnKill : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleKill(AActor* Target, FVector DeathLocation);

private:
	TWeakObjectPtr<UYogAbilitySystemComponent> BoundASC;
};
