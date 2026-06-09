#include "Data/WeaponComboNodeConfig.h"

#include "Data/GameplayAbilityComboGraph.h"

namespace
{
	ECombatCardTriggerTiming TriggerTimingTagToEnum(const FGameplayTag& Tag)
	{
		if (Tag.IsValid())
		{
			static const FName OnHitName(TEXT("Combo.TriggerTiming.OnHit"));
			if (Tag.GetTagName() == OnHitName)
			{
				return ECombatCardTriggerTiming::OnHit;
			}
		}
		return ECombatCardTriggerTiming::OnCommit;
	}
}

FWeaponComboNodeConfig FWeaponComboNodeConfig::FromComboGraphNode(const UGameplayAbilityComboGraphNode* Node, ECardRequiredAction InputAction)
{
	FWeaponComboNodeConfig Config;
	if (!Node)
	{
		return Config;
	}

	Config.NodeId = !Node->NodeId.IsNone() ? Node->NodeId : FName(*Node->GetName());
	Config.InputAction = InputAction;
	Config.CombatDeckActionSlot = ECombatDeckActionSlot::Attack;
	Config.AttackType = Node->AttackType;
	Config.Montage = Node->Montage;
	Config.bIsComboFinisher = Node->bIsComboFinisher;
	Config.CombatDeckFlowRole = Node->bIsComboFinisher ? ECombatDeckFlowRole::Finisher : ECombatDeckFlowRole::Starter;
	Config.bOverrideComboWindow = Node->bUseNodeComboWindow;
	Config.ComboWindowStartFrame = Node->ComboWindowStartFrame;
	Config.ComboWindowEndFrame = Node->ComboWindowEndFrame;
	Config.ComboWindowTotalFrames = Node->TotalFrames > 0 ? Node->TotalFrames : 30;
	Config.CardTriggerTiming = TriggerTimingTagToEnum(Node->TriggerTimingTag);
	return Config;
}
