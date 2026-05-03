# GenericGraph Dash 与连击 Tag 配置说明

## 用途

本功能用于把轻击、重击、冲刺统一放进 `GameplayAbilityComboGraph` 管理。策划只需要配置动作节点，不需要手动处理 `CanCombo`、Combo1/2/3、BlockTag 或 DashSaveTag。

## 配置位置

- 武器 DA：`WeaponDefinition -> GameplayAbilityComboGraph`
- 连招图资产：`GameplayAbilityComboGraph`
- 节点类型：`GameplayAbilityComboGraphNode`
- 边类型：`GameplayAbilityComboGraphEdge`

## 攻击节点

攻击节点按原连招方式配置：

- `RootInputAction`：根节点输入，轻击填 `Light`，重击填 `Heavy`。
- 边 `InputAction`：从当前节点接到下一节点的输入，填 `Light` / `Heavy`。
- `GameplayAbilityClass`：选择要激活的旧 GA，推荐填写。
- `AbilityTagOverride`：需要覆盖 GA 自带 AbilityTag 时填写；运行时会先按 Tag 找 GA，找不到时用 `GameplayAbilityClass` 兜底。
- `MontageConfig`：选择本节点播放的蒙太奇配置。
- `AttackDataOverride`：需要覆盖攻击参数时填写。
- `bIsComboFinisher`：该节点是否是终结击。
- `bAllowDashSave`：该攻击节点是否允许被冲刺保存连击。

不用配置：

- `PlayerState.AbilityCast.CanCombo`
- `PlayerState.AbilityCast.LightAtk.Combo1/2/3/4`
- `PlayerState.AbilityCast.HeavyAtk.Combo1/2/3/4`
- `Action.Combo.DashSavePoint`
- GAS BlockTag

这些由运行时自动兼容旧 GA，并在攻击结束、冲刺结束或连击重置时清理。

如果使用旧 `WeaponComboConfigDA` 而不是 Graph，也可以在节点高级字段里填写 `GameplayAbilityClass`。该字段只用于旧 GA 激活兜底，策划正常配置仍以 Graph 节点为主。

## Dash 节点

如果希望冲刺也走 GenericGraph，创建一个根节点：

- `RootInputAction`：填 `Dash`。
- `AbilityTagOverride`：填 `PlayerState.AbilityCast.Dash`，或选择对应 Dash GA。
- `MontageConfig`：一般不填。
- `AttackDataOverride`：不填。

没有 Dash 节点时，系统会回退旧冲刺激活逻辑。

## DashSave 设置

Dash 节点提供以下字段：

- `DashSaveMode`
  - `None`：冲刺直接断连击。
  - `PreserveIfSourceAllows`：只有当前攻击节点勾选 `bAllowDashSave` 时保存连击，推荐默认。
  - `ForcePreserve`：无视攻击节点设置，强制保存当前连击。
- `DashSaveExpireSeconds`：冲刺保存连击的有效时间，默认 `2.0` 秒。
- `bSavePendingLinkContext`：是否保留一次卡牌 Link 上下文，默认开启。
- `bClearCombatTagsOnDashEnd`：冲刺结束时清理连击临时 Tag，默认开启。
- `bBreakComboOnDashCancel`：冲刺被打断时是否取消保存的连击，默认开启。

## 推荐配置

512 当前推荐：

1. Light/Heavy 攻击节点按连招树配置。
2. 允许冲刺保连击的攻击节点勾选 `bAllowDashSave`。
3. 新增一个 Dash 根节点，`RootInputAction = Dash`。
4. Dash 节点使用：
   - `DashSaveMode = PreserveIfSourceAllows`
   - `DashSaveExpireSeconds = 2.0`
   - `bSavePendingLinkContext = true`
   - `bClearCombatTagsOnDashEnd = true`
   - `bBreakComboOnDashCancel = true`

## 验收

- L1 -> L2 -> L3 可以正常完成。
- L3 期间继续按攻击不会立刻覆盖回 L1。
- Dash 后不会残留 `CanCombo` 或旧 Combo Tag 导致无法攻击。
- 允许 DashSave 的节点，Dash 后下一次攻击能继续连击。
- 不允许 DashSave 的节点，Dash 后下一次攻击从根节点开始。
- 玩家处于 HitReact / Dead / Knockback 时仍然会被真实状态阻断，运行时清理不会绕过这些状态。
