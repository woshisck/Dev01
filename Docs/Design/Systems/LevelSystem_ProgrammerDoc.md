# 关卡系统 — 程序技术文档

> 最后更新：2026-04-11（难度配置迁移至 DA_Campaign；波次预算随机化；MobSpawner 白名单；类型上限 + 按需补刷；错开刷新；计时触发可配置 + 保底；刷怪特效接口）  
> 配套文档：[跨关状态持久化](CrossLevelState_Technical.md)、[传送门系统设计](Portal_Design.md)

---

## 一、系统概览

```
数据层（Data Assets）
  ├── URoomDataAsset       单房间配置（敌人池 / Buff / 战利品 / 传送门目标）
  └── UCampaignDataAsset   局内关卡序列（FloorTable，每关含完整难度配置）

逻辑层（GameMode）
  └── AYogGameMode         波次刷怪 / 阶段切换 / 战利品发放 / 切关

场景层
  └── AMobSpawner          敌人出生点（含白名单过滤和 VFX 接口）
```

---

## 二、数据结构

### URoomDataAsset（`Source/DevKit/Public/Data/RoomDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `RoomName` | `FName` | 房间标识，调试用 |
| `RoomTags` | `FGameplayTagContainer` | 类型 Tag（`Room.Type.*`）+ 层级 Tag（`Room.Layer.*`），供系统按类型选取 |
| `EnemyPool` | `TArray<FEnemyEntry>` | 敌人池，含难度分和每关类型上限 |
| `BuffPool` | `TArray<TObjectPtr<URuneDataAsset>>` | 进关时随机选 N 个 RuneDA 施加给所有怪 |
| `LootPool` | `TArray<TObjectPtr<URuneDataAsset>>` | 符文掉落池，关卡结束三选一 |
| `PortalDestinations` | `TArray<FPortalDestConfig>` | 关卡结束各传送门的目标配置 |

> ⚠️ `DifficultyConfigs` 已于 2026-04-11 从 RoomDA 移除，难度配置现在在 DA_Campaign 的 FFloorConfig 内。

### FEnemyEntry（`SpawnTypes.h`）

```cpp
TObjectPtr<UEnemyData> EnemyData;    // 敌人数据资产（含 DifficultyScore / EnemyClass）
int32 MaxCountPerLevel = -1;         // 该类型在本关内最多出现多少只（-1 = 不限）
```

### UCampaignDataAsset（`Source/DevKit/Public/Data/CampaignDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `FloorTable` | `TArray<FFloorConfig>` | 关卡序列，按游玩顺序排列 |
| `LayerTag` | `FGameplayTag` | 此 Campaign 对应的层级（`Room.Layer.L1`），过滤 RoomPool |
| `RoomPool` | `TArray<URoomDataAsset*>` | 全局 DA_Room 候选池 |
| `DefaultStartingRoom` | `URoomDataAsset*` | 第一关强制使用的房间 |

### FFloorConfig（`CampaignDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `FloorNumber` | `int32` | 序号，仅参考 |
| `DifficultyConfig` | `FDifficultyConfig` | 本关完整难度配置（波次/预算/触发/刷怪方式等）|
| `bForceElite` | `bool` | 强制精英关 |
| `EliteChance` | `float` | 精英关概率 |
| `ShopChance` | `float` | 商店关概率 |
| `EventChance` | `float` | 事件关概率 |
| `CommonWeight/RareWeight/EpicWeight` | `float` | 符文稀有度权重 |

### FDifficultyConfig（`SpawnTypes.h`）

| 字段 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `WaveCountMin/Max` | `int32` | 2/3 | 波次数随机范围 |
| `WaveBudgetMin/Max` | `int32` | 15/25 | 单波难度分预算（每波独立随机） |
| `AllowedTriggers` | `TArray<FSpawnTriggerOption>` | — | 可用触发条件池 |
| `AllowedSpawnModes` | `TArray<FSpawnModeOption>` | — | 可用刷怪方式池 |
| `HealthMultiplier` | `float` | 1.0 | 怪物血量倍率 |
| `DamageMultiplier` | `float` | 1.0 | 怪物伤害倍率 |
| `BuffCount` | `int32` | 1 | 从 BuffPool 选取的数量 |
| `GoldMin/Max` | `int32` | 10/20 | 关卡结束金币奖励 |
| `MaxTotalEnemies` | `int32` | -1 | 关卡总敌人上限（-1 = 不限） |
| `SpawnStaggerMin/Max` | `float` | 0.0/0.5 | 每只敌人之间的随机延迟（秒） |

