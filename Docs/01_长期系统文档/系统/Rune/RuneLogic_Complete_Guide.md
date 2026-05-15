# 符文逻辑完整指南

> 版本：v1.0 | 日期：2026-04-24  
> 定位：从概念到配置的完整符文开发手册  
> 前置文档：[Rune_System_Design](../../../04_调研与玩法设计/设计文档/Rune/Rune_System_Design.md)（设计框架）、[TestRune_CreationGuide](TestRune_CreationGuide.md)（已实现符文）

---

## 一、系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                     数据层（Data Layer）                      │
│                                                              │
│  URuneDataAsset                                              │
│    └── FRuneInstance                                          │
│          ├── FRuneConfig    → 名称/图标/TriggerType/链路      │
│          ├── FRuneShape     → 背包格子占位                    │
│          └── FRuneFlowConfig → 指向 UFlowAsset（FA）          │
└───────────────────────────┬──────────────────────────────────┘
                            │ 放入背包 / AN_MeleeDamage 携带
┌───────────────────────────▼──────────────────────────────────┐
│                   管理层（Management Layer）                   │
│                                                               │
│  BackpackGridComponent                                        │
│    ├── Passive 符文 → 进激活区直接 StartBuffFlow              │
│    └── Event 符文   → 注册 ASC 事件监听 → 事件触发时 Start    │
│                                                               │
│  AN_MeleeDamage.AdditionalRuneEffects                         │
│    └── 敌人攻击命中时 → ReceiveOnHitRune → StartBuffFlow      │
└───────────────────────────┬──────────────────────────────────┘
                            │ StartBuffFlow / StopBuffFlow
┌───────────────────────────▼──────────────────────────────────┐
│                   运行层（Execution Layer）                    │
│                                                               │
│  BuffFlowComponent (BFC)                                      │
│    └── UFlowSubsystem::StartRootFlow(BFC, FlowAsset)         │
│          └── FA 图中的 BFNode 节点按连线执行                   │
│                ├── 触发节点（OnDamageDealt / OnKill / ...）    │
│                ├── 效果节点（ApplyAttributeModifier / ...）    │
│                ├── 逻辑节点（Probability / HasTag / ...）      │
│                └── 表现节点（PlayNiagara / HitStop / ...）     │
└──────────────────────────────────────────────────────────────┘
```

### 1.1 核心组件

| 组件 | 文件 | 职责 |
|---|---|---|
| URuneDataAsset | `Source/DevKit/Public/Data/RuneDataAsset.h` | 符文数据容器：元信息 + FA 引用 |
| UBuffFlowComponent | `Source/DevKit/Public/BuffFlow/BuffFlowComponent.h` | FA 生命周期管理：启动/停止/共享上下文 |
| UBackpackGridComponent | `Source/DevKit/Public/Component/BackpackGridComponent.h` | 背包格子 + 激活区 + 热度/阶段 |
| UBFNode_Base | `Source/DevKit/Public/BuffFlow/Nodes/BFNode_Base.h` | 所有 FA 节点基类，访问 BFC/ASC/Owner |

### 1.2 生命周期

```
DA 创建 → 放入背包 → 进入激活区 →【Passive: 立即启动 FA】
                                   【Event: 注册监听器，事件触发时启动 FA】
    ↓
FA 运行中（节点按图执行，触发器持续监听）
    ↓
离开激活区 / 符文移除 → StopBuffFlow → 所有节点 Cleanup()
                                        ├── GE 自动移除
                                        ├── GA 自动撤销
                                        ├── Tag 自动清理
                                        └── 计时器/粒子清理
