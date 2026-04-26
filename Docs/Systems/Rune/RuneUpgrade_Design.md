# 符文重复与升级系统设计

> **核心规则**：不允许重复格子 + 自动升级。
>
> 来源：4.15 玩测反馈后定稿（原 memory `project_rune_upgrade_design.md`，2026-04-16）。已转写到本文件作为正式设计文档。
>
> 配套：[BackpackSystem_Technical](BackpackSystem_Technical.md) 第六节 · [BackpackSystem_Guide](BackpackSystem_Guide.md)

## 设计决策

| 项 | 规则 |
|---|---|
| 一种符文（按 `RuneName` 判断）在背包中只能存在一张 | ✅ |
| 拿到已有符文 → 自动升级（Lv.I → II → III）| ✅ |
| 升级不新占格子 | ✅ |
| 升级倍率（线性） | Lv.I × 1.0 / Lv.II × 1.5 / Lv.III × 2.0 |
| 满级（Lv.III）后 | 从奖励池过滤，不再出现 |
| 后期满级再遇到 | 转化为资源（当前暂时过滤） |

## 为什么

背包扩展 + 符文种类扩展的方向下，格子应填充**多种不同符文**以丰富空间决策。叠加同一张符文会破坏拼图策略感，且与 Backpack Hero 类游戏缺乏差异化。

## 实现要点

### 数据结构

`FRuneInstance` 新增 `UpgradeLevel`（int32，0 / 1 / 2 对应 Lv.I / II / III）。

### 入口逻辑

`PlayerCharacterBase::AddRuneToInventory(URuneDataAsset* Rune)`：
1. 先在背包中查找同名符文
2. 找到 → 升级（`UpgradeLevel++`，限 [0, 2]）
3. 没找到 → 走原放置流程（自动找格 → 满则进 PendingRunes）

### 奖励池过滤

`YogGameMode::GenerateIndependentLootOptions()` 在抽奖时过滤掉玩家背包中已 `UpgradeLevel == 2` 的符文。

### 倍率应用

BuffFlow 中用统一节点（如 `UpgradeMultiplier`）读取等级应用倍率，**不**为每级单独制作 FlowAsset — 一张 FA 配齐三个等级的差异。

## 验收方式

1. 背包没有 `符文A`，拾取 `符文A` → 自动放入新格子
2. 背包已有 `符文A` Lv.I，再次拾取 → 应升级为 Lv.II（不新占格）；信息卡显示等级
3. 已有 Lv.III → 关卡结算 `GenerateLootOptions` 不应再出 `符文A`
