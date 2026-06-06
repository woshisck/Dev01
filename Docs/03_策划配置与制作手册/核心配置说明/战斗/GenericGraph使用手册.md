# GenericGraph 使用手册

## 作用

`GenericGraph` 是一个通用节点图插件，只负责提供“图编辑器 + 节点/边数据结构”。它本身不定义战斗规则、任务规则或对话规则。

在当前战斗系统里，我们基于它做了 `GameplayAbilityComboGraph`，用于配置武器连招。

## 基本概念

| 名称 | 说明 |
| --- | --- |
| Graph | 整张图资产，例如 `CG_THSword_Test` |
| Node | 节点，表示一个连招段、状态或步骤 |
| Edge | 节点之间的连线，表示从父节点进入子节点的条件 |
| Root Node | 没有父节点、没有入边的节点 |
| Leaf Node | 没有子节点、没有出边的节点 |

GenericGraph **没有单独的 Root 节点类型**。一个节点只要没有任何父节点，它就是 Root Node。

## 当前战斗用法

创建资产：

1. 在 Content Browser 右键创建 `Gameplay Ability Combo Graph`。
2. 打开 Graph。
3. 右键添加 `Combo Ability Node`。
4. 从父节点拖线到子节点。
5. 选中节点或连线，在 Details 面板填写字段。
6. 保存资产。

保存时 GenericGraph 会重建运行时数据，包括：

- `RootNodes`
- `AllNodes`
- `ParentNodes`
- `ChildrenNodes`
- `Edges`

所以修改 Graph 后必须保存，否则运行时可能仍读取旧数据。

## Root Node 规则

战斗连招里，第一段攻击节点就是 Root Node。

例如：

- `L1` 没有入边，`RootInputAction = Light`，表示轻攻击起手。
- `H1` 没有入边，`RootInputAction = Heavy`，表示重攻击起手。

玩家第一次按 Light 时，运行时会在 RootNodes 里找 `RootInputAction = Light` 的节点；第一次按 Heavy 时找 `RootInputAction = Heavy` 的节点。

## 节点配置

`GameplayAbilityComboGraphNode` 常用字段：

| 字段 | 用途 |
| --- | --- |
| `NodeId` | 节点唯一 ID，例如 `L1`、`L2H`、`H2` |
| `RootInputAction` | 只有 Root Node 需要重点配置，决定起手输入 |
| `GameplayAbilityClass` | 可选，填写旧 GA 类 |
| `AbilityTagOverride` | 推荐填写，用于指定激活哪个 GA |
| `MontageConfig` | 必填，决定当前节点播放哪个蒙太奇配置 |
| `AttackDataOverride` | 可选，用于同蒙太奇不同攻击参数 |
| `bIsComboFinisher` | 是否终结击 |
| `bAllowDashSave` | 是否允许 DashSave 保存该节点 |
| `bUseNodeComboWindow` | 是否使用节点自己的连招窗口 |
| `ComboWindowStartFrame` | 可接下一击的开始帧 |
| `ComboWindowEndFrame` | 可接下一击的结束帧 |
| `ComboWindowTotalFrames` | 当前动作总帧数 |

## 连线配置

`GameplayAbilityComboGraphEdge` 常用字段：

| 字段 | 用途 |
| --- | --- |
| `InputAction` | 从父节点进入子节点需要的输入，通常填 `Light` 或 `Heavy` |

例子：

- `L1 -> L2` 的连线填 `Light`
- `L2 -> L3` 的连线填 `Light`
- `L2 -> L2H` 的连线填 `Heavy`

这样玩家输入 `Light -> Light -> Heavy` 时，就会进入 `L2H`，而不是 `L3`。

## 推荐配置流程

1. 先创建两个起手节点：`L1`、`H1`。
2. `L1.RootInputAction = Light`，`H1.RootInputAction = Heavy`。
3. 每个节点填写唯一 `NodeId`。
4. 每个节点填写 `AbilityTagOverride` 和 `MontageConfig`。
5. 从节点拖线配置分支。
6. 每条连线填写 `InputAction`。
7. 每个节点填写连招窗口帧。
8. 保存 Graph。
9. 在 `WeaponDefinition.GameplayAbilityComboGraph` 引用该 Graph。

## 示例

测试资产：

- Graph：`/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test`
- 测试武器：`/Game/Docs/Combat/TwoHandedSword/DA_WPN_THSword_ComboGraphTest`

当前结构：

- `L1 --Light--> L2 --Light--> L3`
- `L2 --Heavy--> L2H`
- `H1 --Heavy--> H2`

Root Node：

- `L1`
- `H1`

Finisher：

- `L3`
- `L2H`
- `H2`

## 常见问题

### 找不到 Root Node

检查该节点是否有入边。只要被别的节点连进来，它就不再是 Root Node。

### 按键没有进入预期分支

检查父节点到子节点的 Edge 是否正确填写 `InputAction`。同一个父节点下不要有两条相同 `InputAction` 的边。

### Graph 修改后运行时没变化

保存 Graph。GenericGraph 的运行时节点/边数据在保存时重建。

### 第一击无法触发

检查：

- Root Node 的 `RootInputAction`
- 节点 `AbilityTagOverride`
- 武器 DA 的 `GameplayAbilityComboGraph`
- 角色是否已获得对应 GA

### 不建议做的事

- 不要启用旧 `ComboGraph` 插件。
- 不要把 `RootInputAction` 当作普通子节点跳转条件；子节点跳转看 Edge 的 `InputAction`。
- 不要直接改 GenericGraph 插件逻辑来做战斗规则；战斗规则应放在 `GameplayAbilityComboGraph` 和运行时组件里。