```

---

## 二、触发类型（ERuneTriggerType）

符文的 `TriggerType` 决定 FA 何时被启动。

| TriggerType | 枚举值 | BGC 行为 | FA 启动时机 |
|---|---|---|---|
| Passive | `Passive` | 进激活区直接 `StartBuffFlow` | 立即常驻运行 |
| OnAttackHit | `OnAttackHit` | 注册 `Ability.Event.Attack.Hit` | 攻击命中时，每次命中启动一次 |
| OnDash | `OnDash` | 注册 `Ability.Event.Dash` | 冲刺时启动 |
| OnKill | `OnKill` | 注册 `Ability.Event.Kill` | 击杀目标时启动 |
| OnCritHit | `OnCritHit` | 注册暴击事件 | 暴击时启动 |
| OnDamageReceived | `OnDamageReceived` | 注册 `Ability.Event.Damaged` | 自身受伤时启动 |

### Passive vs Event 的区别

| 维度 | Passive | Event |
|---|---|---|
| FA 实例存活时间 | 与激活区同生命周期（常驻） | 每次事件创建新实例（一次性） |
| RuneGuid | 使用背包格子里的 RuneGuid | 每次 `FGuid::NewGuid()` |
| 触发节点 | FA 内用 OnDamageDealt 等节点自行监听 | FA 从入口直接执行效果 |
| 典型用法 | 持续增益、周期 DoT、状态监听 | 命中附加效果、击杀爆发 |

### 什么时候用哪种

- **Passive + FA 内触发节点**：需要持续监听多种事件、需要记住状态（如叠层）、需要同时运行多个逻辑分支
- **Event TriggerType**：简单的"事件→效果"一次性逻辑、不需要持续状态

> 目前大部分已实现符文（1001-1021）使用 Passive + FA 内触发节点模式。

---

## 三、数据资产配置

### 3.1 RuneDataAsset（DA）

每个符文需要一个 DA，包含：

| 字段 | 说明 | 示例 |
|---|---|---|
| **RuneConfig.RuneID** | 策划表 ID | `1001` |
| **RuneConfig.RuneName** | 显示名称 | `攻击强化` |
| **RuneConfig.RuneDescription** | 描述文字 | `激活期间攻击力+10` |
| **RuneConfig.RuneIcon** | 图标 | `T_Rune_AttackUp` |
| **RuneConfig.RuneType** | Buff / Debuff / None | `Buff` |
| **RuneConfig.TriggerType** | 触发类型 | `Passive` |
| **RuneConfig.GoldCost** | 购买价格（卖价=半价） | `100` |
| **RuneConfig.ChainRole** | None / Producer / Consumer | `None` |
| **RuneConfig.ChainDirections** | 链路传导方向（8方向） | `[]` |
| **Shape.Cells** | 占格形状 | `[(0,0), (1,0)]` |
| **Flow.FlowAsset** | 指向的 FA 资产 | `FA_Rune_AttackUp` |

### 3.2 FlowAsset（FA）

FA 是符文的核心逻辑载体，在 UE 编辑器中以可视化节点图的形式编辑。

**创建方式**：Content Browser → 右键 → Flow → Flow Asset

**命名规范**：`FA_Rune_<符文名>` 或 `FA_Effect_<效果名>`

---

## 四、BFNode 节点参考手册

### 4.1 触发节点（Trigger）

触发节点是 FA 的事件入口。执行到触发节点后，节点注册委托并等待事件发生，事件触发时执行输出引脚。

| 节点 | 显示名 | 触发条件 | 输出数据引脚 |
|---|---|---|---|
| BFNode_OnDamageDealt | On Damage Dealt | BuffOwner 对别人造成伤害 | `LastDamageOutput` (float) |
| BFNode_OnDamageReceived | On Damage Received | BuffOwner 自身受伤 | `CachedDamage` (float) |
| BFNode_OnKill | On Kill | BuffOwner 击杀目标 | — (写入 BFC.LastKillLocation) |
| BFNode_OnCritHit | On Crit Hit | BuffOwner 暴击命中 | — |
| BFNode_OnDash | On Dash | BuffOwner 冲刺 | — |
| BFNode_OnPeriodic | On Periodic | 定时触发 | — |
| BFNode_OnHealthChanged | On Health Changed | BuffOwner HP 变化 | `NewHP` (float) |
| BFNode_OnPhaseUpReady | On Phase Up Ready | 热度满 + LastHit 升阶 | — |
| BFNode_OnHeatReachedZero | On Heat Reached Zero | 热度归零 | — |
| BFNode_OnBuffAdded | On Buff Added | 其他 FA 启动 | — |
| BFNode_OnBuffRemoved | On Buff Removed | 其他 FA 停止 | — |
| BFNode_WaitGameplayEvent | Wait Gameplay Event | 收到指定 GameplayEvent | EventData |

**关键属性**：

```
OnDamageDealt:
  bOncePerSwing = false    // true: 同帧多命中只触发一次（AOE 用）

