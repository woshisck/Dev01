#include "Data/WeaponSkillDataAsset.h"

FGameplayTag UWeaponSkillDataAsset::GetResolvedSkillTag() const
{
	if (SkillTag.IsValid())
	{
		return SkillTag;
	}

	if (AbilityClass)
	{
		if (const UGA_WeaponSkill* AbilityCDO = AbilityClass->GetDefaultObject<UGA_WeaponSkill>())
		{
			return AbilityCDO->GetWeaponSkillTag();
		}
	}

	return FGameplayTag();
}
