# GameplayAbilityComboGraph 编辑器配置说明

## 作用

用于用节点图配置武器轻/重攻击连招。512 阶段 Graph 是主配置入口，旧 `WeaponComboConfigDA` 只做兼容回退。

## 配置位置

| 配置对象 | 位置 |
| --- | --- |
| 连招图资产 | 内容浏览器右键创建 `Gameplay Ability Combo Graph` |
| 武器引用 | `WeaponDefinition.GameplayAbilityComboGraph` |
| 旧配置回退 | `WeaponDefinition.WeaponComboConfig` |

## 节点字段

| 字段 | 说明 | 推荐 |
| --- | --- | --- |
| `NodeId` | 节点运行时 ID | 每个节点唯一，如 `L1`、`L2H` |
| `RootInputAction` | 根节点输入 | 第一段轻击填 `Light`，第一段重击填 `Heavy` |
| `GameplayAbilityClass` | 旧 GA 类 | 优先复用旧攻击 GA |
| `AbilityTagOverride` | 手动覆盖 GA Tag | 旧 GA 无法解析 Tag 时填写 |
| `MontageConfig` | 节点播放的蒙太奇配置 | 必填 |
| `AttackDataOverride` | 节点攻击参数覆盖 | 同蒙太奇不同伤害/范围时填写 |
| `bIsComboFinisher` | 是否终结击 | 只在连招收尾节点勾选 |
| `bAllowDashSave` | 是否允许 DashSave | 默认开启 |
| `bUseNodeComboWindow` | 是否用节点帧窗口 | 新 Graph 节点建议开启 |
| `ComboWindowStartFrame` | 可接下一击开始帧 | 按动画手感填写 |
| `ComboWindowEndFrame` | 可接下一击结束帧 | 必须大于等于开始帧 |
| `ComboWindowTotalFrames` | 该动作总帧数 | 与动画帧数一致 |

## 连线字段

| 字段 | 说明 |
| --- | --- |
| `InputAction` | 从父节点进入子节点需要的输入，填 `Light` 或 `Heavy` |

## 推荐配置流程

1. 创建 `Gameplay Ability Combo Graph`。
2. 建根节点 `L1`、`H1`，分别配置 `RootInputAction = Light / Heavy`。
3. 从节点拖线到下一节点，在连线里配置 `InputAction`。
4. 每个节点填写 `MontageConfig`，需要分支差异时填写 `AttackDataOverride`。
5. 在节点上配置 `ComboWindowStartFrame / EndFrame / TotalFrames`。
6. 在武器 `WeaponDefinition.GameplayAbilityComboGraph` 引用该 Graph。

## 验收方式

1. `L-L-L`、`L-L-H`、`H-H` 能路由到不同节点。
2. 同一蒙太奇复用到两个节点时，能使用不同 `AttackDataOverride`。
3. 未到 `ComboWindowStartFrame` 时不能接下一击。
4. 窗口内输入下一击，会打断当前蒙太奇并进入目标节点。
5. `GameplayAbilityComboGraph` 为空时，武器仍可回退 `WeaponComboConfig`。

## 注意事项

- 不要启用旧 `ComboGraph` 插件。
- 同一个父节点下不要配置两个相同 `InputAction` 的子节点。
- `CombatCard.TriggerTiming` 仍由卡牌配置决定，不建议在节点上配置卡牌触发时机。
