# 刷怪系统迭代 工作报告 2026-04-11

> 类型：工作报告  
> 涉及系统：刷怪系统 / 关卡流程 / MobSpawner

---

## 本次完成内容

### 1. MobSpawner 白名单过滤

**问题**：DA_Room 定义了两种怪，但场景所有 Spawner 都刷全部种类。  
**方案**：`AMobSpawner.EnemySpawnClassis` 作为白名单，`SpawnEnemyFromPool` 只选包含目标类型的 Spawner。

效果：同一场景可以为不同区域配置不同怪种（Soldier 在入口刷，Archer 在高台刷）。

---

### 2. SpawnEnemyFromPool 返回 bool

原函数为 `void`，无法判断是否刷出成功。  
改为 `bool`，白名单无匹配时返回 `false`，调用处（Wave 模式 / OneByOne 模式）只在成功时计数（`TotalAliveEnemies++`、`TotalSpawnedInWave++`）。

---

### 3. 同类型敌人数量上限 + 关卡总数上限

- `FEnemyEntry.MaxCountPerLevel`：该类型跨所有波次最多出现多少只（`-1` = 不限）
- `FDifficultyConfig.MaxTotalEnemies`：本关所有类型合计最多多少只（`-1` = 不限）

`BuildWavePlan` 的 Step 3 在选怪时检查这两个上限，超出则跳过该类型。  
计数使用 `LevelTypeSpawnCounts` TMap 和 `TotalLevelPlannedEnemies` Counter，在 `GenerateWavePlans` 内跨波次累计。

---

### 4. 按需补刷（Demand Spawning）

**场景**：Budget=30，只有 A 类（2分/只，上限 10），刷满 10 只花 20 分，剩余 10 分无法继续填。  
**方案**：剩余预算 → 计算 `DemandCount = 剩余 / 平均分`，构建 `DemandEnemyPool`。

运行时机制：
- 每当场内最后一只死亡（AllEnemiesDead 触发），若 `DemandCount > 0`，随机延迟 1s 补刷一只
- 补刷后 DemandCount--，直至归零才进入下一波
- 最后一波的补刷也由 `CheckLevelComplete` 处理，补完后才结算

百分比/时间触发器触发时，立即取消剩余补刷（`DemandCount = 0`），不阻塞下一波。

---

### 5. 刷出时间错开（Staggered Spawning）

原 Wave 模式同时刷出所有怪，体验单调。

**方案**：Wave 和 OneByOne 统一走队列 + 非重复定时器，每只之间随机延迟：
- Wave 模式：`RandRange(SpawnStaggerMin, SpawnStaggerMax)` 秒（默认 0~0.5s）
- OneByOne 模式：固定 `OneByOneInterval` 秒

配置：`FDifficultyConfig.SpawnStaggerMin/Max`（在 DA_Campaign 的 FloorConfig 内）。

---

### 6. 计时触发可配置 + 保底

原 `TimeInterval_5s` 硬编码 5 秒。

**方案**：
- `ESpawnTriggerType::TimeInterval_5s` 重命名为 `TimeInterval`
- `FSpawnTriggerOption.TriggerInterval`：可配置间隔（默认 3s）
- 保底：`CheckWaveTrigger` 中，若场内无敌人（`TotalAliveEnemies <= 0`），立即取消定时器并触发下一波

---

### 7. 难度配置迁移至 DA_Campaign

**原设计问题**：难度配置（波次数、预算、触发条件）在 DA_Room 里，同一个房间要填多套难度档。策划需要在两个地方配置，且波次预算固定数组（WaveBudgets）不够灵活。

**新设计**：
- `FDifficultyConfig` 直接内嵌到 `FFloorConfig`（DA_Campaign 的每一关条目）
- `WaveBudgets: TArray<int32>` → `WaveBudgetMin/Max: int32`（每波独立随机）
- DA_Room 的 `DifficultyConfigs` 字段完全移除
- `EDifficultyTier`、`FDifficultyEntry` 结构体同步移除

配置变化对比：

| 旧 | 新 |
|----|-----|
| DA_Room → DifficultyConfigs[Low/High] → WaveBudgets[12,15] | DA_Campaign → FloorTable[1] → DifficultyConfig → WaveBudgetMin=12, Max=18 |

---

### 8. 刷怪特效接口

`AMobSpawner::OnEnemySpawned(SpawnedEnemy, SpawnLocation)`：  
`BlueprintImplementableEvent`，在 `SpawnMob()` 成功后立即调用。

在 `BP_MobSpawner` 子类中实现，可播放 Niagara 粒子（在 SpawnLocation）或对 SpawnedEnemy 播放出场 Montage。

---

## 遗留任务

| 任务 | 优先级 |
|------|--------|
| 刷怪时将 HealthMultiplier/DamageMultiplier 施加到敌人属性 | 高 |
| 符文三选一蓝图 UI | 高 |
| 金币 HUD | 中 |
| 传送门最少/最多开门数量配置 | 中 |

## 需要策划重新配置

由于 DA_Room 的 DifficultyConfigs 字段已移除，**所有 DA_Room 不再需要填难度配置**。  
原来在 DA_Room 里的难度参数（波次数、预算、触发条件等）**移至 DA_Campaign → FloorTable 的每一关条目里重新填写**。

参见配置文档：[关卡系统配置指南](../Systems/LevelSystem_ConfigGuide.md)