OnPeriodic:
  Interval        = 1.0    // 触发间隔（秒）
  bFireImmediately = false // 首次是否立即触发

WaitGameplayEvent:
  EventTag = Action.Knockback  // 要监听的事件 Tag
```

### 4.2 效果节点（Effect）

| 节点 | 显示名 | 用途 | 需要 GE 资产 |
|---|---|---|---|
| ApplyAttributeModifier | Apply Attribute Modifier | 运行时创建 GE 修改属性 | **不需要** |
| ApplyEffect | Apply Gameplay Effect Class | 施加已有 GE Class | 需要 |
| CalcDamage | Calc Damage | 纯计算伤害值 | 不需要 |
| DoDamage | Do Damage | 快速施加伤害 | 需要 |
| AreaDamage | Area Damage | 范围伤害 | 需要 |
| ApplyGEInRadius | Apply GE In Radius | 范围施加 GE | 需要 |
| GrantGA | Grant GA | 授予 GA | 不需要 |
| SendGameplayEvent | Send Gameplay Event | 向目标发送事件触发 GA | 不需要 |

#### ApplyAttributeModifier（最常用节点）

运行时动态创建 GE，**无需任何蓝图 GE 资产**。大部分符文效果都用这个节点实现。

```
核心属性：
  Attribute       — 目标属性（下拉选择，如 BaseAttributeSet.Attack）
  ModOp           — Additive(加) / Multiplicative(乘) / Override(覆盖)
  Value           — 修改数值（支持数据引脚连线）
  DurationType    — Instant(一次性) / Infinite(持续到FA停止) / HasDuration(定时)
  Duration        — 持续秒数（仅 HasDuration）
  Period          — 周期触发间隔（0=不启用）
  bFireImmediately — Period 首次是否立刻触发
  Target          — 目标选择器

Granted 属性（GE 存续期间生效，到期自动移除）：
  GrantedTagsToASC  — 授予目标 ASC 的 Tag
  GrantedAbilities  — 授予目标 ASC 的 GA

堆叠属性：
  StackMode           — None / Unique / Stackable
  StackLimitCount     — 最大层数（Stackable 时）
  StackDurationRefreshPolicy — 刷新/不刷新持续时间
  StackExpirationPolicy      — 全部清空/逐层移除
```

**典型用法示例**：

```
永久增益（Infinite，FA 停止时自动移除）：
  Attribute=Attack, ModOp=Additive, Value=10, DurationType=Infinite

定时增益（HasDuration，自动到期）：
  Attribute=MoveSpeed, ModOp=Additive, Value=200, Duration=3.0

周期伤害（Period + HasDuration）：
  Attribute=Health, ModOp=Additive, Value=-25, Duration=5.0, Period=1.0

叠层增益（Stackable + HasDuration）：
  Attribute=MoveSpeed, Value=30, Duration=3.0
  StackMode=Stackable, MaxStacks=5
  DurationRefresh=Refresh, ExpirationPolicy=RemoveSingleStack

GE 伴生 GA 授予（GrantedAbilities）：
  Duration=10.0, GrantedTagsToASC=Buff.Status.Bleeding
  GrantedAbilities=[GA_Bleed]  ← GE 到期时 GA 自动撤销
```

#### CalcDamage（纯计算节点）

不直接造成伤害，只计算伤害值并输出到 `FinalDamage` 数据引脚，供下游节点使用。

```
属性：
  BaseMode  — Flat(固定值) / PercentMaxHP / PercentCurrentHP / PercentMissingHP
  BaseValue — 基础数值（Flat=直接值，Percent=0~1 如 0.07=7%）
  Target    — 读取哪个目标的属性

条件乘算（最多 3 组）：
  MultTag1      = Buff.Status.Bleeding   // 检测目标是否有此 Tag
  MultCondition1 = HasTag                // HasTag / MissingTag
  MultValue1     = 1.15                  // 有 Tag 时伤害 ×1.15

输出：
  FinalDamage → 数据引脚（连线到 ApplyAttributeModifier.Value 等）
```

#### SendGameplayEvent

向目标发送 GameplayEvent，触发目标 ASC 上监听该 Tag 的 GA。

```
属性：
  EventTag   — 发送的事件 Tag（如 Action.Knockback）
  Target     — 接收事件的 Actor（通常 LastDamageTarget）
  Instigator — 事件发起者（通常 BuffOwner / DamageCauser）
  Magnitude  — 携带数值（GA 内通过 EventMagnitude 读取）
