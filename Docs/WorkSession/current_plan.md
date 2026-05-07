# 开发方案

## 需求描述

战斗日志编辑器界面（`CombatLogWidget` + `CombatLogStatics`）已有基础版本，需要根据 512 版本的新系统（卡牌消耗/连携/终结技）进行优化，并制定后续制作方案。现有系统只理解 Attack / Crit / Bleed / Rune 四类事件，512 版本引入了更丰富的战斗事件需要被记录和展示。

---

## 已确认决策

| # | 问题 | 决策 |
|---|------|------|
| 1 | 卡牌事件时机 | **两条记录**：① 卡牌消耗行（`DamageType="Card_Consume"`，`FinalDamage=0`，记录消耗即触发，包含未命中情况）；② 伤害行（`DamageType` 含卡牌标记，命中时写入） |
| 2 | 多目标命中 | 每个目标各一条伤害行，各自跟一条从属卡牌事件行 |
| 3 | SignatureTrait | **不需要**，512 版本没有特征机制，所有效果直接走 FA Flow |
| 4 | 日志标记符 | **ASCII**：`[CARD]` / `[MATCH]` / `[FIN]` / `[LNK]` / `[SHUFFLE]` |
| 5 | OnDamageBreakdown 委托 | **保留**（DamageBreakdownWidget 移除，但委托保留供未来使用） |

---

## 现状梳理

### 现有组件

| 文件 | 职责 | 状态 |
|------|------|------|
| `CombatLogStatics.h/cpp` | 静态数据桥，最多 500 条，版本号轮询 | 可用，需扩展 |
| `CombatLogWidget.h/cpp` | Editor Utility Widget 基类，5 种过滤器 | 可用，需扩展 |
| `DamageBreakdownWidget.h/cpp` | 委托订阅式面板（游戏内 HUD 用） | **移除** |
| `FDamageBreakdown` struct | BaseAttack / ActionMultiplier / DmgTaken / FinalDamage / bIsCrit / ActionName / DamageType | 需新增字段 |

### 现有 DamageType（FName）

`"Attack"` / `"Attack_Crit"` / `"Bleed"` / `"Rune_XXX"`

### 512 版本新增事件

| 事件类别 | DamageType 值 | 来源 |
|--------|------|------|
| 卡牌消耗（含未命中） | `"Card_Consume"` | `CombatDeckComponent::ResolveAttackCard` 返回 |
| 卡牌命中（普通） | `"Card_Hit"` | `DamageExecution` + `LastCombatCardEffectContext` |
| 卡牌命中（匹配奖励） | `"Card_Matched"` | 同上，`bTriggeredMatchedFlow=true` |
| 连携命中 | `"Card_Link"` | 同上，`bTriggeredLink=true` |
| 终结技命中 | `"Card_Finisher"` | 同上，`bTriggeredFinisher=true` |
| 洗牌事件 | `"Card_Shuffle"` | Resolve 完成后判断 `bStartedShuffle` |

---

## 方案设计

### 设计原则

- **最小改动**：`FDamageBreakdown` 新增字段，均带默认值，向后兼容
- **颜色语义**：ASCII 标记 + 颜色双重区分，不依赖特殊 Unicode
- **移除 DamageBreakdownWidget**：删除 .h/.cpp，清理 #include，保留委托声明
- **数据通道**：`DamageExecution` 读 `SourceASC->BuffFlowComponent->LastCombatCardEffectContext` 填卡牌字段，无需跨组件传参

### 颜色编码（优先级从高到低）

| DamageType | 颜色 | RGB |
|-----------|------|-----|
| `Card_Finisher` | 金色 | (1, 0.85, 0) |
| `Card_Link` | 橙色 | (1, 0.55, 0.1) |
| `Card_Matched` | 青色 | (0.2, 0.9, 0.9) |
| `Card_Hit` / `Card_Consume` | 淡蓝 | (0.4, 0.7, 1) |
| `Card_Shuffle` | 灰蓝 | (0.5, 0.7, 1) |
| `Attack_Crit` | 黄色 | (1, 0.9, 0.1) |
| `Rune_*` | 紫色 | (0.8, 0.3, 1) |
| `Bleed` | 红色 | (1, 0.2, 0.1) |
| `Attack` | 白色 | (1, 1, 1) |

