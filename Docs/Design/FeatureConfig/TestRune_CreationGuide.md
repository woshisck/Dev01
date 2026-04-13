# 测试符文制作指南 v5

> 版本：v5（2026-04-13）更新重点：1002 热度提升改为零资产（FA 层条件跳过替代 OngoingTagRequirements GE）；新增 1009 弱点窥破、1010 突刺连击、1011 毒牙、1012 幽风低语。  
> v4（2026-04-12）更新重点：新增符文 1008 刀光波（远程投射物，每 3 次命中触发，穿透多敌，C++ 投射物 + C++ 被动 GA）。  
> v3（2026-04-09）更新重点：击退拆分为两个独立符文（1004 击退 + 1007 击退减速），引入符文联动模式（GameplayEvent 跨符文通信）；GA_Knockback 改为 C++ 实现。  
> Tag 规范：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · 系统指南：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)

---

## 资产策略总览

| 符文 | Blueprint GE | C++/Blueprint GA | 说明 |
|------|-------------|-----------------|------|
| 1001 攻击强化 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier 直接处理 |
| 1002 热度提升 | ❌ 不需要 | ❌ 不需要 | On Periodic + Has Tag 条件跳过，零资产 |
| 1003 速度叠加 | ❌ 不需要 | ❌ 不需要 | ApplyAttributeModifier Stackable |
| 1004 击退 | ❌ 不需要 | ✅ 1个（C++ GA_Knockback，精确控距） | 纯位移，无减速；可单独使用 |
| 1007 击退减速 | ❌ 不需要 | ❌ 不需要 | 监听 1004 的击退事件，联动施加减速 |
| 1005 流血 | ❌ 不需要 | ✅ 1个（速度扣血逻辑） | GrantTag 替代 GE_Bleeding |
| 1006 额外伤害 | ❌ 不需要 | ❌ 不需要 | AddTag 守卫 + DoDamage |
| 1008 刀光波 | ✅ 1个（GE_SlashWaveDamage，SetByCaller 扣血） | ✅ 2个（C++ GA_SlashWaveCounter + C++ ASlashWaveProjectile 的 BP 子类） | 命中 3 次发射穿透投射物 |
| 1009 弱点窥破 | ❌ 不需要 | ❌ 不需要（测试版） | 连招末击额外伤害；C++ 版可接入真实暴击系统 |
| 1010 突刺连击 | ❌ 不需要 | ❌ 不需要 | On Dash + 2 秒窗口 + DoDamage |
| 1011 毒牙 | ❌ 不需要（测试版固定值）/ ✅ 1个（正式版 2% MaxHP） | ❌ 不需要 | On Crit Hit → 中毒 DoT |
| 1012 幽风低语 | ❌ 不需要 | ❌ 不需要 | On Periodic 读 MoveSpeed → 动态 Crit_Rate |

**v5 变化：** 1002 从"需要 Blueprint GE"改为零资产（FA 层 `Has Tag` 条件判断替代 GAS `OngoingTagRequirements`）；新增 1009-1012 四个符文。

---

## 前置：GameplayTag 创建

**路径：** Edit → Project Settings → Gameplay Tags

> ★ = 已有；☆ = 本次新增

| Tag | 文件 | 状态 | 用途 |
|---|---|---|---|
| `Buff.Effect.Attribute.Attack` | BuffTag.ini | ☆ | DA.RuneEffectTag 标识 |
| `Buff.Effect.Attribute.Heat` | BuffTag.ini | ☆ | 同上 |
| `Buff.Effect.Attribute.MoveSpeed` | BuffTag.ini | ★ | 同上 |
| `Buff.Status.HeatInhibit` | BuffTag.ini | ★ | 受伤后暂停热度守卫 |
| `Buff.Status.Bleeding` | BuffTag.ini | ★ | 流血状态 |
| `Buff.Status.ExtraDamageApplied` | BuffTag.ini | ★ | 额外伤害递归守卫 |
| `Buff.Status.Knockback` | BuffTag.ini | ★ | 击退期间状态 |
| `Buff.Status.SlashWaveSwingActive` | BuffTag.ini | ★ | 刀光挥刀模式守卫 |
| `Buff.Status.NextHitCrit` | BuffTag.ini | ☆ | 下一次伤害强制暴击信号（C++ 消费） |
| `Buff.Status.DuoAssaultReady` | BuffTag.ini | ☆ | 突刺连击冲刺后2秒待触发状态 |
| `Buff.Status.Poisoned` | BuffTag.ini | ☆ | 中毒状态 |
| `Action.Attack` | PlayerGameplayTag.ini | ★ | 攻击事件父节点 |
| `Action.Attack.Swing` | PlayerGameplayTag.ini | ★ | 挥刀帧事件 |
| `Action.Rune` | PlayerGameplayTag.ini | ★ | 符文内部事件父节点 |
| `Action.Rune.SlashWaveHit` | PlayerGameplayTag.ini | ★ | 刀光波命中计数事件 |
| `Action.Dead` | PlayerGameplayTag.ini | ★ | 死亡触发信号 |
| `Action.HitReact` | PlayerGameplayTag.ini | ★ | 受击硬直信号 |
| `Action.Knockback` | PlayerGameplayTag.ini | ★ | 击退触发信号 |
| `Event.Rune.KnockbackApplied` | PlayerGameplayTag.ini | ★ | 击退完成广播（联动用） |

