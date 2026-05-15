# Rune Editor 数值表连招奖励需求

更新时间：2026-05-08

## 目标

扩展 Yog Rune Editor 的数值表，让玩家动作连招奖励可以作用到任意符文数值，而不是只作用到固定伤害倍率。

这里的“连招奖励”指玩家在攻击动作连招中，随着 `ComboIndex` 增加，对当前符文正在使用的某个数值 Key 进行累计增强。

它不是卡牌之间的 Link Recipe，也不是固定的 `CombatCard.EffectMultiplier`。它应该可以支持：

- 火焰、攻击类效果：随连招提升伤害。
- 月光、飞行物类效果：随连招增加飞行物数量、速度、尺寸或持续时间。
- 中毒、诅咒类效果：随连招增加状态层数、持续时间或爆发强度。

## 当前问题

当前系统中已有卡牌连招倍率字段：

- `CombatCard.bUseComboEffectScaling`
- `CombatCard.ComboScalarPerIndex`
- `CombatCard.MaxComboScalar`

运行时逻辑大致为：

```text
ComboBonusStacks = max(0, ComboIndex - 1)
ComboMultiplier = 1 + min(MaxComboScalar, ComboBonusStacks * ComboScalarPerIndex)
FinalMultiplier = LinkRecipe.Multiplier * ComboMultiplier
```

这个结构适合表达“当前卡牌整体伤害倍率随连招提升”，但不适合表达“某个具体数值随连招变化”。

例如：

- 月光不一定要提升伤害，它可能要增加 `ProjectileCount`。
- 中毒不一定要提升伤害，它可能要增加 `Poison.Stack`。
- 诅咒不一定要提升倍率，它可能要增加 `Curse.Duration` 或 `Curse.Layer`。

因此需要将连招奖励从卡牌级倍率，升级为数值表行级配置。

## 核心设计

每个符文数值表条目都可以独立配置是否受连招影响，以及如何受影响。

推荐概念：

```text
基础值 + 连招奖励配置 = 最终值
```

所有 Flow 节点仍然只引用数值 Key。节点不需要知道当前是第几段连招，也不需要手写连招逻辑。运行时解析数值 Key 时，自动根据当前 Combat Card / Combo 上下文计算最终值。

## 示例

### 火焰伤害

需求：连招越高，燃烧或火焰伤害越高。

| 字段 | 示例 |
| --- | --- |
| Key | `Burn.Damage` |
| BaseValue | `20` |
| ComboBonusMode | `Multiply` |
| BonusPerStack | `0.15` |
| MaxBonus | `0.60` |
| RoundMode | `None` |

计算：

```text
ComboStacks = max(0, ComboIndex - 1)
FinalValue = BaseValue * (1 + min(MaxBonus, ComboStacks * BonusPerStack))
```

结果：

| 连招段 | 最终值 |
| --- | --- |
| 第 1 段 | `20` |
| 第 2 段 | `23` |
| 第 3 段 | `26` |
| 封顶 | `32` |

### 月光飞行物数量

需求：连招越高，月光发射数量越多。

| 字段 | 示例 |
| --- | --- |
| Key | `Moonlight.ProjectileCount` |
| BaseValue | `1` |
| ComboBonusMode | `Add` |
| BonusPerStack | `1` |
| MaxBonus | `3` |
| RoundMode | `Floor` |

计算：

```text
FinalValue = BaseValue + min(MaxBonus, ComboStacks * BonusPerStack)
```

结果：

| 连招段 | 最终值 |
| --- | --- |
| 第 1 段 | `1` 道月光 |
| 第 2 段 | `2` 道月光 |
| 第 3 段 | `3` 道月光 |
| 封顶 | `4` 道月光 |

### 中毒层数

需求：连招越高，中毒层数越高。

| 字段 | 示例 |
| --- | --- |
| Key | `Poison.Stack` |
| BaseValue | `3` |
| ComboBonusMode | `Add` |
| BonusPerStack | `1` |
| MaxBonus | `4` |
| RoundMode | `Floor` |

结果：

| 连招段 | 最终值 |
| --- | --- |
| 第 1 段 | `3` 层 |
| 第 2 段 | `4` 层 |
| 第 3 段 | `5` 层 |
| 封顶 | `7` 层 |

## 推荐数据结构

扩展现有 `FRuneTuningScalar`，增加行级连招奖励配置。

```cpp
UENUM(BlueprintType)
enum class ERuneComboBonusMode : uint8
{
    None     UMETA(DisplayName = "不受连招影响"),
    Add      UMETA(DisplayName = "加算"),
    Multiply UMETA(DisplayName = "乘算")
};

UENUM(BlueprintType)
enum class ERuneTuningRoundMode : uint8
{
    None  UMETA(DisplayName = "不取整"),
    Floor UMETA(DisplayName = "向下取整"),
    Round UMETA(DisplayName = "四舍五入"),
    Ceil  UMETA(DisplayName = "向上取整")
};

USTRUCT(BlueprintType)
struct FRuneComboBonusConfig
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Bonus")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Bonus")
    ERuneComboBonusMode Mode = ERuneComboBonusMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Bonus")
    float BonusPerStack = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Bonus")
    float MaxBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo Bonus")
    ERuneTuningRoundMode RoundMode = ERuneTuningRoundMode::None;
};
```

