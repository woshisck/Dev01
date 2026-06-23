# 旧连招输入缓存与 Notify 开窗说明

## 当前状态

当前玩家动作主流程不再依赖轻/重攻击续段连招。新内容使用 Attack / Skill / WeaponSkill / Dash 动作槽，卡牌顺序和 Link 条件负责构筑节奏。

本文件只用于旧连招资产排查。

## 旧兼容规则

- 旧连招窗口由实际 `AnimMontage` 上的 `Combo Window` NotifyState 控制。
- 旧 `ANS_AddGameplayTag` 仍兼容，但不推荐新内容继续手填 `PlayerState.AbilityCast.CanCombo`。
- 旧 `GameplayAbilityComboGraph` 节点上的 `ComboWindowStartFrame / EndFrame / TotalFrames` 只作为展示数据，不驱动当前主流程。
- 旧连招输入早于窗口时会进入 Buffer；`CanCombo` 打开后系统会尝试消费一次旧输入。

## 新内容检查

1. Attack 可以直接进入对应攻击 GA。
2. WeaponSkill 可以直接进入战技 GA。
3. Dash 可以触发 Dash 单槽卡。
4. 卡牌 Link 依赖卡牌顺序、CardIdTag 或 FlowRole，不依赖旧 Combo Window。

## 注意

- Graph 连招武器下，`PostAtkWindow` 不再负责攻击输入时清理 `CanCombo`，避免和输入缓存抢时序。
- 如果旧资产一直无法续段，优先检查旧蒙太奇是否真的有 `Combo Window` NotifyState。
- 新资产不要为了卡牌连携新增旧连招窗口。