---

## 符文 1：攻击强化（1001）⚡ 零资产

**设计：** 被动。激活期间永久 +10 攻击力。

**需要创建：** FA + DA

### 1-1 FA：`FA_Rune_AttackUp`

```
[Start] ──→ [Apply Attribute Modifier]
                Attribute    = BaseAttributeSet.Attack
                ModOp        = Additive
                Value        = 10.0
                DurationType = Infinite
                Target       = BuffOwner
```

**Cleanup：** FA 停止时节点自动移除 GE，攻击力恢复。

### 1-2 DA：`DA_Rune_AttackUp`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1001` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_AttackUp` |

### 1-3 测试要点

- GAS Debugger → Attributes：激活后 Attack +10
- 拖出激活区：恢复原值，不累积

---

## 符文 2：热度提升（1002）⚡ 零资产

**设计：** 被动。每秒 +1 热度。受到伤害后暂停 5 秒。

**需要创建：** FA + DA（无需 GE）

> **为什么不再需要 Blueprint GE？**  
> 旧版用 GAS 原生 `OngoingTagRequirements.IgnoreTags` 实现"有 Tag 时暂停 Period"，需要 Blueprint GE 配置。  
> 新版在 FA 层用 `On Periodic` + `Has Tag` 条件判断代替：每秒触发时先检查是否有 `HeatInhibit` Tag，有则跳过，无则加热度。效果完全等价，零额外资产。

### 2-1 FA：`FA_Rune_HeatUp`

```
[Start]
  ↓
[On Periodic]
    Interval        = 1.0
    FireImmediately = false
    ↓ Tick（每秒触发）

[Has Tag]
    Tag    = Buff.Status.HeatInhibit
    Target = BuffOwner
    ↓ No（无守卫 Tag，正常加热度）     ↓ Yes → （跳过，本秒不加热度）

[Apply Attribute Modifier]
    Attribute    = BaseAttributeSet.Heat
    ModOp        = Additive
    Value        = 1.0
    DurationType = Instant              ← 直接改基值，On Periodic 控制频率
    Target       = BuffOwner

─────────────────────────────────────────

[On Damage Received]
    ↓ OnDamage
[Grant Tag (Timed)]
    Tag      = Buff.Status.HeatInhibit
    Duration = 5.0
    Target   = BuffOwner
    ↓ Out → （继续监听下次受伤，再次受伤时刷新 5 秒计时）
```

**生命周期：**
- 激活 → On Periodic 开始每秒判断
- 受伤 → 授予 HeatInhibit 5s → 接下来数次 Tick 的 Has Tag 返回 Yes → 跳过
- 5s 后 Tag 到期 → Has Tag 返回 No → 恢复加热度
- 再次受伤 → GrantTag In 引脚再触发 → 重置 5 秒计时
- FA 停止 → On Periodic 停止；GrantTag Cleanup 移除 HeatInhibit

### 2-2 DA：`DA_Rune_HeatUp`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1002` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_HeatUp` |

### 2-3 测试要点

- 激活后每秒 Heat +1（GAS Debugger → Attributes 观察 Heat）
- 受伤后 5 秒内 Heat 停止增长
- 5 秒后恢复每秒增长
- 连续受伤：计时器刷新
- 拖出激活区：On Periodic 停止，HeatInhibit Tag 清理

---

## 符文 3：速度叠加（1003）⚡ 零资产

**设计：** 命中敌人后自身获得移速 +30，持续 3 秒，最多叠 5 层（+150）。每次命中刷新计时，逐层衰减。

**需要创建：** FA + DA

### 3-1 FA：`FA_Rune_SpeedStack`

