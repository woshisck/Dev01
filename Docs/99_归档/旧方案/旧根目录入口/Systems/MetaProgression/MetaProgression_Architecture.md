> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# YogMetaProgressionSubsystem 元进度系统架构

> 最后更新：2026-05-28
> 源文件：`Source/DevKit/Public/MetaProgression/YogMetaProgressionSubsystem.h`

---

## 职责

`UYogMetaProgressionSubsystem` 是 GameInstance 级 Subsystem，管理**局外成长**的业务逻辑。

- 局外货币的增减与上限控制
- 升级节点的购买（前置检查 + 货币扣除）
- 功能解锁（剧情触发，不花费货币）
- 神秘侧等级（解锁高级节点的门槛）
- 打造存档（局内 BeginPlay 时 AsyncLoad 后 Grant 给玩家）
- 跑局结算数据汇总与广播

**核心约定：本系统只做业务逻辑，不持久化数据。**
所有持久化操作通过 `CommitSave()` 委托 `YogSaveSubsystem`。

---

## 数据来源

配置数据来自两张 DataTable，通过 `UMetaProgressionSettings`（`Config/DefaultGame.ini`）配置资产路径，`Initialize()` 时同步加载：

| 字段 | 内容 |
| --- | --- |
| `MetaUpgradeNodeTable` | 所有升级节点定义（前置/价格/MaxLevel/解锁的 FeatureTag） |
| `MetaCurrencyRuleTable` | 货币规则（MaxCapacity、汇率等） |

加载失败时会打 Warning 日志，但不会 crash。

运行时数据从 `YogSaveSubsystem->GetCurrentSave()->MetaProgressionData` 读写。

---

## 对外接口分组

### 货币操作

| 接口 | 说明 |
| --- | --- |
| `GetCurrencyAmount(FGameplayTag)` | 查询当前余额 |
| `AddCurrency(FGameplayTag, int32)` | 增加货币（Amount 可为负，结果限 [0, MaxCapacity]） |
| `SpendCurrency(FGameplayTag, int32)` | 扣除货币，余额不足返回 false 且不扣除 |

### 升级节点

| 接口 | 说明 |
| --- | --- |
| `GetNodeLevel(FName)` | 查询某节点当前等级 |
| `CanPurchaseNode(FName)` | 检查：前置满足 + 神秘等级满足 + 货币充足 + 未达 MaxLevel |
| `TryPurchaseNode(FName)` | 购买一级；成功返回 true 并触发存档 |
| `GetAllNodeNames(TArray<FName>&)` | 枚举所有节点 RowName（UI 遍历用） |

### 功能解锁

| 接口 | 调用方 | 说明 |
| --- | --- | --- |
| `IsFeatureUnlocked(FGameplayTag)` | 故事引擎 / 教程系统 / 任意查询 | 查询功能是否已解锁 |
| `UnlockFeature(FGameplayTag)` | **导演 / 故事引擎**（剧情触发） | 直接解锁，不花费货币 |

### 神秘侧等级

| 接口 | 说明 |
| --- | --- |
| `GetMysticSideLevel()` | 当前神秘等级 |
| `GetAvailableMysticPoints()` | 可用神秘点数 |
| `AddMysticPoints(int32)` | 增加神秘点数 |

### 打造存档

| 接口 | 说明 |
| --- | --- |
| `AddCraftedStarterRune(FPrimaryAssetId)` | 添加一个打造好的开局符文 |
| `AddCraftedWeaponFinisher(FPrimaryAssetId)` | 添加打造好的武器终结技 |
| `GetCraftedStarterRunes()` | 读取（局内 BeginPlay 时 AsyncLoad + Grant） |
| `GetCraftedWeaponFinishers()` | 同上 |

### 结算

| 接口 | 说明 |
| --- | --- |
| `BroadcastRunEnded(int32 Floor, int32 Kills)` | 构建 FRunSummaryData 并广播 OnRunEnded |
| `ClearRunCurrencyAccumulator()` | 新局开始时调用，清零本局货币累计器 |
| `GetRunCurrencyGained()` | 读取本局毛收入（不扣除局内花费） |

---

## 事件广播

| 事件 | 触发时机 |
| --- | --- |
| `OnCurrencyChanged(CurrencyTag, NewAmount)` | AddCurrency / SpendCurrency 后 |
| `OnNodePurchased(NodeRowName)` | TryPurchaseNode 成功后 |
| `OnFeatureUnlocked(FeatureTag)` | UnlockFeature 后 |
| `OnRunEnded(FRunSummaryData)` | BroadcastRunEnded 后 |

---

## 与其他系统的关系

```
YogMetaProgressionSubsystem
    │
    ├── 读写 → YogSaveSubsystem (CommitSave)
    │
    ├── 被查询 → StoryEngineSubsystem (IsFeatureUnlocked)
    │
    ├── 被调用 → FirstRunTutorialDirectorSubsystem (UnlockFeature)
    │
    └── 被调用 → BackpackGridComponent (隐藏被动激活时查询)
```

---

## 导演可用接口

| 接口 | 使用场景 |
| --- | --- |
| `UnlockFeature(FGameplayTag)` | 剧情节点触发功能解锁（如解锁背包、解锁某武器） |
| `IsFeatureUnlocked(FGameplayTag)` | 导演判断某功能是否已解锁再决定后续分支 |

