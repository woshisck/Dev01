# 关卡系统配置使用指南

> 面向：策划 / 关卡设计  
> 配套文档：[传送门配置指南](Portal_ConfigGuide.md)、[关卡 Buff 池配置指南](BuffPool_ConfigGuide.md)  
> 最后更新：2026-04-13（奖励配置移至DA_Room难度档位；BuffPool改为FBuffEntry；主城房间支持；敌人专属Buff池）

---

## 快速概念说明

系统由两类数据资产组成：

- **DA_Room（房间配置）** — 描述"这个房间里有什么"：哪些怪、哪些符文 Buff、哪些符文奖励、传送门目标、以及各难度档位的最大波次数
- **DA_Campaign（局内序列）** — 描述"这一局按什么顺序打、每关总难度分是多少、房间类型概率是多少"

**难度分配逻辑（程序自动执行，策划只填两处）**：

```
FFloorConfig.TotalDifficultyScore（DA_Campaign 填）
  ↓ 与 GameMode.LowDifficultyScoreMax / HighDifficultyScoreMin 比较
  ↓ 自动选档：Low / Medium / High
  ↓ 读取 DA_Room 对应档位的 MaxWaveCount
  ↓ 随机波次数 = [1, MaxWaveCount]
  ↓ 每波预算 = TotalDifficultyScore / 实际波次数
```

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
| **Room Name** | 填写对应的 **关卡文件名**（Level 资产名，不含路径），系统用此值调用 `OpenLevel`。例：`Level_Prison_01` |
| **Room Tags** | 同时填两类 Tag：类型（`Room.Type.Normal` / `Room.Type.Elite` / `Room.Type.Shop`）+ 层级（`Room.Layer.L1` / `Room.Layer.L2`） |

> Room Name 必须与关卡文件名完全匹配（大小写一致），否则 OpenLevel 失败。

---

### 1.3 配置敌人池（Enemy Pool）

敌人池定义"这个房间里可能出哪些怪"。系统在刷怪时按当关难度预算，从这个池里随机选怪。

**配置流程**：

1. 先创建或找到对应的 `DA_EnemyData_XXX`（EnemyData 数据资产），填写以下字段：

| 字段 | 说明 | 建议值 |
|------|------|--------|
| **Enemy Class** | 对应的敌人蓝图类 | 从下拉列表中选 |
| **Difficulty Score** | 这只怪"值多少分"，系统按分预算决定刷多少只 | 普通小怪：**2-4**，精英怪：**6-10** |
| **Enemy Buff Pool** | 此敌人专属的 Buff 池（如老鼠→流血、霸体）。BuildWavePlan 时随机选取1个施加，同时扣除对应难度分。留空则不选取 | 可选 |
| **Behavior Tree** | 此敌人使用的行为树（留空则用 AIController 默认行为树）| 可选 |

2. 在 DA_Room 的 **Enemy Pool** 中点 **+**，填写：

| 字段 | 说明 |
|------|------|
| **Enemy Data** | 拖入对应的 `DA_EnemyData_XXX` |
| **Max Count Per Level** | 这只怪在整关内最多出现多少只（跨所有波次）。`-1` = 不限制 |

**实际例子：**

| EnemyData | Enemy Class | Difficulty Score | Max Count Per Level |
|-----------|-------------|-----------------|---------------------|
| DA_Enemy_Soldier | BP_Enemy_Soldier | 3 | 8 |
| DA_Enemy_Archer | BP_Enemy_Archer | 2 | 6 |
| DA_Enemy_EliteKnight | BP_Enemy_EliteKnight | 8 | 2 |

---

### 1.4 配置难度档位（Difficulty）

程序根据 DA_Campaign 当前关的 **TotalDifficultyScore** 自动选取 Low / Medium / High 三档之一，再从对应档位读取完整的难度参数。

在 **Difficulty** 分类下，填写三个档位的所有字段：

| 字段 | 说明 | 建议值 |
|------|------|--------|
| **Max Wave Count** | 本档位允许的最大波次数（程序在 [1, MaxWaveCount] 随机）| Low:2, Mid:3, High:4 |
| **Max Enemies Per Wave** | 单波同屏最多存活敌人数（0 = 不限）| Low:0, Mid:0, High:8 |
| **Gold Min / Gold Max** | 关卡结算金币范围 | Low:10/15, Mid:15/25, High:25/40 |
| **Buff Count** | 从 DA_Room.BuffPool 中随机抽几个施加给怪（0=不施加）| Low:0, Mid:1, High:1 |
| **Common / Rare / Epic Weight** | 符文奖励稀有度权重（相对值，无需归一化）| 1.0 / 0.3 / 0.1 |

