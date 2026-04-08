# BuffFlow 架构思路与开发计划

> 项目：星骸降临
> 文档性质：架构设计思路整理 + 开发决策记录 + 完整开发计划
> 更新日期：2026-04-08
> 作者：龚正昂

---

## 一、为什么要做 BuffFlow？

### 1.1 核心问题

Roguelite 类游戏的符文/Buff 系统有一个本质矛盾：

- **策划**需要频繁新增、调整符文效果，要求快速迭代
- **程序**如果把每个符文都写成独立的 C++/Blueprint 类，维护成本随符文数量爆炸性增长
- **UE 原生 GAS**（GameplayAbilitySystem）功能强大，但 GameplayEffect 和 GameplayAbility 都是资产文件，符文越多资产越散、逻辑越难追踪

传统做法（每个符文一个 GA + 一个或多个 GE）的问题：

```
符文 1 → GA_Rune001 + GE_Rune001_Attack
符文 2 → GA_Rune002 + GE_Rune002_Heat + GE_Rune002_Inhibit
符文 3 → GA_Rune003 + ...
...
50 个符文 → 50 个 GA + 100+ 个 GE，逻辑散落在各处
```

**策划无法直接看到"这个符文在什么情况下做了什么"，必须依赖程序解释。**

### 1.2 目标

让符文逻辑**完全可视化**，让策划能够：
1. 不需要打开 C++ 就能看懂每个符文的完整行为
2. 不需要创建新的 GE Blueprint 就能做基础的属性修改类符文
3. 在同一张图里看到：触发条件 → 目标选择 → 效果执行 → Cleanup 的完整流程

### 1.3 技术路线选择

我们选择了 **FlowGraph 插件**（MothCocoon/FlowGraph 2.1-5.4）作为可视化逻辑层，原因：

| 方案 | 优势 | 劣势 |
|------|------|------|
| Blueprint GA/GE | UE 原生，无依赖 | 逻辑散落，节点图不直观 |
| 行为树 | AI 决策成熟方案 | 不适合事件驱动的符文逻辑 |
| **FlowGraph** | 事件驱动、持久状态、数据引脚 | 需要 C++ 扩展节点 |
| 自研节点编辑器 | 完全定制 | 开发成本极高 |

FlowGraph 的关键特性：
- UFlowNode 是 **UObject**（有状态），不是函数调用（无状态），天然支持持久 handle 存储
- 支持**数据引脚**（16 种类型），节点间可以传值
- 内置控制流节点（Timer、Counter、LogicalAND/OR、SubGraph）
- FA 实例生命周期与 BFC 绑定，Cleanup 机制天然支持符文卸下时的清理

---

## 二、整体架构设计

### 2.1 三层职责划分

```
┌─────────────────────────────────────────────────────┐
│                    逻辑层 (FA)                        │
│   Flow Asset — 符文行为的完整可视化描述               │
│   ┌─────────────────────────────────────────────┐   │
│   │  FlowGraph 内置节点  控制流、计时、计数、随机  │   │
│   │  BFNode_*           GAS集成：触发/属性/Tag   │   │
│   │  数据引脚            节点间值传递（16种类型）  │   │
│   └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
                        ↕ 调用
┌─────────────────────────────────────────────────────┐
│                    效果层 (GAS)                       │
│   GameplayEffect  属性计算、Tag授予、Cue             │
│   GameplayAbility 复杂能力（AbilityTask、网络同步）  │
└─────────────────────────────────────────────────────┘
                        ↕ 读取
┌─────────────────────────────────────────────────────┐
│                    配置层 (Data)                      │
│   RuneDataAsset   符文的静态配置（名字/图标/形状/FA）│
│   Blueprint GE    有 Cue/ExecutionCalc 时才需要      │
│   Blueprint GA    有 AbilityTask/网络同步时才需要    │
└─────────────────────────────────────────────────────┘
```

**核心原则：**
- 符文的**所有行为逻辑**在 FA 里可视化，不散落在蓝图或 C++ 里
- GAS 负责"效果执行"，FA 负责"何时/对谁/以什么顺序执行"
- **能用 FA 节点表达的逻辑，不创建额外资产**（ApplyAttributeModifier 是典型）

### 2.2 组件关系图

```
PlayerCharacter
  ├── YogAbilitySystemComponent (ASC)   ← GAS 核心
  ├── BackpackGridComponent (BGC)        ← 背包 + 热度 + FA 生命周期
  └── BuffFlowComponent (BFC)            ← FA 实例管理 + 节点间共享上下文

BGC 职责：
  ├── 管理 5×5 背包格
  ├── 管理热度 Phase（0-3）和激活区大小
  ├── 监测符文是否在激活区内 → Start/Stop FA
  └── 维护 PermanentRunes 永久符文列表

BFC 职责：
  ├── FA 实例的 Map（每个 DA 对应一个 FA 实例）
  ├── 共享上下文（LastEventContext、LastKillLocation、ActiveNiagaraEffects）
  └── 暴露委托（OnBuffFlowStarted、OnBuffFlowStopped）
```

