# 关卡系统 — 程序技术文档

> 最后更新：2026-04-10（更新数据结构、整理阶段、传送门/切关接入）  
> 配套文档：[跨关状态持久化](CrossLevelState_Technical.md)、[传送门系统设计](Portal_Design.md)

---

## 一、系统概览

关卡系统由以下三层构成：

```
数据层（Data Assets）
  ├── URoomDataAsset       单房间配置（敌人/Buff/战利品/三套难度）
  └── UCampaignDataAsset   局内关卡序列（FloorTable）

逻辑层（GameMode）
  └── AYogGameMode         波次刷怪、阶段切换、战利品发放

角色层（PlayerCharacterBase）
  └── AddGold / AddRuneToInventory
```

---

## 二、数据结构

### URoomDataAsset（`Source/DevKit/Public/Data/RoomDataAsset.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `RoomName` | `FName` | 房间标识，调试用 |
| `bIsEliteRoom` | `bool` | 精英关标记，控制 `bEliteOnly` 怪物的出现 |
| `EnemyPool` | `TArray<FEnemyEntry>` | 敌人池，含难度分和精英标记 |
| `BuffPool` | `TArray<TObjectPtr<UBuffDataAsset>>` | 进关时随机选 N 个 Buff DA 施加给所有怪（旧 GE 直引用已移除）|
| `LootPool` | `TArray<TObjectPtr<URuneDataAsset>>` | 符文掉落池，关卡结束三选一 |
| `DifficultyConfigs` | `TArray<FDifficultyEntry>` | 难度配置数组，按需填写档位（替代原 LowConfig/MediumConfig/HighConfig）|

`FDifficultyEntry`：
```cpp
Tier   EDifficultyTier   // Low / Medium / High / Elite
Config FDifficultyConfig // 该档位的完整配置
```

### FDifficultyConfig（`Source/DevKit/Public/GameModes/SpawnTypes.h`）

| 字段 | 类型 | 说明 |
|------|------|------|
| `WaveCountMin/Max` | `int32` | 波次数随机范围 |
| `WaveBudgets` | `TArray<int32>` | 每波难度分预算，超出长度循环使用最后值 |
| `AllowedTriggers` | `TArray<FSpawnTriggerOption>` | 可用触发条件池（含费用） |
| `AllowedSpawnModes` | `TArray<FSpawnModeOption>` | 可用刷怪方式池（含费用） |
| `HealthMultiplier` | `float` | 怪物血量倍率 |
| `DamageMultiplier` | `float` | 怪物伤害倍率 |
| `BuffCount` | `int32` | 从 BuffPool 选取的数量 |
| `GoldMin/Max` | `int32` | 关卡结束金币奖励范围 |

### UCampaignDataAsset（`Source/DevKit/Public/Data/CampaignDataAsset.h`）

- `FloorTable: TArray<FFloorEntry>` — 按顺序排列的关卡序列

### FFloorEntry

| 字段 | 类型 | 说明 |
|------|------|------|
| `FloorNumber` | `int32` | 序号，仅策划参考 |
| `RoomData` | `URoomDataAsset*` | 指向对应房间配置 |
| `Difficulty` | `EDifficultyTier` | Low / Medium / High / Elite |
| `LevelName` | `FName` | UE 关卡资产名（旧系统回退兼容，新系统由传送门决定）|
| `PortalDestinations` | `TArray<FPortalDestConfig>` | 本关各传送门目标配置（Index → NextLevelPool）|

---

## 三、运行时流程

### 3.1 刷怪流程

```
StartPlay()
  └─ CampaignData != null → StartLevelSpawning()
       ├─ 读 FloorTable[CurrentFloor-1]
       ├─ 选难度 Config（Low/Medium/High）
       ├─ 缓存 ActiveRoomData、ActiveDifficultyConfig
       ├─ SelectRoomBuffs() → ActiveRoomBuffs
       ├─ GenerateWavePlans() → WavePlans[]
       └─ TriggerNextWave() → 开始第一波
```

### 3.2 波次生成算法（BuildWavePlan）

每波按预算三步消耗：

1. **选触发条件**：从 `AllowedTriggers` 中筛出费用 ≤ 剩余预算的条件，随机选一个
2. **选刷怪方式**：从 `AllowedSpawnModes` 中筛出费用 ≤ 剩余预算的方式，随机选一个
3. **填充敌人**：循环从 `EnemyPool` 按剩余预算随机选怪，第一只不受预算限制（保证至少有一只）

### 3.3 波次推进

```
UpdateFinishLevel(count) ← 由 EnemyCharacterBase 死亡时调用，count 传 1
  ├─ TotalAliveEnemies--
  ├─ CurrentWave.TotalKilledInWave++
  ├─ CheckWaveTrigger()
  │    ├─ AllEnemiesDead  → TotalAliveEnemies <= 0
  │    ├─ PercentKilled50 → Killed >= Spawned * 0.5
  │    ├─ PercentKilled20 → Killed >= Spawned * 0.2
  │    └─ TimeInterval5s  → 定时器触发（SetupWaveTrigger 中设置）
  └─ CheckLevelComplete() → bAllWavesSpawned && TotalAliveEnemies <= 0
```