```

#### GrantGA

向目标授予 GA，FA 停止时自动撤销。

```
属性：
  AbilityClass — 要授予的 GA 类
  Target       — BuffOwner / LastDamageTarget
  AbilityLevel — 数据引脚可驱动
```

### 4.3 逻辑节点（Condition / Logic）

| 节点 | 显示名 | 用途 |
|---|---|---|
| Probability | Probability | 概率分支（0~1） |
| HasTag | Has Tag | 检查目标是否有指定 Tag |
| IfStatement | If Statement | 布尔分支 |
| CompareFloat | Compare Float | 数值比较（>、<、>=、==...） |
| CompareInt | Compare Int | 整数比较 |
| GetAttribute | Get Attribute | 读取属性值（输出 CachedValue） |
| GetRuneInfo | Get Rune Info | 查询 GE 状态（层数/时间/是否存在） |
| CheckTargetType | Check Target Type | 检查目标类型（玩家/敌人） |
| DoOnce | Do Once | 只执行一次（后续触发跳过） |

### 4.4 工具节点（Utility）

| 节点 | 显示名 | 用途 |
|---|---|---|
| AddTag | Add Tag | 向目标 ASC 添加 Tag |
| RemoveTag | Remove Tag | 移除 Tag |
| GrantTag | Grant Tag (Timed) | 授予限时 Tag（到期自动移除） |
| Delay | Delay | 延时执行 |
| MathFloat | Math Float | 四则运算（+−×÷） |
| MathInt | Math Int | 整数运算 |
| Literals | Literals | 常量输出 |
| FinishBuff | Finish Buff | 手动终止当前 FA |
| IncrementPhase | Increment Phase | 热度阶段 +1 |
| DecrementPhase | Decrement Phase | 热度阶段 -1 |

### 4.5 表现节点（Visual / Audio）

| 节点 | 显示名 | 用途 |
|---|---|---|
| PlayNiagara | Play Niagara | 播放 Niagara 粒子 |
| DestroyNiagara | Destroy Niagara | 销毁粒子 |
| PlayMontage | Play Montage | 播放动画蒙太奇 |
| HitStop | Hit Stop | 命中停顿（冻帧） |
| SpawnGameplayCueAtLocation | Spawn GC at Location | 在位置播放 GameplayCue |
| SpawnGameplayCueOnActor | Spawn GC on Actor | 在 Actor 上播放 GameplayCue |
| SpawnActorAtLocation | Spawn Actor | 在位置生成 Actor |

---

## 五、目标选择器（EBFTargetSelector）

所有需要指定目标的节点都使用 `EBFTargetSelector`。

| 值 | 显示名 | 含义 |
|---|---|---|
| `BuffOwner` | Buff 拥有者 | 拥有 BFC 的角色（FA 运行在谁身上） |
| `BuffGiver` | Buff 施加者 | StartBuffFlow 的 Giver 参数 |
| `LastDamageTarget` | 上次伤害目标 | 最近一次伤害的被击者 |
| `DamageCauser` | 伤害来源 | 最近一次伤害的攻击者 |

### 玩家 vs 敌人的 Target 含义

| 场景 | BuffOwner | BuffGiver | LastDamageTarget |
|---|---|---|---|
| 玩家符文 FA（从背包激活） | 玩家 | 玩家 | 被攻击的敌人 |
| 敌人 AN_MeleeDamage FA（ReceiveOnHitRune） | 敌人（攻击者） | 玩家（被打者） | 玩家（被打者） |

---

## 六、共享上下文（FBFEventContext）

BFC 上维护一个 `LastEventContext`，由触发节点写入，供后续节点读取。

```cpp
struct FBFEventContext
{
    TWeakObjectPtr<AActor> DamageCauser;    // 攻击者
    TWeakObjectPtr<AActor> DamageReceiver;  // 被击者
    float DamageAmount;                      // 伤害量
    FGameplayTag EventTag;                   // 事件 Tag
};
```

另外 BFC 还维护：
- `LastKillLocation` — 由 OnKill 节点写入，供 SpawnActor 等节点读取
- `ActiveNiagaraEffects` — 按名称存储活跃粒子，供 DestroyNiagara 引用

---

## 七、符文制作工作流

### 7.1 从零创建一个符文

```
步骤 1：确定设计
  ├── 触发条件是什么？→ 选 TriggerType
  ├── 效果是什么？    → 选 BFNode 组合
  └── 需要额外资产吗？→ 判断是否需要 GE / GA

