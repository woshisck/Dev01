# BuffFlow 系统指南

> 更新日期：2026-04-05 (rev5)  
> 对象：策划 + 程序

---

## 一、系统概述

**BuffFlow = 可视化 Buff/Effect 逻辑系统。**

它解决了一个核心问题：符文的数值配置很简单（填表），但符文的"行为逻辑"很复杂（什么时候触发、触发条件、触发后做什么）。BuffFlow 把这部分逻辑从代码中抽出来，放进可视化的 Flow Graph 里，让策划能直接看到并修改符文的行为。

### 两层分离原则

```
DA（Data Asset）= 数据层        FA（Flow Asset）= 逻辑层
─────────────────────────────   ─────────────────────────────
· 符文展示信息（名称/图标）    · 决定 GE 何时施加（Start/事件触发）
· GE 行为规则（Duration/Stack） · 管理 GE 生命周期（ApplyRuneGE 节点）
· Effects[]（属性修改/Tag）      · 授予 GA（GrantGA 节点）
· 背包格子形状                  · 条件判断 / 特效 / 音效
─────────────────────────────   ─────────────────────────────
          策划填表配置                    策划用节点搭建
```

**GE 生命周期归 FA 管。**  
BackpackGrid 只负责启停 FA，不直接 apply / remove GE。  
FA 内的 `ApplyRuneGE` 节点负责在正确时机施加 GE，FA 停止时自动移除（含叠层）。

**GAS 数值流向：**
```
GE（施加）→ AttributeSet（存储）→ GA（读取并使用）
```

**三种 GE 施加时机（对应 ApplyRuneGE 的前置节点）：**
```
激活即生效（永久被动类）：
  [Start] → [ApplyRuneGE](DA_Rune_Knockback)

命中时触发/叠层：
  [OnDamageDealt] → [ApplyRuneGE](DA_Rune_Berserk)

条件触发：
  [OnKill] → [ApplyRuneGE](DA_Rune_Rage)
```

---

## 二、DA 结构（RuneDataAsset）

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description
  ├── Shape
  ├── ▼ Rune Config
  │     ├── Rune Type / Duration Type / Duration / Period
  │     ├── Unique Type / Stack Type / Max Stack / Stack Reduce Type
  │     ├── Rune ID / Rune Tag
  │     └── Effects[]
  └── ▼ Flow → Flow Asset
```

完整字段说明见 [RuneDataAsset_Guide.md](RuneDataAsset_Guide.md)。

---

## 三、节点参考

节点按类别分组。每个节点标注：**配置项 → 输出引脚 → 典型用法**。

---

### 📡 事件类节点（Flow 起点，监听游戏事件）

---

#### `On Damage Dealt`
触发时机：**本角色造成伤害时**  
上下文写入：`Target` = 受伤方，`DamageAmount` = 实际伤害量

输出：`OnDamage`（持续监听，每次伤害触发一次）

**典型用法：**
```
命中叠层：
  [OnDamageDealt] → [ApplyRuneGE](DA_Berserk)
  ← 每次命中调用一次，GAS 按 StackType 自动叠层

命中触发爆发：
  [OnDamageDealt] → [GetRuneInfo](Rune.Berserk)
                         └─ Found → [ApplyEffect](GE_BurstATK)
```

---

#### `On Damage Received`
触发时机：**本角色受到伤害时**  
上下文写入：`DamageCauser` = 攻击方，`DamageAmount` = 受到伤害量

输出：`OnDamage`

**典型用法：**
```
受击触发反甲：
  [OnDamageReceived] → [ApplyAttributeModifier]
                           Attribute = Attack, Value = 30, Duration = 5s

受击计数：
  [OnDamageReceived] → [ApplyRuneGE](DA_Endurance)   ← 每受击叠一层韧性
```

---

#### `On Crit Hit`
触发时机：**本角色打出暴击时**

输出：`OnCrit`

**典型用法：**
```
暴击触发特效：
  [OnCritHit] → [PlayNiagara](NS_CritSpark, Target=Target)

暴击叠加：
  [OnCritHit] → [ApplyRuneGE](DA_Frenzy)   ← 暴击时叠加狂怒层数