### 日志格式

```
[01:23] 玩家 -> BP_Rat  [轻击2]  25 x 1.20 x 1.00 *CRIT = 30.0
[01:24] 玩家 [CARD] 怒锋入铠 消耗 (OnCommit)
         -> BP_Rat  [重击1] [MATCH] x1.5 = 45.0
[01:25] 玩家 [CARD] 怒锋入铠 消耗 (OnCommit)
         -> BP_Rat  [重击1] [FIN] = 160.0
[01:26]          -> BP_Rat  [LNK] -> 28.0
[01:27] 玩家 [CARD] 怒锋入铠 消耗 [SHUFFLE]
```

**两种行类型**：
- **消耗行**（`Card_Consume` / `Card_Shuffle`）：`FinalDamage=0`，记录卡牌名+消耗时机，无目标
- **伤害行**（`Card_Hit/Matched/Link/Finisher`）：有目标，有伤害值，每目标一行，紧跟消耗行之后缩进显示

### 摘要统计

```
-- 卡牌统计 --
消耗: 9次 | 命中: 12次 | 匹配: 7次 | 连携: 5次 | 终结技: 2次 | 洗牌: 3次
-- 伤害统计 --
普通: 350(18次) | 暴击: 280(6次) | 符文: 120(4次) | 流血: 80(12次)
卡牌命中: 520(12次) | 终结技: 320(2次) | 连携: 90(5次)
总计: 1760
```

---

## 实现步骤

### 步骤 1 — 扩展 FDamageBreakdown（不含 SignatureTrait）

**文件**：`Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h`

在 `FDamageBreakdown` 末尾新增（均有默认值，向后兼容）：

```cpp
// -- 512版本：卡牌字段 --
/** 是否有卡牌在场 */
UPROPERTY(BlueprintReadOnly) bool bHadCard = false;
/** 卡牌已消耗（ConsumedCard.IsValidCard()） */
UPROPERTY(BlueprintReadOnly) bool bConsumedCard = false;
/** 动作类型匹配 */
UPROPERTY(BlueprintReadOnly) bool bActionMatched = false;
/** 匹配奖励 Flow 实际触发 */
UPROPERTY(BlueprintReadOnly) bool bTriggeredMatchedFlow = false;
/** 连携 */
UPROPERTY(BlueprintReadOnly) bool bTriggeredLink = false;
/** 终结技 */
UPROPERTY(BlueprintReadOnly) bool bTriggeredFinisher = false;
/** 洗牌（仅 Card_Consume/Card_Shuffle 行有效） */
UPROPERTY(BlueprintReadOnly) bool bStartedShuffle = false;
/** 卡牌显示名称（调试用） */
UPROPERTY(BlueprintReadOnly) FString CardDisplayName;
/** 消耗时机标记（"OnCommit" / "OnHit" 等，调试用） */
UPROPERTY(BlueprintReadOnly) FName CardConsumeTiming;
/** 是否为纯卡牌消耗行（FinalDamage=0，无目标） */
UPROPERTY(BlueprintReadOnly) bool bIsCardEventOnly = false;
```

### 步骤 2 — 扩展 ECombatLogFilter（追加到末尾）

**文件**：`Source/DevKit/Public/UI/CombatLogStatics.h`

```cpp
// 追加到 Bleed 之后，不能插入中间
Card      UMETA(DisplayName = "卡牌"),
Finisher  UMETA(DisplayName = "终结技"),
Link      UMETA(DisplayName = "连携"),
Shuffle   UMETA(DisplayName = "洗牌"),
```

