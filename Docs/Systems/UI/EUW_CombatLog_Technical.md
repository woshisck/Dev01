# EUW_CombatLog 技术文档（512 版本）

> 战斗日志编辑器工具的 C++ 系统架构、数据流和接口说明。  
> UI 布局规格见 [WBP_EUW_CombatLog_Layout.md](WBP_EUW_CombatLog_Layout.md)

---

## 系统概览

战斗日志由三层构成：

```
写入层           静态数据桥               显示层
ASC / DeckComponent  →  UCombatLogStatics  ←  UCombatLogWidget (NativeTick 轮询)
```

| 层 | 职责 | 关键类 |
|---|---|---|
| 写入层 | 产生伤害事件和卡牌消耗事件，推入静态桥 | `UYogAbilitySystemComponent`、`UCombatDeckComponent` |
| 静态数据桥 | 跨 PIE / Editor 边界的静态存储，最多 500 条，版本号驱动刷新 | `UCombatLogStatics` |
| 显示层 | Editor Utility Widget 基类，NativeTick 轮询版本号，过滤器 + 日志行 + 摘要统计 | `UCombatLogWidget`、`UCombatFilterProxy` |

---

## 数据结构：FDamageBreakdown

定义位置：`Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h`

### 基础字段（历史版本）

| 字段 | 类型 | 说明 |
|------|------|------|
| `BaseAttack` | float | 来源的 Attack 属性（基础攻击力） |
| `ActionMultiplier` | float | 动作系数（ActDamage Notify 设置） |
| `DmgTakenMult` | float | 目标减伤系数（>1 易伤，<1 减伤） |
| `FinalDamage` | float | 最终伤害（含暴击；消耗行为 0） |
| `bIsCrit` | bool | 是否暴击 |
| `ActionName` | FName | 动作名（"轻击1"、"重击2" 等） |
| `DamageType` | FName | 伤害类型（见下表） |
| `TargetName` | FString | 目标名称（调试用） |
| `SourceName` | FString | 来源 Actor 名（调试用） |
| `GameTime` | float | 伤害发生时的游戏时间（秒） |

### 512 版本新增卡牌字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `bHadCard` | bool | 攻击时有可用卡牌 |
| `bConsumedCard` | bool | 卡牌已被消耗 |
| `bActionMatched` | bool | 动作类型与卡牌 RequiredAction 匹配 |
| `bTriggeredMatchedFlow` | bool | 匹配奖励 Flow 实际触发 |
| `bTriggeredLink` | bool | 连携触发（含正/反向） |
| `bTriggeredFinisher` | bool | 终结技触发 |
| `bStartedShuffle` | bool | 本次消耗后进入洗牌（消耗行有效） |
| `bIsCardEventOnly` | bool | 纯消耗行（FinalDamage=0，无目标） |
| `CardDisplayName` | FName | 卡牌显示名称 |
| `CardConsumeTiming` | FName | 消耗时机（"OnCommit" / "OnHit"） |

### DamageType 枚举值速查

| DamageType | 含义 | 颜色 |
|-----------|------|------|
| `"Attack"` | 普通攻击 | 白色 (1,1,1) |
| `"Attack_Crit"` | 普通暴击 | 黄色 (1,0.9,0.1) |
| `"Bleed"` | 流血 | 红色 (1,0.2,0.1) |
| `"Rune_*"` | 符文（前缀匹配） | 紫色 (0.8,0.3,1) |
| `"Card_Consume"` | 卡牌消耗（无伤害） | 淡蓝 (0.4,0.7,1) |
| `"Card_Shuffle"` | 消耗并触发洗牌 | 灰蓝 (0.5,0.7,1) |
| `"Card_Hit"` | 卡牌命中（普通） | 淡蓝 (0.4,0.7,1) |
| `"Card_Matched"` | 卡牌命中（匹配奖励） | 青色 (0.2,0.9,0.9) |
| `"Card_Link"` | 连携命中 | 橙色 (1,0.55,0.1) |
| `"Card_Finisher"` | 终结技命中 | 金色 (1,0.85,0) |

---

## 过滤器：ECombatLogFilter

定义位置：`Source/DevKit/Public/UI/CombatLogStatics.h`

```
All / Normal / Crit / Rune / Bleed / Card / Finisher / Link / Shuffle
```

**注意**：512 版本新增的 4 个值（Card / Finisher / Link / Shuffle）追加在末尾，蓝图枚举值不能插入中间否则错位。

---

## UCombatLogStatics（静态数据桥）

`Source/DevKit/Public/UI/CombatLogStatics.h`

