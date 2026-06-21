# ComboGraph Reference Cleanup

- Mode: `dry-run`

## Migrated Data Assets
- Would resave `/Game/Code/Weapon/UnArmed/DA_WPN_UnArm.DA_WPN_UnArm` (drop transient legacy weapon ComboGraph fields after compile).
- Would resave `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword.DA_WPN_THSword` (drop transient legacy weapon ComboGraph fields after compile).
- Would resave `/Game/Code/Weapon/TwoHandedSword/DA_Weapon_FirstRun_DemoSword.DA_Weapon_FirstRun_DemoSword` (drop transient legacy weapon ComboGraph fields after compile).
- Would resave `/Game/Code/Weapon/GreatSword/DA_WPN_RedSword.DA_WPN_RedSword` (drop transient legacy weapon ComboGraph fields after compile).
- Would resave `/Game/Developers/sunchuankai/ComboGraph/DA_BlackSword.DA_BlackSword` (drop transient legacy weapon ComboGraph fields after compile).

## Player Blueprints
- Would compile/resave Blueprint `/Game/Code/Characters/B_PlayerOne.B_PlayerOne`.
- Would compile/resave Blueprint `/Game/Developers/sunchuankai/LootTest/DebugPlayer.DebugPlayer`.

## Remaining binary reference strings
- `/Game/Code/Characters/B_PlayerOne.B_PlayerOne`: ComboRuntimeComponent
- `/Game/Code/Weapon/Disarm/GA_ComboGraph_Disarm.GA_ComboGraph_Disarm`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Code/Weapon/GreatSword/DA_WPN_RedSword.DA_WPN_RedSword`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Code/Weapon/TwoHandedSword/DA_Weapon_FirstRun_DemoSword.DA_Weapon_FirstRun_DemoSword`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword.DA_WPN_THSword`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Code/Weapon/UnArmed/CG_UnArm.CG_UnArm`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Code/Weapon/UnArmed/DA_WPN_UnArm.DA_WPN_UnArm`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test.CG_THSword_Test`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Docs/Combat/UnArm/CG_UnArm.CG_UnArm`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Docs/WeaponSkill/WS_Smash.WS_Smash`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Developers/sunchuankai/ComboGraph/CG_UltraSword.CG_UltraSword`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Developers/sunchuankai/ComboGraph/DA_BlackSword.DA_BlackSword`: GameplayAbilityComboGraph, YogComboGraph
- `/Game/Developers/sunchuankai/LootTest/DebugPlayer.DebugPlayer`: ComboRuntimeComponent
- Notes: actual ComboGraph assets and intentionally archived developer assets may remain. Player Blueprints should be opened/resaved if `ComboRuntimeComponent` still appears here.
