# Story Director FA — 节点参考

> 版本：v1.1 | 2026-05-26
> 适用：`UStoryFlowAsset`（Story Director Flow），节点类 `USNode_*`

---

## 概述

Story Director FA 是专为剧情引擎设计的 Flow Asset 类型，与 `ULevelFlowAsset`（关卡 FA）和 `UYogRuneFlowAsset`（符文 FA）严格隔离。

| FA 类型 | C++ 类 | 编辑器显示名 | 用途 |
| --- | --- | --- | --- |
| 剧情导演 | `UStoryFlowAsset` | Story Director Flow | 绑定到 `UStoryEncounterPointDA.NodeEventFlow`，处理剧情专属事件 |
| 关卡 | `ULevelFlowAsset` | — | 关卡流程、传送门、触发器逻辑 |
| 符文 | `UYogRuneFlowAsset` | — | Buff/符文效果逻辑 |

**节点隔离机制：**

- `USNode_*` 仅出现在 Story Director FA 编辑器（`AllowedAssetClasses = { UStoryFlowAsset }`）
- `ULENode_*` 仅出现在关卡 FA（`AllowedAssetClasses = { ULevelFlowAsset }`）
- `UBFNode_*` 被排除在 Story FA 和关卡 FA 之外（`DeniedAssetClasses` 同时含两者）

**上下文访问：** Story FA 由 `AStoryFlowProxy` 运行，节点内可通过 `GetStoryProxy()->GetContextSourceActor()` 获取触发该节点的源 Actor（例如死亡的木头人）。

---

## 节点一览

### USNode_ShowHint — 显示提示

**分类：** `StoryDirector|UI`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `HintTitle` | `FText` | 可选标题。空 = 无标题（WeakHint 样式）；有内容 = 带标题的 InfoHint |
| `HintText` | `FText` | 提示正文（多行） |
| `Duration` | `float` | 持续时长（秒）。0 = 常驻直到被替换；默认 3.0 |

**引脚：** In → Out（立即触发，非阻塞）

**说明：** 调用 `UStoryEngineSubsystem::ExecuteStoryAction`，类型 `ShowInfoHint`。适合轻量、非打断式的引导提示。HintTitle 留空即为原 WeakHint 行为。

---

### USNode_TutorialAreaHint — 区域常驻提示

**分类：** `StoryDirector|UI`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `HintText` | `FText` | 提示文本（多行） |
| `Duration` | `float` | 显示时长（秒）。默认 0 = 常驻，直到被新提示替换 |

**引脚：** In → Out（立即触发，非阻塞）

**说明：** 与 ShowHint 相同底层调用，但默认 Duration=0（常驻）。适合"进入某区域后持续展示操作说明"的场景。

---

### USNode_ShowTutorialPopup — 显示教程弹窗

**分类：** `StoryDirector|UI`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `TutorialEventId` | `FName` | 引用已有 TutorialEventId（可选，设置后忽略 InlinePages） |
| `InlinePages` | `TArray<FTutorialPage>` | 直接写入页面内容 |
| `bPauseGame` | `bool` | 弹窗时是否暂停游戏，默认 true |

**引脚：** In → Out（非阻塞，弹窗不等待玩家关闭）

**说明：** 适合需要打断式教程弹窗的场景（首局引导等）。

---

### USNode_RecordProgress — 记录剧情进度

**分类：** `StoryDirector|Progress`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `EncounterId` | `FName` | 剧情图 ID（如 `EM_FirstRun_Tutorial`） |
| `ProgressKey` | `FName` | 进度键（如 `DummyKilled`） |

**引脚：** In → Out

**说明：** 生成 Tag `Story.Encounter.Progress.{EncounterId}.{ProgressKey}` 并通过 `SetFlag` 写入存档。EncounterId 或 ProgressKey 为空时打印 Warning 并跳过。

---

### USNode_UnlockFeature — 解锁游戏功能

**分类：** `StoryDirector|Progress`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `FeatureTag` | `FGameplayTag` | 要解锁的功能 Tag（如 `Feature.Sprint`） |

**引脚：** In → Out

**说明：** 调用 `UYogMetaProgressionSubsystem::UnlockFeature(FeatureTag)`，写入元进度存档。FeatureTag 无效时打印 Warning 并跳过。

---

### USNode_SetQuestObjective — 设置任务目标

**分类：** `StoryDirector|Quest`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `QuestTaskId` | `FGameplayTag` | 任务 Tag，唯一标识（如 `Quest.FirstRun.FindWeapon`） |
| `DisplayText` | `FText` | 显示给玩家的任务文本 |
| `SourceTag` | `FGameplayTag` | 可选：任务来源 Tag（遗圣目录 / 黑夜少女 / 系统等） |
| `RelatedFlagTag` | `FGameplayTag` | 可选：关联的 StoryFlag Tag |

