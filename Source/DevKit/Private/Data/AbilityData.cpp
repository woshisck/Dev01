// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/AbilityData.h"
#include "Data/MontageConfigDA.h"
#include "GameplayTagsManager.h"

namespace
{
FGameplayTag GetEquivalentPlayerActionTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return FGameplayTag();
	}

	const FString TagString = Tag.ToString();
	FString EquivalentTagString;
	if (TagString.StartsWith(TEXT("Character.State.Skill.Attack")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("Character.State.Skill.Attack"),
			TEXT("PlayerState.AbilityCast.Attack"));
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.Attack")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.Attack"),
			TEXT("Character.State.Skill.Attack"));
	}
	else if (TagString.StartsWith(TEXT("Character.State.Skill.WeaponSkill")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("Character.State.Skill.WeaponSkill"),
			TEXT("PlayerState.AbilityCast.WeaponSkill"));
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.WeaponSkill")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.WeaponSkill"),
			TEXT("Character.State.Skill.WeaponSkill"));
	}
	else if (TagString.StartsWith(TEXT("Character.State.Skill.Active")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("Character.State.Skill.Active"),
			TEXT("PlayerState.AbilityCast.Skill"));
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.Skill")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.Skill"),
			TEXT("Character.State.Skill.Active"));
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.LightAtk")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.LightAtk"),
			TEXT("Character.State.Skill.Attack"));
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.HeavyAtk")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.HeavyAtk"),
			TEXT("Character.State.Skill.WeaponSkill"));
	}
	else if (TagString == TEXT("Character.State.Skill.Reload"))
	{
		EquivalentTagString = TEXT("PlayerState.AbilityCast.Reload");
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.Reload"))
	{
		EquivalentTagString = TEXT("Character.State.Skill.Reload");
	}
	else if (TagString.StartsWith(TEXT("Character.State.Movement.Dash")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("Character.State.Movement.Dash"),
			TEXT("PlayerState.AbilityCast.Dash"));
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.Dash.Dash1")
		|| TagString == TEXT("PlayerState.AbilityCast.Dash.DashATK1")
		|| TagString == TEXT("PlayerState.AbilityCast.DashAtk"))
	{
		EquivalentTagString = TEXT("Character.State.Movement.Dash");
	}
	else if (TagString.StartsWith(TEXT("PlayerState.AbilityCast.Dash")))
	{
		EquivalentTagString = TagString.Replace(
			TEXT("PlayerState.AbilityCast.Dash"),
			TEXT("Character.State.Movement.Dash"));
	}
	else if (TagString == TEXT("Character.State.Equipment.SwitchWeapon"))
	{
		EquivalentTagString = TEXT("PlayerState.AbilityCast.SwitchWeapon");
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.SwitchWeapon"))
	{
		EquivalentTagString = TEXT("Character.State.Equipment.SwitchWeapon");
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.CanCombo"))
	{
		EquivalentTagString = TEXT("Character.State.Window.CanCombo");
	}
	else if (TagString == TEXT("Character.State.Window.PostAttackRecovery"))
	{
		EquivalentTagString = TEXT("PlayerState.AbilityCast.PostAttackRecovery");
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.PostAttackRecovery"))
	{
		EquivalentTagString = TEXT("Character.State.Window.PostAttackRecovery");
	}

	return EquivalentTagString.IsEmpty()
		? FGameplayTag()
		: FGameplayTag::RequestGameplayTag(FName(*EquivalentTagString), false);
}

void AddLookupCandidate(TArray<FGameplayTag>& OutCandidates, const FGameplayTag& Tag)
{
	if (Tag.IsValid())
	{
		OutCandidates.AddUnique(Tag);
	}
}

void AddLookupCandidateByName(TArray<FGameplayTag>& OutCandidates, const TCHAR* TagName)
{
	AddLookupCandidate(OutCandidates, FGameplayTag::RequestGameplayTag(FName(TagName), false));
}

TArray<FGameplayTag> BuildPlayerActionLookupCandidates(const FGameplayTag& Key)
{
	TArray<FGameplayTag> Candidates;
	AddLookupCandidate(Candidates, Key);
	AddLookupCandidate(Candidates, GetEquivalentPlayerActionTag(Key));

	const FString TagString = Key.ToString();
	if (TagString == TEXT("Character.State.Skill.Attack"))
	{
		AddLookupCandidateByName(Candidates, TEXT("Character.State.Skill.Attack.Combo1"));
		AddLookupCandidateByName(Candidates, TEXT("PlayerState.AbilityCast.Attack.Combo1"));
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.Attack"))
	{
		AddLookupCandidateByName(Candidates, TEXT("PlayerState.AbilityCast.Attack.Combo1"));
		AddLookupCandidateByName(Candidates, TEXT("Character.State.Skill.Attack.Combo1"));
	}
	else if (TagString == TEXT("Character.State.Skill.WeaponSkill"))
	{
		AddLookupCandidateByName(Candidates, TEXT("Character.State.Skill.WeaponSkill.Combo1"));
		AddLookupCandidateByName(Candidates, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"));
	}
	else if (TagString == TEXT("PlayerState.AbilityCast.WeaponSkill"))
	{
		AddLookupCandidateByName(Candidates, TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"));
		AddLookupCandidateByName(Candidates, TEXT("Character.State.Skill.WeaponSkill.Combo1"));
	}

	return Candidates;
}

bool HasUsableMontageConfigList(const FAbilityMontageConfigList& ConfigList)
{
	for (const FTaggedMontageConfig& Candidate : ConfigList.Configs)
	{
		if (Candidate.MontageConfig)
		{
			return true;
		}
	}
	return false;
}

const FAbilityMontageConfigList* FindMontageConfigListWithFallback(
	const TMap<FGameplayTag, FAbilityMontageConfigList>& MontageConfigMap,
	const FGameplayTag& Key)
{
	for (const FGameplayTag& CandidateKey : BuildPlayerActionLookupCandidates(Key))
	{
		const FAbilityMontageConfigList* ConfigList = MontageConfigMap.Find(CandidateKey);
		if (ConfigList && HasUsableMontageConfigList(*ConfigList))
		{
			return ConfigList;
		}
	}
	return nullptr;
}

TObjectPtr<UAnimMontage> const* FindMontageWithFallback(
	const TMap<FGameplayTag, TObjectPtr<UAnimMontage>>& MontageMap,
	const FGameplayTag& Key)
{
	for (const FGameplayTag& CandidateKey : BuildPlayerActionLookupCandidates(Key))
	{
		TObjectPtr<UAnimMontage> const* Found = MontageMap.Find(CandidateKey);
		if (Found && Found->Get())
		{
			return Found;
		}
	}
	return nullptr;
}
}

UAnimMontage* UAbilityData::GetMontage(const FGameplayTag& Key) const
{
	if (UMontageConfigDA* Config = GetMontageConfig(Key, FGameplayTagContainer()))
	{
		return Config->Montage;
	}

	TObjectPtr<UAnimMontage> const* Found = FindMontageWithFallback(MontageMap, Key);
	return Found ? Found->Get() : nullptr;
}

UMontageConfigDA* UAbilityData::GetMontageConfig(const FGameplayTag& Key, const FGameplayTagContainer& ContextTags) const
{
	for (const FGameplayTag& CandidateKey : BuildPlayerActionLookupCandidates(Key))
	{
		const FAbilityMontageConfigList* ConfigList = MontageConfigMap.Find(CandidateKey);
		if (!ConfigList)
		{
			continue;
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

		if (Best)
		{
			return Best->MontageConfig.Get();
		}
	}

	return nullptr;
}

bool UAbilityData::HasAbility(const FGameplayTag& Key) const
{
	TObjectPtr<UAnimMontage> const* Found = FindMontageWithFallback(MontageMap, Key);
	if (Found && Found->Get() != nullptr)
	{
		return true;
	}

	const FAbilityMontageConfigList* ConfigList = FindMontageConfigListWithFallback(MontageConfigMap, Key);
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
		"Character.State.Skill.Attack.Combo1", "Character.State.Skill.Attack.Combo2",
		"Character.State.Skill.Attack.Combo3", "Character.State.Skill.Attack.Combo4",
		"Character.State.Movement.Dash.Combo1", "Character.State.Movement.Dash.Combo2",
		"Character.State.Movement.Dash.Combo3", "Character.State.Movement.Dash.Combo4",
		"Character.State.Movement.Dash",
		"Character.State.Skill.Reload",
		"Character.State.Equipment.SwitchWeapon",
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
		"Character.State.Skill.Attack.Combo1", "Character.State.Skill.Attack.Combo2",
		"Character.State.Skill.Attack.Combo3", "Character.State.Skill.Attack.Combo4",
		"Character.State.Skill.WeaponSkill.Combo1", "Character.State.Skill.WeaponSkill.Combo2",
		"Character.State.Skill.WeaponSkill.Combo3", "Character.State.Skill.WeaponSkill.Combo4",
		"Character.State.Movement.Dash.Combo1", "Character.State.Movement.Dash.Combo2",
		"Character.State.Movement.Dash.Combo3", "Character.State.Movement.Dash.Combo4",
		"Character.State.Movement.Dash",
		"Character.State.Skill.Reload",
		"Character.State.Equipment.SwitchWeapon",
		"Character.State.Skill.Active.Skill1",
		"Character.State.Skill.Active.Skill2",
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
		"Character.State.Skill.WeaponSkill.Combo1", "Character.State.Skill.WeaponSkill.Combo2",
		"Character.State.Skill.WeaponSkill.Combo3", "Character.State.Skill.WeaponSkill.Combo4",
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
