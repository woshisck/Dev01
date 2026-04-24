# 符文制作主指南 v1.0

> 版本：v1.0（2026-04-17）  
> 范围：符文 1001-1021 全量合并 + 制作优先级排列 + 表现层规格  
> 前置文档：[TestRune_CreationGuide.md](TestRune_CreationGuide.md)（逻辑层详细 FA 流程）  
>　　　　　[TestRune_HighPerception_Guide.md](TestRune_HighPerception_Guide.md)（1017-1021 高感知符文设计）  
> 用途：策划一站式制作参考，逻辑层 + 表现层均在此对齐

---

## 一、优先级总览

| 优先级 | ID | 名称 | 触发条件 | 感知等级 | 资产量 | 状态 |
|-------|-----|------|---------|---------|-------|------|
| **P0** | 1004 | 击退 | 命中 | ⭐⭐⭐⭐⭐ | FA+DA+C++GA | 逻辑已实现，表现待补 |
| **P0** | 1017 | 击杀爆炸 | 击杀 | ⭐⭐⭐⭐⭐ | FA+DA+GE+GC | 待创建 |
| **P0** | 1018 | 生命汲取 | 命中 | ⭐⭐⭐⭐⭐ | FA+DA+GC | 待创建 |
| **P0** | 1019 | 燃烧印记 | 命中 | ⭐⭐⭐⭐⭐ | FA+DA+C++GA+GC | 待创建 |
| **P1** | 1008 | 刀光波 | 命中×3 | ⭐⭐⭐⭐⭐ | FA+DA+GE+BP×2 | 逻辑已实现，GC待补 |
| **P1** | 1013 | 震爆 | 暴击 | ⭐⭐⭐⭐ | FA+DA（复用GA） | 逻辑已实现，GC待补 |
| **P1** | 1020 | 冲天一击 | 命中 | ⭐⭐⭐⭐⭐ | FA+DA+C++GA+GC | 待创建 |
| **P1** | 1021 | 余震 | 击退后 | ⭐⭐⭐⭐ | FA+DA+GE+GC | 待创建 |
| **P1** | 1005 | 流血 | 命中 | ⭐⭐⭐ | FA+DA+C++GA+GC | 逻辑已实现，GC待补 |
| **P1** | 1002 | 热度提升 | 被动每秒 | ⭐⭐ | FA+DA | 逻辑已实现 |
| **P2** | 1010 | 突刺连击 | 冲刺后攻击 | ⭐⭐⭐ | FA+DA+GC | 逻辑已实现，GC待补 |
| **P2** | 1009 | 弱点窥破 | 末击命中 | ⭐⭐⭐ | FA+DA+GC | 逻辑已实现，GC待补 |
| **P2** | 1016 | 致命先机 | 命中满血敌 | ⭐⭐⭐ | FA+DA+GC | 逻辑已实现，GC待补 |
| **P2** | 1011 | 毒牙 | 暴击 | ⭐⭐⭐ | FA+DA+GC | 逻辑已实现，GC待补 |
| **P2** | 1014 | 暗影疾驰 | 被动 | ⭐⭐ | FA+DA | 逻辑已实现 |
| **P2** | 1007 | 击退减速 | 击退后（联动） | ⭐⭐⭐ | FA+DA+GC | 逻辑已实现，GC待补 |
| **P2** | 1003 | 速度叠加 | 命中后 | ⭐⭐ | FA+DA | 逻辑已实现 |
| **P3** | 1012 | 幽风低语 | 速度加成时 | ⭐ | FA+DA | 逻辑已实现 |
| **P3** | 1015 | 痛苦契约 | 血量变化 | ⭐ | FA+DA | 逻辑已实现 |
| **停用** | 1001 | 攻击强化 | 被动 | ☆ | FA+DA | 测试阶段禁用 |
| **停用** | 1006 | 额外伤害 | 命中 | ☆ | FA+DA | 测试阶段禁用 |

> **感知等级说明：** ⭐⭐⭐⭐⭐ = 激活后 1 秒内所有玩家都能察觉；⭐⭐ = 需要关注数字才能感知；☆ = 几乎不可见

---

## 二、停用符文（测试阶段）

> 以下符文效果全靠数字变化，测试阶段优先级最低，暂不放入任何符文池。

### 1001 攻击强化 — 停用

**原因：** +10 攻击力纯数字，玩家无法感知。  
**代替方案：** 用 1017 击杀爆炸或 1019 燃烧印记替代同格位。

### 1006 额外伤害 — 停用

**原因：** +1 固定真实伤害，效果极小，感知为零。  
**代替方案：** 完全移出测试符文池。

---

## 三、P0 — 引导核心符文（下次测试前必须完成）

---

### 1004 击退

> **用途：** 引导三选一备选，高感知基础型符文。所有联动符文（1007/1013/1021）的依赖前提。

#### 逻辑层（已实现，简述）

- FA：`On Damage Dealt` → `Send Gameplay Event(Action.Knockback, Target=LastHitTarget)`
- GA_Knockback（C++）：接收事件 → `RootMotionMoveToForce`（500cm，0.3s）→ 结束时广播 `Event.Rune.KnockbackApplied`
- DA：RuneID=1004，Shape=1×1

#### 表现层规格

```
触发点：On Damage Dealt → GA_Knockback 激活瞬间

[命中接触点 GameplayCue]
  Tag          = GameplayCue.Melee.HitImpact（复用现有打击感 GC 即可）
  位置         = 命中位置

[目标位移过程 GameplayCue]
  Tag          = GameplayCue.Rune.Knockback
  位置         = 目标 Actor（Attached）
  Niagara      : 目标身上扬起白色/灰色残影拖尾（速度线方向，持续 0.3s）
  目标材质闪白 : Knockback 开始时 1 帧白色材质 Flash（可通过 GC 驱动 MI 参数）
  SFX          : "钝击+位移"组合音效（低频冲击声），播放于命中瞬间

[落点 GameplayCue（可选，P2 再补）]
  Tag          = GameplayCue.Rune.KnockbackLand
  位置         = 目标落点
  Niagara      : 尘埃小爆发（圆形扩散，0.2s）
  SFX          : 落地撞击声（短）

UI浮字    : 无（纯位移）
角色反馈  : 无
```

**当前遗漏：** GA_Knockback 结束时已广播 `Event.Rune.KnockbackApplied`，确认此事件在 C++ 代码中已发送（1007/1021 依赖此事件）。

---

### 1017 击杀爆炸

> **用途：** 引导三选一 A 槽。制造"啊哈时刻"，击杀一敌引发连锁爆炸，感知最强。

#### 逻辑层

FA：`FA_Rune_KillExplosion`

