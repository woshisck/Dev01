# 关卡系统 — 程序技术文档

> 最后更新：2026-04-13（与代码对齐；FRoomDifficultyTier 新增全部奖励/Buff字段；FFloorConfig 删除已移走字段；FBuffEntry / FPlannedEnemy 新增；BuildWavePlan 更新成本算法；主城房间逻辑；EnemyData 新增字段）  
> 配套文档：[跨关状态持久化](CrossLevelState_Technical.md)、[传送门系统设计](Portal_Design.md)

---

## 一、系统概览

```
数据层（Data Assets）
  ├── URoomDataAsset       单房间配置（敌人池 / 符文 Buff / 战利品 / 传送门目标 / 难度档位）
  └── UCampaignDataAsset   局内关卡序列（FloorTable，每关含总难度分 + 关卡类型参数）

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
| `RoomName` | `FName` | **关卡文件名**（用于 OpenLevel），同时作为调试标识 |
| `RoomTags` | `FGameplayTagContainer` | 类型 Tag（`Room.Type.*`）+ 层级 Tag（`Room.Layer.*`），供系统按类型选取 |
| `EnemyPool` | `TArray<FEnemyEntry>` | 敌人池，含难度分和每关类型上限 |
| `BuffPool` | `TArray<FBuffEntry>` | 进关时随机选 N 个条目；每条目含 RuneDA + DifficultyScore（每只怪身上激活 Flow Graph，并从波次预算额外扣除对应分数）|
| `bIsHubRoom` | `bool` | 主城/枢纽房间标记；勾选后跳过波次刷怪，所有传送门立即全开，目标 FloorTable[0] |
| `LootPool` | `TArray<TObjectPtr<URuneDataAsset>>` | 符文掉落池，关卡结束三选一 |
| `LowDifficulty` | `FRoomDifficultyTier` | 低分段难度配置（Score ≤ GameMode.LowDifficultyScoreMax）|
| `MediumDifficulty` | `FRoomDifficultyTier` | 中分段难度配置 |
| `HighDifficulty` | `FRoomDifficultyTier` | 高分段难度配置（Score ≥ GameMode.HighDifficultyScoreMin）|
| `PortalDestinations` | `TArray<FPortalDestConfig>` | 关卡结束各传送门的目标配置 |

### FRoomDifficultyTier（`SpawnTypes.h`）

```cpp
int32 MaxWaveCount        = 3;    // 本档位允许的最大波次数（程序在 [1, MaxWaveCount] 内随机）
int32 MaxEnemiesPerWave   = 0;    // 单波最多同屏存活数（0 = 不限；超出时优选高分怪）
int32 GoldMin             = 10;   // 关卡结算金币下限
int32 GoldMax             = 20;   // 关卡结算金币上限
int32 BuffCount           = 1;    // 从 BuffPool 随机抽几个条目施加给怪
float CommonWeight        = 1.0f; // 符文稀有度：普通权重
float RareWeight          = 0.3f; // 符文稀有度：稀有权重
float EpicWeight          = 0.1f; // 符文稀有度：史诗权重
```

> DA_Room 中所有奖励与波次参数均在难度档位里配置，按实际 TotalDifficultyScore 自动选档。

### FBuffEntry（`EnemyData.h`）

```cpp
TObjectPtr<URuneDataAsset> RuneDA;   // 关联符文
int32 DifficultyScore;               // 施加此 Buff 时从波次预算额外扣的难度分
```

> `URoomDataAsset::BuffPool` 和 `UEnemyData::EnemyBuffPool` 均使用此结构体。

### FEnemyEntry（`SpawnTypes.h`）

```cpp
TObjectPtr<UEnemyData> EnemyData;    // 敌人数据资产（含 DifficultyScore / EnemyClass）
int32 MaxCountPerLevel = -1;         // 该类型在本关内最多出现多少只（-1 = 不限）
```

### FPlannedEnemy（`YogGameMode.h`，内部结构）

```cpp
TSubclassOf<AEnemyCharacterBase>    EnemyClass;         // 要生成的敌人类
TObjectPtr<URuneDataAsset>          SelectedEnemyBuff;  // BuildWavePlan 时选好的敌人专属 Buff（可为空）
```

> `WavePlan.EnemiesToSpawn` 和 `WavePlan.DemandEnemyPool` 均为 `TArray<FPlannedEnemy>`。

### UEnemyData（`EnemyData.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `EnemyClass` | `TSubclassOf<AEnemyCharacterBase>` | 对应 BP 角色类 |
| `DifficultyScore` | `int32` | 该敌人的基础难度分 |
| `EnemyBuffPool` | `TArray<FBuffEntry>` | 敌人专属 Buff 池（如老鼠→流血/霸体）；BuildWavePlan 时随机选一个，对应 DifficultyScore 从波次预算额外扣除 |
| `BehaviorTree` | `TObjectPtr<UBehaviorTree>` | 该敌人使用的行为树（留空则用 AIController 默认）|

