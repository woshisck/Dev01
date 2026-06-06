# 导演系统接口汇总

> 最后更新：2026-05-28
> 维护规范：见 [DocConventions.md](../../../00_入口与规范/DocConventions.md)

导演系统当前由 `UFirstRunTutorialDirectorSubsystem`（教程专属）承担。通用剧情逻辑通过 `StoryEngineSubsystem` 的 RuleSet 配置，无需单独建导演类。

本文分三部分：
1. **导演主动调用**的系统接口
2. **外部系统调用导演**的钩子入口
3. **缺口分析**（有需求但无接口）

---

## 一、导演主动调用的接口

### 关卡与房间控制（Arrangement 时）

通过 `FStoryNextRoomPlan` 结构体向 `YogGameInstanceBase` 写入计划，`AYogGameMode` 在 Arrangement 阶段读取并执行。

**调用时机**：房间生成前（Arrangement Phase）
**调用方式**：`GI->SetPendingStoryNextRoomPlan(Plan)`

`FStoryNextRoomPlan` 可控制的字段：

| 字段 | 类型 | 效果 |
| --- | --- | --- |
| `bForceSinglePortal` | bool | 强制只显示一个传送门 |
| `PortalIndex` | int32 | 指定传送门索引 |
| `RoomDataOverride` | URoomDataAsset* | 指定下一关使用的关卡资产 |
| `bOverrideRewardOptions` | bool | 是否覆盖奖励选项 |
| `RewardOptionsOverride` | TArray\<FLootOption\> | 指定奖励选项（符文/金币/材料） |
| `bSuppressRoomClearRewardPickup` | bool | 抑制普通清房奖励 |
| `bMarkLastEnemyAsSpecialRewardEnemy` | bool | 最后一个敌人携带特殊奖励 |
| `SpecialRewardEnemyLootOptions` | TArray\<FLootOption\> | 特殊奖励敌人的掉落 |
| `bOverrideBuffs` | bool | 是否覆盖关卡 Buff 池 |
| `BuffsOverride` | TArray\<FBuffEntry\> | 指定本关敌人的 Buff 列表 |

---

### 故事事件广播

**接口**：`StoryEngine->BroadcastStoryEventWithPayload(EventTag, ContextTag, AreaTag, ItemTag, Actor, PC)`
**源文件**：`Story/StoryEngineSubsystem.h`（BlueprintCallable）

- **调用时机**：任意（运行时）
- **效果**：触发 RuleSet 中匹配该 EventTag 的规则
- **副作用**：可能触发 Action（弹窗/标志位/任务/解锁等），取决于规则配置
- **已用于**：月光卡获取（`Story.Event.FirstRun.MoonlightObtained`）、终结技获取（`Story.Event.FirstRun.FinisherObtained`）

---

### 直接执行故事动作

**接口**：`StoryEngine->ExecuteStoryAction(FStoryAction, FStoryEventContext)`
**源文件**：`Story/StoryEngineSubsystem.h`（BlueprintCallable）

- **调用时机**：任意（运行时）
- **效果**：绕过规则匹配，直接执行单个 Action
- **已用于**：显示背包教程弹窗（`ShowTutorialPopup`，`tutorial_backpack`）

可执行的 Action 类型见 [StoryEngine_Architecture.md](../Story/StoryEngine_Architecture.md)。

---

### 故事标志位

**接口**：`StoryEngine->SetStoryFlag(FlagTag, EStoryFlagScope Scope, bool)`
**源文件**：`Story/StoryEngineSubsystem.h`（BlueprintCallable）

- **调用时机**：任意
- **效果**：写入指定作用域的标志位
- **作用域**：`Save`（永久写入存档）/ `Run`（本局，死亡后清除）/ `Session`（本次启动，退出后清除）

> 注意：`Map` 不是标志位作用域，它是规则触发策略（`OncePerMap`）。

---

### 功能解锁

**接口**：`MetaProgression->UnlockFeature(FGameplayTag FeatureTag)`
**源文件**：`MetaProgression/YogMetaProgressionSubsystem.h`（BlueprintCallable）

- **调用时机**：剧情触发时（运行时）
- **效果**：解锁指定功能，不花费货币，写入存档
- **副作用**：触发 `OnFeatureUnlocked` 广播

---

### 教程结束（完整状态清理）

教程脚本死亡时，`HandleScriptedDefeatDeath` 执行以下完整流程：

1. `SaveSubsystem->MarkFirstRunTutorialCompleted()` — 标记教程完成，写入存档
2. `SaveSubsystem->ClearRunCheckpoint()` — 清除本局存档点并写盘
3. `GI->ClearRunState()` — 清除 GameInstance 中的局内运行状态

**注意**：这三步是绑定的，不能单独只调用 `MarkFirstRunTutorialCompleted()`。

---

### 强制触发遭遇战

**接口**：`GameMode->StartForcedSurvivalEncounter()`
**源文件**：`GameModes/YogGameMode.h`

- **调用时机**：运行时直接指令（教程终结技获取后）
- **效果**：强制触发 Survival 遭遇战（教程最终战）
- **限制**：当前实现依赖已有的 WavePlan 配置，不等同于"自由指定遭遇内容"；与普通刷怪流程互斥

---

## 二、外部系统调用导演的入口

