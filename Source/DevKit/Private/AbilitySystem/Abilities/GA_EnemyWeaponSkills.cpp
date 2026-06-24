#include "AbilitySystem/Abilities/GA_EnemyWeaponSkills.h"

namespace
{
	FGameplayTag EnemySkillDeadStatusTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Dead")));
	}
}

UGA_Enemy_Skill1::UGA_Enemy_Skill1()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Skill.Skill1")));
	ActivationBlockedTags.AddTag(EnemySkillDeadStatusTag());
}

UGA_Enemy_Skill2::UGA_Enemy_Skill2()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Skill.Skill2")));
	ActivationBlockedTags.AddTag(EnemySkillDeadStatusTag());
}

UGA_Enemy_Skill3::UGA_Enemy_Skill3()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Skill.Skill3")));
	ActivationBlockedTags.AddTag(EnemySkillDeadStatusTag());
}

UGA_Enemy_Skill4::UGA_Enemy_Skill4()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Enemy.Skill.Skill4")));
	ActivationBlockedTags.AddTag(EnemySkillDeadStatusTag());
}
