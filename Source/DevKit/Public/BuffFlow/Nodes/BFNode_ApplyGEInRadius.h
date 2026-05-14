#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "BFNode_ApplyGEInRadius.generated.h"

/**
 * 对指定位置半径内的所有符合条件的 Actor 施加 GameplayEffect。
 *
 * 位置来源与 BFNode_SpawnGameplayCueAtLocation 一致：
 *  · bUseKillLocation = true  → BFC->LastKillLocation
 *  · bUseKillLocation = false → LocationSource Actor 的位置
 *
 * Filter：
 *  · bEnemyOnly = true（默认）→ 只影响 AEnemyCharacterBase 类型
 *  · bExcludeSelf = true（默认）→ 不影响 BuffOwner
 *
 * SetByCaller 最多支持 2 个槽位，用法与 ApplyEffect 节点一致。
 *
 * 典型用法：
 *  · FA_Rune_KillExplosion：在死亡位置 300cm 半径内施加爆炸伤害 GE
 *  · FA_Rune_Aftershock：在击退落点 250cm 半径内施加减速 GE
 *
 * 输出数据引脚：
 *  · HitCount — 被施加 GE 的目标数量
 *
 * Out    — 至少 1 个目标被施加
 * NoHit  — 半径内无有效目标
 * Failed — GE 无效或位置解析失败
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply GE to Targets in Radius", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyGEInRadius : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要施加的 GameplayEffect 类 */
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSubclassOf<UGameplayEffect> Effect;

	/** 球形检测半径（cm） */
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "1.0"))
	FFlowDataPinInputProperty_Float Radius;

	/**
	 * 中心位置来源 Actor（取其 GetActorLocation()）。
	 * 仅在 bUseKillLocation = false 时生效。
	 */
	UPROPERTY(EditAnywhere, Category = "Effect",
		meta = (EditCondition = "!bUseKillLocation"))
	EBFTargetSelector LocationSource = EBFTargetSelector::LastDamageTarget;

	/** 使用 BFC->LastKillLocation 作为中心位置 */
	UPROPERTY(EditAnywhere, Category = "Effect")
	bool bUseKillLocation = false;

	/** 在基准位置上的额外偏移 */
	UPROPERTY(EditAnywhere, Category = "Effect")
	FVector LocationOffset = FVector::ZeroVector;

	/** 只影响敌方（AEnemyCharacterBase），玩家角色不受影响 */
	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bEnemyOnly = true;

	/** 排除 BuffOwner 自身 */
	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bExcludeSelf = true;

	/** Exclude the actor used as LocationSource. Useful when the center target already receives a primary effect. */
	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bExcludeLocationSourceActor = false;

	/** Maximum number of targets affected. <= 0 means unlimited. */
	UPROPERTY(EditAnywhere, Category = "Filter")
	int32 MaxTargets = 0;

	/** Number of GE applications per target. Useful for poison stacks. */
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "1", ClampMax = "20"))
	int32 ApplicationCount = 1;

	// ─── SetByCaller 槽位 ─────────────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "Slot 1 Tag"))
	FGameplayTag SetByCallerTag1;

	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "Slot 1 Value",
		EditCondition = "SetByCallerTag1.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue1;

	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "Slot 2 Tag"))
	FGameplayTag SetByCallerTag2;

	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "Slot 2 Value",
		EditCondition = "SetByCallerTag2.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue2;

	// ─── 输出数据引脚 ─────────────────────────────────────────────

	/** 被施加 GE 的目标数量 */
	UPROPERTY(EditAnywhere, Category = "Output")
	FFlowDataPinOutputProperty_Int32 HitCount;

	bool ShouldRestrictTargetsToEnemiesForRuntime() const;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
