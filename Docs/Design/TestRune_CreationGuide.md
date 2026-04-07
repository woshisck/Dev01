# 测试符文制作指南

> 本文档记录 6 个测试符文的完整制作步骤。  
> Tag 命名规范遵循 [`Buff_Tag_Spec.md`](Buff_Tag_Spec.md)。  
> 制作过程中请在文档内直接标注反馈（`[反馈]` 开头），以便后续根据实际习惯优化工作流。

---

## 前置：GameplayTag 创建

**路径：** Edit → Project Settings → Gameplay Tags → 点 + 逐条添加

Tag 按照 `Buff_Tag_Spec.md` 的四层职责模型划分，**每层只做一件事**：

| 层 | 前缀 | 职责 | 贴在哪里 |
|---|---|---|---|
| 身份层 | `Buff.Rune.*` | 这个符文**是什么**（类型/稀有度） | DA 的 IdentityTags |
| 行为层 | `Buff.Effect.*` | 这个 GE **做了什么** | GE 的 Asset Tags |
| 触发层 | `Buff.Trigger.*` | 这个效果**在何时生效** | GA 的 Asset Tags |
| 状态层 | `Buff.Status.*` | 角色**现在的运行时状态** | GE 的 Granted Tags → 授予目标 ASC |
| 参数层 | `Buff.Data.*` | **SetByCaller 的键名**，FA 运行时向 GE 传值 | GE Modifier 的 SetByCaller 槽 + BFNode_ApplyEffect 的 SetByCallerTag 槽 |

> **参数层说明：**  
> `Buff.Data.*` 不挂在任何角色身上，也不描述状态或效果。它是 FA（BFNode_ApplyEffect）和 GE 之间的**数据通道**：  
> - GE Modifier 的 Magnitude Type 选 `SetByCaller`，填入 `Buff.Data.AttributeMod` 等 Tag  
> - FA 的 `BFNode_ApplyEffect` 节点在 SetByCallerTag 槽填同一个 Tag，Value 槽填实际数值  
> - 这样同一个 GE 资产就能被不同符文或阶段以不同数值复用

### 本次需要创建的 Tag

> ★ = Buff_Tag_Spec.md 中已有；☆ = 本次新增（需补充到 Spec）

#### 行为层 `Buff.Effect.*`（贴在 GE 的 Asset Tags，同时填入 DA.RuneConfig.RuneEffectTag）

| Tag | 状态 | 用于哪个符文 | 说明 |
|---|---|---|---|
| `Buff.Effect.Attribute.MoveSpeed` | ★ 已有 | 符文 3 移速堆叠、符文 4 击退硬直 | 修改移动速度属性的 GE |
| `Buff.Effect.Attribute.Attack` | ☆ 新增 | 符文 1 攻击提升 | 修改攻击力属性的 GE |
| `Buff.Effect.Attribute.Heat` | ☆ 新增 | 符文 2 热度提升 | 修改热度属性的 GE |
| `Buff.Effect.Bleed` | ☆ 新增 | 符文 5 流血 | 流血效果 GE 的身份标识 |

> **DevComment（新增 Tag 时填写）：**  
> `Buff.Effect.Attribute.Attack`：`【Effect-Attr】修改攻击力属性（对应 GAS AttributeSet.Attack）`  
> `Buff.Effect.Attribute.Heat`：`【Effect-Attr】修改热度属性（对应 GAS AttributeSet.Heat）`  
> `Buff.Effect.Bleed`：`【Effect】流血效果 GE，配合 Buff.Status.Bleeding 状态使用`

#### 状态层 `Buff.Status.*`（GE 通过 Granted Tags 授予目标，GE 移除时自动消失）

| Tag | 状态 | 用于哪个符文 | 说明 |
|---|---|---|---|
| `Buff.Status.Knockback` | ☆ 新增 | 符文 4 击退 | 瞬时信号：加给目标 → GA_Knockback 激活 → 执行冲量 → 移除 |
| `Buff.Status.Bleeding` | ☆ 新增 | 符文 5 流血 | 持续状态：GE_Bleeding 存在期间目标持有，GA_Bleed 以此为激活条件 |
| `Buff.Status.ExtraDamageApplied` | ☆ 新增 | 符文 6 额外扣血 | 防递归守卫：同帧内有此 Tag 则跳过额外伤害逻辑 |

