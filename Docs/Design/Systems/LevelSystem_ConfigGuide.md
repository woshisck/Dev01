# 关卡系统配置使用指南

> 面向：策划 / 关卡设计  
> 配套文档：[传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)、[关卡 Buff 池配置指南](../FeatureConfig/BuffPool_ConfigGuide.md)  
> 最后更新：2026-04-11（难度配置迁移至 DA_Campaign；波次预算随机化；计时触发可配置；新增 MobSpawner 白名单 / 类型上限 / 错开刷新）

---

## 快速概念说明

系统由两类数据资产组成：

- **DA_Room（房间配置）** — 描述"这个房间里有什么"：哪些怪、哪些 Buff、哪些符文奖励、传送门目标
- **DA_Campaign（局内序列）** — 描述"这一局按什么顺序打、每关有多少怪、难度是多少"

> **架构变更（2026-04-11）**：难度配置（波次数量、预算、触发条件等）已从 DA_Room 移至 DA_Campaign 的每一关条目中。DA_Room 只保留"这个房间有什么"，DA_Campaign 负责"这一关有多难"。

---

## 第一步：创建房间配置（DA_Room）

### 1.1 新建资产

1. 在 Content Browser 中选好存放路径（建议 `Content/Data/Rooms/`）
2. 右键 → **Miscellaneous → Data Asset**
3. 在弹出的类选择窗口中，搜索并选择 **`RoomDataAsset`**
4. 命名格式：`DA_Room_<场景名>_<类型>`

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
| **Room Name** | 随便填一个你能认出来的名字，比如 `Prison_Normal`，只用于调试 |
| **Room Tags** | 同时填两类 Tag：类型（`Room.Type.Normal` / `Room.Type.Elite` / `Room.Type.Shop`）+ 层级（`Room.Layer.L1` / `Room.Layer.L2`） |

> Room Tags 示例：`Room.Type.Normal` + `Room.Layer.L1`

---

### 1.3 配置敌人池（Enemy Pool）

敌人池定义"这个房间里可能出哪些怪"。系统在刷怪时按 DA_Campaign 给出的难度预算，从这个池里随机选怪。

**配置流程**：

1. 先创建或找到对应的 `DA_EnemyData_XXX`（EnemyData 数据资产），填写以下字段：

| 字段 | 说明 | 建议值 |
|------|------|--------|
| **Enemy Class** | 对应的敌人蓝图类 | 从下拉列表中选 |
| **Difficulty Score** | 这只怪"值多少分"，系统按分预算决定刷多少只 | 普通小怪：**2-4**，精英怪：**6-10** |

2. 在 DA_Room 的 **Enemy Pool** 中点 **+**，填写：

| 字段 | 说明 |
|------|------|
| **Enemy Data** | 拖入对应的 `DA_EnemyData_XXX` |
| **Max Count Per Level** | 这只怪在整关内最多出现多少只（跨所有波次）。`-1` = 不限制。用于避免同类怪大量重复 |

**实际例子：**

| EnemyData | Enemy Class | Difficulty Score | Max Count Per Level |
|-----------|-------------|-----------------|---------------------|
| DA_Enemy_Soldier | BP_Enemy_Soldier | 3 | 8 |
| DA_Enemy_Archer | BP_Enemy_Archer | 2 | 6 |
| DA_Enemy_EliteKnight | BP_Enemy_EliteKnight | 8 | 2 |

> 当某种怪达到 MaxCountPerLevel 上限后，剩余预算会自动转为"按需补刷"（每死一只补一只），而不是切换到其他怪。

---

### 1.4 配置关卡 Buff 池（Buff Pool）

进入关卡时，系统从 Buff Pool 中随机选 N 个 Buff 施加给所有刷出的怪。N 由 DA_Campaign 当前关的 **Buff Count** 决定。

点 **Buff Pool** 右侧的 **+**，将 `DA_Buff_XXX` 资产拖入槽位。

> **如何创建 DA_Buff？** 参见 [关卡 Buff 池配置指南](../FeatureConfig/BuffPool_ConfigGuide.md)

---

### 1.5 配置符文掉落池（Loot Pool）

关卡结束后，系统从这里随机抽 **最多 3 个** 符文供玩家三选一。

点 **Loot Pool** 右侧的 **+** 添加 `RuneDataAsset`。

> 至少填 3 个，系统才能展示完整三选一界面。

---

### 1.6 配置传送门目标（Portal Destinations）

关卡结束时各传送门可去往哪里。详见 [传送门配置指南](../FeatureConfig/Portal_ConfigGuide.md)。

---

### 1.7 DA_Room 完整配置示例

以 `DA_Room_Prison_Normal` 为例：

- **Room Name**：`Prison_Normal`
- **Room Tags**：`Room.Type.Normal`，`Room.Layer.L1`