### 2.3 符文从装备到执行的完整流程

```
[策划配置 RuneDataAsset]
  └── RuneName, Shape, RuneConfig, Flow.FlowAsset

[玩家拖拽符文进激活区]
  └── BGC.OnRuneEnterActivationZone()
       └── BFC.StartFlow(RuneDA)
            └── FA 实例被创建
                 └── [Start] 节点触发
                      └── 符文效果开始运行

[玩家拖出激活区 / Phase 收缩]
  └── BGC.OnRuneLeaveActivationZone()
       └── BFC.StopFlow(RuneDA)
            └── FA.FinishFlow()
                 └── 所有 BFNode.Cleanup() 被调用
                      ├── ApplyAttributeModifier → 移除 GE
                      ├── GrantGA → 撤销 GA
                      └── AddTag → 移除 Loose Tag
```

**关键设计：Cleanup 机制保证零遗漏**

每个 BFNode 重写 `Cleanup()` 虚函数，FA 停止时框架自动调用全部活跃节点的 Cleanup。程序只需要在节点内部用 `bFinish=false` 保持活跃，不需要任何额外的注销逻辑。

---

## 三、关键设计决策（及其背后的思考）

### 3.1 决策：为什么用 Dota 的"道具模型"管理 GE Handle？

**背景**：GE 施加后会返回 `FActiveGameplayEffectHandle`，如果想之后移除这个 GE，必须保存这个 handle。

**初始想法**：用 TArray 存所有 handle，以防多次施加时积累。

**最终决策**：每个 BFNode 实例只存**一个** handle，对应一个 GE 实例。

**为什么**：

Dota 的道具模型——每件道具（符文）在数据层面是一个独立实体，有自己唯一的标识。多个符文同时存在时，它们的 GE 之间天然不冲突，因为每个 BFNode 实例属于不同的 FA 实例，FA 实例与 RuneDataAsset 一一对应。

```
符文A (FA_A 实例) → BFNode_ApplyEffect 实例A → GrantedHandle_A
符文B (FA_B 实例) → BFNode_ApplyEffect 实例B → GrantedHandle_B
```

不需要 TArray，不需要 Tag 去区分不同符文的同名 GE。**实例隔离本身就是去重机制**。

唯一例外：同一个符文的同一个 BFNode 被触发多次（如 OnHit 堆叠），此时用 `EBFGEStackMode.Stackable` 让 GAS 管理堆叠数，BFNode 依然只存一个 handle（handle 指向那个 Stackable GE 实例）。

### 3.2 决策：BFNode_ApplyAttributeModifier 替代 Blueprint GE

**背景**：最常见的符文效果是"加攻击力+N"这类简单属性修改。传统做法需要创建 Blueprint GE（`.uasset` 文件）。

**问题**：
- 50个符文 × 每个平均2个属性修改 = 100个 GE 资产文件
- 每次修改数值都要打开 GE 文件
- 策划无法在 FA 里直接看到数值

**解决方案**：`BFNode_ApplyAttributeModifier` ——在运行时用 C++ 动态构建 `FGameplayEffectSpecHandle`，无需任何预先创建的 GE 资产。

```cpp
// 节点上直接填写
Attribute = AttackDamage
ModOp     = Additive
Value     = 10
Duration  = Infinite
Target    = BuffOwner
```

对应内部实现：动态创建 UGameplayEffect CDO，填入 Modifier，通过 ASC 施加。

**权衡**：
- ✅ 零资产文件、策划直接在 FA 节点上改数值
- ✅ Cleanup 时自动移除 GE（handle 保存在节点内）
- ⚠️ 无法使用 ExecutionCalculation（需要 Blueprint GE → 用 BFNode_ApplyEffect）
- ⚠️ 无法使用 GameplayCue（待做 BFNode_PlayGameplayCue）

**结论**：简单属性修改用 ApplyAttributeModifier；需要 Cue/ExecutionCalculation/复杂计算时才用 ApplyEffect + Blueprint GE。

### 3.3 决策：否定了 GEConfigDataAsset

**曾经的想法**：创建一个独立的 `GEConfigDataAsset`，把 GE 的参数（Modifier、Duration、DurationType）存成 DA，让 BFNode 读取 DA 来构建 GE，解决"数值集中管理"的问题。

**为什么否定**：

1. DA 文件本身就是一个资产（.uasset），和 Blueprint GE 资产数量上没有区别
2. 策划仍然需要先创建 DA，再在 FA 里引用 DA，步骤更多而不是更少
3. `BFNode_ApplyAttributeModifier` 已经把数值直接放在节点上，策划在图里就能改——这才是真正的"减少资产"

