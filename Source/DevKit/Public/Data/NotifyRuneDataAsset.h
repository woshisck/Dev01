#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NotifyRuneDataAsset.generated.h"

class UNotifyFlowAsset;

/**
 * 专用于 AN_MeleeDamage::AdditionalRuneEffects 的一次性命中符文资产。
 *
 * 与背包符文（URuneDataAsset）的区别：
 *  - 不含 Shape / RuneConfig 等背包系统字段
 *  - FlowAsset 类型为 UNotifyFlowAsset（一次性 FA，无 OnDamageDealt 等待）
 *  - 由 ReceiveOnHitRune 在命中时启动，BuffOwner=攻击者，BuffGiver=被命中目标
 *
 * 创建方式：Content Browser → 右键 → Miscellaneous → Data Asset → NotifyRuneDataAsset
 * 命名建议：DA_HitRune_<效果名>（区别于背包符文的 DA_Rune_<名>）
 */
UCLASS(BlueprintType, meta = (DisplayName = "Notify Rune Data Asset"))
class DEVKIT_API UNotifyRuneDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 命中时在攻击者 BFC 上启动的一次性 FA（Start → 效果 → Finish，无 OnDamageDealt 等待节点）*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NotifyRune")
	TObjectPtr<UNotifyFlowAsset> FlowAsset;
};
