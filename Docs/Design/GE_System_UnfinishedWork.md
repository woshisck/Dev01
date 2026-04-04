# GE 效果系统 — 已完成 GE 详解 + 编辑器待做清单

> 更新日期：2026-04-04  
> 源文件：`Source/DevKit/Public/AbilitySystem/GameplayEffect/RuneStatEffect.h`  
> 源文件：`Source/DevKit/Private/AbilitySystem/GameplayEffect/RuneStatEffect.cpp`

---

## 一、GE 基类继承结构

```
UGameplayEffect
  └── UYogGameplayEffect
        └── UPowerUpEffect              ← Duration = Infinite（常驻）
              └── URuneStatEffect_Base  ← 符文数值 GE 基类（无额外逻辑）
                    └── 各具体 GE 类
```

**关键点：** `UPowerUpEffect` 已将 `DurationPolicy = Infinite`，所有子类不需要再设。  
行为类 GE（授予 Tag 的）直接继承 `URuneStatEffect_Base`；有时限的 Buff GE 继承 `UPowerUpEffect` 并覆盖 Duration。

---

## 二、辅助宏说明

```cpp
#define ADD_MODIFIER(AttributeGetter, Op, Value)
```

等价于在 GE 的 `Modifiers` 数组里加一条：
- **Attribute**：要改的属性（如 `GetAttackAttribute()`）
- **Op**：`Additive`（加法）、`Multiplicative`（乘法）、`Override`（覆盖）
- **Value**：固定数值（`FScalableFloat`）

---

## 三、已完成的 GE 详解

### 3.1 通用数值 GE（静态数值，装备时 Apply / 卸下时 Remove）

这些是符文 DA 里 `AttributeModifiers` 数组对应的 GE 类，供 `BackpackGridComponent` 动态构建用，**也可以作为 `BehaviorEffect` 直接引用**。

| 类名 | 作用 | 数值 | Op |
|---|---|---|---|
| `URuneStatEffect_Attack` | 攻击 +20 | 20.0 | Additive |
| `URuneStatEffect_AttackPower` | 伤害倍率 +0.1（即+10%） | 0.1 | Additive |
| `URuneStatEffect_MaxHealth` | 最大生命 +50 | 50.0 | Additive |
| `URuneStatEffect_AttackSpeed` | 攻击速度 +0.1 | 0.1 | Additive |
| `URuneStatEffect_MoveSpeed` | 移动速度 +50 | 50.0 | Additive |
| `URuneStatEffect_CritRate` | 暴击率 +5% | 0.05 | Additive |
| `URuneStatEffect_CritDamage` | 暴击伤害 +20% | 0.2 | Additive |
| `URuneStatEffect_DmgTaken` | 受伤加成 +10%（负面！慎用） | 0.1 | Additive |
| `URuneStatEffect_Dodge` | 闪避率 +5% | 0.05 | Additive |
| `URuneStatEffect_AttackRange` | 攻击范围 +50 | 50.0 | Additive |

> **使用方式**：在 `DA_Rune_xxx` 的 `AttributeModifiers` 里选择对应属性填数值即可，不需要手动引用这些类。`BackpackGridComponent.ActivateRune` 会自动构建。

---

### 3.2 振奋 —— `UGE_Rune_ZhenFen`（好战符文的叠加 Buff）

**用途**：好战符文的 Flow 里，每次命中目标时施加。叠加最多5层，每层提供+1攻击，持续3秒不刷新计时。

```
Duration: HasDuration, 3.0s
Modifier: Attack +1（Additive）
Stacking:
  - StackingType = AggregateByTarget（同一目标上叠加）
  - StackLimitCount = 5（最多5层）
  - StackDurationRefreshPolicy = NeverRefresh（加层不刷新计时）
  - StackExpirationPolicy = RemoveSingleStackAndRefreshDuration（到期移除1层并刷新剩余层）
```

**编辑器操作**：不需要创建 Blueprint，C++ 已全部配置完毕。Flow Graph 里用 `BFNode_ApplyEffect` 施加此 GE 类即可。

---

### 3.3 分离一击 —— `UGE_Rune_FenLiYiJi`（行为 GE）

**用途**：符文装备时施加，授予玩家 `Rune.FenLiYiJi.Active` Tag。Flow 和蓝图用这个 Tag 检测符文是否激活，触发"末招额外弹射"逻辑。

