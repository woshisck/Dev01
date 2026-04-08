# BuffFlow 系统指南

> 项目：星骸降临
> 版本：Sprint 4.15（2026-04-08）
> 关联文档：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · [HeatSystem_Design.md](HeatSystem_Design.md)

---

## 一、系统定位

BuffFlow 是项目专属的**符文逻辑可视化框架**，构建在 FlowGraph 插件之上，结合 GAS（Gameplay Ability System）实现符文效果的全部行为。

**设计原则：**
- 每个符文的行为逻辑 **100% 在 FA（Flow Asset）里可视化**，不散落在蓝图或 C++ 里
- GAS/GE 只负责"效果本体"（数值修改、Tag 授予、生命周期）
- FA 负责"逻辑编排"（触发时机、条件判断、效果施加顺序）
- C++ BFNode 提供可复用的原子操作，设计师通过拖连线组合

```
符文 DA（配置层）
  └─ FA（逻辑层）← 本系统的核心工作区
       └─ BFNode（原子操作层）← C++ 实现，FA 中复用
            └─ GAS/GE（效果执行层）← UE 原生
```

---

## 二、核心组件

### 2.1 RuneDataAsset（符文配置资产）

每个符文对应一个 DA，包含全部静态数据。

```
URuneDataAsset
  └─ FRuneInstance RuneTemplate
       ├─ FName         RuneName          符文名称
       ├─ UTexture2D*   RuneIcon          图标
       ├─ FText         RuneDescription   描述文本
       ├─ FRuneShape    Shape             背包格子占用（FIntPoint 数组）
       ├─ FRuneConfig   RuneConfig        元数据
       │    ├─ ERuneType  RuneType        Buff / Debuff / None（UI 分类）
       │    └─ int32      RuneID          策划表 ID
       └─ FRuneFlowConfig Flow            逻辑层
            └─ UFlowAsset* FlowAsset     符文激活时启动的 FA
```

**字段填写规范：**

| 字段 | 填写内容 | 示例 |
|------|---------|------|
| RuneName | 符文显示名 | `AttackUp` |
| RuneID | 策划表唯一 ID，从 1001 开始递增 | `1001` |
| RuneType | 符文功能分类 | `Buff` |
| Shape.Cells | 背包格坐标，原点左上角 | `[(0,0),(1,0),(0,1)]` |
| Flow.FlowAsset | 对应的 FA 资产引用 | `FA_Rune_AttackUp` |

---

### 2.2 BackpackGridComponent（BGC）

符文系统的**状态管理中心**，挂载在玩家身上。

**核心职责：**
1. 维护 5×5 背包格子占用状态
2. 管理热度阶段（Phase 0–3）和激活区大小
3. 根据符文是否在激活区内，自动调用 `StartBuffFlow / StopBuffFlow`
4. 广播热度边沿事件（归零 / 回升）供 FA 监听

**关键委托：**

| 委托 | 触发条件 | 监听节点 |
|------|---------|---------|
| `OnPhaseUpReady` | 热度满 + CanPhaseUp Tag | `BFNode_OnPhaseUpReady` |
| `OnHeatReachedZero` | 热度从 >0 降至 0（Phase>0 守卫） | `BFNode_OnHeatReachedZero` |
| `OnHeatAboveZero` | 热度从 ≤0 升至 >0 | `BFNode_OnHeatReachedZero` |

**激活区逻辑：**

```
Phase 0 → Tier1（中心 1×1）
Phase 1 → Tier2（中心 2×2）
Phase 2 → Tier3（中心 4×4）
Phase 3 → Transcendence（全格）

每次 Phase 变化 → RefreshAllActivations()
  → 重新计算哪些符文在激活区内
  → 不在区内的停止 FA，进入区内的启动 FA
```

**永久符文（PermanentRunes）：**
- 在 BGC 的 `PermanentRunes` 数组里填写 DA（如 `DA_Rune_HeatPhase`）
- `BeginPlay` 自动激活，不占格子，不受激活区限制
- 典型用途：热度升降阶逻辑 FA

---

### 2.3 BuffFlowComponent（BFC）

**执行上下文管理器**，挂载在玩家身上，与 BGC 配合。