```
[Start] ──→ [On Damage Dealt] ──→ [Apply Attribute Modifier]
                                      Attribute    = BaseAttributeSet.MoveSpeed
                                      ModOp        = Additive
                                      Value        = 30.0
                                      DurationType = HasDuration
                                      Duration     = 3.0
                                      Target       = BuffOwner
                                      StackMode    = Stackable
                                      StackLimit   = 5
                                      Duration Refresh  = Refresh On Successful Application
                                      Expiration Policy = Remove Single Stack And Refresh Duration
```

**堆叠行为：**
- 最多 5 层，每层 +30；每次命中刷新全部层计时；到期时逐层衰减

### 3-2 DA：`DA_Rune_SpeedStack`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1003` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_SpeedStack` |

### 3-3 测试要点

- 命中后 MoveSpeed +30；连续命中最多 +150（5层）
- 停止攻击：3 秒后开始逐层掉落
- 拖出激活区：全部层立即移除

---

## 符文 4：击退（1004）✅ 需要 C++ GA

**设计：** 命中敌人后，将其沿击退方向位移固定距离（默认 500cm）。  
**纯位移效果，不含减速。** 若需要击退后减速，搭配符文 1007（击退减速）使用。

**需要创建：** GA_Knockback（C++ 已实现）的 Blueprint 子类 + FA + DA

---

### 符文使用说明

> **独立使用：** 仅装备 1004，命中敌人后触发纯位移击退。  
> **联动使用：** 同时装备 1004 + 1007，击退完成后自动触发减速效果。联动不需要额外配置。

**工作流程（联动时）：**
```
FA_Rune_Knockback（1004）
  OnDamageDealt → 向目标发送 GameplayEvent(Action.Knockback)
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

### 4-1 GA：`BGA_Knockback`（GA_Knockback C++ 子类）

> Content Browser → Blueprint Class → 父类 `GA_Knockback` → 命名 `BGA_Knockback`

| 配置字段 | 值 |
|---|---|
| Activation Owned Tags | `Buff.Status.Knockback` |
| Trigger Tag / Source | `Action.Knockback` / `GameplayEvent` |
| KnockbackDistance | `500.0` |
| KnockbackDuration | `0.3` |
| bZeroVelocityOnFinish | `true` |

**授予方式：** 敌人角色 BP → BeginPlay → `GiveAbility(BGA_Knockback)`

---

### 4-2 FA：`FA_Rune_Knockback`

```
[Start] ──→ [On Damage Dealt]
                ↓
            [Send Gameplay Event]
                EventTag   = Action.Knockback
                Target     = LastDamageTarget
                Instigator = BuffOwner
```

### 4-3 DA：`DA_Rune_Knockback`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1004` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_Knockback` |

### 4-4 测试要点

- 命中后敌人被位移约 500cm
- 击退期间 GAS Debugger：目标 ASC 有 `Buff.Status.Knockback` Tag
- 击退结束后 Tag 自动消失

---

## 符文 7：击退减速（1007）⚡ 零资产 · 依赖符文 1004

**设计：** 监听击退完成事件，对被击退目标施加移速 -300，持续 1 秒。  
**必须与 1004 同时装备，单独装备无效果。**

### 7-1 FA：`FA_Rune_KnockbackStagger`

```
[Start]
  ↓
[Wait Gameplay Event]
    EventTag = Event.Rune.KnockbackApplied
    ↓ Out（收到事件时触发）

[Apply Attribute Modifier]
    Target       = EventData.Target        ← 被击退的目标
    Attribute    = BaseAttributeSet.MoveSpeed
    ModOp        = Additive
    Value        = -300.0
    DurationType = HasDuration
    Duration     = 1.0
↓ Out → （回到 Wait，等待下次击退）
```

### 7-2 DA：`DA_Rune_KnockbackStagger`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1007` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_KnockbackStagger` |

### 7-3 测试要点

- 单独装备 1007：无效果（正确）
- 同时装备 1004 + 1007：命中后先击退位移，击退完成瞬间目标移速降 300，1 秒后恢复

---

## 符文 5：流血（1005）⚡ 无需 Blueprint GE

**设计：** 命中敌人施加流血状态（10 秒），流血期间移动速度越快扣血越多（每秒扣血 = 速度 / 200）。

**需要创建：** GA_Bleed（Blueprint）+ FA + DA

### 5-1 GA：`GA_Bleed`

**授予方式：** 敌人角色 BP → BeginPlay → `GiveAbility(GA_Bleed)`

| 配置字段 | 值 |
|---|---|
| Activation Owned Tags | `Buff.Status.Bleeding` |

**ActivateAbility 蓝图逻辑（检测 Tag 消失退出循环）：**
```
ActivateAbility
  → Loop:
      WaitGameplayTag: Remove → Buff.Status.Bleeding（Tag 消失时退出）
      WaitDelay: 0.2s
      → GetOwnerVelocity().Size() → Speed
      → DamagePerTick = Speed / 200.0 × 0.2
      → if DamagePerTick > 0.01: ApplyDamage(OwnerActor, DamagePerTick)
  ← Loop 结束
  → EndAbility
