# 开发方案：导演系统（Story FA）升级

## 需求描述

当前故事引擎（StoryEncounterRuntimeSubsystem）中，每个剧情节点的行为由 DA 的 `Actions[]` 数组配置。配置方式是展开一个巨大 struct，逐字段填表，不直观、不可视化、难以理解节点之间的顺序关系。

用户希望：
1. 将每个剧情节点的行为配置从 DA struct 迁移到独立的 FA（Flow Asset）可视化图中。
2. 新建 `UStoryFlowAsset` 作为独立 FA 类型，与 `ULevelFlowAsset`、`UYogRuneFlowAsset` 平级，不共用。
3. 新建 `USNode_Base` 及一组核心故事节点（`USNode_*`），覆盖第一章教程所需的所有操作。
4. 节点应直接调用现有游戏接口（StoryEngineSubsystem、Portal、CombatDeckComponent），不需要重新实现逻辑。
5. **Story FA 与 Level FA 严格隔离**：`LENode_*` 节点不在 Story FA 中复用，所有剧情相关操作均使用 `USNode_*`。

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

Story FA 节点直接构建 `FStoryAction` 并调用 `StoryEngineSubsystem::ExecuteStoryAction()`，绕过 DA 转换层。对于 `EStoryActionType` 未覆盖的操作（GiveCard、EnablePortal、SpawnRewardPickup），节点直接调用对应游戏 API。

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

### 节点过滤规则（已审查确认）

- `ULENode_Base` 的 `AllowedAssetClasses = { ULevelFlowAsset::StaticClass() }`，**LENode_* 在 Story FA 中不可用**
- `UBFNode_Base` 目前排除 `ULevelFlowAsset`，但不排除 `UStoryFlowAsset`（需要补充）
- `USNode_Base` 必须设置 `AllowedAssetClasses = { UStoryFlowAsset::StaticClass() }`，确保 SNode 只出现在 Story FA 中

---

## 方案设计

### Phase A — 基础类型（2 个新文件）

**`UStoryFlowAsset`**（继承 `UFlowAsset`，非 `ULevelFlowAsset`）
- 纯类型标识类，无额外逻辑
- 目的：编辑器 picker 中仅显示 Story FA；与 Level FA 资产分开管理
- 路径：`Source/DevKit/Public/Story/Flow/StoryFlowAsset.h`

**`USNode_Base`**（继承 `UFlowNode`，非 `ULENode_Base`）
- 设置 `AllowedAssetClasses = { UStoryFlowAsset::StaticClass() }`（构造函数中）
- 提供 story 节点所需的三个便捷访问器：

  - `GetStoryEngine()` → `UStoryEngineSubsystem*`
  - `GetPlayerController()` → `APlayerController*`（世界首个本地 PC）
  - `GetStoryProxy()` → `AStoryFlowProxy*`（从 `TryGetRootFlowActorOwner()` cast）

- 路径：`Source/DevKit/Public/Story/Flow/Nodes/SNode_Base.h`

---

### Phase B — 核心故事节点（6 个新节点）

#### `USNode_ShowHint`
- 分类：`StoryDirector|UI`
- 功能：在底部提示条显示一行提示文字（对应 WeakHint/Dialogue/TutorialAreaHint）
- 字段：`FText HintText`、`float Duration = 3.0f`（0 = 无限直到离开区域）
- 实现：构建 `FStoryAction{Type=ShowInfoHint, HintText, HintDuration}` → `StoryEngine->ExecuteStoryAction()`
- Pin：In → Out（非阻塞，立即触发 Out）

#### `USNode_ShowTutorialPopup`
- 分类：`StoryDirector|UI`
- 功能：触发教程弹窗后立即触发 Out（**非阻塞**）
- 字段：`FName TutorialEventId`、`TArray<FTutorialPage> InlinePages`、`bool bPauseGame = true`
- 实现：构建 `FStoryAction{Type=ShowTutorialPopup}` → `StoryEngine->ExecuteStoryAction()`
- 说明：`TutorialEventId` 和 `InlinePages` 二选一
- Pin：In → Out（立即触发，不等待玩家关闭）

#### `USNode_RecordProgress`
- 分类：`StoryDirector|Progress`
- 功能：写入剧情进度 Flag（Save 作用域）
- 字段：`FName EncounterId`、`FName ProgressKey`
- Tag 格式：`Story.Encounter.Progress.{EncounterId}.{ProgressKey}`（使用 `MakeProgressTag()` 生成）
- 实现：调用 `UStoryEncounterRuntimeSubsystem::MakeProgressTag()` 构造 tag，构建 `FStoryAction{Type=SetFlag, FlagScope=Save}` → `StoryEngine->ExecuteStoryAction()`；若 tag 无效则打 Warning 并走 Out（不 crash）
- Pin：In → Out（立即触发）

