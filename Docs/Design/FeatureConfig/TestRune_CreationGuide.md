# 测试符文制作指南 v4

> 版本：v4（2026-04-12）更新重点：新增符文 1008 刀光波（远程投射物，每 3 次命中触发，穿透多敌，C++ 投射物 + C++ 被动 GA）。  
> v3（2026-04-09）更新重点：击退拆分为两个独立符文（1004 击退 + 1007 击退减速），引入符文联动模式（GameplayEvent 跨符文通信）；GA_Knockback 改为 C++ 实现，使用 RootMotionMoveToForce 精确控距。
> Tag 规范：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · 系统指南：[BuffFlow_SystemGuide.md](BuffFlow_SystemGuide.md)

---

## 资产策略总览

| 符文 | Blueprint GE | C++/Blueprint GA | 说明 |
|------|-------------|-----------------|------|
| 1001 攻击强化 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier 直接处理 |
| 1002 热度提升 | ⚠️ 1个（HeatTick，需 Inhibit 功能） | ❌ 不需要 | Period GE + GrantTag |
| 1003 速度叠加 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier Stackable |
| 1004 击退 | ❌ 不需要 | ✅ 1个（C++ GA_Knockback，精确控距） | 纯位移，无减速；可单独使用 |
| 1007 击退减速 | ❌ 不需要 | ❌ 不需要 | 监听 1004 的击退事件，联动施加减速 |
| 1005 流血 | ❌ 不需要 | ✅ 1个（速度扣血逻辑） | GrantTag 替代 GE_Bleeding |
| 1006 额外伤害 | ❌ 不需要 | ❌ 不需要 | AddTag 守卫 + DoDamage |
| 1008 刀光波 | ✅ 1个（GE_SlashWaveDamage，SetByCaller 扣血） | ✅ 2个（C++ GA_SlashWaveCounter + C++ ASlashWaveProjectile 的 BP 子类） | 命中 3 次发射穿透投射物 |

**v4 变化：** 新增 1008 刀光波，引入 C++ 投射物模式（ASlashWaveProjectile）和持久被动 GA 计数器模式（GA_SlashWaveCounter）。  
**v3 变化：** Blueprint GE 从 2 个减少到 **1 个**（仅 HeatTick）；击退改用 C++ GA + RootMotion，减速改为零资产联动符文。

---

## 前置：GameplayTag 创建

**路径：** Edit → Project Settings → Gameplay Tags

> ★ = Buff_Tag_Spec.md 中已有；☆ = 本次新增

| Tag | 层 | 状态 | 用途 |
|---|---|---|---|
| `Buff.Effect.Attribute.Attack` | 行为层 | ☆ 新增 | GAS Debugger 查询用，贴在 DA.RuneEffectTag |
| `Buff.Effect.Attribute.Heat` | 行为层 | ☆ 新增 | 同上 |
| `Buff.Effect.Attribute.MoveSpeed` | 行为层 | ★ 已有 | 同上 |
| `Buff.Effect.Bleed` | 行为层 | ☆ 新增 | GE_HeatTick Asset Tag |
| `Buff.Status.HeatInhibit` | 状态层 | ☆ 新增 | 受伤后暂停热度的 Inhibit 守卫 Tag |
| `Buff.Status.Bleeding` | 状态层 | ☆ 新增 | 流血状态，GA_Bleed 以此为激活条件 |
| `Buff.Status.ExtraDamageApplied` | 状态层 | ☆ 新增 | 递归守卫 Tag（符文6） |
| `Buff.Status.Knockback` | 状态层 | ☆ 新增 | GA_Knockback 激活期间自动授予，结束时移除 |
| `Ability.Knockback` | 触发层 | ☆ 新增 | FA 向目标发送此 Event，激活 GA_Knockback |
| `Event.Rune.KnockbackApplied` | 触发层 | ☆ 新增 | GA_Knockback 完成后向玩家 ASC 广播，供联动符文监听 |

---

## 符文 1：攻击强化（1001）⚡ 零资产

**设计：** 被动。激活期间永久 +10 攻击力。

**需要创建：** FA + DA（无需 GE）

### 1-1 FA：`FA_Rune_AttackUp`

```
[Start] ──→ [Apply Gameplay Effect]           ← BFNode_ApplyAttributeModifier
                Attribute    = AttackDamage（或项目实际属性名）
                ModOp        = Additive
                Value        = 10.0
                DurationType = Infinite
                Period       = 0（不启用）
                Target       = Buff拥有者
```