**生命周期：**
```
BGC.ActivateRune(rune)
  └─ BFC.StartBuffFlow(FlowAsset, RuneGuid, Giver)
       └─ 启动 FA 实例，BFNode 开始监听事件

BGC.DeactivateRune(rune)
  └─ BFC.StopBuffFlow(RuneGuid)
       └─ FA 实例停止，所有 BFNode.Cleanup() 被调用
            └─ GE 移除、Tag 清理、NiagaraComponent 销毁
```

**共享上下文数据（BFNode 间通信）：**

| 字段 | 写入时机 | 读取节点 |
|------|---------|---------|
| `LastEventContext.DamageCauser` | OnDamageDealt / OnDamageReceived | 所有 Target=DamageCauser 的节点 |
| `LastEventContext.DamageReceiver` | OnDamageDealt | Target=LastDamageTarget 的节点 |
| `LastEventContext.DamageAmount` | OnDamageDealt | BFNode_DoDamage.DamageMultiplier |
| `LastKillLocation` | OnKill | BFNode_SpawnActorAtLocation |
| `ActiveNiagaraEffects` | BFNode_PlayNiagara | BFNode_DestroyNiagara |

---

## 三、节点全览

### 3.1 触发节点（何时执行）

| 节点 | 触发条件 | 输出 Pin | 说明 |
|------|---------|---------|------|
| `BFNode_OnDamageDealt` | 造成伤害时 | Out | `bOncePerSwing=true` 可限制 AOE 每次挥击只触发一次 |
| `BFNode_OnDamageReceived` | 受到伤害时 | Out | 写入 LastEventContext（含 DamageAmount） |
| `BFNode_OnCritHit` | 触发暴击时 | Out | |
| `BFNode_OnKill` | 击杀时 | Out | 写入 LastKillLocation |
| `BFNode_OnDash` | 闪避/冲刺时 | Out | |
| `BFNode_OnBuffAdded` | 任意 FA 启动时 | Out | 监听 BFC.OnBuffFlowStarted |
| `BFNode_OnBuffRemoved` | 任意 FA 停止时 | Out | 监听 BFC.OnBuffFlowStopped |
| `BFNode_OnPhaseUpReady` | 热度满 + LastHit 条件 | OnPhaseUp | 触发升阶流程入口 |
| `BFNode_OnHeatReachedZero` | 热度边沿变化 | OnReachedZero / OnAboveZero | 双向检测，带 Phase>0 守卫 |
| `BFNode_OnPeriodic` | 定时重复 | Tick | `Interval` 配置间隔，`bFireImmediately` 控制首次是否立即触发 |

---

### 3.2 条件节点（是否执行）

| 节点 | 功能 | 输出 Pin |
|------|------|---------|
| `BFNode_HasTag` | 检查目标是否有指定 Tag | True / False |
| `BFNode_CompareFloat` | 浮点数比较（>, >=, ==, <=, <, !=） | True / False |
| `BFNode_CompareInt` | 整数比较 | True / False |
| `BFNode_CheckTargetType` | 判断 LastDamageTarget 是自己/敌人/其他 | Self / Enemy / Other |

**数据读取节点：**

| 节点 | 输出数据引脚 | 说明 |
|------|------------|------|
| `BFNode_GetAttribute` | `CachedValue (Float)` | 读取目标属性当前值 |
| `BFNode_GetRuneInfo` | `bIsActive, StackCount, Level, TimeRemaining` | 通过 Tag 查询 GE 状态 |

---

### 3.3 效果节点（执行什么）

#### BFNode_ApplyAttributeModifier（最常用）

无需 Blueprint GE 资产，直接在节点上配置属性修改。

| 字段 | 说明 |
|------|------|
| `Attribute` | 要修改的属性（下拉选择） |
| `ModOp` | Additive / Multiplicative / Override |
| `Value` | 数值（可连接数据引脚覆盖） |
| `DurationType` | Instant / Infinite / HasDuration |
| `Duration` | 持续秒数（HasDuration 时） |
| `Target` | 施加目标 |
| `StackMode` | None / Unique / Stackable |
| `StackLimitCount` | 最大层数（Stackable） |
| `DynamicAssetTags` | 附加到 GE Spec 的 Tag |
| `PassThroughOwnerTags` | 将 Owner 身上的指定 Tag 透传入 GE Spec |

