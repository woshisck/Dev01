#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "GA_EnemyWeaponSkills.generated.h"

/**
 * Enemy weapon skill GAs.
 * AbilityTags / AbilityData Key: Enemy.Skill.Skill1~4
 * ActivationBlockedTags: Buff.Status.Dead
 *
 * Same pattern as the enemy melee attacks (GA_EnemyMeleeAttacks): a thin
 * UGA_MeleeAttack subclass per skill slot, differing only by tag. The montage
 * and hitbox auto-resolve from CharacterData.AbilityData.AbilityMap[AbilityTags[0]],
 * so the weapon definition supplies them by adding an Enemy.Skill.SkillN row.
 * Grant the slots a given enemy uses through its GASTemplate.AbilityMap.
 */

UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_Skill1 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_Skill1(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_Skill2 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_Skill2(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_Skill3 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_Skill3(); };
UCLASS(BlueprintType, Blueprintable) class DEVKIT_API UGA_Enemy_Skill4 : public UGA_MeleeAttack { GENERATED_BODY() public: UGA_Enemy_Skill4(); };