**结论**：GEConfigDataAsset 是一个**用复杂度换复杂度**的方案，被彻底放弃。

### 3.4 决策：GA 的哪些逻辑可以 FA 化？

**能 FA 化（做成 BFNode）的 GA 功能**：
- 属性修改 → `BFNode_ApplyAttributeModifier`
- Tag 授予/移除 → `BFNode_AddTag` / `BFNode_RemoveTag`
- 简单的 GE 施加 → `BFNode_ApplyEffect`
- GA 授予与撤销 → `BFNode_GrantGA`
- 伤害输出 → `BFNode_DoDamage`
- 特效播放 → `BFNode_PlayNiagara`

**不适合 FA 化，保留 Blueprint GA 的情况**：
- 有 `AbilityTask`（如 WaitGameplayEvent、PlayMontageAndWait）
- 需要网络同步（Server/Client RPC）
- 复杂的命中判定（PhysicsTrace 等）

**原则**：简单的"施加一个效果"用 FA 节点；复杂的"执行一段能力流程"保留 Blueprint GA，然后用 `BFNode_GrantGA` 从 FA 里启动它。

### 3.5 决策：热度/Phase 系统放在 BGC 里，而不是独立系统

**理由**：
- Phase 直接决定激活区大小（BGC 的核心职责之一）
- Phase 变化需要重新计算哪些格子的符文处于激活状态
- 把 Phase 放在 BGC 里，激活区 resize 和符文 Start/Stop 可以在同一个地方原子完成

激活区随 Phase 扩展规则：
```
Phase 0 → 中心 1×1 格
Phase 1 → 中心 2×2 格
Phase 2 → 中心 4×4 格
Phase 3 → 全部 5×5 格
```

---

## 四、数据流与节点间通信

### 4.1 三种数据共享机制

**机制 1：数据引脚连线**（首选，最直观）
```
[GetRuneInfo] → Level (Float) → [ApplyEffect] Level
```
适用于：相邻节点间传递计算结果或查询值。

**机制 2：BFC 共享上下文**（跨节点事件数据）
```cpp
// BFC 上的字段，由触发节点写入，由效果节点读取
FBFEventContext LastEventContext;
  ├── DamageCauser   (攻击者)
  ├── DamageReceiver (被击者)
  └── DamageAmount   (伤害量)

FVector LastKillLocation;           // 击杀位置
TMap<FName, AActor*> ActiveNiagaraEffects; // 活跃特效
```

**机制 3：BFNode 成员变量**（节点自身的持久状态）
```cpp
// 节点内部，跨帧保存
FActiveGameplayEffectHandle GrantedHandle;  // GE handle
TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
FGameplayAbilitySpecHandle GAHandle;        // GA handle
```

**VarContainer 的立场**：虽然 FlowGraph 的 UI 里显示了 Var Container，但其 C++ API 不在我们当前的插件版本（2.1-5.4）里。现有三种机制已完全覆盖所有数据共享场景，暂不引入 VarContainer。

### 4.2 目标选择器（EBFTargetSelector）

所有效果节点都通过统一的 `EBFTargetSelector` 决定对谁施加效果：

| 枚举值 | 含义 | 典型用途 |
|--------|------|---------|
| `BuffOwner` | 符文拥有者（玩家） | **被动符文必须用这个** |
| `BuffGiver` | 符文施加者（通常同上） | 特殊 PvP 场景 |
| `LastDamageTarget` | 上次伤害的目标（敌人） | 给被击者施 Debuff |
| `DamageCauser` | 伤害来源（攻击者） | 反弹效果 |

**⚠️ 常见错误**：被动常驻符文（无伤害事件）把 Target 设为 `LastDamageTarget`，导致 `Failed` 输出——因为没有伤害事件时 `LastDamageTarget` 为空。**被动符文必须用 `BuffOwner`**。

---

## 五、已实现节点清单（35个）

### 5.1 触发节点（10个）

| 节点 | 触发条件 | 关键参数 |
|------|---------|---------|
| `BFNode_OnDamageDealt` | 造成伤害 | `bOncePerSwing` 防 AOE 多触发 |
| `BFNode_OnDamageReceived` | 受到伤害 | 写入 BFC.LastEventContext |
| `BFNode_OnCritHit` | 触发暴击 | — |
| `BFNode_OnKill` | 击杀时 | 写入 BFC.LastKillLocation |
| `BFNode_OnDash` | 闪避/冲刺 | — |
| `BFNode_OnBuffAdded` | 任意 FA 启动 | 监听 BFC.OnBuffFlowStarted |
| `BFNode_OnBuffRemoved` | 任意 FA 停止 | 监听 BFC.OnBuffFlowStopped |
| `BFNode_OnPhaseUpReady` | 热度满 + CanPhaseUp Tag | 升阶流程入口 |
| `BFNode_OnHeatReachedZero` | 热度边沿：>0→0 或 0→>0 | 双向检测，含 Phase>0 守卫 |
| `BFNode_OnPeriodic` | 定时重复 | `Interval` + `bFireImmediately` |

