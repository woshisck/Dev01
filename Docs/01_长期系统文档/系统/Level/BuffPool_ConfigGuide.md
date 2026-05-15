# 关卡 Buff 池配置指南

> 适用范围：关卡敌人增强 / 敌人专属 Buff  
> 适用人群：策划  
> 配套文档：[关卡系统配置指南](LevelSystem_ConfigGuide.md)  
> 最后更新：2026-04-13（修正为实际使用 RuneDataAsset + FBuffEntry；补充敌人专属 Buff 池）

---

## 概述

系统有两种 Buff 池：

| 类型 | 配置位置 | 效果范围 |
|---|---|---|
| **关卡 Buff 池（Room BuffPool）** | DA_Room → BuffPool | 本关所有刷出的怪都被施加 |
| **敌人专属 Buff 池（Enemy BuffPool）** | DA_EnemyData → EnemyBuffPool | 仅被选中的那只怪被施加 |

两种池的条目格式相同，都使用 **FBuffEntry**：

| 字段 | 说明 |
|---|---|
| **Rune DA** | 关联的符文数据资产（RuneDataAsset），其 Flow Graph 会被激活到怪身上 |
| **Difficulty Score** | 施加此 Buff 时从波次预算中额外扣除的难度分（Buff 越强，建议扣分越高） |

---

## 关卡 Buff 池（Room BuffPool）

### 作用

关卡开始时，程序从 DA_Room 的 BuffPool 中随机选取 N 个 Buff（N 由当前难度档位的 **Buff Count** 决定）。之后本关所有波次刷出的每只怪都自动被施加这些 Buff。

### 配置步骤

**步骤 1：确认 RuneDataAsset 已存在**

Buff 效果使用项目中已有的 RuneDataAsset（符文资产）实现，符文的 Flow Graph 定义实际效果（属性加成、行为修改等）。无需新建专用 DA 类型。

**步骤 2：在 DA_Room 中配置 BuffPool**

1. 打开对应的 `DA_Room_XXX` 资产
2. 找到 **Buff Pool** 数组，点 `+` 添加条目
3. 每个条目填写：
   - **Rune DA**：拖入 `DA_Rune_XXX`（该符文的 Flow Graph 即为 Buff 效果）
   - **Difficulty Score**：填写此 Buff 对每只怪的额外扣分（建议：轻微增益填 1，中等增益填 2，强力增益填 3~5）

**步骤 3：设置抽取数量（Buff Count）**

打开 DA_Room → 展开对应难度档位（Low / Medium / High） → 修改 **Buff Count**：
- `0` = 本关不抽 Buff，敌人正常
- `1` = 随机抽 1 个 Buff 施加给所有怪
- `2` = 随机抽 2 个 Buff（全部施加给所有怪）

> Buff Count 在各难度档位中独立配置。例如：低难度 BuffCount=0（无增益），高难度 BuffCount=2（双重增益）。

### 难度分影响

关卡 Buff 的 DifficultyScore 会在 BuildWavePlan 时计入每只怪的有效成本：

```
每只怪的有效成本 = EnemyData.DifficultyScore + 所有关卡Buff的DifficultyScore之和 + 敌人专属Buff的DifficultyScore
```

这意味着开启高 DifficultyScore 的关卡 Buff 后，同等预算下刷出的怪会更少，但每只更强。

### 示例

```
DA_Room_Prison_Normal
  BuffPool:
    [0] Rune DA = DA_Rune_Haste,      Difficulty Score = 1
    [1] Rune DA = DA_Rune_ArmorBreak, Difficulty Score = 2
    [2] Rune DA = DA_Rune_Regen,      Difficulty Score = 2
  
  Low Difficulty:
    Buff Count = 0  （无 Buff）
  High Difficulty:
    Buff Count = 1  （随机选 1 个）
```

---

## 敌人专属 Buff 池（EnemyBuffPool）

### 作用

在 DA_EnemyData 中配置，代表"这种敌人独特的增强词条"。例如老鼠可携带"流血"或"霸体"。

BuildWavePlan 时，如果预算允许，程序会从该敌人的 EnemyBuffPool 中随机选取 1 个 Buff，在计划中标记（FPlannedEnemy.SelectedEnemyBuff）。刷出时由 SpawnEnemyFromPool 激活到该敌人的 BuffFlowComponent。

### 配置步骤

1. 打开 `DA_EnemyData_XXX`
2. 找到 **Enemy Buff Pool** 数组，点 `+` 添加条目
3. 每个条目填写：
   - **Rune DA**：选中的符文 Flow Graph 定义实际效果
   - **Difficulty Score**：此词条对波次预算的额外扣分

### 与关卡 Buff 的区别

| 对比项 | 关卡 Buff（Room BuffPool）| 敌人专属 Buff（EnemyBuffPool）|
|---|---|---|
| 配置位置 | DA_Room | DA_EnemyData |
| 施加范围 | 本关所有刷出的怪 | 仅选中的单只怪 |
| 抽取时机 | 关卡开始时（固定）| BuildWavePlan 时逐只敌人独立随机 |
| 扣分方式 | 所有关卡 Buff 扣分之和，每只怪都扣 | 选中时一次性扣，未选中不扣 |

---

## 注意事项

| 情况 | 行为 |
|---|---|
| Rune DA 为空 | 跳过该条目，不崩溃 |
| Rune DA 的 Flow Graph 为空 | 跳过激活，无效果 |
| Buff Count > BuffPool 数量 | 取 BuffPool 全部，不重复选取 |
| BuffPool 为空或 Buff Count = 0 | 关卡无 Buff 效果（但敌人专属 Buff 仍可生效）|
| 预算不足以承担敌人专属 Buff | 程序不选取敌人专属 Buff（该怪仍会刷出，只是没有专属词条）|

---

## 常见问题

**Q：Buff 没有施加给怪**
1. 确认 DA_Room 难度档位的 **Buff Count > 0**（注意是 DA_Room 里的难度档位，不是 DA_Campaign 的 FloorConfig）
2. 确认 **Buff Pool** 有条目且 **Rune DA** 字段已填
3. 确认对应符文的 **Flow Graph（FlowAsset）** 不为空

**Q：高难度关卡的 Buff 比低难度少**
A：检查各难度档位的 Buff Count 配置，High 档的 BuffCount 建议 ≥ Low 档。

**Q：关卡 Buff 和敌人专属 Buff 会叠加吗？**
A：会。一只怪可以同时被关卡 Buff（多个）和自身专属 Buff（最多 1 个）施加。
