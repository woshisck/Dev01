# RuneDataAsset 使用指南

> 更新日期：2026-04-05 (rev3)  
> 对象：策划 + 程序  
> FA 节点详细说明见 [BuffFlow_Guide.md](BuffFlow_Guide.md)

---

## 一、概念说明

**DA_Rune（RuneDataAsset）是游戏中符文的数据层。**

DA 只负责：
- 展示信息（名称/图标/描述）
- GE 行为规则（DurationType / Stack / UniqueType）
- 数值效果（Effects[]：属性修改 / 状态 Tag）
- 背包格子形状
- 指向哪个 FA（逻辑由 FA 负责，DA 不写逻辑）

> **GA 的授予不在 DA 里配置。** 在 FA 里使用 `Grant GA` 节点完成。

---

## 二、DA 结构一览

```
DA_Rune_XXX
  ├── Rune Name / Icon / Description          ← 展示信息（顶层平铺）
  ├── Shape                                   ← 背包格子形状
  ├── ▼ Rune Config                           ← GE 行为 + 效果列表
  │     ├── Rune Type         增益/减益/无（UI 分类）
  │     ├── Duration Type     瞬发/永久/有时限
  │     ├── Duration          秒数（仅"有时限"时可填）
  │     ├── Period            周期触发间隔（DoT/HoT，0=不周期）
  │     ├── Unique Type       唯一/源唯一/非唯一
  │     ├── Stack Type        刷新/叠加/禁止（仅"唯一/源唯一"时显示）
  │     ├── Max Stack         最大层数（仅"叠加"时显示）
  │     ├── Stack Reduce Type 全部/逐一（仅"刷新/叠加"时显示）
  │     ├── Rune ID           数值 ID（策划表引用）
  │     ├── Rune Tag          GameplayTag（唯一标识此符文 GE）
  │     └── Effects[]         效果片段，点击 + 添加
  └── ▼ Flow
        └── Flow Asset        FA 资产（符文激活时启动，卸下时停止）
```

---

## 三、展示信息

| 字段 | 类型 | 说明 |
|---|---|---|
| `Rune Name` | FName | 符文名称 |
| `Rune Icon` | Texture2D | 背包 UI 图标 |
| `Rune Description` | FText | Tooltip 描述 |
| `Shape` | FRuneShape | 背包格子形状，Cells 填相对于锚点的格子偏移列表 |

---

## 四、Rune Config 详解

### Rune Type（符文类型）

| 值 | 说明 |
|---|---|
| 增益 | 对自身/友方有益的效果，UI 显示蓝色/正向图标 |
| 减益 | 对敌方施加的负面效果，UI 显示红色/负向图标 |
| 无 | 功能性符文（如装备被动），无明确方向性 |

仅用于 UI 分类显示，不影响 GE 构建。

---

### Duration Type（持续时间类型）

| 值 | GE 类型 | 说明 |
|---|---|---|
| **瞬发** | Instant | 立即触发一次 Effect，不留存（常配合 `Period=0`） |
| **永久** | Infinite | 无限持续，需手动移除（装备类符文） |
| **有时限** | HasDuration | 按 `Duration` 秒数持续后自动移除 |

> **Duration 字段**仅在"有时限"时可见（`ClampMin=0.01`）。

---

### Period（周期触发间隔）

`Period > 0` 时，GE 每隔指定秒数触发一次 Effects 中的 AttributeModifier（GAS 内置机制）。

| 配置 | 行为 |
|---|---|
| `Period = 0` | Effects 只在 GE 施加/移除时触发（属性持续修改型） |
| `Period = 1.0` | 每秒触发一次 Effects（适用于每秒流血/中毒/回血） |

> DoT 配置示例：`Duration = 8s, Period = 1.0, Effects[0] = Health -15`  
> → 8 秒内每秒造成 15 点伤害，共 8 次。

---

### Unique Type（唯一性类型）

控制同名 GE 被多次施加时的聚合方式：

| 值 | GAS 对应 | 说明 |
|---|---|---|
| **唯一** | AggregateByTarget | 目标上只存在一个 GE 实例，配合 Stack Type 使用 |
| **源唯一** | AggregateBySource | 每个施加者在目标上各有一个独立实例 |
| **非唯一** | None | 每次施加都是完全独立的 GE，Stack Type 不生效 |

---

### Stack Type / Max Stack / Stack Reduce Type

仅在 `Unique Type = 唯一 / 源唯一` 时生效。

**Stack Type（堆叠方式）：**

| 值 | 含义 | MaxStack |
|---|---|---|
| **刷新** | 不增加层数，每次施加重置计时 | 固定 1 层 |
| **叠加** | 每次施加 +1 层（至 MaxStack），重置计时 | 由 Max Stack 字段控制 |
| **禁止** | 不叠加，不刷新时间 | 固定 1 层 |

**Stack Reduce Type（到期减层，仅刷新/叠加时显示）：**

| 值 | 含义 |
|---|---|
| **全部** | 时间到期时一次性移除所有层数 |
| **逐一** | 时间到期时只移除 1 层，并刷新剩余层的计时 |

---

### 常用配置组合速查

