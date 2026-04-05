# BuffFlow 系统指南

> 更新日期：2026-04-05 (rev3)  
> 对象：策划 + 程序

---

## 一、系统概述

**BuffFlow = 可视化 Buff/Effect 逻辑系统。**

它解决了一个核心问题：符文的数值配置很简单（填表），但符文的"行为逻辑"很复杂（什么时候触发、触发条件、触发后做什么）。BuffFlow 把这部分逻辑从代码中抽出来，放进可视化的 Flow Graph 里，让策划能直接看到并修改符文的行为。

### 两层分离原则

```
DA（Data Asset）= 数据层        FA（Flow Asset）= 逻辑层
─────────────────────────────   ─────────────────────────────
· 符文展示信息（名称/图标）    · 什么时候触发
· GE 行为规则（Duration/Stack） · 触发条件判断（层数/属性）
· Effects[]（属性修改/Tag）      · 授予 GA（GrantGA 节点）
· 背包格子形状                  · 触发后的特效/音效/即时效果
─────────────────────────────   ─────────────────────────────
          策划填表配置                    策划用节点搭建
```

**FA 不引用 DA。** FA 通过 `GetRuneInfo` 查询 ASC 上 GE 的运行时状态，而不是读 DA 的字段，也不应在 FA 里再次 apply 本符文的 GE。

**GAS 数值流向：**
```
GE（施加）→ AttributeSet（存储）→ GA（读取并使用）
```
- GE 是数据写入者，负责把数值写进 AttributeSet
- GA 是数值使用者，执行时从 AttributeSet 读取当前值
- FA 是行为决策者，决定 GA 何时授予、条件是什么、何时触发特效

---

## 二、DA 结构（RuneDataAsset）

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description          ← 展示信息
  ├── Shape                                   ← 背包格子形状
  ├── ▼ Rune Config                           ← GE 行为规则 + Effects
  │     ├── Rune Type     (增益/减益/无，UI 分类)
  │     ├── Duration Type (瞬发/永久/有时限)
  │     ├── Duration      (秒，仅 有时限 时显示)
  │     ├── Period        (周期触发间隔，DoT/HoT 用)
  │     ├── Unique Type   (唯一/源唯一/非唯一)
  │     ├── Max Stack     (仅 叠加 模式时显示)
  │     ├── Stack Type    (刷新/叠加/禁止，仅 唯一/源唯一 时显示)
  │     ├── Stack Reduce Type (全部/逐一)
  │     ├── Rune ID       (数值 ID，策划表引用)
  │     ├── Rune Tag      (GameplayTag，唯一标识此符文 GE)
  │     └── Effects[]     ← 效果片段列表（见下）
  └── ▼ Flow
        └── Flow Asset    ← FA 资产引用
```

### RuneConfig.Effects[] — 效果片段

点击 `+` 选择类型，支持组合使用：

| 类型 | 用途 | 配置示例 |
|---|---|---|
| **Add Attribute Modifier** | 修改属性（加/乘/覆盖） | Attack +20, KnockbackForce +600 |
| **Add Gameplay Tags** | 激活期间授予状态 Tag，GE 移除时自动撤销 | `State.Berserk.Active` |
| **Gameplay Cue** | 挂载音效/特效标记 | 高级 |
| **Advanced Modifier** | 直接填 FGameplayModifierInfo | 高级 |
| **Execution Calculation** | GAS ExecutionCalculation | 高级 |

> **GA 授予在 FA 层完成。** 在 FA 里使用 `Grant GA` 节点授予被动 GA，无需在 DA 的 Effects[] 中配置。

### 属性说明

与符文相关的 GAS 属性分布：

| 属性 | 所在 AttributeSet | 含义 |
|---|---|---|
| Attack | BaseAttributeSet | 基础攻击力 |
| AttackSpeed | BaseAttributeSet | 攻击速度 |
| MoveSpeed | BaseAttributeSet | 移动速度 |
| Crit_Rate / Crit_Damage | BaseAttributeSet | 暴击率/倍率 |
| **KnockbackForce** | **BaseAttributeSet** | **攻击方施加的击退冲量** |
| KnockBackDist | EnemyAttributeSet | 敌方对击退的承受幅度 |
| DamagePhysical/Magic/Pure | DamageAttributeSet | 本次伤害值（瞬态） |

---

## 三、FA 结构（FlowAsset）

FA 是纯逻辑图，**不引用 DA**。它只做三件事：
1. **监听事件**（事件节点 → 触发输入）
2. **查询状态**（条件节点 → 数据引脚）
3. **执行动作**（效果节点 → 输出）

FA 通过 `Get Rune Info` 查询 ASC 上 GE 的运行时状态，不应在 FA 里再次 apply 本符文自己的 GE（会形成 DA→FA→DA 循环）。

**FA 能做什么、不能做什么：**

| ✅ FA 应该做 | ❌ FA 不应该做 |
|---|---|
| 监听事件（OnDamageDealt 等） | 引用本符文的 DA |
| 用 GetRuneInfo 查询 GE 状态 | 在 FA 里 apply 本符文的 GE |
| 用 ApplyEffect 施加独立 GE 类 | 读取 DA 的配置字段 |
| GrantGA 授予被动 GA | — |
| 播放粒子/音效 | — |

```
DA_Rune_Berserk ─── Flow ──→ FA_Berserk
                              │
                              ├─ [OnDamageDealt]
                              ├─ [GetRuneInfo](Rune.Berserk)    ← 查询层数
                              ├─ [CompareFloat](StackCount >= 5)
                              └─ [ApplyEffect](GE_BonusATK)     ← 超阈值触发即时效果