### 5.2 条件与查询节点（6个）

| 节点 | 功能 | 输出 |
|------|------|------|
| `BFNode_HasTag` | 目标是否有指定 Tag | True / False |
| `BFNode_CompareFloat` | 浮点比较（>, >=, ==, <=, <, !=） | True / False |
| `BFNode_CompareInt` | 整数比较 | True / False |
| `BFNode_CheckTargetType` | LastDamageTarget 身份判断 | Self / Enemy / Other |
| `BFNode_GetAttribute` | 读取目标属性值 | `CachedValue (Float)` 数据引脚 |
| `BFNode_GetRuneInfo` | 通过 Tag 查询 GE 状态 | Found/NotFound + bIsActive, StackCount, Level, TimeRemaining |

### 5.3 效果节点（13个）

| 节点 | 功能 | Cleanup |
|------|------|---------|
| `BFNode_ApplyAttributeModifier` | 零资产属性修改（核心节点） | 自动移除 GE |
| `BFNode_ApplyEffect` | 施加 Blueprint GE，含 Remove 引脚 | 自动移除 GE |
| `BFNode_GrantGA` | 授予 GA | 自动撤销 GA |
| `BFNode_AddTag` | 添加 Loose Tag | 自动移除 Tag |
| `BFNode_RemoveTag` | 移除 Tag | — |
| `BFNode_DoDamage` | 造成伤害（Flat 或 LastAmount×倍率） | — |
| `BFNode_PlayNiagara` | 播放粒子特效 | 自动销毁 |
| `BFNode_DestroyNiagara` | 销毁指定特效 | — |
| `BFNode_PlayMontage` | 播放动画蒙太奇 | — |
| `BFNode_SpawnActorAtLocation` | 在 LastKillLocation 生成 Actor | — |
| `BFNode_FinishBuff` | 终止整个 FA | — |

### 5.4 计算节点（2个）

| 节点 | 功能 |
|------|------|
| `BFNode_MathFloat` | 浮点运算（+、-、×、÷） |
| `BFNode_MathInt` | 整数运算 |

### 5.5 热度系统节点（3个）

| 节点 | 功能 |
|------|------|
| `BFNode_IncrementPhase` | Phase++，扩大激活区，授予 Phase Tag |
| `BFNode_DecrementPhase` | Phase--（含 Phase>0 守卫），收缩激活区 |
| `BFNode_PhaseDecayTimer` | 防重复激活计时器，含 Cancel Pin |

### 5.6 可免费使用的 FlowGraph 内置节点

这些是插件自带的，无需任何 C++ 开发，直接在 FA 里使用：

| 节点 | 功能 | 我们的使用场景 |
|------|------|--------------|
| `FlowNode_ExecutionSequence` | 依次触发多个输出 | 升阶时依次加属性→播特效 |
| `FlowNode_ExecutionMultiGate` | 顺序/随机触发，可循环 | 随机符文效果 |
| `FlowNode_LogicalAND` | 等待所有输入都触发 | 多事件同步触发 |
| `FlowNode_LogicalOR` | 任一输入触发即输出 | OnKill OR OnCritHit |
| `FlowNode_Timer` | 倒计时（Step/Completed） | 通用计时（勿用于热度衰减，有 BFNode） |
| `FlowNode_Counter` | 计数到 N 后触发 | 击杀 5 次后爆发 |
| `FlowNode_DefineProperties` | 定义字面量数据输出 | 固定数值常量 |
| `FlowNode_Log` | 打印调试信息 | 开发期快速排查 |
| `FlowNode_SubGraph` | 引用另一个 FA 作为子图 | 复用符文逻辑片段 |
| `FlowNode_Branch` | 条件分支（配合 AddOn 谓词） | 复杂多条件判断 |

---

## 六、符文设计模式（11种）

### 模式 A：被动属性加成（最常用）
```
[Start] → [ApplyAttributeModifier: Attack+10, Infinite, BuffOwner]
```
**适用**：攻击强化、速度提升、暴击率等所有常驻被动。

### 模式 B：条件触发（击中/击杀触发效果）
```
[Start] → [OnKill] → [HasTag: Phase.2]
                         True  → [ApplyAttributeModifier: Attack+20]
                         False → [ApplyAttributeModifier: Attack+10]
```
**适用**：需要判断玩家状态来决定效果强度的符文。