```
永久被动（装备时持续）：
  Duration Type = 永久, Unique Type = 唯一, Stack Type = 刷新

叠加 Buff（狂暴：每次命中+1层，最多10层，逐层消退）：
  Duration Type = 有时限, Duration = 5.0s
  Unique Type = 唯一, Stack Type = 叠加
  Max Stack = 10, Stack Reduce Type = 逐一

刷新 Buff（护盾：再次触发重置计时）：
  Duration Type = 有时限, Duration = 3.0s
  Unique Type = 唯一, Stack Type = 刷新

一次性爆发（施加即生效，不留存）：
  Duration Type = 瞬发, Unique Type = 非唯一

DoT（中毒：每秒-15 HP，持续8秒）：
  Duration Type = 有时限, Duration = 8.0s, Period = 1.0
  Unique Type = 唯一, Stack Type = 叠加, Max Stack = 5
  Effects[0] = Attribute Modifier(Health, Additive, -15)
```

---

### Rune Tag

用于标识这个 GE 的 GameplayTag。

**两个用途：**
1. `GetRuneInfo` 节点按此 Tag 查询 GE 的运行时状态（层数、剩余时间等）
2. `RemoveRune` 节点按此 Tag 移除目标上的 GE

**注意 RuneTag 与 AddTags Fragment 的区别：**
- `RuneTag` = GE 的**身份标签**（"我是谁"）
- `Add Gameplay Tags` Fragment = **授予目标的状态标签**（"你现在处于中毒状态"）

两者同时存在，用途不同。

---

## 五、Effects[] 效果片段

点击 `+` 选择类型，同一 DA 可叠加多个：

### Add Attribute Modifier（属性修改）

最常用的效果类型，直接在编辑器里选属性、操作符、填数值。

| 子字段 | 说明 |
|---|---|
| `Attribute` | 要修改的属性（下拉选择，来自各 AttributeSet） |
| `Mod Op` | Additive（加）/ Multiplicative（乘）/ Override（覆盖） |
| `Value` | 数值 |

**示例：**
```
Attack +20          → Attribute=Attack,         ModOp=Additive,       Value=20
攻速 ×1.1          → Attribute=AttackSpeed,    ModOp=Multiplicative, Value=0.1
击退力 +600         → Attribute=KnockbackForce, ModOp=Additive,       Value=600
每周期 -15 HP       → Attribute=Health,         ModOp=Additive,       Value=-15  （配合 Period 使用）
```

---

### Add Gameplay Tags（授予状态 Tag）

GE 激活期间给目标授予 GameplayTag，GE 移除时自动撤销。

```
Tags: State.Berserk.Active
→ 其他 GA/GE/FA 可通过 HasTag(State.Berserk.Active) 判断当前状态
```

---

### Gameplay Cue / Advanced Modifier / Execution Calculation

高级用法，不熟悉 GAS 的策划保持空，联系程序配置。

---

## 六、Flow（逻辑层）

| 字段 | 说明 |
|---|---|
| `Flow Asset` | FA 资产（FlowAsset）。符文激活时自动启动，卸下时自动停止 |

FA 里的逻辑**不读取 DA 的配置字段**。  
FA 通过 `Get Rune Info` 节点查询 ASC 上 GE 的运行时状态（层数、剩余时间等）。  
GA 授予通过 FA 里的 `Grant GA` 节点完成（FA 停止时自动撤销）。

详见 [BuffFlow_Guide.md](BuffFlow_Guide.md)。

---

## 七、常见符文配置速查

| 效果类型 | 配置位置 |
|---|---|
| 纯数值加成（Attack +20） | Effects → Add Attribute Modifier |
| 击退力增强 | Effects → Add Attribute Modifier（KnockbackForce）|
| 装备时授予被动 GA | Flow Asset → FA → GrantGA 节点 |
| 激活期间标记状态 | Effects → Add Gameplay Tags |
| 叠加 Buff（狂暴） | Unique Type=唯一, Stack Type=叠加, Max Stack=N, Stack Reduce Type=逐一 |
| 持续 DoT | Period=1.0 + Effects(Health -N) |
| 可视化条件逻辑 | Flow Asset（FA 里用 GetRuneInfo + PlayNiagara）|

---

## 八、变更记录

### rev3（2026-04-05）
- `BuffDuration: float`（0/-1/>0 编码）→ `DurationType` 枚举 + `Duration` float
- 新增 `Period` 字段（周期触发间隔）
- 新增 `UniqueType` 字段（唯一/源唯一/非唯一，映射 GAS StackingType）
- `RuneType` 从 `FRuneInstance` 移入 `FRuneConfig`
- 字段重命名：`BuffID/BuffTag/BuffFlowAsset` → `RuneID/RuneTag/FlowAsset`
- 枚举重命名：`ERuneBuffType` → `ERuneType`

### rev2（2026-04-05）
- `Trigger Gameplay Ability` Fragment 从 DA 移除，改用 FA 的 `Grant GA` 节点
- `BuffFlowAsset` 节点字段更名为 `FlowAsset`
- 新增 `KnockbackForce` 属性（RuneAttributeSet）

### 策划迁移清单（rev3）
- [ ] 所有 DA_Rune：`Buff Duration float` 值 → 选择对应的 `Duration Type` 枚举
- [ ] 所有 DA_Rune：`Buff Tag` → `Rune Tag`（内容不变，字段名已改）
- [ ] 所有 DA_Rune：在 Effects[] 中的旧 `Trigger GA Fragment` → 迁移到 FA 的 `Grant GA` 节点