```

---

#### `On Kill`
触发时机：**本角色击杀目标时**  
上下文写入：`LastKillLocation` = 击杀位置，`DamageReceiver` = 被击杀目标

输出：`OnKill`

**典型用法：**
```
击杀加速：
  [OnKill] → [ApplyAttributeModifier]
                 Attribute = MoveSpeed, Value = 200, Duration = 3s

击杀生成拾取物：
  [OnKill] → [SpawnActorAtLocation](BP_HealthOrb)   ← 在击杀位置生成血球
```

---

#### `On Dash`
触发时机：**本角色执行冲刺时**

输出：`OnDash`

**典型用法：**
```
冲刺后无敌：
  [OnDash] → [ApplyAttributeModifier]
                 Attribute = Defense, ModOp = Override, Value = 9999, Duration = 0.3s

冲刺粒子：
  [OnDash] → [PlayNiagara](NS_DashTrail, Target=BuffOwner)
```

---

#### `On Buff Added`
触发时机：**同角色上任意 BuffFlow（FA）启动时**  
注意：监听的是 FA 生命周期，不是特定 GE 的添加。

输出：`OnAdded`

**典型用法：**
```
符文联动（任意符文激活时触发自身效果）：
  [Start] → [OnBuffAdded]
                 └─ OnAdded → [GetRuneInfo](Rune.Shield)
                                    ├─ Found → [PlayNiagara](NS_ShieldSync)
                                    └─ NotFound → [结束]
```

---

#### `On Buff Removed`
触发时机：**同角色上任意 BuffFlow（FA）停止时**

输出：`OnRemoved`

**典型用法：**
```
符文卸下时触发爆发：
  [Start] → [OnBuffRemoved]
                 └─ OnRemoved → [ApplyEffect](GE_RemovalBurst, Instant)
```

---

#### `On Periodic`
触发时机：**每隔固定时间触发一次**（`In` 启动，`Stop` 停止）

配置：`Interval`（秒），`bFireImmediately`（是否立即触发首次）

输出：`Tick`

**典型用法：**
```
DoT 可视化反馈（每秒播特效，配合 DA.Period 驱动的数值伤害）：
  [Start] → [OnPeriodic](Interval=1.0)
                 └─ Tick → [PlayNiagara](NS_PoisonTick)

护盾剩余时间告警（每 0.5s 检查一次）：
  [Start] → [OnPeriodic](Interval=0.5)
                 └─ Tick → [GetRuneInfo](Rune.Shield)
                                 └─ Found → [CompareFloat](TimeRemaining <= 2.0)
                                                  └─ True → [PlayNiagara](NS_ShieldWarning)
```

---

### 🔍 条件类节点（查询状态，控制分支）

---

#### `Get Rune Info`  ⭐
查询目标 ASC 上匹配 `RuneTag` 的符文 GE 的运行时状态。

| 配置项 | 说明 |
|---|---|
| `Target` | 查询目标 |
| `RuneTag` | DA 中 `RuneConfig.RuneTag` 的值，两处必须一致 |

| 数据输出引脚 | 类型 | 说明 |
|---|---|---|
| `bIsActive` | Bool | GE 是否活跃 |
| `StackCount` | Int32 | 当前叠加层数 |
| `Level` | Float | GE 等级 |
| `TimeRemaining` | Float | 剩余秒数，-1 = 永久 |

执行输出：`Found` / `NotFound`

**典型用法：**
```
层数阈值触发：
  [GetRuneInfo](Rune.Berserk) → Found → [CompareFloat](StackCount >= 5) → True → [效果]

剩余时间告警：
  [GetRuneInfo](Rune.Shield) → Found → [CompareFloat](TimeRemaining <= 2.0) → True → [特效]

符文是否在身 → 条件路由：
  [GetRuneInfo](Rune.Poison) → Found → [走毒强化路线] / NotFound → [走普通路线]
```

---

#### `Compare Float`
比较两个浮点数。`A` 支持数据引脚连线，`B` 直接填阈值。

| 配置 | 说明 |
|---|---|
| `A` | 左操作数（可连入 GetRuneInfo.StackCount、GetAttribute.CachedValue 等） |
| `Operator` | `>` / `>=` / `==` / `<=` / `<` / `!=` |
| `B` | 右操作数（阈值，直接填写） |

输出：`True` / `False`

**典型用法：**
```
层数判断：
  GetRuneInfo.StackCount ──→ CompareFloat.A
                             CompareFloat.B = 5, Operator = >=