步骤 2：创建 FA
  ├── Content Browser → 右键 → Flow → Flow Asset
  ├── 命名 FA_Rune_<名称>
  ├── 双击打开 → 放置节点 → 连接引脚
  └── 保存

步骤 3：创建 DA
  ├── Content Browser → 右键 → Data Asset → RuneDataAsset
  ├── 命名 DA_Rune_<名称>
  ├── 填写 RuneConfig（ID/名称/图标/TriggerType）
  ├── 填写 Shape（格子形状）
  ├── 填写 Flow.FlowAsset → 指向刚创建的 FA
  └── 保存

步骤 4：测试
  ├── 将 DA 加入奖励池或直接放入背包
  ├── 进入激活区 → 验证效果
  └── 离开激活区 → 验证清理
```

### 7.2 零资产模式 vs 需要额外资产

**零资产**（大部分符文）：只需 FA + DA，无需蓝图 GE / GA。
- 属性修改 → `ApplyAttributeModifier`（运行时创建 GE）
- GA 授予 → `GrantGA` 或 `ApplyAttributeModifier.GrantedAbilities`
- Tag 授予 → `AddTag` / `GrantTag` 或 `ApplyAttributeModifier.GrantedTagsToASC`

**需要额外资产**的情况：
- GA 需要引用蓝图资产（如投射物类）→ 需要 GA Blueprint 子类
- GE 需要 AttributeBased Magnitude → 需要蓝图 GE
- GE 需要 SetByCaller → 需要蓝图 GE + `ApplyEffect` 节点

---

## 八、常用 FA 模式

### 模式 A：永久增益

```
[Start] → [ApplyAttributeModifier]
              Attribute    = BaseAttributeSet.Attack
              ModOp        = Additive
              Value        = 10.0
              DurationType = Infinite
              Target       = BuffOwner
```

FA 停止时自动移除。示例：1001 攻击强化。

### 模式 B：命中触发临时效果

```
[Start] → [OnDamageDealt] → [ApplyAttributeModifier]
                                Attribute    = BaseAttributeSet.MoveSpeed
                                ModOp        = Additive
                                Value        = 30.0
                                DurationType = HasDuration
                                Duration     = 3.0
                                StackMode    = Stackable
                                MaxStacks    = 5
                                Target       = BuffOwner
```

每次命中叠一层，自动到期。示例：1003 速度叠加。

### 模式 C：周期 + 条件判断

```
[Start]
  ↓
[OnPeriodic] Interval=1.0
  ↓
[HasTag] Tag=Buff.Status.HeatInhibit, Target=BuffOwner
  ↓ No                    ↓ Yes → (跳过)
[ApplyAttributeModifier]
    Attribute    = Heat
    ModOp        = Additive
    Value        = 1.0
    DurationType = Instant
```

周期触发 + Tag 条件守卫。示例：1002 热度提升。

### 模式 D：事件 → GA 触发

```
[Start] → [OnDamageDealt] → [SendGameplayEvent]
                                EventTag   = Action.Knockback
                                Target     = LastDamageTarget
                                Instigator = BuffOwner
```

通过 GameplayEvent 触发目标身上已有的 GA。示例：1004 击退。

### 模式 E：GE 伴生 GA

```
[Start] → [OnDamageDealt] → [ApplyAttributeModifier]
                                Duration        = 10.0
                                GrantedTagsToASC = Buff.Status.Bleeding
                                GrantedAbilities = [GA_Bleed]
                                Target           = LastDamageTarget
                             → [SendGameplayEvent]
                                EventTag  = Buff.Event.Bleed
                                Magnitude = 10.0
                                Target    = LastDamageTarget
```

GE 授予 Tag + GA，GE 到期时 GA 自动撤销。示例：1005 流血。

### 模式 F：符文联动（Wait Event）

```
[Start] → [WaitGameplayEvent]
              EventTag = Event.Rune.KnockbackApplied
              ↓
           [ApplyAttributeModifier]
              Target    = EventData.Target
              Attribute = MoveSpeed
              Value     = -300
              Duration  = 1.0
