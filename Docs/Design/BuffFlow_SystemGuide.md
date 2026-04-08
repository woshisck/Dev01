# BuffFlow 系统指南 v3

> 项目：星骸降临
> 版本：Sprint 4.15（2026-04-08）
> 关联文档：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · [HeatSystem_Design.md](HeatSystem_Design.md)

---

## 一、系统定位

BuffFlow 是项目专属的**符文逻辑可视化框架**，构建在 FlowGraph 2.1 插件之上，结合 GAS 实现符文效果的全部行为。

**三层职责模型：**

```
FA（Flow Asset）= 逻辑层
  ├── FlowGraph 内置节点  控制流、计时、计数、随机、条件同步
  ├── 我们的 BFNode_*    GAS 集成：触发/属性修改/Tag/GE/GA
  └── 数据引脚           节点间值传递（16种类型）

GAS / GE / GA = 效果层
  └── 属性计算、Tag授予、Cue、能力激活

数据 = 配置层
  └── RuneDataAsset / Blueprint GE（仅复杂场景）/ Blueprint GA（仅复杂场景）
```

**核心设计原则：**
- 符文的**所有行为逻辑**在 FA 里可视化，不散落在蓝图或 C++ 里
- GAS 负责"效果执行"，FA 负责"何时/对谁/以什么顺序执行"
- **能用 BFNode_ApplyAttributeModifier 的，不创建 Blueprint GE 资产**
- Blueprint GE 仅在需要 ExecutionCalculation / Cue / SetByCaller 时才创建
- Blueprint GA 仅在需要 AbilityTask / 网络同步 / 物理冲量时才创建

---

## 二、核心组件

### 2.1 RuneDataAsset（符文配置资产）

```
URuneDataAsset
  └─ FRuneInstance RuneTemplate
       ├─ FName         RuneName
       ├─ UTexture2D*   RuneIcon
       ├─ FText         RuneDescription
       ├─ FRuneShape    Shape             背包格子（FIntPoint 数组）
       ├─ FRuneConfig   RuneConfig
       │    ├─ ERuneType  RuneType        Buff/Debuff/None（UI分类）
       │    └─ int32      RuneID          策划表ID
       └─ FRuneFlowConfig Flow
            └─ UFlowAsset* FlowAsset     激活时启动的 FA
```

---

### 2.2 BackpackGridComponent（BGC）

**职责：** 管理 5×5 背包格、热度阶段（Phase 0–3）、激活区大小，根据符文是否在激活区内自动 Start/Stop FA。

**关键委托：**

| 委托 | 触发条件 | 对应 BFNode |
|------|---------|------------|
| `OnPhaseUpReady` | 热度满 + CanPhaseUp Tag | `BFNode_OnPhaseUpReady` |
| `OnHeatReachedZero` | 热度 >0 → 0（Phase>0守卫） | `BFNode_OnHeatReachedZero` |
| `OnHeatAboveZero` | 热度 ≤0 → >0 | `BFNode_OnHeatReachedZero` |

**激活区随 Phase 扩展：**
```
Phase 0 → 中心 1×1    Phase 1 → 中心 2×2
Phase 2 → 中心 4×4    Phase 3 → 全格
```

**永久符文：** BGC.PermanentRunes 数组，`BeginPlay` 自动激活，不占格子，不受激活区限制。

---

### 2.3 BuffFlowComponent（BFC）

**职责：** FA 实例生命周期管理 + 节点间共享数据通道。

**共享上下文（节点间数据传递的桥梁）：**

| 字段 | 写入节点 | 读取场景 |
|------|---------|---------|
| `LastEventContext.DamageCauser` | OnDamageDealt | Target=DamageCauser |
| `LastEventContext.DamageReceiver` | OnDamageDealt | Target=LastDamageTarget |
| `LastEventContext.DamageAmount` | OnDamageDealt | DoDamage 倍率计算 |
| `LastKillLocation` | OnKill | SpawnActorAtLocation |
| `ActiveNiagaraEffects` | PlayNiagara | DestroyNiagara |

---

## 三、FlowGraph 内置节点（免费获得，直接可用）

### 3.1 控制流