```

> 每 0.2 秒检测，等价于"每秒 Speed/200 点伤害"，帧率无关。

### 5-2 FA：`FA_Rune_Bleed`

```
[Start] ──→ [On Damage Dealt] ──→ [Grant Tag (Timed)]
                                      Tag      = Buff.Status.Bleeding
                                      Duration = 10.0
                                      Target   = LastDamageTarget
                                      ↓ Out → （继续监听，刷新流血计时）
```

**生命周期：** 命中 → 授予 Bleeding 10s → GA_Bleed 激活 → 再次命中刷新计时 → 10s 未命中 Tag 到期 → GA_Bleed 退出

### 5-3 DA：`DA_Rune_Bleed`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1005` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_Bleed` |

### 5-4 测试要点

- 命中后目标 ASC 出现 `Buff.Status.Bleeding` Tag
- 移动中目标持续掉血（移速越高越快）；静止不扣血
- 10 秒未命中：Tag 消失，GA_Bleed 停止
- 连续命中：计时器刷新

---

## 符文 6：额外伤害（1006）⚡ 零资产

**设计：** 每次造成伤害时额外附带 1 点伤害，防止递归。

### 6-1 FA：`FA_Rune_ExtraDamage`（带递归守卫）

```
[Start] ──→ [On Damage Dealt]
                ↓
            [Has Tag]
                Tag    = Buff.Status.ExtraDamageApplied
                Target = BuffOwner
                ↓ No
            [Add Tag]
                Tag    = Buff.Status.ExtraDamageApplied
                ↓
            [Do Damage]
                Target = LastDamageTarget
                Damage = 1.0
                ↓
            [Remove Tag]
                Tag    = Buff.Status.ExtraDamageApplied
            （Yes 分支 → 直接跳过）
```

### 6-2 DA：`DA_Rune_ExtraDamage`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1006` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_ExtraDamage` |

---

## 符文 8：刀光波（1008）✅ 需要 1 个 Blueprint GE + 2 个 Blueprint GA（C++ 子类）

**设计：** 命中敌人后累计次数，每命中 3 次向前方发射一道水平刀光波，穿透多个敌人。

| 模式 | bHitRequired | 触发条件 |
|------|-------------|---------|
| 命中模式（默认） | `true` | 攻击命中敌人后计数 |
| 挥刀模式 | `false` | 每次挥刀帧计数（无论是否命中） |

**需要创建：** GE_SlashWaveDamage + BP_SlashWaveProjectile + BGA_SlashWaveCounter + FA + DA

---

### 8-1 GE：`GE_SlashWaveDamage`

| 配置项 | 值 |
|---|---|
| Duration Policy | **Instant** |
| Modifier Attribute | 目标 HP 属性, Op=Additive |
| Modifier Magnitude | **SetByCaller**, Tag=`Attribute.ActDamage` |

---

### 8-2 GA：`BGA_SlashWaveCounter`（GA_SlashWaveCounter C++ 子类）

> Content Browser → Blueprint Class → 父类 `GA_SlashWaveCounter` → 命名 `BGA_SlashWaveCounter`

| 配置字段 | 值 |
|---|---|
| HitsRequired | `3` |
| bHitRequired | `true`（命中模式） |
| SwingModeGateTag | 留空 |
| ProjectileClass | `BP_SlashWaveProjectile` |
| SlashDamage | `30.0` |
| SlashDamageEffect | `GE_SlashWaveDamage` |
| SpawnOffset | `80.0` |

**授予方式：** 玩家角色 BP → BeginPlay → `GiveAbility(BGA_SlashWaveCounter)`

---

### 8-3 Projectile：`BP_SlashWaveProjectile`（ASlashWaveProjectile C++ 子类）

> Content Browser → Blueprint Class → 父类 `SlashWaveProjectile`

| 字段 | 默认值 |
|---|---|
| Lifetime | `1.2` |
| Speed | `1400` |

表现层（BP 事件图）：`BP_OnHitEnemy` → 命中粒子/音效；`BP_OnExpired` → 消散特效

---

### 8-4 FA：`FA_Rune_SlashWave`（命中模式）

```
[Start]
  ↓
