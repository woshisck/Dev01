# 高感知测试符文设计指南

> 版本：v1.0（2026-04-17）  
> 用途：测试阶段专用符文，替代 1001/1002/1003/1006 等纯数值被动  
> 原则：**激活后肉眼立即可见，效果和操作之间的因果链清晰**  
> 接续文档：[TestRune_CreationGuide.md](TestRune_CreationGuide.md)（已有 1001-1016）

---

## 设计禁区

| 禁止 | 原因 |
|------|------|
| 攻击力/防御力 ±N 类被动 | 数字变化玩家感知不到 |
| 移速 ±N 类被动 | 幅度小时感知差，容易误判为角色基础手感 |
| CD 缩减类被动 | 完全不可见 |
| 概率类效果（触发率 <50%） | 随机触发不能建立稳定的因果认知 |

---

## 新增符文一览

| ID | 名称 | 触发 | 效果类型 | 感知方式 |
|----|------|------|----------|----------|
| 1017 | 击杀爆炸 | 击杀敌人 | AoE 爆炸伤害 | 大范围爆炸粒子 + 范围内敌人击飞 |
| 1018 | 生命汲取 | 命中敌人 | 回血 | 角色绿色回血数字 + 血条恢复 |
| 1019 | 燃烧印记 | 命中敌人 | 火焰 DoT | 敌人持续燃烧粒子，死亡不熄灭 |
| 1020 | 冲天一击 | 命中敌人 | 垂直击飞 | 敌人被弹向空中（Y 轴方向击退） |
| 1021 | 余震 | 敌人落地后 | 范围减速 | 落点产生震荡圈 + 范围内敌人减速 VFX |

---

## 符文 1017：击杀爆炸

> **定位：** 引导关卡核心符文，制造"啊哈时刻"。  
> **用途：** 引导三选一必选一个放入此符文池。

**设计：** 击杀敌人瞬间，在死亡位置生成一次范围爆炸，对半径 300cm 内所有敌人造成 30 点伤害并施加小幅击退。

### FA：`FA_Rune_KillExplosion`

```
[Start]
  ↓
[On Damage Dealt]
    ↓ OnDamageDealt（每次命中触发）

[Has Tag]
    Tag    = Action.Dead         ← 检查目标是否已在死亡状态
    Target = LastHitTarget
    ↓ Yes（目标确认死亡）   ↓ No → 忽略

[Spawn Gameplay Cue at Location]
    CueTag   = GameplayCue.Rune.KillExplosion
    Location = LastHitTarget.Location   ← 死亡位置

[Apply GE to Targets in Radius]
    GE       = GE_RuneKillExplosionDamage
    Radius   = 300.0
    Center   = LastHitTarget.Location
    Filter   = Enemy Only
    ← 注：复用 GA_Knockback 触发逻辑，传入 Action.Knockback 事件
```

**需要创建：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_KillExplosion` | 上述 Flow Graph |
| `GE_RuneKillExplosionDamage` | Instant 伤害 GE，Value = 30，同时发送 Action.Knockback 事件 |
| `GameplayCue.Rune.KillExplosion` | 爆炸粒子 GameplayCue（可复用现有爆炸 Niagara，缩小 50%）|
| `DA_Rune_KillExplosion` | RuneID=1017，Shape=1×1 |

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1017` |
| RuneConfig.RuneName | `击杀爆炸` |
| RuneConfig.RuneDescription | `击杀敌人时，在其位置引发一次爆炸，波及附近敌人。` |
| RuneConfig.RuneType | `Buff` |
| Shape.Cells | `(0,0)` （1×1）|
| Flow.FlowAsset | `FA_Rune_KillExplosion` |

**已知限制：**
- `Has Tag(Action.Dead)` 依赖死亡 Tag 授予时序，若 GA_Dead 比伤害处理延迟一帧，需改用 `LastDamageOutput.bKillingBlow`（待 FDamageBreakdown 补充此字段）

---

## 符文 1018：生命汲取

> **定位：** 回血类，让玩家体验"攻击即治疗"的正向循环。

**设计：** 每次命中敌人，回复相当于本次伤害 15% 的 HP。有回血数字浮现（绿色）。

### FA：`FA_Rune_LifeSteal`

