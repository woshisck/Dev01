# 热度系统设计文档

> 项目：星骸降临  
> 系统版本：Sprint 4.15（2026-04-06 实现）  
> Tag 命名规范遵循 [`Buff_Tag_Spec.md`](Buff_Tag_Spec.md)

---

## 一、系统定位

热度系统是本作核心的**战斗循环驱动器**。

玩家通过命中敌人积累热度值，热度满后完成"升阶"，背包激活区扩展、更多符文生效，从而形成"打得越猛、符文越强"的正反馈循环。长时间脱战则热度衰减、阶段下降，促使玩家保持进攻节奏。

```
积累热度 → 热度满 + LastHit → 升阶 → 激活区扩展 → 更多符文生效
                                                          ↓
                                              脱战衰减 → 降阶
```

---

## 二、Tag 体系

热度系统使用以下 GameplayTag，全部定义在 `Config/Tags/BuffTag.ini`。

### 2.1 热度状态标签（状态层）

> 状态层 Tag 由 GE 的 `GrantedTags` 授予角色 ASC，GE 失效时自动移除。  
> 详见 [`Buff_Tag_Spec.md`](Buff_Tag_Spec.md) §3.4

| Tag | 授予时机 | 移除时机 | 用途 |
|-----|----------|----------|------|
| `Buff.Status.Heat.Active` | 角色进入战斗状态 | 脱战时 GE 失效 | C++ 侧阻断 `GE_HeatDecay` 执行，战斗期间保热 |
| `Buff.Status.Heat.Phase` | （父节点，不直接授予） | — | 父节点匹配：`HasTag(Phase)` 等价于"任意阶段" |
| `Buff.Status.Heat.Phase.1` | `BFNode_IncrementPhase` 升至阶段 1 | `BFNode_DecrementPhase` 降阶 | 标识当前处于第一阶段 |
| `Buff.Status.Heat.Phase.2` | 升至阶段 2 | 降阶 | 标识当前处于第二阶段 |
| `Buff.Status.Heat.Phase.3` | 升至阶段 3 | 降阶 | 标识当前处于第三阶段（满阶） |

### 2.2 升阶条件标签（跨系统）

> 定义在 `Config/Tags/PlayerGameplayTag.ini`，属于 `Action.*` 树。

| Tag | 来源 | 用途 |
|-----|------|------|
| `Action.Heat.CanPhaseUp` | `AnimNotifyState_AddGameplayTag` 在 LastHit 动画窗口内写入；完美闪避等可扩展 | `DamageExecution` → `BaseAttributeSet` 检测此 Tag，满足时才允许升阶 |

---

## 三、系统架构

### 3.1 完整流程图

```
【热度积累】
攻击命中
  └─ DamageExecution
       └─ 施加 GE_HeatBuff（携带 Action.Heat.CanPhaseUp 动态标签）
            └─ BaseAttributeSet.PostGameplayEffectExecute
                 ├─ 热度未满 → SetHeat(当前值 + 增量)，卡 MaxHeat 上限
                 └─ 热度已满（PreGE 快照已满）+ 携带 CanPhaseUp
                      └─ BGC.OnPhaseUpReady.Broadcast()

【升阶】
BGC.OnPhaseUpReady
  └─ FA_Rune_HeatPhase（Flow Graph）
       └─ BFNode_OnPhaseUpReady
            └─ BFNode_IncrementPhase
                 ├─ CurrentPhase++
                 ├─ 更新 EHeatTier → 激活区扩展
                 └─ 授予 Buff.Status.Heat.Phase.N（对应阶段 GE GrantedTags）

【热度衰减】
GE_HeatDecay（周期 Tick）
  └─ PreGameplayEffectExecute
       ├─ 持有 Buff.Status.Heat.Active → 阻断，不执行衰减
       └─ 未持有 → 执行衰减，Heat -= DecayAmount

OnHeatValueChanged（AttributeSet 边沿检测）
  ├─ >0 → ≤0（且 Phase>0）→ BGC.OnHeatReachedZero → 启动 PhaseDecayTimer
  └─ ≤0 → >0              → BGC.OnHeatAboveZero  → 取消 PhaseDecayTimer

【降阶】
BFNode_PhaseDecayTimer（10 秒，可配置）
  └─ BFNode_DecrementPhase
       ├─ CurrentPhase--（含 Phase>0 守卫，不会降到负数）
       ├─ 更新 EHeatTier → 激活区收缩
       └─ 移除 Buff.Status.Heat.Phase.N（上一阶段 GE 失效）
```

