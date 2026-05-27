// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayEffect/GE_MeleeAttackFrame.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

UGE_MeleeAttackFrame::UGE_MeleeAttackFrame()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
	static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
	static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
	static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

	auto AddMod = [this](FGameplayAttribute Attr, EGameplayModOp::Type Op, FGameplayTag Tag)
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = Attr;
		Mod.ModifierOp = Op;
		FSetByCallerFloat SBC;
		SBC.DataTag = Tag;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
		Modifiers.Add(Mod);
	};

	AddMod(UBaseAttributeSet::GetAttackAttribute(), EGameplayModOp::Additive, TAG_ActDamage);
	AddMod(UBaseAttributeSet::GetAttackRangeAttribute(), EGameplayModOp::Override,  TAG_ActRange);
	AddMod(UBaseAttributeSet::GetResilienceAttribute(),  EGameplayModOp::Override,  TAG_ActRes);
	AddMod(UBaseAttributeSet::GetResistAttribute(),      EGameplayModOp::Override,  TAG_ActDmgReduce);
}
