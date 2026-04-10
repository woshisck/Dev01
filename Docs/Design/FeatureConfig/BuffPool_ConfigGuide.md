# 关卡 Buff 池配置指南

> 适用范围：关卡敌人增强 / BuffPool 配置  
> 适用人群：策划  
> 配套文档：[关卡系统配置指南](../Systems/LevelSystem_ConfigGuide.md)  
> 最后更新：2026-04-10

---

## 概述

BuffPool 是给"这个房间里所有敌人"叠加额外效果的池子。

关卡开始时，系统从 BuffPool 中随机抽取 N 个 Buff（N 由难度配置决定），之后每只刷出的敌人都会自动被施加这些 Buff。

> 类比：BuffPool 就是给关卡内所有怪物一次性"装备"的词条，类似《以撒》中精英房间怪物的特殊词条（护盾、速度、再生）。

---

## 第一步：创建 BuffDataAsset

每个 Buff 效果是一个独立的数据资产。

1. 打开 Content Browser → 选好存放路径（建议 `Content/Data/Buff/`）
2. 右键 → **Miscellaneous → Data Asset**
3. 搜索并选择 **`BuffDataAsset`**
4. 命名格式：`DA_Buff_<效果名>`

**示例命名**：
```
DA_Buff_ArmorBreak     护甲破碎（敌人防御降低）
DA_Buff_Haste          急速（敌人移速提升）
DA_Buff_Regen          再生（敌人每秒回血）
DA_Buff_Shield         护盾（敌人额外护盾值）
```

---

## 第二步：填写 BuffDataAsset 字段

打开刚创建的资产，填写以下字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `BuffName` | FName | Buff 的名称（供内部识别 / 未来 UI 显示用）|
| `BuffDescription` | FText | Buff 效果的说明文字（未来 UI 展示用，现阶段可留空）|
| `BuffEffect` | TSubclassOf\<UGameplayEffect\> | 实际施加给敌人的 GameplayEffect 类 |

⚠️ `BuffEffect` 必须填写，否则这个 Buff 不会有任何实际效果。

---

## 第三步：在 DA_Room 中配置 BuffPool

1. 打开对应的 `DA_Room_XXX` 资产
2. 在 **BuffPool** 数组中，点击 `+` 添加条目
3. 将创建好的 `DA_Buff_XXX` 拖入对应槽位

---

## 第四步：设置随机抽取数量（BuffCount）

BuffCount 在**难度配置**中设置：

1. 在 DA_Room 的 **DifficultyConfigs** 数组中，找到对应难度档（Low / Medium / High）
2. 展开该档的 `Config` 字段
3. 修改 **`BuffCount`** — 本关从 BuffPool 中随机抽取几个 Buff 施加给敌人

**示例**：

```
DA_Room_Prison_Normal
  BuffPool = [DA_Buff_ArmorBreak, DA_Buff_Haste, DA_Buff_Regen, DA_Buff_Shield]
  DifficultyConfigs:
    Low  → Config.BuffCount = 0   （普通关无额外 Buff）
    High → Config.BuffCount = 1   （高难度随机抽 1 个 Buff）
```

---

## 注意事项

| 情况 | 行为 |
|---|---|
| `BuffEffect` 字段为空 | 跳过该 Buff，不会崩溃，也不会有效果 |
| `BuffCount` 大于 BuffPool 数量 | 取 BuffPool 全部，不重复选取 |
| `BuffPool` 为空 | 关卡无 Buff 效果，敌人正常 |
| `BuffCount = 0` | 不抽取任何 Buff |

---

## 常见问题

**Q：同一个房间在低难度和高难度下能有不同 Buff 数量吗？**  
A：可以。每个难度档（DifficultyEntry）都有独立的 `BuffCount`，按当前难度等级选取对应配置即可。

**Q：Buff 是关卡开始就固定了，还是每波刷怪都重新随机？**  
A：关卡开始时（`StartLevelSpawning`）固定选好，之后本关所有波次的所有敌人都使用同一批 Buff。

**Q：怎么快速创建一个 BuffEffect（GameplayEffect）？**  
A：
1. Content Browser → 右键 → Blueprint Class → 搜索 GameplayEffect → 创建
2. 在 GE 的 Modifiers 中配置对应属性修改（如 Attack + 50%）
3. 将该 GE Blueprint Class 填入 DA_Buff_XXX 的 BuffEffect 字段