这些是 GameMode / UI 调用 `FirstRunTutorialDirectorSubsystem` 的钩子，属于导演接口体系的另一个方向。

| 入口 | 调用方 | 触发时机 | 说明 |
| --- | --- | --- | --- |
| `HandleArrangementPhase(GameMode*)` | AYogGameMode | 每次房间生成前 | 导演写入下一关的 Plan |
| `HandleRewardRuneAdded(Rune, Player)` | 奖励发放逻辑 | 玩家拾取符文奖励时 | 检查是否为月光卡，触发相应事件 |
| `HandleSacrificeConfirmed(Rune, Player)` | 献祭系统 | 玩家确认献祭时 | PrayerRoom 阶段强制给予终结技，启动最终战 |
| `ResolveSacrificeRewardOverride(DefaultRune)` | 献祭系统 | 生成献祭奖励时 | PrayerRoom 阶段覆盖默认奖励为终结技 |
| `IsPrayerSacrificeOverrideActive()` | 献祭 UI | 显示献祭奖励前 | 判断是否激活终结技覆盖 |
| `ShouldHandleScriptedDefeatDeath()` | AYogGameMode | 玩家死亡时 | 判断是否应走教程脚本死亡流程 |
| `HandleScriptedDefeatDeath(GameMode*)` | AYogGameMode | 教程最终战结束时 | 执行完整教程结束状态清理 |
| `HandleFirstBackpackOpened(PC*)` | BackpackScreenWidget | 背包首次打开时 | 月光阶段后显示背包教程弹窗（仅一次） |

---

## 三、缺口分析

以下是导演"有需求但无接口"的功能，按实现优先级排序。

### 高优先级（近期开发需要）

#### 刷特定敌人 / 追加波次

- **当前状态**：只能通过 `RoomDataOverride` 间接控制关卡内容，无法在运行时追加一波敌人
- **需求**：支持两种场景：
  1. 本关刷出指定种类的敌人（覆盖 Wave 配置，Arrangement 时）
  2. 特定时机（如剧情节点触发后）运行时追加一波敌人
- **候选接口**（未实现，需另开代码方案设计）：
  - `GameMode::OverrideNextSpawnWave(FSpawnWaveData)` — Arrangement 时写入
  - `GameMode::AppendSpawnWave(FSpawnWaveData)` — 运行时追加
- **状态**：✗ 候选接口，未实现

#### 锁定/解锁传送门

- **当前状态**：传送门由 GameMode 管理，无独立的导演控制接口
- **需求**：教程中需要在特定步骤前阻止玩家进入传送门
- **注意**：实现时需明确碰撞、视觉、HUD 预览、`TryEnter` 防重入、`DisablePortal/NeverOpen` 的关系，不只是加一个 bool 字段
- **候选接口**（未实现）：`APortal::SetLocked(bool)` 或 `GameMode::SetPortalLocked(int32, bool)`
- **状态**：✗ 候选接口，未实现

---

### 硬性需求但非近期（保留占位）

#### 触发实时对话气泡 / NPC 对话流程

- **当前状态**：项目已有 `GameDialogWidget`、`DialogContentDA`、`TutorialRegistryDA`、StoryEncounter 的 `Dialogue` action（映射为 `ShowInfoHint`）。缺少的是"实时气泡/角色对白/玩家可选择跳过"的完整对话流程。
- **需求**：导演在剧情节点触发角色对话，玩家可等待或跳过
- **依赖**：需先设计对话序列系统（DialogSequenceSystem），现有 InfoHint 只能做单行提示
- **候选接口**（系统未设计）：通过扩展 `EStoryActionType` 新增 `PlayDialogue` 类型
- **状态**：✗ 系统未设计，硬性需求

---

## 已覆盖功能确认

| 导演控制需求 | 覆盖方式 |
| --- | --- |
| 指定下一关的关卡类型 | `FStoryNextRoomPlan.RoomDataOverride` ✓ |
| 指定掉落奖励内容 | `FStoryNextRoomPlan.RewardOptionsOverride` ✓ |
| 指定关卡 Buff 池 | `FStoryNextRoomPlan.BuffsOverride` ✓ |
| 强制单传送门 | `FStoryNextRoomPlan.bForceSinglePortal` ✓ |
| 抑制清房奖励 | `FStoryNextRoomPlan.bSuppressRoomClearRewardPickup` ✓ |
| 显示教程弹窗 | `StoryEngine.ExecuteStoryAction(ShowTutorialPopup)` ✓ |
| 写入进度标志（永久） | `StoryEngine.SetStoryFlag(Save)` ✓ |
| 解锁局外功能 | `MetaProgression.UnlockFeature` ✓ |
| 结束教程（完整清理） | `HandleScriptedDefeatDeath`（内含3步状态清理）✓ |

---

## 参考文档

- [SystemDependencyMap.md](../SystemDependencyMap.md) — 系统依赖关系图
- [StoryEngine_Architecture.md](StoryEngine_Architecture.md) — 故事引擎接口详情
- [SaveSubsystem_Architecture.md](../Progression/SaveSubsystem_Architecture.md) — 存档系统接口详情
- [MetaProgression_Architecture.md](../Progression/MetaProgression_Architecture.md) — 元进度接口详情
