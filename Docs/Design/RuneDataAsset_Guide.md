# RuneDataAsset 使用指南

> 更新日期：2026-04-05  
> 对象：策划 + 程序

---

## 一、概念说明

**DA_Rune（RuneDataAsset）是游戏中"符文 / Buff / 卡牌"的统一数据源。**  
三者在本系统里是同一个概念：一张 DA 资产描述了一个效果的全部信息——它的外观、行为规则、数值、逻辑。

- 策划创建 DA，填写参数
- 程序通过 `BackpackGridComponent` 或 `BFNode_AddRune` 使用它
- 运行时由系统自动构建 GE、启动 Flow、授予 GA，无需策划操心底层细节

---

## 二、DA 结构一览

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description / Buff Type   ← 展示信息
  ├── Shape                                         ← 背包格子形状
  ├── ▼ Buff Config    Duration / Period / Stacking / Tags
  ├── ▼ Values         属性修改 / 公式联动 / GA参数
  └── ▼ Flow           BuffFlow / 被动GA
```

---

## 三、各字段详解

### 展示信息（顶层）

| 字段 | 说明 |
|---|---|
| `Rune Name` | 符文名称（FName） |
| `Rune Icon` | 图标贴图，背包 UI 显示 |
| `Rune Description` | 描述文本，Tooltip 显示 |
| `Buff Type` | **增益 / 减益 / 无**，供 UI 显示颜色/图标方向 |
| `Shape` | 背包格子占用形状，Cells 填相对于锚点的格子偏移列表 |

---

### Buff Config（Buff 行为配置）

| 字段 | 说明 | 示例 |
|---|---|---|
| `Buff Tag` | 标识该 Buff 的 GameplayTag，用于 EffectRegistry 查找和精确移除 | `Buff.Poison` |
| `Granted Tags to Target` | GE 激活期间授予目标的 Tag，GE 移除时自动撤销 | `Rune.TuXi.Active` |
| `Duration Policy` | 瞬发(Instant) / 有时限(HasDuration) / 永久(Infinite) | |
| `Duration Magnitude` | 持续时间（秒），仅 HasDuration 时显示 | `5.0` |
| `Period` | 周期时长（秒），>0 时变为周期效果（如每秒掉血） | `1.0` |
| `Execute Periodic Effect on Application` | 施加瞬间是否立即触发一次周期效果 | 通常关闭 |
| `Stacking Type` | **None** = 不叠加 / **AggregateByTarget** = 按目标叠加 / **AggregateBySource** = 同来源唯一 | |
| `Stack Limit Count` | 最大堆叠层数 | `5` |
| `Stack Duration Refresh Policy` | 叠加时是否刷新计时：RefreshOnSuccessfulApplication（刷新）/ NeverRefresh | |
| `Stack Expiration Policy` | 到期移除方式：**ClearEntireStack**（全部移除）/ **RemoveSingleStackAndRefreshDuration**（逐层移除） | |

**常用配置组合：**

```
叠加 Buff（振奋：每次命中+1攻击，最多5层，不刷新，到期逐层）：
  Duration Policy = HasDuration, 3.0s
  Stacking Type   = AggregateByTarget
  Stack Limit     = 5
  Duration Refresh= NeverRefresh
  Expiration      = RemoveSingleStackAndRefreshDuration

持续掉血（周期 DoT）：
  Duration Policy = HasDuration, 5.0s
  Period          = 1.0s          ← 每秒触发
  Execute on App  = false         ← 不在施加瞬间触发
  Values.Attribute Modifiers: Health -20 Additive

永久被动（装备时持续生效）：
  Duration Policy = Infinite
  （其余默认）