```
Duration: Infinite（继承自 URuneStatEffect_Base → PowerUpEffect）
GrantedTags: Rune.FenLiYiJi.Active
```

**编辑器操作**：
1. 在 `DA_Rune_FenLiYiJi` 的 `BehaviorEffect` 字段，选择 `GE_Rune_FenLiYiJi`（C++ 类）
2. GameplayTag 需注册：`Rune.FenLiYiJi.Active`（见第五章）

**触发逻辑在哪里**：攻击能力蓝图检测 `Action.Combo.LastHit` Tag + `Rune.FenLiYiJi.Active` Tag 同时存在时，触发弹射。

---

### 3.4 突袭 —— `UGE_Rune_TuXi`（行为 GE）

**用途**：符文装备时施加，授予 `Rune.TuXi.Active` Tag。Flow 里监听 Dash 事件，Dash 后下一次攻击附加突袭伤害。

```
Duration: Infinite
GrantedTags: Rune.TuXi.Active
```

**编辑器操作**：
1. `DA_Rune_TuXi` → `BehaviorEffect` = `GE_Rune_TuXi`
2. `BuffFlowAsset` = 新建 `FA_Rune_TuXi`（见 FlowAsset 搭建部分）
3. GameplayTag 注册：`Rune.TuXi.Active`

---

### 3.5 双重打击 —— `UGE_Rune_ShuangChongDaJi`（行为 GE）

**用途**：授予 `Rune.ShuangChongDaJi.Active` Tag，激活后 Flow 监听普攻命中，有概率触发第二次伤害。

```
Duration: Infinite
GrantedTags: Rune.ShuangChongDaJi.Active
```

**编辑器操作**：
1. `DA_Rune_ShuangChongDaJi` → `BehaviorEffect` = `GE_Rune_ShuangChongDaJi`
2. `BuffFlowAsset` = 新建 `FA_Rune_ShuangChongDaJi`
3. GameplayTag 注册：`Rune.ShuangChongDaJi.Active`、`State.DoubleHit`

---

### 3.6 风行者 —— `UGE_Rune_FengXingZhe`（行为 GE）

**用途**：授予 `Rune.FengXingZhe.Active` Tag，激活后启动冲刺蓄力系统（`InitDashChargeSystem`），冲刺冷却结束时授予 `GE.DashCoolDown` Tag 触发额外效果。

```
Duration: Infinite
GrantedTags: Rune.FengXingZhe.Active
```

**编辑器操作**：
1. `DA_Rune_FengXingZhe` → `BehaviorEffect` = `GE_Rune_FengXingZhe`
2. `BuffFlowAsset` = 新建 `FA_Rune_FengXingZhe`
3. **关键**：`BackpackGridComponent` 激活此符文时需调用 `InitDashChargeSystem()`  
   → 在 `OnRuneActivationChanged` 委托里蓝图判断 Guid，或在 C++ `ActivateRune` 里处理
4. GameplayTag 注册：`Rune.FengXingZhe.Active`、`GE.DashCoolDown`

---

### 3.7 滑行速度增益 —— `UGE_Buff_SlideSpeed`（临时 Buff）

**用途**：风行者符文 Flow 里，冲刺结束后施加，提供3秒移速加成（滑行感）。

```
Duration: HasDuration, 3.0s
Modifier: MoveSpeed +120（≈ 基础速度 600 的 20%）
```

**编辑器操作**：在 `FA_Rune_FengXingZhe` 的 Flow Graph 里，`BFNode_OnDash` → `BFNode_ApplyEffect`（选 `GE_Buff_SlideSpeed`）。

---

### 3.8 蛇咬中毒 —— `UGE_Buff_Poison_Rune`（周期性伤害 Buff）

**用途**：蛇咬符文的毒池（`BP_PoisonPool`）对进入范围的敌人施加，5秒内每秒造成目标最大生命 2% 的伤害。

```
Duration: HasDuration, 5.0s
Period: 1.0s（每秒 Tick）
bExecutePeriodicEffectOnApplication = false（不在施加瞬间触发）
Modifier: Health += Target.MaxHealth × (-0.02)
  → 用 AttributeBasedFloat 动态捕获目标 MaxHealth
Stacking:
  - StackingType = AggregateBySource（同一来源的毒不叠伤害）
  - StackLimitCount = 1
  - StackDurationRefreshPolicy = RefreshOnSuccessfulApplication（重叠时刷新计时）
```