```
[On Damage Dealt]
  ↓
[Has Tag(Action.Dead, Target=LastHitTarget)]
  ↓ Yes
[Spawn Gameplay Cue at Location]
  CueTag   = GameplayCue.Rune.KillExplosion
  Location = LastHitTarget.Location
  ↓
[Apply GE to Targets in Radius]
  GE      = GE_RuneKillExplosionDamage （Instant，Damage=30，发送 Action.Knockback 事件）
  Radius  = 300.0
  Center  = LastHitTarget.Location
  Filter  = Enemy Only
```

**需要创建的资产：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_KillExplosion` | 上述 Flow Graph |
| `GE_RuneKillExplosionDamage` | Instant GE，HP Additive=-30；末尾向半径内所有敌人 Send Gameplay Event(Action.Knockback) |
| `GameplayCue.Rune.KillExplosion` | 爆炸效果 GC（见表现层） |
| `DA_Rune_KillExplosion` | RuneID=1017，Shape=1×1 |

#### 表现层规格

```
触发点：击杀确认后（Has Tag(Action.Dead)=true），立即在死亡位置触发

[爆炸核心 GameplayCue]
  Tag          = GameplayCue.Rune.KillExplosion
  位置         = 死亡位置（One-Shot，单次）
  
  Niagara 主体：
    - 中心向外扩散的橙红色爆炸圆环（直径约 600cm，0.3s 扩张完毕）
    - 中心上升热浪柱（橙黄渐变，1.0s 衰减）
    - 向外喷射的细碎火花粒子（约 30 粒，各自方向随机，0.5~0.8s 衰减）
    - 地面焦黑圆印（Decal，约 80cm 直径，持续 3s 后 Fade Out）
  
  参考外观："Roguelite 小型爆炸"，比手雷爆炸小一档，比命中打击感大三档
  
SFX：
  - 爆炸声（中等体积，有低频冲击感）
  - 余震尾音（0.5s 后渐灭）

范围内敌人反馈：
  - 被波及的敌人触发正常 GA_Knockback 白色材质 Flash
  - 被波及的敌人同时播放 GA_HitReaction 动画

UI浮字：
  - 爆炸对每个被波及敌人产生一个伤害浮字（复用现有伤害浮字系统，颜色：橙色）
```

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1017` |
| RuneConfig.RuneName | `击杀爆炸` |
| RuneConfig.RuneDescription | `击杀敌人时，在其位置引发一次爆炸，波及附近敌人。` |
| RuneConfig.RuneType | `Buff` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_KillExplosion` |

**已知限制：** `Has Tag(Action.Dead)` 依赖 GA_Dead 授予时序；若出现漏触发，改用 `LastDamageOutput.bKillingBlow` 字段（需在 FDamageBreakdown 中添加）。

---

### 1018 生命汲取

> **用途：** 引导三选一 B 槽。"攻击即治疗"正向循环，绿色回血数字是最直接的认知锚点。

#### 逻辑层

FA：`FA_Rune_LifeSteal`

```
[On Damage Dealt]
  ↓
[Apply Attribute Modifier]
  Attribute    = BaseAttributeSet.HP
  ModOp        = Additive
  Value        = LastDamageOutput.FinalDamage × 0.15  （数据引脚连线）
  DurationType = Instant
  Target       = BuffOwner
  ↓
[Spawn Gameplay Cue on Actor]
  CueTag = GameplayCue.Rune.LifeSteal
  Target = BuffOwner
```

**需要创建的资产：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_LifeSteal` | 上述 Flow Graph |
| `GameplayCue.Rune.LifeSteal` | 回血效果 GC（见表现层） |
| `DA_Rune_LifeSteal` | RuneID=1018，Shape=1×1 |

#### 表现层规格

```
触发点：每次 Apply Attribute Modifier(HP 回复) 执行后立即触发 GC

[回血 GameplayCue]
  Tag          = GameplayCue.Rune.LifeSteal
  位置         = 玩家角色（Attached，One-Shot）

  Niagara 主体：
    - 玩家周身向上浮动的绿色光点（约 8~12 粒，从腰部到头顶，0.6s 浮起消散）
    - 玩家短暂绿色轮廓边缘光（Fresnel，0.1s 亮起，0.4s 衰减）

SFX：
  - 轻柔的"吸收/治愈"音效（高频，短，非侵入性，不压制战斗音效）

UI 浮字（核心感知点）：
  - 在玩家头顶位置产生绿色回血浮字（"+XX"）
  - 字体与伤害浮字相同大小，颜色：鲜绿（#00FF88 或类似色）
  - 浮动方向：向上漂移，1.0s 消散
  - 实现方式：复用现有伤害浮字系统，传入负伤害量（或单独回血类型）

血条反馈：
  - HP 条绿色增量闪光（HP 增加时血条有 0.2s 亮绿色高亮区间）
```

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1018` |
| RuneConfig.RuneName | `生命汲取` |
| RuneConfig.RuneDescription | `每次命中敌人，恢复造成伤害 15% 的生命值。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_LifeSteal` |

**已知限制：** 需要 `LastDamageOutput.FinalDamage` 可在 FA 中作为数值引脚读取（依赖 COMBAT-003 的 FDamageBreakdown 委托暴露到 BF 节点）。单次回血建议钳制：最多 MaxHP × 5%，防止高倍率秒杀场景爆炸回满。

---

### 1019 燃烧印记

> **用途：** 引导三选一 C 槽。持续火焰粒子是强烈的持久感知，适合帮玩家建立"命中→附加效果"的认知。

#### 逻辑层

FA：`FA_Rune_BurnMark`

```
[On Damage Dealt]
  ↓
[Send Gameplay Event]
  EventTag = Buff.Event.Burn
  Target   = LastHitTarget
  Payload  = 5.0           （每跳伤害量）
```

`GA_Burn`（C++，复用 GA_Bleed 结构）：
- 监听 `Buff.Event.Burn` 事件
- OwnedTagPresent = `Buff.Status.Burning`
- 每 0.5s 造成 5 点伤害
- 持续 3s（重复触发刷新计时）
- 激活时 Spawn GameplayCue.Rune.Burn（Attached Loop），退出时 Remove GC

**需要创建的资产：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_BurnMark` | 上述 Flow Graph |
| `C++ GA_Burn` | 复制 GA_Bleed，改 Tag 为 `Buff.Status.Burning`，Interval=0.5，GC=`GameplayCue.Rune.Burn` |
| `GameplayCue.Rune.Burn` | 持续火焰 GC（见表现层）|
| `DA_Rune_BurnMark` | RuneID=1019，Shape=1×1 |

**新增 Tag：** `Buff.Status.Burning`（BuffTag.ini）、`Buff.Event.Burn`（BuffTag.ini）

#### 表现层规格