> **DevComment（新增 Tag 时填写）：**  
> `Buff.Status.Knockback`：`【Status】击退触发信号，由 FA AddTag 节点写入，GA_Knockback 消费后立即移除`  
> `Buff.Status.Bleeding`：`【Status】流血状态，由 GE_Bleeding 通过 Granted Tags 授予，GE 移除时自动消失`  
> `Buff.Status.ExtraDamageApplied`：`【Status】额外伤害递归防护守卫，同帧有此 Tag 则跳过，由 FA 手动管理`

#### 参数层 `Buff.Data.*`（SetByCaller 键名，GE Modifier + FA 节点配对使用）

| Tag | 状态 | 用于哪个符文 | 说明 |
|---|---|---|---|
| `Buff.Data.AttributeMod` | ★ 已有 | 符文 4 击退硬直 | 属性修改量，GE Modifier SetByCaller 键 + FA 节点运行时填值 |

#### 身份层 `Buff.Rune.*`（贴在 DA，当前为规范预留，DA 暂无 IdentityTags 字段）

| Tag | 状态 | 用于哪个符文 |
|---|---|---|
| `Buff.Rune.Type.Attack` | ★ 已有 | 符文 1 攻击提升、符文 4 击退、符文 5 流血、符文 6 额外扣血 |
| `Buff.Rune.Type.Utility` | ★ 已有 | 符文 2 热度提升、符文 3 移速堆叠 |
| `Buff.Rune.Rarity.Common` | ★ 已有 | 全部 6 个测试符文 |

> **注：** 当前 `RuneDataAsset` 暂无 `IdentityTags` 字段，`Buff.Rune.*` Tag 填写暂时跳过，待 DA 结构扩展后补充。

---

## 符文 1：永久攻击提升

**效果：** 符文在激活区内时，攻击力永久 +10。离开激活区立即失效。  
**触发方式：** 被动常驻（`Buff.Trigger.Passive`）

### 1-1 创建 GE：`GE_Rune_AttackUp`

路径：Content Browser 右键 → Gameplay → Gameplay Effect Blueprint

| 配置项 | 值 | 意义 |
|---|---|---|
| Duration Policy | **Infinite** | 永久生效，不自动到期 |
| Stacking Type | **Aggregate By Target** | 同一目标只保留一个实例，防止重复叠加 |
| Stack Limit Count | `1` | 最多 1 层 |
| Stack Duration Refresh | **Refresh On Successful Application** | 重复施加时刷新（Infinite 下无实际影响） |
| **Modifiers** → 添加一条 | Attribute = `Attack`，Op = **Additive**，Magnitude = `10` | 攻击力 +10 |
| **Components** → 添加 `Asset Tags Gameplay Effect Component` | Tag：`Buff.Effect.Attribute.Attack` | **行为层**：标识这个 GE 修改了攻击属性，同时供 GetSelfRuneInfo 节点查询 |

### 1-2 创建 FA：`FA_Rune_AttackUp`

路径：Content Browser 右键 → Flow Asset

Graph 连接：
```
[Start] ──→ [Apply Gameplay Effect Class]
                Effect   = GE_Rune_AttackUp
                Level    = 1.0
                Target   = Buff拥有者
```

> **意义：** FA 启动时立即施加 Infinite GE。FA 停止时（符文离开激活区），`ApplyEffect` 节点的 `Cleanup()` 自动移除 GE，攻击力恢复原值。

### 1-3 创建 DA：`DA_Rune_AttackUp`

路径：Content Browser 右键 → Miscellaneous → Data Asset → `RuneDataAsset`

| 字段 | 值 | 说明 |
|---|---|---|
| Rune Name | `AttackUp` | |
| Rune Icon | （选图标） | |
| Rune Description | 永久提升攻击力 | |
| Shape.Cells | `(0,0)` | 1 格符文 |
| RuneConfig.RuneType | `Buff` | |
| RuneConfig.RuneID | `1001` | |
| RuneConfig.RuneEffectTag | `Buff.Effect.Attribute.Attack` | 必须与 GE 的 Asset Tag 一致，GetSelfRuneInfo 依赖此 Tag 查询 GE |
| Flow.FlowAsset | `FA_Rune_AttackUp` | |