### 3.2 升阶精确条件

升阶需要同时满足**三个条件**：

| 条件 | 实现方式 | 防止的问题 |
|------|----------|------------|
| 热度在 GE 施加**前**已满 | `CachedPreEffectHeat`（PreGE 快照） | 热度 90 时 LastHit 打入 20，溢出不触发升阶 |
| GE 携带 `CanPhaseUp` 标签 | `PassThroughOwnerTags` 将蒙太奇 Tag 追加到 GE DynamicAssetTags | 普通攻击不能升阶，只有 LastHit 帧或指定来源才行 |
| 本次 GE 命中 bWasAlreadyFull | 与第一条联动 | 同上 |

---

## 四、核心组件详解

### 4.1 BackpackGridComponent（BGC）

热度系统的**状态管理中心**，负责阶段值维护和委托广播。

| 成员 | 类型 | 说明 |
|------|------|------|
| `CurrentPhase` | int（0–3） | 当前热度阶段，驱动 `EHeatTier` 决定激活区大小 |
| `PreviousHeatValue` | float，初始值 `1.f` | 边沿检测用，初始非零防止开局误触发衰减 |
| `OnPhaseUpReady` | Multicast Delegate | 热度满 + CanPhaseUp → 广播，FA 的 BFNode_OnPhaseUpReady 监听 |
| `OnHeatReachedZero` | Multicast Delegate | 热度跌零（Phase>0 守卫）→ 广播，启动衰减计时 |
| `OnHeatAboveZero` | Multicast Delegate | 热度从零回升 → 广播，取消衰减计时 |
| `IncrementPhase()` | 函数 | Phase++，联动 EHeatTier，授予对应阶段 Tag |
| `DecrementPhase()` | 函数 | Phase--（含守卫），联动 EHeatTier，移除对应阶段 Tag |

**关键设计决策：**

- `PreviousHeatValue` 初始化为 `1.f`（非零），防止游戏启动时热度=0 误触发 `OnHeatReachedZero`
- `OnHeatReachedZero` 内置 `CurrentPhase > 0` 守卫，Phase 0 时热度归零不启动衰减计时
- 委托类型为非动态 `DECLARE_MULTICAST_DELEGATE`，BFNode 通过 `AddUObject` 绑定，无 GC 风险

---

### 4.2 GE 配置

#### GE_HeatBuff — 命中增热

| 配置项 | 值 | 说明 |
|--------|-----|------|
| Duration Policy | Instant | 瞬时应用，直接修改属性 |
| Modifier | Attribute=`Heat`，Op=Additive，Magnitude=命中增热量 | 每次命中增加热度 |
| 动态 Asset Tag | `Action.Heat.CanPhaseUp`（由 PassThroughOwnerTags 追加） | LastHit 帧命中时携带，BaseAttributeSet 检测此 Tag 判断是否可升阶 |

#### GE_HeatDecay — 周期衰减

| 配置项 | 值 | 说明 |
|--------|-----|------|
| Duration Policy | Infinite | 持续运行 |
| Period | 1.0s（可配置） | 每秒触发一次 PreGameplayEffectExecute |
| Modifier | Attribute=`Heat`，Op=Additive，Magnitude=负值 | 每 Tick 减少热度 |
| 阻断逻辑 | C++ `PreGameplayEffectExecute` 检测 `Buff.Status.Heat.Active` | 战斗状态下整个 GE Tick 被跳过，热度不流失 |

---

### 4.3 BFNode 节点说明

> 所有节点在 `FA_Rune_HeatPhase`（Flow Graph）中编排，全部逻辑可视化。