**引脚：** In → Out

**说明：** 调用 `StoryEngineSubsystem::SetQuestTask()`，写入存档并广播 `OnQuestTaskChanged`。QuestTaskId 无效时打印 Warning 并跳过。

---

### USNode_GiveCard — 给予卡牌

**分类：** `StoryDirector|Gameplay`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `CardToGive` | `URuneDataAsset*` | 要赠予的卡牌 DA |

**引脚：**

| 引脚 | 触发条件 |
| --- | --- |
| Out | 卡牌成功加入牌组和背包 |
| DeckFailed | `AddCardFromRuneReward` 返回 false（牌组已满等） |
| InventoryFailed | 无法获取玩家角色（罕见） |

**说明：** 先调用 `UCombatDeckComponent::AddCardFromRuneReward()`，成功后再调用 `AddRuneToInventory(CardToGive->CreateInstance())`。DeckFailed 时不写背包。

---

### USNode_SetActorEnabled — 显示/隐藏关卡 Actor

**分类：** `StoryDirector|Level`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `TargetActorName` | `FName` | 按 Actor FName 精确匹配 |
| `TargetActorTag` | `FName` | 按 Actor Tags 数组匹配（优先） |
| `bEnabled` | `bool` | true = 显示并启用碰撞；false = 隐藏并禁用碰撞 |

**引脚：** In → Out

**说明：** `TActorIterator<AActor>` 遍历场景，匹配到的 Actor 统一调用 `SetActorHiddenInGame / SetActorEnableCollision / SetActorTickEnabled`。Name 和 Tag 可同时填，任一匹配均生效。无匹配时打印 Warning。

---

### USNode_EnablePortal — 激活传送门

**分类：** `StoryDirector|Level`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `PortalActorTag` | `FName` | 优先通过 Actor Tag 匹配（非空时优先于 Index） |
| `PortalIndex` | `int32` | 按序号匹配；-1 = 忽略 |
| `SelectedLevel` | `FName` | 目标关卡名；空 = 只启用不打开 |
| `SelectedRoom` | `URoomDataAsset*` | 目标房间；null = 只启用不打开 |

**引脚：** In → Out

**执行逻辑：**

1. `TActorIterator<APortal>` 遍历场景，匹配 Tag 或 Index
2. 调用 `EnablePortal()`（视觉激活）
3. 若 `SelectedLevel` 非空且 `SelectedRoom` 非 null，追加调用 `Open(SelectedLevel, SelectedRoom, {})`

---

### USNode_SetRoomRewardOverride — 覆盖房间奖励

**分类：** `StoryDirector|Level`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `bClearOverride` | `bool` | true = 清除已有覆盖，恢复默认奖励池 |
| `LootOptions` | `TArray<FLootOption>` | 覆盖后的奖励选项（bClearOverride=false 时生效） |

**引脚：** In → Out

**说明：** 调用 `AYogGameMode::SetRoomRewardOptionsOverride()` 或 `ClearRoomRewardOptionsOverride()`。GameMode 不是 AYogGameMode 时打印 Warning 并跳过。

---

### USNode_SetPortalOverride — 强制传送门目的地

**分类：** `StoryDirector|Level`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `bClearOverride` | `bool` | true = 清除强制覆盖，恢复正常传送门逻辑 |
| `PortalIndex` | `int32` | 强制目的地序号（bClearOverride=false 时生效）；默认 0 |

**引脚：** In → Out

**说明：** 调用 `AYogGameMode::SetForcedPortalOverride()` 或 `ClearForcedPortalOverride()`。

---

### USNode_SpawnRewardPickup — 生成奖励拾取物

**分类：** `StoryDirector|Gameplay`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `RewardPickupClass` | `TSubclassOf<ARewardPickup>` | 拾取物蓝图类 |
| `RewardLootOptions` | `TArray<FLootOption>` | 奖励选项列表 |
| `RewardPickupCount` | `int32` | 生成数量 |
| `RewardSpawnOffset` | `FVector` | 相对源 Actor 的生成偏移 |
| `bAllowPickupOutsideArrangement` | `bool` | 允许在战斗排列外生成 |

**引脚：** In → Out

**说明：** 生成位置使用 `GetStoryProxy()->GetContextTransform()` 加上 `RewardSpawnOffset`。适用于角色死亡后在原地掉落奖励道具。

---

### USNode_ActivateTutorialSpawner — 激活教程刷怪点

**分类：** `StoryDirector|Tutorial`

| 属性 | 类型 | 说明 |
| --- | --- | --- |
| `SpawnerActorTag` | `FName` | 目标 ATutorialMobSpawner 的 Actor Tag |

