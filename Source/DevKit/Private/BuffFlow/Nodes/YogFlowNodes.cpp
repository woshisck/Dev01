#include "BuffFlow/Nodes/YogFlowNodes.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/ComboRuntimeComponent.h"

namespace
{
	const TCHAR* SkillCategory = TEXT("Skill");
	const TCHAR* TriggerCategory = TEXT("Skill|Trigger");
	const TCHAR* LifecycleCategory = TEXT("Skill|Lifecycle");
	const TCHAR* InstantEffectCategory = TEXT("Effect|Instant");
	const TCHAR* DurationEffectCategory = TEXT("Effect|Duration");
	const TCHAR* StateEffectCategory = TEXT("Effect|State");
	const TCHAR* ProfileEffectCategory = TEXT("Effect|Profile");
	const TCHAR* RadiusEffectCategory = TEXT("Effect|Radius");
	const TCHAR* TaskCategory = TEXT("Task");
	const TCHAR* SpawnCategory = TEXT("Task|Spawn");
	const TCHAR* ConditionCategory = TEXT("Condition");
	const TCHAR* PresentationCategory = TEXT("Presentation");
}

UYogFlowNode_SkillPass::UYogFlowNode_SkillPass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SkillCategory;
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UYogFlowNode_SkillPass::ExecuteInput(const FName& PinName)
{
	TriggerOutput(TEXT("Out"), true);
}

UYogFlowNode_Fork::UYogFlowNode_Fork(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Skill|Flow");
#endif
}

UYogFlowNode_TriggerDamageDealt::UYogFlowNode_TriggerDamageDealt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_TriggerDamageReceived::UYogFlowNode_TriggerDamageReceived(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_TriggerCritHit::UYogFlowNode_TriggerCritHit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_TriggerKill::UYogFlowNode_TriggerKill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_TriggerDash::UYogFlowNode_TriggerDash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_TriggerGameplayEvent::UYogFlowNode_TriggerGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_EffectSendGameplayEvent::UYogFlowNode_EffectSendGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TriggerCategory;
#endif
}

UYogFlowNode_EffectDamage::UYogFlowNode_EffectDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = InstantEffectCategory;
#endif
}

UYogFlowNode_EffectHeal::UYogFlowNode_EffectHeal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = InstantEffectCategory;
#endif
	Value.Value = 10.f;
}

UYogFlowNode_EffectCost::UYogFlowNode_EffectCost(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = InstantEffectCategory;
#endif
	Value.Value = -1.f;
}

UYogFlowNode_EffectAttributeModify::UYogFlowNode_EffectAttributeModify(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = DurationEffectCategory;
#endif
}

UYogFlowNode_EffectApplyState::UYogFlowNode_EffectApplyState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = StateEffectCategory;
#endif
}

UYogFlowNode_EffectApplyProfile::UYogFlowNode_EffectApplyProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ProfileEffectCategory;
#endif
}

UYogFlowNode_EffectApplyInRadius::UYogFlowNode_EffectApplyInRadius(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = RadiusEffectCategory;
#endif
}

UYogFlowNode_EffectAreaDamage::UYogFlowNode_EffectAreaDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = RadiusEffectCategory;
#endif
}

UYogFlowNode_EffectAddTag::UYogFlowNode_EffectAddTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = StateEffectCategory;
#endif
}

UYogFlowNode_EffectRemoveTag::UYogFlowNode_EffectRemoveTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = StateEffectCategory;
#endif
}

UYogFlowNode_EffectGrantAbility::UYogFlowNode_EffectGrantAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ProfileEffectCategory;
#endif
}

UYogFlowNode_TaskSearchTarget::UYogFlowNode_TaskSearchTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TaskCategory;
#endif
}

UYogFlowNode_TaskEndSkill::UYogFlowNode_TaskEndSkill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TaskCategory;
#endif
}

UYogFlowNode_TaskPlayAnimation::UYogFlowNode_TaskPlayAnimation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TaskCategory;
#endif
}

UYogFlowNode_SpawnProjectileProfile::UYogFlowNode_SpawnProjectileProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SpawnCategory;
#endif
}

UYogFlowNode_SpawnAreaProfile::UYogFlowNode_SpawnAreaProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SpawnCategory;
#endif
}

UYogFlowNode_SpawnGroundPath::UYogFlowNode_SpawnGroundPath(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SpawnCategory;
#endif
}

UYogFlowNode_SpawnRangedProjectiles::UYogFlowNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SpawnCategory;
#endif
}