```
触发点①：命中施加燃烧时（GA_Burn 激活）
触发点②：每次燃烧 Tick（0.5s 一次）
触发点③：燃烧结束（3s 到期或被消除）

[燃烧持续 GameplayCue — Looping]
  Tag          = GameplayCue.Rune.Burn
  位置         = 目标 Actor（Attached，LoopDuration = Buff 持续时间）
  
  Niagara 主体（Loop）：
    - 从目标腰部向上的橙红火焰柱（宽度约 20cm，高约 60cm）
    - 上升火苗粒子（约 15~20 粒/s，颜色橙→黄→透明渐变）
    - 间歇性向外弹射的小火星（每 0.3s 约 3 粒，扩散范围 30cm）
    - 目标身上的橙红色自发光叠加（EmissiveColor += 橙色，中等强度）
  
  死亡不熄灭：燃烧 GC 随目标 ASC 生命周期，GA_Burn 退出时才停止

[燃烧 Tick 伤害浮字]
  颜色 = 橙色（与普通攻击白/黄色区分）
  大小 = 比普通伤害浮字小 20%（表示 DoT 性质）
  位置 = 目标头顶，每 Tick 随机小偏移（避免堆叠）

SFX：
  - 施加时：轻微"点燃"音效（短促，嗖声）
  - Loop：持续低音量火焰燃烧声（3D 空间声，距离衰减）
  - 每 Tick：可省略（避免音效密度过高）
  - 结束时：短促熄灭声（可选）

角色施加反馈：
  - 无（玩家自身无变化，避免混淆"谁在燃烧"）
```

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1019` |
| RuneConfig.RuneName | `燃烧印记` |
| RuneConfig.RuneDescription | `命中敌人后附加燃烧，每 0.5 秒造成 5 点伤害，持续 3 秒。重复命中刷新计时。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_BurnMark` |

---

## 四、P1 — 高感知测试池

---

### 1008 刀光波

> **触发：** 命中 3 次后向前方发射穿透型投射物。视觉最清晰的进攻型符文。

#### 逻辑层（已实现，简述）

- FA：`On Damage Dealt` → `Send Gameplay Event(Action.Rune.SlashWaveHit, Target=BuffOwner)`
- `GA_SlashWaveCounter`（C++）计数，每 3 次生成 `BP_SlashWaveProjectile`（速度 1400，穿透，寿命 1.2s）
- 投射物命中：应用 `GE_SlashWaveDamage`（Instant，Damage=30）
- DA：RuneID=1008，Shape=1×1

#### 表现层规格

```
触发点①：每次命中计数增加（可选轻反馈）
触发点②：第 3 次命中——发射刀光波
触发点③：刀光波命中敌人
触发点④：刀光波飞行到寿命结束消散

[命中计数轻反馈（可选，P2 再做）]
  角色攻击音效末尾可叠加轻微"蓄力"高频音，每次计数递增

[刀光波发射 GameplayCue]
  Tag          = GameplayCue.Rune.SlashWaveFire（在 GA_SlashWaveCounter 中触发）
  位置         = 玩家前方 80cm 处（SpawnOffset 位置）
  
  Niagara 主体（跟随 Projectile）：
    - 细长水平弧光（白色→浅蓝，长约 80cm，宽 5cm）
    - 弧光周围细碎刀气拖尾（速度线粒子，随投射物移动方向）
    - 穿透目标时：短暂分裂闪光（1 帧白色扩散圆）

SFX：
  - 发射时：高速切割的"嗖"声（有穿透感）
  - 命中敌人：清脆打击声（比普通攻击音更高频）
  - 消散时：轻微消散音（可省略）

[投射物命中 GameplayCue]
  Tag          = GameplayCue.Rune.SlashWaveHit
  位置         = 命中点
  Niagara      : 小型白色冲击圆（直径约 60cm，0.15s 消散）
  浮字         : 正常伤害浮字（白色，与普通攻击一致，颜色可改为淡蓝加以区分）

投射物 Blueprint（BP_SlashWaveProjectile）事件：
  BP_OnHitEnemy → 触发 GameplayCue.Rune.SlashWaveHit + 播放命中音效
  BP_OnExpired  → 短暂消散粒子（可省略，投射物会自然消失）
```

---

### 1013 震爆

> **触发：** 暴击时触发击退，视觉与 1004 相同但触发条件更稀有，形成"暴击爽感加成"。

#### 逻辑层（已实现，简述）

- FA：`On Crit Hit` → `Send Gameplay Event(Action.Knockback, Target=LastDamageTarget)`
- 复用 `GA_Knockback`（已授予所有角色）
- DA：RuneID=1013，Shape=1×1

#### 表现层规格

```
触发点：暴击命中瞬间（On Crit Hit 触发）

[暴击触发击退附加 GameplayCue]
  在正常暴击打击特效基础上叠加：
  Tag          = GameplayCue.Rune.Shockwave
  位置         = 命中点（One-Shot）
  
  Niagara 主体：
    - 比普通命中更大的冲击环（白色，直径约 120cm vs 普通 60cm）
    - 额外向外辐射的 4~6 条锐利光线（"暴击星芒"，0.1s 出现后快速消散）
    - 可复用 1004 击退 GC（目标拖尾、落点尘埃）

  注意：不需要额外新建 Niagara，以上靠参数复用即可

SFX：
  - 暴击音效已有；在此基础上叠加更重的"震爆"低频音（可用 Sound Mix 叠加）

浮字：
  - 暴击伤害浮字通常已有特殊颜色/字体；无需额外修改
```

---

### 1020 冲天一击

> **触发：** 每次命中均将敌人垂直弹飞，视觉冲击力最强，适合引导外的正式测试阶段。

#### 逻辑层

FA：`FA_Rune_Uppercut`

```
[On Damage Dealt]
  ↓
[Send Gameplay Event]
  EventTag = Action.Uppercut
  Target   = LastHitTarget
  Payload  = 800.0          （上弹冲量）
```

`GA_Uppercut`（C++，复用 GA_Knockback 结构）：
- Launch 方向 = `FVector::UpVector`，冲量 800
- 激活时附加 `Buff.Status.Airborne` Tag，持续 2s
- 2s 后 Tag 到期，重力自然落下

**需要创建的资产：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_Uppercut` | 上述 Flow Graph |
| `C++ GA_Uppercut` | 复用 GA_Knockback，Launch 方向=UpVector，冲量=800 |
| `GameplayCue.Rune.Uppercut` | 击飞 GC（见表现层） |
| `DA_Rune_Uppercut` | RuneID=1020，Shape=1×1 |

**新增 Tag：** `Buff.Status.Airborne`（BuffTag.ini）、`Action.Uppercut`（PlayerGameplayTag.ini）

#### 表现层规格