血量低于阈值：
  GetAttribute(Health).CachedValue ──→ CompareFloat.A
                                        CompareFloat.B = 30, Operator = <=
```

---

#### `Get Attribute`
读取目标的某个属性当前值，输出到 `CachedValue`（可连入 CompareFloat.A）。

配置：`Attribute`, `Target` | 输出：`Out`，数据引脚 `CachedValue`

**典型用法：**
```
根据攻速动态调整效果值：
  [GetAttribute](AttackSpeed) → CachedValue ──→ [ApplyAttributeModifier].Value
  ← 攻速越高，施加的额外加成越大

血量条件判断：
  [GetAttribute](Health) → CachedValue ──→ [CompareFloat].A
                                            CompareFloat.B = 50, Operator = <=
                                            → True → [触发低血量效果]
```

---

#### `Has Tag`
检查目标是否携带某个 GameplayTag。

配置：`Target`, `Tag` | 输出：`True` / `False`（注：代码中实际为 `Yes` / `No`）

**典型用法：**
```
检查中毒状态：
  [OnDamageDealt] → [HasTag](Target, State.Poisoned)
                         └─ True → [ApplyEffect](GE_PoisonBonus)   ← 对中毒目标额外伤害

互斥检查：
  [HasTag](BuffOwner, State.Exhausted)
       └─ True → [FinishBuff]   ← 疲惫状态下跳过逻辑
```

---

#### `Check Target Type`
判断伤害目标是敌人还是自身。

输出：`对敌人` / `对自己`

**典型用法：**
```
仅对敌人触发效果：
  [OnDamageDealt] → [CheckTargetType]
                         ├─ 对敌人 → [ApplyEffect](GE_Bleed)
                         └─ 对自己 → [结束]   ← 反伤不触发流血
```

---

### ⚡ 效果类节点（执行实际动作）

---

#### `Apply Rune GE`  ⭐
将符文 DA 的 RuneConfig 构建为 GE 并施加到目标，**FA 停止时自动移除（含所有叠层）**。  
GE 生命周期的核心节点。BackpackGrid 只负责启停 FA，GE 完全由此节点管理。

| 配置项 | 说明 |
|---|---|
| `RuneAsset` | 符文 DA（RuneConfig + Effects → 构建 GE） |
| `Target` | 施加目标（通常 BuffOwner） |
| `Level` | GE 等级 |

输出：`Out` / `Failed`

**典型用法：**
```
永久被动（激活即生效）：
  [Start] → [ApplyRuneGE](DA_Rune_Knockback)

命中叠层：
  [OnDamageDealt] → [ApplyRuneGE](DA_Rune_Berserk)
  ← GAS 按 DA.StackType 自动处理叠层

击杀触发：
  [OnKill] → [ApplyRuneGE](DA_Rune_Bloodlust)   ← 击杀后叠加嗜血层

DoT 符文：
  [Start] → [ApplyRuneGE](DA_Rune_Poison)        ← DA.Period=1.0 驱动每秒伤害
```

---

#### `Apply Attribute Modifier`  ⭐
**无需 DA 或 GE 资产**，直接在节点上配置属性修改并施加。`Value` 支持固定数值和数据引脚连线两种模式。

| 配置项 | 说明 |
|---|---|
| `Attribute` | 要修改的属性（下拉选择） |
| `Mod Op` | Additive（加）/ Multiplicative（乘）/ Override（覆盖） |
| `Value` | 修改数值（无连线 = 固定值；连线 = 运行时动态值） |
| `Duration Type` | Instant（立即）/ Infinite（持续到 FA 停止）/ HasDuration（固定秒数） |
| `Duration` | 秒数（仅 HasDuration 时显示） |
| `Target` | 施加目标 |

输出：`Out` / `Failed`

**典型用法：**
```
击杀后临时加速（固定值）：
  [OnKill] → [ApplyAttributeModifier]
                 Attribute = MoveSpeed, Value = 200, Duration = 3s

暴击后攻速加成（固定值）：
  [OnCritHit] → [ApplyAttributeModifier]
                    Attribute = AttackSpeed, Value = 0.2, Duration = 2s

