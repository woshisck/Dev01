# BuffFlow 系统 —— 使用说明

## 概述

BuffFlow 是基于 FlowGraph 插件的可视化 Buff/符文效果编辑系统。策划可以通过拖拽节点、连线的方式组合复杂的 Gameplay 效果，无需编写 Blueprint 或 C++ 代码。

---

## 系统架构

```
符文 DA（RuneDataAsset）
  ├── AttributeModifiers   → 纯数值效果（攻击+20等，DA里直接填）
  ├── BehaviorEffect       → 预制行为GE（击退等，选一个GE类）
  └── BuffFlowAsset        → FlowAsset 可视化逻辑（本系统）

角色上挂载：
  BuffFlowComponent → 管理 Flow 生命周期
  ↕ 与 BackpackGridComponent 联动
  ↕ 符文激活时自动启动 Flow，卸下时自动停止
```

---

## 快速开始

### 1. 角色设置

在玩家角色 Blueprint 上添加 `BuffFlowComponent` 组件：
- 打开玩家角色 BP
- Add Component → 搜索 "BuffFlow"
- 添加 `BuffFlowComponent`

### 2. 创建 FlowAsset

1. Content Browser 右键 → Flow → Flow Asset
2. 命名为 `FA_Rune_效果名`（如 `FA_Rune_Frenzy`）

### 3. 编辑 Flow 逻辑

双击 FlowAsset 打开编辑器，右键创建节点：

| 节点 | 用途 |
|------|------|
| **添加Buff** | 施加一个 YogBuffDefinition 定义的 Buff |
| **移除Buff** | 移除指定 Buff |
| **结束当前Buff** | 终止整个 Flow（终端节点） |
| **添加标签** | 给目标添加 GameplayTag（可指定数量） |
| **移除标签** | 移除 GameplayTag |
| **检查标签** | 分支节点：目标是否有某标签 → Yes/No |
| **当造成伤害时** | 触发器：BuffOwner 对别人造成伤害时触发 |
| **当受到伤害时** | 触发器：BuffOwner 受到伤害时触发 |
| **获取属性值** | 读取目标的属性值（Attack、MaxHealth 等） |
| **浮点比较** | 分支节点：比较两个浮点数 → True/False |

### 4. 挂到符文 DA

1. 打开符文的 RuneDataAsset
2. 在 Effect 分类下找到 `BuffFlowAsset`
3. 选择刚创建的 FlowAsset

### 5. 运行测试

- 将符文放入背包激活区域
- Flow 自动启动
- 移除符文时 Flow 自动停止

---

## 示例：狂暴符文

**效果：** 每命中敌人 1 次叠加 1 层狂暴标签，叠满 5 层后攻击力 +50%

### Flow 连线

```
[Start]
    ↓
[当造成伤害时]  ← 延时节点，每次命中都触发
    ↓
[添加标签]       Tag=Passive.Rage, Count=1
    ↓
[获取属性值]     Attribute=TagCount(Passive.Rage)
    ↓
[浮点比较]       A=Value, Op=>=, B=5.0
    ├─True→  [添加Buff]  BuffDefinition=BD_AttackBoost
    └─False→ (不连，等待下次命中)
```

### 需要准备的资产

1. **GameplayTag：** `Passive.Rage`（在 Config/Tags/Buff.ini 中，已存在）
2. **YogBuffDefinition：** `BD_AttackBoost`
   - DurationPolicy: Infinite
   - Modifiers: Attack → Multiplicative → 0.5

---

## 三种效果配置方式对比

| 场景 | 推荐方式 | 示例 |
|------|----------|------|
| 纯数值加成 | DA `AttributeModifiers` 直接填 | 攻击 +20 |
| 简单行为 | DA `BehaviorEffect` 选预制 GE | 击退 |
| 条件逻辑 | DA `BuffFlowAsset` 可视化编辑 | 命中5次后触发 |
| 数值 + 条件 | `AttributeModifiers` + `BuffFlowAsset` 同时填 | 基础攻击+10，且命中3次额外暴击 |

---

## 节点详细说明

### 触发器节点（延时）

**当造成伤害时 / 当受到伤害时**
- 输入 Pin: `In`（开始监听）、`Stop`（停止监听）
- 输出 Pin: `OnDamage`（每次伤害都触发，不会只触发一次）
- 节点保持激活状态，持续监听直到 Flow 结束或收到 Stop

### 分支节点

**检查标签**
- 输入 Pin: `In`
- 输出 Pin: `Yes`（有标签）、`No`（无标签）

**浮点比较**
- 输入 Pin: `In`
- 输出 Pin: `True`、`False`
- 支持: >, >=, ==, <=, <, !=

---

## 注意事项

1. **BuffFlowComponent 必须挂在角色上** — 否则 Flow 无法启动
2. **Flow 会随符文自动启停** — 不需要手动管理生命周期
3. **一个符文可以同时有 AttributeModifiers + BehaviorEffect + BuffFlowAsset** — 三者独立，互不影响
4. **延时节点的委托在 Flow 结束时自动解绑** — 不会内存泄漏