```
触发点①：命中时激活 GA_Uppercut（上弹瞬间）
触发点②：敌人在空中（Airborne 期间，可选持续 VFX）
触发点③：敌人落地（Airborne Tag 到期后）

[上弹命中 GameplayCue]
  Tag          = GameplayCue.Rune.Uppercut
  位置         = 命中点（One-Shot）
  
  Niagara 主体：
    - 垂直向上的白色冲击光柱（高约 100cm，粗 20cm，0.2s 亮起后向上消散）
    - 向下辐射的小型冲击环（直径约 80cm，0.15s 消散）
    - 目标材质白色 Flash（1 帧，比普通更强烈）

[空中持续效果（可选，P2 补充）]
  目标在 Airborne 期间：轻微白色自发光（表示"受控制"状态）
  可以不做，落地效果更重要

[落地 GameplayCue]
  Tag          = GameplayCue.Rune.UppecrutLand
  位置         = 目标落点（Airborne Tag 到期时触发）
  
  Niagara 主体：
    - 落点地面尘埃爆发（圆形，直径约 100cm，0.4s）
    - 落点冲击圈（白色，0.15s 扩散消散）

SFX：
  - 上弹命中：有力的"弹射"音效（低频+高频混合，短促有力）
  - 空中飞行：目标飞行声（风声，3D 跟随目标，可省略）
  - 落地：沉重撞地声（低频短促）

俯视角相机问题处理：
  - 敌人被弹飞期间可能短暂离开镜头；建议 GA_Uppercut 限制最大弹飞高度（MaxHeight = 200cm）
  - 或保持高度但同时做水平小位移（45° 斜向击飞代替纯垂直，视觉更清晰）
```

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1020` |
| RuneConfig.RuneName | `冲天一击` |
| RuneConfig.RuneDescription | `命中敌人时将其弹向空中，短暂失控，随重力落下。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_Uppercut` |

---

### 1021 余震

> **触发：** 击退完成后在落点产生减速震荡圈，与 1004/1020 形成天然协同感知。

#### 逻辑层

FA：`FA_Rune_Aftershock`

```
[Wait Gameplay Event]
  EventTag = Event.Rune.KnockbackApplied
  ↓ OnEventReceived

[Spawn Gameplay Cue at Location]
  CueTag   = GameplayCue.Rune.Aftershock
  Location = Event.Payload.Location

[Apply GE to Targets in Radius]
  GE      = GE_AfterShockSlow （HasDuration 2s，MoveSpeed -200，Magnitude 20 伤害）
  Radius  = 250.0
  Center  = Event.Payload.Location
  Filter  = Enemy Only
```

**需要创建的资产：**

| 资产 | 说明 |
|------|------|
| `FA_Rune_Aftershock` | 上述 Flow Graph |
| `GE_AfterShockSlow` | HasDuration 2s，MoveSpeed Additive -200，额外 Instant Damage 20 |
| `GameplayCue.Rune.Aftershock` | 震荡圈 GC（见表现层） |
| `DA_Rune_Aftershock` | RuneID=1021，Shape=1×1 |

#### 表现层规格

```
触发点：Event.Rune.KnockbackApplied 事件接收时（即击退结束、目标停止时）

[震荡圈 GameplayCue]
  Tag          = GameplayCue.Rune.Aftershock
  位置         = 落点（One-Shot，持续约 0.5s）
  
  Niagara 主体：
    - 从落点向外扩散的地面震荡圆环（白色/浅蓝，直径最大 500cm，0.5s 扩散完毕）
    - 圆环有 2~3 条同心圆跟随（表现波纹感）
    - 环内地面短暂扬起尘埃颗粒（细碎，约 30 粒，0.3s 消散）
    - 圈内被命中敌人：减速状态标识（蓝色/紫色轮廓光，持续整个减速时间）

SFX：
  - 落点震动声（低频震颤 + 短暂轰鸣，类似远处爆炸余波）
  - 减速音效（可省略，用视觉标识即可）

被减速敌人持续反馈：
  - 目标移动时有拖慢的残影效果（Motion Blur 或半透明拖影，持续 2s）
  - 减速期间目标轮廓颜色：蓝色（与 1019 橙色燃烧区分）

浮字：
  - 触发时对范围内每个被命中敌人产生伤害浮字（20 点，淡蓝色）
```

**DA 配置：**

| 字段 | 值 |
|------|-----|
| RuneConfig.RuneID | `1021` |
| RuneConfig.RuneName | `余震` |
| RuneConfig.RuneDescription | `击退敌人后，其落点产生震荡，波及附近敌人并使其减速。` |
| Shape.Cells | `(0,0)` |
| Flow.FlowAsset | `FA_Rune_Aftershock` |

**已知限制：** 依赖 `GA_Knockback` 末尾广播 `Event.Rune.KnockbackApplied`，需确认 C++ 代码中已实现此广播。

---

### 1005 流血

> **触发：** 命中施加流血状态，持续 DoT。感知中等，但视觉上粒子可以强化。

#### 逻辑层（已实现，简述）

- FA：`On Damage Dealt` → `Apply Attribute Modifier(HP, Duration=10, GrantedAbilities=GA_Bleed)` → `Send Gameplay Event(Buff.Event.Bleed, Magnitude=10)`
- `GA_Bleed`（C++）：每 0.5s 扣 5 血，流血期间屏蔽 HitReact 动画
- DA：RuneID=1005，Shape=1×1

#### 表现层规格

```
触发点①：流血施加成功（GA_Bleed 激活）
触发点②：每次流血 Tick（每 0.5s）
触发点③：流血结束

[流血持续 GameplayCue — Looping]
  Tag          = GameplayCue.Rune.Bleed（或 GameplayCue.Status.Bleeding）
  位置         = 目标 Actor（Attached，LoopDuration 与 Bleeding Tag 同步）
  
  Niagara 主体（Loop）：
    - 从目标躯干向下飘落的红色液滴粒子（约 5~8 粒/s，随重力下落，落地消散）
    - 目标身上有极低强度的红色自发光叠加（刚施加时 0.3s 内快速亮起，之后保持低强度）

SFX：
  - 施加时：撕裂声（短，1 次）
  - Loop：低音量血液滴落声（可每隔 1s 随机触发 1 次，非连续）
  - 结束时：无特殊音效

[每 Tick 伤害浮字]
  颜色 = 深红色（与普通攻击浅色区分，表示 DoT）
  大小 = 比普通伤害浮字小 30%（表示 DoT 性质）
  注意：流血屏蔽了 HitReact，所以浮字是玩家唯一的感知方式，必须有

目标血条：
  流血期间目标血条可以有小幅红色颤动（可选，P2 补充）
