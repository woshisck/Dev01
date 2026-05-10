#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_AddTag.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h"
#include "BuffFlow/Nodes/BFNode_AreaDamage.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/Nodes/BFNode_CheckDistance.h"
#include "BuffFlow/Nodes/BFNode_CheckTargetType.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"
#include "BuffFlow/Nodes/BFNode_Delay.h"
#include "BuffFlow/Nodes/BFNode_DoDamage.h"
#include "BuffFlow/Nodes/BFNode_DoOnce.h"
#include "BuffFlow/Nodes/BFNode_FinishBuff.h"
#include "BuffFlow/Nodes/BFNode_Fork.h"
#include "BuffFlow/Nodes/BFNode_GrantGA.h"
#include "BuffFlow/Nodes/BFNode_GetRuneTuningValue.h"
#include "BuffFlow/Nodes/BFNode_HasTag.h"
#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "BuffFlow/Nodes/BFNode_OnCritHit.h"
#include "BuffFlow/Nodes/BFNode_OnDamageDealt.h"
#include "BuffFlow/Nodes/BFNode_OnDamageReceived.h"
#include "BuffFlow/Nodes/BFNode_OnDash.h"
#include "BuffFlow/Nodes/BFNode_OnKill.h"
#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"
#include "BuffFlow/Nodes/BFNode_PlayMontage.h"
#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h"
#include "BuffFlow/Nodes/BFNode_Probability.h"
#include "BuffFlow/Nodes/BFNode_RemoveTag.h"
#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h"
#include "BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h"
#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
#include "BuffFlow/Nodes/BFNode_PureData.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Types/FlowDataPinResults.h"
#include "YogFlowNodes.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "流程控制", Category = "技能"))
class DEVKIT_API UYogFlowNode_SkillPass : public UBFNode_Base
{
	GENERATED_BODY()

public:
	UYogFlowNode_SkillPass(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "分叉", Category = "技能|流程", Keywords = "Fork Branch Split Parallel Multi Output 分叉 并行 多输出 流程"))
