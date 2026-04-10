# GameplayTag 总体设计指南

> 版本：v1（2026-04-09）  
> 适用范围：项目全部 GameplayTag，覆盖 GAS、AI、状态机、符文系统、跨系统通信

---

## 一、命名空间总图

```
项目 GameplayTag 根命名空间
│
├── Action.*          【动作信号层】  战斗/交互中发生了什么（瞬时事件）
├── Attribute.*       【属性引用层】  AttributeSet 字段的 Tag 化映射
├── Buff.*            【符文效果层】  符文/Buff 系统专属（5层模型）
├── Enemy.*           【敌人技能层】  敌人 GA 身份标签
├── Enemy_Behavior.*  【敌人AI层】   敌人行为状态机
├── PlayerState.*     【玩家状态层】  玩家 GA 身份 + 状态机
├── Room.*            【关卡房间层】  RoomDataAsset 房间类型分类，供刷怪系统匹配
├── State.*           【冲突管理层】  StateConflictDataAsset 使用的互斥/共存规则
└── Event.*           【跨系统事件层】 跨命名空间通信（符文间/系统间）
```

---

## 二、各命名空间详解

### 2.1 `Action.*` — 动作信号层

**职责：** 描述"战斗中发生了某件事"，是一次性信号，不是持续状态。  
**挂载位置：** 不挂 ASC；通过 `SendGameplayEventToActor` 广播，或由动画/代码发送。  
**读取方式：** GA 的 `AbilityTriggers → GameplayEvent`，或 FA 的 `BFNode_WaitGameplayEvent`。

| 现有 Tag | 含义 |
|---------|------|
| `Action.Combo.LastHit` | 连招最后一击判定帧 |
| `Action.Dead` | 触发死亡 GA 的事件信号（由 Die() 自动发送） |
| `Action.Heat.CanPhaseUp` | 允许热度升阶（可由 LastHit / 完美闪避等授予） |
| `Action.HitReact` | 触发受击 GA 的事件信号（由 FA 判断后发送） |
| `Action.Knockback` | 触发击退 GA 的事件信号 |
| `Action.Dash` | ⭐ 建议新增：闪避动作发生 |
| `Action.Interact` | ⭐ 建议新增：交互动作发生 |
| `Action.SkillCast` | ⭐ 建议新增：主动技能释放 |

**`Action.*` 的双重用途（通用响应型 GA）：**

对于 GA_Knockback / GA_HitReaction / GA_Dead 这类玩家和敌人均可触发的通用 GA：

| 用途 | 说明 |
|-----|------|
| AbilityTriggers Trigger Tag | 触发激活的事件信号（FA/代码发送） |
| GA.AbilityTags | GA 身份标签（用于 CancelAbilitiesWithTag 查询） |
| AbilityData.PassiveMap key | 查找动画/效果数据的 key |

同一个 `Action.*` Tag 兼顾三个角色，不需要额外命名空间。  
`AbilityTags` 不会挂到 actor 的 OwnedGameplayTags，不违反 "Action.* 不挂 ASC" 的规则。

**创建规则：**
- 命名用动词或名词，表示"某事发生了"
- 不描述持续状态，不做条件判断
- 每个动作时机只对应一个 Tag

---

### 2.2 `Attribute.*` — 属性引用层

**职责：** 将 AttributeSet 字段 Tag 化，用于 SetByCaller / 伤害管道的数据通道。  
**挂载位置：** 不挂 ASC，仅作 Key 使用。

| 现有 Tag | 含义 |
|---------|------|
| `Attribute.ActDamage` | 攻击力属性引用 |
| `Attribute.ActResilience` | 韧性属性引用 |
| `Attribute.ActRange` | 攻击范围属性引用 |

---

### 2.3 `Buff.*` — 符文效果层（5层模型）

**职责：** 符文/Buff 系统专属，详见 [Buff_Tag_Spec.md](Buff_Tag_Spec.md)。

| 子层 | 职责 |
|-----|------|
| `Buff.Rune.*` | 符文身份（类型/稀有度），贴在 RuneDataAsset |
| `Buff.Effect.*` | GE/效果行为标识，贴在 GE AssetTags |
| `Buff.Trigger.*` | 符文触发时机过滤，贴在符文 GA AssetTags |
| `Buff.Status.*` | 运行时状态，挂在目标 ASC |
| `Buff.Data.*` | SetByCaller 数值通道 Key |