[On Damage Dealt]
  ↓ Out
[Send Gameplay Event]
    EventTag   = Action.Rune.SlashWaveHit
    Target     = BuffOwner
    Instigator = BuffOwner
↓ Out → （继续监听下次命中）
```

---

### 8-5 DA：`DA_Rune_SlashWave`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1008` |
| RuneConfig.RuneType | `Buff` |
| Flow.FlowAsset | `FA_Rune_SlashWave` |

---

### 8-6 测试要点

1. 不装备：攻击无刀光
2. 装备后：命中 3 次生成投射物
3. 穿透：刀光经过 2 个敌人，两个都受伤
4. 计数重置：第 3 次触发后归零
5. 移出激活区：不再发送事件，不再触发刀光

---

### 8-7 挥刀模式附加配置

额外创建 `BGA_SlashWaveCounter_Swing`（差异配置）：

| 字段 | 值 |
|---|---|
| bHitRequired | `false` |
| SwingModeGateTag | `Buff.Status.SlashWaveSwingActive` |

`FA_Rune_SlashWave_Swing`：
```
[Start] ──→ [Grant Tag]
                Tag      = Buff.Status.SlashWaveSwingActive
                Duration = Infinite
                Target   = BuffOwner
（FA 停止时 Cleanup 自动移除守卫 Tag）
```

---

## 符文 9：弱点窥破（1009）⚡ 零资产（测试版）

**设计：** 连招最后一击（Combo4）命中时，额外造成相当于暴击加成的伤害（等效强制暴击）。

**需要创建：** FA + DA

> **测试版 vs 正式版**  
> 测试版：FA 层用 `Has Tag(PlayerState.AbilityCast.LightAtk.Combo4)` 检测末击，用 `Do Damage` 额外伤害模拟暴击。此方案不触发 `On Crit Hit` 事件，毒牙等暴击联动符文不会响应。  
> 正式版：在 C++ 伤害管线中加入 `Buff.Status.NextHitCrit` Tag 检测，实现真实暴击（触发 `On Crit Hit`）。

### 9-1 FA：`FA_Rune_WeaknessUnveiled`

```
[Start] ──→ [On Damage Dealt]
                ↓
            [Has Tag]
                Tag    = PlayerState.AbilityCast.LightAtk.Combo4
                Target = BuffOwner
                ↓ Yes（末击命中）

            [Has Tag]
                Tag    = Buff.Status.ExtraDamageApplied
                Target = BuffOwner
                ↓ No

            [Add Tag]
                Tag    = Buff.Status.ExtraDamageApplied
                ↓
            [Do Damage]
                Target     = LastDamageTarget
                Multiplier = 0.5              ← 额外 50% 伤害（CritDamage=1.5 时的加成量）
                ↓
            [Remove Tag]
                Tag    = Buff.Status.ExtraDamageApplied
            （Yes 分支 → 跳过，递归守卫）
```

> 若武器连招为重击或其他组合，同样可添加 `HeavyAtk.Combo4` 分支。

### 9-2 DA：`DA_Rune_WeaknessUnveiled`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1009` |
| Buff.Rune.Type | `Buff.Rune.Type.Attack` |
| Buff.Rune.Rarity | `Buff.Rune.Rarity.Rare` |
| Flow.FlowAsset | `FA_Rune_WeaknessUnveiled` |

### 9-3 测试要点

- 普通命中（非末击）：无额外效果
- 末击（Combo4）命中：目标多受 50% 本次伤害
- 递归守卫验证：额外伤害不再触发第二次 Do Damage
- 多连击：每个 Combo4 命中都触发一次

---

## 符文 10：突刺连击（1010）⚡ 零资产

**设计：** 冲刺后 2 秒内下一次攻击造成双倍伤害。双倍伤害不走暴击流程，不触发 `On Crit Hit`。

**需要创建：** FA + DA

### 10-1 FA：`FA_Rune_DuoAssault`