| 节点 | 监听 / 调用 | 输出 Pin | 说明 |
|------|------------|----------|------|
| `BFNode_OnPhaseUpReady` | `BGC.OnPhaseUpReady` 委托 | `OnPhaseUp` | 热度满 + LastHit 时触发升阶流程入口 |
| `BFNode_IncrementPhase` | 调用 `BGC.IncrementPhase()` | — | 阶段 +1，联动激活区，授予 Phase Tag |
| `BFNode_OnHeatReachedZero` | `BGC.OnHeatReachedZero` / `OnHeatAboveZero` | `OnReachedZero` / `OnAboveZero` | 热度跌零/回升的双向边沿检测 |
| `BFNode_PhaseDecayTimer` | C++ 计时器（内置重复守卫） | 超时触发 | 重复激活静默忽略，支持 `Cancel` Pin 取消 |
| `BFNode_DecrementPhase` | 调用 `BGC.DecrementPhase()` | — | 阶段 -1，含 Phase>0 守卫，移除 Phase Tag |

#### FA_Rune_HeatPhase Flow Graph 连接示意

```
[BFNode_OnPhaseUpReady]
    OnPhaseUp ──→ [BFNode_IncrementPhase]

[BFNode_OnHeatReachedZero]
    OnReachedZero ──→ [BFNode_PhaseDecayTimer]
                           超时 ──→ [BFNode_DecrementPhase]
    OnAboveZero   ──→ [BFNode_PhaseDecayTimer] Cancel Pin
```

---

### 4.4 AnimNotifyState_AddGameplayTag

蒙太奇窗口内自动添加/移除 GameplayTag 的通用 NotifyState。

| 特性 | 说明 |
|------|------|
| `NotifyBegin` | 向角色 ASC 添加指定 Tag |
| `NotifyEnd` | 移除 Tag，**中断时也调用**，确保 Tag 不残留 |
| 典型用途 | LastHit 攻击帧窗口内添加 `Action.Heat.CanPhaseUp`，窗口结束自动清理 |
| 扩展性 | 完美闪避、特定技能均可通过同一机制授予 `CanPhaseUp`，不局限于连击末帧 |

---

### 4.5 BFNode_ApplyAttributeModifier — PassThroughOwnerTags

在施加 GE 时，将 Owner ASC 当前持有的**匹配标签**追加到 `GESpec.DynamicAssetTags`。

**作用**：蒙太奇授予的瞬时 Tag（如 `Action.Heat.CanPhaseUp`）只存在于角色 ASC 上，默认不会进入 GE Spec。PassThroughOwnerTags 将其"复制"进 GE，使 `BaseAttributeSet` 的 C++ 侧能在执行时检测到。

**注意**：依赖 `#include "AbilitySystem/YogAbilitySystemComponent.h"` 在 cpp 文件中完整包含（前向声明不够）。

---

## 五、Buff.Status.Heat.Active 详解

这个 Tag 是热度系统中最容易被误分类的设计点，单独说明。

### 它不是什么

| 误判 | 原因 |
|------|------|
| ❌ 不是行为层 `Buff.Effect.*` | 它不描述"GE 做了什么"，它描述"角色现在处于什么状态" |
| ❌ 不是触发层 `Buff.Trigger.*` | 它不是符文的触发时机，而是 C++ 的执行守卫 |
| ❌ 不是 `Buff.Heat.Active`（旧命名） | 缺少层级前缀，不符合 Tag 规范，已废弃 |

### 它是什么

**状态层守卫 Tag**（与 `Buff.Status.ExtraDamageApplied` 同类型）：

```
战斗开始
  └─ 战斗状态 GE 的 GrantedTags 授予角色 Buff.Status.Heat.Active

GE_HeatDecay 每秒 Tick
  └─ PreGameplayEffectExecute（C++）
       ├─ HasTag(Buff.Status.Heat.Active) == true  → return false（阻断，不执行衰减）
       └─ HasTag(Buff.Status.Heat.Active) == false → return true（放行，执行衰减）

战斗结束
  └─ 战斗状态 GE 失效 → GrantedTags 自动撤销 → Active Tag 消失 → 衰减恢复
```

### 与 Phase Tag 的区别

| | `Buff.Status.Heat.Active` | `Buff.Status.Heat.Phase.*` |
|-|--------------------------|---------------------------|
| 含义 | 是否处于战斗/保热状态 | 当前处于哪个热度阶段 |
| 授予来源 | 战斗状态 GE | 升阶流程 GE |
| 影响系统 | C++ `PreGameplayEffectExecute` | 激活区大小、符文条件判断 |
| 能否同时存在 | 与 Phase Tag 相互独立，可同时存在 | Phase 子节点同一时刻只有一个 |

