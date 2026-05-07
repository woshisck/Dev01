// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/CharacterData.h"

const FMovementData* UCharacterData::GetMovementData() const
{
	const FMovementData* pMovementCharacterData = nullptr;
	if (!MovementDataRow.IsNull())
	{
		pMovementCharacterData = MovementDataRow.GetRow<FMovementData>(TEXT("UCharacterData::GetMovementData"));
	}

	return pMovementCharacterData;

}

const FYogBaseAttributeData* UCharacterData::GetBaseAttributeData() const
{
	const FYogBaseAttributeData* pCharacterStatsData = nullptr;
	if (!YogBaseAttributeDataRow.IsNull())
	{
		pCharacterStatsData = YogBaseAttributeDataRow.GetRow<FYogBaseAttributeData>(TEXT("UCharacterData::GetBaseAttributeData"));
	}

	return pCharacterStatsData;

}


const UAbilityData* UCharacterData::GetAbilityData() const
{
	const UAbilityData* pAbilityData = nullptr;
	if (AbilityData)
	{
		pAbilityData = AbilityData;
	}

	return pAbilityData;
}

const UGASTemplate* UCharacterData::GetGASTemplate() const
{
	const UGASTemplate* pGASTemplate = nullptr;
	if (GasTemplate)
	{
		pGASTemplate = GasTemplate;
	}

	return pGASTemplate;
}

const TArray<TSubclassOf<UAnimInstance>> UCharacterData::GetDefaultAnimeLayers() const
{
	const TArray<TSubclassOf<UAnimInstance>> result_array;
	if (DefaultAnimeLayers.Num() > 0)
	{
		return DefaultAnimeLayers;
	}

	return result_array;
}

// ── 细粒度访问器实现 ─────────────────────────────────────────
// 行未配置时回退到 FYogBaseAttributeData 的字段默认值（与构造器一致）。

namespace
{
	const FYogBaseAttributeData& ResolveAttrs(const UCharacterData* Self)
	{
		const FYogBaseAttributeData* Row = Self ? Self->GetBaseAttributeData() : nullptr;
		return Row ? *Row : UCharacterData::DefaultCharacterData;
	}
}

float UCharacterData::GetMaxHealth() const  { return ResolveAttrs(this).MaxHealth; }
float UCharacterData::GetMaxHeat() const    { return ResolveAttrs(this).MaxHeat; }
float UCharacterData::GetShield() const     { return ResolveAttrs(this).Shield; }
float UCharacterData::GetAttack() const     { return ResolveAttrs(this).Attack; }
float UCharacterData::GetAttackPower() const{ return ResolveAttrs(this).AttackPower; }
float UCharacterData::GetAttackSpeed() const{ return ResolveAttrs(this).AttackSpeed; }
float UCharacterData::GetAttackRange() const{ return ResolveAttrs(this).AttackRange; }
float UCharacterData::GetMoveSpeed() const  { return ResolveAttrs(this).MoveSpeed; }
float UCharacterData::GetCritRate() const   { return ResolveAttrs(this).Crit_Rate; }
float UCharacterData::GetCritDamage() const { return ResolveAttrs(this).Crit_Damage; }
float UCharacterData::GetMaxArmorHP() const { return ResolveAttrs(this).MaxArmorHP; }
float UCharacterData::GetDodge() const      { return ResolveAttrs(this).Dodge; }
float UCharacterData::GetResilience() const { return ResolveAttrs(this).Resilience; }
float UCharacterData::GetResist() const     { return ResolveAttrs(this).Resist; }
float UCharacterData::GetDmgTaken() const   { return ResolveAttrs(this).DmgTaken; }
float UCharacterData::GetSanity() const     { return ResolveAttrs(this).Sanity; }