动态数值（根据当前攻速计算加成）：
  [GetAttribute](AttackSpeed) → CachedValue ──→ [ApplyAttributeModifier].Value
                                                  Attribute = Attack, Duration = 5s
  ← 攻速越高，攻击加成越大

层数驱动伤害加成：
  [GetRuneInfo](Rune.Berserk) → StackCount ──→ [ApplyAttributeModifier].Value
                                                  Attribute = Attack, Instant
  ← 每次命中时，按当前层数施加即时攻击加成
```

---

#### `Apply Effect`
向目标施加一个 GE 类（直接引用 GE 资产，不经过 DA）。适合需要复杂配置（ExecutionCalculation、SetByCaller 等）的效果。

配置：`Effect`（GE 类）, `Level`, `Target` | 输出：`Out` / `Failed`

**典型用法：**
```
触发即时爆发效果（GE 已在编辑器中配置好数值）：
  [OnDamageDealt] → [GetRuneInfo](Rune.Berserk)
                         └─ Found → [ApplyEffect](GE_BerserkBurst, Target=Target)

触发独立叠加 GE（不经过 DA）：
  [OnDamageDealt] → [ApplyEffect](GE_BerserkStack)   ← 命中叠层，由 CompareFloat 读取
```

---

#### `Grant GA`  ⭐
向目标授予 GameplayAbility，**FA 停止时自动撤销**。

| 配置项 | 说明 |
|---|---|
| `AbilityClass` | 要授予的 GA 类（继承自 RuneGameplayAbility） |
| `Target` | 授予目标（通常 BuffOwner） |
| `AbilityLevel` | GA 等级（默认 1） |

输出：`Out` / `Failed`

**典型用法：**
```
击退符文（GE 写入属性，GA 读取并执行行为）：
  [Start]
    → [ApplyRuneGE](DA_Rune_Knockback)       ← GE 写入 KnockbackForce +600
         └─ Out → [GrantGA](GA_Knockback)    ← 授予击退 GA（监听 Event.Combat.Knockback）
                       └─ Out → [OnDamageDealt]
                                    → [PlayNiagara](NS_KnockbackImpact, Target=Target)

技能加强（装备符文时临时授予额外技能）：
  [Start] → [GrantGA](GA_IceTrail)   ← 移动时留下冰迹，FA 停止时自动撤销
```

---

#### `Do Damage`
直接向目标造成伤害。支持固定伤害值或基于上次伤害事件的倍率。

| 配置项 | 说明 |
|---|---|
| `DamageEffect` | 伤害用 GE 类（需配置 SetByCaller(Data.Damage)） |
| `FlatDamage` | 固定伤害值（>0 时使用，否则用倍率） |
| `DamageMultiplier` | 上次事件伤害量的倍率（FlatDamage=0 时生效） |
| `Target` | 目标（默认：上次伤害目标） |

输出：`Out` / `Failed`

**典型用法：**
```
命中追加固定伤害：
  [OnDamageDealt] → [DoDamage](GE_Bleed, FlatDamage=30)

反伤（受到伤害的 50%）：
  [OnDamageReceived] → [DoDamage](GE_Thorns, DamageMultiplier=0.5, Target=DamageCauser)

击杀爆炸（固定 AOE 伤害，通过 SpawnActor 的碰撞体触发）：
  [OnKill] → [SpawnActorAtLocation](BP_Explosion)
  ← BP_Explosion 内部负责 AOE 伤害，DoD amage 也可直接用于单目标
```

---

#### `Add Tag` / `Remove Tag`
手动给目标的 ASC 添加/移除 GameplayTag（独立于 GE 生命周期）。

配置：`Tag`, `Count`, `Target` | 输出：`Out`

**典型用法：**
```
临时标记调试状态：
  [Start] → [AddTag](BuffOwner, Debug.Rune.Active)

互斥控制（手动添加冷却标记，一段时间后移除）：
  [OnKill] → [HasTag](BuffOwner, Rune.Cooldown.Bloodlust)
                  └─ False → [ApplyAttributeModifier](...)
                              → [AddTag](BuffOwner, Rune.Cooldown.Bloodlust)
  ← 结合 OnPeriodic 计时后 RemoveTag