### 模式 C：堆叠型（命中叠加，达到上限后维持）
```
[Start] → [OnDamageDealt] → [ApplyAttributeModifier: Speed+5, 3s, Stackable, Max=5]
```
**注意**：Stackable 模式下 GAS 管理堆叠层数，BFNode 只存一个 handle。

### 模式 D：一次性加成（触发后用完即撤）
```
[Start] → [OnKill] → [ApplyEffect: GE_AttackBoost]
[OnDamageDealt] → [ApplyEffect.Remove]
```

### 模式 E：周期性效果（每秒触发）
```
GE_HeatTick: Infinite, Period=1.0s, Modifier: Heat+1
[Start] → [ApplyEffect: GE_HeatTick]
```
**说明**：Period 字段让 GAS 自动每秒触发一次 Modifier 计算，是实现"每秒+N热度"的正确方案。

### 模式 F：受伤暂停（OngoingTagRequirements 自动 Inhibit）
```
GE_HeatTick:
  OngoingTagRequirements.IgnoreTags = Buff.Status.HeatInhibit

GE_HeatInhibit: HasDuration=5s, GrantedTags: Buff.Status.HeatInhibit

[Start] → [ApplyEffect: GE_HeatTick]
[OnDamageReceived] → [ApplyEffect: GE_HeatInhibit]
```
**机制**：当 `Buff.Status.HeatInhibit` 存在时，GAS 自动暂停 GE_HeatTick 的 Period 执行。5秒后 HeatInhibit GE 自然结束，HeatTick 恢复。**无需任何 FA 节点参与控制**。

### 模式 G：递归守卫（防止触发自身形成无限循环）
```
[OnDamageDealt]
  → [HasTag: Buff.Status.ExtraDamageApplied, DamageCauser]
        False → [AddTag: ExtraDamageApplied]
                    → [DoDamage: ×0.5]
                         → [RemoveTag: ExtraDamageApplied]
        True  → （跳过）
```
**场景**：附加伤害符文触发伤害事件，伤害事件又触发附加伤害，形成死循环。用一个状态 Tag 作为守卫。

### 模式 H：计数型（N次触发后爆发）
```
[Start] → [OnKill]
               → [FlowNode_Counter: Goal=5]
                      Goal → [ApplyAttributeModifier: Attack+100, 10s]
```

### 模式 I：随机效果
```
[Start] → [OnKill]
               → [FlowNode_ExecutionMultiGate: Random=true]
                      Output0 → [ApplyAttributeModifier: Attack+15]
                      Output1 → [ApplyAttributeModifier: MoveSpeed+100]
                      Output2 → [ApplyAttributeModifier: CritRate+0.1]
```

### 模式 J：多事件同步（AND 门）
```
[OnKill]    ─→ [FlowNode_LogicalAND]
[OnCritHit] ─→                      ─→ [ApplyAttributeModifier: 大额加成]
```
**两个事件都发生了，才触发效果**（顺序无关）。

### 模式 K：SubGraph 复用（共享逻辑片段）
```
FA_Util_PhaseBonus（子图）:
  [CustomInput: In] → [HasTag: Phase.2]
                           True  → [ApplyAttributeModifier: +20]
                           False → [ApplyAttributeModifier: +10]
                       → [CustomOutput: Done]

任意符文 FA：
  [OnKill] → [SubGraph: FA_Util_PhaseBonus] → Done → [下一步]
  [OnHit]  → [SubGraph: FA_Util_PhaseBonus] → Done → [下一步]
```

---

## 七、GAS 功能的 FA 覆盖情况

| GAS 功能 | FA 支持情况 | 实现方式 |
|---------|-----------|---------|
| 属性修改（Modifier） | ✅ 完全，零资产 | `BFNode_ApplyAttributeModifier` |
| Tag 授予/移除 | ✅ 完全 | `BFNode_AddTag` / `RemoveTag`，Cleanup 自动 |
| Tag 条件检查 | ✅ 完全 | `BFNode_HasTag` |
| Stackable GE | ✅ 完全 | `BFNode_ApplyEffect` + GAS 管理堆叠 |
| SetByCaller | ✅ 完全 | `BFNode_ApplyEffect` 的 3 个 SetByCaller 槽 |
| ExecutionCalculation | ✅ 通过 Blueprint GE | `BFNode_ApplyEffect` 引用含 Exec 的 GE |
| GA 授予/撤销 | ✅ 完全 | `BFNode_GrantGA`，Cleanup 自动撤销 |
| Immunity（免疫） | ✅ FA 等价 | `BFNode_HasTag(Shielded) → 跳过伤害 GE` |
| Conditional Effects | ✅ FA 等价 | `HasTag(Wet) → ApplyEffect(Electro)` |
| Periodic GE（每秒触发） | ✅ 通过 GE Period 字段 | GE_HeatTick: Period=1.0s |
| OngoingTagRequirements | ✅ 通过 Blueprint GE 配置 | 自动 Inhibit，无需 FA 节点参与 |
| GameplayCue（触发） | ⏳ 待做 | `BFNode_PlayGameplayCue`（P1） |
| GA 激活（触发已有 GA）| ⏳ 待做 | `BFNode_ActivateAbilityByTag`（P2） |
| Gameplay Event | ⏳ 待做 | `BFNode_SendGameplayEvent`（P2） |
| AbilityTask（复杂） | ❌ 不适合 FA | 保留 Blueprint GA |
| 网络同步 Replication | ❌ 不适合 FA | 保留 Blueprint GA + Replication |