```

---

## 四、节点参考

### 事件类节点（起点）

#### `On Damage Dealt`
触发时机：**本角色造成伤害时**

输出引脚：`Out` | 上下文：Target = 受伤方，BuffOwner = 攻击方

---

#### `On Damage Received`
触发时机：**本角色受到伤害时**

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

配置项：`Tag`（监听哪个符文 GE 的添加事件）| 输出引脚：`Out`

---

#### `On Buff Removed`
触发时机：**指定 Tag 的 GE 从目标上移除时**

配置项：`Tag` | 输出引脚：`Out`

---

#### `On Periodic`
触发时机：**每隔固定时间触发一次**

配置项：`Interval`（秒）| 输出引脚：`Out`

---

### 条件类节点

#### `Compare Float`
比较两个浮点数，左操作数可接受数据引脚连线。

| 配置项 | 说明 |
|---|---|
| `A` | 左操作数（可接 `GetRuneInfo.StackCount` 等数据引脚，无连线时用本地值） |
| `Operator` | `>` / `>=` / `==` / `<=` / `<` / `!=` |
| `B` | 右操作数（直接填阈值，如 `5`） |

输出引脚：`True` / `False`

**典型连线：**
```
GetRuneInfo.StackCount ──→ CompareFloat.A
                           CompareFloat.B = 5, Operator = >=
                           → True / False
```

---

#### `Get Rune Info`  ⭐
查询目标 ASC 上匹配 `RuneTag` 的符文 GE 的运行时状态。是 FA 中条件分支的核心节点。

| 配置项 | 说明 |
|---|---|
| `Target` | 查询目标（BuffOwner / Instigator / Target） |
| `RuneTag` | DA 中 `RuneConfig.RuneTag` 填写的 Tag，两处必须一致 |

| 数据输出引脚 | 类型 | 说明 |
|---|---|---|
| `bIsActive` | Bool | 符文 GE 是否活跃 |
| `StackCount` | Int32 | 当前叠加层数 |
| `Level` | Float | GE 等级 |
| `TimeRemaining` | Float | 剩余持续时间（秒），-1 = 永久 |

执行输出引脚：`Found` / `NotFound`

**用法示例（狂暴 — 激活时持续加攻速，命中 5 次后触发额外效果）：**
```
[OnDamageDealt]
  → [GetRuneInfo](RuneTag=Rune.Berserk, Target=BuffOwner)
       ├─ Found
       │    → [CompareFloat](StackCount >= 5)
       │         ├─ True  → [ApplyEffect](GE_BonusATK10Pct, Instant)
       │         └─ False → [结束]
       └─ NotFound → [结束]
```

> 注：GE 的叠加由 BackpackGrid 激活时施加（stack=1），后续叠层机制见"符文模型"说明。
> FA 里 **不引用 DA**，只通过 `GetRuneInfo` 读取运行时状态。

---

#### `Has Tag`
检查目标是否携带某个 GameplayTag。

配置项：`Target`, `Tag` | 输出引脚：`True` / `False`

---

#### `Check Target Type`
检查目标是否为特定类型。

输出引脚：`Player` / `Enemy` / `Other`

---

#### `Get Attribute`
获取目标的某个属性值，输出到 `CachedValue`（float）。

配置项：`Target`, `Attribute` | 输出引脚：`Out`

---

### 效果类节点

#### `Add Rune Effect`
根据 RuneDataAsset 施加 GE 到目标，并可选择启动关联 FA。

| 配置项 | 说明 |
|---|---|
| `RuneAsset` | DA_Rune 资产 |
| `Level` | GE 等级 |
| `Target` | 施加目标 |

数据输出引脚：`CachedRuneAsset`（Object）  
执行输出引脚：`Out` / `Failed`

---

#### `Remove Rune Effect`
移除目标上与 RuneAsset 关联的 GE 和 FA（通过 `RuneTag` 匹配）。

配置项：`RuneAsset`, `Target` | 输出引脚：`Out`

---

#### `Grant GA`  ⭐
向目标授予 GameplayAbility，**FA 停止时自动撤销**（无需手动连接撤销逻辑）。

| 配置项 | 说明 |
|---|---|
| `AbilityClass` | 要授予的 GA 类（继承自 RunePassiveAbility） |
| `Target` | 授予目标（通常为 BuffOwner） |
| `AbilityLevel` | GA 等级（默认 1） |

输出引脚：`Out`（授予成功）/ `Failed`（无效目标或无 ASC）

**设计说明：**  
节点内部持有 `FGameplayAbilitySpecHandle`。当 BuffFlow 实例停止时（`Cleanup()` 被调用），自动执行 `ASC->ClearAbility(Handle)`。FA 图不需要显式连接"撤销"逻辑。

**典型用法（击退符文 FA）：**
```
[Start]
  → [GrantGA](GA_Knockback, Target=BuffOwner)
       └─ Out → [OnDamageDealt]
                    → [PlayNiagara](NS_KnockbackImpact, Target=Target)
                    → [结束]