### UCampaignDataAsset（`Source/DevKit/Public/Data/CampaignDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `FloorTable` | `TArray<FFloorConfig>` | 关卡序列，按游玩顺序排列 |
| `LayerTag` | `FGameplayTag` | 此 Campaign 对应的层级（`Room.Layer.L1`），过滤 RoomPool |
| `RoomPool` | `TArray<TObjectPtr<URoomDataAsset>>` | 全局 DA_Room 候选池（所有类型混放，按 RoomTags 过滤） |
| `DefaultStartingRoom` | `TObjectPtr<URoomDataAsset>` | 第一关强制使用的房间 |

### FFloorConfig（`CampaignDataAsset.h`）

| 字段 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `TotalDifficultyScore` | `int32` | 30 | **本关总难度分**（驱动档位选择 + 波次预算分配）|
| `bForceElite` | `bool` | false | 强制精英关 |
| `EliteChance` | `float` | 0.2 | 精英关概率 |
| `ShopChance` | `float` | 0.15 | 商店关概率 |
| `EventChance` | `float` | 0.1 | 事件关概率 |

> 注意：`GoldMin/GoldMax`、`BuffCount`、`CommonWeight/RareWeight/EpicWeight`、`FloorNumber` 已从 FFloorConfig 移除，现全部在 **FRoomDifficultyTier** 中按难度档位配置。

### FPortalDestConfig（`RoomDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `PortalIndex` | `int32` | 对应场景中 APortal.Index |
| `RoomPool` | `TArray<TObjectPtr<URoomDataAsset>>` | 此门专属的 DA_Room 候选池；为空时回退到 Campaign 全局 RoomPool |

> 关卡文件名 = 选中 DA_Room 的 `RoomName` 字段，直接用于 `OpenLevel` 调用。

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
       ├─ 从 GI.PendingNextFloor 或默认值确定 CurrentFloor
       ├─ 从 GI.PendingRoomData 读取传送门传递的 DA_Room（切关传递）
       │    └─ 若为空：第一关用 DefaultStartingRoom，后续骰子选 RollRoomTypeForFloor + SelectRoomByTag
       ├─ 检查 ActiveRoomData->bIsHubRoom：
       │    ├─ true  → CurrentFloor = 0，跳过波次刷怪，调用 ActivateHubPortals()
       │    └─ false → 正常波次流程（见下）
       ├─ 根据 Config.TotalDifficultyScore 选档（Low/Medium/High）→ 取对应 Tier（含全部奖励参数）
       ├─ SelectRoomBuffs() → ActiveRoomBuffs（TArray<FBuffEntry>，进关时确定，刷怪时施加）
       ├─ GenerateWavePlans(TotalDifficultyScore, MaxWaveCount, MaxEnemiesPerWave, ActiveRoomData)
       ├─ 标记场景中未登记在 PortalDestinations 的门为 NeverOpen
       └─ 延迟 InitialSpawnDelay 秒后 → TriggerNextWave()
```

### 3.2 波次生成算法（GenerateWavePlans / BuildWavePlan）

```
GenerateWavePlans(TotalScore, MaxWaveCount, MaxEnemiesPerWave, Room):
  WaveCount = RandRange(1, MaxWaveCount)
  BasePerWave = TotalScore / WaveCount
  Remainder   = TotalScore % WaveCount    ← 加到第 0 波

  for i in [0, WaveCount):
    Budget = BasePerWave + (i==0 ? Remainder : 0)
    WavePlans[i] = BuildWavePlan(Budget, MaxEnemiesPerWave, Room)