| 节点 | 功能 | 符文用途示例 |
|------|------|------------|
| `FlowNode_ExecutionSequence` | 依次触发多个输出（顺序保证） | 升阶时依次：加属性→播特效→显示UI |
| `FlowNode_ExecutionMultiGate` | 顺序或随机触发输出，支持循环 | 随机符文效果：每次击杀随机加攻/防/速 |
| `FlowNode_LogicalAND` | 等待所有输入都触发后才输出 | "同时满足：热度≥50 AND 连击末帧" |
| `FlowNode_LogicalOR` | 任一输入触发即输出（可限次数） | OnKill OR OnComboEnd 都触发同一效果 |

### 3.2 计时与计数

| 节点 | 功能 | 符文用途示例 |
|------|------|------------|
| `FlowNode_Timer` | Step（周期）+ Completed（结束）双输出 | 通用倒计时（与 BFNode_Delay 二选一） |
| `FlowNode_Counter` | 计数到 Goal 后触发输出，含存档支持 | "击杀 5 次后触发爆发效果" |

### 3.3 数据定义

| 节点 | 功能 | 符文用途示例 |
|------|------|------------|
| `FlowNode_DefineProperties` | 定义多个命名数据引脚输出（通用字面量） | 一次性输出多个不同类型的常量 |
| `FlowNode_Log` | 打印调试信息到 Log/屏幕 | 开发期快速排查 FA 执行路径 |

### 3.4 图结构

| 节点 | 功能 | 说明 |
|------|------|------|
| `FlowNode_SubGraph` | 引用另一个 FA 作为子图执行 | 通过 CustomInput/Output 传递数据 |
| `FlowNode_CustomInput` | 子图的自定义入口 | 在被引用的 FA 里定义 |
| `FlowNode_CustomOutput` | 子图的自定义出口 | 触发父图继续执行 |
| `FlowNode_Reroute` | 连线重定向（纯视觉整理） | 复杂图的线路组织 |

### 3.5 Branch + AddOn 谓词系统

`FlowNode_Branch` 本身不含条件——条件逻辑来自**AddOn 子节点**：

| AddOn | 功能 |
|-------|------|
| `FlowNodeAddOn_PredicateAND` | 所有子谓词都为真 |
| `FlowNodeAddOn_PredicateOR` | 任一子谓词为真 |
| `FlowNodeAddOn_PredicateNOT` | 取反 |

---

## 四、BFNode 节点全览

### 4.1 触发节点

| 节点 | 触发条件 | 输出 Pin | 说明 |
|------|---------|---------|------|
| `BFNode_OnDamageDealt` | 造成伤害 | Out | `bOncePerSwing` 防 AOE 多次触发 |
| `BFNode_OnDamageReceived` | 受到伤害 | Out | 写入 LastEventContext |
| `BFNode_OnCritHit` | 触发暴击 | Out | |
| `BFNode_OnKill` | 击杀时 | Out | 写入 LastKillLocation |
| `BFNode_OnDash` | 闪避/冲刺 | Out | |
| `BFNode_OnBuffAdded` | 任意 FA 启动 | Out | 监听 BFC.OnBuffFlowStarted |
| `BFNode_OnBuffRemoved` | 任意 FA 停止 | Out | 监听 BFC.OnBuffFlowStopped |
| `BFNode_OnPhaseUpReady` | 热度满 + LastHit | OnPhaseUp | 升阶流程入口 |
| `BFNode_OnHeatReachedZero` | 热度边沿变化 | OnReachedZero / OnAboveZero | 双向检测，含 Phase>0 守卫 |
| `BFNode_OnPeriodic` | 定时重复 | Tick | `Interval` 配置，`bFireImmediately` 控制首次 |

---

### 4.2 条件与数据节点

| 节点 | 功能 | 输出 |
|------|------|------|
| `BFNode_HasTag` | 目标是否有指定 Tag | True / False |
| `BFNode_CompareFloat` | 浮点比较（>, >=, ==, <=, <, !=） | True / False |
| `BFNode_CompareInt` | 整数比较 | True / False |
| `BFNode_CheckTargetType` | LastDamageTarget 身份判断 | Self / Enemy / Other |
| `BFNode_GetAttribute` | 读取目标属性值 | `CachedValue (Float)` 数据引脚 |
| `BFNode_GetRuneInfo` | 通过 Tag 查询 GE 状态 | Found/NotFound + bIsActive, StackCount, Level, TimeRemaining |

---

### 4.3 效果节点

#### BFNode_ApplyAttributeModifier（核心节点，零资产）