```

---

### 1002 热度提升

> **触发：** 被动，每秒加 1 热度，受伤后暂停 5 秒。感知依赖热度 UI，本身表现层极简。

#### 逻辑层（已实现）

- FA：`On Periodic(1s)` → `Has Tag(HeatInhibit)? → No` → `Apply Attribute Modifier(Heat, +1, Instant)`
- `On Damage Received` → `Grant Tag(HeatInhibit, Duration=5)`
- DA：RuneID=1002，Shape=1×1

#### 表现层规格

```
无战斗 GameplayCue（被动型，无合适的单次触发点）

热度积累的视觉反馈依赖：
  - 热度 UI 条每秒增长（已有）
  - 角色武器热度发光（已有，WeaponInstance 系统）
  - 角色激活区颜色变化（背包 UI）

可选轻反馈（P2 补充）：
  - 角色周身每秒轻微热浪粒子脉冲（极低强度，只在装备此符文且符文激活时）
  - 受伤后热度暂停期间：热度条有"冻结"闪光提示

SFX：
  - 无（避免每秒都有音效带来的干扰）
```

---

## 五、P2 — 条件触发扩展池

---

### 1010 突刺连击

> **触发：** 冲刺后 2 秒内下一次攻击造成双倍伤害。

#### 逻辑层（已实现）

- FA：`On Dash` → `Grant Tag(Buff.Status.DuoAssaultReady, Duration=2)`
- `On Damage Dealt` → `Has Tag(DuoAssaultReady)? → Remove Tag → Do Damage(×1.0 额外)`
- DA：RuneID=1010，Shape=1×1

#### 表现层规格

```
触发点①：冲刺后（DuoAssaultReady 状态激活，2s 窗口开启）
触发点②：双倍伤害触发（窗口内攻击命中）

[就绪状态 VFX（轻量）]
  角色武器或手部有短暂的琥珀色轮廓光（0.3s 内亮起，在 Ready 状态持续期间保持低强度）
  不做 GameplayCue，只通过 Tag 驱动角色蓝图内的材质参数

[触发双倍伤害 GameplayCue]
  Tag          = GameplayCue.Rune.DuoAssault
  位置         = 命中点（One-Shot）
  
  Niagara：
    - 比普通命中更强的冲击粒子（双层冲击圆，内小外大，表现"两段伤害"）
    - 琥珀色高光扩散（0.15s）
  
  浮字：
    - 双倍伤害产生两个浮字（原始伤害 + 额外伤害），或一个更大字号的合并浮字
    - 颜色：琥珀/金色（与普通白色区分）
  
SFX：
  - 双倍命中音效：比普通攻击音效更重（低频+中频叠加）
```

---

### 1009 弱点窥破

> **触发：** 连招末击（Combo4）命中时额外造成 50% 伤害（等效强制暴击）。

#### 逻辑层（已实现）

- FA：`On Damage Dealt` → `Has Tag(LightAtk.Combo4)` → 递归守卫 → `Do Damage(×0.5)`
- DA：RuneID=1009，Shape=1×1

#### 表现层规格

```
触发点：末击（Combo4）命中且额外伤害触发时

[末击强化 GameplayCue]
  Tag          = GameplayCue.Rune.WeaknessUnveiled
  位置         = 命中点（One-Shot）
  
  Niagara：
    - 青色（teal）裂纹扩散效果（从命中点向外放射，类似玻璃碎裂，0.25s）
    - 可选：目标身上有短暂"暴露弱点"高光效果（红色轮廓，0.5s）
  
  SFX：
    - 末击特殊音效（比前三击更有力，有尾音余响，可用音频 Pitch 调高表示"收尾感"）
  
  浮字：
    - 额外伤害浮字（青色，大小与普通伤害相同）
    - 可选：与原始伤害合并为一个更大浮字（两次伤害时间差极短）
```

---

### 1016 致命先机

> **触发：** 命中满血敌人时追加等量伤害（合计双倍）。

#### 逻辑层（已实现）

- FA：`On Damage Dealt` → 判断目标命中前是否满血 → 递归守卫 → `Do Damage(×1.0)`
- DA：RuneID=1016，Shape=1×1

#### 表现层规格

```
触发点：额外 Do Damage 触发时

[满血追加 GameplayCue]
  Tag          = GameplayCue.Rune.DeadlyStrike
  位置         = 命中点（One-Shot）
  
  Niagara：
    - 金色光芒爆发（比普通命中更亮，持续 0.2s）
    - "首击破防"感：目标身上短暂金色裂纹（0.15s）
  
  SFX：
    - 特殊"击破满血"音效（金属破裂质感）
  
  浮字：
    - 额外伤害为金色浮字
    - 可选在浮字前显示"⚡首击！"小图标（P2 补充）
```

---

### 1011 毒牙

> **触发：** 暴击时对目标施加持续 5 秒的中毒状态（每秒 -25 HP）。

#### 逻辑层（已实现，测试版）

- FA：`On Crit Hit` → `Apply Attribute Modifier(HP, -25, HasDuration=5, Period=1)` + `Grant Tag(Buff.Status.Poisoned, 5s)`
- DA：RuneID=1011，Shape=1×1

#### 表现层规格

```
触发点①：暴击命中，中毒施加（GA_Bleed 类似的流程）
触发点②：每次中毒 Tick（每 1s）
触发点③：中毒结束

[中毒持续 GameplayCue — Looping]
  Tag          = GameplayCue.Rune.VenomFang（或 GameplayCue.Status.Poisoned）
  位置         = 目标 Actor（Attached）
  
  Niagara（Loop）：
    - 从目标头部向下的绿色毒液粒子（约 5~8 粒/s，颜色：毒绿 #55FF00）
    - 目标表面有绿色污染感覆盖（EmissiveColor += 绿色低强度）
    - 可选：间歇性从目标位置冒出的绿色气泡（每 0.5s 约 3 粒）
  
  SFX：
    - 施加时：刺耳毒液声（短）
    - Loop：低音量腐蚀/沸腾声（约每 1.5s 随机一次）

[每 Tick 伤害浮字]
  颜色 = 毒绿色
  大小 = 比普通伤害小 25%（DoT 标识）
```

---

### 1014 暗影疾驰

> **触发：** 被动，MaxDashCharge +1，充能时间 ×1.25。纯属性修改，无战斗 GC。

#### 逻辑层（已实现）

- FA：激活时 `Apply Attribute Modifier(MaxDashCharge, +1, Infinite)` + `Apply Attribute Modifier(DashCooldownDuration, ×1.25, Infinite)`
- DA：RuneID=1014，Shape=1×1

#### 表现层规格

```
无战斗 GameplayCue（纯被动属性修改）

感知依赖：
  - HUD 冲刺格数 UI 显示 2 格（已有充能格）
  - 第二次冲刺产生的残影效果（视觉上明显有"储备了两次"的感知）

