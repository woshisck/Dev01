# 刷怪系统技术文档

> 更新：2026-04-22
> 覆盖：MobSpawner 刷怪器 / 波次系统（YogGameMode）/ 预生成 FX

---

## 一、系统概览

```
YogGameMode::StartLevelSpawning()
  ↓ InitialSpawnDelay（1.5s）
TriggerNextWave()
  ↓ FirstDelay（随机错开）
SpawnNextOneByOne()
  ↓
BeginSpawnEnemyFromPool()      ← 选 Spawner → 定位 → 播 FX → SetTimer(FXDuration)
  ↓ FXDuration 后
FinishSpawnFromPool()          ← 真正 SpawnActor → 施 Buff → 计数 → CheckLevelComplete
```

---

## 二、核心文件

| 文件 | 职责 |
|------|------|
| `Mob/MobSpawner.h/.cpp` | 场景中的刷怪点：位置计算、实际 SpawnActor、FX 接口 |
| `GameModes/YogGameMode.h/.cpp` | 波次管理、刷怪调度、预生成序列、关卡完成检测 |
| `Data/EnemyData.h` | 敌人数据资产：包含 PreSpawnFX / PreSpawnFXDuration |
| `GameModes/SpawnTypes.h` | 枚举：ESpawnMode / ESpawnTriggerType；结构：FEnemyEntry / FRoomDifficultyTier |

---

## 三、波次系统（YogGameMode）

### 3.1 刷怪模式

| ESpawnMode | 含义 | 间隔来源 |
|---|---|---|
| `Wave` | 所有敌人随机错开刷出 | `FRandRange(SpawnStaggerMin, SpawnStaggerMax)` |
| `OneByOne` | 固定间隔逐只刷出 | `FWavePlan::OneByOneInterval` |

> `bWaveStaggerMode` 由 BuildWavePlan 根据当前波次敌人数和阈值自动决定。

### 3.2 触发条件（ESpawnTriggerType）

| 类型 | 触发时机 |
|---|---|
| `AllEnemiesDead` | 场内存活数归零（默认）|
| `PercentKilled_50` | 本波 50% 死亡后 |
| `PercentKilled_20` | 本波 20% 死亡后 |
| `TimeInterval` | 固定秒数后，保底：场内无敌人立即触发 |

### 3.3 关键运行时变量

| 变量 | 说明 |
|---|---|
| `TotalAliveEnemies` | 当前已实际 SpawnActor 且存活的敌人数 |
| `PendingSpawnCount` | FX 播放中、尚未 SpawnActor 的敌人数 |
| `bAllWavesSpawned` | 所有波次的 BeginSpawn 均已发出 |
| `CurrentWaveIndex` | 当前波次下标（从 0 开始）|

**CheckLevelComplete 触发条件：**
```cpp
if (!bAllWavesSpawned || TotalAliveEnemies > 0 || PendingSpawnCount > 0) return;
```

---

## 四、预生成 FX 系统

### 4.1 数据配置（填表）

**`DA_Enemy_*`（UEnemyData）：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `PreSpawnFX` | `UNiagaraSystem*` | 预生成粒子资产（留空 = 无 FX）|
| `PreSpawnFXDuration` | `float`（秒）| FX 持续时长，决定怪出现的延迟（0 = 立即刷出）|

**场景中 `MobSpawner` Actor：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `SpawnFXVariance` | `float`（秒）| 随机浮动幅度（±），各区域刷怪点可独立配置 |

**实际 FX 时长 = `PreSpawnFXDuration + FRandRange(-SpawnFXVariance, +SpawnFXVariance)`**

### 4.2 执行流程

```
BeginSpawnEnemyFromPool(Planned)
  ├─ 找 ValidSpawner（EnemySpawnClassis 白名单）
  ├─ Spawner->PrepareSpawnLocation()     ← NavMesh 随机点
  ├─ 计算 FXDuration（基础 + 随机浮动）
  ├─ NiagaraFunctionLibrary::SpawnSystemAtLocation(PreSpawnFX, Location)
  ├─ PendingSpawnCount++
  └─ SetTimer(FXDuration) → FinishSpawnFromPool

FinishSpawnFromPool(Planned, Spawner, Location, WaveIdx)
  ├─ PendingSpawnCount--
  ├─ Spawner->SpawnMobAtLocation(EnemyClass, Location)
  │    └─ SpawnActor → SpawnDefaultController → OnEnemySpawned（BP 出生后回调）
  ├─ 施加 ActiveRoomBuffs + SelectedEnemyBuff
  ├─ TotalAliveEnemies++ / TotalSpawnedInWave++
  └─ CheckLevelComplete()
```

### 4.3 向后兼容

- `PreSpawnFXDuration = 0`（默认）时，`FXDuration <= 0` 分支直接调 `FinishSpawnFromPool`，行为与旧版完全一致
- `CheckDemandSpawn` / `FallbackToPreplacedEnemies` 路径仍调旧 `SpawnEnemyFromPool`（即时刷出），不受影响

---

## 五、MobSpawner 接口速查

| 函数 | 说明 |
|------|------|
| `SpawnMob(EnemyClass)` | 旧接口，随机位置 + 立即刷出（向后兼容）|
| `PrepareSpawnLocation()` | 只计算 NavMesh 随机点，不 SpawnActor |
| `SpawnMobAtLocation(Class, Loc)` | 在指定位置刷出（跳过位置随机化）|
| `OnEnemySpawned(Enemy, Loc)` | BlueprintImplementableEvent：出生后 BP 回调（播出场特效等）|
| `OnPreSpawnFX(Loc, Duration)` | BlueprintImplementableEvent：预生成 BP 扩展（可选，C++ 已自动播 Niagara）|

---

## 六、按需补刷（DemandSpawn）

最后一波 `DemandCount > 0` 时，每死一只怪补刷一只：

```
CheckLevelComplete()
  → CheckDemandSpawn()（延迟 SpawnStaggerMin~Max）
    → SpawnEnemyFromPool(DemandPool 随机一只)
```

> 按需补刷目前走旧 `SpawnEnemyFromPool`（即时刷出），暂不走预生成 FX 路径。

---

## 七、常见问题

**怪一直不出现（FX 播了但怪没刷）**
检查 `FinishSpawnFromPool` 是否被调：Log 中是否有 `[刷怪] 波次N | 存活M`。若无，检查 `WeakSpawner.IsValid()`，Spawner 是否在 FXDuration 期间被销毁。

**关卡不结算（所有怪死亡后卡在 Combat）**
用 `showdebug` 或 Log 确认 `PendingSpawnCount == 0`。若 FXDuration 很长而 Spawner 中途失效，`PendingSpawnCount` 可能未减到 0。

**新敌人类型不刷出**
确认 `BP_MobSpawner` 的 `EnemySpawnClassis` 白名单包含该敌人蓝图类。