```

---

#### `Finish Buff`
终止当前 BuffFlow 实例（触发 Cleanup）。无输出引脚，是 FA 的终止节点。

**典型用法：**
```
找不到 GE 时提前退出（避免无效循环）：
  [OnPeriodic] → [GetRuneInfo](Rune.Shield)
                      └─ NotFound → [FinishBuff]

一次性触发后关闭（Instant 符文逻辑只需执行一次）：
  [Start] → [ApplyRuneGE](...) → Out → [FinishBuff]
```

---

### 🎨 视觉/音频类节点

---

#### `Play Niagara`
在目标位置播放 Niagara 粒子系统，并注册到 BFC.ActiveNiagaraEffects（可由 DestroyNiagara 手动销毁）。

| 配置项 | 说明 |
|---|---|
| `NiagaraSystem` | 粒子资产 |
| `Target` | 附着目标（Actor） |
| `AttachSocket` | 骨骼 Socket 名称（可选） |
| `EffectName` | 注册名称（供 DestroyNiagara 查找） |
| `bDestroyWithFlow` | true = FA 停止时自动销毁粒子（Cleanup）|

输出：`Out`

**典型用法：**
```
命中闪光（Fire & Forget，无需手动销毁）：
  [OnDamageDealt] → [PlayNiagara](NS_HitSpark, Target=Target, bDestroyWithFlow=false)

持续 Buff 光环（FA 停止时自动销毁）：
  [Start] → [PlayNiagara](NS_BerserkAura, Target=BuffOwner, bDestroyWithFlow=true)

中毒粒子（每秒触发）：
  [OnPeriodic](1.0s) → [PlayNiagara](NS_PoisonTick, Target=BuffOwner)
```

---

#### `Destroy Niagara`
按 `EffectName` 销毁之前由 `PlayNiagara` 注册的粒子。

配置：`EffectName` | 输出：`Out`

**典型用法：**
```
条件撤销光环（层数归零时关闭粒子）：
  [GetRuneInfo] → [CompareFloat](StackCount == 0)
                       └─ True → [DestroyNiagara](BerserkAura)
```

---

#### `Play Montage`
在目标角色上播放 AnimMontage（立即输出 Out，不等待蒙太奇结束）。

配置：`Montage`, `Target`, `PlayRate` | 输出：`Out`

---

#### `Spawn Actor At Location`
在击杀位置或自定义位置生成 Actor（子弹、AOE 碰撞体、拾取物等）。

| 配置项 | 说明 |
|---|---|
| `ActorClass` | 要生成的 Actor 类 |
| `bUseKillLocation` | true = 使用 OnKill 写入的 LastKillLocation |
| `Offset` | 位置偏移 |

输出：`Out`

**典型用法：**
```
击杀生成血球：
  [OnKill] → [SpawnActorAtLocation](BP_HealthOrb, bUseKillLocation=true)

击杀爆炸：
  [OnKill] → [SpawnActorAtLocation](BP_DeathExplosion, Offset=(0,0,50))
```

---

## 四、完整使用案例

### 案例 1：狂暴（装备时持续加攻速，命中触发爆发）

**DA_Rune_Berserk：**
```
Duration Type: 永久 | Unique Type: 唯一 | Stack Type: 刷新
Rune Tag: Rune.Berserk
Effects[0]: AttackSpeed Additive +0.15
```

**FA_Berserk：**
```
[Start] → [ApplyRuneGE](DA_Rune_Berserk)   ← 持续 +攻速

[OnDamageDealt]
  → [GetRuneInfo](Rune.Berserk)
       └─ Found → [ApplyEffect](GE_BerserkBurst, Target=Target)
```

---

### 案例 2：击退（GE 增强属性，GA 执行行为）

**DA_Rune_Knockback：**
```
Duration Type: 永久 | Rune Tag: Rune.Knockback
Effects[0]: KnockbackForce Additive +600  (RuneAttributeSet)
```

**FA_Knockback：**
```
[Start]
  → [ApplyRuneGE](DA_Rune_Knockback)
       └─ Out → [GrantGA](GA_Knockback)
                    └─ Out → [OnDamageDealt]
                                 → [PlayNiagara](NS_KnockbackImpact, Target=Target)
