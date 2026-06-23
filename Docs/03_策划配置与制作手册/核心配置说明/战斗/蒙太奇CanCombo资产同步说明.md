# 蒙太奇 CanCombo 资产同步说明

## 作用

旧工具用于把武器连招配置里的可接续节点同步到实际 `AnimMontage` 的 `ANS_AddGameplayTag`。

当前玩家动作不再依赖轻/重攻击续段连招；新内容不要继续依赖该工具批量写入开窗。本工具仅用于旧资产维护或迁移对照。

## 当前规则

- 当前新武器以 `AttackAbilityData` / `WeaponSkillAbilityData` 和 `MontageConfigMap` 为主。
- 旧 `Combo Window` / `ANS_AddGameplayTag` + `PlayerState.AbilityCast.CanCombo` 仍兼容，用于旧资产过渡。
- 旧 Combo1/2/3/4 Tag 只作为旧 GA / 旧蒙太奇选择兼容，不再作为卡牌构筑条件。

## 使用方式

如需维护旧资产，可关闭编辑器后运行：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=CombatMontageSync -Weapon=/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword -Apply -unattended -nop4
```

运行后检查：

- 报告：`Saved/CombatMontageSyncReport.md`
- 资产：相关 `AnimMontage` 的 `Combo` Notify Track

## 配置检查

1. 只在维护旧 ComboGraph 资产时检查 `Combo Window` NotifyState。
2. 新武器不要为了卡牌连携新增 `CanCombo` 开窗。
3. 同一个蒙太奇如果仍被旧可接续节点和旧终结节点复用，需在迁移时拆到当前 AbilityData / MontageConfigMap。