### FSpawnTriggerOption（`SpawnTypes.h`）

```cpp
ESpawnTriggerType TriggerType;   // AllEnemiesDead / PercentKilled_50 / PercentKilled_20 / TimeInterval
int32 DifficultyScore;           // 使用此触发条件消耗的难度分
float TriggerInterval = 3.0f;   // TimeInterval 类型时有效（秒）
```

### AMobSpawner（`Source/DevKit/Public/Mob/MobSpawner.h`）

| 字段/函数 | 说明 |
|-----------|------|
| `EnemySpawnClassis` | 白名单：只有在此列表中的敌人类型才会在本 Spawner 出生 |
| `SpawnRadius` | 随机出生半径 |
| `SpawnZOffset` | NavMesh 表面 Z 偏移（默认 96，补偿 Capsule 半高） |
| `OnEnemySpawned(SpawnedEnemy, SpawnLocation)` | BlueprintImplementableEvent：生成后立即调用，BP 子类中可播放特效/动画 |

---

## 三、运行时流程

### 3.1 关卡启动

```
StartPlay()
  └─ CampaignData != null → StartLevelSpawning()
       ├─ 读 FloorTable[CurrentFloor-1] → FloorConfig
       ├─ 从 GI 或 DefaultStartingRoom 确定 ActiveRoomData
       ├─ 直接使用 FloorConfig.DifficultyConfig（无需在 RoomDA 查找）
       ├─ 缓存 ActiveRoomData、ActiveDifficultyConfig
       ├─ SelectRoomBuffs() → ActiveRoomBuffs
       ├─ GenerateWavePlans() → WavePlans[]
       └─ 延迟 InitialSpawnDelay 秒后 → TriggerNextWave()
```

### 3.2 波次生成算法（BuildWavePlan）

每波按预算四步消耗：

1. **选触发条件**：筛出费用 ≤ 剩余预算的条件，随机选一个，存入 `Plan.WaveTriggerInterval`
2. **选刷怪方式**：筛出费用 ≤ 剩余预算的方式，随机选一个
3. **填充敌人**：循环从 EnemyPool 按剩余预算随机选怪，遵守以下限制：
   - `FEnemyEntry.MaxCountPerLevel`：该类型跨波次累计上限
   - `FDifficultyConfig.MaxTotalEnemies`：全关卡总数上限
   - 第一只不受预算限制（保证至少 1 只）
4. **按需补刷池（Demand）**：若 Step 3 因上限提前结束而预算有剩余：
   - 从 EnemyPool 中收集仍可用（分数 ≤ 剩余预算）的类型，构建 `DemandEnemyPool`
   - 计算 `DemandCount = 剩余预算 / 平均分`（至少 1 次）

跨波次计数由 `LevelTypeSpawnCounts` 和 `TotalLevelPlannedEnemies` 两个 Map/Counter 在 `GenerateWavePlans` 中累计。

### 3.3 波次执行（TriggerNextWave）

Wave 和 OneByOne 模式统一使用队列 + 非重复定时器：

```
TriggerNextWave()
  ├─ ClearTimer(DemandSpawnTimer)   ← 清除上一波残留的补刷定时器
  ├─ OneByOneSpawnQueue = Wave.EnemiesToSpawn
  ├─ bWaveStaggerMode = (Wave.SpawnMode == Wave)
  └─ SetTimer(OneByOneTimer, FirstDelay)   ← FirstDelay = RandRange(StaggerMin, StaggerMax)

SpawnNextOneByOne()
  ├─ SpawnEnemyFromPool(CurrentClass)
  │    ├─ 白名单过滤：找包含该 Class 的 Spawner
  │    ├─ 随机选一个有效 Spawner → Spawner->SpawnMob()
  │    ├─ 对刷出的敌人施加 ActiveRoomBuffs
  │    └─ 调用 OnEnemySpawned(SpawnedEnemy, SpawnLocation)
  ├─ 若队列未空：重新 SetTimer(NextDelay)
  │    Wave模式：NextDelay = RandRange(StaggerMin, StaggerMax)
  │    OneByOne: NextDelay = Wave.OneByOneInterval
  └─ 若队列已空：SetupWaveTrigger(Wave)
```

### 3.4 波次触发（CheckWaveTrigger）