---

## 符文 2：永久热度提升

**效果：** 符文在激活区内时，热度属性 +50。  
**触发方式：** 被动常驻（`Buff.Trigger.Passive`）  
**注：** 热度增加会推动升阶判定，属预期行为。后续若需禁止升阶，可在 `BFNode_OnPhaseUpReady` 加 `Buff.Status.*` 检查。

### 2-1 创建 GE：`GE_Rune_HeatUp`

| 配置项 | 值 |
|---|---|
| Duration Policy | **Infinite** |
| Stacking | Aggregate By Target，Limit=1，Refresh |
| **Modifiers** | Attribute = `Heat`，Op = **Additive**，Magnitude = `50` |
| Asset Tags Component | Tag = `Buff.Effect.Attribute.Heat` |

### 2-2 创建 FA：`FA_Rune_HeatUp`

```
[Start] ──→ [Apply Gameplay Effect Class]
                Effect = GE_Rune_HeatUp
                Level  = 1.0
                Target = Buff拥有者
```

### 2-3 创建 DA：`DA_Rune_HeatUp`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1002` |
| RuneConfig.RuneEffectTag | `Buff.Effect.Attribute.Heat` |
| Flow.FlowAsset | `FA_Rune_HeatUp` |

---

## 符文 3：攻击获得移速堆叠

**效果：** 每次攻击命中敌人后，自身获得移速 +30，持续 3 秒，最多叠 5 层（+150），逐层衰减。  
**触发方式：** 命中触发（`Buff.Trigger.OnHit`）

### 3-1 创建 GE：`GE_Rune_SpeedStack`

| 配置项 | 值 | 意义 |
|---|---|---|
| Duration Policy | **Has Duration**，Duration = `3.0` | 每层持续 3 秒 |
| Stacking Type | **Aggregate By Target** | 同目标共享一个 GE 实例，支持叠层 |
| Stack Limit Count | `5` | 最多 5 层 |
| Stack Duration Refresh | **Refresh On Successful Application** | 每次命中刷新整体计时 |
| Stack Expiration Policy | **Remove Single Stack And Refresh Duration** | 到期只减一层，剩余层重置计时 |
| **Modifiers** | Attribute = `MoveSpeed`，Op = **Additive**，Magnitude = `30` | 每层 +30 移速 |
| Asset Tags Component | Tag = `Buff.Effect.Attribute.MoveSpeed` | **行为层**（Spec 已有此 Tag） |

### 3-2 创建 FA：`FA_Rune_SpeedStack`

```
[OnDamageDealt] ──→ [Apply Gameplay Effect Class]
                        Effect = GE_Rune_SpeedStack
                        Level  = 1.0
                        Target = Buff拥有者        ← 移速给自己，不是给敌人
```

> **意义：** `OnDamageDealt` 每次对外造成伤害时触发，对自身叠加一层移速 GE。GAS 堆叠机制自动管理层数和倒计时。

### 3-3 创建 DA：`DA_Rune_SpeedStack`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1003` |
| RuneConfig.RuneEffectTag | `Buff.Effect.Attribute.MoveSpeed` |
| Flow.FlowAsset | `FA_Rune_SpeedStack` |

---

## 符文 4：击退（含 SetByCaller 示例）

**效果：** 每次命中敌人，对其施加一次物理击退，同时减少目标 300 点移速持续 1 秒（击退硬直）。  
**触发方式：** 命中触发（`Buff.Trigger.OnHit`）  
**机制：**  
1. FA 施加 `GE_KnockbackStagger`（含 SetByCaller 移速减少量）→ GAS 写入目标属性  
2. FA 给目标写入 `Buff.Status.Knockback` 信号 Tag → 敌人 GA_Knockback 激活 → 施加冲量 → 移除 Tag

> **前提：** `GA_Knockback` 需要预先授予所有敌人角色。

> **SetByCaller 要点：**  
> GE 的 Modifier Magnitude 选 `SetByCaller` 并填入 Tag `Buff.Data.AttributeMod`，具体数值留空。  
> FA 节点（BFNode_ApplyEffect）的 `SetByCallerTag1 = Buff.Data.AttributeMod`，`Value1 = -300.0`。  
> 这样硬直强度可以在不修改 GE 资产的前提下，直接在 FA 节点上调整。

