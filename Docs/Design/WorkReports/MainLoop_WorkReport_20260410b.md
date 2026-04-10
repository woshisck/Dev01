# 主循环系统 工作报告 2026-04-10（第二版）

> 适用范围：主循环 / 关卡流程 / 传送门 / 跨关状态  
> 适用人群：策划 + 程序  
> 配套文档：[传送门系统设计](../Systems/Portal_Design.md)、[跨关状态持久化](../Systems/CrossLevelState_Technical.md)  
> 最后更新：2026-04-10

---

## 一、本次完成

| 功能 | 说明 |
|---|---|
| **传送门系统** | APortal 完全重构（去掉 YogMapDefinition 依赖）；新增 ActivatePortals（随机开门）；新增 TransitionToLevel（Portal 切关入口）|
| **奖励拾取物** | ARewardPickup（新 Actor）：生成在最后一个击杀敌人的位置，玩家接触触发战利品选择，触碰后销毁 |
| **跨关状态持久化** | FRunState 完整存储/恢复流程：HP + 金币 + Phase + 非永久符文，切关时写入 GI，新关卡 Possess 后恢复 |
| **BuffDataAsset** | 新 DA 类型，取代 RoomDataAsset.BuffPool 中原先的 GE 类直引用，增加 BuffName / BuffDescription 字段 |
| **EnemyData 重构** | DifficultyScore（难度预算分值）/ bEliteOnly（精英专属）/ EnemyClass 全部移入 UEnemyData DA |
| **难度配置数组化** | FDifficultyEntry[]，取代原先 LowConfig/MediumConfig/HighConfig 三固定字段，支持按需填写档位 |
| **废弃代码清理** | 移除 YogMapDefinition 引用、旧 YogWorldSubsystem 死代码、OnPostLoadMap 旧逻辑 |

---

## 二、已确认设计决策（2026-04-10）

| 决策 | 内容 |
|---|---|
| 传送门视觉 | EnablePortal / DisablePortal 为 BlueprintImplementableEvent，视觉效果（雾效）全由 BP 实现，C++ 不介入 |
| 随机开门算法 | Fisher-Yates 洗牌，洗牌后第一个必开，后续每扇 50% 概率。至少保证 1 扇开启 |
| 传送门随机约束 | 当前真随机（无最少/最多限制）。后续将新增配置字段（MinOpenPortals / MaxOpenPortals）|
| BuffPool 类型 | 使用 DA 资产（UBuffDataAsset），不直接引用 GE 类，便于策划查阅和未来 UI 展示 Buff 信息 |
| 符文切关策略 | 非永久符文写入 RunState 跨关恢复；永久符文由 BeginPlay 自动重放，不写入 RunState |
| 关卡序列 | **当前**：手动填表（CampaignDataAsset.FloorTable）。**计划**：后续改为随机关卡生成（无需手动填表）|

---

## 三、当前完成状态

### 已完成

| 功能 | 状态 | 关键接口 |
|---|---|---|
| 波次刷怪（预算算法）| ✅ | `StartLevelSpawning` / `GenerateWavePlans` / `BuildWavePlan` |
| 结算流程（解锁背包 / 发金币）| ✅ | `EnterArrangementPhase` |
| 关卡 Buff 施加 | ✅ | `SelectRoomBuffs` / `SpawnEnemyFromPool` |
| 传送门系统 | ✅ | `ActivatePortals` / `TransitionToLevel` |
| 奖励拾取物 | ✅ | `ARewardPickup` / `RewardPickupClass` |
| 跨关状态持久化 | ✅ | `FRunState` / `RestoreRunStateFromGI` |
| 符文背包系统 | ✅ | `BackpackGridComponent` |
| 热度系统 | ✅ | `BaseAttributeSet.cpp`（Phase 升降）|
| 金币系统 | ✅ | `AddGold` / `OnGoldChanged` |

### 有框架未完成

| 功能 | 缺失 | 接口/字段已有 |
|---|---|---|
| 血量/伤害倍率应用 | 刷怪时未将 HealthMultiplier/DamageMultiplier 施加到敌人属性 | `FDifficultyConfig.HealthMultiplier / DamageMultiplier` |
| 符文三选一 UI | 蓝图界面未实现 | `OnLootGenerated` 委托 |
| 金币 HUD | HUD 绑定未实现 | `OnGoldChanged` 委托 |
| 传送门随机约束配置 | 无最少/最多开门限制 | 代码内注释预留 |

---

## 四、遗留任务

- [ ] 刷怪时将 HealthMultiplier / DamageMultiplier 应用到敌人属性（**高优先级**）
- [ ] 符文三选一蓝图 UI（绑定 `OnLootGenerated`）
- [ ] 金币 HUD（绑定 `OnGoldChanged`）
- [ ] 传送门最少/最多开门数量配置字段（`FFloorEntry` 或 `FDifficultyConfig`）
- [ ] `RestoreRunStateFromGI` 完成后将 `bIsValid` 置 false（防重复恢复）

---

## 五、下阶段计划

### 近期（结算/UI 优先）
1. **符文三选一蓝图界面** — 绑定 `OnLootGenerated`，实现 UI 展示和选择
2. **金币 HUD** — 绑定 `OnGoldChanged`
3. **血量/伤害倍率** — 刷怪时对敌人 ASC 施加属性倍率 GE

### 中期
4. **随机关卡生成系统** — 取代手动填表的 FloorTable，根据规则池自动组合 DA_Room 序列
5. **传送门随机约束** — MinOpenPortals / MaxOpenPortals 配置

### 远期
6. 商店关卡
7. Boss 关卡流程
8. 局外成长系统（永久解锁 / 遗物 / 元素池）
9. 死亡 UI 和返回大厅流程

---

## 六、架构变更记录

| 变更 | 旧做法 | 新做法 |
|---|---|---|
| 关卡 Buff 配置 | `RoomDataAsset.BuffPool: TArray<TSubclassOf<UGameplayEffect>>` | `TArray<TObjectPtr<UBuffDataAsset>>`，Buff 配置独立为 DA |
| 难度档配置 | 3 个固定字段 LowConfig/MediumConfig/HighConfig | `TArray<FDifficultyEntry>`，按需填写 |
| 敌人数据 | EnemyClass/DifficultyScore 分散在 FEnemyEntry 或 GameMode | 全部集中到 `UEnemyData` DA |
| 切关触发 | ConfirmArrangementAndTransition（手动调用）| 优先由 APortal 触发 TransitionToLevel；旧入口保留兼容 |
| 战利品触发 | EnterArrangementPhase 直接调用 GenerateLootOptions | 改由 ARewardPickup 接触触发，UI 弹出时机延迟到玩家主动走过去 |
