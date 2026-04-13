# 刷怪系统 — 敌人 Buff 与预算扩展 工作报告 2026-04-13

> 适用范围：刷怪系统 / 难度预算 / 敌人数据  
> 适用人群：策划 + 程序  
> 配套文档：[关卡系统技术文档](../Systems/LevelSystem_ProgrammerDoc.md)、[关卡 Buff 池配置指南](../FeatureConfig/BuffPool_ConfigGuide.md)  
> 最后更新：2026-04-13

---

## 一、本次完成

| 功能 | 说明 |
|---|---|
| **FBuffEntry 统一结构** | 新增 `FBuffEntry` 结构体（定义在 `EnemyData.h`），字段：`RuneDA` + `DifficultyScore`。用于 DA_Room.BuffPool 和 EnemyData.EnemyBuffPool 两处，取代原先的裸 RuneDataAsset 引用 |
| **关卡 Buff 有效成本计入预算** | DA_Room.BuffPool 条目携带 DifficultyScore，BuildWavePlan 时对每只怪额外扣分（代表 Buff 让怪更强，消耗更多预算） |
| **敌人专属 Buff 池** | UEnemyData 新增 `EnemyBuffPool: TArray<FBuffEntry>`，BuildWavePlan 时随机选一个词条施加，对应 DifficultyScore 额外扣分；预算不足时不选取但怪仍刷出 |
| **行为树字段** | UEnemyData 新增 `BehaviorTree: TObjectPtr<UBehaviorTree>`，支持在 DA 中为每种敌人指定行为树 |
| **单波敌人上限（MaxEnemiesPerWave）** | FRoomDifficultyTier 新增此字段（0=不限）；达到上限后停止填充，最后一个名额优先选高分怪以充分消耗预算 |
| **主城/枢纽房间（bIsHubRoom）** | URoomDataAsset 新增 `bIsHubRoom`；StartLevelSpawning 中检测后跳过刷怪，调用 ActivateHubPortals（所有门立即全开，目标 FloorTable[0]） |
| **奖励配置移至难度档位** | GoldMin/Max、BuffCount、CommonWeight/RareWeight/EpicWeight 从 FFloorConfig 移入 FRoomDifficultyTier，按难度分级配置 |
| **FloorNumber 删除** | FFloorConfig 移除 FloorNumber（无实际用途） |

---

## 二、已确认设计决策（2026-04-13）

| 决策 | 内容 |
|---|---|
| FBuffEntry 定义位置 | 定义在 `EnemyData.h` 而非 `SpawnTypes.h`，避免循环依赖（SpawnTypes.h → EnemyData.h） |
| 敌人专属 Buff 最多选 1 个 | 一只怪在 BuildWavePlan 时最多从 EnemyBuffPool 随机选 1 个，不叠加 |
| Buff 预算扣分时机 | 关卡 Buff 扣分在 BuildWavePlan 调用前已确定（ActiveRoomBuffs 进关时固定）；敌人专属 Buff 扣分在每只怪确认刷出时计算 |
| MaxEnemiesPerWave = 0 表示不限 | 保持向后兼容，旧配置留空即为不限制 |
| 主城房间使用相同 DA 类型 | bIsHubRoom=true 即可，不需要新建 DA 类；EnemyPool / 难度档位可留空 |
| 奖励字段按难度分级 | 高难度档奖励更丰厚，策划在 DA_Room 三档中分别配置，比在 FloorConfig 全局一刀切更灵活 |

---

## 三、代码架构变更

| 变更 | 旧 | 新 |
|---|---|---|
| DA_Room.BuffPool 类型 | `TArray<TObjectPtr<URuneDataAsset>>` | `TArray<FBuffEntry>`（含 DifficultyScore） |
| FWavePlan.EnemiesToSpawn | `TArray<TSubclassOf<AEnemyCharacterBase>>` | `TArray<FPlannedEnemy>`（含 SelectedEnemyBuff） |
| FWavePlan.DemandEnemyPool | `TArray<TSubclassOf<AEnemyCharacterBase>>` | `TArray<FPlannedEnemy>` |
| OneByOneSpawnQueue | 同上 | `TArray<FPlannedEnemy>` |
| ActiveRoomBuffs | `TArray<URuneDataAsset*>` | `TArray<FBuffEntry>` |
| SelectRoomBuffs 返回值 | `TArray<URuneDataAsset*>` | `TArray<FBuffEntry>` |
| SpawnEnemyFromPool 参数 | `TSubclassOf<AEnemyCharacterBase>` | `const FPlannedEnemy&` |
| BuildWavePlan 参数 | `(Budget, Room)` | `(Budget, Room, MaxEnemies)` |
| 奖励配置位置 | FFloorConfig（DA_Campaign 全局） | FRoomDifficultyTier（DA_Room 按档位） |

---

## 四、当前完成状态

| 功能 | 状态 |
|---|---|
| 波次预算算法（基础） | ✅ |
| 关卡 Buff 有效成本计入预算 | ✅ |
| 敌人专属 Buff 池（EnemyBuffPool）| ✅ |
| 单波敌人上限（MaxEnemiesPerWave）| ✅ |
| 主城/枢纽房间（bIsHubRoom）| ✅ |
| 奖励配置按难度分级（FRoomDifficultyTier）| ✅ |
| 行为树字段（EnemyData.BehaviorTree）| ✅ 字段已加，AIController 读取待实现 |
| 血量/伤害倍率应用 | ⚠️ 未实现（字段预留中） |
| 符文三选一 UI | ⚠️ 未实现（OnLootGenerated 委托已存在）|
| 金币 HUD | ⚠️ 未实现（OnGoldChanged 委托已存在）|
| 传送门最少/最多开门数量配置 | ⚠️ 未实现（当前固定"第一个必开 + 后续50%"）|

---

## 五、遗留任务

- [ ] **AIController 读取 EnemyData.BehaviorTree** — 字段已加，需在 AIController 的 `OnPossess` 或 `BeginPlay` 中读取并运行指定行为树
- [ ] **血量/伤害倍率** — 将 FRoomDifficultyTier 扩展 HealthMultiplier / DamageMultiplier，刷怪时施加属性倍率 GE
- [ ] **符文三选一蓝图 UI**（绑定 OnLootGenerated）
- [ ] **金币 HUD**（绑定 OnGoldChanged）
- [ ] **传送门最少/最多开门配置**（MinOpenPortals / MaxOpenPortals）
