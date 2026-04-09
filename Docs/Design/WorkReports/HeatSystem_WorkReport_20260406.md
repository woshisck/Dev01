# 热度系统工作汇报

**项目**：星骸降临
**版本**：Sprint 4.15
**完成日期**：2026-04-06

---

## 一、背景与目标

热度系统是本作核心的战斗循环驱动器：玩家通过命中敌人积累热度，热度满值后完成"升阶"，背包激活区扩展、更多符文生效，从而强化战斗能力。本次迭代完成热度系统从属性记录到阶段管理的完整闭环，全部逻辑可在 Flow Graph 中可视化配置。

---

## 二、系统架构总览

```
攻击命中
  └─ DamageExecution → 施加 GE_HeatBuff（含 Action.Heat.CanPhaseUp 标签）
       └─ BaseAttributeSet.PostGameplayEffectExecute
            ├─ 热度未满：SetHeat(MaxHeat) [卡上限]
            └─ 热度已满 + bWasAlreadyFull + CanPhaseUp → BGC.OnPhaseUpReady.Broadcast()
                                                              └─ FA_Rune_HeatPhase (Flow Graph)
                                                                   ├─ BFNode_OnPhaseUpReady
                                                                   ├─ BFNode_IncrementPhase
                                                                   └─ BFNode_PhaseDecayTimer
                                                                        └─ BFNode_DecrementPhase

热度衰减（GE_HeatDecay 周期触发）
  └─ PreGameplayEffectExecute：持有 Buff.Heat.Active 时阻断（战斗状态保热）
  └─ OnHeatValueChanged → 边沿检测
       ├─ >0 → ≤0（Phase>0）：BGC.OnHeatReachedZero → 启动 PhaseDecayTimer
       └─ ≤0 → >0：BGC.OnHeatAboveZero → Cancel PhaseDecayTimer
```

---

## 三、新增功能模块

### 3.1 Phase 阶段管理（BackpackGridComponent）

| 新增内容 | 说明 |
|---|---|
| `CurrentPhase`（0–3） | 当前阶段，驱动 `EHeatTier` 决定激活区大小 |
| `PreviousHeatValue` | 边沿检测，仅在热度"穿越零点"时广播委托 |
| `OnPhaseUpReady` | 热度满值 + LastHit → 触发升阶 |
| `OnHeatReachedZero` | 热度跌零（Phase>0 守卫）→ 启动衰减计时 |
| `OnHeatAboveZero` | 热度回升 → 取消衰减计时 |
| `IncrementPhase / DecrementPhase` | 阶段升降，联动 `EHeatTier` 更新激活区 |

**关键设计决策**：

- `PreviousHeatValue` 初始值设为 `1.f`（非零），防止游戏启动时热度=0 误触发衰减
- `OnHeatReachedZero` 加入 `CurrentPhase > 0` 守卫，Phase0 时热度归零不启动计时
- 委托类型为非动态 `DECLARE_MULTICAST_DELEGATE`，BFNode 通过 `AddUObject` 绑定（无 GC 风险）

### 3.2 永久符文（PermanentRunes）

在 `BackpackGridComponent` 新增 `PermanentRunes` 数组：

- `BeginPlay` 自动寻位放置，不依赖 Debug 数据填写
- `FPlacedRune.bIsPermanent = true`，`RefreshAllActivations` 中跳过激活区检查
- 放置后直接调用 `ActivateRune()`，不依赖后续的 `RefreshAllActivations` 回调时序

**典型用途**：`DA_Rune_HeatPhase` 作为永久相位符文，玩家无需手动放置。

### 3.3 升阶条件精确控制（BaseAttributeSet）

```cpp
bWasAlreadyFull = (CachedPreEffectHeat >= GetMaxHeat())
```

- `CachedPreEffectHeat` 在 `PreGameplayEffectExecute` 捕获，代表 GE 施加**前**的热度
- 升阶要求：热度在本次 GE 命中**之前已满** + 本次携带 `CanPhaseUp` 标签
- 避免了"热度90，LastHit打入20溢出"这种意外升阶场景

### 3.4 新增 BFNodes（Flow Graph 节点）

| 节点 | 功能 |
|---|---|
| `BFNode_OnPhaseUpReady` | 监听 `BGC.OnPhaseUpReady`，输出 `OnPhaseUp` |
| `BFNode_OnHeatReachedZero` | 监听热度归零/回升，输出 `OnReachedZero` / `OnAboveZero` |
| `BFNode_IncrementPhase` | 调用 `BGC.IncrementPhase()` |
| `BFNode_DecrementPhase` | 调用 `BGC.DecrementPhase()`（含 Phase>0 守卫） |
| `BFNode_PhaseDecayTimer` | C++ 计时器，重复触发静默忽略，支持 Cancel pin |

### 3.5 AnimNotifyState_AddGameplayTag

蒙太奇窗口内自动添加/移除 Gameplay Tag，`NotifyEnd` 在中断时也会被调用，确保标签不残留。典型用途：动画 `LastHit` 帧窗口内添加 `Action.Heat.CanPhaseUp`，窗口结束自动清理。