#### `USNode_GiveCard`
- 分类：`StoryDirector|Gameplay`
- 功能：向玩家卡组 + 背包添加一张符文卡
- 字段：`TObjectPtr<URuneDataAsset> CardToGive`
- 实现：
  1. `GetPlayerController()` → `GetPawn()` → 获取 `UCombatDeckComponent` → `AddCardFromRuneReward(CardToGive)`
  2. `GetPlayerController()` → `Cast<APlayerCharacterBase>` → `AddRuneToInventory(FRuneInstance{CardToGive, 1})`
- Pin：In → Out（卡组成功）、DeckFailed（找不到 CombatDeckComponent）、InventoryFailed（找不到 PlayerCharacterBase）
- 两步独立执行，其中一步失败走对应 Failed 分支（另一步已完成的不回滚）

#### `USNode_EnablePortal`
- 分类：`StoryDirector|Level`
- 功能：找到场景中指定的 `APortal`，调用 `EnablePortal()` 表现 + `Open()` 完整开启
- 字段：
  - `FName PortalActorTag`（优先）
  - `int32 PortalIndex = -1`（-1 表示不用 Index 匹配）
  - `FName SelectedLevel`（传给 `Open()`）
  - `TObjectPtr<URoomDataAsset> SelectedRoom`（传给 `Open()`）
- 实现：`TActorIterator<APortal>` → 按 Tag/Index 匹配 → `Portal->EnablePortal()` → `Portal->Open(SelectedLevel, SelectedRoom, {})`
- Pin：In → Out（立即触发）

#### `USNode_SpawnRewardPickup`
- 分类：`StoryDirector|Gameplay`
- 功能：在 `AStoryFlowProxy` 的 `ContextTransform` 位置附近生成奖励拾取物（替代 `LENode_SpawnRewardPickup`，用于 Story FA 中）
- 字段：与 `LENode_SpawnRewardPickup` 完全相同：
  - `TSubclassOf<ARewardPickup> RewardPickupClass`
  - `TArray<FLootOption> RewardLootOptions`
  - `int32 RewardPickupCount = 1`
  - `FVector RewardSpawnOffset = FVector(120.f, 0.f, 20.f)`
  - `bool bAllowPickupOutsideArrangement = true`
- 实现：从 `GetStoryProxy()` 获取 `ContextTransform`，复用 `SpawnRewardPickupAtContext()` 同等逻辑

- Pin：In → Out（立即触发）

---

### Phase C — 字段类型迁移与 Commandlet 更新

#### C1 — NodeEventFlow 类型收窄

将 `FStoryEncounterNode.NodeEventFlow` 和 `UStoryEncounterPointDA.NodeEventFlow` 的类型从 `TObjectPtr<UFlowAsset>` 改为 `TObjectPtr<UStoryFlowAsset>`。

> **注意**：`StoryEncounterRuntimeSubsystem::RunFlowViaProxy()` 当前已是 `UFlowAsset*`，`AStoryFlowProxy::RunFlow()` 也已是 `UFlowAsset*`，**无需修改**。

#### C2 — BFNode_Base 排除 UStoryFlowAsset

在 `UBFNode_Base` 构造函数中，向 `DeniedAssetClasses`（或等效排除列表）添加 `UStoryFlowAsset::StaticClass()`，防止 Buff 节点污染 Story FA 编辑器。

#### C3 — DummyDeathFlowSetupCommandlet 迁移

将 `DummyDeathFlowSetupCommandlet.cpp` 从使用 `ULevelFlowAsset` + `LENode_SpawnRewardPickup` 改为 `UStoryFlowAsset` + `USNode_SpawnRewardPickup`。

具体变更：
- `#include "LevelFlow/LevelFlowAsset.h"` → `#include "Story/Flow/StoryFlowAsset.h"`
- `#include "LevelFlow/Nodes/LENode_SpawnRewardPickup.h"` → `#include "Story/Flow/Nodes/SNode_SpawnRewardPickup.h"`
- `LoadOrCreateFlowAsset` 函数：所有 `ULevelFlowAsset` 替换为 `UStoryFlowAsset`
- `FindSpawnRewardNode` / `EnsureSpawnRewardNode` / `ConfigureRewardNode`：所有 `ULENode_SpawnRewardPickup` 替换为 `USNode_SpawnRewardPickup`
- 函数签名和内部转换同步更新

旧资产 `FA_DummyDeath_DropHeavyCard`（当前为 `ULevelFlowAsset`）在下次运行 commandlet 时由于类型不匹配会 `LoadObject` 失败，进入新建分支自动覆盖重建为 `UStoryFlowAsset`。

---

## 实现步骤