```

---

### Values（数值配置）

#### Attribute Modifiers — 简化属性修改器

最常用的方式。在编辑器里选择属性、操作类型、填数值，无需写代码。

| 子字段 | 说明 |
|---|---|
| `Attribute` | 要修改的属性（下拉选择） |
| `Mod Op` | Additive（加）/ Multiplicative（乘）/ Override（覆盖） |
| `Value` | 数值 |

**示例：**
```
Attack +20      → Attribute=Attack,    ModOp=Additive,       Value=20
攻速 ×1.1倍    → Attribute=AttSpeed,  ModOp=Multiplicative,  Value=0.1
固定攻速=2.0   → Attribute=AttSpeed,  ModOp=Override,        Value=2.0
```

---

#### Calc Specs — 属性联动公式

当效果值依赖于另一个属性时使用。公式：  
**`result = Op(AttributeA, AttributeB) × Coefficient + Addend`**  
**`OutputAttribute ModOp= result`**

| 子字段 | 说明 |
|---|---|
| `Attribute A` | 第一个属性（必填） |
| `Attribute B` | 第二个属性（运算非 UseA 时填） |
| `Operation` | UseA / A-B / A+B / A×B |
| `Coefficient` | 结果乘以此系数（默认 1.0） |
| `Addend` | 结果加上此常量（默认 0） |
| `Output Attribute` | 写入哪个属性 |
| `Mod Op` | Additive / Multiplicative / Override |

**示例：损失血量 → 攻速加成**
```
Attribute A    = MaxHealth
Attribute B    = Health
Operation      = A - B           ← LostHP = MaxHealth - Health
Coefficient    = 0.01
Output Attr    = AttackSpeed
Mod Op         = Additive
→ AttackSpeed += (MaxHealth - Health) × 0.01
```

> **注意**：数值在 GE 施加时快照。如需实时跟随属性变化，在 Buff Config 里设置 Period（如 0.5s），使 GE 定期重新施加，每次重新计算。

---

#### Params — GA 参数字典

当符文需要授予一个被动 GA（如击退 GA）时，通过此字典向 GA 传递数值参数，避免为每种强度创建多个 GA 蓝图。

**DA 填写：**
```
Params:
  KnockbackStrength → 800.0
  KnockbackZBoost   → 300.0
```

**GA 蓝图读取（在 OnAvatarSet 或 OnGiveAbility 里）：**
```
Get Current Ability Spec
  → Source Object
  → Cast To RuneDataAsset
  → Rune Template → Values → Params
  → Find "KnockbackStrength"  → Set KnockbackStrength
  → Find "KnockbackZBoost"    → Set KnockbackZBoost
```

---

#### Modifiers（高级）

直接配置 GAS 的 `FGameplayModifierInfo`，支持 AttributeBasedFloat（线性属性引用）和 SetByCaller（运行时传值）。

不熟悉 GAS 的策划保持空即可，用 Attribute Modifiers 或 Calc Specs 替代。

---

### Flow（逻辑实现）

| 字段 | 说明 |
|---|---|
| `Buff Flow Asset` | 可视化逻辑资产（FlowAsset）。符文激活时自动启动，卸下时自动停止 |
| `Passive Ability Class` | 符文激活时授予宿主的被动 GA 类。GA 通过 `SourceObject` 读取 `Values.Params` |

---

## 四、常见符文配置速查

| 效果类型 | 配置方式 |
|---|---|
| 纯数值加成（Attack +20） | Values → Attribute Modifiers |
| 基于属性的加成（损失血量→攻速） | Values → Calc Specs |
| 持续掉血（DoT） | Buff Config（Period=1s, Duration=5s）+ Values → AttributeModifiers（Health -X） |
| 叠加 Buff（振奋） | Buff Config（Stacking=AggregateByTarget, StackLimit=5, NeverRefresh） |
| 授予 Tag（行为激活标记） | Buff Config → Granted Tags to Target |
| 带参数的 GA（击退） | Flow → Passive Ability Class + Values → Params |
| 可视化条件逻辑 | Flow → Buff Flow Asset |

---

## 五、系统架构变更记录（2026-04-05）

### 删除
- `UYogBuffDefinition` 类（`.h` + `.cpp`）已删除

### 合并
`UYogBuffDefinition` 的所有功能合并进 `FRuneInstance`，DA_Rune 成为唯一的效果数据源：

| 之前 | 现在 |
|---|---|
| DA_Rune 的 `BehaviorEffect`（C++ GE 类引用） | 改为在 DA 里直接配置 Buff Config + Values |
| `UYogBuffDefinition.BuffFlowAsset` | → `FRuneInstance.Flow.BuffFlowAsset` |
| `UYogBuffDefinition.BuffTag` | → `FRuneInstance.BuffConfig.BuffTag` |
| `BackpackGridComponent` 分别 Apply 两个 GE | → 统一调 `CreateTransientGE()` 一次完成 |

### `EffectRegistry` 变更
`TMap<FGameplayTag, UYogBuffDefinition*>` → `TMap<FGameplayTag, URuneDataAsset*>`

已有的 `DA_EffectRegistry` 资产需要重新填写映射（旧的 BuffDefinition 引用失效）。

### `BFNode_AddRune` / `BFNode_RemoveRune` 变更
节点属性 `BuffDefinition`（UYogBuffDefinition*）改名为 `RuneAsset`（URuneDataAsset*）。

**Flow Graph 中已放置的此类节点需要重新选择 RuneAsset 引用。**

### 新增能力
- `FRuneCalcSpec`：多属性联动公式（A op B → 另一属性）
- `FRuneInstance.Values.Params`：DA 向被动 GA 传递数值参数
- `FRuneInstance.BuffType`：增益/减益/无，供 UI 读取
- `FPlacedRune.GrantedAbilityHandle`：被动 GA 完整的激活/卸下生命周期