**Cleanup 行为：** FA 停止时节点自动移除 GE，攻击力恢复。

### 1-2 DA：`DA_Rune_AttackUp`

| 字段 | 值 |
|---|---|
| RuneName | `AttackUp` |
| Shape.Cells | `(0,0)` |
| RuneConfig.RuneID | `1001` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_AttackUp` |

### 1-3 测试要点

- GAS Debugger → 属性面板：激活后 AttackDamage 增加 10
- 拖出激活区：AttackDamage 恢复原值
- 多次装卸：数值不累积

---

## 符文 2：热度提升（1002）⚠️ 需要 1 个 Blueprint GE

**设计：** 被动。每秒 +1 热度。受到伤害后暂停 5 秒，伤害结束后恢复。

**需要创建：** GE_HeatTick（Blueprint，需 OngoingTagRequirements）+ FA + DA

> **为什么这个符文需要 Blueprint GE？**
> `BFNode_ApplyAttributeModifier` 的 Period 字段只设置 GE 的 Period，但无法配置 `OngoingTagRequirements.IgnoreTags`。
> 受伤暂停功能依赖 GAS 的 Inhibit 机制（自动暂停/恢复），必须用 Blueprint GE 配置这个字段。

### 2-1 GE：`GE_HeatTick`

| 配置项 | 值 | 说明 |
|---|---|---|
| Duration Policy | **Infinite** | 永久运行 |
| Period | `1.0` | 每秒触发一次 Modifier |
| Execute Period Effect On Application | `false` | 施加时不立即触发，等第一个 Period |
| **Modifiers** | Attribute=Heat, Op=Additive, Magnitude=1 | 每次 Period 触发时 +1 热度 |
| Stacking | AggregateByTarget, Limit=1 | 同目标只保留一个实例 |
| **Ongoing Tag Requirements** → **Ignore Tags** | `Buff.Status.HeatInhibit` | ⭐ 目标有此 Tag 时自动暂停 Period 执行 |
| Asset Tags | `Buff.Effect.Attribute.Heat` | 行为层标识 |

> `OngoingTagRequirements.IgnoreTags`：GE 在目标身上激活期间，如果目标有 HeatInhibit Tag，GAS 自动暂停 Period 执行（GE 不移除，只是"睡眠"）。Tag 消失后自动恢复。

### 2-2 FA：`FA_Rune_HeatUp`

```
[Start] ──→ [Apply Gameplay Effect Class]     ← BFNode_ApplyEffect
                Effect = GE_HeatTick
                Level  = 1.0
                Target = Buff拥有者
                ↓ Out（保持活跃，Cleanup 时移除 GE）

[OnDamageReceived] ──→ [Grant Tag (Timed)]    ← BFNode_GrantTag
                            Tag      = Buff.Status.HeatInhibit
                            Duration = 5.0
                            Target   = Buff拥有者
                            ↓ Out → （继续监听下次受伤）
                            ↓ Expired → （5秒到期，热度自动恢复）
```

**生命周期：**
- 受伤 → GrantTag 授予 HeatInhibit 5s → GAS 检测到 IgnoreTags 命中 → GE_HeatTick Period 暂停
- 5s 后 GrantTag 到期自动移除 HeatInhibit → GAS 恢复 GE_HeatTick Period
- 再次受伤 → 重置 5s 计时（GrantTag 的 In 引脚再触发会刷新计时器）
- FA 停止 → BFNode_ApplyEffect Cleanup 移除 GE_HeatTick；BFNode_GrantTag Cleanup 移除 HeatInhibit

### 2-3 DA：`DA_Rune_HeatUp`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1002` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_HeatUp` |

### 2-4 测试要点

- 激活后每秒 Heat +1（GAS Debugger 观察 Heat 属性）
- 受伤后 5 秒内 Heat 停止增长
- 5 秒后 Heat 恢复每秒增长
- 连续受伤时计时器刷新（再次受伤重置 5 秒）
- 拖出激活区：GE 移除，Tag 清理

---

## 符文 3：速度叠加（1003）⚡ 零资产

**设计：** 命中敌人后自身获得移速 +30，持续 3 秒，最多叠 5 层（+150）。每次命中刷新计时，逐层衰减。

**需要创建：** FA + DA（无需 GE）

### 3-1 FA：`FA_Rune_SpeedStack`

```
[Start] ──→ [OnDamageDealt] ──→ [Apply Gameplay Effect]  ← BFNode_ApplyAttributeModifier
                                     Attribute    = MoveSpeed（或项目实际属性名）
                                     ModOp        = Additive
                                     Value        = 30.0
                                     DurationType = HasDuration
                                     Duration     = 3.0
                                     Period       = 0（不启用）
                                     Target       = Buff拥有者
                                     StackMode    = Stackable
                                     StackLimit   = 5
                                     Duration Refresh = Refresh On Successful Application
                                     Expiration Policy = Remove Single Stack And Refresh Duration