### 步骤 3 — DamageExecution 填写卡牌字段

**文件**：`Source/DevKit/Private/AbilitySystem/ExecutionCalculation/DamageExecution.cpp`

在 `Execute_Implementation` 构造 `FDamageBreakdown` 后，追加：

```cpp
// 尝试从 SourceASC->BuffFlowComponent 读取卡牌上下文
if (UBuffFlowComponent* BFC = SourceASC ? SourceASC->GetBuffFlowComponent() : nullptr)
{
    if (BFC->HasCombatCardEffectContext())
    {
        const FCombatCardEffectContext& Ctx = BFC->GetLastCombatCardEffectContext();
        Breakdown.bHadCard             = true;
        Breakdown.bConsumedCard        = Ctx.ResolveResult.ConsumedCard.IsValidCard();
        Breakdown.bActionMatched       = Ctx.ResolveResult.bActionMatched;
        Breakdown.bTriggeredMatchedFlow= Ctx.ResolveResult.bTriggeredMatchedFlow;
        Breakdown.bTriggeredLink       = Ctx.ResolveResult.bTriggeredLink
                                      || Ctx.ResolveResult.bTriggeredForwardLink
                                      || Ctx.ResolveResult.bTriggeredBackwardLink;
        Breakdown.bTriggeredFinisher   = Ctx.ResolveResult.bTriggeredFinisher;
        Breakdown.CardDisplayName      = Ctx.SourceCard.Config.DisplayName.ToString();
        // DamageType 根据卡牌字段升级
        if (Breakdown.bTriggeredFinisher)
            Breakdown.DamageType = "Card_Finisher";
        else if (Breakdown.bTriggeredLink)
            Breakdown.DamageType = "Card_Link";
        else if (Breakdown.bTriggeredMatchedFlow)
            Breakdown.DamageType = "Card_Matched";
        else
            Breakdown.DamageType = "Card_Hit";
    }
}
```

### 步骤 4 — CombatDeckComponent 推消耗行

**文件**：`Source/DevKit/Private/Component/CombatDeckComponent.cpp`

在 `ResolveAttackCard` 返回之前，若 `Result.ConsumedCard.IsValidCard()`，推一条消耗行：

```cpp
if (Result.ConsumedCard.IsValidCard())
{
    FDamageBreakdown Consume;
    Consume.bIsCardEventOnly  = true;
    Consume.bHadCard          = true;
    Consume.bConsumedCard     = true;
    Consume.bStartedShuffle   = Result.bStartedShuffle;
    Consume.CardDisplayName   = Result.ConsumedCard.Config.DisplayName.ToString();
    Consume.CardConsumeTiming = Result.ConsumedCard.Config.ConsumeTiming; // FName
    Consume.SourceName        = GetNameSafe(GetOwner());
    Consume.GameTime          = GetWorld()->GetTimeSeconds();
    Consume.DamageType        = Result.bStartedShuffle ? FName("Card_Shuffle") : FName("Card_Consume");
    UCombatLogStatics::PushEntry(Consume);
}
```

### 步骤 5 — 更新 CombatLogStatics

**文件**：`Source/DevKit/Private/UI/CombatLogStatics.cpp`

- `PassesFilter`：Card 过滤器匹配 `DamageType` 前缀 "Card_"；Finisher 匹配 "Card_Finisher"；Link 匹配 "Card_Link"；Shuffle 匹配 "Card_Shuffle"
- `GetEntryColor`：按上表优先级 if-else 判断 DamageType
- `GetEntryText`：消耗行格式 `[MM:SS] SOURCE [CARD] CardName (Timing)`；伤害行在现有基础上追加 `[MATCH]/[FIN]/[LNK]` 前缀
- `GetFormattedSummary`：新增卡牌统计块

### 步骤 6 — 更新 CombatLogWidget.h / .cpp