每次敌人死亡时调用 `UpdateFinishLevel(1)` → `CheckWaveTrigger()`：

```
CheckWaveTrigger()
  ├─ PercentKilled_50 / PercentKilled_20：满足条件 → 清除 DemandSpawnTimer → TriggerNextWave
  ├─ TimeInterval：
  │    TotalAliveEnemies <= 0 → ClearTimer(WaveTriggerTimer) → TriggerNextWave（保底提前触发）
  └─ AllEnemiesDead：
       TotalAliveEnemies <= 0
         ├─ DemandCount > 0 → SetTimer(DemandSpawnTimer, RandRange(StaggerMin, StaggerMax))
         └─ DemandCount == 0 → TriggerNextWave
```

### 3.5 按需补刷（CheckDemandSpawn）

```
CheckDemandSpawn()
  ├─ 从 Wave.DemandEnemyPool 随机选一类型
  ├─ Wave.DemandCount--
  └─ SpawnEnemyFromPool(DemandClass)
       ├─ 成功 → TotalAliveEnemies++、TotalSpawnedInWave++
       └─ 失败（无 Spawner）→ DemandCount 已减，避免死循环
```

### 3.6 关卡完成（CheckLevelComplete）

```
CheckLevelComplete()
  条件：bAllWavesSpawned && TotalAliveEnemies <= 0
  ├─ 最后一波 DemandCount > 0 → 先补刷（SetTimer DemandSpawnTimer）→ 等待下次调用
  └─ DemandCount == 0 → 广播 OnFinishLevel → EnterArrangementPhase()
```

### 3.7 整理阶段与切关

```
EnterArrangementPhase()
  ├─ SpawnActor<ARewardPickup>(LastEnemyKillLocation)
  └─ ActivatePortals()（Fisher-Yates 洗牌，第一个必开，后续 50% 随机）

玩家接触 APortal → EnterPortal() → GM->TransitionToLevel(SelectedLevel, SelectedRoom)
  ├─ 写入 GI.PendingRunState（HP/Gold/Phase/符文）
  ├─ GI.PendingNextFloor = CurrentFloor + 1
  └─ OpenLevel(SelectedLevel)
```

---

## 四、关键 Timer 汇总

| Timer | 作用 |
|-------|------|
| `InitialSpawnDelayTimer` | 关卡开始后的初始延迟，之后触发第一波 |
| `OneByOneTimer` | 队列式逐只刷怪（Wave 错开 / OneByOne 间隔） |
| `WaveTriggerTimer` | TimeInterval 类型触发器 |
| `DemandSpawnTimer` | 按需补刷延迟 |

所有 Timer 均在 `TriggerNextWave` 或关卡完成时被正确 ClearTimer。

---

## 五、已知 TODO

| 任务 | 优先级 | 状态 |
|------|--------|------|
| 刷怪时将 HealthMultiplier/DamageMultiplier 施加到敌人属性 | 高 | ⚠️ 未实现 |
| 符文三选一蓝图 UI（绑定 OnLootGenerated）| 高 | ⚠️ 未实现 |
| 金币 HUD（绑定 OnGoldChanged）| 中 | ⚠️ 未实现 |
| 传送门最少/最多开门数量配置（MinOpenPortals / MaxOpenPortals）| 中 | ⚠️ 未实现 |
| 随机关卡生成系统（取代手动 FloorTable）| 远期 | ⏳ 规划中 |
| 商店关卡类型 | 远期 | ⏳ 规划中 |

**已完成**：
- ✅ 难度配置迁移至 DA_Campaign.FFloorConfig
- ✅ 波次预算随机化（WaveBudgetMin/Max）
- ✅ MobSpawner 白名单过滤（EnemySpawnClassis）
- ✅ 类型上限（MaxCountPerLevel）+ 关卡总数上限（MaxTotalEnemies）
- ✅ 按需补刷（DemandCount + DemandEnemyPool）
- ✅ 刷出时间错开（SpawnStaggerMin/Max，Wave/OneByOne 统一队列）
- ✅ 计时触发可配置间隔（TriggerInterval）+ 场内无敌人时保底触发
- ✅ 刷怪特效接口（AMobSpawner::OnEnemySpawned BlueprintImplementableEvent）
- ✅ 跨关状态持久化（FRunState 完整实现）
- ✅ 切关入口（TransitionToLevel 由 Portal 触发）
