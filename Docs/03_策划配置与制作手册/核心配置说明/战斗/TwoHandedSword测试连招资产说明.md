# TwoHandedSword 旧测试资产说明

## 当前状态

这些资产是旧 ComboGraph 连招测试资产，仅用于回归旧资产加载和迁移对照。当前新武器不要复制这套配置方式。

## 资产位置

- 测试武器 DA：`/Game/Docs/Combat/TwoHandedSword/DA_WPN_THSword_ComboGraphTest`
- 旧 Graph：`/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test`
- MontageConfig：`/Game/Docs/Combat/TwoHandedSword/MC_THSword_*`

## 使用方式

- 只在排查旧 ComboGraph 兼容时使用。
- 不要把原始 `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword` 指回这套旧 Graph。
- 新双手剑动作应迁移到 `AttackAbilityData` / `WeaponSkillAbilityData` 和 `MontageConfigMap`。