### 3.4 整理阶段

```
EnterArrangementPhase()
  ├─ CurrentPhase → Arrangement
  ├─ OnPhaseChanged.Broadcast()
  ├─ BackpackGridComponent.SetLocked(false)
  ├─ AddGold(RandRange(GoldMin, GoldMax))
  ├─ SpawnActor<ARewardPickup>(RewardPickupClass, LastEnemyKillLocation)
  │    └─ 玩家靠近时：GM->GenerateLootOptions() → OnLootGenerated.Broadcast
  └─ ActivatePortals()
       ├─ 读 FloorTable[CurrentFloor-1].PortalDestinations
       ├─ GetAllActorsOfClass(APortal) → 建 Index→APortal* 映射
       ├─ Fisher-Yates 洗牌 PortalDestinations
       └─ 第一个必开，后续 50% 随机 → Portal->Open(随机 NextLevel)
```

⚠️ `GenerateLootOptions` 不再在 `EnterArrangementPhase` 直接调用，而是由玩家接触 `ARewardPickup` 触发。

### 3.5 切关流程（TransitionToLevel）

由 `APortal::EnterPortal_Implementation` 调用：

```
TransitionToLevel(FName NextLevel)
  ├─ CurrentPhase → Transitioning
  ├─ OnPhaseChanged.Broadcast()
  ├─ Backpack->SetLocked(true)
  ├─ 构建 FRunState（HP / Gold / Phase / 非永久符文）→ GI->PendingRunState
  ├─ GI->PendingNextFloor = CurrentFloor + 1
  └─ OpenLevel(NextLevel)
```

详细数据流参见 [跨关状态持久化技术文档](CrossLevelState_Technical.md)。

---

## 四、金币系统

**位置：** `APlayerCharacterBase`（`Source/DevKit/Public/Character/PlayerCharacterBase.h`）

```cpp
int32 Gold = 0;                                  // 当前持有金币
FGoldChangedDelegate OnGoldChanged;              // UI 监听此委托

void AddGold(int32 Amount);                      // 发放金币，值不低于 0
int32 GetGold() const;                           // 读取当前金币
```

UI 绑定示例（蓝图中）：
- `OnGoldChanged` 委托绑定到 HUD 组件，参数 `NewGold` 为最新总量

---

## 五、战利品发放流程

```
GenerateLootOptions() → OnLootGenerated(TArray<FLootOption>)
                          │
SelectLoot(int32 Index) ──┘
  └─ Player->AddRuneToInventory(Chosen.RuneAsset->CreateInstance())

ConfirmArrangementAndTransition()
  └─ BackpackGridComponent.SetLocked(true)
  └─ OpenLevel(LevelSequenceData->NextLevelName)
```

`FLootOption` 结构：
```cpp
ELootType LootType;         // 目前只有 Rune
URuneDataAsset* RuneAsset;  // LootType == Rune 时有效
```

---

## 六、需要外部配合的接入点

| 接入点 | 位置 | 说明 |
|--------|------|------|
| 敌人死亡通知 | `EnemyCharacterBase::Die()` | 调用 `GameMode->UpdateFinishLevel(1)` |
| MobSpawner | 场景中放置 | `SpawnEnemyFromPool` 随机选一个出生点 |
| 关卡 Buff 施加 | `SpawnEnemyFromPool` 末尾 | TODO：需要 MobSpawner 返回刷出的 Actor |
| 下一关切换 | `ConfirmArrangementAndTransition` | 当前读 `LevelSequenceData->NextLevelName` |

---

## 七、已知 TODO

| 任务 | 优先级 | 状态 |
|------|--------|------|
| 刷怪时将 HealthMultiplier/DamageMultiplier 施加到敌人属性 | 高 | ⚠️ 未实现 |
| 符文三选一蓝图 UI（绑定 OnLootGenerated）| 高 | ⚠️ 未实现 |
| 金币 HUD（绑定 OnGoldChanged）| 中 | ⚠️ 未实现 |
| 传送门随机约束配置（MinOpenPortals / MaxOpenPortals）| 中 | ⚠️ 未实现 |
| 随机关卡生成系统（取代手动 FloorTable）| 远期 | ⏳ 规划中 |
| 商店关卡类型 | 远期 | ⏳ 规划中 |
| `ELootType`：预留 Gold 掉落类型 | 低 | ⏳ 规划中 |

**已完成 TODO（本次会话）**：
- ✅ `SpawnEnemyFromPool`：BuffDataAsset 施加已实现
- ✅ 切关入口：TransitionToLevel（由 Portal 触发）
- ✅ 跨关状态持久化：FRunState 完整实现
