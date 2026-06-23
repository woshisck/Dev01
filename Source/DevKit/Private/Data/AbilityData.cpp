// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/AbilityData.h"
#include "Data/MontageConfigDA.h"
#include "GameplayTagsManager.h"

UAnimMontage* UAbilityData::GetMontage(const FGameplayTag& Key) const
{
	if (UMontageConfigDA* Config = GetMontageConfig(Key, FGameplayTagContainer()))
	{
		return Config->Montage;
	}

	TObjectPtr<UAnimMontage> const* Found = MontageMap.Find(Key);
	return Found ? Found->Get() : nullptr;
}

UMontageConfigDA* UAbilityData::GetMontageConfig(const FGameplayTag& Key, const FGameplayTagContainer& ContextTags) const
{
	const FAbilityMontageConfigList* ConfigList = MontageConfigMap.Find(Key);
	if (!ConfigList)
	{
		return nullptr;
	}

	const FTaggedMontageConfig* Best = nullptr;
	for (const FTaggedMontageConfig& Candidate : ConfigList->Configs)
	{
		if (!Candidate.Matches(ContextTags))
		{
			continue;
		}

		if (!Best || Candidate.Priority > Best->Priority)
		{
			Best = &Candidate;
		}
	}

	return Best ? Best->MontageConfig.Get() : nullptr;
}

bool UAbilityData::HasAbility(const FGameplayTag& Key) const
{
	TObjectPtr<UAnimMontage> const* Found = MontageMap.Find(Key);
	if (Found && Found->Get() != nullptr)
	{
		return true;
	}

	const FAbilityMontageConfigList* ConfigList = MontageConfigMap.Find(Key);
	if (!ConfigList)
	{
		return false;
	}

	for (const FTaggedMontageConfig& Candidate : ConfigList->Configs)
	{
		if (Candidate.MontageConfig)
		{
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------
// 公用填充函数
// ---------------------------------------------------------------
static void AddDefaultKeys(
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>>& MontageMap,
	TMap<FGameplayTag, FPassiveActionData>& PassiveMap,
	TArrayView<const FName> MontageKeys,
	TArrayView<const FName> PassiveKeys)
{
	for (const FName& TagName : MontageKeys)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, /*bErrorIfNotFound=*/false);
		if (Tag.IsValid() && !MontageMap.Contains(Tag))
			MontageMap.Add(Tag, nullptr);
	}
	for (const FName& TagName : PassiveKeys)
	{
		FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (Tag.IsValid() && !PassiveMap.Contains(Tag))
			PassiveMap.Add(Tag, FPassiveActionData());
	}
}

static void AddWeaponAttackDefaultKeys(
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>>& MontageMap,
	TMap<FGameplayTag, FPassiveActionData>& PassiveMap)
{
	static const FName MontageKeys[] = {
		"PlayerState.AbilityCast.Attack.Combo1", "PlayerState.AbilityCast.Attack.Combo2",
		"PlayerState.AbilityCast.Attack.Combo3", "PlayerState.AbilityCast.Attack.Combo4",
		"PlayerState.AbilityCast.Dash.Combo1", "PlayerState.AbilityCast.Dash.Combo2",
		"PlayerState.AbilityCast.Dash.Combo3", "PlayerState.AbilityCast.Dash.Combo4",
		"PlayerState.AbilityCast.Dash",
		"PlayerState.AbilityCast.Dash.Dash1",
		"PlayerState.AbilityCast.Dash.DashATK1",
		"PlayerState.AbilityCast.DashAtk",
		"PlayerState.AbilityCast.SwitchWeapon",
	};
	AddDefaultKeys(MontageMap, PassiveMap, MontageKeys, TArrayView<const FName>());
}

// ---------------------------------------------------------------
// UEnemyAbilityMontageData
// ---------------------------------------------------------------
void UEnemyAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	static const FName MontageKeys[] = {
		"Enemy.Melee.LAtk1", "Enemy.Melee.LAtk2", "Enemy.Melee.LAtk3", "Enemy.Melee.LAtk4",
		"Enemy.Melee.HAtk1", "Enemy.Melee.HAtk2", "Enemy.Melee.HAtk3", "Enemy.Melee.HAtk4",
		"Enemy.Range.LAtk1", "Enemy.Range.LAtk2", "Enemy.Range.LAtk3", "Enemy.Range.LAtk4",
		"Enemy.Range.HAtk1", "Enemy.Range.HAtk2", "Enemy.Range.HAtk3", "Enemy.Range.HAtk4",
		"Enemy.Skill.Skill1", "Enemy.Skill.Skill2", "Enemy.Skill.Skill3", "Enemy.Skill.Skill4",
	};
	static const FName PassiveKeys[] = {
		"Action.HitReact.Front", "Action.Dead", "Action.HitReact.Back",
		"Action.HitReact.Blocked", "Action.HitReact.Parried",
	};

	AddDefaultKeys(MontageMap, PassiveMap, MontageKeys, PassiveKeys);
}

