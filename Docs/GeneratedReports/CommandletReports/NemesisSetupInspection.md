# Nemesis Setup Inspection

## `/Game/Docs/Data/Enemy/Rat/DA_Nemesis`
- Asset: `/Game/Docs/Data/Enemy/Rat/DA_Nemesis.DA_Nemesis`
- `enemy_class` = `<Object '/Game/Code/Enemy/Minion/Nemesis/BP_Enemy_Nemesis.BP_Enemy_Nemesis_C' (0x0000074B3D4AB600) Class 'BlueprintGeneratedClass'>`
- `gas_template` = `<Object '/Game/Docs/Data/Enemy/DA_Nemesis_GAS.DA_Nemesis_GAS' (0x0000074B413D4B00) Class 'GASTemplate'>`
- `ability_data` = `None`
- `default_weapon_definition` = `<Object '/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword.DA_Nemesis_GreatSword' (0x0000074B41156E00) Class 'EnemyWeaponDefinition'>`
- `allowed_weapon_definitions` = `["/Script/DevKit.EnemyWeaponDefinition'/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword.DA_Nemesis_GreatSword'"]`
- `attack_profile` = `<Struct 'EnemyAIAttackProfile' (0x0000074B4079AF40) {attacks: , recent_attack_memory_duration: 2.000000, repeat_attack_weight_multiplier: 0.250000}>`

## `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword`
- Asset: `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword.DA_Nemesis_GreatSword`
- `ability_data` = `<Object '/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword_Ability.DA_Nemesis_GreatSword_Ability' (0x0000074B41105500) Class 'EnemyAbilityMontageData'>`
- `attack_profile` = `<Struct 'EnemyAIAttackProfile' (0x0000074B41156E58) {attacks: ((AttackName="Enemy_Melee_LAtk1",AbilityTags=(GameplayTags=((TagName="Enemy.Melee.LAtk1")),ParentTags=((TagName="Enemy.Melee"),(TagName="Enemy"))),MaxRange=280.000000,Weight=3.000000),(AttackName="Enemy_Melee_LAtk2",AbilityTags=(GameplayTags=((TagName="Enemy.Melee.LAtk2")),ParentTags=((TagName="Enemy.Melee"),(TagName="Enemy"))),Weight=2.500000,Cooldown=1.150000),(AttackName="Enemy_Melee_LAtk3",AbilityTags=(GameplayTags=((TagName="Enemy.Melee.LAtk3")),ParentTags=((TagName="Enemy.Melee"),(TagName="Enemy"))),MaxRange=320.000000,Weight=2.000000,Cooldown=1.300000),(AttackName="Enemy_Skill_Skill1",AbilityTags=(GameplayTags=((TagName="Enemy.Skill.Skill1")),ParentTags=((TagName="Enemy.Skill"),(TagName="Enemy"))),MaxRange=420.000000,Weight=1.200000,Cooldown=7.000000,AttackRole=Skill),(AttackName="Enemy_Skill_Skill2",AbilityTags=(GameplayTags=((TagName="Enemy.Skill.Skill2")),ParentTags=((TagName="Enemy.Skill"),(TagName="Enemy"))),MaxRange=480.000000,Cooldown=10.000000,AttackRole=Skill),(AttackName="Enemy_Skill_Skill3",AbilityTags=(GameplayTags=((TagName="Enemy.Skill.Skill3")),ParentTags=((TagName="Enemy.Skill"),(TagName="Enemy"))),MaxRange=540.000000,Weight=0.800000,Cooldown=13.000000,AttackRole=Skill)), recent_attack_memory_duration: 2.500000, repeat_attack_weight_multiplier: 0.350000}>`
- `actors_to_spawn` = `[{should_save_to_game: False}]`
- `anim_layers` = `[]`

## `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword_Ability`
- Asset: `/Game/Code/Enemy/Weapon/DA_Nemesis_GreatSword_Ability.DA_Nemesis_GreatSword_Ability`
- `montage_map` = `{{}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_01_L1_Combo.AM_Sword_LATK_01_L1_Combo'", {}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_02_L2_Combo.AM_Sword_LATK_02_L2_Combo'", {}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_LATK_03_L3_Finisher.AM_Sword_LATK_03_L3_Finisher'", {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: None, {}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_H1_Combo.AM_Sword_HATK_01_H1_Combo'", {}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_01_L2H_Finisher.AM_Sword_HATK_01_L2H_Finisher'", {}: "/Script/Engine.AnimMontage'/Game/Code/Weapon/TwoHandedSword/GeneratedMontages/AM_Sword_HATK_02_H2_Finisher.AM_Sword_HATK_02_H2_Finisher'", {}: None}`
- `montage_config_map` = `{}`
- `passive_map` = `{{}: {montage: None, unique_effects: , dissolve_gameplay_cue_tag: {}}, {}: {montage: None, unique_effects: , dissolve_gameplay_cue_tag: {}}, {}: {montage: None, unique_effects: , dissolve_gameplay_cue_tag: {}}, {}: {montage: None, unique_effects: , dissolve_gameplay_cue_tag: {}}, {}: {montage: None, unique_effects: , dissolve_gameplay_cue_tag: {}}}`

## `/Game/Code/Enemy/Weapon/DA_GreatSword_Ability`
- Asset: `/Game/Code/Enemy/Weapon/DA_GreatSword_Ability.DA_GreatSword_Ability`
- `montage_map` = `{{}: None}`
- `montage_config_map` = `{}`
- `passive_map` = `{}`

## `/Game/Docs/Data/Enemy/DA_Nemesis_GAS`
- Asset: `/Game/Docs/Data/Enemy/DA_Nemesis_GAS.DA_Nemesis_GAS`
- `ability_map` = `["/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_LAtk1'", "/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_LAtk2'", "/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_LAtk3'", "/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_Skill1'", "/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_Skill2'", "/Script/CoreUObject.Class'/Script/DevKit.GA_Enemy_Skill3'"]`
- `passive_map` = `[]`