#### BFNode_ApplyEffect（需要 Blueprint GE 时用）

| 字段 | 说明 |
|------|------|
| `Effect` | Blueprint GE 类引用 |
| `Level` | GE 等级（可连接数据引脚） |
| `Target` | 施加目标 |
| `SetByCallerTag1-3` + `SetByCallerValue1-3` | 最多 3 个 SetByCaller 槽位 |
| `RemoveMode` | AllStacks / OneStack / CustomCount |

**输出数据引脚：** `bGEApplied, GEStackCount, GELevel, GETimeRemaining`

**输入 Pin：** `In`（施加）/ `Remove`（移除）
**输出 Pin：** `Out` / `Failed` / `Removed`

#### 其他效果节点

| 节点 | 功能 |
|------|------|
| `BFNode_GrantGA` | 授予 GA（Cleanup 自动撤销）；输出 `bGAGranted, GALevel` |
| `BFNode_AddTag` | 添加 Loose Tag（Cleanup 自动移除） |
| `BFNode_RemoveTag` | 移除 Tag |
| `BFNode_DoDamage` | 造成伤害（Flat 或 LastDamageAmount × 倍率） |
| `BFNode_PlayNiagara` | 播放粒子特效（Cleanup 自动销毁） |
| `BFNode_DestroyNiagara` | 销毁指定名称的粒子特效 |
| `BFNode_PlayMontage` | 播放动画蒙太奇 |
| `BFNode_SpawnActorAtLocation` | 在 LastKillLocation 生成 Actor |
| `BFNode_FinishBuff` | 终止当前整个 FA |

---

### 3.4 计算节点

| 节点 | 功能 |
|------|------|
| `BFNode_MathFloat` | 浮点运算（+, -, ×, ÷）；Result 可连出 |
| `BFNode_MathInt` | 整数运算 |

---

### 3.5 热度系统专属节点

| 节点 | 功能 |
|------|------|
| `BFNode_IncrementPhase` | Phase++，扩大激活区，授予 Phase Tag |
| `BFNode_DecrementPhase` | Phase--（含 Phase>0 守卫），收缩激活区，移除 Phase Tag |
| `BFNode_PhaseDecayTimer` | 定时器（防重复激活），`Cancel` Pin 可取消 |

---

## 四、数据引脚（Data Pins）

数据引脚是节点间的**值传递通道**，无需额外变量或临时存储。

**输入数据引脚类型：**
- `FFlowDataPinInputProperty_Float` — 浮点输入（可连线或填固定值）
- `FFlowDataPinInputProperty_Int32` — 整数输入

**输出数据引脚类型：**
- `FFlowDataPinOutputProperty_Float` — 浮点输出
- `FFlowDataPinOutputProperty_Int32` — 整数输出
- `FFlowDataPinOutputProperty_Bool` — 布尔输出

**连线规则：**
- 有连线 → 运行时使用上游节点的值
- 无连线 → 使用节点详情面板里填写的默认值

**典型数据流：**
```
[GetRuneInfo] ─→ StackCount ─→ [MathFloat(×10)] ─→ Result ─→ [ApplyAttributeModifier] Value
                 Level       ─→ [ApplyEffect] Level
```

---

## 五、EBFTargetSelector（目标选择器）

所有效果节点都通过 `EBFTargetSelector` 指定作用目标。

| 枚举值 | 含义 | 典型用途 |
|--------|------|---------|
| `BuffOwner` | 符文拥有者（玩家自身） | 加攻击力、加速移动 |
| `BuffGiver` | 符文施加者（通常同上） | 特殊情况 |
| `LastDamageTarget` | 上次伤害目标（被击者） | 给敌人施加 Debuff |
| `DamageCauser` | 伤害来源（攻击者） | 反弹效果 |

> ⚠️ 被动常驻符文（Passive）使用 `LastDamageTarget` 会因没有伤害目标而 Failed，**必须改为 `BuffOwner`**。

---

## 六、符文开发标准流程

### Step 1：明确设计

在开始创建资产前确认：