无需任何 Blueprint GE 资产——直接在节点上填写，运行时动态构建 GE。

| 字段 | 说明 |
|------|------|
| Attribute | 属性（下拉选择） |
| ModOp | Additive / Multiplicative / Override |
| Value | 数值（可连数据引脚覆盖） |
| DurationType | Instant / Infinite / HasDuration |
| Duration | 持续秒数（HasDuration 时） |
| **Period (0=Off)** | **> 0 时每隔 N 秒执行一次 Modifier（每秒+1热度用此字段）** |
| **Fire Immediately** | **Period > 0 时，是否在施加瞬间立即执行一次** |
| Target | 目标选择器 |
| StackMode | None / Unique / Stackable |
| StackLimitCount | 最大堆叠层数（Stackable） |
| DynamicAssetTags | 附加到 GE Spec 的 Tag |
| PassThroughOwnerTags | 将 Owner 当前 Tag 透传入 GE Spec |

**Period 使用说明：**
- `Period = 0`：普通持续 GE，Modifier 施加后一直生效
- `Period = 1.0, DurationType = Infinite`：每秒执行一次 Modifier（"每秒+N热度"的正确方案）
- GAS 的 `OngoingTagRequirements.IgnoreTags` 可配合 Blueprint GE 实现自动 Inhibit；Period GE 本身用此节点不需要 Blueprint GE

#### BFNode_ApplyEffect（需要 Blueprint GE 时用）

- 输入 Pin：`In`（施加）/ `Remove`（移除，支持 AllStacks/OneStack/CustomCount）
- 输出 Pin：`Out` / `Failed` / `Removed`
- 输出数据引脚：`bGEApplied, GEStackCount, GELevel, GETimeRemaining`
- 最多 3 个 SetByCaller 槽位（Tag + Value）

> 使用场景：GE 内含 ExecutionCalculation、GameplayCue，或需要 SetByCaller 传入运行时参数时。

#### BFNode_ApplyExecution（直接引用 C++ ExecCalc 类，零资产）

无需 Blueprint GE 资产——运行时动态构建含 ExecutionCalculation 的 Instant GE。

| 字段 | 说明 |
|------|------|
| ExecCalcClass | C++ ExecutionCalculation 子类（下拉选择） |
| Target | 目标选择器 |
| SetByCallerTag1/2/3 | SetByCaller 槽，向 ExecCalc 传入运行时浮点参数 |

**使用方式：** 告诉程序"我需要什么计算逻辑" → 程序写 C++ ExecCalc 类 → 在节点下拉里选择 → 通过 SetByCaller 槽传参。无需创建任何 GE 资产。

#### 其他效果节点

| 节点 | 功能 | Cleanup 行为 |
|------|------|------------|
| `BFNode_GrantGA` | 授予 GA；输出 `bGAGranted, GALevel` | 自动撤销 GA |
| `BFNode_AddTag` | 永久添加 Loose Tag（FA 停止时 Cleanup 移除） | 自动移除 Tag |
| `BFNode_RemoveTag` | 移除 Tag | — |
| `BFNode_DoDamage` | 造成伤害（Flat 或 LastAmount×倍率） | — |
| `BFNode_PlayNiagara` | 播放粒子特效（EffectName 标识） | 自动销毁 |
| `BFNode_DestroyNiagara` | 销毁指定特效 | — |
| `BFNode_PlayMontage` | 播放动画蒙太奇 | — |
| `BFNode_SpawnActorAtLocation` | 在 LastKillLocation 生成 Actor | — |
| `BFNode_FinishBuff` | 终止整个 FA | — |

---

### 4.4 Tag 节点（新增）

#### BFNode_GrantTag（带 Duration 的临时 Tag 授予）

与 `BFNode_AddTag` 的区别：支持 **Duration 自动过期**，适合临时状态（如受伤后 5 秒 HeatInhibit）。

| 字段 | 说明 |
|------|------|
| Tag | 要授予的 GameplayTag |
| Duration | 0 = 不自动过期（只在 FA 停止时移除）；> 0 = N 秒后自动移除 |
| Target | 目标选择器 |

**输入 Pin：** `In`（授予）/ `Remove`（手动提前移除）
**输出 Pin：** `Out`（授予成功）/ `Expired`（到期自动移除）/ `Removed`（手动移除）/ `Failed`