### 4-1 创建 GE：`GE_KnockbackStagger`

| 配置项 | 值 | 意义 |
|---|---|---|
| Duration Policy | **Has Duration**，Duration = `1.0` | 硬直效果持续 1 秒后自动解除 |
| Stacking Type | **Aggregate By Target**，Limit=1，Refresh | 命中刷新倒计时，不会叠层 |
| **Modifiers** → 添加一条 | Attribute = `MoveSpeed`，Op = **Additive**，Magnitude Type = **Set By Caller**，Tag = `Buff.Data.AttributeMod` | 移速减少量由 FA 运行时传入（填负值即为减速） |
| Asset Tags Component | Tag = `Buff.Effect.Attribute.MoveSpeed` | **行为层**：标识这个 GE 修改了移速 |

> **GE 侧只定义"我需要一个数值"（SetByCaller Tag），FA 侧负责"我传入多少"（-300）。**  
> 后续若想在二阶段把硬直从 -300 改为 -500，只需改 FA 节点的 Value，不需要改 GE 资产。

### 4-2 创建 GA：`GA_Knockback`（蓝图 Gameplay Ability）

路径：Content Browser 右键 → Gameplay → Gameplay Ability Blueprint

| 配置字段 | 值 | 意义 |
|---|---|---|
| Activation Owned Tags | `Buff.Status.Knockback` | **状态层**：角色 ASC 有此 Tag 时自动激活 |

**ActivateAbility 蓝图逻辑：**

```
ActivateAbility
  → 获取 Owner Actor（GetOwningActorFromActorInfo）
  → Cast to Character
  → 计算方向（攻击者 → 目标，或固定向外）
  → LaunchCharacter(方向 * 力度, true, true)
        力度建议：600～1200（可调）
  → Remove Gameplay Tag From Actor Owner
        Tag = Buff.Status.Knockback，Count = 1
  → End Ability
```

### 4-3 创建 FA：`FA_Rune_Knockback`

```
[OnDamageDealt]
    ↓
[Apply Gameplay Effect Class]          ← BFNode_ApplyEffect
    Effect             = GE_KnockbackStagger
    Level              = 1.0
    Target             = 上次伤害目标
    SetByCallerTag1    = Buff.Data.AttributeMod    ← 参数层：SetByCaller 键
    SetByCallerValue1  = -300.0                    ← 运行时传入：移速 -300
    ↓ Out
[Add Tag]                              ← BFNode_AddTag
    Tag    = Buff.Status.Knockback     ← 状态层：信号 Tag，触发 GA_Knockback
    Count  = 1
    Target = 上次伤害目标
```

> **节点顺序说明：**  
> 先施加 GE（移速减少 → 目标开始硬直），再写入信号 Tag（GA_Knockback 激活施加冲量）。  
> **Cleanup 行为：**  
> - `ApplyEffect` 节点 Cleanup：移除 GE_KnockbackStagger（硬直 GE 到期也会自动移除，Cleanup 只是双重保险）  
> - `AddTag` 节点 Cleanup：移除目标身上残留的 Buff.Status.Knockback，防止符文卸下后孤儿状态

### 4-4 创建 DA：`DA_Rune_Knockback`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1004` |
| RuneConfig.RuneEffectTag | `Buff.Effect.Attribute.MoveSpeed` | 
| Flow.FlowAsset | `FA_Rune_Knockback` |

> `RuneTag` 填 `Buff.Effect.Attribute.MoveSpeed` 是因为 GE_KnockbackStagger 用了此 Tag 作为 Asset Tag，保持一致。

### 4-5 授予敌人 GA（在敌人角色 BP）

```
BeginPlay → AbilitySystemComponent → GiveAbility(GA_Knockback, Level=1)
```

---

## 符文 5：流血（位移扣血）

**效果：** 命中敌人后，敌人进入流血状态。流血期间，移动速度越快扣血越多。  
**触发方式：** 命中触发（`Buff.Trigger.OnHit`）  
**扣血公式：** `每秒扣血 = 移动速度 / 200`