```

**堆叠行为说明：**
- `Stackable + Limit=5`：最多叠 5 层，每层 +30 MoveSpeed
- `Refresh On Successful Application`：每次命中刷新所有层的计时
- `Remove Single Stack And Refresh Duration`：到期时只减一层，剩余层重置计时（逐层衰减）

**Cleanup 行为：** FA 停止时自动移除整个 Stackable GE（所有层同时移除）。

### 3-2 DA：`DA_Rune_SpeedStack`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1003` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_SpeedStack` |

### 3-3 测试要点

- 命中后 MoveSpeed +30
- 连续命中：最多 +150（5层），计时刷新
- 停止攻击：3秒后开始逐层掉落（每层 3 秒）
- 拖出激活区：全部层立即移除

---

## 符文 4：击退（1004）✅ 需要 C++ GA

**设计：** 命中敌人后，将其沿击退方向位移固定距离（默认 500cm）。  
**纯位移效果，不含减速。** 若需要击退后减速，搭配符文 1007（击退减速）使用。

**需要创建：** GA_Knockback（C++ 已实现）的 Blueprint 子类 + FA + DA

---

### 符文使用说明

> **独立使用：** 仅装备 1004，命中敌人后触发纯位移击退。  
> **联动使用：** 同时装备 1004 + 1007，击退完成后自动触发减速效果。  
> 联动不需要任何额外配置——只要两个符文都放在背包激活区，1007 会自动监听 1004 发出的事件。

**工作流程（联动时）：**
```
FA_Rune_Knockback（1004）
  OnDamageDealt → 向目标发送 GameplayEvent(Ability.Knockback)
                        ↓
              GA_Knockback（C++）在目标上激活
                        ↓
              执行 RootMotionMoveToForce（精确位移 KnockbackDistance cm）
                        ↓
              向玩家 ASC 广播 Event.Rune.KnockbackApplied（携带目标引用）
                        ↓
FA_Rune_KnockbackStagger（1007）监听到事件 → 对目标施加移速 -300，持续 1 秒
```

---

### 4-1 GA：`GA_Knockback`（C++ 类，已实现）

**C++ 源码：** `Source/DevKit/Public/AbilitySystem/Abilities/GA_Knockback.h`

**需要在编辑器创建 Blueprint 子类** 用于配置数值：

> Content Browser → 右键 → Blueprint Class → 父类选 `GA_Knockback`  
> 命名为 `BGA_Knockback`（B 前缀表示 Blueprint）

| 配置字段 | 值 | 说明 |
|---|---|---|
| Ability Tags | （不设置） | 击退可作用于玩家/敌人，无明确命名空间归属；无代码通过身份 Tag 查询此 GA |
| Activation Owned Tags | `Buff.Status.Knockback` | 击退期间自动挂到目标 ASC，结束自动移除 |
| **KnockbackDistance** | `500.0` | 击退距离（cm），直接填单位，策划可调 |
| **KnockbackDuration** | `0.3` | 位移持续时间（秒），越短击退越"爆" |
| bZeroVelocityOnFinish | `true` | 击退结束时清零速度，防止敌人继续滑行 |

**授予方式：** 在敌人角色 BP 的 BeginPlay → `GiveAbility(BGA_Knockback)`

**触发方式：** FA 通过 `BFNode_SendGameplayEvent` 发送 `Ability.Knockback` 事件至目标，  
GA 的 `AbilityTriggers` 配置监听此 Event 自动激活（无需手动调用）。

> **GA 内部流程（C++ 已实现，无需修改）：**
> 1. 从 `TriggerEventData.Instigator` 获取攻击者位置，计算击退方向
> 2. 执行 `AbilityTask_ApplyRootMotionMoveToForce`，精确移动至目标点
> 3. 位移完成后，向攻击者 ASC 广播 `Event.Rune.KnockbackApplied`（携带被击退目标引用）
> 4. 结束 GA，自动移除 `Buff.Status.Knockback`

**GA_Knockback AbilityTriggers 配置（编辑器里填）：**

| 字段 | 值 |
|---|---|
| Trigger Tag | `Ability.Knockback` |
| Trigger Source | `GameplayEvent` |

---

### 4-2 FA：`FA_Rune_Knockback`

```
[Start] ──→ [OnDamageDealt]
                ↓
            [BFNode_SendGameplayEvent]
                EventTag   = Ability.Knockback      ← 触发目标上的 GA_Knockback
                Target     = 上次伤害目标（被击退对象）
                Instigator = 玩家 Actor             ← GA 用此计算方向
                ↓ Out
            （回到 OnDamageDealt 继续监听）
