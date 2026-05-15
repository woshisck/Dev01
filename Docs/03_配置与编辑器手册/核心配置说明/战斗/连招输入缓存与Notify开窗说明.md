# 连招输入缓存与 Notify 开窗说明

## 当前规则

- 连招窗口由实际 `AnimMontage` 上的 `Combo Window` NotifyState 控制。
- 旧的 `ANS_AddGameplayTag` 仍兼容，但不推荐新内容继续手填 `PlayerState.AbilityCast.CanCombo`。
- `GameplayAbilityComboGraph` 节点上的 `ComboWindowStartFrame / EndFrame / TotalFrames` 只作为展示数据，不驱动运行时。
- 玩家在窗口打开前提前按下一次轻/重攻击时，输入会进入 Buffer；`CanCombo` 打开后系统会消费一次输入并尝试进入下一段。

## 配置检查

1. 需要继续接下一段的蒙太奇必须加 `Combo Window` NotifyState。
2. 终结击通常不需要加 `Combo Window` Notify。
3. 如果日志出现 `Queue child activation until CanCombo opens`，说明输入早于窗口，这是正常缓存行为。
4. 如果一直无法接下一段，优先检查当前段蒙太奇是否真的有 `Combo Window` NotifyState。

## 注意

- Graph 连招武器下，`PostAtkWindow` 不再负责攻击输入时清理 `CanCombo`，避免和输入缓存抢时序。
- 旧的非 Graph 连招仍可继续使用原有 Notify/Tag 逻辑。