```

监听其他符文广播的事件。示例：1007 击退减速（依赖 1004 击退）。

### 模式 G：递归守卫（ExtraDamage 模式）

```
[OnDamageDealt]
  ↓
[HasTag] Tag=Buff.Status.ExtraDamageApplied, Target=BuffOwner
  ↓ No                              ↓ Yes → (跳过，防递归)
[AddTag] Buff.Status.ExtraDamageApplied
  ↓
[DoDamage] Target=LastDamageTarget, Multiplier=1.0
  ↓
[RemoveTag] Buff.Status.ExtraDamageApplied
```

DoDamage 会再次触发 OnDamageDealt，用 Tag 守卫防止无限递归。示例：1006 额外伤害。

### 模式 H：冲刺后限时窗口

```
[Start]

[OnDash] → [GrantTag(Timed)]
              Tag      = Buff.Status.DuoAssaultReady
              Duration = 2.0
              Target   = BuffOwner

[OnDamageDealt]
  ↓
[HasTag] Buff.Status.DuoAssaultReady
  ↓ Yes
[RemoveTag] DuoAssaultReady  ← 立即消耗
  ↓
[DoDamage] Multiplier=1.0   ← 双倍伤害
```

冲刺创造时间窗口，下次命中消耗。示例：1010 突刺连击。

### 模式 I：数据引脚计算链

```
[GetAttribute] MoveSpeed → CachedValue
  ↓ 数据引脚
[MathFloat] CachedValue - 600 → Result(bonus)
  ↓ 数据引脚
[MathFloat] bonus ÷ 6 → Result(pct)
  ↓ 数据引脚
[ApplyAttributeModifier] Value ← pct
```

多个节点通过数据引脚传递中间值。示例：1012 幽风低语。

### 模式 J：SubGraph 复用基础效果

```
符文 FA（如 FA_Rune_CritKnockback）:
[Start TriggerType=OnCritHit]
  ↓
[SubGraph Asset=FA_Effect_Knockback]
  ↓ Done
[Finish]

基础效果 FA（如 FA_Effect_Knockback）:
[Custom Input "Apply"]
  ↓
[ApplyAttributeModifier]
    ...击退效果配置...
  ↓
[Custom Output "Done"]
```

通过 SubGraph 隔离复用基础效果 FA，详见 [StatusEffect_EngineConfig_Guide](../../../04_调研与玩法设计/设计文档/StatusEffects/StatusEffect_EngineConfig_Guide.md)。

---

## 九、敌人配置

敌人和玩家共用同一套 FA 节点系统，区别仅在触发来源。

### 9.1 通过 AN_MeleeDamage 配置

在敌人的攻击动画 Montage 上放置 AN_MeleeDamage Notify，填写 `AdditionalRuneEffects`。

```
敌人攻击动画 → AN_MeleeDamage
  ├── EventTag = GameplayEffect.DamageType.GeneralAttack
  ├── ActDamage = 20
  ├── ActRange = 300
  └── AdditionalRuneEffects = [DA_Effect_Poison]  ← 攻击附带中毒