── 符文卸下时 FA 停止，GrantGA.Cleanup() 自动撤销 GA_Knockback ──
```

GA_Knockback 需配置 AbilityTrigger（`Event.Combat.Knockback`），  
由攻击 GA 在命中时发送该 Event 以激活击退。

---

#### `Apply Effect`
向目标施加一个 GE 类（直接填 GE 类，不经过 DA）。

配置项：`Target`, `GameplayEffectClass`, `Level` | 输出引脚：`Out`

---

#### `Do Damage`
向目标施加伤害。

配置项：`Target`, `DamageAmount`, `DamageType` | 输出引脚：`Out`

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

#### `Destroy Niagara` / `Play Montage` / `Spawn Actor At Location`
粒子销毁 / 动画蒙太奇 / 生成 Actor（子弹、AOE 碰撞体等）。

---

## 五、完整使用案例

### 案例 1：狂暴（装备时持续加攻速，命中触发额外爆发）

**设计思路：**
狂暴符文装备时 GE 永久生效，持续提升攻速。FA 只做事件监听 + 条件判断，不负责叠加 GE。若需要"命中计数"达到阈值的逻辑，用一个单独的即时 GE 来记录计数，或直接在 FA 里做简单的命中触发。

**DA_Rune_Berserk 配置：**
```
Rune Type:     增益
Duration Type: 永久
Unique Type:   唯一
Stack Type:    刷新
Rune Tag:      Rune.Berserk

Effects:
  [0] Add Attribute Modifier
      Attribute = AttackSpeed
      ModOp     = Additive
      Value     = 0.15   (装备时持续 +15% 攻速)
```

**FA_Berserk 逻辑（命中时触发额外爆发伤害）：**
```
[OnDamageDealt]
  → [GetRuneInfo](RuneTag=Rune.Berserk, Target=BuffOwner)
       ├─ Found → [ApplyEffect](GE_BerserkBurst, Instant, Target=Target)
       └─ NotFound → [结束]
```

> FA 只查询"符文是否在身上"，确认后直接触发效果。
> 不在 FA 里引用 DA，不在 FA 里 apply 本符文的 GE。

**若需要"命中 N 次才触发"：** 改用 `GE_BerserkStack`（独立 GE 类，StackType=叠加，MaxStack=5），FA 用 `ApplyEffect(GE_BerserkStack)` 叠层，再用 `GetRuneInfo(RuneTag=Rune.BerserkStack)` 查层数。此时 FA 引用的是 **GE 类**而非 DA，分离原则不受破坏。

---

### 案例 2：击退（GE 增强击退力，GA 读取属性执行）

**DA_Rune_Knockback 配置：**
```
Rune Type:     无（功能性，非显示型）
Duration Type: 永久
Unique Type:   唯一
Stack Type:    刷新
Rune Tag:      Rune.Knockback

Effects:
  [0] Add Attribute Modifier
      Attribute = KnockbackForce
      ModOp     = Additive
      Value     = 600.0
      ← GA 授予在 FA 里用 GrantGA 节点完成，不在此处配置
```

**FA_Knockback 配置：**
```
[Start]
  → [GrantGA](GA_Knockback, Target=BuffOwner)
       └─ Out → [OnDamageDealt]
                    → [PlayNiagara](NS_KnockbackImpact, Target=Target)
                    → [结束]