在 `FRuneTuningScalar` 中追加：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
FRuneComboBonusConfig ComboBonus;
```

## 推荐运行时解析

数值解析应该集中在 `URuneDataAsset::GetRuneTuningValue` 或类似函数中完成。

推荐增加上下文结构：

```cpp
USTRUCT(BlueprintType)
struct FRuneTuningResolveContext
{
    GENERATED_BODY();

    UPROPERTY(BlueprintReadWrite)
    int32 ComboIndex = 1;

    UPROPERTY(BlueprintReadWrite)
    int32 ComboBonusStacks = 0;

    UPROPERTY(BlueprintReadWrite)
    FGameplayTagContainer ComboTags;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> TargetActor = nullptr;

    // 可按需要接入 FCombatCardEffectContext 或其轻量摘要。
};
```

推荐计算逻辑：

```text
BaseValue = 按现有 Literal / Formula / MMC / Context 方式解析基础值
ComboStacks = max(0, Context.ComboIndex - 1)

if ComboBonus.bEnabled:
    Bonus = min(ComboBonus.MaxBonus, ComboStacks * ComboBonus.BonusPerStack)

    if ComboBonus.Mode == Add:
        FinalValue = BaseValue + Bonus

    if ComboBonus.Mode == Multiply:
        FinalValue = BaseValue * (1 + Bonus)

FinalValue = ApplyRoundMode(FinalValue)
```

没有配置 `ComboBonus` 的数值 Key 必须保持旧行为。

## 编辑器需求

### 数值表分页

底层仍然可以使用同一张 `TuningScalars` 表，但 UI 上提供分页或筛选：

- 全部
- 伤害
- 飞行物
- 状态层数
- 持续时间
- 连招奖励

每一行建议展示：

| Key | 基础值 | 来源方式 | 分类 | 连招奖励 | 每段奖励 | 奖励上限 | 取整 |
| --- | --- | --- | --- | --- | --- | --- | --- |

### 数值分类

为了方便筛选和维护，建议为数值行增加分类字段，例如：

```cpp
UENUM(BlueprintType)
enum class ERuneTuningCategory : uint8
{
    General,
    Damage,
    Projectile,
    Stack,
    Duration,
    Radius,
    Chance,
    Presentation,
    Debug
};
```

也可以先使用 `FName Category`，避免后续扩展频繁改枚举。

### 策划使用方式

策划在数值表中配置：

```text
Burn.Damage
Moonlight.ProjectileCount
Moonlight.ProjectileSpeed
Poison.Stack
Curse.Duration
```

然后为每一行选择：

- 是否受连招影响。
- 加算还是乘算。
- 每段增加多少。
- 最多增加多少。
- 是否取整。

Flow 节点只引用 Key，不直接写连招逻辑。

## Flow 节点需求

所有需要数值的节点应优先读取数值表 Key：

- 伤害节点读取 `Burn.Damage`、`Moonlight.Damage`。
- 飞行物节点读取 `Moonlight.ProjectileCount`、`Moonlight.ProjectileSpeed`。
- 状态节点读取 `Poison.Stack`、`Curse.Stack`。
- 持续效果节点读取 `Burn.Duration`、`Curse.Duration`。

节点执行时，应从当前 `UBuffFlowComponent` 或 Combat Card 上下文拿到当前 `ComboIndex`，再调用统一数值解析函数。

## 兼容性要求

- 保留当前 `CombatCard.bUseComboEffectScaling`、`ComboScalarPerIndex`、`MaxComboScalar` 字段。
- 旧字段可以标记为高级字段或旧版卡牌倍率字段。
- 新逻辑优先使用数值表行级 `ComboBonus`。
- 如果某个数值 Key 没有配置行级连招奖励，则行为保持不变。
- 现有 512 资产不强制迁移。
- 新编辑器生成或新制作的符文，优先使用数值表行级连招奖励。

## 推荐迁移方式

第一阶段只做新增能力，不自动迁移旧资产：

1. 增加数据结构。
2. 扩展数值解析函数。
3. 更新 Rune Editor 数值表 UI。
4. 让关键节点开始使用统一数值解析。
5. 新制作的月光、燃烧、中毒、诅咒等符文使用新方案。

第二阶段再按需要把旧 512 资产迁移到新数值表结构。

## 验收标准

1. 数值表可以为每个 Key 配置连招奖励。
2. 火焰伤害可以随攻击连招段数提升。
3. 月光飞行物数量可以随攻击连招段数增加。
4. 中毒、诅咒等层数可以随攻击连招段数增加。
5. Flow 节点只需引用数值 Key，不需要手写连招判断。
6. 未配置连招奖励的符文行为保持不变。
7. 旧 `CombatCard` 连招倍率字段仍可读取，不影响已有资产。
8. 编辑器中数值表分页、筛选和行级连招配置可正常保存、关闭后重新打开仍存在。

## 实现注意点

- 不建议把连招奖励写进每个节点自己的属性里，否则月光、火焰、中毒会各自重复实现。
- 不建议只扩展 `CombatCard.EffectMultiplier`，因为它无法自然表达飞行物数量、层数、持续时间等非伤害数值。
- 数值 Key 命名建议使用稳定层级格式：

```text
Burn.Damage
Burn.Duration
Moonlight.ProjectileCount
Moonlight.ProjectileSpeed
Moonlight.ProjectileScale
Poison.Stack
Poison.Duration
Curse.Stack
Curse.Duration
```

- 整数类数值必须提供取整方式，例如飞行物数量和层数。
- 运行时应避免节点直接读取 `FRuneTuningScalar.Value`，应走统一解析函数，确保 Formula / MMC / Context / ComboBonus 都能生效。