```

```
BuildWavePlan(Budget, MaxEnemies, Room):

  Step 1: 触发条件（内部，当前硬编码）
    Plan.TriggerType = AllEnemiesDead

  Step 2: 刷怪方式（程序随机）
    Plan.SpawnMode = RandBool() ? Wave : OneByOne
    Plan.OneByOneInterval = OneByOneDefaultInterval（GameMode 属性）

  Step 3: 计算每只怪的有效成本并按预算填充敌人：
    有效成本 = EnemyData.DifficultyScore
             + RoomBuffCostPerEnemy（= 所有 ActiveRoomBuffs[i].DifficultyScore 之和）
             + EnemyBuffCost（= 选中的敌人专属 Buff 的 DifficultyScore，无则 0）

    敌人专属 Buff 选取：从 EnemyData.EnemyBuffPool 随机选一个；
      仅当预算允许（或为第一只）时才采用，否则不施加 Buff。

    填充规则：
    - 第一只：不受预算限制（保证至少 1 只）
    - 后续：有效成本 ≤ RemainingBudget
    - MaxCountPerLevel：跨波次累计上限（-1 = 不限）
    - MaxEnemiesPerWave：单波上限（0 = 不限）；达上限后优先选高分怪填满最后一槽
    - 循环直到预算耗尽或无合法候选
    - EnemiesToSpawn 元素类型为 FPlannedEnemy（含 EnemyClass + SelectedEnemyBuff）

  Step 4: 剩余预算 → 构建按需补刷池（DemandEnemyPool，TArray<FPlannedEnemy>）
    - 收集有效成本 ≤ 剩余预算的敌人（不受 MaxCountPerLevel 限制）
    - DemandCount = Max(1, RemainingBudget / AvgScore)
```

### 3.3 波次执行（TriggerNextWave）

Wave 和 OneByOne 模式统一使用队列 + 非重复定时器：

```
TriggerNextWave()
  ├─ ClearTimer(DemandSpawnTimer)   ← 清除上一波残留的补刷定时器
  ├─ OneByOneSpawnQueue = Wave.EnemiesToSpawn  （TArray<FPlannedEnemy>）
  ├─ bWaveStaggerMode = (Wave.SpawnMode == Wave)
  └─ SetTimer(OneByOneTimer, FirstDelay)
       FirstDelay = RandRange(SpawnStaggerMin, SpawnStaggerMax)

SpawnNextOneByOne()
  ├─ SpawnEnemyFromPool(PlannedEnemy)
  │    ├─ 白名单过滤：找包含该 Class 的 Spawner（EnemySpawnClassis）
  │    ├─ 随机选一个有效 Spawner → Spawner->SpawnMob()
  │    ├─ 遍历 ActiveRoomBuffs（TArray<FBuffEntry>）→ 激活每个 Entry.RuneDA->RuneInfo.Flow.FlowAsset
  │    └─ 若 Planned.SelectedEnemyBuff != null → 激活敌人专属 Buff Flow Graph
  ├─ 若队列未空：重新 SetTimer(NextDelay)
  │    Wave    模式：NextDelay = RandRange(SpawnStaggerMin, SpawnStaggerMax)
  │    OneByOne模式：NextDelay = Wave.OneByOneInterval
  └─ 若队列已空：SetupWaveTrigger(Wave)
```

### 3.4 波次触发（CheckWaveTrigger）

每次敌人死亡时调用 `UpdateFinishLevel(1)` → `CheckWaveTrigger()`：

```
CheckWaveTrigger()
  ├─ PercentKilled_50：本波击杀 ≥ 50% → 清 DemandSpawnTimer → TriggerNextWave
  ├─ PercentKilled_20：本波击杀 ≥ 20% → 清 DemandSpawnTimer → TriggerNextWave
  ├─ TimeInterval：
  │    TotalAliveEnemies <= 0 → ClearTimer(WaveTriggerTimer) → TriggerNextWave（保底提前触发）
  └─ AllEnemiesDead：
       TotalAliveEnemies <= 0
         ├─ DemandCount > 0 → SetTimer(DemandSpawnTimer, RandRange(StaggerMin, StaggerMax))
         └─ DemandCount == 0 → TriggerNextWave
```

> 当前 `BuildWavePlan` 硬编码 `TriggerType = AllEnemiesDead`，其他触发条件的代码路径已实现，后续可扩展为可配置。

### 3.5 按需补刷（CheckDemandSpawn）

```
CheckDemandSpawn()
  ├─ 从 Wave.DemandEnemyPool（TArray<FPlannedEnemy>）随机选一条目
  ├─ Wave.DemandCount--
  └─ SpawnEnemyFromPool(PlannedEnemy)
       ├─ 成功 → TotalAliveEnemies++、Wave.TotalSpawnedInWave++
       └─ 失败（无合法 Spawner）→ DemandCount 已减，避免死循环
