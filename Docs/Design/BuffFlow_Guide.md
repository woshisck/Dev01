# BuffFlow 系统指南

> 更新日期：2026-04-05  
> 对象：策划 + 程序

---

## 一、系统概述

**BuffFlow = 可视化 Buff/Effect 逻辑系统。**

它解决了一个核心问题：Buff 的数值配置很简单（填表），但 Buff 的"行为逻辑"很复杂（什么时候触发、触发条件、触发后做什么）。BuffFlow 把这部分逻辑从代码中抽出来，放进可视化的 Flow Graph 里，让策划能直接看到并修改 Buff 的行为。

### 两层分离原则

```
DA（Data Asset）= 数据层        FA（Flow Asset）= 逻辑层
─────────────────────────────   ─────────────────────────────
· Buff 的展示信息（名称/图标）  · 什么时候触发
· GE 行为规则（Duration/Stack） · 触发条件判断（层数/属性）
· Effects[]（属性修改/Tag/GA）  · 触发后做什么（GA/特效/音效）
· 背包格子形状                  · 复杂的状态机/计时器逻辑
─────────────────────────────   ─────────────────────────────
                策划配置                    策划用节点搭建
```

**GAS 数值流向：**  
```
GE（施加）→ AttributeSet（存储）→ GA（读取并使用）
```
- GE 是数据写入者，负责把数值写进 AttributeSet
- GA 是数值使用者，执行时从 AttributeSet 读取当前值
- FA 是行为决策者，决定 GA 何时触发、条件是什么

---

## 二、DA 结构（RuneDataAsset）

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description / Buff Type   ← 展示信息
  ├── Shape                                         ← 背包格子形状
  ├── ▼ Rune Config                                 ← GE 行为规则 + Effects
  │     ├── Buff ID / Buff Tag
  │     ├── Buff Duration   (0=瞬发, -1=永久, >0=秒数)
  │     ├── Stack Type      (None/Refresh/Stack)
  │     ├── Max Stack       (仅 Stack 模式)
  │     ├── Stack Reduce Type (All/One)
  │     └── Effects[]       ← 效果片段列表（见下）
  └── ▼ Flow
        └── Buff Flow Asset   ← FA 资产引用
```

### RuneConfig.Effects[] — 效果片段

点击 `+` 选择类型，支持组合使用：

| 类型 | 用途 | 配置示例 |
|---|---|---|
| **Add Attribute Modifier** | 修改属性（加/乘/覆盖） | Attack +20, AttSpeed ×1.1 |
| **Add Gameplay Tags** | 激活期间授予 Tag，GE 移除时自动撤销 | `State.Berserk.Active` |
| **Trigger Gameplay Ability** | 装备时授予被动 GA（如击退GA）| 选择 GA 类 |
| **Gameplay Cue** | 挂载音效/特效标记 | 高级 |
| **Advanced Modifier** | 直接填 FGameplayModifierInfo | 高级 |
| **Execution Calculation** | GAS ExecutionCalculation | 高级 |

### 属性与 Knockback 说明

与 Buff 相关的 GAS 属性分布在：

| 属性 | 所在 AttributeSet | 含义 |
|---|---|---|
| Attack | BaseAttributeSet | 基础攻击力 |
| AttackSpeed | BaseAttributeSet | 攻击速度 |
| MoveSpeed | BaseAttributeSet | 移动速度 |
| Crit_Rate / Crit_Damage | BaseAttributeSet | 暴击率/倍率 |
| **KnockbackForce** | **BaseAttributeSet** | **攻击方施加的击退冲量** |
| KnockBackDist | EnemyAttributeSet | 敌方对击退的承受幅度 |
| DamagePhysical/Magic/Pure | DamageAttributeSet | 本次伤害值（瞬态） |

**击退符文配置示例：**
```
Effects:
  [0] Add Attribute Modifier
      Attribute = KnockbackForce
      ModOp     = Additive
      Value     = 600.0

  [1] Trigger Gameplay Ability
      AbilityClass = GA_Knockback    ← GA 从 AttributeSet 读 KnockbackForce