| 问题 | 答案示例 |
|------|---------|
| 触发时机是什么？ | 被动 / OnHit / OnKill / OnDamageReceived |
| 效果作用于谁？ | 自己 / 目标敌人 |
| 效果持续多久？ | Instant / Infinite / 有时限 |
| 是否需要堆叠？ | None / Unique / Stackable |
| 是否需要 Blueprint GE？ | 需要 SetByCaller / Cue / Tag 授予时才需要 |

---

### Step 2：创建 FA（Flow Asset）

路径：`Content/BuffFlow/FlowAssets/`
命名：`FA_Rune_<功能名>`

**被动常驻（Passive）模板：**
```
[Start] ──→ [ApplyAttributeModifier]
              Attribute = Attack
              Value     = 10
              Duration  = Infinite
              Target    = BuffOwner
```

**触发型模板：**
```
[Start] ──→ [OnDamageDealt]
                  Out ──→ [HasTag] Tag=Buff.Status.Heat.Phase.2, Target=BuffOwner
                              True  ──→ [ApplyAttributeModifier] Value=20
                              False ──→ [ApplyAttributeModifier] Value=10
```

**受伤暂停型模板（利用 OngoingTagRequirements 自动 Inhibit）：**
```
[Start] ──→ [ApplyEffect] GE_Rune_HeatTick（已配置 OngoingTagRequirements.IgnoreTags=Buff.Status.HeatInhibit）
[OnDamageReceived] ──→ [ApplyEffect] GE_HeatInhibit（HasDuration=5s，授予 Buff.Status.HeatInhibit）
```

---

### Step 3：创建 DA（DataAsset）

路径：`Content/BuffFlow/RuneData/`
命名：`DA_Rune_<功能名>`

必填字段：

```
RuneName     = "攻击强化"
RuneID       = 1001
RuneType     = Buff
Shape.Cells  = [(0,0)]        ← 1×1 格子
Flow.FlowAsset = FA_Rune_AttackUp
```

---

### Step 4：测试验证

1. 将 DA 拖入背包格子激活区
2. **GAS 调试器**（编辑器 → GAS Debugger）验证属性变化
3. 检查 `Output Log` 是否有 `[Warning]` 或 `Failed` 输出
4. 将符文移出激活区，验证 Cleanup 是否正确移除效果

---

## 七、常见设计模式

### 模式 A：被动属性加成（最常见）

```
[Start] → [ApplyAttributeModifier: Attack+10, Infinite, BuffOwner]
```
FA 停止时 Cleanup 自动移除 GE。

---

### 模式 B：条件触发型

```
[Start] → [OnKill]
               → [HasTag: Buff.Status.Heat.Phase.2]
                    True  → [ApplyAttributeModifier: Attack+20, Instant]
                    False → [ApplyAttributeModifier: Attack+10, Instant]
```

---

### 模式 C：堆叠型（击中叠加）

```
[Start] → [OnDamageDealt]
               → [ApplyAttributeModifier: Speed+5, HasDuration 3s, Stackable, Max=5]
```
GE 设为 Stackable，每次命中加一层，最多 5 层，每层 3 秒到期移除一层。

---

### 模式 D：一次性触发后消失

```
[Start] → [OnKill]
               → [ApplyEffect: GE_AttackBoost] ← Remove Pin 接 OnDamageDealt
           [OnDamageDealt] → [ApplyEffect.Remove]
                                 Removed → （已移除，FA 继续运行等待下次击杀）
```

---

### 模式 E：周期性效果（每秒 +N）

使用 Blueprint GE，配置：
- `DurationPolicy = Infinite`
- `Period = 1.0s`（或 0.1s 平滑版）
- `Modifier: Attribute=Heat, +1`

```
[Start] → [ApplyEffect: GE_HeatTick]
```

---

### 模式 F：受伤暂停型

```
GE_HeatTick.OngoingTagRequirements.IgnoreTags = Buff.Status.HeatInhibit

[Start] → [ApplyEffect: GE_HeatTick]
[OnDamageReceived] → [ApplyEffect: GE_HeatInhibit（HasDuration=5s，授予 HeatInhibit Tag）]
```
受伤时自动 Inhibit GE_HeatTick 5 秒，无需在 FA 里手动 Remove 和重新 Apply。

---

### 模式 G：递归守卫（防止效果触发自身）