UYogFlowNode_SpawnBuffFlowProjectile::UYogFlowNode_SpawnBuffFlowProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = SpawnCategory;
#endif
}

UYogFlowNode_ConditionAttributeCompare::UYogFlowNode_ConditionAttributeCompare(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionCompareFloat::UYogFlowNode_ConditionCompareFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionCompareBool::UYogFlowNode_ConditionCompareBool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionHasTag::UYogFlowNode_ConditionHasTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionProbability::UYogFlowNode_ConditionProbability(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionDoOnce::UYogFlowNode_ConditionDoOnce(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_ConditionCheckDistance::UYogFlowNode_ConditionCheckDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = ConditionCategory;
#endif
}

UYogFlowNode_PresentationPlayVFX::UYogFlowNode_PresentationPlayVFX(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = PresentationCategory;
#endif
}

UYogFlowNode_PresentationCueOnActor::UYogFlowNode_PresentationCueOnActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = PresentationCategory;
#endif
}

UYogFlowNode_PresentationCueAtLocation::UYogFlowNode_PresentationCueAtLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = PresentationCategory;
#endif
}

UYogFlowNode_PresentationVFXProfile::UYogFlowNode_PresentationVFXProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = PresentationCategory;
#endif
}

UYogFlowNode_PresentationFlipbook::UYogFlowNode_PresentationFlipbook(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = PresentationCategory;
#endif
}

UYogFlowNode_LifecycleDelay::UYogFlowNode_LifecycleDelay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = LifecycleCategory;
#endif
}

UYogFlowNode_LifecycleFinishBuff::UYogFlowNode_LifecycleFinishBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = LifecycleCategory;
#endif
}

UYogFlowNode_RuneTuningValue::UYogFlowNode_RuneTuningValue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Pure");
#endif
}

UYogFlowNode_MathFloat::UYogFlowNode_MathFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Pure");
#endif
}

TArray<FString> UBFNode_Pure_TuningValue::GetPresetKeyNames()
{
	return {
		TEXT("Attack.Damage"),
		TEXT("Attack.Damage.01"), TEXT("Attack.Damage.02"), TEXT("Attack.Damage.03"),
		TEXT("Burn.Damage"),
		TEXT("Burn.Damage.01"), TEXT("Burn.Damage.02"), TEXT("Burn.Damage.03"),
		TEXT("Burn.Duration"),
		TEXT("Burn.Duration.01"), TEXT("Burn.Duration.02"),
		TEXT("Poison.Stack"),
		TEXT("Poison.Stack.01"), TEXT("Poison.Stack.02"),
		TEXT("Poison.Duration"),
		TEXT("Poison.Duration.01"), TEXT("Poison.Duration.02"),
		TEXT("Moonlight.ProjectileCount"),
		TEXT("Moonlight.ProjectileCount.01"), TEXT("Moonlight.ProjectileCount.02"),
		TEXT("Moonlight.ProjectileSpeed"),
		TEXT("Moonlight.ProjectileSpeed.01"),
		TEXT("Finisher.Damage"),
		TEXT("Finisher.Damage.01"), TEXT("Finisher.Damage.02"),
		TEXT("Finisher.AOERadius"),
		TEXT("Finisher.AOERadius.01"),
		TEXT("DetonationDamage"),
		TEXT("KnockbackDistance"),
	};
}

UBFNode_Pure_TuningValue::UBFNode_Pure_TuningValue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Pure");
#endif
	OutputPins = { FFlowPin(FName("Value"), EFlowPinType::Float) };
}

FFlowDataPinResult_Float UBFNode_Pure_TuningValue::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
{
	if (PinName == FName("Value"))
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			return FFlowDataPinResult_Float(BFC->GetRuneTuningValueForFlow(GetFlowAsset(), GetActiveKey(), DefaultValue));
		}
		return FFlowDataPinResult_Float(DefaultValue);
	}
	return FFlowDataPinResult_Float();
}

UBFNode_Pure_ComboIndex::UBFNode_Pure_ComboIndex(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Pure");
#endif
	OutputPins = { FFlowPin(FName("ComboIndex"), EFlowPinType::Int) };
}

FFlowDataPinResult_Int UBFNode_Pure_ComboIndex::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
{
	if (PinName == FName("ComboIndex"))
	{
		if (APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetBuffOwner()))
		{
			if (PC->ComboRuntimeComponent)
			{
				return FFlowDataPinResult_Int(PC->ComboRuntimeComponent->GetComboIndex());
			}
		}
		return FFlowDataPinResult_Int(1);
	}
	return FFlowDataPinResult_Int();
}