```

**Cleanup：** SendGameplayEvent 无持久资源，FA 停止时无需额外清理。

---

### 4-3 DA：`DA_Rune_Knockback`

| 字段 | 值 |
|---|---|
| RuneName | `Knockback` |
| RuneConfig.RuneID | `1004` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_Knockback` |

---

### 4-4 测试要点

- 命中后敌人被位移约 500cm（5 米）
- 击退期间 GAS Debugger：目标 ASC 上有 `Buff.Status.Knockback` Tag
- 击退结束后 Tag 自动消失
- 调整 `BGA_Knockback` 的 `KnockbackDistance` 测试不同距离
- Output Log 中 `Event.Rune.KnockbackApplied` 被发送（可加临时 log 验证）

---

## 符文 7：击退减速（1007）⚡ 零资产 · 依赖符文 1004

**设计：** 监听击退事件，每次击退完成后对被击退目标施加移速 -300，持续 1 秒。  
**必须与符文 1004 同时装备**，单独装备无效果。

**需要创建：** FA + DA（零资产，BFNode_ApplyAttributeModifier 直接处理）

> **这是符文联动模式的标准范例。**  
> 联动机制：符文 A 执行效果 → 广播 GameplayEvent → 符文 B 的 FA 监听事件 → 执行附加效果。  
> 两个符文完全解耦，击退符文不知道减速符文存在，减速符文只负责监听信号。

---

### 7-1 FA：`FA_Rune_KnockbackStagger`

```
[Start]
  ↓
[BFNode_WaitGameplayEvent]                      ← 持续监听，不阻塞
    EventTag = Event.Rune.KnockbackApplied      ← 击退完成时由 GA_Knockback 广播
    ↓ Out（收到事件时触发）
[BFNode_ApplyAttributeModifier]
    Target       = EventData.Target             ← 事件携带的被击退目标
    Attribute    = MoveSpeed
    ModOp        = Additive
    Value        = -300.0                       ← 移速 -300（填负值）
    DurationType = HasDuration
    Duration     = 1.0                          ← 1 秒后自动解除
    ↓ Out
（回到 WaitGameplayEvent，等待下次击退）
```

**EventData.Target 说明：**  
GA_Knockback 完成时广播的 `FGameplayEventData.Target` 即被击退的敌人 Actor，  
BFNode_WaitGameplayEvent 收到后可直接作为 ApplyAttributeModifier 的 Target 引脚。

**Cleanup：** ApplyAttributeModifier 自动移除其创建的 GE；WaitGameplayEvent 停止监听。

---

### 7-2 DA：`DA_Rune_KnockbackStagger`

| 字段 | 值 |
|---|---|
| RuneName | `KnockbackStagger` |
| RuneConfig.RuneID | `1007` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_KnockbackStagger` |

---

### 7-3 测试要点

**单独装备 1007（无 1004）：**
- 无任何效果（正确，无事件源）

**同时装备 1004 + 1007：**
- 命中后敌人先被击退位移
- 击退完成瞬间敌人移速降低 300，持续 1 秒
- GAS Debugger：目标 MoveSpeed 减少了 300
- 1 秒后移速恢复正常
- 连续命中：每次击退都触发一次减速（计时刷新）

---

## 符文 5：流血（1005）⚡ 无需 Blueprint GE

**设计：** 命中敌人施加流血状态（10 秒），流血期间移动速度越快扣血越多（每秒扣血 = 速度/200）。

**需要创建：** GA_Bleed（Blueprint）+ FA + DA（GE_Bleeding 由 GrantTag 替代）

> **v2 变化：** 旧版需要 Blueprint GE_Bleeding 通过 Target Tags 授予 `Buff.Status.Bleeding`。新版改用 `BFNode_GrantTag` 直接授予 Tag，省去一个 GE 资产。

### 5-1 GA：`GA_Bleed`

**授予方式：** 在敌人角色 BP 的 BeginPlay → GiveAbility(GA_Bleed)

| 配置字段 | 值 |
|---|---|
| Activation Owned Tags | `Buff.Status.Bleeding` |

**ActivateAbility 蓝图逻辑：**
```
ActivateAbility
  → Loop:
      WaitGameplayTag: Remove → Buff.Status.Bleeding（检测 Tag 消失时退出）
      WaitDelay: 0.2s
      → GetOwnerVelocity().Size() → Speed
      → DamagePerTick = Speed / 200.0 × 0.2   （每 0.2s 的伤害份额）
      → if DamagePerTick > 0.01:
            ApplyDamage(OwnerActor, DamagePerTick)
  ← Loop 结束（Tag 消失触发）
  → EndAbility