**Cleanup 行为：** FA 停止时无论是否到期，都自动移除 Tag（含清理计时器）。

**vs BFNode_AddTag 选择：**
- 永久驻留直到 FA 停止 → `BFNode_AddTag`
- 有明确存在时间（如 HeatInhibit 5s）→ `BFNode_GrantTag`

---

### 4.5 计算节点

| 节点 | 功能 |
|------|------|
| `BFNode_MathFloat` | 浮点运算（+, -, ×, ÷），Result 可连出 |
| `BFNode_MathInt` | 整数运算 |

---

### 4.6 工具节点（新增）

#### BFNode_Delay（等待 N 秒）

| 字段 | 说明 |
|------|------|
| Duration | 等待时间（秒），支持数据引脚连线动态传入 |

**输入 Pin：** `In`（启动计时）/ `Cancel`（提前取消）
**输出 Pin：** `Completed`（计时结束）/ `Cancelled`（取消时触发）

**Cleanup 行为：** FA 停止时自动清理计时器（不会触发 Completed）。

---

#### BFNode_LiteralFloat / LiteralInt / LiteralBool（字面量）

**纯数据节点，无执行引脚。** 等价于 Blueprint 的 "Make Literal Float/Int/Bool"。

- 放在 FA 图里，不需要接入执行流
- 将 Value 输出数据引脚连到下游节点的对应输入引脚
- 在 Details 面板里填写固定值

**典型用途：**
```
[LiteralFloat: Value=5.0] ──(数据引脚)──→ [ApplyAttributeModifier].Value
[LiteralFloat: Value=1.0] ──(数据引脚)──→ [BFNode_Delay].Duration
```

---

### 4.7 热度系统节点

| 节点 | 功能 |
|------|------|
| `BFNode_IncrementPhase` | Phase++，扩大激活区，授予 Phase Tag |
| `BFNode_DecrementPhase` | Phase--（含 Phase>0 守卫），收缩激活区 |
| `BFNode_PhaseDecayTimer` | 防重复激活计时器，含 Cancel Pin |

---

## 五、数据引脚完整类型

FlowGraph 支持 16 种数据引脚类型，当前使用情况：

| 类型 | 使用情况 | 典型场景 |
|------|---------|---------|
| Bool | ✅ 使用 | bGEApplied, bGAGranted 等输出 |
| Int | ✅ 使用 | StackCount, StacksToRemove 等 |
| Float | ✅ 使用 | Value, Level, Duration 等 |
| **GameplayTag** | ❌ 未使用 | 动态 Tag 参数传入节点（P1 计划） |
| **Class** | ❌ 未使用 | 动态指定 GE/GA Class（P1 计划） |
| **Object** | ❌ 未使用 | Actor 引用传递 |
| **Vector** | ❌ 未使用 | 生成位置、击退方向 |
| Name | ❌ 未使用 | 动态 EffectName 参数 |

---

## 六、GAS 功能 FA 化全览

| GAS 功能 | 能否 FA 化 | FA 实现方式 |
|---------|-----------|------------|
| 属性修改（Modifier） | ✅ 零资产 | `BFNode_ApplyAttributeModifier` |
| 属性修改（Periodic） | ✅ 零资产 | `BFNode_ApplyAttributeModifier` Period 字段 |
| Tag 授予（永久） | ✅ 完全 | `BFNode_AddTag`，Cleanup 自动移除 |
| Tag 授予（临时） | ✅ 完全 | `BFNode_GrantTag`，Duration 自动过期 |
| Tag 检查 | ✅ 完全 | `BFNode_HasTag` |
| SetByCaller | ✅ 完全 | `BFNode_ApplyEffect` 槽位 |
| ExecutionCalculation | ✅ 零资产 | `BFNode_ApplyExecution`（直接引用 C++ 类） |
| ExecutionCalculation（via GE） | ✅ 通过 GE | `BFNode_ApplyEffect` 引用含 Exec 的 Blueprint GE |
| GA 授予/撤销 | ✅ 完全 | `BFNode_GrantGA`，Cleanup 自动撤销 |
| Gameplay Cue | ⏳ 待做 | `BFNode_PlayGameplayCue`（P1） |
| GA 激活 | ⏳ 待做 | `BFNode_ActivateAbilityByTag`（P2） |
| Gameplay Event | ⏳ 待做 | `BFNode_SendGameplayEvent`（P2） |
| **Immunity（免疫）** | ✅ FA 等价 | `HasTag(Shielded) → 跳过伤害 GE` |
| **Conditional Effects** | ✅ FA 等价 | `HasTag(Wet) → ApplyEffect(Electro)` |
| AbilityTask（复杂） | ❌ 不适合 | 保留 Blueprint GA |
| 网络同步 Replication | ❌ 不适合 | 保留 Blueprint GA + Replication |