---

## 八、运行时数据流（一次完整的符文触发）

以"速度叠加符文（模式C）"为例，追踪一次 OnHit 触发的完整数据流：

```
1. 玩家普攻命中敌人
   └── C++ DamageSystem 广播 OnDamageDealt 事件
        └── BFC.OnDamageDealt(Caster, Target, Amount)
             └── 写入 BFC.LastEventContext:
                   DamageCauser = PlayerActor
                   DamageReceiver = EnemyActor
                   DamageAmount = 45.0f

2. FA 中的 BFNode_OnDamageDealt 接收到事件
   └── ExecuteInput("Out")
        └── TriggerOutput("Out", false) → 执行下一个节点

3. BFNode_ApplyAttributeModifier 执行
   ├── ResolveTarget(LastDamageTarget) → EnemyActor
   │     └── 从 BFC.LastEventContext.DamageReceiver 获取
   ├── 构建临时 GE: Attribute=MoveSpeed, Additive, +5, Duration=3s, Stackable
   ├── ASC.ApplyGameplayEffectSpecToSelf(Spec)
   │     └── GAS 检查：是否已有 Stackable GE？
   │          是 → 堆叠层数 + 1，刷新持续时间
   │          否 → 创建新 GE 实例
   └── 保存 GrantedHandle（首次施加时）

4. GE 激活后
   └── EnemyActor.MoveSpeed 属性值变化 → 移速降低（Debuff 场景）
       或 PlayerActor.MoveSpeed 变化（Buff 场景，需改 Target=BuffOwner）

5. 符文卸下时
   └── FA.FinishFlow()
        └── BFNode_ApplyAttributeModifier.Cleanup()
             └── GrantedASC.RemoveActiveGameplayEffect(GrantedHandle)
                  └── EnemyActor.MoveSpeed 恢复原值
```

---

## 九、已完成的符文测试状态

| 符文 ID | 名称 | 触发 | 关键技术 | 测试状态 |
|---------|------|------|---------|---------|
| 1001 | 攻击强化 | Passive | `BFNode_ApplyAttributeModifier` | ✅ 通过 |
| 1002 | 热度提升 | Passive | Periodic GE + OngoingTagRequirements | ⬜ 待开发 |
| 1003 | 速度叠加 | OnHit | Stackable GE | ⬜ 待测试 |
| 1004 | 击退 | OnHit | SetByCaller + GA | ⬜ 待测试 |
| 1005 | 流血 | OnHit | `BFNode_ApplyEffect` + `BFNode_GrantGA` | ⬜ 待测试 |
| 1006 | 额外伤害 | OnDamageDealt | 递归守卫（模式G） | ⬜ 待测试 |

---

## 十、开发计划

### P0 — 符文 2（热度提升）所需节点

**目标**：实现"每秒 +1 热度，受伤后暂停 5 秒"符文。

#### P0-1：BFNode_GrantTag

**需求**：临时授予角色一个 Loose Tag，FA 停止时自动移除。不同于 AddTag（永久授予），GrantTag 有持续时间或直到 FA 结束才移除。

**用途**：原型上，可以替代简单的 GE_HeatInhibit 临时授予（但模式 F 的 OngoingTagRequirements 方案实际上不需要 GrantTag 节点，用 Blueprint GE 更稳）。

**接口设计**：
```
输入引脚：In / Remove
配置：
  Tag           GameplayTag
  DurationType  Instant / Infinite / HasDuration
  Duration      float（HasDuration 时）
  Target        EBFTargetSelector
输出引脚：Out / Removed
Cleanup：自动移除 Tag
```

#### P0-2：BFNode_ApplyAttributeModifier 加 Period 支持

**需求**：实现"每秒 +N 热度"——即属性修改每隔 N 秒执行一次。

**设计方案**：在节点上增加 `Period` 字段（默认 0 = 不启用）。当 Period > 0 时，动态构建的 GE 设置 `Period = this.Period`，让 GAS 自动周期性执行 Modifier。

**注意**：Period 模式下 GE 的 DurationType 必须是 Infinite（否则 Period 无意义）。

