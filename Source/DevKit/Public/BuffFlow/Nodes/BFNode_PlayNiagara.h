#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_PlayNiagara.generated.h"

class UNiagaraSystem;

/**
 * 瞬时节点：在目标上播放 Niagara 特效，并存入 BuffFlowComponent::ActiveNiagaraEffects
 * 供 DestroyNiagara 节点后续销毁
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Play Niagara", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayNiagara : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** Niagara 特效资产 */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** 用于在 ActiveNiagaraEffects 中索引（DestroyNiagara 用同名找到它） */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	FName EffectName = NAME_None;

	/** 特效附加的挂点 */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	FName AttachSocketName = NAME_None;

	/** 特效附加的目标 */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	EBFTargetSelector AttachTarget = EBFTargetSelector::BuffOwner;

	/** Flow 结束时自动销毁特效 */
	UPROPERTY(EditAnywhere, Category = "Niagara")
	bool bDestroyWithFlow = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;
};