---

## 七、EBFTargetSelector 目标选择器

| 枚举值 | 含义 | 典型用途 |
|--------|------|---------|
| `BuffOwner` | 符文拥有者（玩家自身） | **被动常驻符文必须用这个** |
| `BuffGiver` | 符文施加者（通常同上） | 特殊情况 |
| `LastDamageTarget` | 上次伤害目标（被击者） | 给敌人施加 Debuff |
| `DamageCauser` | 伤害来源（攻击者） | 反弹效果 |

> ⚠️ 被动常驻符文（无伤害事件触发）**必须用 `BuffOwner`**，用 `LastDamageTarget` 会因没有目标而 Failed。

---

## 八、SubGraph 模式（可复用效果库）

`FlowNode_SubGraph` 可以引用另一个 FA，通过 `CustomInput/Output` 传入参数。

```
创建 FA_Util_PhaseBonus（子图）：
  [CustomInput: In] → [HasTag: Phase.2]
                           True  → [ApplyAttributeModifier: Value=20]
                           False → [ApplyAttributeModifier: Value=10]
                      → [CustomOutput: Done]

在任意符文 FA 中复用：
  [OnKill] → [SubGraph: FA_Util_PhaseBonus]
                  Done → [继续...]
```

---

## 九、符文开发标准流程

### Step 1：明确设计

| 问题 | 常见答案 |
|------|---------|
| 触发时机？ | Passive / OnHit / OnKill / OnDamageReceived |
| 效果目标？ | BuffOwner（自己）/ LastDamageTarget（敌人） |
| 持续多久？ | Instant / Infinite / HasDuration |
| 需要堆叠？ | None / Unique / Stackable |
| 需要 Period？ | 每秒 +N 类效果 → 是；普通持续效果 → 否 |
| 需要 Blueprint GE？ | 只有 Cue / 复杂 SetByCaller / 旧版 ExecutionCalculation 时才需要 |
| 需要 Blueprint GA？ | AbilityTask / 网络同步 / 物理冲量时才需要 |

### Step 2：选择正确节点

```
效果是属性修改？
  ├─ 简单数值（+N攻击、+N速度）→ BFNode_ApplyAttributeModifier（零资产）
  ├─ 每秒 +N 类型            → BFNode_ApplyAttributeModifier + Period 字段
  ├─ 需要复杂计算公式         → BFNode_ApplyExecution（程序写 C++ ExecCalc）
  └─ 需要 Cue / SetByCaller  → BFNode_ApplyEffect + Blueprint GE

效果是 Tag 状态？
  ├─ 永久到 FA 停止           → BFNode_AddTag
  └─ 有明确时限（如 5s 抑制）→ BFNode_GrantTag + Duration

效果需要能力执行？
  └─ BFNode_GrantGA + Blueprint GA（AbilityTask 逻辑写在 GA 里）
```

### Step 3：创建 FA

路径：`Content/Game/Runes/<符文名>/` · 命名：`FA_Rune_<功能名>`

### Step 4：创建 DA

路径：`Content/Game/Runes/<符文名>/` · 命名：`DA_Rune_<功能名>`

必填：RuneName, RuneID, RuneType, Shape.Cells, Flow.FlowAsset

### Step 5：测试

1. 将 DA 拖入背包激活区
2. GAS Debugger 验证属性变化和 Tag 状态
3. 移出激活区验证 Cleanup（GE/Tag 是否正确移除）

---

## 十、设计模式

### 模式 A：被动属性加成（零资产）
```
[Start] → [ApplyAttributeModifier: Attack+10, Infinite, BuffOwner]
```

### 模式 B：条件触发
```
[Start] → [OnKill] → [HasTag: Phase.2]
                         True  → [ApplyAttributeModifier: +20]
                         False → [ApplyAttributeModifier: +10]
```

### 模式 C：堆叠型（击中叠加，零资产）
```
[Start] → [OnDamageDealt] → [ApplyAttributeModifier: Speed+5, 3s, Stackable, Max=5, BuffOwner]
```

