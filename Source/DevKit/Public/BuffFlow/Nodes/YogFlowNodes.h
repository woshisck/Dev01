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
#include "BuffFlow/Nodes/BFNode_CompareBool.h"
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
#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
#include "BuffFlow/Nodes/BFNode_PureData.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Nodes/Graph/FlowNode_Finish.h"
#include "Types/FlowDataPinResults.h"
#include "YogFlowNodes.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Flow Pass", Category = "Skill"))
class DEVKIT_API UYogFlowNode_SkillPass : public UBFNode_Base
{
	GENERATED_BODY()

public:
	UYogFlowNode_SkillPass(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Fork", Category = "Skill|Flow", Keywords = "Fork Branch Split Parallel Multi Output Flow"))
class DEVKIT_API UYogFlowNode_Fork : public UBFNode_Fork
{
	GENERATED_BODY()

public:
	UYogFlowNode_Fork(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "On Damage Dealt", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerDamageDealt : public UBFNode_OnDamageDealt
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDamageDealt(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "On Damage Received", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerDamageReceived : public UBFNode_OnDamageReceived
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDamageReceived(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "On Crit Hit", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerCritHit : public UBFNode_OnCritHit
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerCritHit(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "On Kill", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerKill : public UBFNode_OnKill
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerKill(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "On Dash", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerDash : public UBFNode_OnDash
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerDash(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Wait Gameplay Event", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_TriggerGameplayEvent : public UBFNode_WaitGameplayEvent
{
	GENERATED_BODY()

public:
	UYogFlowNode_TriggerGameplayEvent(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Send Gameplay Event", Category = "Skill|Trigger"))
class DEVKIT_API UYogFlowNode_EffectSendGameplayEvent : public UBFNode_SendGameplayEvent
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectSendGameplayEvent(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Damage", Category = "Effect|Instant"))
class DEVKIT_API UYogFlowNode_EffectDamage : public UBFNode_DoDamage
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectDamage(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Heal", Category = "Effect|Instant"))
class DEVKIT_API UYogFlowNode_EffectHeal : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectHeal(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Cost", Category = "Effect|Instant"))
class DEVKIT_API UYogFlowNode_EffectCost : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectCost(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Modify Attribute", Category = "Effect|Duration"))
class DEVKIT_API UYogFlowNode_EffectAttributeModify : public UBFNode_ApplyAttributeModifier
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAttributeModify(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Apply State", Category = "Effect|State"))
class DEVKIT_API UYogFlowNode_EffectApplyState : public UBFNode_ApplyEffect
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyState(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Effect Profile", Category = "Effect|Profile"))
class DEVKIT_API UYogFlowNode_EffectApplyProfile : public UBFNode_ApplyRuneEffectProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Apply GE in Radius", Category = "Effect|Radius"))
class DEVKIT_API UYogFlowNode_EffectApplyInRadius : public UBFNode_ApplyGEInRadius
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectApplyInRadius(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Area Damage", Category = "Effect|Radius"))
class DEVKIT_API UYogFlowNode_EffectAreaDamage : public UBFNode_AreaDamage
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAreaDamage(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Add Tag", Category = "Effect|State"))
class DEVKIT_API UYogFlowNode_EffectAddTag : public UBFNode_AddTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectAddTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Remove Tag", Category = "Effect|State"))
class DEVKIT_API UYogFlowNode_EffectRemoveTag : public UBFNode_RemoveTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectRemoveTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Grant Ability", Category = "Effect|Profile"))
class DEVKIT_API UYogFlowNode_EffectGrantAbility : public UBFNode_GrantGA
{
	GENERATED_BODY()

public:
	UYogFlowNode_EffectGrantAbility(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Search Target", Category = "Task"))
class DEVKIT_API UYogFlowNode_TaskSearchTarget : public UBFNode_CheckTargetType
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskSearchTarget(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "End Skill", Category = "Task"))
class DEVKIT_API UYogFlowNode_TaskEndSkill : public UFlowNode_Finish
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskEndSkill(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Animation", Category = "Task"))
class DEVKIT_API UYogFlowNode_TaskPlayAnimation : public UBFNode_PlayMontage
{
	GENERATED_BODY()

public:
	UYogFlowNode_TaskPlayAnimation(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Projectile Profile", Category = "Task|Spawn"))
class DEVKIT_API UYogFlowNode_SpawnProjectileProfile : public UBFNode_SpawnRuneProjectileProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnProjectileProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Area Profile", Category = "Task|Spawn"))
class DEVKIT_API UYogFlowNode_SpawnAreaProfile : public UBFNode_SpawnRuneAreaProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnAreaProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Ground Path", Category = "Task|Spawn"))
class DEVKIT_API UYogFlowNode_SpawnGroundPath : public UBFNode_SpawnRuneGroundPathEffect
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnGroundPath(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Ranged Projectiles", Category = "Task|Spawn"))
class DEVKIT_API UYogFlowNode_SpawnRangedProjectiles : public UBFNode_SpawnRangedProjectiles
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn BuffFlow Projectile", Category = "Task|Spawn"))
class DEVKIT_API UYogFlowNode_SpawnBuffFlowProjectile : public UBFNode_SpawnBuffFlowProjectile
{
	GENERATED_BODY()

public:
	UYogFlowNode_SpawnBuffFlowProjectile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Attribute", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionAttributeCompare : public UBFNode_CompareFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionAttributeCompare(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Float", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionCompareFloat : public UBFNode_CompareFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionCompareFloat(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Compare Bool", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionCompareBool : public UBFNode_CompareBool
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionCompareBool(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Has Tag", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionHasTag : public UBFNode_HasTag
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionHasTag(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Probability", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionProbability : public UBFNode_Probability
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionProbability(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Do Once", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionDoOnce : public UBFNode_DoOnce
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionDoOnce(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Check Distance", Category = "Condition"))
class DEVKIT_API UYogFlowNode_ConditionCheckDistance : public UBFNode_CheckDistance
{
	GENERATED_BODY()

public:
	UYogFlowNode_ConditionCheckDistance(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Play VFX", Category = "Presentation"))
class DEVKIT_API UYogFlowNode_PresentationPlayVFX : public UBFNode_PlayNiagara
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationPlayVFX(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Cue on Actor", Category = "Presentation"))
class DEVKIT_API UYogFlowNode_PresentationCueOnActor : public UBFNode_SpawnGameplayCueOnActor
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationCueOnActor(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Cue at Location", Category = "Presentation"))
class DEVKIT_API UYogFlowNode_PresentationCueAtLocation : public UBFNode_SpawnGameplayCueAtLocation
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationCueAtLocation(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Play VFX Profile", Category = "Presentation"))
class DEVKIT_API UYogFlowNode_PresentationVFXProfile : public UBFNode_PlayRuneVFXProfile
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationVFXProfile(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Play Flipbook VFX", Category = "Presentation"))
class DEVKIT_API UYogFlowNode_PresentationFlipbook : public UBFNode_PlayFlipbookVFX
{
	GENERATED_BODY()

public:
	UYogFlowNode_PresentationFlipbook(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Delay", Category = "Skill|Lifecycle"))
class DEVKIT_API UYogFlowNode_LifecycleDelay : public UBFNode_Delay
{
	GENERATED_BODY()

public:
	UYogFlowNode_LifecycleDelay(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Finish Buff", Category = "Skill|Lifecycle"))
class DEVKIT_API UYogFlowNode_LifecycleFinishBuff : public UBFNode_FinishBuff
{
	GENERATED_BODY()

public:
	UYogFlowNode_LifecycleFinishBuff(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Rune Tuning Value", Category = "Pure"))
class DEVKIT_API UYogFlowNode_RuneTuningValue : public UBFNode_GetRuneTuningValue
{
	GENERATED_BODY()

public:
	UYogFlowNode_RuneTuningValue(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Math Float", Category = "Pure"))
class DEVKIT_API UYogFlowNode_MathFloat : public UBFNode_MathFloat
{
	GENERATED_BODY()

public:
	UYogFlowNode_MathFloat(const FObjectInitializer& ObjectInitializer);
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Tuning Value", Category = "Pure"))
class DEVKIT_API UBFNode_Pure_TuningValue : public UBFNode_PureData
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (DisplayName = "Custom Key"))
	bool bCustomKey = false;

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "Choose a preset tuning value key.", GetOptions = "GetPresetKeyNames", EditCondition = "!bCustomKey", EditConditionHides))
	FName TuningKey;

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (DisplayName = "Custom Key", ToolTip = "Enter any tuning value key name.", EditCondition = "bCustomKey", EditConditionHides))
	FName CustomTuningKey;

	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "Fallback value when the key does not exist."))
	float DefaultValue = 0.f;

	UFUNCTION()
	static TArray<FString> GetPresetKeyNames();

	FName GetActiveKey() const { return bCustomKey ? CustomTuningKey : TuningKey; }

	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Combo Index", Category = "Pure"))
class DEVKIT_API UBFNode_Pure_ComboIndex : public UBFNode_PureData
{
	GENERATED_UCLASS_BODY()

	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
};