**编辑器操作**：
1. 创建蓝图 `BP_PoisonPool`，Overlap 触发时对目标 ASC 施加 `GE_Buff_Poison_Rune`
2. 10秒后自动销毁 Actor（在 BP 里 `SetLifeSpan(10.0f)`）
3. 此 GE 已在 C++ 完整实现，`BP_PoisonPool` 只需调用 `ApplyGameplayEffectToTarget`

---

### 3.9 战斗渴望 —— `UGE_Rune_ZhanDouKewang`（行为 GE）+ `UGE_Buff_ZhanDouKewang`（动态攻速）

**行为 GE（标记符文激活）**：
```
Duration: Infinite
GrantedTags: Rune.ZhanDouKewang.Active
```

**动态攻速 Buff GE**：
```
Duration: Infinite
Modifier: AttackSpeed += SetByCaller(Data.ZhanDouKewang.AttackSpeedBonus)
  → 数值由 Flow 在运行时传入，不写死
```

**编辑器操作**：
1. `DA_Rune_ZhanDouKewang` → `BehaviorEffect` = `GE_Rune_ZhanDouKewang`
2. `BuffFlowAsset` = 新建 `FA_Rune_ZhanDouKewang`
3. Flow 逻辑：`OnDamageDealt` → 计数 → 每5次累计 → `BFNode_ApplyEffect`（`GE_Buff_ZhanDouKewang`，通过 SetByCaller 传攻速值）
4. GameplayTag 注册：`Rune.ZhanDouKewang.Active`、`Data.ZhanDouKewang.AttackSpeedBonus`

---

### 3.10 全能 —— `UGE_Rune_QuanNeng`（行为 GE）+ `UGE_Buff_QuanNeng`（动态暴击率）

**行为 GE（标记符文激活）**：
```
Duration: Infinite
GrantedTags: Rune.QuanNeng.Active
```

**动态暴击率 Buff GE**：
```
Duration: HasDuration, 5.0s（暴击触发后，5秒内暴击率提升）
Modifier: Crit_Rate += SetByCaller(Data.QuanNeng.CritRateBonus)
  → 数值由 Flow 传入
```

**编辑器操作**：
1. `DA_Rune_QuanNeng` → `BehaviorEffect` = `GE_Rune_QuanNeng`
2. `BuffFlowAsset` = 新建 `FA_Rune_QuanNeng`
3. Flow 逻辑：`OnCritHit` → `BFNode_ApplyEffect`（`GE_Buff_QuanNeng`，SetByCaller 传暴击率值）
4. GameplayTag 注册：`Rune.QuanNeng.Active`、`Data.QuanNeng.CritRateBonus`

---

## 四、FlowAsset 搭建参考

下面列出每个需要新建 FlowAsset 的符文的 Flow Graph 结构：

### FA_Rune_TuXi（突袭）
```
[Start]
  ↓
[BFNode_OnBuffAdded]
  ↓
[BFNode_OnDash]            ← 监听冲刺
  ↓
[BFNode_AddTag]            ← 给自身加 State.DashJustUsed（临时标记）
  ↓（回到 OnDash 继续监听）
------ 另一条分支 ------
[BFNode_OnDamageDealt]     ← 监听下一次攻击命中
  → 检查 HasTag(State.DashJustUsed)
      → 是：BFNode_DoDamage（额外伤害）+ RemoveTag(DashJustUsed)
      → 否：跳过
```

### FA_Rune_ShuangChongDaJi（双重打击）
```
[Start]
  ↓
[BFNode_OnBuffAdded]
  ↓
[BFNode_OnDamageDealt]     ← 每次命中都触发（循环）
  ↓
[BFNode_CompareFloat]      ← 随机数 < 0.3（30% 概率）
  ↓（满足）
[BFNode_AddTag]            ← 加 State.DoubleHit（标记本次双重打击）
  ↓
[BFNode_DoDamage]          ← 对相同目标再造一次伤害
  ↓
[BFNode_RemoveTag]         ← 移除 State.DoubleHit
```

