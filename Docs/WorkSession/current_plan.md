# 开发方案：导演系统（Story FA）升级

## 需求描述

当前故事引擎（StoryEncounterRuntimeSubsystem）中，每个剧情节点的行为由 DA 的 `Actions[]` 数组配置。配置方式是展开一个巨大 struct，逐字段填表，不直观、不可视化、难以理解节点之间的顺序关系。

用户希望：
1. 将每个剧情节点的行为配置从 DA struct 迁移到独立的 FA（Flow Asset）可视化图中。
2. 新建 `UStoryFlowAsset` 作为独立 FA 类型，与 `ULevelFlowAsset`、`UYogRuneFlowAsset` 平级，不共用。
3. 新建 `USNode_Base` 及一组核心故事节点（`USNode_*`），覆盖第一章教程所需的所有操作。
4. 节点应直接调用现有游戏接口（StoryEngineSubsystem、Portal、CombatDeckComponent），不需要重新实现逻辑。

---

## 现有系统分析

### 调用路径
```
DA 配置（FStoryEncounterAction）
  → StoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest()
  → FStoryAction（Type = ShowInfoHint / ShowTutorialPopup / SetFlag / ...）
  → UStoryEngineSubsystem::ExecuteStoryAction()
  → 实际调用（HUD::ShowHint / TutorialManager::ShowPopup / ...）
```

Story FA 节点直接构建 `FStoryAction` 并调用 `StoryEngineSubsystem::ExecuteStoryAction()`，绕过 DA 转换层。对于 `EStoryActionType` 未覆盖的操作（GiveCard、EnablePortal），节点直接调用对应游戏 API。

### EStoryActionType 现有值（已可复用）

| 类型 | 用途 |
|------|------|
| SetFlag / ClearFlag | 记录进度标志 |
| ShowInfoHint | 底部提示条（WeakHint/Dialogue/TutorialAreaHint） |
| ShowTutorialPopup | 教程弹窗（含内联页面 / EventId 注册表两种模式） |
| UnlockFeature | 功能解锁 |
| SetQuestTask | 设置任务目标 |
| PlayLevelFlow | 运行 LevelFlowAsset（已有，不需要新节点） |
| TriggerStoryEvent | 广播故事事件 |

### 已有 LENode 可在 Story FA 中直接复用（无需新 SNode）
- `LENode_Delay` — 延迟
- `LENode_SetStoryFlag` — 设置 Story Flag
- `LENode_BroadcastStoryEvent` — 广播事件
- `LENode_ShowTutorial` — 显示教程弹窗（TutorialManager 路径）
- `LENode_SpawnRewardPickup` — 掉落奖励拾取物
- `LENode_ActivateTutorialSpawner` — 激活 TutorialMobSpawner

---

## 方案设计

### Phase A — 基础类型（2 个新文件）

**`UStoryFlowAsset`**（继承 `UFlowAsset`，非 `ULevelFlowAsset`）
- 纯类型标识类，无额外逻辑
- 目的：编辑器 picker 中仅显示 Story FA；与 Level FA 资产分开管理
- 路径：`Source/DevKit/Public/Story/Flow/StoryFlowAsset.h`

**`USNode_Base`**（继承 `UFlowNode`，非 `ULENode_Base`）
- 提供 story 节点所需的三个便捷访问器：
  - `GetStoryEngine()` → `UStoryEngineSubsystem*`
  - `GetPlayerController()` → `APlayerController*`（世界首个本地 PC）
  - `GetStoryProxy()` → `AStoryFlowProxy*`（从 `TryGetRootFlowActorOwner()` cast）
- 路径：`Source/DevKit/Public/Story/Flow/Nodes/SNode_Base.h`

---

### Phase B — 核心故事节点（5 个新节点）

#### `USNode_ShowHint`
- 分类：`StoryDirector|UI`
- 功能：在底部提示条显示一行提示文字（对应 WeakHint/Dialogue/TutorialAreaHint）
- 字段：`FText HintText`、`float Duration = 3.0f`（0 = 无限直到离开区域）
- 实现：构建 `FStoryAction{Type=ShowInfoHint, HintText, HintDuration}` → `StoryEngine->ExecuteStoryAction()`
- Pin：In → Out（非阻塞，立即触发 Out）

#### `USNode_ShowTutorialPopup`
- 分类：`StoryDirector|UI`
- 功能：显示教程弹窗，玩家关闭后触发 Out（阻塞型）
- 字段：`FName TutorialEventId`、`TArray<FTutorialPage> InlinePages`、`bool bPauseGame = true`
- 实现：构建 `FStoryAction{Type=ShowTutorialPopup}` → `StoryEngine->ExecuteStoryAction()`
- 说明：`TutorialEventId` 和 `InlinePages` 二选一（与现有 LENode_ShowTutorial 机制相同，但走 StoryEngine 路径）
- Pin：In → Out（等待弹窗关闭后触发，通过 `UTutorialManager::OnPopupClosed` 委托接收）

#### `USNode_RecordProgress`
- 分类：`StoryDirector|Progress`
- 功能：写入剧情进度 Flag（Save 作用域）
- 字段：`FName EncounterId`、`FName ProgressKey`（组合为 `Story.Progress.{EncounterId}.{ProgressKey}` tag）
- 实现：调用 `UStoryEncounterRuntimeSubsystem::MakeProgressTag()` 构造 tag，再构建 `FStoryAction{Type=SetFlag, FlagScope=Save}` → `StoryEngine->ExecuteStoryAction()`
- Pin：In → Out（立即触发）