可选冲刺残影强化（装备此符文时）：
  - 每次冲刺产生的残影数量增加（从 1 条变 2 条，表现双倍冲刺储存感）
  - 残影颜色：暗紫色/深蓝色（区别于普通冲刺残影）
  - 通过检测玩家是否有 Airborne/DashCharge 属性 > 1 来切换材质参数（BP 实现）
```

---

### 1007 击退减速

> **触发：** 监听击退完成事件，对目标施加 1 秒减速（-300 MoveSpeed）。与 1004 联动。

#### 逻辑层（已实现）

- FA：`Wait Gameplay Event(Event.Rune.KnockbackApplied)` → `Apply Attribute Modifier(MoveSpeed, -300, HasDuration=1)`
- DA：RuneID=1007，Shape=1×1，**必须与 1004 同时装备**

#### 表现层规格

```
触发点：击退结束、目标停止位移后

[减速状态 VFX（轻量）]
  不新建单独 GC；与 1021 余震保持视觉一致：
    - 目标脚下淡蓝色小范围冷气粒子（1s 持续，约 30cm 范围）
    - 目标移动时有轻微减速残影感（Motion Blur，通过 GAS Tag 驱动 BP 参数）
  
SFX：
  - 减速施加时：短促"凝固/制动"音效（低沉快速）

浮字：
  - 无（减速不直接造成伤害）
```

---

### 1003 速度叠加

> **触发：** 命中后获得 MoveSpeed +30（可叠 5 层，每层持续 3s）。

#### 逻辑层（已实现）

- FA：`On Damage Dealt` → `Apply Attribute Modifier(MoveSpeed, +30, HasDuration=3, Stackable=5)`
- DA：RuneID=1003，Shape=1×1

#### 表现层规格

```
触发点：每次命中叠层

[速度叠层轻反馈]
  不单独触发 GC（叠层频率高，避免 VFX 过多）
  
  可选层数指示（P2 补充）：
    - 角色脚部有横向速度线粒子，层数越多粒子越密集
    - 或：角色脚部轮廓光随层数增加而增亮（1层=微亮，5层=明显白色边缘光）

  最大层数（×5）视觉：
    - 角色移动时明显更快（MoveSpeed +150 时体感明显）
    - 可以有一次性的"速度爆发"粒子（到达 5 层时，可选）

SFX：
  - 无（每次命中音效已有）
```

---

## 六、P3 — 复杂联动（暂缓）

---

### 1012 幽风低语

> **触发：** 每 0.5s 采样 MoveSpeed，有速度加成时同比提升暴击率（持续 5s）。依赖与 1003 联动。

#### 逻辑层（已实现）

- FA：`On Periodic(0.5s)` → `Get Attribute(MoveSpeed)` → 数学计算 → `Apply Attribute Modifier(Crit_Rate, bonus÷6, HasDuration=5, Unique)`
- DA：RuneID=1012，Shape=1×1

#### 表现层规格

```
此符文纯被动感知极低，表现层的目标是"让玩家察觉暴击率在变化"

[速度→暴击联动指示（P3 后考虑）]
  - HUD 暴击率显示随速度变化（若有暴击率 HUD）
  - 角色攻击时击中高亮颜色随 Crit_Rate 提升而更频繁出现暴击色
  - 无专属 GameplayCue（开销太高，与感知等级不匹配）
```

---

### 1015 痛苦契约

> **触发：** 每次 HP 变化时重新计算 AttackSpeed 加成（lostHP% × 0.25）。

#### 逻辑层（已实现）

- FA：`On Health Changed` → 计算 `(MaxHP-HP)/MaxHP × 0.25` → `Apply Attribute Modifier(AttackSpeed, scaledBonus, Infinite, BySource=Unique)`
- DA：RuneID=1015，Shape=1×1

#### 表现层规格

```
此符文感知依赖攻击速度（PlayRate）变化，数字感知为主

[血量越低越快的视觉强化（P3 考虑）]
  - 低血量时角色攻击动作明显更快（PlayRate 系统已有）
  - 可在 HP < 30% 时为角色添加轻微红色自发光边缘（表现"濒死暴走"感）
  - 无专属 GameplayCue

SFX：
  - HP 损失时攻击 PlayRate 提升 → 蒙太奇播放速度加快，自然带来"更急促"的音效节奏