```

### 3.6 关卡完成（CheckLevelComplete）

```
CheckLevelComplete()
  条件：bAllWavesSpawned && TotalAliveEnemies <= 0
  ├─ 最后一波 DemandCount > 0 → 先补刷 → 等待下次调用
  └─ DemandCount == 0 → 广播 OnFinishLevel → EnterArrangementPhase()
```

### 3.7 整理阶段与切关

```
EnterArrangementPhase()
  ├─ 解锁背包 SetLocked(false)
  ├─ 发放金币 RandRange(ActiveTier.GoldMin, ActiveTier.GoldMax)
  ├─ SpawnActor<ARewardPickup>(LastEnemyKillLocation)
  └─ ActivatePortals()

ActivatePortals()（普通关卡；传送门目标在 ActiveRoomData.PortalDestinations）
  ├─ 读下一关 FloorConfig → RollRoomTypeForFloor → RequiredRoomType（所有门共享此类型）
  ├─ Fisher-Yates 洗牌 PortalDestinations
  └─ 遍历洗牌后的列表：
       ├─ SelectRoomByTag(Cfg, RequiredRoomType) → ChosenRoom
       │    优先级：门专属 RoomPool → Campaign 全局 RoomPool → 门专属池任意 → Normal 兜底
       ├─ 第一个必开（bAtLeastOneOpened=false）；后续 50% 概率
       └─ Portal->Open(ChosenRoom->RoomName, ChosenRoom)（关卡名 = DA_Room.RoomName）

ActivateHubPortals()（主城/枢纽房间专用）
  ├─ 所有已配置的传送门立即全开（不走 50% 随机）
  └─ 目标均为 FloorTable[0]（CurrentFloor=0 → TransitionToLevel 写入 PendingNextFloor=1）

玩家接触 APortal → EnterPortal() → GM->TransitionToLevel(LevelName, SelectedRoom)
  ├─ 锁背包
  ├─ 写入 GI.PendingRunState（HP/Gold/Phase/符文/武器）
  ├─ GI.PendingNextFloor = CurrentFloor + 1
  ├─ GI.PendingRoomData  = SelectedRoom（下一关从 GI 读取）
  └─ OpenLevel(LevelName)