> **分段阈值**在 GameMode 蓝图中配置（`LowDifficultyScoreMax` 和 `HighDifficultyScoreMin`），策划在 DA_Room 里只需填各档的参数。

> 所有奖励相关字段（金币、BuffCount、符文权重）现在都在 DA_Room 难度档位中配置，不再在 DA_Campaign 的 FloorConfig 里填写。

实际波次数由程序在 [1, MaxWaveCount] 内随机，每波预算 = TotalDifficultyScore ÷ 实际波次数。

---

### 1.5 配置关卡 Buff 池（Buff Pool）

BuffPool 中的每个条目是一个 **FBuffEntry**，包含两个字段：

| 字段 | 说明 |
|------|------|
| **Rune DA** | 拖入对应的 `DA_Rune_XXX`（RuneDataAsset），符文的 Flow Graph 会在每只刷出的怪身上激活 |
| **Difficulty Score** | 此 Buff 施加给怪时，从波次预算中额外扣除的难度分（代表该 Buff 使怪更强）|

> 随机抽取数量（N）由当前难度档位的 **Buff Count** 字段决定（在难度档位配置中填写，不再在 DA_Campaign 里填）。

---

### 1.6 配置符文掉落池（Loot Pool）

关卡结束后，系统从这里随机抽 **最多 3 个** 符文供玩家三选一。

点 **Loot Pool** 右侧的 **+** 添加 `RuneDataAsset`。

> 至少填 3 个，系统才能展示完整三选一界面。

---

### 1.7 配置传送门目标（Portal Destinations）

关卡结束时各传送门可去往哪里。详见 [传送门配置指南](Portal_ConfigGuide.md)。

**传送门目标配置在 DA_Room（不在 DA_Campaign）**，每条记录对应一个门：

| 字段 | 说明 |
|------|------|
| **Portal Index** | 与场景中 APortal.Index 对应（0 开始） |
| **Room Pool** | 此门专属的 DA_Room 候选池（按类型 Tag 过滤后随机选）；为空则只走 Campaign 全局 RoomPool |

---

### 1.8 DA_Room 完整配置示例

以 `DA_Room_Prison_Normal` 为例：

- **Room Name**：`Level_Prison_01`（关卡文件名）
- **Room Tags**：`Room.Type.Normal`，`Room.Layer.L1`

**Enemy Pool（3条）：**

| Enemy Data | Score | MaxCountPerLevel |
|------------|-------|-----------------|
| DA_Enemy_Soldier | 3 | 8 |
| DA_Enemy_Archer | 2 | 6 |
| DA_Enemy_EliteKnight | 8 | 2 |

**难度档位：**

| 档位 | MaxWaveCount | MaxEnemiesPerWave | GoldMin/Max | BuffCount |
|------|-------------|-------------------|-------------|-----------|
| Low | 2 | 0 | 10/15 | 0 |
| Medium | 3 | 0 | 15/25 | 1 |
| High | 4 | 8 | 25/40 | 1 |

**Buff Pool**：（每条目含 RuneDA + DifficultyScore）DA_Rune_Buff_ArmorBreak(score:2)、DA_Rune_Buff_Haste(score:1)

**Loot Pool**：DA_Rune_FireBall、DA_Rune_Shield、DA_Rune_SpeedUp、DA_Rune_LifeSteal

**Portal Destinations**：见传送门配置指南

---

### 1.9 主城/枢纽房间（Hub Room）

勾选 DA_Room 的 **Is Hub Room** 后，该房间视为主城/枢纽：

- **不进行波次刷怪**（跳过整个 GenerateWavePlans 流程）
- **关卡开始时立即开启所有已配置的传送门**（无随机，全部直接开）
- 传送门目标指向 FloorTable 第 0 关（即 `CurrentFloor = 0` → 切关后进入 FloorTable[0]）

**典型用法**：作为游戏开局的大厅关卡，传送门通往第一个战斗关。

> Is Hub Room 勾选后，BuffPool / EnemyPool / 难度档位等字段可以不填（系统会跳过）。传送门配置仍然生效——需在 PortalDestinations 中指定各门可前往的 DA_Room 池。

