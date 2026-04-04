#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_DestroyNiagara.generated.h"

/**
 * 瞬时节点：销毁指定名称的 Niagara 特效（从 BuffFlowComponent::ActiveNiagaraEffects 中查找并停用）
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Destroy Niagara", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_DestroyNiagara : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 与 PlayNiagara 节点填写相同的 EffectName */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	FName EffectName = NAME_None;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