| 函数 | 说明 |
|------|------|
| `PushEntry(Entry)` | 追加一条记录（超 500 条移除最旧） |
| `GetVersion()` | 版本号，每次 Push/Clear 自增，供 Widget 轮询判断是否需刷新 |
| `GetAllEntries()` | 返回全量记录数组 |
| `ClearEntries()` | 清空（绑定到重置按钮） |
| `GetEntryText(Entry)` | 格式化单条记录文字 |
| `GetEntryColor(Entry)` | 返回单条记录颜色（按 DamageType 优先级） |
| `PassesFilter(Entry, Filter)` | 判断记录是否通过指定过滤器 |

---

## UCombatLogWidget（显示层基类）

`Source/DevKit/Public/UI/CombatLogWidget.h`

### BindWidgetOptional 控件

| 变量名 | 类型 | 说明 |
|--------|------|------|
| `FilterButtonBox` | UWrapBox | 过滤按钮容器；若 Blueprint 未绑定，NativeConstruct 自动在根 Canvas 创建（Top 36px） |
| `LogScrollBox` | UScrollBox | 日志行滚动区域 |
| `SummaryText` | UTextBlock | 摘要统计文字块 |

### Config 属性（Class Defaults 可调）

| 变量名 | 默认值 | 说明 |
|--------|--------|------|
| `LogFontSize` | 11 | 日志行字体大小 |
| `FilterFontSize` | 10 | 过滤按钮标签字体大小 |

### 蓝图可调用函数

| 函数 | 说明 |
|------|------|
| `SetFilter(NewFilter)` | 切换当前过滤器（同 Filter 时跳过） |
| `ResetLog()` | 清空日志 + 归零所有统计 |
| `GetActiveFilter()` | 返回当前激活的过滤器（BlueprintPure） |

### 内部流程

```
NativeConstruct
  └── FilterButtonBox 未绑定 → 自动创建 WrapBox 并挂到根 Canvas (Anchor=0,0,1,0 / H=36)
  └── BuildFilterButtons() — 生成 9 个 UCombatFilterProxy + UButton

NativeTick
  └── GetVersion() != CachedVersion → RebuildLog() + RefreshSummary()

RebuildLog()
  └── 遍历 GetAllEntries()
  └── 按 DamageType 累计 Session 统计数据
  └── PassesFilter → AddLogRow(Entry)

AddLogRow(Entry)
  └── 新建 UTextBlock，SetText/Color/Font
  └── AddChild 到 LogScrollBox
  └── 若滚动已在底部 → ScrollToEnd()
```

---

## UCombatFilterProxy（按钮点击代理）

因 `UButton::OnClicked` 是动态多播委托，不支持 Lambda，所以每个过滤按钮对应一个轻量 `UObject` 子类代理。

```cpp
UCLASS()
class UCombatFilterProxy : public UObject
{
    ECombatLogFilter Filter;
    TWeakObjectPtr<UCombatLogWidget> Owner;
    UFUNCTION() void OnClicked(); // 调用 Owner->SetFilter(Filter)
};
```

---

## 写入路径

### 普通攻击 / 符文 / 流血

`DamageExecution::Execute_Implementation` 构造 `FDamageBreakdown`，调用：

```cpp
SourceASC->LogDamageDealtDetailed(Target, Breakdown);
// → UCombatLogStatics::PushEntry(Breakdown)
```

### 卡牌字段填充

`DamageExecution` 从 `SourceASC->BuffFlowComponent->LastCombatCardEffectContext` 读取卡牌上下文，填入 bTriggeredLink / bTriggeredFinisher / bTriggeredMatchedFlow 等字段，并升级 DamageType（Card_Hit / Card_Matched / Card_Link / Card_Finisher）。

### 卡牌消耗行（无伤害）

`UCombatDeckComponent::ResolveAttackCard` 在消耗成功后，直接调用 `UCombatLogStatics::PushEntry(Consume)` 推入一条 `bIsCardEventOnly=true` 的消耗行（DamageType="Card_Consume" 或 "Card_Shuffle"）。

---

## 已知限制

- FilterButtonBox 自动创建时插入根 Canvas 最顶层（Z-order 最高），可能覆盖其他控件。推荐在 Blueprint Designer 手动添加 WrapBox 并命名为 `FilterButtonBox` 并精确定位。
- 按钮目前无选中态高亮（无法区分当前激活过滤器），UI 优化时可通过 `GetActiveFilter()` 驱动按钮视觉状态。
- 最多 500 条记录，超出时移除最旧条目。
