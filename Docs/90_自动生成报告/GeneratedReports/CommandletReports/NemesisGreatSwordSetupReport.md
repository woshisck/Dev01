# Nemesis GreatSword Setup

## Assets
- EnemyData: `/Game/Docs/Data/Enemy/Rat/DA_Nemesis.DA_Nemesis`
- WeaponDefinition: `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword.DA_Nemesis_GreatSword`
- AbilityData: `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword_Ability.DA_Nemesis_GreatSword_Ability` (created `False`)
- GASTemplate: `/Game/Docs/Data/Enemy/DA_Nemesis_GAS.DA_Nemesis_GAS`

## Montage Rows
- `Enemy.Melee.LAtk1` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_01_L1_Combo`
- `Enemy.Melee.LAtk2` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_02_L2_Combo`
- `Enemy.Melee.LAtk3` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_03_L3_Finisher`
- `Enemy.Skill.Skill1` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_H1_Combo`
- `Enemy.Skill.Skill2` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_L2H_Finisher`
- `Enemy.Skill.Skill3` -> `/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_02_H2_Finisher`

## Weapon Attack Profile
- `Enemy.Melee.LAtk1` role `<EnemyAIAttackRole.CLOSE_MELEE: 0>` range `0-280` weight `3.00` cooldown `1.00`
- `Enemy.Melee.LAtk2` role `<EnemyAIAttackRole.CLOSE_MELEE: 0>` range `0-300` weight `2.50` cooldown `1.15`
- `Enemy.Melee.LAtk3` role `<EnemyAIAttackRole.CLOSE_MELEE: 0>` range `0-320` weight `2.00` cooldown `1.30`
- `Enemy.Skill.Skill1` role `<EnemyAIAttackRole.SKILL: 2>` range `0-420` weight `1.20` cooldown `7.00`
- `Enemy.Skill.Skill2` role `<EnemyAIAttackRole.SKILL: 2>` range `0-480` weight `1.00` cooldown `10.00`
- `Enemy.Skill.Skill3` role `<EnemyAIAttackRole.SKILL: 2>` range `0-540` weight `0.80` cooldown `13.00`

## GAS AbilityMap
- Changed: `False`
- Previous: `GA_Enemy_LAtk1, GA_Enemy_LAtk2, GA_Enemy_LAtk3, GA_Enemy_Skill1, GA_Enemy_Skill2, GA_Enemy_Skill3`
- Current: `GA_Enemy_LAtk1, GA_Enemy_LAtk2, GA_Enemy_LAtk3, GA_Enemy_Skill1, GA_Enemy_Skill2, GA_Enemy_Skill3`

## Enemy Data
- `default_weapon_definition` -> `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword.DA_Nemesis_GreatSword`
- `gas_template` -> `/Game/Docs/Data/Enemy/DA_Nemesis_GAS.DA_Nemesis_GAS`