```

> 每 0.2 秒检测一次，等价于 "每秒 Speed/200 点伤害"，帧率无关。

### 5-2 FA：`FA_Rune_Bleed`

```
[Start] ──→ [OnDamageDealt] ──→ [Grant Tag (Timed)]  ← BFNode_GrantTag
                                     Tag      = Buff.Status.Bleeding
                                     Duration = 10.0
                                     Target   = 上次伤害目标
                                     ↓ Out → （继续监听下次命中，刷新流血计时）
                                     ↓ Expired → （10秒到期，Tag 自动移除，GA_Bleed 退出）
```

**生命周期：**
- 命中 → GrantTag 授予 `Buff.Status.Bleeding` 10s → GA_Bleed 因 ActivationOwnedTags 激活
- 再次命中 → GrantTag 的 `In` 再触发 → 计时器刷新为 10 秒（重置倒计时）
- 10s 未命中 → Tag 到期自动移除 → GA_Bleed 检测到 Tag 消失 → 自动退出
- FA 停止 → GrantTag Cleanup 立即移除 Tag → GA_Bleed 退出

### 5-3 DA：`DA_Rune_Bleed`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1005` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_Bleed` |

### 5-4 测试要点

- 命中后 GAS Debugger：目标 ASC 出现 `Buff.Status.Bleeding` Tag
- 移动中目标持续掉血（移速越高掉血越快）
- 静止目标：不扣血（速度为 0）
- 10 秒未命中：Tag 消失，GA_Bleed 停止
- 连续命中：计时器刷新，流血延续

---

## 符文 6：额外伤害（1006）⚡ 零资产

**设计：** 每次造成伤害时，额外附带 1 点伤害（防止递归触发自身）。

**需要创建：** FA + DA（无需 GE/GA）

### 6-1 FA：`FA_Rune_ExtraDamage`

**基础版（先验证是否有递归问题）：**
```
[Start] → [OnDamageDealt] → [DoDamage]
                                Target = 上次伤害目标
                                Damage = 1.0
```

**安全版（带递归守卫，推荐）：**
```
[Start] → [OnDamageDealt]
               ↓
           [Has Tag]                        ← BFNode_HasTag
               Tag    = Buff.Status.ExtraDamageApplied
               Target = Buff拥有者
               ↓ False（无守卫 Tag，正常执行）
           [Add Tag]                        ← BFNode_AddTag
               Tag    = Buff.Status.ExtraDamageApplied
               Count  = 1
               Target = Buff拥有者
               ↓
           [DoDamage]                       ← BFNode_DoDamage
               Target = 上次伤害目标
               Damage = 1.0
               ↓
           [Remove Tag]                     ← BFNode_RemoveTag
               Tag    = Buff.Status.ExtraDamageApplied
               Target = Buff拥有者
           （True 分支 → 直接跳过，无操作）
