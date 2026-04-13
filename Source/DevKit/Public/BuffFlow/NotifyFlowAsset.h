#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.h"
#include "NotifyFlowAsset.generated.h"

/**
 * FlowAsset 子类，专用于 AN_MeleeDamage::AdditionalRuneEffects 中的一次性命中效果。
 *
 * FA 结构固定为：[Start] → [Send Gameplay Event（Target=BuffGiver）] → [Finish]
 * 不含 OnDamageDealt 等待节点，命中时立即执行一次，不循环。
 *
 * 创建方式：Content Browser → 右键 → Flow → Flow Asset → 类选择器中选 NotifyFlowAsset
 * 命名建议：FA_HitEffect_<效果名>（区别于背包符文的 FA_Rune_<名>）
 */
UCLASS(BlueprintType, meta = (DisplayName = "Notify Flow Asset"))
class DEVKIT_API UNotifyFlowAsset : public UFlowAsset
{
	GENERATED_BODY()
};
