# 关卡系统配置使用指南

> 面向：策划 / 关卡设计  
> 配套文档：[传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)、[关卡 Buff 池配置指南](../FeatureConfig/BuffPool_ConfigGuide.md)  
> 最后更新：2026-04-10（更新 FloorTable 结构：FFloorEntry→FFloorConfig，移除直接 RoomData 引用，改为四类型 RoomPool + 骰子系统）

---

## 快速概念说明

系统由两类数据资产组成：

- **DA_Room（房间配置）** — 描述"这个关卡里有什么"：哪些怪、哪些buff、哪些符文奖励、金币多少
- **DA_Campaign（局内序列）** — 描述"这一局按什么顺序打哪些房间"

打个比方：DA_Room 是"菜单"，DA_Campaign 是"点菜单"。

---

## 第一步：创建房间配置（DA_Room）

### 1.1 新建资产

1. 在 Content Browser 中选好存放路径（建议 `Content/Docs/Map/`）
2. 右键 → **Miscellaneous → Data Asset**
3. 在弹出的类选择窗口中，搜索并选择 **`RoomDataAsset`**
4. 命名格式：`DA_Room_<场景名>_<类型>`

**示例：**
```
DA_Room_Prison_Normal     监狱场景普通关
DA_Room_Prison_Elite      监狱场景精英关
DA_Room_Forest_Normal     森林场景普通关
```

---

### 1.2 填写基础标识

打开刚创建的资产，在 **Room** 分类下：

| 字段 | 如何填 |
|------|--------|
| **Room Name** | 随便填一个你能认出来的名字，比如 `Prison_Normal`，只用于调试显示 |
| **Is Elite Room** | 这是精英关就打勾，普通关不勾。打勾后才会让"精英专属"的怪出现 |

---

### 1.3 配置敌人池（Enemy Pool）

⚠️ **结构更新（2026-04-10）**：敌人的 Class / 难度分 / 精英标记已统一移入 **`UEnemyData` 数据资产**，Enemy Pool 中的每条条目只需选对应的 DA_EnemyData_XXX。

**配置流程**：
1. 先创建 `DA_EnemyData_XXX`（EnemyData 数据资产），填写以下字段：

| 字段 | 说明 | 建议值 |
|------|------|--------|
| **Enemy Class** | 对应的敌人蓝图类 | 从下拉列表中选 |
| **Difficulty Score** | 这只怪"值多少分"，系统按分预算决定刷多少只 | 普通小怪：**2-4**，精英怪：**6-10** |
| **Elite Only** | 勾上后，只有精英关（Is Elite Room=true）才刷出这只怪 | 精英专属怪勾，普通怪不勾 |

2. 在 DA_Room 的 **Enemy Pool** 中点 **+**，将 `DA_EnemyData_XXX` 拖入 EnemyData 槽位

**实际例子：**

| EnemyData 资产 | Enemy Class | Difficulty Score | Elite Only |
|----------------|-------------|-----------------|-----------|
| DA_Enemy_Soldier | BP_Enemy_Soldier | 3 | 不勾 |
| DA_Enemy_Archer | BP_Enemy_Archer | 2 | 不勾 |
| DA_Enemy_EliteKnight | BP_Enemy_EliteKnight | 8 | **勾上** |

---

### 1.4 配置关卡 Buff 池（Buff Pool）

⚠️ **结构更新（2026-04-10）**：Buff Pool 现在填 `DA_Buff_XXX`（BuffDataAsset 数据资产），不再直接填 GE 类。

进入关卡时，系统会从 Buff Pool 中随机选 N 个 Buff 施加给所有怪。

点 **Buff Pool** 右侧的 **+**，将 `DA_Buff_XXX` 资产拖入槽位。

> **如何创建 DA_Buff？** 参见 [关卡 Buff 池配置指南](../FeatureConfig/BuffPool_ConfigGuide.md)

> **提示：** 具体选几个由下方难度配置的 **Buff Count** 字段决定。填 0 就是不用关卡 Buff。

---

### 1.5 配置符文掉落池（Loot Pool）

关卡结束后，系统从这里随机抽 **最多3个** 符文供玩家三选一。

点 **Loot Pool** 右侧的 **+** 添加 `RuneDataAsset`。

> **注意：** 至少填 3 个，系统才能展示完整的三选一界面。少于3个时有几个显示几个。

---

### 1.6 配置难度档位（DifficultyConfigs）

⚠️ **结构更新（2026-04-10）**：原先的三个固定字段（Low Config / Medium Config / High Config）已改为 **`DifficultyConfigs` 数组**，可以按需填写任意档位，不需要的难度档可以不填。

在 **DifficultyConfigs** 数组中，点 **+** 添加条目，每条包含：
- **Tier**：难度等级（Low / Medium / High / Elite）
- **Config**：该难度档的详细配置

> 这样做的好处：你可以只填 Low 和 High，中间的 Medium 不填。如果某关卡的 Difficulty 选了 Medium 但 DA_Room 里没有 Medium 档，系统会自动降级到数组里第一个档位（通常是 Low）。

展开 Config，填写以下字段：