```
[Start]

[On Dash]
    ↓ OnDash
[Grant Tag (Timed)]
    Tag      = Buff.Status.DuoAssaultReady
    Duration = 2.0
    Target   = BuffOwner
    ↓ Out → （继续监听下次冲刺，重复冲刺会刷新 2 秒窗口）

──────────────────────────────────────────

[On Damage Dealt]
    ↓ OnDamage
[Has Tag]
    Tag    = Buff.Status.DuoAssaultReady
    Target = BuffOwner
    ↓ Yes

[Remove Tag]
    Tag    = Buff.Status.DuoAssaultReady    ← 立即消耗，确保只生效一次
    ↓
[Has Tag]
    Tag    = Buff.Status.ExtraDamageApplied
    Target = BuffOwner
    ↓ No

[Add Tag]
    Tag    = Buff.Status.ExtraDamageApplied
    ↓
[Do Damage]
    Target     = LastDamageTarget
    Multiplier = 1.0                        ← 额外 100% 伤害，合计 200%
    ↓
[Remove Tag]
    Tag    = Buff.Status.ExtraDamageApplied
```

**关键设计：**
- `Grant Tag (Timed)` 的 `In` 引脚重复触发自动重置 2 秒计时
- `Remove Tag` 手动消耗状态，确保只在下一次攻击生效，而非持续 2 秒内所有攻击
- `ExtraDamageApplied` 守卫防止 Do Damage 递归触发

### 10-2 DA：`DA_Rune_DuoAssault`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1010` |
| Buff.Rune.Type | `Buff.Rune.Type.Attack` |
| Buff.Rune.Rarity | `Buff.Rune.Rarity.Rare` |
| Flow.FlowAsset | `FA_Rune_DuoAssault` |

### 10-3 测试要点

- 无冲刺：攻击无额外效果
- 冲刺后立即攻击：本次攻击伤害翻倍
- 冲刺后 2 秒内攻击：翻倍（消耗后不再翻）
- 冲刺后超过 2 秒：状态消失，攻击正常
- 冲刺后再次冲刺：2 秒窗口刷新
- 翻倍伤害不出现暴击颜色/不触发 `On Crit Hit`

---

## 符文 11：毒牙（1011）⚡ 测试版零资产

**设计：** 触发暴击时，对目标施加中毒（每秒持续扣血，持续 5 秒）。  
重复暴击刷新中毒持续时间（不叠加层数）。

**需要创建：** FA + DA（测试版）；正式版另需 GE_Poison（Blueprint GE）

> **测试版 vs 正式版**  
> 测试版：固定每秒扣 25 点 HP（`Apply Attribute Modifier` 直接处理，零资产）  
> 正式版：`GE_Poison`（AttributeBased 计算：MaxHealth × -0.02，即每秒扣最大生命值的 2%）

### 11-1 FA：`FA_Rune_VenomFang`（测试版）

```
[Start]
  ↓
[On Crit Hit]
    ↓ OnCrit
[Apply Attribute Modifier]
    Attribute    = BaseAttributeSet.Health
    ModOp        = Additive
    Value        = -25.0                     ← 测试用固定值
    DurationType = HasDuration
    Duration     = 5.0
    Period       = 1.0                       ← 每秒触发一次
    FireImmediately = false
    Target       = LastDamageTarget
    StackMode    = Unique                    ← 重复中毒只刷新持续时间
  ↓
[Grant Tag (Timed)]
    Tag      = Buff.Status.Poisoned
    Duration = 5.0
    Target   = LastDamageTarget             ← 状态图标 / 其他符文查询用
```

### 11-2 正式版：`GE_Poison`（Blueprint GE）

需要 AttributeBased magnitude 时创建：

| 配置项 | 值 |
|---|---|
| Duration Policy | **Has Duration = 5.0s** |
| Period | `1.0` |
| FireImmediately | `false` |
| Modifier Attribute | Health, Op=Additive |
| Magnitude Type | **AttributeBased** |
| Backing Attribute | `MaxHealth` |
| Coefficient | `-0.02` |
| Stacking | AggregateByTarget, Limit=1, Duration Refresh=刷新 |
| Granted Tags | `Buff.Status.Poisoned`（GE 到期自动移除） |

正式版 FA 简化：
```
[Start]
  ↓
[On Crit Hit]
    ↓
[Apply Gameplay Effect Class]
    Effect = GE_Poison
    Target = LastDamageTarget
```

### 11-3 DA：`DA_Rune_VenomFang`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1011` |
| Buff.Rune.Type | `Buff.Rune.Type.Attack` |
| Buff.Rune.Rarity | `Buff.Rune.Rarity.Epic` |
| Flow.FlowAsset | `FA_Rune_VenomFang` |

### 11-4 测试要点