```
新增字段：
  Period   float  > 0 时启用周期执行，单位：秒
```

#### P0-3：BFNode_Delay

**需求**：等待 N 秒后继续执行（非阻塞，基于 Timer）。

**用途**：
- 触发效果后延迟清理
- 序列效果的时序控制
- 配合 SubGraph 做"先等待再执行"逻辑

**接口设计**：
```
输入引脚：In / Cancel
配置：
  Duration  float（秒）
输出引脚：Completed / Cancelled
```

---

### P1 — 框架完善

#### P1-1：BFNode_PlayGameplayCue

**需求**：从 FA 里触发 Gameplay Cue，而不绑定到 GE 生命周期。

**用途**：升阶特效、击杀音效、技能触发反馈——这些特效不需要跟随 GE 持续，只是一次性播放。

**接口设计**：
```
输入引脚：In
配置：
  CueTag    FGameplayTag（Gameplay Cue 标签）
  Target    EBFTargetSelector
输出引脚：Out
```

#### P1-2：BFNode_Base 开放 Blueprint 继承

**目标**：让策划/TD 可以在编辑器里创建简单的 Flow Node，无需 C++。

**实现**：把 `BFNode_Base` 改为 `Blueprintable`，暴露以下 BlueprintCallable 接口：
```cpp
UFUNCTION(BlueprintCallable)
void BPTriggerOutput(FName PinName, bool bFinish);

UFUNCTION(BlueprintCallable)
UYogAbilitySystemComponent* GetOwnerASC() const;

UFUNCTION(BlueprintCallable)
AActor* BPResolveTarget(EBFTargetSelector Selector) const;
```

**限制**：Blueprint 节点性能略低于 C++，适合原型验证，性能敏感路径仍用 C++。

#### P1-3：GameplayTag 数据引脚

**目标**：让 `BFNode_HasTag`、`BFNode_ApplyEffect` 的 Tag 参数可以从上游节点动态传入。

**价值**：支持"动态选择 Tag"类符文，比如根据玩家当前阶段选择不同的 Tag 进行检查。

---

### P2 — 战斗能力扩展

#### P2-1：BFNode_ActivateAbilityByTag

**用途**：触发目标角色已授予的 GA（通过 AbilityTag 寻址）。

```
配置：
  AbilityTag   FGameplayTag（GA 的 AbilityTags）
  Target       EBFTargetSelector
```

#### P2-2：BFNode_SendGameplayEvent

**用途**：向目标发送 Gameplay Event，配合 GA 的 WaitGameplayEvent 节点。

```
配置：
  EventTag     FGameplayTag
  EventData    FGameplayEventData（可选）
  Target       EBFTargetSelector
```

---

### P3 — 体验完善

| 任务 | 说明 | 依赖 |
|------|------|------|
| 热度 UI | 显示当前 Phase，激活区视觉反馈 | 无 |
| 升阶特效/音效 | BFNode_PlayGameplayCue 完成后接入 | P1-1 |
| CanPhaseUp 检查移入 FA | 当前在 C++ FNode 内部，移到 FA 用 HasTag 节点更透明 | 无 |
| Phase Tag 互斥保证 | 确保升/降阶时 Phase Tag 始终只有一个 | 无 |

---

## 十一、关键文件索引

### C++ 源码

| 路径 | 说明 |
|------|------|
| [Source/DevKit/Public/BuffFlow/Nodes/BFNode_Base.h](../../Source/DevKit/Public/BuffFlow/Nodes/BFNode_Base.h) | 所有 BFNode 的基类，提供 GetOwnerASC / ResolveTarget |
| [Source/DevKit/Public/BuffFlow/BuffFlowTypes.h](../../Source/DevKit/Public/BuffFlow/BuffFlowTypes.h) | 枚举定义：EBFTargetSelector / EBFGEStackMode / EBFRemoveMode |
| [Source/DevKit/Public/BuffFlow/BuffFlowComponent.h](../../Source/DevKit/Public/BuffFlow/BuffFlowComponent.h) | FA 实例管理 + 共享上下文 |
| [Source/DevKit/Public/Component/BackpackGridComponent.h](../../Source/DevKit/Public/Component/BackpackGridComponent.h) | 背包格子、热度 Phase、激活区管理 |
| [Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h](../../Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h) | 核心节点：零资产属性修改 |
| [Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h](../../Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h) | Blueprint GE 施加，含 Remove 引脚 |

### 设计文档

| 文档 | 说明 |
|------|------|
| [BuffFlow_SystemGuide.md](BuffFlow_SystemGuide.md) | 完整系统使用手册（节点参考 + 开发流程） |
| [Buff_Tag_Spec.md](Buff_Tag_Spec.md) | Tag 命名规范与四层职责模型 |
| [HeatSystem_Design.md](HeatSystem_Design.md) | 热度系统详细设计 |
| [LevelSystem_ProgrammerDoc.md](LevelSystem_ProgrammerDoc.md) | 关卡系统接入点 |