### 模式 D：周期性效果（每秒 +N，零资产）
```
[Start] → [ApplyAttributeModifier: Heat+1, Infinite, Period=1.0, BuffOwner]
```
> 使用 Period 字段，让 GAS 每秒自动执行一次 Modifier，不需要 Blueprint GE。

### 模式 E：受伤暂停（Period GE + GrantTag 自动 Inhibit）
```
GE_HeatTick（Blueprint GE）:
  Infinite, Period=1.0, Modifier: Heat+1
  OngoingTagRequirements.IgnoreTags = Buff.Status.HeatInhibit

[Start] → [ApplyEffect: GE_HeatTick]
[OnDamageReceived] → [GrantTag: Buff.Status.HeatInhibit, Duration=5s, BuffOwner]
                          Expired → （Tag 自动移除，热度恢复积累）
```
> GrantTag 到期时 GAS 自动恢复 GE_HeatTick 的 Period 执行。

### 模式 F：受伤暂停（纯 FA，无 Blueprint GE）
```
[Start] → [ApplyAttributeModifier: Heat+1, Infinite, Period=1.0, BuffOwner]
           （此节点的 GE handle 存储在节点内）
[OnDamageReceived] → [GrantTag: Buff.Status.HeatInhibit, Duration=5s, BuffOwner]
```
> 注意：纯 FA 方案中 GrantTag 只是标记，不能直接 Inhibit Period GE（Period GE 需要 OngoingTagRequirements 才能响应）。如需真正暂停计时，使用模式 E（Blueprint GE）。

### 模式 G：递归守卫（防止效果触发自身）
```
[OnDamageDealt]
  → [HasTag: Buff.Status.ExtraDamageApplied, BuffOwner]
        False → [AddTag: ExtraDamageApplied] → [DoDamage: ×0.5] → [RemoveTag: ExtraDamageApplied]
        True  → （跳过）
```

### 模式 H：计数型（N次触发后爆发）
```
[Start] → [OnKill]
               → [FlowNode_Counter: Goal=5]
                      Goal reached → [ApplyAttributeModifier: Attack+100, 10s]
```

### 模式 I：随机效果
```
[Start] → [OnKill]
               → [FlowNode_ExecutionMultiGate: Random=true]
                      Output0 → [ApplyAttributeModifier: Attack+15]
                      Output1 → [ApplyAttributeModifier: MoveSpeed+100]
                      Output2 → [ApplyAttributeModifier: CritRate+0.1]
```

### 模式 J：多条件同步
```
[OnKill]    ─→ [FlowNode_LogicalAND]
[OnCritHit] ─→                      ─→ [ApplyAttributeModifier: 大额加成]
```

### 模式 K：SubGraph 复用
```
FA_Util_PhaseBonus:
  [In] → [HasTag: Phase.2] → True: +20 / False: +10 → [Done]

符文 FA：
  [OnKill] → [SubGraph: FA_Util_PhaseBonus] → Done → [下一步]
  [OnHit]  → [SubGraph: FA_Util_PhaseBonus] → Done → [下一步]
```

### 模式 L：延迟效果（新增）
```
[OnKill] → [Delay: Duration=2s]
                Completed → [ApplyAttributeModifier: Attack+50, 5s]
```

### 模式 M：字面量驱动数据引脚（新增）
```
[LiteralFloat: 5.0] ──(数据)──→ [ApplyAttributeModifier].Value
[LiteralFloat: 1.0] ──(数据)──→ [Delay].Duration
（LiteralFloat 不需要接入执行流，只连数据引脚）
```

---

## 十一、调试工具

### GAS Debugger
编辑器菜单：`Window → Gameplay Debugger`（或 `'` 键）
- 验证 Active GE 列表（GE 是否施加）
- 验证属性当前值（Attack, Heat 等）
- 验证角色 Tag 列表（GrantedTags 是否生效）

### FlowNode_Log
直接在 FA 图里插入 Log 节点，输出到屏幕+Log，定位执行路径。