```
[Start]
  ↓
[On Damage Dealt]
    ↓ OnDamageDealt

[Apply Attribute Modifier]
    Attribute    = BaseAttributeSet.HP
    ModOp        = Additive
    Value        = LastDamageOutput.FinalDamage × 0.15   ← 表达式节点
    DurationType = Instant
    Target       = BuffOwner

[Spawn Gameplay Cue on Actor]
    CueTag = GameplayCue.Rune.LifeSteal   ← 绿色回血数字 + 粒子
    Target = BuffOwner
```

**需要创建：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_LifeSteal` | 上述 Flow Graph |
| `GameplayCue.Rune.LifeSteal` | 绿色粒子 + 数字显示（可复用伤害浮字系统，改为绿色）|
| `DA_Rune_LifeSteal` | RuneID=1018，Shape=1×1 |

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1018` |
| RuneConfig.RuneName | `生命汲取` |
| RuneConfig.RuneDescription | `每次命中敌人，恢复造成伤害 15% 的生命值。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_LifeSteal` |

**已知限制：**
- 需要 `LastDamageOutput.FinalDamage` 可在 FA 中作为数值引脚读取（依赖 COMBAT-003 的 FDamageBreakdown 委托暴露到 BF 节点）
- 回血上限建议钳制：单次最多回 Max HP × 5%，防止高倍率秒杀场景下爆炸回满

---

## 符文 1019：燃烧印记

> **定位：** 持续 DoT，但视觉冲击极强（持续火焰粒子）。

**设计：** 每次攻击命中，对目标施加燃烧状态：持续 3 秒，每 0.5 秒造成 5 点伤害，期间目标身上持续播放火焰粒子。同一目标重复命中时刷新计时。

### FA：`FA_Rune_BurnMark`

```
[Start]
  ↓
[On Damage Dealt]
    ↓

[Send Gameplay Event]
    EventTag = Buff.Event.Burn
    Target   = LastHitTarget
    Payload  = 5.0           ← 每跳伤害量（复用 GA_Bleed 模式）
```

> `GA_Burn`（复用 `GA_Bleed` 结构，OwnedTagPresent = `Buff.Status.Burning`）
> 每 0.5s 伤害 5；到期自动清理；重复触发刷新计时

**需要创建：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_BurnMark` | 上述 Flow Graph |
| `C++ GA_Burn` | 复制 GA_Bleed，改 Tag 为 `Buff.Status.Burning`，Interval=0.5，GC=`GameplayCue.Rune.Burn` |
| `GameplayCue.Rune.Burn` | 橙红火焰 Niagara 附着粒子（Loop，attached to target）|
| `DA_Rune_BurnMark` | RuneID=1019，Shape=1×1 |

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1019` |
| RuneConfig.RuneName | `燃烧印记` |
| RuneConfig.RuneDescription | `命中敌人后附加燃烧，每 0.5 秒造成 5 点伤害，持续 3 秒。重复命中刷新计时。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_BurnMark` |

**已知限制：**
- 需要新增 `C++ GA_Burn` 和 `Buff.Status.Burning` Tag
- 多个敌人同时燃烧时粒子数量较多，低端机注意性能

---

## 符文 1020：冲天一击

> **定位：** 垂直击飞，视觉最夸张，适合"展示系统存在感"。

**设计：** 命中敌人时，有 100% 概率将其向正上方弹飞（Y 轴 +800cm 冲量），2 秒后落地，受重力自然落下。

### FA：`FA_Rune_Uppercut`

```
[Start]
  ↓
[On Damage Dealt]
    ↓

[Send Gameplay Event]
    EventTag = Action.Uppercut      ← 新 Tag
    Target   = LastHitTarget
    Payload  = 800.0                ← 上弹冲量
```

> `GA_Uppercut`（C++，复用 GA_Knockback 结构，Launch 方向改为 `FVector::UpVector`，冲量 800）

**需要创建：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_Uppercut` | 上述 Flow Graph |
| `C++ GA_Uppercut` | 复用 GA_Knockback，Launch 方向 = UpVector，附加 `Buff.Status.Airborne` Tag 持续 2s |
| `GameplayCue.Rune.Uppercut` | 击飞瞬间白色冲击波（单次），落地时尘埃粒子（单次）|
| `DA_Rune_Uppercut` | RuneID=1020，Shape=1×1 |

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1020` |
| RuneConfig.RuneName | `冲天一击` |
| RuneConfig.RuneDescription | `命中敌人时将其弹向空中，短暂失控，随重力落下。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_Uppercut` |