```

---

## 四、关键函数索引

| 函数 | 文件 | 说明 |
|------|------|------|
| `StartLevelSpawning()` | `YogGameMode.cpp` | 关卡启动入口，选 RoomData + Hub 判断 + 生成波次计划 |
| `GenerateWavePlans()` | `YogGameMode.cpp` | 将总难度分分配到各波 |
| `BuildWavePlan()` | `YogGameMode.cpp` | 按预算从 EnemyPool 填充单波敌人（含 Buff 成本计算）|
| `TriggerNextWave()` | `YogGameMode.cpp` | 启动新一波刷怪队列 |
| `SpawnNextOneByOne()` | `YogGameMode.cpp` | 队列式刷怪驱动函数 |
| `CheckWaveTrigger()` | `YogGameMode.cpp` | 判断是否触发下一波 |
| `CheckDemandSpawn()` | `YogGameMode.cpp` | 按需补刷 |
| `CheckLevelComplete()` | `YogGameMode.cpp` | 判断关卡是否结束 |
| `EnterArrangementPhase()` | `YogGameMode.cpp` | 进入整理阶段：金币 + RewardPickup + 开门 |
| `ActivatePortals()` | `YogGameMode.cpp` | 普通关卡：随机开门并分配下一关 RoomData |
| `ActivateHubPortals()` | `YogGameMode.cpp` | 主城：全部传送门立即开启，目标 FloorTable[0] |
| `SelectRoomByTag()` | `YogGameMode.cpp` | 按类型 Tag 从 RoomPool 中选 DA_Room |
| `RollRoomTypeForFloor()` | `YogGameMode.cpp` | 按概率骰出下一关的房间类型 Tag |
| `TransitionToLevel()` | `YogGameMode.cpp` | 保存状态 + OpenLevel |
| `SpawnEnemyFromPool()` | `YogGameMode.cpp` | 白名单过滤 Spawner + 生成敌人 + 施加房间 Buff + 敌人专属 Buff |
| `SelectRoomBuffs()` | `YogGameMode.cpp` | Fisher-Yates 洗牌 BuffPool（TArray<FBuffEntry>），取前 N 个 |

---

## 五、关键 Timer 汇总

| Timer | 作用 |
|-------|------|
| `InitialSpawnDelayTimer` | 关卡开始后的初始延迟，之后触发第一波 |
| `OneByOneTimer` | 队列式逐只刷怪（Wave 错开 / OneByOne 间隔） |
| `WaveTriggerTimer` | TimeInterval 类型触发器 |
| `DemandSpawnTimer` | 按需补刷延迟 |

所有 Timer 均在 `TriggerNextWave` 或关卡完成时被正确 ClearTimer。

---

## 六、AYogGameMode 关键属性（程序/BP配置，非 DA）

| 属性 | 说明 |
|------|------|
| `LowDifficultyScoreMax` | 低难度档上限分数（Score ≤ 此值 → Low 档）|
| `HighDifficultyScoreMin` | 高难度档下限分数（Score ≥ 此值 → High 档）|
| `InitialSpawnDelay` | 关卡开始到第一波刷怪的延迟（秒）|
| `SpawnStaggerMin/Max` | Wave 模式下每只敌人之间的随机延迟范围（秒）|
| `OneByOneDefaultInterval` | OneByOne 模式的固定间隔（秒）|
| `CampaignData` | 当前局的 DA_Campaign 引用 |
| `RewardPickupClass` | 奖励拾取物 BP 类（在 GameMode BP 中指定）|

---

## 七、已知 TODO

| 任务 | 优先级 | 状态 |
|------|--------|------|
| 刷怪时将 HealthMultiplier/DamageMultiplier 施加到敌人属性 | 高 | ⚠️ 未实现 |
| 符文三选一蓝图 UI（绑定 OnLootGenerated）| 高 | ⚠️ 未实现 |
| 金币 HUD（绑定 OnGoldChanged）| 中 | ⚠️ 未实现 |
| 传送门最少/最多开门数量配置（MinOpenPortals / MaxOpenPortals）| 中 | ⚠️ 未实现（当前固定"第一个必开 + 后续 50%"）|
| 波次触发条件可配置化（当前 AllEnemiesDead 硬编码）| 远期 | ⏳ 规划中 |
| 随机关卡生成系统（取代手动 FloorTable）| 远期 | ⏳ 规划中 |
| 商店关卡类型 | 远期 | ⏳ 规划中 |

**已完成**：
- ✅ 波次预算按 TotalDifficultyScore 自动分配（总分 / 实际波次数）
- ✅ DA_Room 三档难度（LowDifficulty / MediumDifficulty / HighDifficulty → MaxWaveCount）
- ✅ MobSpawner 白名单过滤（EnemySpawnClassis）
- ✅ 类型上限（MaxCountPerLevel）
- ✅ 按需补刷（DemandCount + DemandEnemyPool）
- ✅ 刷出时间错开（SpawnStaggerMin/Max，Wave/OneByOne 统一队列）
- ✅ 刷怪特效接口（AMobSpawner::OnEnemySpawned BlueprintImplementableEvent）
- ✅ 跨关状态持久化（FRunState 完整实现，见 CrossLevelState_Technical.md）
- ✅ 切关入口（TransitionToLevel 由 Portal 触发）
- ✅ 传送门目标配置在 DA_Room（PortalDestinations）
- ✅ 房间类型骰子（RollRoomTypeForFloor + SelectRoomByTag）
- ✅ 关卡 Buff 有效成本计入波次预算（BuffPool 条目含 DifficultyScore，每只怪扣分）
- ✅ 敌人专属 Buff 池（EnemyData.EnemyBuffPool，BuildWavePlan 时随机选取，扣分）
- ✅ 单波敌人上限（MaxEnemiesPerWave，0=不限，超出时优选高分怪）
- ✅ 主城/枢纽房间支持（bIsHubRoom，跳过刷怪，所有传送门立即开启）
- ✅ 奖励配置移至难度档位（GoldMin/Max、BuffCount、符文权重全在 FRoomDifficultyTier）
- ✅ 敌人行为树字段（EnemyData.BehaviorTree）
