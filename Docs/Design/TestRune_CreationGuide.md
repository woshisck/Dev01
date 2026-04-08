# 测试符文制作指南 v2

> 版本：v2（2026-04-08）更新重点：全面采用 `BFNode_ApplyAttributeModifier` 零资产方案，大幅减少 Blueprint GE 资产数量。
> Tag 规范：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · 系统指南：[BuffFlow_SystemGuide.md](BuffFlow_SystemGuide.md)

---

## 资产策略总览

| 符文 | Blueprint GE | Blueprint GA | 说明 |
|------|-------------|-------------|------|
| 1001 攻击强化 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier 直接处理 |
| 1002 热度提升 | ⚠️ 1个（HeatTick，需 Inhibit 功能） | ❌ 不需要 | Period GE + GrantTag |
| 1003 速度叠加 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier Stackable |
| 1004 击退 | ✅ 1个（KnockbackStagger，需 SetByCaller） | ✅ 1个（物理冲量） | GE+GA 不可避免 |
| 1005 流血 | ❌ 不需要 | ✅ 1个（速度扣血逻辑） | GrantTag 替代 GE_Bleeding |
| 1006 额外伤害 | ❌ 不需要 | ❌ 不需要 | AddTag 守卫 + DoDamage |

**v2 变化：** 从 6 个 Blueprint GE 减少到 **2 个**（HeatTick、KnockbackStagger）。

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
| `Buff.Data.AttributeMod` | 参数层 | ★ 已有 | GE_KnockbackStagger SetByCaller 键 |

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

## 符文 4：击退（1004）✅ 需要 GE + GA

**设计：** 命中敌人后：① 对其施加移速 -300 硬直 1 秒；② 施加物理击退冲量。

**需要创建：** GE_KnockbackStagger（Blueprint，SetByCaller）+ GA_Knockback（Blueprint）+ FA + DA

> **为什么需要 Blueprint GE？** 移速减少量由 FA 运行时传入（SetByCaller），GE 内部不写死数值，方便阶段调整。
> **为什么需要 Blueprint GA？** 物理冲量（LaunchCharacter）是 AbilityTask 类操作，不适合用 FA 节点。

### 4-1 GE：`GE_KnockbackStagger`

| 配置项 | 值 | 说明 |
|---|---|---|
| Duration Policy | **Has Duration**，Duration = 1.0 | 硬直 1 秒自动解除 |
| Stacking | AggregateByTarget, Limit=1, Refresh | 命中刷新，不叠层 |
| **Modifiers** | Attribute=MoveSpeed, Op=Additive, **Magnitude Type=SetByCaller**, Tag=`Buff.Data.AttributeMod` | 数值由 FA 运行时传入 |
| Asset Tags | `Buff.Effect.Attribute.MoveSpeed` | 行为层标识 |

> GE 只声明"我需要一个数值"，FA 节点负责传入具体值（-300）。

### 4-2 GA：`GA_Knockback`

**授予方式：** 在敌人角色 BP 的 BeginPlay → GiveAbility(GA_Knockback)

| 配置字段 | 值 |
|---|---|
| Ability Tags | `Ability.Knockback`（自定义） |
| Activation Owned Tags | `Buff.Status.Knockback` |

**ActivateAbility 蓝图逻辑：**
```
ActivateAbility
  → GetOwningActorFromActorInfo → Cast to Character → TargetChar
  → 获取攻击方向（或固定向外方向）
  → LaunchCharacter(TargetChar, 方向 × 800, OverrideXY=true, OverrideZ=true)
  → Remove Gameplay Tag From Actor Owner (Buff.Status.Knockback, Count=1)
  → End Ability
```

### 4-3 FA：`FA_Rune_Knockback`

```
[Start] ──→ [OnDamageDealt]
                ↓
            [Apply Gameplay Effect Class]       ← BFNode_ApplyEffect
                Effect            = GE_KnockbackStagger
                Level             = 1.0
                Target            = 上次伤害目标
                SetByCallerTag1   = Buff.Data.AttributeMod
                SetByCallerValue1 = -300.0          ← 移速 -300，填负值
                ↓ Out
            [Add Tag]                           ← BFNode_AddTag
                Tag    = Buff.Status.Knockback
                Count  = 1
                Target = 上次伤害目标
```

**节点顺序：** 先施加 GE（硬直），再写 Tag（触发 GA 冲量）。
**Cleanup：** ApplyEffect 移除 GE；AddTag 移除残留的 Knockback Tag。

### 4-4 DA：`DA_Rune_Knockback`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1004` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_Knockback` |

### 4-5 测试要点

- 命中后敌人移速降低，持续 1 秒
- 同时被弹飞（GA_Knockback 激活）
- GAS Debugger：目标 MoveSpeed 减少了 300
- 调整 SetByCallerValue1 测试不同力度

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

## 资产目录结构

```
Content/Game/Runes/
├── AttackUp/
│   ├── DA_Rune_AttackUp
│   └── FA_Rune_AttackUp
├── HeatUp/
│   ├── DA_Rune_HeatUp
│   ├── FA_Rune_HeatUp
│   └── GE_HeatTick                 ← 仅此符文需要 Blueprint GE
├── SpeedStack/
│   ├── DA_Rune_SpeedStack
│   └── FA_Rune_SpeedStack
├── Knockback/
│   ├── DA_Rune_Knockback
│   ├── FA_Rune_Knockback
│   ├── GE_KnockbackStagger         ← SetByCaller，必须用 Blueprint GE
│   └── GA_Knockback                ← 物理冲量，必须用 Blueprint GA
├── Bleed/
│   ├── DA_Rune_Bleed
│   ├── FA_Rune_Bleed
│   └── GA_Bleed                    ← 速度扣血逻辑，必须用 Blueprint GA
└── ExtraDamage/
    ├── DA_Rune_ExtraDamage
    └── FA_Rune_ExtraDamage
```

---

## 依赖总览

| 符文 | DA | FA | Blueprint GE | Blueprint GA | 备注 |
|------|----|----|-------------|-------------|------|
| 攻击强化 | ✓ | ✓ | — | — | 全零资产 |
| 热度提升 | ✓ | ✓ | ✓ GE_HeatTick | — | 仅因需要 Inhibit |
| 速度叠加 | ✓ | ✓ | — | — | 全零资产 |
| 击退 | ✓ | ✓ | ✓ GE_KnockbackStagger | ✓ GA_Knockback | GE 因 SetByCaller，GA 因物理 |
| 流血 | ✓ | ✓ | — | ✓ GA_Bleed | GE 由 GrantTag 替代 |
| 额外伤害 | ✓ | ✓ | — | — | 全零资产 |

---

## 制作顺序建议

1. **符文 1（攻击强化）** → 最简单，验证 ApplyAttributeModifier 基础路径
2. **符文 3（速度叠加）** → 验证 Stackable + Duration 模式
3. **符文 6（额外伤害）** → 验证递归守卫模式
4. **符文 5（流血）** → 验证 GrantTag + GA 激活链
5. **符文 2（热度提升）** → 验证 Period + OngoingTagRequirements Inhibit
6. **符文 4（击退）** → 最复杂，SetByCaller + GA 物理冲量

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