```

**守卫逻辑：**
1. 第一次触发：无守卫 Tag → 加 Tag → 执行额外伤害
2. 额外伤害再次触发 OnDamageDealt → 检测到守卫 Tag 已存在 → 走 True 分支跳过
3. 额外伤害完成 → 移除守卫 Tag → 恢复正常检测

### 6-2 DA：`DA_Rune_ExtraDamage`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1006` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_ExtraDamage` |

### 6-3 测试要点

- 命中后伤害数字额外多 1（需游戏有伤害数字显示）
- GAS Debugger 无异常（无无限循环）
- 建议先用基础版，如发现递归改用安全版

---

---

## 符文 8：刀光波（1008）✅ 需要 1 个 Blueprint GE + 2 个 Blueprint GA（C++ 子类）

**设计：** 命中敌人后累计次数，每命中 3 次向前方发射一道水平刀光波，穿透多个敌人。  
刀光独立伤害，与攻击伤害叠加。不依赖热度，纯符文被动效果。

**两个模式（同一套 C++ 代码，Blueprint 配置切换）：**

| 模式 | bHitRequired | 触发条件 | 适合 |
|------|-------------|---------|------|
| 命中模式 | `true`（默认） | 攻击命中敌人后计数 | 精准、不浪费层数 |
| 挥刀模式 | `false` | 每次挥刀帧计数（无论是否命中） | 高频触发、需配置守卫 Tag |

> 本指南先制作命中模式（1008），挥刀模式配置差异见 [8-5 挥刀模式附加配置](#85-挥刀模式附加配置)。

**需要创建：**
- `GE_SlashWaveDamage`（Blueprint GE，一次性扣血）
- `BP_SlashWaveProjectile`（ASlashWaveProjectile 的 Blueprint 子类，做表现层）
- `BGA_SlashWaveCounter`（GA_SlashWaveCounter 的 Blueprint 子类，配置数值）
- `FA_Rune_SlashWave`（命中模式 FlowAsset）
- `DA_Rune_SlashWave`（符文 DataAsset）

---

### 8-0 前置：GameplayTag 创建

**路径：** Edit → Project Settings → Gameplay Tags

| Tag | 用途 |
|---|---|
| `Action.Rune.SlashWaveHit` | 命中模式：FA 命中敌人后发送到玩家 ASC，GA 计数用 |
| `Action.Attack.Swing` | 挥刀模式：AN_MeleeDamage 每次攻击帧自动发送（C++ 已添加） |
| `Buff.Status.SlashWaveSwingActive`| 挥刀模式守卫 Tag（挥刀模式 FA 激活时授予，符文移除时自动清理） |

---

### 8-1 GE：`GE_SlashWaveDamage`

在内容浏览器创建 Blueprint → GameplayEffect，命名 `GE_SlashWaveDamage`。

| 配置项 | 值 | 说明 |
|---|---|---|
| Duration Policy | **Instant** | 瞬间扣血 |
| **Modifiers** | Attribute = 目标 HP 属性, Op = Additive | 填项目实际 HP 属性 |
| Modifier Magnitude | **SetByCaller** | 通过代码传入伤害量 |
| SetByCaller Tag | `Attribute.ActDamage` | 与 C++ 代码中的 SetByCaller Tag 一致 |

> `Attribute.ActDamage` 与现有伤害管线标签一致，无需新增 Tag。

---

### 8-2 GA：`BGA_SlashWaveCounter`（GA_SlashWaveCounter C++ 子类）

**创建 Blueprint 子类：**

> Content Browser → 右键 → Blueprint Class → 父类搜索 `GA_SlashWaveCounter`  
> 命名为 `BGA_SlashWaveCounter`

**配置字段（Class Defaults）：**

| 配置字段 | 值 | 说明 |
|---|---|---|
| **HitsRequired** | `3` | 每 3 次命中触发一次刀光 |
| **bHitRequired** | `true` | 命中模式（监听 Action.Rune.SlashWaveHit） |
| SwingModeGateTag | （留空） | 命中模式不需要守卫 Tag |
| **ProjectileClass** | `BP_SlashWaveProjectile` | 见下方 8-3 |
| **SlashDamage** | `30.0` | 刀光伤害量 |
| **SlashDamageEffect** | `GE_SlashWaveDamage` | 见 8-1 |
| **SpawnOffset** | `80.0` | 从角色中心向前 80cm 生成 |

**授予方式：**

> 玩家角色 BP → BeginPlay → `GiveAbility(BGA_SlashWaveCounter)`

GA 被授予时自动激活（持久监听），无需手动触发。

---

### 8-3 Projectile：`BP_SlashWaveProjectile`（ASlashWaveProjectile C++ 子类）

> Content Browser → 右键 → Blueprint Class → 父类搜索 `SlashWaveProjectile`  
> 命名为 `BP_SlashWaveProjectile`

**Class Defaults 可调整：**

| 字段 | 默认值 | 说明 |
|---|---|---|
| Lifetime | `1.2` | 生存时间（秒） |
| Speed | `1400` | 飞行速度（cm/s） |

**表现层（在 BP 事件图中实现）：**

| 事件 | 推荐实现 |
|---|---|
| `BP_OnHitEnemy(HitActor, HitLocation)` | 在 HitLocation 生成命中粒子、播放音效 |
| `BP_OnExpired()` | 播放消散特效（如光芒消失） |

> C++ 层控制所有碰撞/伤害/穿透逻辑，Blueprint 只做视觉表现。

---

### 8-4 FA：`FA_Rune_SlashWave`（命中模式）

```
[Start]
  ↓