**文件**：
- `Source/DevKit/Public/UI/CombatLogWidget.h` — 新增计数器成员变量
- `Source/DevKit/Private/UI/CombatLogWidget.cpp` — `RebuildLog` 时累计卡牌统计

新增成员：

```cpp
int32 HitCard = 0, HitConsumed = 0, HitMatched = 0;
int32 HitLink = 0, HitFinisher = 0, HitShuffle = 0;
float SessionCardHit = 0.f, SessionLink = 0.f, SessionFinisher = 0.f;
```

### 步骤 7 — 移除 DamageBreakdownWidget（清理清单）

1. 删除 `Source/DevKit/Public/UI/DamageBreakdownWidget.h`
2. 删除 `Source/DevKit/Private/UI/DamageBreakdownWidget.cpp`
3. 在 `YogAbilitySystemComponent.h` 中更新 `FOnDamageBreakdown` 注释（移除"广播给 DamageBreakdownWidget"字样）
4. 搜索并移除所有 `#include "UI/DamageBreakdownWidget.h"`
5. 在编辑器中检查 Content 内是否有以 `DamageBreakdownWidget` 为父类的 WBP，若有改为 `UserWidget` 或直接删除

### 步骤 8 — 编译验证

使用引擎自带 .NET 6.0 编译 `DevKitEditor` 目标，确认无编译错误。

### 步骤 9 — EUW 蓝图更新（编辑器内）

- `EUW_CombatLog`：新增 Card / Finisher / Link / Shuffle 四个过滤按钮（追加到现有过滤按钮行末）
- 摘要区增加卡牌统计行

---

## 涉及文件

- `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h` — FDamageBreakdown 新增 11 个卡牌字段
- `Source/DevKit/Public/UI/CombatLogStatics.h` — ECombatLogFilter 追加 4 个枚举值（末尾）
- `Source/DevKit/Private/UI/CombatLogStatics.cpp` — PassesFilter / GetEntryColor / GetEntryText / GetFormattedSummary
- `Source/DevKit/Public/UI/CombatLogWidget.h` — 新增卡牌计数器成员
- `Source/DevKit/Private/UI/CombatLogWidget.cpp` — RebuildLog 累计卡牌统计
- `Source/DevKit/Private/AbilitySystem/ExecutionCalculation/DamageExecution.cpp` — 读取 LastCombatCardEffectContext 填卡牌字段
- `Source/DevKit/Private/Component/CombatDeckComponent.cpp` — ResolveAttackCard 后推消耗行
- `Source/DevKit/Public/UI/DamageBreakdownWidget.h` — **删除**
- `Source/DevKit/Private/UI/DamageBreakdownWidget.cpp` — **删除**

---

## 潜在风险

1. **DamageExecution 读取 BFC 的时机**：`LastCombatCardEffectContext` 在 `BuffFlow` 启动时写入，在 `DamageExecution` 执行时读取。若同帧内多个 GE 连续执行，第二个 GE 可能读到第一次的旧 Context。缓解：读取后检查 `Ctx.SourceCard.InstanceGuid` 是否与当前 GE 预期一致（通过 GE 的 SetByCaller 或 EffectContext 传递 Guid）。

2. **消耗行 SourceName 无目标**：消耗行 `TargetName` 为空，FormatEntry 须跳过 `->` 符号，仅显示 `SOURCE [CARD] CardName`。

3. **DamageBreakdownWidget Content 引用**：删除 .h/.cpp 后若有蓝图以其为父类，Cook 时会报错。需要在删除前先在编辑器内确认。

---

## 待确认问题

全部已确认，无遗留问题。

---

## 后续制作清单

- [x] ~~游戏内 HUD DamageBreakdownWidget~~ 已决定移除
- [ ] 导出日志到 CSV
- [ ] 时间线视图（横轴=时间，纵轴=伤害量）
- [ ] 连携链条分组（同一连携链缩进分组）
- [ ] 敌人视角聚合