```
[OnDamageDealt]
  → [HasTag: Buff.Status.ExtraDamageApplied, Target=DamageCauser]
        False → [AddTag: Buff.Status.ExtraDamageApplied]
                → [DoDamage: ×0.5]
                → [RemoveTag: Buff.Status.ExtraDamageApplied]
        True  → （跳过，防止循环）
```

---

## 八、Tag 使用规范（与 Buff_Tag_Spec.md 对照）

| 场景 | Tag 层 | 填写位置 |
|------|--------|---------|
| 符文背包 UI 分类 | `Buff.Rune.Type.*` | DA → RuneConfig（Identity） |
| GE 行为描述（供查询） | `Buff.Effect.*` | Blueprint GE AssetTags |
| 角色运行时状态 | `Buff.Status.*` | GE GrantedTags 或 AddTag 节点 |
| SetByCaller 传值 | `Buff.Data.*` | GE SetByCaller 槽位 |

**BFNode_ApplyAttributeModifier 不需要 GE 资产，不需要 Effect Tag。** Effect Tag 只在需要通过 Tag 查询 GE 时才用（配合 BFNode_GetRuneInfo）。

---

## 九、GAS vs FA 职责划分

| 职责 | 由谁负责 | 原则 |
|------|---------|------|
| 属性修改、GE 施加与移除 | GAS / GE | GE 就是效果本体 |
| Tag 授予与自动清理 | GAS GE GrantedTags | GE 失效时自动撤销 |
| 触发时机判断 | FA 触发节点 | `OnKill, OnHit, OnDamageReceived` 等 |
| 条件分支逻辑 | FA 条件节点 | `HasTag, CompareFloat, CompareInt` |
| SetByCaller 值填入 | FA（BFNode_ApplyEffect 的槽位） | 运行时动态值 |
| GA 授予与撤销 | FA（BFNode_GrantGA） | Cleanup 自动撤销 |
| 特效播放与销毁 | FA（PlayNiagara / DestroyNiagara） | Cleanup 自动销毁 |
| 复杂命中判定 | Blueprint GA + AbilityTask | FA 无法覆盖 |

---

## 十、开发现状与计划

### 已完成

| 模块 | 状态 | 说明 |
|------|------|------|
| BFNode 基础框架（34个节点） | ✅ 完成 | 触发/条件/效果/数学/热度全覆盖 |
| BFNode_ApplyEffect 输出引脚 | ✅ 完成 | bGEApplied, StackCount, Level, TimeRemaining |
| BFNode_ApplyEffect Remove 引脚 | ✅ 完成 | AllStacks / OneStack / CustomCount |
| BFNode_GrantGA 输出引脚 | ✅ 完成 | bGAGranted, GALevel |
| 热度升降阶系统 | ✅ 完成 | 含 CanPhaseUp 三重守卫 |
| RuneDataAsset 简化 | ✅ 完成 | 只保留 RuneType + RuneID |
| BuffFlowComponent 清理 | ✅ 完成 | 移除 ActiveRuneDAs / GetRuneDataAssetByFlow |
| Tag 规范文档 | ✅ 完成 | 四层模型，含 DevComment 规范 |
| GEConfigDA | ❌ 废弃 | 方案被回退，不再使用 |

---

### 近期待开发（优先级排序）

#### P0：热度符文 2 需要的基础设施

**① BFNode_ApplyAttributeModifier 扩展 — Period / RatePerSecond**

当前 ApplyAttributeModifier 没有周期触发支持，无法实现"每秒 +N 热度"模式。

需增加字段：
- `EGEValueMode ValueMode`（Direct / RatePerSecond）
- `float ValuePerSecond`（RatePerSecond 模式下使用，内部自动换算 Period=0.1s）

**② BFNode_GrantTag（新节点）**

向目标施加 Loose Gameplay Tag，FA 停止时 Cleanup 自动移除。用于受伤暂停机制中替代独立 Blueprint GE（GE_HeatInhibit 可用此节点代替）。

**③ BFNode_Delay（新节点）**

等待 N 秒后触发 Out。与 GrantTag 配合实现"受伤后 X 秒内暂停某效果"模式。

---

#### P1：框架扩展性

**④ BFNode_Base 开放蓝图继承（Blueprintable）**