```

---

## 七、表现层资产汇总

### 需要新建的 GameplayCue

| GC Tag | 优先级 | 类型 | 描述 |
|--------|--------|------|------|
| `GameplayCue.Rune.KillExplosion` | P0 | One-Shot | 橙红色爆炸圆环+火花+地面焦印 |
| `GameplayCue.Rune.LifeSteal` | P0 | One-Shot | 绿色光点上升+绿色边缘光 |
| `GameplayCue.Rune.Burn` | P0 | Looping | 橙红火焰柱附着于目标，含热浪 |
| `GameplayCue.Rune.Knockback` | P0 | One-Shot | 目标拖尾+白色材质 Flash |
| `GameplayCue.Rune.KnockbackLand` | P1 | One-Shot | 落点尘埃爆发（可选） |
| `GameplayCue.Rune.Shockwave` | P1 | One-Shot | 更大的暴击冲击环+锐利光线 |
| `GameplayCue.Rune.Uppercut` | P1 | One-Shot | 垂直白色冲击柱+落地尘埃 |
| `GameplayCue.Rune.UppecrutLand` | P1 | One-Shot | 落地震荡圈+尘埃 |
| `GameplayCue.Rune.Aftershock` | P1 | One-Shot | 地面震荡同心圆扩散 |
| `GameplayCue.Rune.Bleed` | P1 | Looping | 红色液滴飘落+低强度红色自发光 |
| `GameplayCue.Rune.SlashWaveFire` | P1 | One-Shot | 弧光+刀气拖尾 |
| `GameplayCue.Rune.SlashWaveHit` | P1 | One-Shot | 白色冲击圆（命中点） |
| `GameplayCue.Rune.DuoAssault` | P2 | One-Shot | 双层冲击圆+琥珀色高光 |
| `GameplayCue.Rune.WeaknessUnveiled` | P2 | One-Shot | 青色裂纹扩散 |
| `GameplayCue.Rune.DeadlyStrike` | P2 | One-Shot | 金色光芒爆发+金色裂纹 |
| `GameplayCue.Rune.VenomFang` | P2 | Looping | 绿色毒液粒子+绿色自发光 |

### 需要新建的 Gameplay Tag

| Tag | 文件 | 用途 | 优先级 |
|-----|------|------|--------|
| `Buff.Status.Burning` | BuffTag.ini | 燃烧状态守卫（1019）| P0 |
| `Buff.Event.Burn` | BuffTag.ini | GA_Burn 触发信号（1019）| P0 |
| `Buff.Status.Airborne` | BuffTag.ini | 空中状态守卫（1020）| P1 |
| `Action.Uppercut` | PlayerGameplayTag.ini | 上弹事件（1020）| P1 |
| `GameplayCue.Rune.*` | — | 各 GC 注册（全部）| 各级别 |

---

## 八、制作顺序建议

### 阶段一：本次引导测试前（P0）

1. 确认 `GA_Knockback` 末尾已广播 `Event.Rune.KnockbackApplied`（1021/1007 依赖）
2. 创建 `FA_Rune_Knockback` + `DA_Rune_Knockback` + `GameplayCue.Rune.Knockback`（1004）
3. 创建 `GE_RuneKillExplosionDamage` + `FA_Rune_KillExplosion` + `DA_Rune_KillExplosion` + `GameplayCue.Rune.KillExplosion`（1017）
4. 创建 `FA_Rune_LifeSteal` + `DA_Rune_LifeSteal` + `GameplayCue.Rune.LifeSteal`（1018）
5. 创建 `C++ GA_Burn` + `FA_Rune_BurnMark` + `DA_Rune_BurnMark` + `GameplayCue.Rune.Burn`（1019）
6. 配置 `DA_Campaign` 引导 FallbackLootPool = [1017, 1018, 1019]

### 阶段二：扩大测试符文池（P1）

7. 补全 1008 刀光波 GC（SlashWaveFire / SlashWaveHit）
8. 补全 1013 震爆 GC（Shockwave）
9. 创建 `C++ GA_Uppercut` + 1020 冲天一击全套
10. 创建 1021 余震全套（`GE_AfterShockSlow` + GC）
11. 补全 1005 流血 GC（Bleed Looping）

### 阶段三：丰富符文池（P2）

12. 补全 1010/1009/1016/1011 各自的 GC
13. 配置正式关卡 BuffPool 包含 P0+P1+P2 符文
14. 确认 1007 余震减速 GC

---

## 九、引导三选一推荐配置

> 引导关卡 `FallbackLootPool` 填入以下三个，每种触发类型各一：

| 槽位 | 符文 ID | 名称 | 感知类型 |
|------|---------|------|---------|
| 选项 A | 1017 | 击杀爆炸 | 击杀触发，AoE 视觉冲击最强 |
| 选项 B | 1018 | 生命汲取 | 命中触发，回血数字直接 |
| 选项 C | 1019 | 燃烧印记 | 命中触发，持续粒子与 DoT |

> 1020/1021 放入正式关卡符文池，不进引导（概念过多）。  
> 1004 击退可作为武器天然符文预置，无需进入三选一。

---

## 十、FlowGraph 节点类对照表

> 以下为本文档 FA 流程图中所有 `[方括号节点]` 对应的 C++ BFNode 类。  
> 头文件路径：`Source/DevKit/Public/BuffFlow/Nodes/`

### 触发器节点（Event）

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[On Damage Dealt]` | `UBFNode_OnDamageDealt` | 造成伤害时触发 |
| `[On Damage Received]` | `UBFNode_OnDamageReceived` | 受到伤害时触发 |
| `[On Crit Hit]` | `UBFNode_OnCritHit` | 暴击命中时触发 |
| `[On Kill]` | `UBFNode_OnKill` | 击杀时触发 |
| `[On Dash]` | `UBFNode_OnDash` | 冲刺时触发 |
| `[On Periodic]` | `UBFNode_OnPeriodic` | 定时触发（可配置间隔） |
| `[On Health Changed]` | `UBFNode_OnHealthChanged` | 生命值变化时触发 |
| `[On Buff Added]` | `UBFNode_OnBuffAdded` | Buff 添加时触发 |
| `[On Buff Removed]` | `UBFNode_OnBuffRemoved` | Buff 移除时触发 |
| `[Wait Gameplay Event]` | `UBFNode_WaitGameplayEvent` | 监听 ASC 上的 GameplayEvent |

### 效果节点（Effect）

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[Apply Attribute Modifier]` | `UBFNode_ApplyAttributeModifier` | 直接修改属性值 |
| `[Apply Gameplay Effect Class]` | `UBFNode_ApplyEffect` | 施加 GE 到目标 |
| `[Apply GE to Targets in Radius]` | `UBFNode_ApplyGEInRadius` | **新增** — 对半径内所有有效目标施加 GE |
| `[Do Damage]` | `UBFNode_DoDamage` | 直接造成伤害（语义化 ApplyEffect） |
| `[Area Damage]` | `UBFNode_AreaDamage` | 持续区域伤害（生成 DamageZone Actor） |
| `[Apply Execution]` | `UBFNode_ApplyExecution` | 施加 GameplayEffect Execution |

### Tag / Event 节点

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[Has Tag]` | `UBFNode_HasTag` | 条件判断：目标是否拥有指定 Tag |
| `[Grant Tag]` | `UBFNode_GrantTag` | 授予 Tag（可配置持续时间） |
| `[Add Tag]` | `UBFNode_AddTag` | 添加 Loose Tag |
| `[Remove Tag]` | `UBFNode_RemoveTag` | 移除 Tag |
| `[Send Gameplay Event]` | `UBFNode_SendGameplayEvent` | 向目标 ASC 发送 GameplayEvent |
| `[Grant GA]` | `UBFNode_GrantGA` | 授予 GameplayAbility |

### 视觉 / Cue 节点

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[Spawn Gameplay Cue on Actor]` | `UBFNode_SpawnGameplayCueOnActor` | **新增** — 在目标 Actor 上执行一次性 GC |
| `[Spawn Gameplay Cue at Location]` | `UBFNode_SpawnGameplayCueAtLocation` | **新增** — 在世界位置执行一次性 GC |
| `[Play Niagara]` | `UBFNode_PlayNiagara` | 在目标上播放 Niagara 特效 |
| `[Destroy Niagara]` | `UBFNode_DestroyNiagara` | 销毁已播放的 Niagara 特效 |
| `[Play Montage]` | `UBFNode_PlayMontage` | 播放动画蒙太奇 |
| `[Spawn Actor At Location]` | `UBFNode_SpawnActorAtLocation` | 在指定位置生成 Actor |

### 逻辑 / 工具节点

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[Compare Float]` | `UBFNode_CompareFloat` | 浮点数比较 |
| `[Compare Int]` | `UBFNode_CompareInt` | 整数比较 |
| `[Math Float]` | `UBFNode_MathFloat` | 浮点数学运算 |
| `[Math Int]` | `UBFNode_MathInt` | 整数数学运算 |
| `[Literal Float]` | `UBFNode_LiteralFloat` | 浮点常量 |
| `[Literal Int]` | `UBFNode_LiteralInt` | 整数常量 |
| `[Literal Bool]` | `UBFNode_LiteralBool` | 布尔常量 |
| `[Get Attribute]` | `UBFNode_GetAttribute` | 读取 ASC 属性值 |
| `[Get Rune Info]` | `UBFNode_GetRuneInfo` | 读取符文数据 |
| `[Check Target Type]` | `UBFNode_CheckTargetType` | 判断目标类型 |
| `[Delay]` | `UBFNode_Delay` | 延迟执行 |
| `[Finish Buff]` | `UBFNode_FinishBuff` | 结束 Buff Flow |