### 常见问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 被动符文没效果 | Target=LastDamageTarget，被动无伤害目标 | 改为 BuffOwner |
| GE 没有效果 | Attribute 填错或 AttributeSet 未注册 | GAS Debugger 查属性 |
| FA 停后 GE 未移除 | bFinish=true 节点提前退出 | 确认 TriggerOutput 的 bFinish=false |
| Period 不执行 | DurationType = Instant | 改为 Infinite 或 HasDuration |
| GrantTag 未自动移除 | Duration=0 时不启动计时器 | Duration > 0 才有自动过期；FA 停止时 Cleanup 兜底 |
| Delay 计时器不触发 | GetWorld() 返回 null | 确认节点在有效 World 中执行 |
| ApplyExecution Failed | ExecCalcClass 未设置 | 在节点 Details 面板选择 C++ ExecCalc 类 |
| PassThroughOwnerTags 无效 | .cpp 仅有前向声明 | 补 `#include YogAbilitySystemComponent.h` |

---

## 十二、开发现状与计划

### 已完成

| 模块 | 状态 |
|------|------|
| BFNode 基础框架（40个节点） | ✅ |
| BFNode_ApplyAttributeModifier（含 Period/StackMode） | ✅ |
| BFNode_ApplyEffect（含 Remove 引脚 + SetByCaller） | ✅ |
| BFNode_ApplyExecution（零资产 ExecutionCalculation） | ✅ |
| BFNode_GrantTag（Duration 自动过期） | ✅ |
| BFNode_Delay（可取消计时器） | ✅ |
| BFNode_LiteralFloat/Int/Bool（字面量数据节点） | ✅ |
| BFNode_GrantGA 输出引脚 | ✅ |
| 热度升降阶系统 | ✅ |
| RuneDataAsset 精简结构 | ✅ |
| Tag 规范文档 | ✅ |

### P1 — 框架扩展

| 任务 | 说明 |
|------|------|
| `BFNode_PlayGameplayCue` | 触发 Gameplay Cue，不绑 GE |
| `BFNode_Base` Blueprintable | 开放蓝图继承，支持 Flow Node Blueprint |
| GameplayTag / Class 数据引脚 | 让 Tag、GE Class 参数可动态传入 |

### P2 — 战斗能力节点

| 任务 | 说明 |
|------|------|
| `BFNode_ActivateAbilityByTag` | 触发目标身上的 GA |
| `BFNode_SendGameplayEvent` | 发送 Gameplay Event |

### P3 — 体验完善

| 任务 | 说明 |
|------|------|
| 升阶 CanPhaseUp 检查移入 FA | 当前在 FNode 内部，移到 FA 用 HasTag 节点更透明 |
| 升阶特效/音效 | BFNode_PlayGameplayCue 完成后接入 |
| 热度 UI | 当前阶段显示、激活区视觉反馈 |

### 测试符文进度

| 符文 | 触发 | 效果 | 关键技术 | 状态 |
|------|------|------|---------|------|
| 攻击强化（1001） | Passive | Attack+10, Infinite | ApplyAttributeModifier | ✅ 已测试 |
| 热度提升（1002） | Passive | 每秒+1 Heat，受伤暂停5s | Period + GrantTag | ⬜ 待测试 |
| 速度叠加（1003） | OnHit | Speed+30 叠加，3s，Max5 | ApplyAttributeModifier Stackable | ⬜ 待测试 |
| 击退（1004） | OnHit | 移速-300 硬直+冲量 | ApplyEffect SetByCaller + GA | ⬜ 待测试 |
| 流血（1005） | OnHit | GrantTag 流血 + GA 速度扣血 | GrantTag + GrantGA | ⬜ 待测试 |
| 额外伤害（1006） | OnDamageDealt | 额外 1 伤害，递归守卫 | AddTag 守卫模式 | ⬜ 待测试 |

---

## 十三、文件清单

### C++ 节点
- `Source/DevKit/Public/BuffFlow/Nodes/` — 所有 BFNode 头文件
- `Source/DevKit/Private/BuffFlow/Nodes/` — 所有 BFNode 实现

### FA 资产
- `Content/Game/Runes/HeatPhase/FA_Rune_HeatPhase` — 热度升降阶（永久）
- `Content/Game/Runes/AttackUp/FA_Rune_AttackUp` — 攻击强化（已测试）

### 关联文档
- `Docs/Design/Buff_Tag_Spec.md` — Tag 命名规范
- `Docs/Design/HeatSystem_Design.md` — 热度系统详细设计
- `Docs/Design/BuffFlow_ArchitectureAndPlan.md` — 架构决策与开发计划