**Enemy Pool（3条）：**

| Enemy Data | Score | MaxCountPerLevel |
|------------|-------|-----------------|
| DA_Enemy_Soldier | 3 | 8 |
| DA_Enemy_Archer | 2 | 6 |
| DA_Enemy_EliteKnight | 8 | 2 |

**Buff Pool**：DA_Buff_ArmorBreak、DA_Buff_Haste

**Loot Pool**：DA_Rune_FireBall、DA_Rune_Shield、DA_Rune_SpeedUp、DA_Rune_LifeSteal

**Portal Destinations**：见传送门配置指南

---

## 第二步：创建局内序列（DA_Campaign）

### 2.1 新建资产

1. 右键 → **Miscellaneous → Data Asset**
2. 搜索并选择 **`CampaignDataAsset`**
3. 命名：`DA_Campaign_MainRun`

---

### 2.2 填写 Floor Table（关卡序列）

**Floor Table** 按游玩顺序定义每一关的宏观参数和难度配置。

点 **+** 添加条目，每条 `FFloorConfig` 包含两大块：

#### 基础参数

| 字段 | 说明 |
|------|------|
| **Floor Number** | 序号（1、2、3…），仅策划参考，不影响运行 |
| **bForce Elite** | 强制精英关，覆盖 EliteChance |
| **Elite Chance** | 精英关概率（0.0~1.0） |
| **Shop Chance** | 商店关概率（0.0~1.0） |
| **Event Chance** | 事件关概率（0.0~1.0） |
| **Common / Rare / Epic Weight** | 符文奖励稀有度权重 |

#### 难度配置（Difficulty Config）

> 这是 **2026-04-11 新架构**：难度配置现在在每一关条目里直接填，不再去 DA_Room 里找。

展开 **Difficulty Config**，填写：

**波次数量**

| 字段 | 说明 | 建议 |
|------|------|------|
| **Wave Count Min** | 最少几波 | `2` |
| **Wave Count Max** | 最多几波（随机取值）| `3` |

两个值一样 = 固定波数。

**单波预算（每波独立随机）**

| 字段 | 说明 | 建议 |
|------|------|------|
| **Wave Budget Min** | 单波难度分最小值 | `12` |
| **Wave Budget Max** | 单波难度分最大值（每波独立骰子）| `20` |

分数决定刷多少怪：Budget=15，Soldier(3分) → 约5只。

**可用触发条件（Allowed Triggers）**

点 **+** 添加，每条代表"这关可以出现的触发方式"，系统随机抽取：

| 触发类型 | 含义 | 费用 | 备注 |
|---------|------|------|------|
| **All Enemies Dead** | 场内全部死亡才触发 | 0分 | 最安全 |
| **Percent Killed 50** | 本波 50% 死亡时触发 | 2分 | 中等压力 |
| **Percent Killed 20** | 本波 20% 死亡时触发 | 3分 | 高压叠怪 |
| **Time Interval** | 计时触发（可自定义秒数）| 2分 | 需填 **Trigger Interval**（默认 3s）|

> **计时触发保底**：若场内已无敌人，不等计时结束，立即触发下一波。

**可用刷怪方式（Allowed Spawn Modes）**

| 方式 | 含义 | 费用 | 备注 |
|-----|------|------|------|
| **Wave** | 这波所有怪随机错开时间刷出 | 1分 | — |
| **One By One** | 按固定间隔逐只刷出 | 1分 | 需填 **One By One Interval**（秒）|

**刷出时间错开（每只之间的随机延迟）**

| 字段 | 说明 | 建议 |
|------|------|------|
| **Spawn Stagger Min** | 每只之间延迟最小值（秒） | `0.0` |
| **Spawn Stagger Max** | 每只之间延迟最大值（秒） | `0.5` |

**数值倍率**

| 字段 | 说明 | 默认 |
|------|------|------|
| **Health Multiplier** | 怪物血量倍率 | `1.0` |
| **Damage Multiplier** | 怪物伤害倍率 | `1.0` |

**Buff 与金币**

| 字段 | 说明 |
|------|------|
| **Buff Count** | 从 DA_Room.BuffPool 中随机选几个施加给怪，填 `0` = 不用 |
| **Gold Min / Max** | 关卡结束金币奖励范围 |
| **Max Total Enemies** | 本关所有波次合计最多刷多少只敌人，`-1` = 不限 |

---

### 2.3 填写全局 Room Pool

在 DA_Campaign 底部：

| 字段 | 说明 |
|------|------|
| **Room Pool** | 把所有可用的 DA_Room 都放在这里（Normal / Elite / Shop 混放），系统按 Room Tags 自动过滤 |
| **Layer Tag** | 此 Campaign 对应的层级（`Room.Layer.L1`），只选包含此 Tag 的 DA_Room |
| **Default Starting Room** | 第一关强制使用的 DA_Room（不填则按概率骰子选） |