```

---

## 三、FA 结构（FlowAsset）

FA 是纯逻辑图，不引用 DA。它只做三件事：
1. **监听事件**（事件节点 → 触发输入）
2. **查询状态**（条件节点 → 数据引脚）
3. **执行动作**（效果节点 → 输出）

### 与 DA 的关系

```
DA_Rune_Berserk ─── Flow ──→ FA_Berserk
                              │
                              ├─ [OnDamageDealt]          ← 监听攻击事件
                              ├─ [AddRune] (DA_Berserk)   ← 施加/叠加 GE
                              ├─ [GetGEInfo] (Buff.Berserk)← 查询层数
                              ├─ [CompareFloat] (≥ 5)     ← 条件判断
                              └─ [AddEffect] (+10% ATK)   ← 超过阈值时触发
```

FA 不需要也不应该直接读取 DA 的配置字段——它通过 `GetGEInfo` 查询 ASC 上 GE 的运行时状态。

---

## 四、节点参考

### 事件类节点（起点）

#### `On Damage Dealt`
触发时机：**本角色造成伤害时**（SourceASC 广播）

输出引脚：`Out`  
上下文：Target = 受伤方，BuffOwner = 攻击方

---

#### `On Damage Received`
触发时机：**本角色受到伤害时**（TargetASC 广播）

输出引脚：`Out`

---

#### `On Crit Hit`
触发时机：**本角色打出暴击时**

输出引脚：`Out`

---

#### `On Kill`
触发时机：**本角色击杀目标时**

输出引脚：`Out`

---

#### `On Buff Added`
触发时机：**指定 Tag 的 GE 被施加到目标时**

配置项：`BuffTag`（监听哪个 Buff 的添加事件）  
输出引脚：`Out`

---

#### `On Buff Removed`
触发时机：**指定 Tag 的 GE 从目标上移除时**

配置项：`BuffTag`  
输出引脚：`Out`

---

#### `On Periodic`
触发时机：**每隔固定时间触发一次**

配置项：`Interval`（秒）  
输出引脚：`Out`

---

### 条件类节点

#### `Compare Float`
比较两个浮点数，支持从数据引脚接收左操作数。

| 配置项 | 说明 |
|---|---|
| `A` | 左操作数。可接受来自 `GetGEInfo` 等节点的数据引脚连线；无连线时使用本地填写的值 |
| `Operator` | `>` / `>=` / `==` / `<=` / `<` / `!=` |
| `B` | 右操作数（通常直接填阈值，如 `5`） |

输出引脚：`True` / `False`

**典型连线：**
```
GetGEInfo.StackCount ──→ CompareFloat.A
                         CompareFloat.B = 5
                         CompareFloat.Operator = >=
                         → True / False
```

---

#### `Get GE Info`  ⭐ 新增
查询目标 ASC 上匹配 BuffTag 的活跃 GE 的运行时信息，是 FA 中条件分支的核心节点。

| 配置项 | 说明 |
|---|---|
| `Target` | 查询目标（BuffOwner / Instigator / Target） |
| `BuffTag` | 要查询的 GE 的 Tag（与 DA 中 RuneConfig.BuffTag 一致） |

| 数据输出引脚 | 类型 | 说明 |
|---|---|---|
| `bIsActive` | Bool | GE 是否在目标上活跃 |
| `StackCount` | Int32 | 当前叠加层数（无叠加时为 1） |
| `Level` | Float | GE 等级 |
| `TimeRemaining` | Float | 剩余持续时间（秒），-1 = 永久 |

执行输出引脚：`Found` / `NotFound`

**用法示例（狂暴 — 超过 5 层时触发额外效果）：**
```
[OnDamageDealt]
  → [AddRune](DA_Berserk)              ← 叠加一层
  → [GetGEInfo](BuffTag=Buff.Berserk)
       ├─ Found
       │    → [CompareFloat](StackCount >= 5)
       │         ├─ True  → [AddEffect](+10% ATK, Instant)
       │         └─ False → [结束]
       └─ NotFound → [结束]
```

**用法示例（限时技 — 剩余时间 < 2s 时提示）：**
```
[OnPeriodic](Interval=0.5s)
  → [GetGEInfo](BuffTag=Buff.ShieldBash)
       ├─ Found
       │    → [CompareFloat](TimeRemaining < 2.0)
       │         └─ True → [PlayNiagara](NS_AboutToExpire)
       └─ NotFound → [结束]