**引脚：** In → Out

**说明：** `TActorIterator<ATutorialMobSpawner>` 遍历场景，找到 Tag 匹配的 Spawner 并调用 `Activate()`。

---

## 节点 × ActionKind 对照表

| EStoryEncounterActionKind | 对应 SNode | 备注 |
| --- | --- | --- |
| `WeakHint` | `USNode_ShowHint`（HintTitle 留空） | |
| `TutorialAreaHint` | `USNode_TutorialAreaHint` | Duration=0 |
| `TutorialPopup` | `USNode_ShowTutorialPopup` | |
| `RecordProgress` | `USNode_RecordProgress` | |
| `UnlockFeature` | `USNode_UnlockFeature` | |
| `SetQuestObjective` | `USNode_SetQuestObjective` | |
| `SetActorEnabled` | `USNode_SetActorEnabled` | |
| `SetRoomRewardOverride` | `USNode_SetRoomRewardOverride` | |
| `SetPortalOverride` | `USNode_SetPortalOverride` | |
| `SpawnRewardPickup` | `USNode_SpawnRewardPickup` | |
| `PlayLevelFlow` | — | DA Actions[] 保留；Story FA 内不需要嵌套 Level FA |
| `Dialogue` | — | 阶段二对话系统，暂缓 |
| `TeleportToNode` | — | 暂无用例，暂缓 |

---

## 典型用法

### 教程期间隐藏/显示武器（SNode_SetActorEnabled）

```
教程入口 Encounter（EP_FirstRun_HubMoveHint）NodeEventFlow FA：
[Start] → [SNode_SetActorEnabled]
            TargetActorTag = Story.MainRun.StartWeapon
            bEnabled = false
         → [Out]

冲刺提示完成 Encounter（EP_FirstRun_HubDashHint）NodeEventFlow FA：
[Start] → [SNode_SetActorEnabled]
            TargetActorTag = Story.FirstRun.DemoWeapon
            bEnabled = true
         → [Out]

教程完成回主城 Encounter（EP_FirstRun_ReturnHubNormalRunStart）NodeEventFlow FA：
[Start] → [SNode_SetActorEnabled (DemoWeapon, false)]
         → [SNode_SetActorEnabled (StartWeapon, true)]
         → [SNode_RecordProgress (FirstRun / Completed)]
         → [Out]
```

**关卡配置：**

- 正常流程起始武器 Spawner：Actor Tag = `Story.MainRun.StartWeapon`，默认可见
- 教程演示武器 Spawner：Actor Tag = `Story.FirstRun.DemoWeapon`，**Hidden In Game = true**（关卡默认隐藏）

### 木头人死亡掉落重击卡（SNode_SpawnRewardPickup）

```
[Start] → [SNode_SpawnRewardPickup]
           RewardPickupClass = BP_RewardPickup
           RewardLootOptions = [{ Type=Rune, RuneAsset=DA_Rune512_Heavy }]
           RewardPickupCount = 1
           RewardSpawnOffset = (120, 0, 20)
           bAllowPickupOutsideArrangement = true
```

在 `EP_FirstRun_TrainingDummyCombo` DA：`Kind=Death, FirePolicy=Once, NodeEventFlow=FA_DummyDeath_DropHeavyCard`

### 武器拾取激活刷怪点（SNode_ActivateTutorialSpawner）

```
[Start] → [SNode_SetActorEnabled (Story.FirstRun.DemoWeapon, true（若未在其他节点显示）)]
         → [SNode_ActivateTutorialSpawner (SpawnerActorTag=TutorialDummy)]
         → [Out]
```

在 `EP_FirstRun_WeaponPickupActivateDummy` DA：`Kind=Object, FirePolicy=Once, NodeEventFlow=FA_ActivateTutorialDummySpawner`

### 教程房间强制奖励（SNode_SetRoomRewardOverride）

```
[Start] → [SNode_SetRoomRewardOverride]
           bClearOverride = false
           LootOptions = [{ Type=Rune, RuneAsset=DA_Attack }, { Type=Rune, RuneAsset=DA_HeavyStrike }, ...]
         → [Out]
```

---

## 注意事项

- `NodeEventFlow` 字段类型是 `UStoryFlowAsset*`，不能绑定旧的 `ULevelFlowAsset`
- 若旧 FA 是 `ULevelFlowAsset` 类型，需在编辑器手动删除后重新运行对应 Commandlet
- `AStoryFlowProxy` 在 FA 结束后自动 Destroy，不需要手动清理
- `SNode_SetActorEnabled` 的目标匹配顺序：先检查 Tag（优先），再检查 Name；两者均填时均可匹配
- 节点在 PlayInEditor 和 Standalone 模式下均可工作；不支持服务器端调用