| 示例速度 | 每秒扣血 |
|---|---|
| 200（慢走） | 1 HP/s |
| 400（正常移动） | 2 HP/s |
| 600（冲刺） | 3 HP/s |

> **前提：** `GA_Bleed` 需要预先授予所有敌人角色。

### 5-1 创建 GE：`GE_Bleeding`

| 配置项 | 值 | 意义 |
|---|---|---|
| Duration Policy | **Infinite** | 持续直到被移除 |
| Stacking | Aggregate By Target，Limit=1，Refresh | 同目标只保留一个流血实例 |
| **Asset Tags Component** | `Buff.Effect.Bleed` | **行为层**：标识这是流血效果 GE |
| **Target Tags Component** | `Buff.Status.Bleeding` | **状态层**：GE 存在时目标 ASC 持有此状态 Tag；GE 移除后自动消失 |

> **Asset Tags 与 Target Tags 的区别：**  
> - **Asset Tags** = GE 自己的身份标签（供查询/移除用，不影响目标）  
> - **Target Tags** = GE 授予目标角色的状态标签（出现在目标 ASC 上，GE 失效后自动撤销）

### 5-2 创建 GA：`GA_Bleed`（蓝图 Gameplay Ability）

| 配置字段 | 值 |
|---|---|
| Activation Owned Tags | `Buff.Status.Bleeding` |

**ActivateAbility 蓝图逻辑：**

```
ActivateAbility
  → Loop（每 0.1 秒一次）：
      Wait Game Time (0.1s)
      → GetVelocity().Size()   → Speed
      → DamageAmount = Speed / 200.0 * 0.1
      → 如果 DamageAmount > 0.05：
          ApplyDamage(Owner, DamageAmount)
      → HasTag(Owner, Buff.Status.Bleeding)
          → True  → 继续 Loop
          → False → End Ability
```

### 5-3 创建 FA：`FA_Rune_Bleed`

```
[OnDamageDealt] ──→ [Apply Gameplay Effect Class]
                        Effect = GE_Bleeding
                        Level  = 1.0
                        Target = 上次伤害目标
```

> **生命周期：**  
> 命中 → GE_Bleeding 施加给目标 → Target Tags 授予 `Buff.Status.Bleeding` → GA_Bleed 激活  
> 符文离开激活区 → FA 停止 → ApplyEffect Cleanup 移除 GE_Bleeding → Tag 消失 → GA_Bleed 自动终止

### 5-4 创建 DA：`DA_Rune_Bleed`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1005` |
| RuneConfig.RuneEffectTag | `Buff.Effect.Bleed` |
| Flow.FlowAsset | `FA_Rune_Bleed` |

### 5-5 授予敌人 GA（在敌人角色 BP）

```
BeginPlay → AbilitySystemComponent → GiveAbility(GA_Bleed, Level=1)
```

---

## 符文 6：敌人掉血时额外扣血

**效果：** 每次对敌人造成伤害时，额外附带 1 点伤害。  
**触发方式：** 命中触发（`Buff.Trigger.OnHit`）  
**注意：** 需要确认 `BFNode_DoDamage` 是否会再次触发 `OnDamageDealt`（存在递归风险）。

### 6-1 创建 FA：`FA_Rune_ExtraDamage`

**基础版（先测试是否有递归问题）：**
```
[OnDamageDealt] ──→ [DoDamage]
                        Target = 上次伤害目标
                        Damage = 1.0
```

**安全版（带 `Buff.Status.ExtraDamageApplied` 守卫）：**
```
[OnDamageDealt] ──→ [Has Tag]
                        Tag    = Buff.Status.ExtraDamageApplied   ← 状态层：守卫 Tag
                        Target = Buff拥有者
                     ↓ False（没有守卫 Tag，才执行）
                    [Add Tag]
                        Tag    = Buff.Status.ExtraDamageApplied
                        Count  = 1
                        Target = Buff拥有者
                     ↓
                    [DoDamage]
                        Target = 上次伤害目标
                        Damage = 1.0
                     ↓
                    [Remove Tag]
                        Tag    = Buff.Status.ExtraDamageApplied
                        Count  = 1
                        Target = Buff拥有者
```