1. 新建 `StoryFlowAsset.h/.cpp`（继承 `UFlowAsset`，空类体）
2. 新建 `SNode_Base.h/.cpp`（继承 `UFlowNode`，设 AllowedAssetClasses，三个访问器）
3. 新建 `USNode_ShowHint.h/.cpp`
4. 新建 `USNode_ShowTutorialPopup.h/.cpp`（非阻塞）
5. 新建 `USNode_RecordProgress.h/.cpp`
6. 新建 `USNode_GiveCard.h/.cpp`（3 输出 Pin）
7. 新建 `USNode_EnablePortal.h/.cpp`（EnablePortal + Open，带 SelectedLevel/Room 字段）
8. 新建 `USNode_SpawnRewardPickup.h/.cpp`（替代 LENode 用于 Story FA）
9. 修改 `StoryEncounterTypes.h`：`NodeEventFlow` 类型改为 `TObjectPtr<UStoryFlowAsset>`
10. 修改 `StoryEncounterPointDataAsset.h`：同上
11. 修改 `BFNode_Base.h/.cpp`：DeniedAssetClasses 加 `UStoryFlowAsset`
12. 修改 `DummyDeathFlowSetupCommandlet.cpp`：全部改为 `UStoryFlowAsset` + `USNode_SpawnRewardPickup`

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
- `Source/DevKit/Public/Story/Flow/Nodes/SNode_SpawnRewardPickup.h`
- `Source/DevKit/Private/Story/Flow/Nodes/SNode_SpawnRewardPickup.cpp`

### 修改
- `Source/DevKit/Public/Story/Encounter/StoryEncounterTypes.h` — `NodeEventFlow` 类型改为 `UStoryFlowAsset*`
- `Source/DevKit/Public/Story/Encounter/StoryEncounterPointDataAsset.h` — 同上
- `Source/DevKit/Public/BuffFlow/Nodes/BFNode_Base.h` — DeniedAssetClasses 加 `UStoryFlowAsset`（需确认字段名）
- `Source/DevKit/Private/BuffFlow/Nodes/BFNode_Base.cpp` — 同上
- `Source/DevKitEditor/Story/DummyDeathFlowSetupCommandlet.cpp` — 全面替换为 UStoryFlowAsset + USNode_SpawnRewardPickup

---

## 潜在风险

- **旧 `FA_DummyDeath_DropHeavyCard` 类型不兼容**：当前为 `ULevelFlowAsset`，改 NodeEventFlow 为 `UStoryFlowAsset*` 后编辑器加载时该引用会变 null；需在编辑器打开前运行新 commandlet 重建资产。
- **Flow plugin 节点过滤**：即使设置了 `AllowedAssetClasses`，Flow 编辑器有时仍显示所有可用节点；编辑阶段验证一次确认过滤生效。
- **`USNode_GiveCard` 运行时依赖**：依赖玩家 Pawn 存在；在没有 Pawn 的时机调用会走 DeckFailed，确保在玩家 spawn 后使用。
- **`BFNode_Base` DeniedAssetClasses 字段名**：需编译期确认 Flow plugin 的排除列表 API 名称（可能是 `NotAllowedAssetClasses` 或构造里直接设 `AllowedAssetClasses`）。
- **`USNode_EnablePortal::Open()` 参数**：`APortal::Open` 签名为 `(FName, URoomDataAsset*, const TArray<FBuffEntry>&)`；SelectedLevel/SelectedRoom 留空时跳过 `Open()` 调用，只执行 `EnablePortal()`。

---

## 已确认决策

1. **`USNode_ShowTutorialPopup`**：非阻塞，触发弹窗后立即 Out。
2. **`USNode_GiveCard`**：同时写入战斗卡组（`AddCardFromRuneReward`）和背包（`AddRuneToInventory`）；各自独立 Pin：Out / DeckFailed / InventoryFailed。
3. **`USNode_EnablePortal`**：同时支持 `FName PortalActorTag`（优先）和 `int32 PortalIndex`；先调 `EnablePortal()` 再调 `Open()`（SelectedLevel/Room 留空时跳过 Open）。
4. **`DummyDeathFlowSetupCommandlet`**：立即迁移，改为创建 `UStoryFlowAsset` 并使用 `USNode_SpawnRewardPickup`。
5. **LENode_* 与 Story FA 严格隔离**：不复用，所有 Story FA 逻辑均使用 `USNode_*`。
6. **`LENode_ActivateTutorialSpawner`**：保持现有命名，继续用于 Level FA（武器拾取流程），无需迁移。
7. **`MakeProgressTag` 前缀**：实际生成 `Story.Encounter.Progress.{EncounterId}.{ProgressKey}`（不是 `Story.Progress.*`）。
8. **`RunFlowViaProxy` 已是 `UFlowAsset*`**：无需修改此签名。