### 热度阶段节点

| FA 节点名 | BFNode 类 | 说明 |
|-----------|-----------|------|
| `[Increment Phase]` | `UBFNode_IncrementPhase` | 提升热度阶段 |
| `[Decrement Phase]` | `UBFNode_DecrementPhase` | 降低热度阶段 |
| `[On Phase Up Ready]` | `UBFNode_OnPhaseUpReady` | 阶段提升就绪时触发 |
| `[Phase Decay Timer]` | `UBFNode_PhaseDecayTimer` | 阶段衰退计时 |
| `[Sacrifice Decay]` | `UBFNode_SacrificeDecay` | 献祭衰退 |

---

## 十一、新节点系统升级方案（2026-04-24）

> Sprint 4.14b 新增：`ERuneTriggerType` 系统、`BFNode_ApplyEffect.OnRemoved` 引脚、`BFNode_GrantGA` Grant/Revoke 引脚。
> 以下 P0 符文的逻辑层方案因新系统而简化或优化，**请以本节方案为准，第三节原有方案仅供历史参考**。

---

### 1017 击杀爆炸 — 改用 TriggerType=OnKill

**变更说明：** 旧方案在 FA 内部用 `On Damage Dealt + Has Tag(Action.Dead)` 检测击杀，是权宜之计。新方案改 `TriggerType=OnKill`，BGC 在收到 `Ability.Event.Kill` 时直接启动 FA 实例，FA 内部无需任何触发节点。

**DA 新增字段：**

| 字段 | 值 |
|------|-----|
| `RuneConfig.TriggerType` | `OnKill` |

**更新后 FA 逻辑（`FA_Rune_KillExplosion`）：**

```
[Start]
  ↓
[Spawn Actor At Location]
  ActorClass = BP_KillExplosion（自处理 AoE 伤害 + GameplayCue.Rune.KillExplosion）
  Target     = BuffGiver（BGC 传入 KilledTarget，即死亡敌人的位置）
  ↓
[Finish Buff]
```

BGC 行为：`Ability.Event.Kill` 到达时，`BFC->StartBuffFlow(FA, NewGuid, Payload.Target)`，`Payload.Target = KilledTarget`。FA 启动时 `BuffGiver = KilledTarget`，Spawn Actor 在其位置生成即可。

**废弃节点：** `[On Damage Dealt]` + `[Has Tag(Action.Dead)]`——这两个节点是旧方案检测击杀的变通写法，新方案删除。

---

### 1018 生命汲取 — 维持 Passive（保留内部 On Damage Dealt）

**变更说明：** 无需改动，维持 `TriggerType=Passive`（默认值）。

**原因：** 生命汲取的回复量为"本次伤害的 15%"，必须通过 `On Damage Dealt` 节点的 `LastDamageAmount` 数据引脚做百分比计算。事件驱动型 FA（TriggerType≠Passive）启动时不携带伤害量，无法实现比例治疗，因此不适合切换。

**正确 FA 连线（不变）：**

```
[Start]
  ↓
[On Damage Dealt] ── LastDamageAmount（数据）──→ [Math Float: × 0.15] ── Result ──┐
  ↓ (执行流)                                                                      │
[Apply Attribute Modifier]  ←──────────────────────────────────────── Value ──────┘
  Attribute    = BaseAttributeSet.HP
  ModOp        = Additive
  DurationType = Instant
  Target       = BuffOwner
```

---

### 1019 燃烧印记 — 改用 GE 生命周期驱动 GA 模式

**变更说明：** 旧方案通过 `Send Gameplay Event(Buff.Event.Burn)` 启动 GA_Burn，GA_Burn 内部自行管理计时与刷新，触发逻辑分散在 FA 和 GA C++ 中。新方案改用 `BFNode_ApplyEffect.OnRemoved → BFNode_GrantGA.Revoke`，GA 的生命周期完全由 GE 驱动，FA 图中即可看到完整逻辑。

**更新后 FA 逻辑（`FA_Rune_BurnMark`）：**

```
[Start]
  ↓
[On Damage Dealt]
  ↓
[Apply Gameplay Effect Class: GE_BurnMark]
  StackMode = Unique（重复命中刷新 3s 计时）
  Target    = LastDamageTarget
  │
  ├── Out ──────────────────→ [Grant GA: GA_Burn].Grant
  │                              Target = LastDamageTarget
  └── OnRemoved（GE 到期）──→ [Grant GA: GA_Burn].Revoke
```

**GE_BurnMark 配置：**

| 字段 | 值 |
|------|-----|
| DurationType | `HasDuration` |
| Duration | `3.0` |
| StackingType | `AggregateBySource` |
| StackDurationRefreshPolicy | `RefreshOnSuccessfulApplication` |
| GrantedTags | `Buff.Status.Burning` |

**更新后 GA_Burn（简化）：**

| 项目 | 旧方案 | 新方案 |
|------|--------|--------|
| 触发方式 | 监听 `Buff.Event.Burn` GameplayEvent | FA 通过 `Grant GA.Grant` 直接授予 |
| 结束方式 | GA 内部 3s Timer 自行 EndAbility | FA 通过 `Grant GA.Revoke` 显式撤销 |
| 重复命中刷新 | GA 内部检测 `Buff.Status.Burning` Tag 刷新 Timer | GE_BurnMark Unique 刷新，GA 不感知 |
| 每 Tick 伤害 | 每 0.5s 施加伤害 GE | 不变 |
| ActivationBlockedTags | `Buff.Status.Burning`（防重复激活） | 保留（多目标命中保护） |

> **多目标说明：** 同一时间对多个敌人使用时，每个敌人的 GE 独立计时；`Grant GA.Grant` 节点含幂等保护，同一目标不会重复授予 GA_Burn（第一次 Grant 成功后，后续命中刷新 GE 但 GA 不重复授予）。对不同敌人的 GA 管理则靠各自 ASC 上的 `Buff.Status.Burning` 块标签隔离。

**废弃的资产：** `Buff.Event.Burn` Tag 可移除；GA_Burn 中的 `WaitGameplayEvent` 监听逻辑可删除。