class DEVKIT_API UYogFlowNode_Fork : public UBFNode_Fork
{
	GENERATED_BODY()

public:
	UYogFlowNode_Fork(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "造成伤害时", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerDamageDealt : public UBFNode_OnDamageDealt
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDamageDealt(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "受到伤害时", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerDamageReceived : public UBFNode_OnDamageReceived
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDamageReceived(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "暴击时", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerCritHit : public UBFNode_OnCritHit
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerCritHit(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "击杀时", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerKill : public UBFNode_OnKill
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerKill(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "冲刺时", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerDash : public UBFNode_OnDash
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDash(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "等待事件", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_TriggerGameplayEvent : public UBFNode_WaitGameplayEvent
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerGameplayEvent(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "发送事件", Category = "技能|触发"))
class DEVKIT_API UYogFlowNode_EffectSendGameplayEvent : public UBFNode_SendGameplayEvent
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectSendGameplayEvent(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "伤害", Category = "效果节点|瞬时效果"))
class DEVKIT_API UYogFlowNode_EffectDamage : public UBFNode_DoDamage
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectDamage(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "治疗", Category = "效果节点|瞬时效果"))
class DEVKIT_API UYogFlowNode_EffectHeal : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectHeal(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "消耗", Category = "效果节点|瞬时效果"))
class DEVKIT_API UYogFlowNode_EffectCost : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectCost(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "属性修改", Category = "效果节点|持续效果"))
class DEVKIT_API UYogFlowNode_EffectAttributeModify : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAttributeModify(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "施加状态", Category = "效果节点|状态效果"))
class DEVKIT_API UYogFlowNode_EffectApplyState : public UBFNode_ApplyEffect
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyState(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "效果配置", Category = "效果节点|通用效果"))
class DEVKIT_API UYogFlowNode_EffectApplyProfile : public UBFNode_ApplyRuneEffectProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "范围施加GE", Category = "效果节点|范围效果"))
class DEVKIT_API UYogFlowNode_EffectApplyInRadius : public UBFNode_ApplyGEInRadius
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyInRadius(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "范围伤害", Category = "效果节点|范围效果"))
class DEVKIT_API UYogFlowNode_EffectAreaDamage : public UBFNode_AreaDamage
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAreaDamage(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "添加Tag", Category = "效果节点|状态效果"))
class DEVKIT_API UYogFlowNode_EffectAddTag : public UBFNode_AddTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAddTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "移除Tag", Category = "效果节点|状态效果"))
class DEVKIT_API UYogFlowNode_EffectRemoveTag : public UBFNode_RemoveTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectRemoveTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "授予能力", Category = "效果节点|通用效果"))
class DEVKIT_API UYogFlowNode_EffectGrantAbility : public UBFNode_GrantGA
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectGrantAbility(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "搜索目标", Category = "任务节点"))
class DEVKIT_API UYogFlowNode_TaskSearchTarget : public UBFNode_CheckTargetType
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskSearchTarget(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "结束技能", Category = "任务节点"))
class DEVKIT_API UYogFlowNode_TaskEndSkill : public UFlowNode_Finish
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskEndSkill(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "动画", Category = "任务节点"))
class DEVKIT_API UYogFlowNode_TaskPlayAnimation : public UBFNode_PlayMontage
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskPlayAnimation(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "生成投射物配置", Category = "任务节点|生成"))
class DEVKIT_API UYogFlowNode_SpawnProjectileProfile : public UBFNode_SpawnRuneProjectileProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnProjectileProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "生成区域配置", Category = "任务节点|生成"))
class DEVKIT_API UYogFlowNode_SpawnAreaProfile : public UBFNode_SpawnRuneAreaProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnAreaProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "生成地面路径", Category = "任务节点|生成"))
class DEVKIT_API UYogFlowNode_SpawnGroundPath : public UBFNode_SpawnRuneGroundPathEffect
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnGroundPath(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "生成远程弹幕", Category = "任务节点|生成"))
class DEVKIT_API UYogFlowNode_SpawnRangedProjectiles : public UBFNode_SpawnRangedProjectiles
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "属性比较", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionAttributeCompare : public UBFNode_CompareFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionAttributeCompare(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "比较数值", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionCompareFloat : public UBFNode_CompareFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionCompareFloat(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "拥有Tag", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionHasTag : public UBFNode_HasTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionHasTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "概率判断", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionProbability : public UBFNode_Probability
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionProbability(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "只执行一次", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionDoOnce : public UBFNode_DoOnce
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionDoOnce(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "距离判断", Category = "条件节点"))
class DEVKIT_API UYogFlowNode_ConditionCheckDistance : public UBFNode_CheckDistance
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionCheckDistance(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "特效表现", Category = "表现节点"))
class DEVKIT_API UYogFlowNode_PresentationPlayVFX : public UBFNode_PlayNiagara
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationPlayVFX(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Cue到角色", Category = "表现节点"))
class DEVKIT_API UYogFlowNode_PresentationCueOnActor : public UBFNode_SpawnGameplayCueOnActor
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationCueOnActor(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Cue到位置", Category = "表现节点"))
class DEVKIT_API UYogFlowNode_PresentationCueAtLocation : public UBFNode_SpawnGameplayCueAtLocation
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationCueAtLocation(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "VFX配置", Category = "表现节点"))
class DEVKIT_API UYogFlowNode_PresentationVFXProfile : public UBFNode_PlayRuneVFXProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationVFXProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "序列帧特效", Category = "表现节点"))
class DEVKIT_API UYogFlowNode_PresentationFlipbook : public UBFNode_PlayFlipbookVFX
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationFlipbook(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "延迟", Category = "技能|生命周期"))
class DEVKIT_API UYogFlowNode_LifecycleDelay : public UBFNode_Delay
{
	GENERATED_BODY()

public:
	UYogFlowNode_LifecycleDelay(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "结束符文", Category = "技能|生命周期"))
class DEVKIT_API UYogFlowNode_LifecycleFinishBuff : public UBFNode_FinishBuff
{
	GENERATED_BODY()

public:
	UYogFlowNode_LifecycleFinishBuff(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "读取调参数值", Category = "Pure"))
class DEVKIT_API UYogFlowNode_RuneTuningValue : public UBFNode_GetRuneTuningValue
{
	GENERATED_BODY()

public:
	UYogFlowNode_RuneTuningValue(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "浮点运算", Category = "Pure"))
class DEVKIT_API UYogFlowNode_MathFloat : public UBFNode_MathFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_MathFloat(const FObjectInitializer& ObjectInitializer);
};

// ---------------------------------------------------------------------------
// Pure 数据节点 — 无执行引脚，仅供输出数值
// ---------------------------------------------------------------------------

UCLASS(NotBlueprintable, meta = (DisplayName = "读取数值（Pure）", Category = "Pure"))
class DEVKIT_API UBFNode_Pure_TuningValue : public UBFNode_PureData
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "数值表中的 Key 名称"))
	FName TuningKey;

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "Key 不存在时的回退值"))
	float DefaultValue = 0.f;

	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "连击段数（Pure）", Category = "Pure"))
class DEVKIT_API UBFNode_Pure_ComboIndex : public UBFNode_PureData
{
	GENERATED_UCLASS_BODY()

	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
};
