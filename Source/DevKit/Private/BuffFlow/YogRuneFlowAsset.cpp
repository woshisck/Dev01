#include "BuffFlow/YogRuneFlowAsset.h"

#include "BuffFlow/Nodes/YogFlowNodes.h"
#include "Nodes/Graph/FlowNode_Start.h"

UYogRuneFlowAsset::UYogRuneFlowAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	AllowedNodeClasses.Reset();
	AllowedNodeClasses.Add(UFlowNode_Start::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SkillPass::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerDamageDealt::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerDamageReceived::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerCritHit::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerKill::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerDash::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TriggerGameplayEvent::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectDamage::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectHeal::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectCost::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectAttributeModify::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectApplyState::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectApplyProfile::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectApplyInRadius::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectAreaDamage::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectAddTag::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectRemoveTag::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_EffectGrantAbility::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TaskSearchTarget::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TaskEndSkill::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_TaskPlayAnimation::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SpawnProjectileProfile::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SpawnAreaProfile::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SpawnGroundPath::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SpawnRangedProjectiles::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_SpawnSlashWave::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_ConditionAttributeCompare::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_ConditionHasTag::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_ConditionProbability::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_ConditionDoOnce::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_ConditionCheckDistance::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_PresentationPlayVFX::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_PresentationCueOnActor::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_PresentationCueAtLocation::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_PresentationVFXProfile::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_PresentationFlipbook::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_LifecycleDelay::StaticClass());
	AllowedNodeClasses.Add(UYogFlowNode_LifecycleFinishBuff::StaticClass());

	AllowedInSubgraphNodeClasses.Reset();
	DeniedNodeClasses.Reset();
	DeniedInSubgraphNodeClasses.Reset();
#endif
}