```

**执行流程**：
1. AN_MeleeDamage 将 DA 存入 `Character->PendingAdditionalHitRunes`
2. GA_MeleeAttack 命中时遍历 PendingAdditionalHitRunes
3. 对每个命中目标调用 `ReceiveOnHitRune(RuneDA, AttackInstigator)`
4. ReceiveOnHitRune 在**攻击者**的 BFC 上启动 FA
5. 此时 BuffOwner = 敌人，LastDamageTarget = 被命中的玩家

### 9.2 敌人也需要 BuffFlowComponent

敌人蓝图（EnemyCharacterBase 或子类）需要添加 `BuffFlowComponent`，否则 ReceiveOnHitRune 找不到 BFC。

### 9.3 配置示例

| 敌人类型 | AN_MeleeDamage.AdditionalRuneEffects | 效果 |
|---|---|---|
| 普通怪 | 空 | 纯物理伤害 |
| 毒蛇 | `[DA_Effect_Poison]` | 攻击附带中毒 |
| 火焰精英 | `[DA_Effect_Burn]` | 攻击附带燃烧 |
| Boss 重击 | `[DA_Effect_Knockback, DA_Effect_Stun]` | 击退 + 眩晕 |

> 同一敌人不同招式可以配不同的 AdditionalRuneEffects，如轻击无效果、重击附带眩晕。

---

## 十、伤害管道

FA 中的伤害节点（ApplyAttributeModifier / DoDamage / CalcDamage）最终修改 DamageAttributeSet 的属性，PostGameplayEffectExecute 统一处理后续逻辑。

| 属性 | 路径 | 行为 |
|---|---|---|
| DamagePhysical | 物理 | 护甲吸收 → 溢出扣 HP → 可击杀 → 广播 Damaged |
| DamagePure | 真实 | 护甲吸收 → 溢出扣 HP → 可击杀 → 广播 Damaged |
| DamageBuff | 状态 | 绕过护甲 → 直扣 HP → 不可击杀（HP≥1）→ 不广播 Damaged |

> **DamageBuff 不广播 Damaged** — 防止 DoT 伤害触发 GA_Wound 等"受伤时额外伤害"效果产生无限递归。

---

## 十一、数据引脚系统

BFNode 之间除了执行流（箭头）连线外，还支持数据引脚连线（传递数值）。

### 11.1 数据引脚类型

| 类型 | 输入属性 | 输出属性 |
|---|---|---|
| Float | `FFlowDataPinInputProperty_Float` | `FFlowDataPinOutputProperty_Float` |
| Int32 | `FFlowDataPinInputProperty_Int32` | `FFlowDataPinOutputProperty_Int32` |
| Bool | — | `FFlowDataPinOutputProperty_Bool` |

### 11.2 使用方式

- **无连线**：直接使用节点上填写的固定值
- **有连线**：运行时从上游节点的输出数据引脚读取值，覆盖固定值

```
[CalcDamage]                     [ApplyAttributeModifier]
  FinalDamage ─────数据引脚────→ Value
  (输出)                          (输入)
```

### 11.3 常见数据流

```
GetAttribute.CachedValue    → MathFloat.A           (读属性值)
MathFloat.Result            → CompareFloat.A         (中间计算)
CalcDamage.FinalDamage      → ApplyAttributeModifier.Value (伤害传递)
OnDamageDealt.LastDamageOutput → MathFloat.A         (本次伤害量)
GetRuneInfo.StackCount      → CompareInt.A           (层数判断)
Probability.Chance          ← MathFloat.Result       (动态概率)
```

---

## 十二、已实现符文速查表

| ID | 名称 | 触发节点 | 核心模式 | 资产需求 |
|---|---|---|---|---|
| 1001 | 攻击强化 | Start(Passive) | 永久增益 | 零资产 |
| 1002 | 热度提升 | OnPeriodic + OnDamageReceived | 周期+条件 | 零资产 |
| 1003 | 速度叠加 | OnDamageDealt | 叠层增益 | 零资产 |
| 1004 | 击退 | OnDamageDealt | 事件→GA | C++ GA_Knockback |
| 1005 | 流血 | OnDamageDealt | GE伴生GA | C++ GA_Bleed |
| 1006 | 额外伤害 | OnDamageDealt | 递归守卫 | 零资产 |
| 1007 | 击退减速 | WaitGameplayEvent | 联动 | 零资产（依赖1004） |
| 1008 | 刀光波 | OnDamageDealt | 事件→GA→投射物 | GE + GA BP + Projectile BP |
| 1009 | 弱点窥破 | OnDamageDealt | 末击检测 | 零资产 |
| 1010 | 突刺连击 | OnDash + OnDamageDealt | 限时窗口 | 零资产 |
| 1011 | 毒牙 | OnCritHit | 周期伤害 | 零资产(测试) |
| 1012 | 幽风低语 | OnPeriodic | 数据引脚链 | 零资产 |
| 1013 | 震爆 | OnCritHit | 事件→GA(复用) | 零资产 |
| 1014 | 暗影疾驰 | Start(Passive) | 永久增益×2 | 零资产 |
| 1015 | 痛苦契约 | OnHealthChanged | 动态计算 | 零资产 |
| 1016 | 致命先机 | OnDamageDealt | 满血检测 | 零资产 |
| 1017 | 击杀爆炸 | OnDamageDealt | 范围伤害 | GE + GameplayCue |
| 1018 | 生命汲取 | OnDamageDealt | 百分比回血 | 零资产 |
| 1019 | 燃烧印记 | OnDamageDealt | 周期DoT | GE |
| 1020 | 冲天一击 | OnDamageDealt | 垂直击退 | C++ GA |
| 1021 | 余震 | WaitGameplayEvent | 联动(依赖1020) | 零资产 |

---

## 十三、注意事项与常见问题

### 13.1 Cleanup 行为

FA 停止时，所有节点的 `Cleanup()` 自动调用：
- `ApplyAttributeModifier`：移除非 Instant 的 GE + 撤销 GrantedAbilities
- `GrantGA`：撤销已授予的 GA（ClearAbility）
- `AddTag`：移除已添加的 Tag
- `OnPeriodic`：清除定时器
- `PlayNiagara`（`bDestroyWithFlow=true`）：销毁粒子

**结论**：FA 停止 = 所有效果干净撤回，不会有残留。

### 13.2 递归守卫

当 FA 中有"命中造伤→额外造伤"模式时，额外造伤会再次触发 `OnDamageDealt`，形成无限循环。

**解决方案**：用 `ExtraDamageApplied` Tag 守卫：
```
[OnDamageDealt]
  → [HasTag ExtraDamageApplied] → Yes: 跳过
  → No: [AddTag] → [DoDamage] → [RemoveTag]