[BFNode_OnDamageDealt]                      ← 等待玩家命中敌人
  ↓ Out（命中时触发）
[BFNode_SendGameplayEvent]
    EventTag   = Action.Rune.SlashWaveHit   ← 通知 BGA_SlashWaveCounter 计数
    Target     = Buff拥有者（玩家角色）      ← 事件发送到自身 ASC
    Instigator = Buff拥有者
  ↓ Out
（回到 OnDamageDealt，继续监听下次命中）
```

**生命周期：**
- 符文装入激活区 → FA 启动 → OnDamageDealt 开始监听
- 每次命中 → SendGameplayEvent → BGA_SlashWaveCounter 计数 +1
- 累计 3 次 → BGA_SlashWaveCounter 自动生成 BP_SlashWaveProjectile
- 符文移出激活区 → FA 停止 → 监听器自动清理；计数器在 GA 下次激活前重置

**Cleanup：** SendGameplayEvent 无持久资源，FA 停止时无需额外清理。

---

### 8-5 DA：`DA_Rune_SlashWave`

| 字段 | 值 |
|---|---|
| RuneName | `SlashWave` |
| RuneConfig.RuneID | `1008` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_SlashWave` |

---

### 8-6 测试要点（命中模式）

1. **装备前验证：** 不装备符文时，攻击敌人无刀光，Output Log 无 SlashWave 相关输出
2. **装备后计数：** 装备 DA_Rune_SlashWave，攻击敌人 1~2 次无刀光，第 3 次命中后生成投射物
3. **穿透验证：** 刀光飞过 2 个并排敌人，两个都受到伤害（Output Log 两条 ApplyGE 记录）
4. **伤害叠加：** 攻击命中 + 刀光命中同一敌人时，敌人血量减少两次（攻击 + 刀光各自独立）
5. **计数重置：** 第 3 次触发后计数归零，再次命中 3 次才触发第二波刀光
6. **移出检测：** 将符文移出激活区，后续攻击不再触发刀光（FA 已停止，不再发送事件）
7. **GAS Debugger：** 命中时可见 `Action.Rune.SlashWaveHit` 事件发送到玩家 ASC

---

### 8-7 挥刀模式附加配置

> 在完成命中模式基础测试后，按需创建挥刀模式版本。

**额外需要创建：**
- `BGA_SlashWaveCounter_Swing`（BGA_SlashWaveCounter 的副本，或新 Blueprint 子类）
- `FA_Rune_SlashWave_Swing`（挥刀模式 FA）

**BGA_SlashWaveCounter_Swing 差异配置：**

| 字段 | 值（与命中模式不同） |
|---|---|
| bHitRequired | `false`（监听 Action.Attack.Swing） |
| SwingModeGateTag | `Buff.Status.SlashWaveSwingActive` |

**FA_Rune_SlashWave_Swing 节点图：**

```
[Start]
  ↓
[BFNode_GrantTag]                                    ← 授予守卫 Tag，告知 GA 符文已激活
    Tag      = Buff.Status.SlashWaveSwingActive
    Duration = Infinite（FA 停止时 Cleanup 自动移除）
    Target   = Buff拥有者
  ↓ Out（Tag 持续存在，FA 停止时自动清理）
（FA 一直保持激活，守卫 Tag 存在 = 符文激活中）
```

**挥刀模式工作流：**
1. 符文激活 → FA 启动 → GrantTag 授予 `Buff.Status.SlashWaveSwingActive`
2. 每次挥刀（AN_MeleeDamage::Notify）→ 发送 `Action.Attack.Swing` 到玩家 ASC
3. GA 收到事件 → 检查 `Buff.Status.SlashWaveSwingActive` 存在 → 计数 +1
4. 累计 HitsRequired 次 → 生成投射物
5. 符文移出 → FA 停止 → GrantTag Cleanup 移除守卫 Tag → GA 忽略后续挥刀事件