#### `USNode_GiveCard`
- 分类：`StoryDirector|Gameplay`
- 功能：向玩家卡组添加一张符文卡
- 字段：`TObjectPtr<URuneDataAsset> CardToGive`
- 实现：`GetPlayerController()` → `GetPawn()` → 获取 `UCombatDeckComponent` → 调用 `AddCardFromRuneReward(CardToGive)`
- Pin：In → Out（成功）、Failed（找不到卡组组件）

#### `USNode_EnablePortal`
- 分类：`StoryDirector|Level`
- 功能：按 Actor Tag 找到场景中的 `APortal` 并调用 `EnablePortal()`
- 字段：`FName PortalActorTag`（为空时启用所有传送门）
- 实现：`TActorIterator<APortal>` → `ActorHasTag(PortalActorTag)` → `Portal->EnablePortal()`
- Pin：In → Out（立即触发）

---

### Phase C — 字段类型迁移（2 个文件修改）

将 `FStoryEncounterNode.NodeEventFlow` 和 `UStoryEncounterPointDA.NodeEventFlow` 的类型从 `TObjectPtr<UFlowAsset>` 改为 `TObjectPtr<UStoryFlowAsset>`。

**同步修改：**
- `StoryEncounterRuntimeSubsystem::RunFlowViaProxy()` 参数类型从 `ULevelFlowAsset*` → `UFlowAsset*`（`UStoryFlowAsset` 继承 `UFlowAsset`，`AStoryFlowProxy::RunFlow` 无需改动）
- `DummyDeathFlowSetupCommandlet` 从创建 `ULevelFlowAsset` 改为创建 `UStoryFlowAsset`

---

## 实现步骤

1. 新建 `StoryFlowAsset.h/.cpp`（继承 `UFlowAsset`，空类体）
2. 新建 `SNode_Base.h/.cpp`（继承 `UFlowNode`，三个访问器）
3. 新建 `USNode_ShowHint.h/.cpp`
4. 新建 `USNode_ShowTutorialPopup.h/.cpp`（确认 TutorialManager popup-closed 回调名称）
5. 新建 `USNode_RecordProgress.h/.cpp`
6. 新建 `USNode_GiveCard.h/.cpp`
7. 新建 `USNode_EnablePortal.h/.cpp`
8. 修改 `StoryEncounterTypes.h`：`NodeEventFlow` 类型改为 `TObjectPtr<UStoryFlowAsset>`
9. 修改 `StoryEncounterPointDataAsset.h`：同上
10. 修改 `StoryEncounterRuntimeSubsystem.h/.cpp`：`RunFlowViaProxy` 改接受 `UFlowAsset*`
11. 修改 `DummyDeathFlowSetupCommandlet.cpp`：改为创建 `UStoryFlowAsset`

---

## 涉及文件

### 新建
- `Source/DevKit/Public/Story/Flow/StoryFlowAsset.h`
- `Source/DevKit/Private/Story/Flow/StoryFlowAsset.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_Base.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_Base.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_ShowHint.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_ShowHint.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_ShowTutorialPopup.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_ShowTutorialPopup.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_RecordProgress.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_RecordProgress.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_GiveCard.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_GiveCard.cpp`
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_EnablePortal.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_EnablePortal.cpp`

### 修改
- `Source/DevKit/Public/Story/Encounter/StoryEncounterTypes.h` — `NodeEventFlow` 类型改为 `UStoryFlowAsset*`
- `Source/DevKit/Public/Story/Encounter/StoryEncounterPointDataAsset.h` — 同上
- `Source/DevKit/Public/Story/Encounter/StoryEncounterRuntimeSubsystem.h` — `RunFlowViaProxy` 签名
- `Source/DevKit/Private/Story/Encounter/StoryEncounterRuntimeSubsystem.cpp` — `RunFlowViaProxy` 实现
- `Source/DevKitEditor/Story/DummyDeathFlowSetupCommandlet.cpp` — 改为创建 `UStoryFlowAsset`

---

## 潜在风险

- **`DummyDeathFlowSetupCommandlet` 迁移**：当前创建 `ULevelFlowAsset`，改为 `UStoryFlowAsset` 后旧资产 `FA_DummyDeath_DropHeavyCard` 需重新用 commandlet 生成（旧资产删除或让 commandlet 跳过已存在的 LevelFlowAsset）。
- **`USNode_ShowTutorialPopup` 阻塞机制**：需确认 `UTutorialManager` 上是否有 popup-closed 委托；若无则改为非阻塞（触发弹窗后直接 Out），弹窗序列用 `LENode_WaitForLootSelected` 类似方案处理。
- **Flow plugin 节点过滤**：默认情况下 Flow plugin 不按 asset 子类过滤节点，LENode_* 仍会出现在 Story FA 编辑器里。这不影响功能，可后期通过自定义 `UFlowGraphSchema` 过滤。
- **`USNode_GiveCard` 运行时依赖**：依赖 `UCombatDeckComponent`，在没有玩家 pawn 的时机调用会走 Failed 分支，确保在玩家 spawn 后的流程中使用。

---

## 已确认决策

1. **`USNode_ShowTutorialPopup`**：非阻塞，触发弹窗后立即 Out。
2. **`USNode_GiveCard`**：同时写入战斗卡组（`AddCardFromRuneReward`）和背包（`AddRuneToInventory`）。
3. **`USNode_EnablePortal`**：同时支持 `FName PortalActorTag`（Actor Tag）和 `int32 PortalIndex`（`APortal::Index`），两者都填时 Tag 优先。
4. **`DummyDeathFlowSetupCommandlet`**：立即迁移，改为创建 `UStoryFlowAsset`；旧 `FA_DummyDeath_DropHeavyCard`（LevelFlowAsset 类型）下次运行 commandlet 时覆盖重建。
5. **`LENode_ActivateTutorialSpawner`**：保持现有命名，不搬入 SNode 命名空间，两者并存即可。