### FA_Rune_ZhanDouKewang（战斗渴望）
```
[Start]
  ↓
[BFNode_OnBuffAdded]
  ↓
[BFNode_OnDamageDealt]     ← 每次造成伤害（循环）
  ↓（内部计数器，暂用 Tag 计数替代）
[BFNode_AddTag]            ← AddGameplayTagWithCount(临时Tag, 1)
  ↓
[BFNode_CompareFloat]      ← 检查 Tag 计数 >= 5
  ↓（满足）
[BFNode_ApplyEffect]       ← 施加 GE_Buff_ZhanDouKewang（SetByCaller 传值）
[BFNode_RemoveTag]         ← 清零计数
```

### FA_Rune_QuanNeng（全能）
```
[Start]
  ↓
[BFNode_OnBuffAdded]
  ↓
[BFNode_OnCritHit]         ← 监听暴击事件（循环）
  ↓
[BFNode_ApplyEffect]       ← 施加 GE_Buff_QuanNeng（SetByCaller 传暴击率加成）
```

### FA_Rune_FengXingZhe（风行者）
```
[Start]
  ↓
[BFNode_OnBuffAdded]
  ↓（符文激活，同时在 ActivateRune 回调里调 InitDashChargeSystem）
[BFNode_OnDash]            ← 监听冲刺
  ↓
[BFNode_ApplyEffect]       ← 施加 GE_Buff_SlideSpeed（3s 移速增益）
  ↓（回到 OnDash 继续监听）
------ 停用时 ------
[BFNode_OnBuffRemoved]     ← 调 ShutdownDashChargeSystem
  ↓
[BFNode_FinishBuff]
```

---

## 五、需要在编辑器注册的 GameplayTag

在 `Config/DefaultGameplayTags.ini` 或编辑器 Project Settings → GameplayTags 中添加：

```
Action.Combo.LastHit
Rune.FenLiYiJi.Active
Rune.TuXi.Active
Rune.FengXingZhe.Active
Rune.ShuangChongDaJi.Active
Rune.ZhanDouKewang.Active
Rune.QuanNeng.Active
State.DoubleHit
State.DashJustUsed
GE.DashCoolDown
Data.ZhanDouKewang.AttackSpeedBonus
Data.QuanNeng.CritRateBonus
```

---

## 六、DA_Rune 资产创建速查表

| DA 名称 | BehaviorEffect（C++ 类） | BuffFlowAsset | 备注 |
|---|---|---|---|
| DA_Rune_FenLiYiJi | GE_Rune_FenLiYiJi | 无（蓝图端检测 Tag） | AttackModifiers 填具体数值 |
| DA_Rune_TuXi | GE_Rune_TuXi | FA_Rune_TuXi | |
| DA_Rune_FengXingZhe | GE_Rune_FengXingZhe | FA_Rune_FengXingZhe | |
| DA_Rune_ShuangChongDaJi | GE_Rune_ShuangChongDaJi | FA_Rune_ShuangChongDaJi | |
| DA_Rune_ZhanDouKewang | GE_Rune_ZhanDouKewang | FA_Rune_ZhanDouKewang | |
| DA_Rune_QuanNeng | GE_Rune_QuanNeng | FA_Rune_QuanNeng | |
| DA_Rune_ZhenFen | GE_Rune_ZhenFen | 无（直接被其他 Flow 施加） | 不是独立符文，是好战的子 GE |

> 振奋（ZhenFen）是好战符文 FlowAsset 里 `BFNode_ApplyEffect` 施加的，不是独立背包符文，不需要单独的 DA_Rune。

---

## 七、蓝图端需要配合的工作

### 攻击能力蓝图（分离一击触发条件）
在连击末招蒙太奇 **开始时**：
```
YogASC → AddGameplayTagWithCount(Action.Combo.LastHit, 1)
```
蒙太奇 **结束或中断时**：
```
YogASC → RemoveGameplayTagWithCount(Action.Combo.LastHit, 1)
```

### 分离一击弹射触发（在攻击命中处理里）
```
检查 ASC HasTag(Rune.FenLiYiJi.Active) AND HasTag(Action.Combo.LastHit)
  → 是：触发弹射逻辑（SpawnProjectile 或 DoDamage 到周边敌人）
```