### 3.6 BFNode_ApplyAttributeModifier — PassThroughOwnerTags

在施加 GE 时，检查 Owner ASC 当前持有的标签集合，将匹配的标签追加到 `Spec.DynamicAssetTags`。FA 通过这个机制将蒙太奇赋予的瞬时标签传递到 GE，让 `BaseAttributeSet` 可以在 C++ 侧检测到。

---

## 四、标签设计

```ini
; PlayerGameplayTag.ini
Action.Heat.CanPhaseUp   ; 允许热度升阶的通用标记
                         ; 可由 LastHit 蒙太奇窗口、完美闪避等多种来源授予

; Buff.ini
Buff.Heat.Active         ; 战斗状态下抑制热度自然衰减
```

从原先的 `Action.Combo.LastHit` 泛化为 `Action.Heat.CanPhaseUp`，使完美闪避、技能命中等多种来源均可授予升阶权限，不局限于连击末帧。

---

## 五、遇到的问题与解决过程

| 问题 | 根本原因 | 解决方案 |
|---|---|---|
| "Timer already active" 报错 | Flow 内置 Timer 节点拒绝重复激活，而 `OnHeatReachedZero` 可能多次触发 | 新建 `BFNode_PhaseDecayTimer`，内部用 `IsTimerActive` 守卫，重复 In 静默忽略 |
| 游戏启动即触发 Phase DOWN | 初始热度=0，`PreviousHeatValue` 默认=0，导致开局广播 `OnHeatReachedZero` | `PreviousHeatValue` 初始化为 `1.f`；另加 `CurrentPhase > 0` 守卫 |
| 热度90时 LastHit 意外升阶 | LastHit 打入后热度溢出，误判为"满值升阶" | 引入 `bWasAlreadyFull`（GE 施加前快照），升阶必须 Pre-GE 热度已满 |
| 永久符文不激活 | `TryPlaceRune` 内部调 `RefreshAllActivations` 时 `bIsPermanent` 尚未置位 | 设置 `bIsPermanent=true` 后手动调 `ActivateRune()`，不依赖 Refresh 回调时序 |
| PassThroughOwnerTags 无效 | 编译器只看到前向声明，无法确认 `UYogAbilitySystemComponent` 继承关系 | 在 cpp 文件补 `#include "AbilitySystem/YogAbilitySystemComponent.h"` |
| 运算符优先级 bug | `A && B \|\| C` 中 `\|\|` 在 `&&` 外，标签守卫失效 | 改为 `A && (B \|\| C)`，显式括号 |

---

## 六、测试验证

日志验证片段（正常运行状态）：

```
[BackpackGrid] Phase UP → 1           ✅ 升阶正常触发
[BackpackGrid] Heat→0 edge (Phase=1)  ✅ 衰减计时正常启动
[BackpackGrid] Phase DOWN → 0         ✅ 10秒后正常降阶
（无 "Timer already active" 报错）    ✅ 计时器重复触发已消除
```

---

## 七、文件变动汇总

### 新增 C++ 文件（12个）

| 文件 | 类型 |
|---|---|
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPhaseUpReady.h` | 新增 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_OnPhaseUpReady.cpp` | 新增 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHeatReachedZero.h` | 新增 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_OnHeatReachedZero.cpp` | 新增 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_IncrementPhase.h` | 新增 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_IncrementPhase.cpp` | 新增 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_DecrementPhase.h` | 新增 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_DecrementPhase.cpp` | 新增 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_PhaseDecayTimer.h` | 新增 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_PhaseDecayTimer.cpp` | 新增 |
| `Source/DevKit/Public/Animation/AnimNotifyState_AddGameplayTag.h` | 新增 |
| `Source/DevKit/Private/Animation/AnimNotifyState_AddGameplayTag.cpp` | 新增 |

### 主要修改文件

| 文件 | 变更内容 |
|---|---|
| `BackpackGridComponent.h/.cpp` | Phase 委托、边沿检测、PermanentRunes |
| `BaseAttributeSet.cpp` | bWasAlreadyFull、CanPhaseUp 检测 |
| `BFNode_ApplyAttributeModifier.h/.cpp` | PassThroughOwnerTags |
| `Config/Tags/PlayerGameplayTag.ini` | 新增 `Action.Heat.CanPhaseUp` |

### 新增 UE 资产

| 资产 | 说明 |
|---|---|
| `FA_Rune_HeatPhase` / `DA_Rune_HeatPhase` | Phase 符文 Flow Graph 配置 |
| `FA_Rune_HeatOnHit` / `DA_Rune_HeatOnHit` | 命中加热 Flow Graph 配置 |
| `GE_HeatBuff` | 命中时施加的热度增加 GE |
| `GE_HeatDecay` | 周期性热度衰减 GE |

---

## 八、后续计划

- [ ] 升阶时播放特效/音效反馈（现只有 Log）
- [ ] 完美闪避授予 `Action.Heat.CanPhaseUp` 的接入
- [ ] 升华阶段（Phase 3）的特殊行为定义
- [ ] 热度 UI 显示（当前阶段、激活区视觉反馈）