```

---

#### `Has Tag`
检查目标是否携带某个 GameplayTag。

配置项：`Target`, `Tag`  
输出引脚：`True` / `False`

---

#### `Check Target Type`
检查目标是否为特定类型（玩家/敌人）。

输出引脚：`Player` / `Enemy` / `Other`

---

#### `Get Attribute`
获取目标的某个 AttributeSet 属性值，输出到 `CachedValue`（float）。

配置项：`Target`, `Attribute`  
输出引脚：`Out`

---

### 效果类节点

#### `Add Rune Effect`
根据 RuneDataAsset 施加 GE 到目标，并可选择启动关联 FA。

| 配置项 | 说明 |
|---|---|
| `RuneAsset` | DA_Rune 资产 |
| `Level` | GE 等级 |
| `Target` | 施加目标 |

数据输出引脚：`CachedRuneAsset`（Object，可连到 SendGameplayEvent）  
执行输出引脚：`Out` / `Failed`

---

#### `Remove Rune Effect`
移除目标上与 RuneAsset 关联的 GE 和 FA。

配置项：`RuneAsset`, `Target`  
输出引脚：`Out`

---

#### `Send Gameplay Event`
向目标发送 GameplayEvent，携带 RuneAsset 作为 OptionalObject，GA 可从事件中读取。

| 配置项 | 说明 |
|---|---|
| `Target` | 事件目标 |
| `EventTag` | 发送的事件 Tag |
| `RuneAsset` | 携带的 DA（可接受数据引脚连线） |
| `EventMagnitude` | 附加数值（GA 可直接读取） |

输出引脚：`Out` / `Failed`

**典型用法（FA 向 GA 传递触发信号）：**
```
[OnDamageDealt]
  → [AddRune](DA_Knockback)               ← 施加击退 GE
  → [SendGameplayEvent]
       EventTag     = Event.Combat.Knockback
       RuneAsset    ← AddRune.CachedRuneAsset  (连线)
       EventMagnitude = 1.0
  → GA_Knockback 通过 ActivateAbilityFromEvent 接收
```

---

#### `Apply Effect`  （旧：Add Effect）
向目标施加一个 GE 类。

配置项：`Target`, `GameplayEffectClass`, `Level`  
输出引脚：`Out`

---

#### `Do Damage`
向目标施加伤害。

配置项：`Target`, `DamageAmount`, `DamageType`  
输出引脚：`Out`

---

#### `Add Tag` / `Remove Tag`
动态给目标添加/移除 GameplayTag。

输出引脚：`Out`

---

#### `Finish Buff`
终止当前 BuffFlow 实例。通常放在 FA 的末尾或作为提前退出节点。

---

### 视觉/音频类节点

#### `Play Niagara`
在目标位置播放 Niagara 粒子系统。

配置项：`NiagaraSystem`, `Target`, `AttachSocket`

---

#### `Destroy Niagara`
停止并销毁之前播放的粒子系统。

---

#### `Play Montage`
在目标上播放动画蒙太奇。

---

#### `Spawn Actor At Location`
在指定位置生成 Actor（子弹、AOE 碰撞体等）。

---

---

## 五、完整使用案例

### 案例 1：狂暴（叠加层数 → 超过阈值触发额外效果）

**DA_Rune_Berserk 配置：**
```
Buff Tag:   Buff.Berserk
Duration:   5s
Stack Type: Stack (叠加)
Max Stack:  10
Stack Reduce Type: One (逐层移除)

Effects:
  [0] Add Attribute Modifier
      Attribute = AttackSpeed
      ModOp     = Additive
      Value     = 0.05   (每层 +5% 攻速)
```

**FA_Berserk 逻辑：**
```
[OnDamageDealt]
  → [AddRune] (DA_Berserk, Target=Target)
  → [GetGEInfo] (BuffTag=Buff.Berserk, Target=BuffOwner)
       └─ Found
            → [CompareFloat] (StackCount >= 5)
                 ├─ True → [Add Tag] (State.Berserk.MaxStacks)
                 │          → [Apply Effect] (GE_BonusATK10Pct, Instant)
                 └─ False → [结束]
```

---

### 案例 2：击退（符文增强击退力，GA 读取属性执行）

**DA_Rune_Knockback 配置：**
```
Buff Tag:  Buff.Knockback
Duration:  -1 (永久，装备时持续生效)

Effects:
  [0] Add Attribute Modifier
      Attribute = KnockbackForce
      ModOp     = Additive
      Value     = 600.0

  [1] Trigger Gameplay Ability
      AbilityClass = GA_Knockback
```

**GA_Knockback 逻辑（蓝图）：**
```
ActivateAbility
  → Get Ability System Component (自身)
  → Get Numeric Attribute (KnockbackForce)
  → Launch Character (方向 * KnockbackForce)