#### 波次数量

| 字段 | 说明 | 建议 |
|------|------|------|
| **Wave Count Min** | 最少几波怪 | `2` |
| **Wave Count Max** | 最多几波怪（系统随机取值） | `3` |

如果你想固定波数，把 Min 和 Max 填一样的值。

#### 每波预算（Wave Budgets）

这是一个数组，每个数字对应一波的"难度分预算"。

点 **+** 按波数添加。例如：
- `[15]` — 只有1波，预算15分
- `[12, 18, 15]` — 3波，依次是12/18/15分

系统用这个预算来从 Enemy Pool 里选怪：分数高 = 怪多或怪强。

> 如果波数多于数组长度，最后一个数值会被循环使用。

#### 可用触发条件（Allowed Triggers）

点 **+** 添加。每条代表"这套难度下，下一波可以用什么条件触发"。

| 触发类型 | 含义 | 费用（Difficulty Score）| 适合 |
|---------|------|------------------------|------|
| **All Enemies Dead** | 场内怪全死了才触发下一波 | 0分 | 安全，新手友好 |
| **Percent Killed 50** | 本波死了50%才触发下一波 | 2分 | 中等压力 |
| **Percent Killed 20** | 本波死了20%就触发下一波 | 3分 | 高压，叠怪 |
| **Time Interval 5s** | 本波开始5秒后自动触发 | 2分 | 时间压力 |

**低难度建议：** 只填 `All Enemies Dead`（费用0，安全）

**高难度建议：** 混填多个，系统会随机选其中一个

#### 可用刷怪方式（Allowed Spawn Modes）

| 刷怪方式 | 含义 | 费用 |
|---------|------|------|
| **Wave** | 这波所有怪同时刷出 | 1分 |
| **One By One** | 按间隔逐只刷出（填写 One By One Interval 秒数） | 1分 |

**低难度建议：** 只填 `Wave`

#### 数值倍率

| 字段 | 说明 | 默认 |
|------|------|------|
| **Health Multiplier** | 怪物血量倍率，1.0=正常，1.5=多50%血 | `1.0` |
| **Damage Multiplier** | 怪物伤害倍率 | `1.0` |

#### Buff和金币

| 字段 | 说明 |
|------|------|
| **Buff Count** | 从 Buff Pool 里随机选几个施加给怪，填 `0` 不用 |
| **Gold Min** | 关卡结束金币奖励最小值 |
| **Gold Max** | 关卡结束金币奖励最大值（系统随机取 Min~Max 之间）|

---

### 1.7 完整配置示例

以 `DA_Room_Prison_Normal` 为例：

**基础：**
- Room Name: `Prison_Normal`
- Is Elite Room: 不勾

**Enemy Pool（3条）：**
- BP_Enemy_Soldier，Score=3，不勾精英专属
- BP_Enemy_Archer，Score=2，不勾精英专属
- BP_Enemy_EliteKnight，Score=8，**勾精英专属**

**Loot Pool（4条）：**
- DA_Rune_FireBall、DA_Rune_Shield、DA_Rune_SpeedUp、DA_Rune_LifeSteal

**DifficultyConfigs（数组形式）：**

第一条 Tier=Low，Config：
- Wave Count: 2~2（固定2波）
- Wave Budgets: `[12, 15]`
- Allowed Triggers: `All Enemies Dead`（费用0）
- Allowed Spawn Modes: `Wave`（费用1）
- Health Multiplier: `1.0`，Buff Count: `0`
- Gold Min: `8`，Gold Max: `15`

第二条 Tier=High，Config：
- Wave Count: 3~3
- Wave Budgets: `[18, 22, 25]`
- Allowed Triggers: `All Enemies Dead`、`Percent Killed 50`、`Percent Killed 20`
- Allowed Spawn Modes: `Wave`、`One By One`（Interval=2秒）
- Health Multiplier: `1.5`，Buff Count: `2`
- Gold Min: `25`，Gold Max: `40`

（Medium 可不填，若关卡指定 Medium 则自动降级到 Low）

---

## 第二步：创建局内序列（DA_Campaign）

### 2.1 新建资产

1. 右键 → **Miscellaneous → Data Asset**
2. 搜索并选择 **`CampaignDataAsset`**
3. 命名：`DA_Campaign_MainRun`（或按需命名）

---

### 2.2 填写 Floor Table（关卡序列 — 宏观配置）

> ⚠️ **新架构说明（2026-04-10）**：Floor Table 现在填的是宏观配置，**不再直接指定 DA_Room**。
> 具体使用哪个 DA_Room，由关卡结束时对每个传送门独立骰子决定，从下方四个类型池中选取。

打开资产，在 **Floor Table** 里按游玩顺序添加每一关。

点 **+** 添加条目，每条 `FFloorConfig` 代表一关的宏观参数：