**重要边界：**  
`Buff.Trigger.*` 只描述"这个符文**何时**生效"（OnHit / OnKill 等），  
**不等于** `Action.*`（动作信号）。两者对比：

```
Action.Combo.LastHit    ← 信号：连招最后一击发生了（发送给 GA / FA）
Buff.Trigger.OnComboEnd ← 标识：这个符文在连招结束时触发（贴在符文 GA）
```

---

### 2.4 `PlayerState.*` — 玩家状态层

**职责：** 玩家 GA 的身份标签 + 玩家状态机标记，挂在玩家 ASC。  
**作用：** GA AbilityTags（用于查找/取消）+ 状态冲突系统 Block/Cancel 的目标。

| 分类 | 示例 | 含义 |
|-----|------|------|
| `PlayerState.AbilityCast.*` | `AbilityCast.LightAtk.Combo1` | 玩家正在执行某个动作 |
| `PlayerState.AbilityCast.Dash` | — | 玩家正在闪避 |
| `PlayerState.AnimLayer.*` | `AnimLayer.Default` | 当前动画层 |

**`PlayerState.AbilityCast.*` 的双重用途：**  
这层 Tag 同时作为：
- GA 的 `AbilityTags`（GA 身份，用于 CancelAbilitiesWithTag）
- `SkillChargeComponent.RegisterSkill()` 的注册键（充能/CD 管理的查询键）

两者使用同一个 Tag，逻辑统一，无需额外命名空间。  
`Ability.*` 在本项目中不应作为独立根命名空间存在。

---

### 2.5 `Enemy.*` — 敌人技能层

**职责：** 敌人 GA 身份标签，结构类比 `PlayerState.AbilityCast.*`。

| 现有 Tag | 含义 |
|---------|------|
| `Enemy.Melee.HAtk1` | 敌人近战重攻击1 |
| `Enemy.Skill.Skill1` | 敌人技能1 |
| `Enemy.Skill.Knockback` | ⭐ 建议新增：敌人击退 GA 身份 |

---

### 2.6 `Enemy_Behavior.*` — 敌人AI层

**职责：** 敌人 AI 状态机，由行为树/AI系统管理，不由 GAS 直接控制。

---

### 2.7 `Room.*` — 关卡房间层

**职责：** 标识 `URoomDataAsset` 的房间**类型**和**层级**，供 `SelectRoomByTag` 在关卡结算时双重过滤。  
**挂载位置：** 只填写在 `RoomDataAsset.RoomTags`（`FGameplayTagContainer`），**不挂 ASC**，与 GAS 无关。  
**配置文件：** `Config/Tags/RoomTag.ini`

每个 DA_Room 的 `RoomTags` 同时包含**一个类型 Tag + 一个层级 Tag**：

```
DA_Room_Prison_Normal  →  RoomTags: { Room.Type.Normal, Room.Layer.L1 }
DA_Room_Prison_Elite   →  RoomTags: { Room.Type.Elite,  Room.Layer.L1 }
DA_Room_Temple_Normal  →  RoomTags: { Room.Type.Normal, Room.Layer.L2 }
```

#### `Room.Type.*` — 房间类型

| Tag | 含义 |
|-----|------|
| `Room.Type.Normal` | 普通战斗房（最常见，标准敌人池） |
| `Room.Type.Elite` | 精英战斗房（精英专属敌人通过 EnemyPool 设计控制，无需 bEliteOnly flag） |
| `Room.Type.Shop` | 商店房（功能待实现） |
| `Room.Type.Event` | 事件房（功能待实现） |

**注意：** Shop / Event 只在 RoomPool 中存在对应资产时才会被骰出；若无匹配资产，自动退化为 Normal。

#### `Room.Layer.*` — 大关卡层级

类比《Hades》的 Act/Region 概念。每个大关卡对应一个独立的 `DA_Campaign`，在其 `LayerTag` 字段填写对应层级 Tag，系统选房时只匹配同层房间。

| Tag | 含义 | 对应 Campaign |
|-----|------|--------------|
| `Room.Layer.L1` | 第一大关卡层 | `DA_Campaign_L1`（`LayerTag = Room.Layer.L1`） |
| `Room.Layer.L2` | 第二大关卡层 | `DA_Campaign_L2` |
| `Room.Layer.L3` | 第三大关卡层 | `DA_Campaign_L3` |