```

> GA 不从 DA 读取参数——它直接读 AttributeSet 的当前值。  
> GE 把 +600 写进 KnockbackForce，GA 读出来就是加成后的结果。

---

### 案例 3：持续 DoT + 层数可视化

**DA_Rune_Poison 配置：**
```
Buff Tag:    Buff.Poison
Duration:    8s
Stack Type:  Stack
Max Stack:   5

Effects:
  [0] Add Attribute Modifier
      Attribute = Health
      ModOp     = Additive
      Value     = -15.0   (每层每次 -15 HP)
      (Period 由 GE 的 DurationMagnitude 配置)
```

**FA_Poison 逻辑：**
```
[OnPeriodic] (Interval=1.0s)
  → [GetGEInfo] (BuffTag=Buff.Poison, Target=BuffOwner)
       └─ Found
            → [Play Niagara] (NS_PoisonTick, 粒子随层数变色)
            → [CompareFloat] (StackCount >= 5)
                 └─ True → [Play Niagara] (NS_PoisonOverdose, 超量特效)
```

---

### 案例 4：护盾即将到期提醒

**FA_Shield：**
```
[OnPeriodic] (Interval=0.5s)
  → [GetGEInfo] (BuffTag=Buff.Shield, Target=BuffOwner)
       ├─ Found
       │    → [CompareFloat] (TimeRemaining <= 2.0)
       │         └─ True → [Play Niagara] (NS_ShieldWarning)
       └─ NotFound → [Finish Buff]
```

---

## 六、常见问题

**Q：FA 里可以施加新的 GE 吗？**  
A：可以，用 `Add Rune Effect` 或 `Apply Effect` 节点。但注意 DA→FA→DA 的循环引用：  
- DA 的 `Flow` 字段指向 FA（这是正常的）
- FA 里的 `Add Rune Effect` 可以引用另一个 DA（没有循环）
- FA 不应引用它所属的同一个 DA（会无限循环）

**Q：FA 怎么获取 DA 的配置数值（如击退距离）？**  
A：不直接读 DA——通过 GAS 属性系统间接获取：
- DA 的 Effects 里用 `Add Attribute Modifier` 把值写进 AttributeSet
- FA 用 `Get Attribute` 节点读出属性当前值（已含所有 GE 的叠加）
- 或者 GA 在执行时直接读 AttributeSet

**Q：`Get GE Info` 的 BuffTag 填哪个？**  
A：填 DA_Rune 里 `Rune Config → Buff Tag` 字段的值。两处要完全一致。

**Q：CompareFloat.A 怎么连数据引脚？**  
A：在 Flow 编辑器里，从 `GetGEInfo` 节点的 `StackCount` 输出引脚拖线到 `CompareFloat` 节点的 `A` 输入引脚即可。`B` 直接在节点属性里填阈值数字。

---

## 七、架构变更记录（2026-04-05）

### 本次重构内容

| 变更 | 之前 | 现在 |
|---|---|---|
| DA 效果配置 | `FRuneAttributeModifier[]` + `FRuneCalcSpec[]` + `Params TMap` | `Effects[]`（`URuneEffectFragment` 多态组件） |
| GE 堆叠配置 | 直接填 GE 枚举 | `StackType`（Refresh/Stack/None）+ `StackReduceType` + `BuffDuration` float |
| 被动GA配置 | `FRuneFlowConfig.PassiveAbilityClass` | 改到 `Effects[]` 中的 `Trigger GA Fragment` |
| CompareFloat | A/B 为 `float`（无法接数据引脚） | A/B 为 `FFlowDataPinInputProperty_Float`（可接线） |
| 新增节点 | — | `Get GE Info`（查询 GE 运行时信息） |
| 新增属性 | — | `KnockbackForce`（BaseAttributeSet） |

### 策划需重新配置的内容
1. 所有 `DA_Rune_XXX` 中的 `BuffConfig` 改为新的 `RuneConfig`（Duration float 替代 DurationPolicy 枚举）
2. `Values → Attribute Modifiers` 迁移到 `RuneConfig → Effects → Add Attribute Modifier`
3. `Flow → Passive Ability Class` 迁移到 `RuneConfig → Effects → Trigger GA`
4. Flow Graph 中已放置的 `CompareFloat` 节点需重新连线（A/B 类型已变）