```

**GA_Knockback 蓝图逻辑（ActivateAbilityFromEvent）：**
```
EventData.Instigator → Get ASC → GetNumericAttribute(KnockbackForce) → AttackerForce
EventData.Target     → Cast to Character
方向 = (Target.Pos - Instigator.Pos).GetSafeNormal()
Launch Character(Target, 方向 * AttackerForce)
End Ability
```

> GA 直接读 AttributeSet 的 `KnockbackForce`，GE 已把 +600 写进去了，无需从 DA 传参。

---

### 案例 3：持续 DoT（Period 驱动周期伤害）

**DA_Rune_Poison 配置：**
```
Rune Type:        减益
Duration Type:    有时限
Duration:         8s
Period:           1.0   ← 每秒触发一次 Effect（GAS 周期机制）
Unique Type:      唯一
Stack Type:       叠加
Max Stack:        5
Stack Reduce Type: 全部
Rune Tag:         Rune.Poison

Effects:
  [0] Add Attribute Modifier
      Attribute = Health
      ModOp     = Additive
      Value     = -15.0  (每周期每层 -15 HP，由 GAS Period 驱动)
```

**FA_Poison 逻辑（可视化层数）：**
```
[OnPeriodic](Interval=1.0s)
  → [GetRuneInfo](RuneTag=Rune.Poison, Target=BuffOwner)
       └─ Found
            → [PlayNiagara](NS_PoisonTick)
            → [CompareFloat](StackCount >= 5)
                 └─ True → [PlayNiagara](NS_PoisonOverdose)
```

---

### 案例 4：护盾即将到期提醒

**FA_Shield：**
```
[OnPeriodic](Interval=0.5s)
  → [GetRuneInfo](RuneTag=Rune.Shield, Target=BuffOwner)
       ├─ Found
       │    → [CompareFloat](TimeRemaining <= 2.0)
       │         └─ True → [PlayNiagara](NS_ShieldWarning)
       └─ NotFound → [FinishBuff]
```

---

## 六、常见问题

**Q：FA 里可以施加新的 GE 吗？**  
A：可以，用 `Add Rune Effect` 或 `Apply Effect` 节点。但注意 DA→FA→DA 的循环引用——FA 不应引用它所属的同一个 DA（会无限循环）。

**Q：FA 怎么获取 DA 的配置数值（如击退力大小）？**  
A：不直接读 DA——通过 GAS 属性系统间接获取：
- DA 的 Effects 里用 `Add Attribute Modifier` 把值写进 AttributeSet
- FA 用 `Get Attribute` 节点读出属性当前值（已含所有 GE 的叠加）
- 或者 GA 在执行时直接读 AttributeSet

**Q：`Get Rune Info` 的 RuneTag 填哪个？**  
A：填 DA_Rune 里 `Rune Config → Rune Tag` 字段的值，两处要完全一致。

**Q：CompareFloat.A 怎么连数据引脚？**  
A：在 Flow 编辑器里，从 `GetRuneInfo` 节点的 `StackCount` 输出引脚拖线到 `CompareFloat` 节点的 `A` 输入引脚即可。`B` 直接在节点属性里填阈值数字。

**Q：DoT 的伤害周期用 DA 的 Period 还是 FA 的 OnPeriodic？**  
A：两者用途不同：
- DA 的 `Period` = GAS 内置周期机制，直接驱动 GE 的 Effects 重复触发（无需 FA 介入）
- FA 的 `OnPeriodic` = 用于 **可视化反馈**（特效、音效、UI 更新），Interval 可与 Period 不同

---

## 七、架构变更记录

### rev3（2026-04-05）
| 变更 | 之前 | 现在 |
|---|---|---|
| 持续时间 | `BuffDuration: float`（0/-1/>0 编码） | `DurationType` 枚举 + `Duration` float |
| 新增字段 | — | `Period`（周期触发间隔）|
| 新增字段 | — | `UniqueType`（唯一/源唯一/非唯一 → GAS StackingType）|
| 符文类型 | `BuffType` 在 FRuneInstance | `RuneType` 移入 FRuneConfig |
| 字段重命名 | `BuffID/BuffTag/BuffFlowAsset` | `RuneID/RuneTag/FlowAsset` |
| 枚举重命名 | `ERuneBuffType` | `ERuneType` |

### rev2（2026-04-05）
| 变更 | 之前 | 现在 |
|---|---|---|
| GA 授予 | DA Effects[] 中的 Trigger GA Fragment | FA 的 `Grant GA` 节点 |
| 节点重命名 | `Get GE Info` / `BFNode_GetGEInfo` | `Get Rune Info` / `BFNode_GetRuneInfo` |
| 删除节点 | `Send Gameplay Event` | 由 `Grant GA` + AbilityTrigger 取代 |
| 删除系统 | `EffectRegistry`（Tag→DA 注册表） | 直接在 FA 用 `AddRune` 节点引用 DA |
| 新增属性 | — | `KnockbackForce`（BaseAttributeSet）|