// ---------------------------------------------------------------
// UPlayerAbilityMontageData
// ---------------------------------------------------------------
void UPlayerAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	static const FName MontageKeys[] = {
		"PlayerState.AbilityCast.Attack.Combo1", "PlayerState.AbilityCast.Attack.Combo2",
		"PlayerState.AbilityCast.Attack.Combo3", "PlayerState.AbilityCast.Attack.Combo4",
		"PlayerState.AbilityCast.WeaponSkill.Combo1", "PlayerState.AbilityCast.WeaponSkill.Combo2",
		"PlayerState.AbilityCast.WeaponSkill.Combo3", "PlayerState.AbilityCast.WeaponSkill.Combo4",
		"PlayerState.AbilityCast.Dash.Combo1", "PlayerState.AbilityCast.Dash.Combo2",
		"PlayerState.AbilityCast.Dash.Combo3", "PlayerState.AbilityCast.Dash.Combo4",
		"PlayerState.AbilityCast.Dash",
		"PlayerState.AbilityCast.Dash.Dash1",
		"PlayerState.AbilityCast.Dash.DashATK1",
		"PlayerState.AbilityCast.DashAtk",
		"PlayerState.AbilityCast.Skill.Skill1",
		"PlayerState.AbilityCast.Skill.Skill2",
	};
	static const FName PassiveKeys[] = {
		"Action.HitReact.Front", "Action.Dead", "Action.HitReact.Back",
		"Action.HitReact.Blocked", "Action.HitReact.Parried",
	};

	AddDefaultKeys(MontageMap, PassiveMap, MontageKeys, PassiveKeys);
}

// ---------------------------------------------------------------
// UWeaponAttackAbilityMontageData
// ---------------------------------------------------------------
void UWeaponAttackAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	AddWeaponAttackDefaultKeys(MontageMap, PassiveMap);
}

void UWeaponAttackAbilityMontageData::PostLoad()
{
	Super::PostLoad();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	AddWeaponAttackDefaultKeys(MontageMap, PassiveMap);
}

// ---------------------------------------------------------------
// UWeaponSkillAbilityMontageData
// ---------------------------------------------------------------
void UWeaponSkillAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	static const FName MontageKeys[] = {
		"PlayerState.AbilityCast.WeaponSkill.Combo1", "PlayerState.AbilityCast.WeaponSkill.Combo2",
		"PlayerState.AbilityCast.WeaponSkill.Combo3", "PlayerState.AbilityCast.WeaponSkill.Combo4",
	};
	AddDefaultKeys(MontageMap, PassiveMap, MontageKeys, TArrayView<const FName>());
}

// ---------------------------------------------------------------
// USpecialAbilityMontageData
// ---------------------------------------------------------------
void USpecialAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;
}

// ---------------------------------------------------------------
// UWeaponPassiveAbilityMontageData
// ---------------------------------------------------------------
void UWeaponPassiveAbilityMontageData::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject)) return;

	static const FName PassiveKeys[] = {
		"Action.HitReact.Front", "Action.HitReact.Back",
		"Action.HitReact.Blocked", "Action.HitReact.Parried",
		"Action.Dead",
	};
	AddDefaultKeys(MontageMap, PassiveMap, TArrayView<const FName>(), PassiveKeys);
}

//const FActionData& FAbilityType::GetAction() const
//{
//	if (!ActionRow.IsNull())
//	{
//		FActionData* actionData = ActionRow.GetRow<FActionData>(__func__);
//		if (actionData)
//		{
//			return *actionData;
//		}
//	}
//	return DefaultActionData;
//}