**已知限制：**
- 需要角色胶囊在 Airborne 期间改为 Falling 移动模式（或禁用 AI 移动组件）
- 俯视角游戏中敌人弹向 Z 轴会短暂离开镜头，建议限制最大高度或镜头跟随

---

## 符文 1021：余震

> **定位：** 配合 1004 击退或 1020 冲天一击使用，形成基础协同感知。

**设计：** 每当己方触发击退（含冲天一击），在目标落点产生范围震荡圈，对半径 250cm 内其他敌人造成 20 点伤害并减速 50% 持续 2 秒。

### FA：`FA_Rune_Aftershock`

```
[Start]
  ↓
[Wait Gameplay Event]
    EventTag = Event.Rune.KnockbackApplied   ← 已有 Tag（1007 联动用）
    ↓ OnEventReceived

[Spawn Gameplay Cue at Location]
    CueTag   = GameplayCue.Rune.Aftershock
    Location = Event.Payload.Location        ← 击退落点

[Apply GE to Targets in Radius]
    GE       = GE_AfterShockSlow
    Radius   = 250.0
    Center   = Event.Payload.Location
    Filter   = Enemy Only
```

**需要创建：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_Aftershock` | 上述 Flow Graph |
| `GE_AfterShockSlow` | HasDuration 2s，MoveSpeed Additive -200，Magnitude Additive 20 伤害 |
| `GameplayCue.Rune.Aftershock` | 地面扩散圆环 Niagara（单次，持续约 0.5s）|
| `DA_Rune_Aftershock` | RuneID=1021，Shape=1×1 |

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1021` |
| RuneConfig.RuneName | `余震` |
| RuneConfig.RuneDescription | `击退敌人后，其落点产生震荡，波及附近敌人并使其减速。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_Aftershock` |

**已知限制：**
- 依赖 `Event.Rune.KnockbackApplied` 事件在 GA_Knockback 结束时广播（已在 1007 设计中要求，确认 GA_Knockback 末尾有 Send Event 调用）
- 若 Payload.Location 无法传递，改为监听最近死亡/被击退敌人的 LastKnownLocation

---

## 引导三选一推荐配置

> 引导关卡 `FallbackLootPool` 填入以下三个，保证每种触发方式都有代表：

| 槽位 | 符文 | 感知类型 |
|------|------|----------|
| 选项 A | 1017 击杀爆炸 | 击杀触发，AoE 视觉 |
| 选项 B | 1018 生命汲取 | 命中触发，回血数字 |
| 选项 C | 1019 燃烧印记 | 命中触发，持续粒子 |

> 1020/1021 加入正式符文池，不出现在引导阶段（避免引导期概念过多）。

---

## 已有高感知符文（可直接复用）

> 以下已实现符文满足"肉眼可见"标准，测试时可保留在符文池：

| ID | 名称 | 感知方式 |
|----|------|----------|
| 1004 | 击退 | 敌人位移 |
| 1008 | 刀光波 | 可见投射物 |
| 1013 | 震爆 | 击退后范围爆炸 |
| 1014 | 暗影疾驰 | 冲刺 VFX |

---

## 需要新增的 Tag

| Tag | 文件 | 用途 |
|-----|------|------|
| `Buff.Status.Burning` | BuffTag.ini | 燃烧状态守卫（1019）|
| `Buff.Status.Airborne` | BuffTag.ini | 空中状态守卫（1020）|
| `Action.Uppercut` | PlayerGameplayTag.ini | 上弹事件（1020）|
| `Buff.Event.Burn` | BuffTag.ini | GA_Burn 触发信号（1019）|
| `GameplayCue.Rune.KillExplosion` | — | GC 注册（1017）|
| `GameplayCue.Rune.LifeSteal` | — | GC 注册（1018）|
| `GameplayCue.Rune.Burn` | — | GC 注册（1019）|
| `GameplayCue.Rune.Uppercut` | — | GC 注册（1020）|
| `GameplayCue.Rune.Aftershock` | — | GC 注册（1021）|