**`LayerTag` 为空时**（开发/单关测试场景），层级过滤自动跳过，选取池中任意匹配类型的房间。

---

### 2.8 `State.*` — 冲突管理层

**职责：** 专为 `StateConflictDataAsset`（状态黑名单系统）设计。  
当状态间的 Block/Cancel 关系不适合挂在某一具体命名空间时，使用此层统一管理。  
**挂载位置：** 由 `YogAbilitySystemComponent.OnTagUpdated` 自动处理，不手动写入。

| 示例 Tag | 含义 |
|---------|------|
| `State.Combat` | 战斗中（影响热度衰减、受击判定） |
| `State.Interact` | 交互进行中（阻止攻击/闪避） |
| `State.Dialogue` | 对话中（阻止所有战斗行为） |
| ~~`State.Dead`~~ | ⚠️ 已废弃，迁移至 `Buff.Status.Dead` |

**与其他层的关系：**
```
PlayerState.AbilityCast.LightAtk  ← GA 身份标签（具体是哪个技能）
State.Combat                       ← 抽象状态标签（用于冲突规则匹配）
```
StateConflictDataAsset 的 ActiveTag 可以填任意层的 Tag，  
`State.*` 是那些"不属于某具体技能但影响全局行为"的状态的归属。

---

### 2.9 `Block.*` — 系统阻断分类层

**职责：** `StateConflictDataAsset.BlockCategoryMap` 的 Key 命名空间，  
标识"哪类底层系统需要被阻断"，不挂在角色 ASC 上，仅用于 DA 配置的 Key。  
**挂载位置：** 仅作为 BlockCategoryMap 的 Key，不写入 ASC。

| Tag | 含义 |
|-----|------|
| `Block.Movement` | 阻断移动组件（DisableMovement + AI StopMovement） |
| `Block.AI` | 暂停行为树逻辑（PauseLogic），玩家角色无效 |

---

### 2.10 `Event.*` — 跨系统事件层

**职责：** 跨命名空间通信，当事件不归属于某单一系统时使用。  
典型场景：符文联动（符文A的效果完成后通知符文B）。  
**挂载位置：** 不挂 ASC；通过 `SendGameplayEventToActor` 广播。

| 示例 Tag | 含义 |
|---------|------|
| `Event.Rune.KnockbackApplied` | 击退完成，供联动符文监听 |
| `Event.Rune.BleedApplied` | 流血施加成功，供联动符文监听 |
| `Event.Rune.KillConfirmed` | 击杀确认，供联动符文监听 |

**与 `Action.*` 的区别：**
```
Action.Knockback            ← 触发 GA 的输入信号（FA → 目标）
Event.Rune.KnockbackApplied ← GA 执行完成的输出信号（GA → 玩家 ASC）
```
`Action.*` 是"请执行某动作"，`Event.Rune.*` 是"某动作已完成"。

---

## 三、状态冲突配置指南（StateConflictDataAsset）

冲突表的 ActiveTag 来源各层的含义：

| 填入的 ActiveTag | 来源层 | 冲突触发时机 |
|----------------|-------|------------|
| `PlayerState.AbilityCast.LightAtk` | 玩家状态层 | 玩家正在普攻时 |
| `Buff.Status.HitReact` | 符文状态层 | 角色受击硬直时 |
| `Buff.Status.Knockback` | 符文状态层 | 目标处于击退硬直时 |
| `Buff.Status.Dead` | 符文状态层 | 角色死亡时 |
| `State.Dialogue` | 冲突管理层 | 对话进行中 |

**典型冲突规则示例：**

| ActiveTag | BlockTags | CancelTags | Priority | 说明 |
|-----------|-----------|------------|----------|------|
| `PlayerState.AbilityCast.LightAtk` | `PlayerState.AbilityCast.Dash` | — | 10 | 普攻中无法闪避 |
| `PlayerState.AbilityCast.Dash` | `PlayerState.AbilityCast.LightAtk` | `PlayerState.AbilityCast.LightAtk` | 20 | 闪避取消普攻 |
| `Buff.Status.HitReact` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 20 | 受击硬直打断所有玩家动作 |
| `Buff.Status.Knockback` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 50 | 被击退时取消全部玩家动作 |
| `Buff.Status.Dead` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 999 | 死亡优先级最高 |
| `State.Dialogue` | `PlayerState.AbilityCast.*` | `PlayerState.AbilityCast.*` | 100 | 对话中禁止所有战斗行为 |
| `Buff.Rune.Type.Attack` | `Buff.Rune.Type.Attack` | — | -1 | 同类攻击型符文只激活一个 |

