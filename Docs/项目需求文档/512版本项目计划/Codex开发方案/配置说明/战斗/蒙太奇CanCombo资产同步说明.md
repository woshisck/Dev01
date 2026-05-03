# 蒙太奇 CanCombo 资产同步说明

## 作用

把武器连招配置里的可接续节点同步到实际 `AnimMontage` 的 `ANS_AddGameplayTag`，避免漏配、重复配置或终结击残留 `CanCombo`。

## 当前规则

- 运行时仍以实际 `AnimMontage` 上的 `ANS_AddGameplayTag` 作为 `CanCombo` 开窗来源。
- 非终结节点会写入 `PlayerState.AbilityCast.CanCombo`。
- 终结节点不会写入 `CanCombo`；如果原蒙太奇已有旧 `CanCombo`，同步时会移除。
- 开窗帧优先读取 `MontageConfigDA.Entries.Combo Window`，没有时使用 Graph/节点窗口字段。

## 使用方式

关闭编辑器后运行：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=CombatMontageSync -Weapon=/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword -Apply -unattended -nop4
```

运行后检查：

- 报告：`Saved/CombatMontageSyncReport.md`
- 资产：相关 `AnimMontage` 的 `Combo` Notify Track

## 配置检查

1. 每个需要接下一击的 `MontageConfigDA` 配一个 `Combo Window`。
2. 终结击节点勾选 `bIsComboFinisher`。
3. 同一个蒙太奇不要同时给“可接续节点”和“终结节点”复用；如果复用，蒙太奇级 `CanCombo` 会影响所有分支。