---

## 六、已知问题与解决方案

| 问题 | 根本原因 | 解决方案 |
|------|----------|----------|
| "Timer already active" 报错 | Flow 内置 Timer 节点拒绝重复激活，`OnHeatReachedZero` 可多次触发 | 新建 `BFNode_PhaseDecayTimer`，内部 `IsTimerActive` 守卫，重复 In 静默忽略 |
| 游戏启动即触发 Phase DOWN | `PreviousHeatValue` 默认 0，开局 Heat=0 误触发 `OnHeatReachedZero` | `PreviousHeatValue` 初始化为 `1.f`；另加 `CurrentPhase > 0` 守卫 |
| 热度 90 时 LastHit 意外升阶 | LastHit 打入后热度溢出，误判为"满值升阶" | 引入 `bWasAlreadyFull`（GE 施加前快照），升阶必须 Pre-GE 热度已满 |
| 永久符文不激活 | `TryPlaceRune` 内部调 `RefreshAllActivations` 时 `bIsPermanent` 尚未置位 | 置位后手动调 `ActivateRune()`，不依赖 Refresh 回调时序 |
| PassThroughOwnerTags 无效 | 编译器只看到前向声明，无法确认继承关系 | cpp 文件补 `#include "AbilitySystem/YogAbilitySystemComponent.h"` |
| 运算符优先级 bug | `A && B \|\| C` 中 `\|\|` 在 `&&` 外，标签守卫失效 | 改为 `A && (B \|\| C)`，显式括号 |

---

## 七、文件清单

### C++ 文件

| 文件 | 职责 |
|------|------|
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPhaseUpReady.h/.cpp` | 监听升阶委托，Flow Graph 入口 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHeatReachedZero.h/.cpp` | 热度零值边沿检测 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_IncrementPhase.h/.cpp` | 阶段升级 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_DecrementPhase.h/.cpp` | 阶段降级（含守卫） |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_PhaseDecayTimer.h/.cpp` | 衰减计时器（防重复激活） |
| `Source/DevKit/Public/Animation/AnimNotifyState_AddGameplayTag.h/.cpp` | 蒙太奇窗口 Tag 管理 |
| `BackpackGridComponent.h/.cpp` | Phase 委托、边沿检测、PermanentRunes |
| `BaseAttributeSet.cpp` | `bWasAlreadyFull`、`CanPhaseUp` 检测、`Heat.Active` 阻断守卫 |
| `BFNode_ApplyAttributeModifier.h/.cpp` | PassThroughOwnerTags |

### UE 资产

| 资产 | 说明 |
|------|------|
| `FA_Rune_HeatPhase` | 升阶/降阶的 Flow Graph，永久符文 |
| `DA_Rune_HeatPhase` | 对应 Data Asset，`bIsPermanent = true` |
| `FA_Rune_HeatOnHit` | 命中加热的 Flow Graph |
| `DA_Rune_HeatOnHit` | 对应 Data Asset |
| `GE_HeatBuff` | 命中时施加的热度增加 GE |
| `GE_HeatDecay` | 周期性热度衰减 GE，受 `Buff.Status.Heat.Active` 阻断 |

### Tag 文件

| 文件 | 新增/变更内容 |
|------|--------------|
| `Config/Tags/BuffTag.ini` | `Buff.Status.Heat.Active`、`Buff.Status.Heat.Phase.1/2/3` |
| `Config/Tags/PlayerGameplayTag.ini` | `Action.Heat.CanPhaseUp` |

---

## 八、后续计划

- [ ] 升阶时播放特效/音效反馈（当前仅有 Log 输出）
- [ ] 完美闪避接入 `Action.Heat.CanPhaseUp`（`AnimNotifyState` 已可复用）
- [ ] Phase 3（满阶）特殊行为定义
- [ ] 热度 UI 显示（当前阶段、激活区视觉反馈）
- [ ] `Buff.Status.Heat.Active` 的授予 GE 与战斗状态系统打通（当前需确认实际授予时机）