---

## 资产目录结构

```
Content/Game/Runes/
├── AttackUp/
│   ├── DA_Rune_AttackUp
│   └── FA_Rune_AttackUp
├── HeatUp/
│   ├── DA_Rune_HeatUp
│   ├── FA_Rune_HeatUp
│   └── GE_HeatTick                     ← 唯一需要的 Blueprint GE
├── SpeedStack/
│   ├── DA_Rune_SpeedStack
│   └── FA_Rune_SpeedStack
├── Knockback/
│   ├── DA_Rune_Knockback               ← 符文 1004
│   ├── FA_Rune_Knockback
│   └── BGA_Knockback                   ← GA_Knockback(C++) 的 Blueprint 子类，配置数值用
├── KnockbackStagger/
│   ├── DA_Rune_KnockbackStagger        ← 符文 1007，联动符文
│   └── FA_Rune_KnockbackStagger
├── Bleed/
│   ├── DA_Rune_Bleed
│   ├── FA_Rune_Bleed
│   └── GA_Bleed                        ← 速度扣血逻辑，必须用 Blueprint GA
├── ExtraDamage/
│   ├── DA_Rune_ExtraDamage
│   └── FA_Rune_ExtraDamage
└── SlashWave/
    ├── DA_Rune_SlashWave
    ├── FA_Rune_SlashWave                   ← 命中模式 FA
    ├── FA_Rune_SlashWave_Swing             ← 挥刀模式 FA（可选）
    ├── BGA_SlashWaveCounter                ← GA_SlashWaveCounter(C++) 的 Blueprint 子类
    ├── BP_SlashWaveProjectile              ← ASlashWaveProjectile(C++) 的 Blueprint 子类
    └── GE_SlashWaveDamage                  ← 刀光伤害 GE（SetByCaller）
```

---

## 依赖总览

| 符文 | DA | FA | Blueprint GE | C++/BP GA | 依赖 | 备注 |
|------|----|----|-------------|-----------|------|------|
| 1001 攻击强化 | ✓ | ✓ | — | — | — | 全零资产 |
| 1002 热度提升 | ✓ | ✓ | ✓ GE_HeatTick | — | — | 仅因需要 Inhibit |
| 1003 速度叠加 | ✓ | ✓ | — | — | — | 全零资产 |
| 1004 击退 | ✓ | ✓ | — | ✓ BGA_Knockback（C++子类） | — | 可独立使用 |
| 1007 击退减速 | ✓ | ✓ | — | — | **必须装备 1004** | 联动模式范例 |
| 1005 流血 | ✓ | ✓ | — | ✓ GA_Bleed | — | GE 由 GrantTag 替代 |
| 1006 额外伤害 | ✓ | ✓ | — | — | — | 全零资产 |
| 1008 刀光波 | ✓ | ✓ | ✓ GE_SlashWaveDamage | ✓ BGA_SlashWaveCounter + BP_SlashWaveProjectile | 玩家 BP 需 GiveAbility | 命中计数器 + 穿透投射物 |

---

## 制作顺序建议

1. **符文 1001（攻击强化）** → 最简单，验证 ApplyAttributeModifier 基础路径
2. **符文 1003（速度叠加）** → 验证 Stackable + Duration 模式
3. **符文 1006（额外伤害）** → 验证递归守卫模式
4. **符文 1005（流血）** → 验证 GrantTag + GA 激活链
5. **符文 1002（热度提升）** → 验证 Period + OngoingTagRequirements Inhibit
6. **符文 1004（击退）** → 验证 GameplayEvent 触发 GA + RootMotion 精确控距
7. **符文 1007（击退减速）** → 验证符文联动模式（WaitGameplayEvent 跨符文通信）
8. **符文 1008（刀光波）** → 验证 C++ 投射物 + 持久被动 GA + OnDamageDealt 计数链路

---

## 制作反馈区

> 遇到的问题、调整、实际属性名等，在下方记录：

```
[反馈] 符文1 - 实际属性名是 ___ 而不是 AttackDamage
[反馈] 符文2 - OngoingTagRequirements 配置位置在 GE Details 的 ___
[反馈] 符文3 - StackMode 在节点的哪个分类下 ___
[反馈] 符文4 - GA_Knockback 激活方式改为 ___
[反馈] 符文5 - GA_Bleed 退出逻辑改为 ___
...
```