> **守卫逻辑：** 第一次触发时无守卫 Tag → 加 Tag → 执行额外伤害 → 若额外伤害再次触发 OnDamageDealt → 检测到 Tag 已有 → 直接跳过 → 额外伤害完成后移除 Tag。

### 6-2 创建 DA：`DA_Rune_ExtraDamage`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1006` |
| RuneConfig.RuneEffectTag | （留空，此符文无 GE 身份 Tag） |
| Flow.FlowAsset | `FA_Rune_ExtraDamage` |

---

## 资产目录建议

```
Content/Game/Runes/
├── AttackUp/
│   ├── DA_Rune_AttackUp
│   ├── FA_Rune_AttackUp
│   └── GE_Rune_AttackUp
├── HeatUp/
│   ├── DA_Rune_HeatUp
│   ├── FA_Rune_HeatUp
│   └── GE_Rune_HeatUp
├── SpeedStack/
│   ├── DA_Rune_SpeedStack
│   ├── FA_Rune_SpeedStack
│   └── GE_Rune_SpeedStack
├── Knockback/
│   ├── DA_Rune_Knockback
│   ├── FA_Rune_Knockback
│   ├── GE_KnockbackStagger
│   └── GA_Knockback
├── Bleed/
│   ├── DA_Rune_Bleed
│   ├── FA_Rune_Bleed
│   ├── GE_Bleeding
│   └── GA_Bleed
└── ExtraDamage/
    ├── DA_Rune_ExtraDamage
    └── FA_Rune_ExtraDamage
```

---

## 依赖总览

| 符文 | DA | FA | GE | GA | 需预授予敌人 |
|---|---|---|---|---|---|
| 攻击提升 | ✓ | ✓ | ✓ | — | — |
| 热度提升 | ✓ | ✓ | ✓ | — | — |
| 移速堆叠 | ✓ | ✓ | ✓ | — | — |
| 击退 | ✓ | ✓ | ✓（SetByCaller） | ✓ | ✓ 所有敌人 |
| 流血 | ✓ | ✓ | ✓ | ✓ | ✓ 所有敌人 |
| 额外扣血 | ✓ | ✓ | — | — | — |

---

## Tag 汇总（需添加到 Buff_Tag_Spec.md 的新增项）

> 制作前请确认这些 Tag 已添加到项目中（Project Settings → Gameplay Tags）

| Tag | 层 | 状态 | 填写位置 |
|---|---|---|---|
| `Buff.Effect.Attribute.Attack` | 行为层 | ☆ 新增 | GE Asset Tags + DA.RuneConfig.RuneEffectTag |
| `Buff.Effect.Attribute.Heat` | 行为层 | ☆ 新增 | GE Asset Tags + DA.RuneConfig.RuneEffectTag |
| `Buff.Effect.Attribute.MoveSpeed` | 行为层 | ★ 已有 | GE Asset Tags + DA.RuneConfig.RuneEffectTag |
| `Buff.Effect.Bleed` | 行为层 | ☆ 新增 | GE Asset Tags + DA.RuneConfig.RuneEffectTag |
| `Buff.Status.Knockback` | 状态层 | ☆ 新增 | AddTag 目标 → GA Activation Owned Tags |
| `Buff.Status.Bleeding` | 状态层 | ☆ 新增 | GE Target Tags → GA Activation Owned Tags |
| `Buff.Status.ExtraDamageApplied` | 状态层 | ☆ 新增 | FA 手动 Add/Remove Tag（守卫用） |
| `Buff.Data.AttributeMod` | 参数层 | ★ 已有 | GE Modifier SetByCaller Tag + FA BFNode_ApplyEffect SetByCallerTag 槽 |

---

## 制作反馈区

> 制作过程中遇到的问题、调整、或需要修改的步骤，请在下方记录：

```
[反馈] 符文1 - 攻击属性名不是 Attack，实际是 ...
[反馈] 符文3 - GE 堆叠配置中 ...
[反馈] 符文4 - GA_Knockback 激活方式改为 ...
[反馈] 符文4 - SetByCaller 数值 -300 效果偏弱/强，调整为 ...
[反馈] Tag   - Buff.Effect.Attribute.Attack 与现有某 Tag 重复，改为 ...
...
```