将 `GetOwnerASC()`、`ResolveTarget()`、`BPTriggerOutput()` 标记为 `BlueprintCallable`，允许设计师通过 Flow Node Blueprint 创建自定义节点，无需 C++ 编译。

适用场景：
- 需要临时自定义 GE 逻辑但不想写 C++
- 原型阶段快速验证效果

---

#### P2：战斗能力类节点

**⑤ BFNode_SendGameplayEvent**

发送 Gameplay Event（FGameplayEventData）给目标 ASC。可用于触发监听特定事件的 GA。

**⑥ BFNode_ActivateAbilityByTag**

强制激活目标身上带有指定 Tag 的 GA（`TryActivateAbilityByTag`）。

---

#### P3：体验完善

**⑦ 升阶 CanPhaseUp 检查移入 FA**

当前 BFNode_OnPhaseUpReady 内部有 CanPhaseUp Tag 检查。
改为：节点纯粹广播"热度满了"，FA 里加 `HasTag` 节点判断是否允许升阶，逻辑对设计师可见。

---

### 待开发符文

| 符文 | 触发 | 效果 | 关键技术点 |
|------|------|------|-----------|
| 符文 1（AttackUp）| Passive | 攻击力 +X，Infinite | ✅ 已完成测试 |
| 符文 2（HeatTick）| Passive | 每秒 +1 热度，受伤 5s 暂停 | 需要 Period 扩展 + GrantTag 节点 |
| 符文 3（SpeedStack）| OnHit | 命中叠加移速，Stackable | 模式 C，已有节点支持 |
| 符文 4（Knockback）| OnHit | 击退，SetByCaller 传力度 | 已有 SetByCaller 支持 |
| 符文 5（Bleed）| OnHit | 施加流血 GE → 触发 GA | Blueprint GE + GrantGA |
| 符文 6（ExtraDamage）| OnDamageDealt | 附加伤害，递归守卫 | 模式 G，AddTag 守卫 |

---

## 十一、调试工具

### GAS Debugger
编辑器菜单：`Window → Gameplay Debugger`（或 `'` 键）

检查内容：
- 角色身上的 Active GE 列表（可验证 GE 是否施加）
- 属性当前值（Attack, Heat, MoveSpeed 等）
- 身上的 Tag 列表（可验证 GrantedTags 是否生效）

### Output Log 关键词
- `[ApplyAttrMod] FAILED` — 检查 Target 选择器和 ASC 是否存在
- `Failed` Pin 被触发 — FA 中 Failed 引脚连接 Log 节点辅助定位

### 常见问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 被动符文没有效果 | Target 默认是 `LastDamageTarget`，被动符文无伤害目标 | 改为 `BuffOwner` |
| GE 没有效果 | Blueprint GE 的 Attribute 填错，或 AttributeSet 没有注册此属性 | 检查 GAS Debugger 属性值 |
| FA 停止后 GE 未移除 | bFinish=true 导致节点提前退出，Cleanup 未被调用 | 确认 TriggerOutput 的 bFinish=false |
| 堆叠 GE 不叠加 | GE class 每次 NewObject 导致 Def 指针不同 | 使用 CachedGE 复用同一实例 |
| PassThroughOwnerTags 无效 | .cpp 只有前向声明 | 在 .cpp 中 `#include YogAbilitySystemComponent.h` |

---

## 十二、文件清单

### FA 资产
| 资产 | 路径 | 说明 |
|------|------|------|
| `FA_Rune_HeatPhase` | Content/BuffFlow/ | 热度升降阶（永久符文） |
| `FA_Rune_HeatOnHit` | Content/BuffFlow/ | 命中加热 |
| `FA_Rune_AttackUp` | Content/BuffFlow/ | 攻击力加成（测试） |

### C++ 节点文件
所有节点头文件位于 `Source/DevKit/Public/BuffFlow/Nodes/`
所有实现文件位于 `Source/DevKit/Private/BuffFlow/Nodes/`

### 关联设计文档
| 文档 | 内容 |
|------|------|
| `Buff_Tag_Spec.md` | Tag 命名规范、四层模型 |
| `HeatSystem_Design.md` | 热度系统详细设计 |
| `LevelSystem_ProgrammerDoc.md` | 关卡系统接入点 |