---

## 第二步：创建局内序列（DA_Campaign）

### 2.1 新建资产

1. 右键 → **Miscellaneous → Data Asset**
2. 搜索并选择 **`CampaignDataAsset`**
3. 命名：`DA_Campaign_MainRun`

---

### 2.2 填写 Floor Table（关卡序列）

**Floor Table** 按游玩顺序定义每一关的宏观参数。

点 **+** 添加条目，每条 `FFloorConfig` 包含：

#### 基础参数

| 字段 | 说明 |
|------|------|
| **Total Difficulty Score** | 本关总难度分（程序用此值决定刷出的敌人总量，并选取 DA_Room 的难度档位）|
| **bForce Elite** | 强制精英关，覆盖 EliteChance |
| **Elite Chance** | 精英关概率（0.0~1.0） |
| **Shop Chance** | 商店关概率（0.0~1.0） |
| **Event Chance** | 事件关概率（0.0~1.0） |

> 金币范围、符文稀有度权重、BuffCount 已移至 **DA_Room 的难度档位配置**（FRoomDifficultyTier），不再在 FloorConfig 中填写。

> **波次数、刷怪方式（Wave/OneByOne）、触发条件**：由程序自动决定，不在 DA 中配置。  
> 如需控制难度，只需调整 **TotalDifficultyScore** 和 DA_Room 各档的 **MaxWaveCount**。

#### 难度分配参考

| TotalDifficultyScore | 效果 |
|----------------------|------|
| ≤ 20（Low 档） | MaxWaveCount 取 DA_Room.LowDifficulty.MaxWaveCount，每波预算小 |
| 21~39（Medium 档）| MaxWaveCount 取 DA_Room.MediumDifficulty.MaxWaveCount |
| ≥ 40（High 档） | MaxWaveCount 取 DA_Room.HighDifficulty.MaxWaveCount，每波预算大 |

> 具体阈值（20/40）在 GameMode 蓝图中配置，以上为示例值。

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

| Floor | EliteChance | TotalDifficultyScore |
|-------|-------------|----------------------|
| 1 | 0 | 20 |
| 2 | 0 | 28 |
| 3 | 0.2 | 36 |

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

屏幕上也会显示 `[刷怪] 波次X | 队列X/N | 存活X | 敌人类名`（每次刷怪时显示）。

---

## 常见问题

**Q：关卡开始后没有怪刷出**
- 检查 GameMode 的 `Campaign Data` 是否填了
- 检查 DA_Campaign 的 Floor Table 有没有条目
- 检查 DA_Room 的 Enemy Pool 有没有填有效的敌人蓝图类
- 检查场景里有没有 MobSpawner，且其 `Enemy Spawn Classis` 与 Enemy Pool 有交集

**Q：只有一只怪**
- `TotalDifficultyScore` 填得太小（小于任何一只怪的 Score 时，系统保底刷 1 只）
- 适当增大分数值

**Q：怪刷出来在地下**
- 调高 MobSpawner 上的 `Spawn Z Offset`（默认 96，对应角色 Capsule 半高）

**Q：所有怪同时出现**
- Wave 模式下，每只之间的错开时间由 GameMode.`SpawnStaggerMin/Max` 控制（程序属性，非 DA 配置）

**Q：精英怪没有出现**
- 确认 DA_Campaign 这关的 `Elite Chance > 0` 或 `bForce Elite` 勾上
- 确认对应 DA_Room 的 `Room Tags` 包含 `Room.Type.Elite`

**Q：符文三选一没有出现**
- 检查 DA_Room 的 `Loot Pool` 是否填了符文
- 确认 `OnLootGenerated` 事件在 UI 蓝图中有绑定

**Q：金币没有增加**
- 确认 DA_Room 对应难度档位的 `Gold Max > 0`（金币配置已移至 DA_Room 难度档位，不在 DA_Campaign 的 FloorConfig 中）
- 确认 HUD 绑定了 `PlayerCharacterBase` 的 `OnGoldChanged` 委托

**Q：Buff 没有施加给怪**
- 确认 DA_Room 对应难度档位的 `Buff Count > 0`（BuffCount 已移至 DA_Room 难度档位）
- 确认 DA_Room 的 Buff Pool 中填了 FBuffEntry 条目（RuneDA + DifficultyScore）
- 确认符文 RuneDataAsset 有配置 Flow Graph（FlowAsset 不为空）