- 普通攻击：无中毒效果
- 暴击命中：目标出现 `Buff.Status.Poisoned` Tag，5 秒内持续掉血
- 再次暴击：持续时间刷新（不叠层，仍每秒 -25）
- 5 秒后无暴击：Tag 消失，停止扣血
- 与弱点窥破（1009）联动说明：测试版弱点窥破用 Do Damage 不触发 `On Crit Hit`，毒牙不会响应；正式版（C++ NextHitCrit）可联动

---

## 符文 12：幽风低语（1012）⚡ 零资产

**设计：** 获得移动速度提升时，获得相同百分比的暴击率提升，持续 5 秒。  
每 0.5 秒检测一次当前速度加成，动态计算并持续刷新暴击率加成。

**需要创建：** FA + DA

> **数值逻辑：** 基础 MoveSpeed = 600。当前速度超出 600 的部分 ÷ 6 = 暴击率加成（%）。  
> 例：MoveSpeed = 660（+30 × 1 层速度叠加）→ 加成 = 30 ÷ 6 = 5 → Crit_Rate +5

### 12-1 FA：`FA_Rune_WraithwindWhisper`

```
[Start]
  ↓
[On Periodic]
    Interval        = 0.5
    FireImmediately = true
    ↓ Tick

[Get Attribute]
    Attribute = BaseAttributeSet.MoveSpeed
    Target    = BuffOwner
    ↓ Out
    │ CachedValue ──────────────────────────────────────────────┐
    ↓                                                           │
[Compare Float]                                            [Math Float]
    A ← CachedValue（通过 Math Float A-B=600 的 Result）       A ← CachedValue
    Operator = >                                               Operator = -
    B = 0                                                      B = 600
    ↓ True                                                     ↓ Result（bonus）
    ↓ False → （速度无加成，跳过本次 Tick）

[Apply Attribute Modifier]
    Attribute    = BaseAttributeSet.Crit_Rate
    ModOp        = Additive
    Value        ← [Math Float: bonus ÷ 6].Result   ← 数据引脚连线
    DurationType = HasDuration
    Duration     = 5.0
    Target       = BuffOwner
    StackMode    = Unique                            ← 每次刷新持续时间，不叠加
```

**数据引脚连线说明：**
1. `Get Attribute.CachedValue` → `Math Float(减600).A`
2. `Math Float(减600).Result`（= bonus）→ `Compare Float.A` 和 `Math Float(除6).A`
3. `Math Float(除6).Result`（= pct）→ `Apply Attribute Modifier.Value`
4. Compare Float 控制执行流：True 才继续到 Apply Attribute Modifier

**行为说明：**
- 每 0.5s 重新采样 MoveSpeed，若有加成则更新 Crit_Rate 并重置 5s 计时
- 速度加成消失后，Crit_Rate 加成再维持 5 秒然后过期
- 与符文 1003（速度叠加）天然联动：命中叠速度 → 幽风低语自动跟进 Crit_Rate

> **注意：** 基础速度 600 目前硬编码于 FA 节点。如角色有永久速度 Buff，需手动同步此数值。

### 12-2 DA：`DA_Rune_WraithwindWhisper`

| 字段 | 值 |
|---|---|
| RuneConfig.RuneID | `1012` |
| Buff.Rune.Type | `Buff.Rune.Type.Utility` |
| Buff.Rune.Rarity | `Buff.Rune.Rarity.Rare` |
| Flow.FlowAsset | `FA_Rune_WraithwindWhisper` |

### 12-3 测试要点

- 无速度加成时：Crit_Rate 不变
- 装备速度叠加符文（1003）后命中：速度上升 → Crit_Rate 同步上升
- MoveSpeed=660（1层）→ Crit_Rate +5；MoveSpeed=750（5层）→ Crit_Rate +25
- 停止命中后速度下降：Crit_Rate 加成维持 5 秒后消失
- 单独装备（无速度符文）：冲刺等带速度 Buff 的操作也会触发

---

## 资产目录结构

