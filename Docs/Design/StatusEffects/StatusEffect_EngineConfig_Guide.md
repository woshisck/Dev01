# 状态效果系统 — 配置指南

> **架构**：10 个基础效果 FA（可复用） + SubGraph 组合 + AN_MeleeDamage 敌人复用  
> **原则**：所有效果逻辑在 FA 中可视化配置，玩家和敌人共享同一套基础效果 FA

---

## 目录

1. [架构总览](#1-架构总览)
2. [前置条件](#2-前置条件)
3. [基础效果 FA 创建清单](#3-基础效果-fa-创建清单)
4. [GA Blueprint 子类配置](#4-ga-blueprint-子类配置)
5. [GE 资产配置](#5-ge-资产配置)
6. [RuneDataAsset 包装](#6-runedataasset-包装)
7. [符文 FA 调用示例](#7-符文-fa-调用示例)
8. [敌人配置](#8-敌人配置)
9. [验证清单](#9-验证清单)

---

## 1. 架构总览

```
┌─────────────────────────────────────────────────┐
│               基础效果层（创建一次，永久复用）         │
│                                                   │
│  FA_Effect_Poison    FA_Effect_Burn    ...共10个    │
│  FA_Effect_Stun      FA_Effect_Curse              │
│  FA_Effect_Bleed     FA_Effect_Knockback          │
│  FA_Effect_Rend      FA_Effect_Fear               │
│  FA_Effect_Wound     FA_Effect_Freeze             │
└───────────┬──────────────────────┬────────────────┘
            │ SubGraph 调用         │ AN_MeleeDamage
            │                      │ AdditionalRuneEffects
┌───────────▼──────────┐  ┌───────▼────────────────┐
│  玩家符文 FA          │  │  敌人攻击动画           │
│                       │  │                        │
│  FA_Rune_暴击击退     │  │  AN_MeleeDamage         │
│  FA_Rune_中毒之刃     │  │    AdditionalRuneEffects│
│  FA_Rune_冲刺冻结     │  │    = [DA_Effect_Poison] │
└───────────────────────┘  └────────────────────────┘
```

### 节点职责

| 节点 | 职责 |
| ---- | ---- |
| CalcDamage | 纯计算：读目标属性 → 按公式算出数值 → 输出 FinalDamage 数据引脚 |
| ApplyAttributeModifier | GE 工厂：创建运行时 GE，管理 Duration/Period/GrantedTags/GrantedAbilities |
| SendGameplayEvent | 向目标发送事件，激活已授予的 GA（携带 Magnitude 参数） |
| SubGraph | 调用基础效果 FA，隔离实例，支持 Custom Input/Output |
| Probability | 概率门控（放在符文 FA 层，不放基础效果 FA 层） |

### 伤害管道

| 属性 | 路径 | 行为 |
| ---- | ---- | ---- |
| DamagePhysical | 物理 | 护甲吸收 → 溢出扣 HP → 可击杀 → 广播 Damaged |
| DamagePure | 真实 | 护甲吸收 → 溢出扣 HP → 可击杀 → 广播 Damaged |
| DamageBuff | 状态 | 绕过护甲 → 直扣 HP → 不可击杀（HP>=1） → 不广播 Damaged |

> 护甲/不可杀/事件广播全在 DamageAttributeSet::PostGameplayEffectExecute 统一处理。

---

## 2. 前置条件

### 2.1 Gameplay Tags 确认

打开 Project Settings → Gameplay Tags，确认以下 Tag 存在：

**状态标签**（Buff.Status.*）：
- Bleeding, Poisoned, Rended, Wounded, KnockbackDebuff
- Burning, Stunned, Feared, Frozen, Chilled, Cursed, Armored

**角色状态**（Character.State.*）：
- Stunned, Feared

**GA 触发事件**（Buff.Event.*）：
- Bleed, Rend, Wound, KnockbackDebuff, Fear, Freeze

**通用事件**：
- Ability.Event.Damaged, Ability.Event.Attack.Hit, Ability.Event.Kill

**SetByCaller 数据标签**（Data.Damage.*）：
- Rend, Wound, Knockback, Fear

### 2.2 AttributeSet

确认目标角色 ASC 的 Attribute Sets 列表包含 `DamageAttributeSet`。

### 2.3 ArmorHP

在 DT_BaseAttribute 数据表中为有护甲的角色填写 `MaxArmorHP`（0=无护甲）。

---

## 3. 基础效果 FA 创建清单

> 每个基础效果是一个独立 FlowAsset，**不含概率节点**（概率由调用方控制）。  
> 入口用 Custom Input "Apply"，出口用 Custom Output "Done"。  
> 建议资产路径：`Content/BuffFlow/Effects/`

### 3.1 FA_Effect_Poison（中毒）— 纯 FA

每 2 秒造成 7% MaxHP 状态伤害，持续 8 秒。

```
[Custom Input "Apply"]
    ↓
[CalcDamage
    BaseMode  = PercentMaxHP
    BaseValue = 0.07
    Target    = LastDamageTarget]
    ↓ Out                 ↓ FinalDamage（数据引脚）
[ApplyAttributeModifier ── Value = FinalDamage
    Attribute       = DamageAttributeSet.DamageBuff
    ModOp           = Additive
    DurationType    = HasDuration
    Duration        = 8.0
    Period          = 2.0
    FireImmediately = false
    GrantedTags     = Buff.Status.Poisoned
    StackMode       = Unique
    DurationRefresh = RefreshOnSuccessfulApplication
    Target          = LastDamageTarget]
    ↓ Out
[Custom Output "Done"]
```

### 3.2 FA_Effect_Burn（燃烧）— 纯 FA

每 1 秒造成 20 点状态伤害，目标有流血时 x1.15。

```
[Custom Input "Apply"]
    ↓
[CalcDamage
    BaseMode  = Flat
    BaseValue = 20.0
    Target    = LastDamageTarget
    MultTag1      = Buff.Status.Bleeding
    MultCondition1 = HasTag
    MultValue1     = 1.15]
    ↓ Out                 ↓ FinalDamage
[ApplyAttributeModifier ── Value = FinalDamage
    Attribute       = DamageAttributeSet.DamageBuff
    ModOp           = Additive
    DurationType    = HasDuration
    Duration        = 6.0
    Period          = 1.0
    FireImmediately = true
    GrantedTags     = Buff.Status.Burning
    StackMode       = Unique
    DurationRefresh = RefreshOnSuccessfulApplication
    Target          = LastDamageTarget]
    ↓ Out
[Custom Output "Done"]
```

### 3.3 FA_Effect_Stun（眩晕）— 纯 FA

纯标签效果，持续 2 秒。

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType = HasDuration
    Duration     = 2.0
    GrantedTags  = Buff.Status.Stunned, Character.State.Stunned
    StackMode    = Unique
    DurationRefresh = RefreshOnSuccessfulApplication
    Target       = LastDamageTarget]
    ↓ Out
[Custom Output "Done"]
```

> 不填 Attribute 即可——ApplyAttributeModifier 无 Attribute 时仅授予标签。  
> 需要在被阻断的 GA 的 Activation Blocked Tags 加入 `Character.State.Stunned`。

### 3.4 FA_Effect_Curse（诅咒）— 纯 FA

每层诅咒减少 50 MaxHP，最多 5 层，持续 10 秒。

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    Attribute        = BaseAttributeSet.MaxHealth
    ModOp            = Additive
    Value            = -50.0
    DurationType     = HasDuration
    Duration         = 10.0
    GrantedTags      = Buff.Status.Cursed
    StackMode        = Stackable
    MaxStacks        = 5
    DurationRefresh  = NeverRefresh
    ExpirationPolicy = RemoveSingleStackAndRefreshDuration
    Target           = LastDamageTarget]
    ↓ Out
[Custom Output "Done"]
```

> MaxHP 下降时 C++ 自动等比缩放 HP（PreAttributeChange），上升时保持绝对值。

### 3.5 FA_Effect_Bleed（流血）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 5.0
    GrantedTags      = Buff.Status.Bleeding
    GrantedAbilities = GA_Bleed
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.Bleed
    Magnitude  = 10.0
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

> GA_Bleed 内部每 0.5s 直接扣 Health（ApplyModToAttributeUnsafe），无需 GE 资产。  
> Magnitude = 每秒伤害量，GA 读取后按 TickInterval 分摊。

### 3.6 FA_Effect_Rend（撕裂）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 8.0
    GrantedTags      = Buff.Status.Rended
    GrantedAbilities = GA_Rend
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.Rend
    Magnitude  = 15.0
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

### 3.7 FA_Effect_Wound（伤口）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 6.0
    GrantedTags      = Buff.Status.Wounded
    GrantedAbilities = GA_Wound
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.Wound
    Magnitude  = 15.0
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

### 3.8 FA_Effect_Knockback（击退）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 5.0
    GrantedTags      = Buff.Status.KnockbackDebuff
    GrantedAbilities = GA_KnockbackDebuff
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.KnockbackDebuff
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

### 3.9 FA_Effect_Fear（恐惧）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 2.5
    GrantedTags      = Buff.Status.Feared, Character.State.Feared
    GrantedAbilities = GA_Fear
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.Fear
    Magnitude  = 30.0
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

> 仅对 AI 敌人有效。

### 3.10 FA_Effect_Freeze（冻结）— FA + GA

```
[Custom Input "Apply"]
    ↓
[ApplyAttributeModifier
    DurationType     = HasDuration
    Duration         = 4.0
    GrantedTags      = Buff.Status.Chilled
    GrantedAbilities = GA_Freeze
    StackMode        = Unique
    DurationRefresh  = RefreshOnSuccessfulApplication
    Target           = LastDamageTarget]
    ↓ Out
[SendGameplayEvent
    EventTag   = Buff.Event.Freeze
    Magnitude  = 30.0
    Target     = LastDamageTarget
    Instigator = DamageCauser]
    ↓ Out
[Custom Output "Done"]
```

---

## 4. GA Blueprint 子类配置

> 纯 FA 效果（Poison/Burn/Stun/Curse）不需要 GA。  
> GA_Bleed 不需要 Blueprint 子类（无 GE 引用，直接用 C++ 基类即可）。  
> 以下 5 个 GA 需要在编辑器里创建 Blueprint 子类并填写 GE 引用。

| Blueprint 名 | C++ 父类 | 需填写的 GE 引用 | 关键参数 |
| ---- | ---- | ---- | ---- |
| GA_Rend_BP | GA_Rend | RendDamageEffect = GE_RendDamage | DamagePerUnits=100, TickInterval=0.2, StationaryTimeout=2.0 |
| GA_Wound_BP | GA_Wound | WoundDamageEffect = GE_WoundDamage | DefaultExtraDamage=10 |
| GA_KnockbackDebuff_BP | GA_KnockbackDebuff | ArmorDamageEffect = GE_KnockbackArmorDamage | ArmorDamagePct=0.15 |
| GA_Fear_BP | GA_Fear | PenaltyDamageEffect = GE_FearPenalty | RequiredDistance=800, FearDuration=2.0 |
| GA_Freeze_BP | GA_Freeze | ChillEffect = GE_Chill, FrozenStunEffect = GE_FrozenStun | RequiredDistance=800, FreezeDuration=3.0 |

建议资产路径：`Content/AbilitySystem/Abilities/StatusEffects/`

> 这些 GA Blueprint 子类只需继承 C++ 父类并填写属性，不需要添加任何蓝图逻辑。

---

## 5. GE 资产配置

> 这些 GE 仅作为 GA 内部伤害通道，不在 FA 图中直接引用。

| GE 名 | Duration | Modifiers | 用途 |
| ---- | ---- | ---- | ---- |
| GE_RendDamage | Instant | DamageBuff Additive SetByCaller(Data.Damage.Rend) | 撕裂每次移动触发的伤害 |
| GE_WoundDamage | Instant | DamageBuff Additive SetByCaller(Data.Damage.Wound) | 伤口受击额外伤害 |
| GE_FearPenalty | Instant | DamageBuff Additive SetByCaller(Data.Damage.Fear) | 恐惧逃跑失败惩罚 |
| GE_KnockbackArmorDamage | Instant | ArmorHP Additive SetByCaller(Data.Damage.Knockback) | 击退额外护甲伤害 |
| GE_Chill | Infinite | MoveSpeed Multiply 0.5, GrantedTags=Buff.Status.Chilled | 冻结减速阶段 |
| GE_FrozenStun | HasDuration 1.5s | GrantedTags=Buff.Status.Frozen + Character.State.Stunned | 冻结眩晕阶段 |

建议资产路径：`Content/AbilitySystem/GameplayEffects/StatusEffects/`

---

## 6. RuneDataAsset 包装

> 每个基础效果 FA 需要一个 RuneDataAsset 包装，用于 AN_MeleeDamage.AdditionalRuneEffects。
> 符文 FA 通过 SubGraph 直接引用 FlowAsset，不需要 DA。

| DA 名 | FlowAsset | RuneConfig.RuneName | RuneConfig.TriggerType |
| ---- | ---- | ---- | ---- |
| DA_Effect_Poison | FA_Effect_Poison | 中毒 | Passive |
| DA_Effect_Burn | FA_Effect_Burn | 燃烧 | Passive |
| DA_Effect_Stun | FA_Effect_Stun | 眩晕 | Passive |
| DA_Effect_Curse | FA_Effect_Curse | 诅咒 | Passive |
| DA_Effect_Bleed | FA_Effect_Bleed | 流血 | Passive |
| DA_Effect_Rend | FA_Effect_Rend | 撕裂 | Passive |
| DA_Effect_Wound | FA_Effect_Wound | 伤口 | Passive |
| DA_Effect_Knockback | FA_Effect_Knockback | 击退 | Passive |
| DA_Effect_Fear | FA_Effect_Fear | 恐惧 | Passive |
| DA_Effect_Freeze | FA_Effect_Freeze | 冻结 | Passive |

> TriggerType 设为 Passive，因为触发时机由调用方（符文 FA 或 AN_MeleeDamage）控制。  
> Shape 字段可留空（这些 DA 不放入背包）。  
> 建议资产路径：`Content/Data/StatusEffects/`

---

## 7. 符文 FA 调用示例

### 7.1 暴击触发击退

```
──── FA_Rune_CritKnockback ──────────────────
│                                            │
│  [Start  TriggerType=OnCritHit]            │
│       ↓                                    │
│  [SubGraph  Asset=FA_Effect_Knockback]     │
│       ↓ Done                               │
│  [Finish]                                  │
│                                            │
──────────────────────────────────────────────
```

RuneDataAsset 配置：
- TriggerType = OnCritHit
- Flow.FlowAsset = FA_Rune_CritKnockback

### 7.2 攻击命中 25% 中毒

```
──── FA_Rune_PoisonBlade ────────────────────
│                                            │
│  [Start  TriggerType=OnAttackHit]          │
│       ↓                                    │
│  [Probability  Chance=0.25]                │
│       ↓ Pass                               │
│  [SubGraph  Asset=FA_Effect_Poison]        │
│       ↓ Done                               │
│  [Finish]                                  │
│                                            │
│       ↓ Fail（Probability）                 │
│  [Finish]                                  │
│                                            │
──────────────────────────────────────────────
```

### 7.3 冲刺后对路径敌人施加冻结

```
──── FA_Rune_DashFreeze ─────────────────────
│                                            │
│  [Start  TriggerType=OnDash]               │
│       ↓                                    │
│  [SubGraph  Asset=FA_Effect_Freeze]        │
│       ↓ Done                               │
│  [Finish]                                  │
│                                            │
──────────────────────────────────────────────
```

> LastDamageTarget 由冲刺 GA 在命中时写入。

### 7.4 击杀后同时施加多个效果

```
──── FA_Rune_KillCombo ──────────────────────
│                                            │
│  [Start  TriggerType=OnKill]               │
│       ↓                                    │
│  [SubGraph  Asset=FA_Effect_Burn]          │
│       ↓ Done                               │
│  [SubGraph  Asset=FA_Effect_Stun]          │
│       ↓ Done                               │
│  [Finish]                                  │
│                                            │
──────────────────────────────────────────────
```

### 7.5 组合模式总结

| 模式 | FA 写法 |
| ---- | ---- |
| 无条件触发 | `[Start] → [SubGraph Effect]` |
| 概率触发 | `[Start] → [Probability] → [SubGraph Effect]` |
| 多效果串联 | `[Start] → [SubGraph A] → [SubGraph B]` |
| 条件分支 | `[Start] → [HasTag X] → True:[SubGraph A] / False:[SubGraph B]` |

> **概率节点放在符文 FA 层**，不放基础效果 FA 层。  
> 基础效果 FA 被调用时 100% 执行，由调用方决定何时、以何概率调用。

---

## 8. 敌人配置

### 8.1 前置：敌人需要 BuffFlowComponent

在敌人蓝图（EnemyCharacterBase 或子类）中添加 `BuffFlowComponent`（如果尚未有的话）。

> ReceiveOnHitRune 在攻击者的 BFC 上启动 FA。  
> BuffOwner = 攻击者（敌人），LastDamageTarget = 被命中者（玩家）。  
> 敌人的 FA 和玩家的 FA 用同一套 Target 选择器，无需区分。

### 8.2 配置方式：AN_MeleeDamage

在敌人的攻击动画 Montage 上放置 AN_MeleeDamage Notify：

```
敌人攻击动画 → AN_MeleeDamage
    ├── EventTag = GameplayEffect.DamageType.GeneralAttack
    ├── ActDamage = 20（基础物理伤害）
    ├── ActRange = 300
    └── AdditionalRuneEffects = [DA_Effect_Poison]    ← 自带中毒
```

**示例配置表**：

| 敌人类型 | AdditionalRuneEffects | 效果 |
| ---- | ---- | ---- |
| 普通怪 | 空 | 纯物理伤害 |
| 毒蛇 | [DA_Effect_Poison] | 攻击附带中毒 |
| 火焰精英 | [DA_Effect_Burn] | 攻击附带燃烧 |
| 冰霜精英 | [DA_Effect_Freeze] | 攻击附带冻结 |
| Boss 重击 | [DA_Effect_Knockback, DA_Effect_Stun] | 同时击退 + 眩晕 |
| 诅咒法师 | [DA_Effect_Curse] | 攻击附带诅咒 |

> AN_MeleeDamage 填入的 DA 在命中时触发 ReceiveOnHitRune，FA 在敌人的 BFC 上运行，Target 指向被命中的玩家。
>
> 敌人不同招式可以分别配置不同的 AdditionalRuneEffects——同一个敌人的轻击无效果，重击附带眩晕。

### 8.3 玩家 vs 敌人对比

| 维度 | 玩家 | 敌人 |
| ---- | ---- | ---- |
| 触发来源 | 背包符文（TriggerType 决定时机） | AN_MeleeDamage.AdditionalRuneEffects |
| FA 运行位置 | 玩家 BFC | 敌人 BFC |
| Target 含义 | LastDamageTarget = 被攻击的敌人 | LastDamageTarget = 被攻击的玩家 |
| 基础效果 FA | 共用 | 共用 |
| 概率控制 | 符文 FA 层的 Probability 节点 | 可在 AN 层用不同 DA（含概率的 FA），或直接 100% 触发 |

> **核心结论**：基础效果 FA 不区分使用者身份。同一个 FA_Effect_Poison 既可以被玩家符文调用，也可以被敌人 AN_MeleeDamage 调用。

---

## 9. 验证清单

### 第一步：编译 + Tag 确认

- [ ] 编译通过（CalcDamage 已简化为纯计算节点）
- [ ] 打开 Project Settings → Gameplay Tags，确认所有 Tag 存在
- [ ] 敌人蓝图已添加 BuffFlowComponent

### 第二步：创建资产

- [ ] 10 个基础效果 FA 已创建（FA_Effect_*）
- [ ] 10 个 RuneDataAsset 已创建（DA_Effect_*）
- [ ] 5 个 GA Blueprint 子类已创建并填写 GE 引用
- [ ] 6 个 GE 资产已创建

### 第三步：功能验证

| 效果 | 测试方式 | 预期 |
| ---- | ---- | ---- |
| 中毒（纯 FA） | 创建 FA_Rune_Test → SubGraph(FA_Effect_Poison) | 每 2 秒 DamageBuff，HP>=1 |
| 燃烧（纯 FA） | 先施加流血再触发燃烧 | 每跳 = 20 x 1.15 = 23 |
| 眩晕（纯 FA） | 触发后检查 GA | Character.State.Stunned 持续 2 秒 |
| 诅咒（纯 FA） | 多次触发检查 MaxHP | 每层 -50，HP 等比缩放，最多 5 层 |
| 流血（FA+GA） | 触发后查看 HP 变化 | 每 0.5s 扣血，持续 5 秒 |
| 击退（FA+GA） | 触发后观察敌人位移 | 敌人被推开 + 护甲额外伤害 |
| 敌人中毒 | 毒蛇攻击玩家 | 玩家中毒，行为与玩家符文一致 |
| SubGraph 复用 | 两个不同符文都 SubGraph(FA_Effect_Poison) | 各自独立实例，互不干扰 |

### 第四步：组合验证

- [ ] 暴击触发击退：FA_Rune_CritKnockback 在暴击时触发
- [ ] 多效果串联：一个符文同时触发燃烧 + 流血
- [ ] 敌人多效果：Boss 重击同时触发击退 + 眩晕

---

## 附录：资产路径汇总

```
Content/
├── BuffFlow/
│   └── Effects/
│       ├── FA_Effect_Poison.uasset
│       ├── FA_Effect_Burn.uasset
│       ├── FA_Effect_Stun.uasset
│       ├── FA_Effect_Curse.uasset
│       ├── FA_Effect_Bleed.uasset
│       ├── FA_Effect_Rend.uasset
│       ├── FA_Effect_Wound.uasset
│       ├── FA_Effect_Knockback.uasset
│       ├── FA_Effect_Fear.uasset
│       └── FA_Effect_Freeze.uasset
├── Data/
│   └── StatusEffects/
│       ├── DA_Effect_Poison.uasset
│       ├── DA_Effect_Burn.uasset
│       └── ... (共 10 个)
├── AbilitySystem/
│   ├── Abilities/
│   │   └── StatusEffects/
│   │       ├── GA_Rend_BP.uasset
│   │       ├── GA_Wound_BP.uasset
│   │       ├── GA_KnockbackDebuff_BP.uasset
│   │       ├── GA_Fear_BP.uasset
│   │       └── GA_Freeze_BP.uasset
│   └── GameplayEffects/
│       └── StatusEffects/
│           ├── GE_RendDamage.uasset
│           ├── GE_WoundDamage.uasset
│           ├── GE_FearPenalty.uasset
│           ├── GE_KnockbackArmorDamage.uasset
│           ├── GE_Chill.uasset
│           └── GE_FrozenStun.uasset
```

---

## 附录：已废弃类

| 类 | 原用途 | 替代方案 |
| ---- | ---- | ---- |
| GEExec_PoisonDamage | 中毒每跳计算 | CalcDamage(PercentMaxHP) + ApplyAttributeModifier(Period) |
| GEExec_BurnDamage | 燃烧每跳计算 | CalcDamage(Flat+MultSlot) + ApplyAttributeModifier(Period) |
