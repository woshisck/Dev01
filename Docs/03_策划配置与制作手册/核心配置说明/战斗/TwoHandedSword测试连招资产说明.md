# TwoHandedSword 测试连招资产说明

## 资产位置

- 测试武器 DA：`/Game/Docs/Combat/TwoHandedSword/DA_WPN_THSword_ComboGraphTest`
- 连招 Graph：`/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test`
- MontageConfig：`/Game/Docs/Combat/TwoHandedSword/MC_THSword_*`

## 当前连招

- `Light -> Light -> Light`：`L1 -> L2 -> L3`
- `Light -> Light -> Heavy`：`L1 -> L2 -> L2H`
- `Heavy -> Heavy`：`H1 -> H2`

`L3`、`L2H`、`H2` 已标记为 Finisher。所有节点暂用 `18-27 / 30` 帧作为 Combo Window。

## 使用方式

测试时使用 `DA_WPN_THSword_ComboGraphTest`，不要直接替换原始 `/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword`。确认无误后再把原武器 DA 的 `GameplayAbilityComboGraph` 指到 `CG_THSword_Test`。