---

## 四、Tag 创建决策树

```
要创建一个新 Tag？
│
├─ 描述"发生了一件事"（瞬时信号）
│   ├─ 触发某个 GA 激活 ──────────────→ Action.*
│   └─ 符文完成后通知其他符文 ─────────→ Event.Rune.*
│
├─ 描述"当前处于某状态"（持续存在）
│   ├─ 玩家正在执行某技能/动作 ─────────→ PlayerState.AbilityCast.*
│   ├─ 敌人正在执行某技能 ────────────→ Enemy.*
│   ├─ 符文/Buff 效果产生的状态 ────────→ Buff.Status.*
│   └─ 全局状态（死亡/对话/交互）───────→ State.*
│
├─ 描述"这是什么"（身份标签）
│   ├─ 这个符文是什么类型/稀有度 ────────→ Buff.Rune.*
│   ├─ 这个 GE/效果做什么 ──────────────→ Buff.Effect.*
│   └─ 这个符文何时触发 ────────────────→ Buff.Trigger.*
│
├─ 传递数值（SetByCaller）───────────────→ Buff.Data.*
│
└─ 敌人 AI 行为状态 ─────────────────────→ Enemy_Behavior.*
```

---

## 五、GA 身份 Tag（AbilityTags）使用原则

**AbilityTags 只在有代码通过它查询/取消 GA 时才添加。**  
没有读取方就是多余 Tag——遵循"没有代码读的 tag 就是多余的 tag"原则。

以 `GA_Knockback` 为例：

| 用途 | 应使用的 Tag | 是否必要 |
|-----|------------|---------|
| GA AbilityTags（身份） | **不设置** | ❌ 无代码通过身份查询此 GA |
| GA AbilityTriggers（激活事件） | `Action.Knockback` | ✅ FA 发送此 Event 触发激活 |
| GA ActivationOwnedTags（持续状态） | `Buff.Status.Knockback` | ✅ StateConflict 系统读此 Tag |

> **为什么不设置 AbilityTags？**  
> GA_Knockback 可作用于玩家或敌人任意角色，无法归入 `Enemy.*` 或 `PlayerState.*` 其中一个。  
> 当前没有系统需要通过 CancelAbilitiesWithTag 单独取消击退 GA（StateConflict 是通过 `Buff.Status.Knockback` 阻断后续能力，而非取消击退本身）。  
> 未来如果需要，再按实际读取方归属到合适命名空间。

**`Ability.*` 命名空间在本项目中不应使用。**  
所有功能均可归入现有命名空间，无需新建根节点。

---

## 六、命名规范

| 规则 | 正确 | 错误 |
|-----|------|------|
| PascalCase | `Action.KnockbackApplied` | `Action.knockback_applied` |
| 层级 ≤ 5 | `Buff.Status.Heat.Phase.1` | `Buff.Status.Heat.Phase.Up.Stage.1` |
| 叶节点必须有 DevComment | `DevComment="触发击退 GA"` | `DevComment=""` |
| 不在父节点挂 ASC | `Buff.Status.Heat`（父节点，不挂） | 把父节点当状态用 |
| 同一时刻互斥的用父节点 Block | `State.*` 冲突规则 | 在代码里硬判断 |

---

## 七、各系统文件对照

| 文件 | 管理的 Tag 层 |
|-----|-------------|
| `Config/Tags/BuffTag.ini` | `Buff.*` 全部 + `Action.*` + `Event.*` |
| `Config/Tags/PlayerTag.ini` | `PlayerState.*` |
| `Config/Tags/EnemyTag.ini` | `Enemy.*` + `Enemy_Behavior.*` |
| `Config/Tags/StateTag.ini` | `State.*` |
| `Config/Tags/RoomTag.ini` | `Room.*` |
| `Docs/Design/Buff_Tag_Spec.md` | `Buff.*` 详细规范 |
| **本文档** | 全命名空间总览 |
| `Docs/Design/StateConflict_TagBlock.md` | 冲突规则配置指南 |
