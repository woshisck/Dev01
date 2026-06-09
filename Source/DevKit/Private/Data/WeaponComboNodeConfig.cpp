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

	ECombatDeckActionSlot ActionSlotTagToEnum(const FGameplayTag& Tag, ECombatDeckActionSlot DefaultActionSlot)
	{
		if (!Tag.IsValid())
		{
			return DefaultActionSlot;
		}

		const FName TagName = Tag.GetTagName();
		if (TagName == TEXT("Combo.CombatDeck.ActionSlot.Attack"))
		{
			return ECombatDeckActionSlot::Attack;
		}
		if (TagName == TEXT("Combo.CombatDeck.ActionSlot.Skill"))
		{
			return ECombatDeckActionSlot::Skill;
		}
		if (TagName == TEXT("Combo.CombatDeck.ActionSlot.WeaponSkill"))
		{
			return ECombatDeckActionSlot::WeaponSkill;
		}
		if (TagName == TEXT("Combo.CombatDeck.ActionSlot.Dash"))
		{
			return ECombatDeckActionSlot::Dash;
		}
		return DefaultActionSlot;
	}

	ECombatDeckFlowRole FlowRoleTagToEnum(const FGameplayTag& Tag, ECombatDeckFlowRole DefaultFlowRole)
	{
		if (!Tag.IsValid())
		{
			return DefaultFlowRole;
		}

		const FName TagName = Tag.GetTagName();
		if (TagName == TEXT("Combo.CombatDeck.FlowRole.Starter"))
		{
			return ECombatDeckFlowRole::Starter;
		}
		if (TagName == TEXT("Combo.CombatDeck.FlowRole.Catalyst"))
		{
			return ECombatDeckFlowRole::Catalyst;
		}
		if (TagName == TEXT("Combo.CombatDeck.FlowRole.Finisher"))
		{
			return ECombatDeckFlowRole::Finisher;
		}
		return DefaultFlowRole;
	}
}

FWeaponComboNodeConfig FWeaponComboNodeConfig::FromComboGraphNode(
	const UGameplayAbilityComboGraphNode* Node,
	ECardRequiredAction InputAction,
	ECombatDeckActionSlot DefaultActionSlot,
	ECombatDeckFlowRole DefaultFlowRole)
{
	FWeaponComboNodeConfig Config;
	if (!Node)
	{
		return Config;
	}

	Config.NodeId = !Node->NodeId.IsNone() ? Node->NodeId : FName(*Node->GetName());
	Config.InputAction = InputAction;
	Config.AttackType = Node->AttackType;
	Config.Montage = Node->Montage;
	Config.CombatDeckActionSlot = ActionSlotTagToEnum(Node->CombatDeckActionSlotTag, DefaultActionSlot);
	Config.CombatDeckFlowRole = FlowRoleTagToEnum(
		Node->CombatDeckFlowRoleTag,
		Node->bIsComboFinisher ? ECombatDeckFlowRole::Finisher : DefaultFlowRole);
	Config.bIsComboFinisher = Node->bIsComboFinisher || Config.CombatDeckFlowRole == ECombatDeckFlowRole::Finisher;
	Config.bOverrideComboWindow = Node->bUseNodeComboWindow;
	Config.ComboWindowStartFrame = Node->ComboWindowStartFrame;
	Config.ComboWindowEndFrame = Node->ComboWindowEndFrame;
	Config.ComboWindowTotalFrames = Node->TotalFrames > 0 ? Node->TotalFrames : 30;
	Config.CardTriggerTiming = TriggerTimingTagToEnum(Node->TriggerTimingTag);
	return Config;
}