| 字段 | 说明 |
|------|------|
| **Floor Number** | 填第几关（1、2、3…），只是参考标注，不影响运行 |
| **Difficulty** | 选 `Low` / `Medium` / `High` / `Elite`，决定使用哪套 DifficultyConfig |
| **bForce Elite** | 勾上后本关强制为精英房，覆盖 EliteChance |
| **Elite Chance** | 精英房概率（0.0~1.0），例如 0.2 = 20% 概率 |
| **Shop Chance** | 商店房概率（0.0~1.0）|
| **Event Chance** | 事件房概率（0.0~1.0）|
| **Common/Rare/Epic Weight** | 符文奖励稀有度相对权重（相加无需为 1）|
| **Portal Destinations** | 填写本关各传送门的目标地图池（见下方说明）|

> **注意：** EliteChance + ShopChance + EventChance 之和超过 1.0 是允许的，超出部分会使普通房变为 0，但不会报错。建议总概率控制在 0.5 以下，留足普通房空间。

#### Portal Destinations（传送门目标地图池）

这是关卡结束后玩家能通过哪些门去往哪里的配置。**地图名与房间类型无关**，同一张地图可以承载任意房间类型。

点 **Portal Destinations** 右侧的 **+**，每条对应场景中一扇门：

| 子字段 | 说明 |
|--------|------|
| **Portal Index** | 对应场景中 APortal Actor 的 Index 值 |
| **Next Level Pool** | 目标关卡名数组，关卡结束时随机选一个（类型无关）|

> **详细配置步骤** 参见 [传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)

**示例序列（12关一局）：**

| Floor | Difficulty | EliteChance | ShopChance | EventChance |
|-------|-----------|-------------|------------|-------------|
| 1 | Low | 0 | 0 | 0 |
| 2 | Low | 0 | 0 | 0 |
| 3 | Medium | 0.2 | 0.1 | 0.1 |
| 4 | Medium | 0 | 0 | 0 | bForceElite=true |
| 5 | Medium | 0.2 | 0.15 | 0.1 |
| 6 | High | 0.3 | 0.15 | 0.1 |
| ... | ... | ... | ... | ... |

---

### 2.3 填写四个房间类型池

在 DA_Campaign 底部，找到 **Room Pools** 分类，填写四类 DA_Room：

| 字段 | 说明 |
|------|------|
| **Normal Room Pool** | 普通战斗房 DA_Room 列表（最常见，必填）|
| **Elite Room Pool** | 精英战斗房 DA_Room 列表（EliteChance > 0 时需要填）|
| **Shop Room Pool** | 商店房 DA_Room 列表（ShopChance > 0 时需要填）|
| **Event Room Pool** | 事件房 DA_Room 列表（EventChance > 0 时需要填）|

系统会按骰子结果从对应池中**随机抽一个** DA_Room 给该传送门的分支使用。若某个类型池为空，系统自动降级到 Normal Pool。

**建议：** 每个池子至少放 2-3 个不同的 DA_Room，以保证同一关卡多次游玩时有变化感。

---

## 第三步：在 GameMode 中指定配置

1. 打开游戏的 **GameMode 蓝图**（通常叫 `BP_YogGameMode` 或类似名称）
2. 在 **Details** 面板中找到 **Campaign** 分类
3. 将 **Campaign Data** 字段指向你的 `DA_Campaign_MainRun`

> `Current Floor` 默认从 `1` 开始，对应 Floor Table 第一条。

---

## 第四步：在场景中放置 MobSpawner

每个战斗关卡场景中需要放置至少 **1 个 MobSpawner**（怪物出生点）。

- 系统刷怪时会从场景内所有 MobSpawner 中随机选一个
- 建议放 **3-6 个**，分布在地图各处，避免刷怪扎堆

---

## 第五步：验证运行

进入 PIE 后，打开**输出日志**（Output Log），搜索关键词：

- `GenerateWavePlans` — 看本关生成了几波
- `BuildWavePlan` — 看每波的触发条件、刷怪方式、敌人数量
- `TriggerNextWave` — 确认波次在正确时机触发
- `CheckLevelComplete` — 关卡是否正确判定结束
- `EnterArrangementPhase` — 看金币发放了多少
- `GenerateLootOptions` — 看生成了几个符文选项

---

## 常见问题

**Q: 关卡开始后没有怪刷出**
- 检查 GameMode 的 `Campaign Data` 是否填了
- 检查 DA_Campaign 的 Floor Table 有没有条目
- 检查 DA_Room 的 Enemy Pool 有没有填有效的敌人蓝图类
- 检查场景里有没有 MobSpawner

**Q: 只有一只怪**
- `Wave Budgets` 填的太小（小于任何一只怪的 Score 时，系统只保底刷1只）
- 适当增大预算值

**Q: 精英怪没有出现**
- 确认 DA_Campaign 这关的 Difficulty 是 `Elite`
- 确认 DA_Room 的 `Is Elite Room` 勾上了
- 确认精英怪条目的 `Elite Only` 勾上了

**Q: 符文三选一没有出现**
- 检查 DA_Room 的 `Loot Pool` 是否填了符文
- 确认 `OnLootGenerated` 事件在 UI 蓝图中有绑定

**Q: 金币没有增加**
- 确认 DA_Room 的对应难度配置里 `Gold Max > 0`
- 确认 HUD 绑定了 `PlayerCharacterBase` 的 `OnGoldChanged` 委托
