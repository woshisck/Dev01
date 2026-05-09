#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "BFNode_DoDamage.generated.h"

/**
 * 瞬时节点：主动对目标施加伤害（通过 GE 施加到目标 ASC）
 * 等同于 ApplyEffect 但语义更直白，专门用于造成伤害
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Do Damage", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_DoDamage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 伤害目标 — 对哪个 Actor 施加伤害
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (DisplayName = "伤害目标"))
	EBFTargetSelector TargetSelector = EBFTargetSelector::LastDamageTarget;

	// 固定伤害值 — > 0 时直接使用；否则用上次伤害量 × 伤害倍率；可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (DisplayName = "固定伤害值"))
	FFlowDataPinInputProperty_Float FlatDamage;

	// 伤害倍率 — 固定伤害值为 0 时生效（默认 1.0），对上次伤害量做乘算；可接数据引脚连线
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (DisplayName = "伤害倍率"))
	FFlowDataPinInputProperty_Float DamageMultiplier;

	// 伤害效果类 — 施加到目标 ASC 的 GE（需配置 Modifier 扣减 Health）
	UPROPERTY(EditAnywhere, Category = "DoDamage", meta = (DisplayName = "伤害效果类"))
	TSubclassOf<UGameplayEffect> DamageEffect;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