```

### 13.3 DamageBuff vs DamagePhysical

- DoT / 状态伤害用 `DamageBuff`（绕过护甲、不可击杀、不广播 Damaged）
- 直接攻击伤害用 `DamagePhysical`（护甲吸收、可击杀、广播 Damaged）
- 选错会导致：递归触发（DamagePhysical 广播 Damaged → GA_Wound 再次扣血 → 循环）

### 13.4 Stacking 注意

- `Unique` + `RefreshOnSuccessfulApplication`：同目标只有一个实例，重复施加刷新时间
- `Stackable` + `RemoveSingleStackAndRefreshDuration`：逐层到期衰减
- `None`：每次施加独立创建一个 GE 实例（小心重叠）

### 13.5 数据引脚 vs 固定值

- 没有连线时使用节点上填的固定值
- 有连线时运行时动态读取上游输出
- 连线优先级高于固定值

---

## 附录 A：资产命名规范

| 类型 | 命名 | 路径 | 示例 |
|---|---|---|---|
| 符文 FA | `FA_Rune_<名称>` | `Content/Game/Runes/<名称>/` | FA_Rune_AttackUp |
| 基础效果 FA | `FA_Effect_<效果>` | `Content/BuffFlow/Effects/` | FA_Effect_Poison |
| 符文 DA | `DA_Rune_<名称>` | `Content/Game/Runes/<名称>/` | DA_Rune_AttackUp |
| 效果 DA | `DA_Effect_<效果>` | `Content/Data/StatusEffects/` | DA_Effect_Poison |
| GA Blueprint | `BGA_<名称>` | `Content/AbilitySystem/Abilities/` | BGA_SlashWaveCounter |
| GE Blueprint | `GE_<名称>` | `Content/AbilitySystem/GameplayEffects/` | GE_SlashWaveDamage |

---

## 附录 B：相关文档索引

| 文档 | 内容 |
|---|---|
| [TestRune_CreationGuide](TestRune_CreationGuide.md) | 符文 1001-1016 详细制作指南 |
| [TestRune_HighPerception_Guide](TestRune_HighPerception_Guide.md) | 高感知符文 1017-1021 |
| [StatusEffect_EngineConfig_Guide](../../../04_调研与玩法设计/设计文档/StatusEffects/StatusEffect_EngineConfig_Guide.md) | 10 个基础效果 FA + SubGraph 复用模式 |
| [Rune_System_Design](../../../04_调研与玩法设计/设计文档/Rune/Rune_System_Design.md) | 符文设计框架（4 流派 + 维度分析） |
| [Rune_Archetype_Catalog](../../../04_调研与玩法设计/设计文档/Rune/Rune_Archetype_Catalog.md) | 流派符文目录（概念设计） |
| [BuffFlow_NodeUsageGuide](BuffFlow_NodeUsageGuide.md) | BFNode 详细用法指南 |
| [BackpackSystem_Guide](../../../99_归档/旧方案/2D背包方案/BackpackSystem_Guide.md) | 背包系统配置 |
