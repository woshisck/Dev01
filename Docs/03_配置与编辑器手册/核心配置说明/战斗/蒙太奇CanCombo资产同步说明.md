# 蒙太奇 CanCombo 资产同步说明

## 作用

旧工具用于把武器连招配置里的可接续节点同步到实际 `AnimMontage` 的 `ANS_AddGameplayTag`。

当前 512 临时简化后，不推荐继续依赖该工具批量写入开窗。新内容优先由策划/动画直接在攻击蒙太奇上放 `Combo Window` NotifyState。

## 当前规则

- 运行时以实际 `AnimMontage` 上的 `Combo Window` NotifyState 作为连招开窗来源。
- `GameplayAbilityComboGraph` 节点窗口帧只作为展示数据，不驱动运行时。
- 旧 `ANS_AddGameplayTag` + `PlayerState.AbilityCast.CanCombo` 仍兼容，用于旧资产过渡。
- BlockTag、Combo1/2/3、DashSaveTag 等系统 Tag 仍由运行时自动处理，不需要策划或美术配置。

## 使用方式

如需维护旧资产，可关闭编辑器后运行：

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=CombatMontageSync -Weapon=/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword -Apply -unattended -nop4
```

运行后检查：

- 报告：`Saved/CombatMontageSyncReport.md`
- 资产：相关 `AnimMontage` 的 `Combo` Notify Track

## 配置检查

1. 每个需要接下一击的攻击 `AnimMontage` 放一个 `Combo Window` NotifyState。
2. 终结击通常不放 `Combo Window` NotifyState。
3. 同一个蒙太奇不要同时给“可接续节点”和“终结节点”复用；如果复用，蒙太奇级开窗会影响所有分支。