```
Content/Game/Runes/
├── AttackUp/
│   ├── DA_Rune_AttackUp
│   └── FA_Rune_AttackUp
├── HeatUp/
│   ├── DA_Rune_HeatUp
│   └── FA_Rune_HeatUp                      ← v5：不再需要 GE_HeatTick
├── SpeedStack/
│   ├── DA_Rune_SpeedStack
│   └── FA_Rune_SpeedStack
├── Knockback/
│   ├── DA_Rune_Knockback                   ← 符文 1004
│   ├── FA_Rune_Knockback
│   └── BGA_Knockback
├── KnockbackStagger/
│   ├── DA_Rune_KnockbackStagger            ← 符文 1007，联动符文
│   └── FA_Rune_KnockbackStagger
├── Bleed/
│   ├── DA_Rune_Bleed
│   ├── FA_Rune_Bleed
│   └── GA_Bleed
├── ExtraDamage/
│   ├── DA_Rune_ExtraDamage
│   └── FA_Rune_ExtraDamage
├── SlashWave/
│   ├── DA_Rune_SlashWave
│   ├── FA_Rune_SlashWave
│   ├── FA_Rune_SlashWave_Swing             ← 挥刀模式（可选）
│   ├── BGA_SlashWaveCounter
│   ├── BP_SlashWaveProjectile
│   └── GE_SlashWaveDamage
├── WeaknessUnveiled/
│   ├── DA_Rune_WeaknessUnveiled
│   └── FA_Rune_WeaknessUnveiled
├── DuoAssault/
│   ├── DA_Rune_DuoAssault
│   └── FA_Rune_DuoAssault
├── VenomFang/
│   ├── DA_Rune_VenomFang
│   ├── FA_Rune_VenomFang
│   └── GE_Poison                           ← 正式版才需要
└── WraithwindWhisper/
    ├── DA_Rune_WraithwindWhisper
    └── FA_Rune_WraithwindWhisper
```

---

## 依赖总览

| 符文 | DA | FA | Blueprint GE | C++/BP GA | 依赖 | 备注 |
|------|----|----|-------------|-----------|------|------|
| 1001 攻击强化 | ✓ | ✓ | — | — | — | 全零资产 |
| 1002 热度提升 | ✓ | ✓ | — | — | — | 全零资产（v5 修正） |
| 1003 速度叠加 | ✓ | ✓ | — | — | — | 全零资产 |
| 1004 击退 | ✓ | ✓ | — | ✓ BGA_Knockback | — | 可独立使用 |
| 1007 击退减速 | ✓ | ✓ | — | — | **必须装备 1004** | 联动模式范例 |
| 1005 流血 | ✓ | ✓ | — | ✓ GA_Bleed | — | GE 由 GrantTag 替代 |
| 1006 额外伤害 | ✓ | ✓ | — | — | — | 全零资产 |
| 1008 刀光波 | ✓ | ✓ | ✓ GE_SlashWaveDamage | ✓ BGA_SlashWaveCounter + BP_SlashWaveProjectile | 玩家 BP 需 GiveAbility | 命中计数器 + 穿透投射物 |
| 1009 弱点窥破 | ✓ | ✓ | — | — | — | 测试版零资产；正式版需 C++ |
| 1010 突刺连击 | ✓ | ✓ | — | — | — | 全零资产 |
| 1011 毒牙 | ✓ | ✓ | — / ✓ GE_Poison | — | — | 测试版零资产；正式版需 GE |
| 1012 幽风低语 | ✓ | ✓ | — | — | — | 全零资产；与 1003 天然联动 |

---

## 制作顺序建议

1. **1001 攻击强化** → 验证 ApplyAttributeModifier 基础路径
2. **1003 速度叠加** → 验证 Stackable + Duration 模式
3. **1006 额外伤害** → 验证递归守卫模式（ExtraDamageApplied）
4. **1002 热度提升** → 验证 On Periodic + Has Tag 条件跳过模式
5. **1005 流血** → 验证 GrantTag + GA 激活链
6. **1004 击退** → 验证 GameplayEvent 触发 GA + RootMotion
7. **1007 击退减速** → 验证符文联动模式（Wait Gameplay Event）
8. **1008 刀光波** → 验证 C++ 投射物 + 持久被动 GA + 计数链路
9. **1010 突刺连击** → 验证 On Dash + GrantTag Timed + Do Damage
10. **1009 弱点窥破** → 验证末击检测 + Do Damage 额外伤害
11. **1011 毒牙** → 验证 On Crit Hit + 周期性属性修改
12. **1012 幽风低语** → 验证 On Periodic + Get Attribute + 数据引脚 Math 链

---

## 制作反馈区

> 遇到问题、调整、实际属性名等记录在下方：

```
[反馈] 符文1 - 实际属性名是 ___ 而不是 BaseAttributeSet.Attack
[反馈] 符文2 - On Periodic 与 Has Tag 执行顺序：___
[反馈] 符文12 - 基础 MoveSpeed 实际值确认为 ___
[反馈] 符文9 - LightAtk.Combo4 Tag 确认可 Has Tag 查询：是/否
[反馈] 符文11 - On Crit Hit 确认触发时机：___
```
