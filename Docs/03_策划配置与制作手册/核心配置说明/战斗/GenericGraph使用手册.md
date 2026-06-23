# GenericGraph 使用手册

## 作用

`GenericGraph` 是一个通用节点图插件，只负责提供“图编辑器 + 节点/边数据结构”。它本身不定义战斗规则、任务规则或对话规则。

当前玩家战斗主流程不再使用 GenericGraph 配置动作。新武器动作配置在 `WeaponDefinition.AttackAbilityData`、`WeaponDefinition.WeaponSkillAbilityData` 和 `AbilityData.MontageConfigMap`。

## 当前战斗中的定位

| 用途 | 状态 |
| --- | --- |
| 新玩家武器动作 | 不使用 GenericGraph |
| 旧 ComboGraph 资产加载 | 保留兼容 |
| 旧节点、边、NodeId 对照 | 可查看，不继续扩展 |
| 新卡牌 Link / 构筑条件 | 使用卡牌顺序、动作槽、FlowRole 和 Link Recipe |

## GenericGraph 基本概念

| 名称 | 说明 |
| --- | --- |
| Graph | 整张图资产 |
| Node | 节点，表示一个状态或步骤 |
| Edge | 节点之间的连线，表示从父节点进入子节点的条件 |
| Root Node | 没有父节点、没有入边的节点 |
| Leaf Node | 没有子节点、没有出边的节点 |

GenericGraph **没有单独的 Root 节点类型**。一个节点只要没有任何父节点，它就是 Root Node。

## 旧战斗资产查看方式

旧 `GameplayAbilityComboGraph` 仍可打开查看：

1. 在 Content Browser 打开旧 Graph。
2. 查看节点上的 `MontageConfig`、`AttackDataOverride`、旧 NodeId。
3. 如需继续制作，把信息迁移到 typed AbilityData。
4. 保存旧 Graph 只用于旧资产维护，不作为新战斗配置流程。

## 迁移建议

| 旧 Graph 数据 | 当前迁移目标 |
| --- | --- |
| 节点蒙太奇 | `AbilityData.MontageConfigMap` |
| 节点攻击参数 | `MontageConfigDA.Entries.Hit Detection Window.AttackDataCandidates` |
| 旧输入分支 | `RequiredActionSlot` / `RequiredFlowRole` / Link Recipe |
| 旧终结节点 | WeaponSkill 或 Finisher FlowRole |

## 注意

- 不要为新武器创建新的战斗 Graph。
- 不要把卡牌连携条件写在旧 Graph 节点或边上。
- 如果旧 Graph 修改后必须验证，明确标注为兼容路径验收。