### 已创建 FA 资产

| 资产路径 | 说明 |
|---------|------|
| `Content/BuffFlow/FlowAssets/FA_Rune_HeatPhase` | 热度升降阶（永久符文） |
| `Content/BuffFlow/FlowAssets/FA_Rune_HeatOnHit` | 命中加热 |
| `Content/BuffFlow/FlowAssets/FA_Rune_AttackUp` | 攻击强化（已测试） |

---

## 十二、已否定的设计（避免重复踩坑）

### 否定 1：GEConfigDataAsset

**曾经的想法**：创建独立的 DA 文件，把 GE 参数（Modifier、Duration 等）以数据形式存储，让 BFNode 读取 DA 而不是 Blueprint GE。

**否定原因**：
- DA 本身就是一个 .uasset 文件，数量上没有减少
- 步骤比 ApplyAttributeModifier 更多（需要先建 DA 再引用）
- `BFNode_ApplyAttributeModifier` 已经实现了"节点上直接填参数"，完全不需要额外 DA

**结论**：彻底放弃。简单属性修改用 ApplyAttributeModifier，复杂效果用 Blueprint GE + ApplyEffect。

### 否定 2：用 TArray 存所有 GE Handle

**曾经的想法**：担心单个节点多次触发导致 handle 积累，所以用 TArray 存所有 handle。

**否定原因**：
- 每个 BFNode 实例属于特定 FA 实例，FA 实例与 RuneDataAsset 一一对应
- 不同符文的同名效果天然隔离（各自的 BFNode 实例持有各自的 handle）
- 同一符文多次触发的情况用 GAS 的 Stackable/Unique 机制处理，BFNode 层面一个 handle 就够
- TArray 增加内存和维护成本，没有实际收益

**结论**：每个 BFNode 实例最多一个 handle，Stackable GE 的多层由 GAS 管理。

### 否定 3：VarContainer 作为节点间数据共享方案

**曾经的想法**：FlowGraph 似乎有 VarContainer（类似黑板），可以作为节点间共享状态的容器。

**否定原因**：
- 当前插件版本（2.1-5.4）的 FlowAsset.h 中没有 VarContainer 的 C++ API
- 现有数据引脚 + BFC.LastEventContext + BFNode 成员变量三套机制已完全覆盖所有需求
- 引入未验证的 API 增加风险

**结论**：暂不使用 VarContainer。如果未来升级插件版本并确认 API，可以重新评估。

---

## 十三、快速开发检查清单

### 新符文开发流程

```
Step 1：明确设计
□ 触发时机是什么？（Passive / OnHit / OnKill / OnDamageReceived / ...）
□ 效果目标是谁？（BuffOwner / LastDamageTarget）
□ 持续多久？（Instant / Infinite / HasDuration）
□ 需要堆叠吗？（None / Unique / Stackable）
□ 需要 Blueprint GE 吗？（有 Cue / ExecutionCalculation / SetByCaller 才需要）
□ 需要 Blueprint GA 吗？（有 AbilityTask / 网络同步才需要）

Step 2：创建 FA
□ 路径：Content/BuffFlow/FlowAssets/
□ 命名：FA_Rune_<功能名>
□ 参考对应的设计模式（A-K）

Step 3：创建 DA
□ 路径：Content/BuffFlow/RuneData/
□ 命名：DA_Rune_<功能名>
□ 必填：RuneName, RuneID, RuneType, Shape.Cells, Flow.FlowAsset

Step 4：测试
□ 拖入背包激活区 → 效果是否生效？
□ GAS Debugger 验证属性变化和 Tag 状态
□ 移出激活区 → Cleanup 是否正确（GE/Tag 是否移除）？
□ 边界情况：被动符文 Target 是否设为 BuffOwner？
```

### 调试速查

| 现象 | 首先检查 |
|------|---------|
| 被动符文没效果 | Target 是否设为 BuffOwner（不是 LastDamageTarget）|
| GE 没效果 | GAS Debugger → 属性当前值 / Active GE 列表 |
| FA 停后 GE 未移除 | 节点的 TriggerOutput 是否用 `bFinish=false` |
| Stackable GE 不叠加 | CachedGE / GrantedHandle 是否在多个实例间冲突 |
| PassThroughOwnerTags 无效 | `.cpp` 是否 `#include YogAbilitySystemComponent.h` |
| OnPeriodic 不触发 | bFireImmediately 是否设置，Interval 值是否合理 |

---

*本文档记录了 BuffFlow 系统从设计讨论到当前实现的完整思路，包括被否定的方案及其原因。目的是保证后续迭代时不重复走弯路，并让架构决策有迹可查。*