---

### 2.4 DA_Campaign 示例（第 1-3 关）

| Floor | EliteChance | WaveCount | BudgetMin/Max | Triggers | SpawnMode |
|-------|-------------|-----------|---------------|----------|-----------|
| 1 | 0 | 2~2 | 12/18 | AllEnemiesDead | Wave |
| 2 | 0 | 2~3 | 15/22 | AllEnemiesDead, TimeInterval(3s) | Wave, OneByOne(3s) |
| 3 | 0.2 | 2~3 | 18/28 | AllEnemiesDead, PercentKilled50 | Wave, OneByOne(2.5s) |

---

## 第三步：在 GameMode 中指定配置

1. 打开游戏的 **GameMode 蓝图**（`BP_YogGameMode`）
2. 在 **Campaign** 分类下，将 **Campaign Data** 指向 `DA_Campaign_MainRun`
3. 在 **Level Flow** 分类下，将 **Reward Pickup Class** 指向 `BP_RewardPickup`

---

## 第四步：在场景中放置 MobSpawner

每个战斗关卡场景中需要放置至少 **1 个 MobSpawner**（`BP_MobSpawner` 或其子类）。

### MobSpawner 白名单

MobSpawner 上的 **Enemy Spawn Classis** 数组作为"白名单"：

- 系统刷怪时，只会在"白名单包含该敌人类型"的 Spawner 里出生
- 同一场景可以放多个 Spawner，每个 Spawner 配置不同的怪，实现分区刷怪

**示例：**
- `Spawner_A`（Enemy Spawn Classis = [BP_Enemy_Soldier]）— 靠近入口
- `Spawner_B`（Enemy Spawn Classis = [BP_Enemy_Archer]）— 地图高台

DA_Room 里 Enemy Pool 有两种怪，Soldier 只从 Spawner_A 出生，Archer 只从 Spawner_B 出生。

### 刷怪特效接口

在 `BP_MobSpawner` 蓝图里实现 **On Enemy Spawned** 事件，可连接 Niagara 特效或触发角色的出场动画：

- `SpawnedEnemy`：刚刚生成的敌人引用（可对其播放 Montage）
- `SpawnLocation`：生成位置（可在此播放粒子特效）

---

## 第五步：验证运行

进入 PIE 后，打开**输出日志**（Output Log），搜索关键词：

| 关键词 | 说明 |
|--------|------|
| `GenerateWavePlans` | 看本关生成了几波，每波预算是多少 |
| `BuildWavePlan` | 看每波的触发条件、刷怪方式、敌人数量、补刷次数 |
| `TriggerNextWave` | 确认波次在正确时机触发 |
| `CheckDemandSpawn` | 按需补刷触发日志 |
| `CheckLevelComplete` | 关卡是否正确判定结束 |
| `EnterArrangementPhase` | 看金币发放了多少 |
| `SpawnEnemyFromPool` | 有"没有 Spawner 支持"的 Warning 时说明白名单未配置 |

---

## 常见问题

**Q：关卡开始后没有怪刷出**
- 检查 GameMode 的 `Campaign Data` 是否填了
- 检查 DA_Campaign 的 Floor Table 有没有条目
- 检查 DA_Room 的 Enemy Pool 有没有填有效的敌人蓝图类
- 检查场景里有没有 MobSpawner，且其 `Enemy Spawn Classis` 与 Enemy Pool 有交集

**Q：只有一只怪**
- `Wave Budget Min` 填得太小（小于任何一只怪的 Score 时，系统保底刷 1 只）
- 适当增大预算值

**Q：怪刷出来在地下**
- 调高 MobSpawner 上的 `Spawn Z Offset`（默认 96，对应角色 Capsule 半高）

**Q：所有怪同时出现**
- 调高 Difficulty Config 的 `Spawn Stagger Max`（当前默认 0.5s）

**Q：计时触发后场内怪太多**
- 换用 `All Enemies Dead` 触发；或缩短 `Trigger Interval`

**Q：精英怪没有出现**
- 确认 DA_Campaign 这关的 `Elite Chance > 0` 或 `bForce Elite` 勾上
- 确认对应 DA_Room 的 `Room Tags` 包含 `Room.Type.Elite`

**Q：符文三选一没有出现**
- 检查 DA_Room 的 `Loot Pool` 是否填了符文
- 确认 `OnLootGenerated` 事件在 UI 蓝图中有绑定

**Q：金币没有增加**
- 确认 DA_Campaign 当前关的 Difficulty Config 里 `Gold Max > 0`
- 确认 HUD 绑定了 `PlayerCharacterBase` 的 `OnGoldChanged` 委托