```

---

### 案例 3：持续 DoT（Period 驱动，FA 做可视化）

**DA_Rune_Poison：**
```
Duration Type: 有时限(8s) | Period: 1.0 | Stack Type: 叠加(Max=5)
Effects[0]: Health Additive -15
```

**FA_Poison：**
```
[Start] → [ApplyRuneGE](DA_Rune_Poison)

[OnPeriodic](1.0s)
  → [GetRuneInfo](Rune.Poison)
       └─ Found
            → [PlayNiagara](NS_PoisonTick)
            → [CompareFloat](StackCount >= 5)
                 └─ True → [PlayNiagara](NS_PoisonOverdose)
```

---

### 案例 4：击杀临时加速（ApplyAttributeModifier 直接配置）

**无需 DA，全部在 FA 内完成：**
```
[Start] → [OnKill]
                → [ApplyAttributeModifier]
                      Attribute    = MoveSpeed
                      ModOp        = Additive
                      Value        = 200.0
                      DurationType = HasDuration
                      Duration     = 3.0s
                → [PlayNiagara](NS_KillRush, Target=BuffOwner)
```

---

### 案例 5：层数驱动动态加成（数据引脚连线）

**狂暴层数越多，命中时的攻击加成越高：**
```
[OnDamageDealt]
  → [GetRuneInfo](Rune.Berserk)
       └─ Found
            → [ApplyAttributeModifier]
                  StackCount ──→ Value   ← 数据引脚连线
                  Attribute  = Attack
                  ModOp      = Additive
                  DurationType = Instant  ← 瞬间加成（每次命中重算）
```

---

## 五、常见问题

**Q：`Apply Rune GE` 和 `Apply Attribute Modifier` 怎么选？**

| 场景 | 推荐节点 |
|---|---|
| 符文激活时施加 DA 定义的完整效果（含堆叠/Tag/Duration 配置） | `Apply Rune GE` |
| 临时加速/减速、触发性小加成、不想建 DA 的一次性效果 | `Apply Attribute Modifier` |
| 数值由运行时数据（层数/属性值）决定 | `Apply Attribute Modifier`（Value 连数据引脚）|
| 需要 ExecutionCalculation 或复杂 SetByCaller 逻辑 | `Apply Effect`（引用 GE 资产）|

**Q：`Apply Attribute Modifier` 的 Value 怎么连数据引脚？**  
A：从 `GetRuneInfo.StackCount`、`GetAttribute.CachedValue` 等节点的数据输出引脚拖线到 `ApplyAttributeModifier` 节点的 `Value` 输入引脚即可。无连线时使用节点属性面板中填写的固定值。

**Q：DoT 的伤害周期用 DA 的 Period 还是 FA 的 OnPeriodic？**  
A：两者用途不同：
- DA 的 `Period` = GAS 内置机制，驱动 GE 的 Effects 重复触发（数值伤害）
- FA 的 `OnPeriodic` = 可视化反馈（特效/音效/UI），Interval 可与 Period 不同

**Q：BackpackGrid 还负责施加 GE 吗？**  
A：不负责。BackpackGrid 只调用 `StartBuffFlow()`/`StopBuffFlow()`，GE 的 apply 和 remove 全部由 `ApplyRuneGE`（或 `ApplyAttributeModifier`）节点的 `Cleanup()` 负责。

---

## 六、架构变更记录

### rev5（2026-04-05）
| 变更 | 内容 |
|---|---|
| 新增节点 | `Apply Attribute Modifier`（BFNode_ApplyAttributeModifier）— 内联 GE，Value 支持数据引脚 |
| 删除节点 | `Add Rune Effect`（BFNode_AddRune）— 与 ApplyRuneGE 冲突（双重 GE 施加） |
| 删除节点 | `Remove Rune Effect`（BFNode_RemoveRune）— 被 ApplyRuneGE.Cleanup() 完全覆盖 |

### rev4（2026-04-05）
| 变更 | 内容 |
|---|---|
| GE 生命周期 | BackpackGrid → FA 内 `ApplyRuneGE` 节点全权负责 |
| FPlacedRune | 移除 `ActiveEffectHandle` 字段 |
| FA 规则 | FA 通过 `ApplyRuneGE` 引用 DA 是允许且必要的 |

### rev3（2026-04-05）
`DurationType` 枚举、`Period`、`UniqueType`、`RuneType` 移入 FRuneConfig，字段全部从 `Buff*` 重命名为 `Rune*`。
