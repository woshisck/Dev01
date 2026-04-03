#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "BFNode_DoDamage.generated.h"

/**
 * 瞬时节点：主动对目标施加伤害（通过 GE 施加到目标 ASC）
 * 等同于 ApplyEffect 但语义更直白，专门用于造成伤害
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "造成伤害", Category = "BuffFlow|效果"))
class DEVKIT_API UBFNode_DoDamage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 伤害目标 */
	UPROPERTY(EditAnywhere, Category = "DoDamage")
	EBFTargetSelector TargetSelector = EBFTargetSelector::LastDamageTarget;

	/** 固定伤害值（若 > 0 则使用，否则使用 LastEventContext.DamageAmount * DamageMultiplier） */
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (ClampMin = "0.0"))
	float FlatDamage = 0.f;

	/** 对上次伤害量的倍率（FlatDamage == 0 时生效） */
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;

	/** 要施加的 GE（伤害 GE，需配置 Modifiers 扣减 Health） */
	UPROPERTY(EditAnywhere, Category = "DoDamage")
	TSubclassOf<UGameplayEffect> DamageEffect;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
