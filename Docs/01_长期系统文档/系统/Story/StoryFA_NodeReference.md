# Story Director FA — 节点参考

> 版本：v1.0 | 2026-05-25
> 适用：`UStoryFlowAsset`（Story Director Flow），节点类 `USNode_*`

---

## 概述

Story Director FA 是专为剧情引擎设计的 Flow Asset 类型，与 `ULevelFlowAsset`（关卡 FA）和 `UYogRuneFlowAsset`（符文 FA）严格隔离。

| FA 类型 | C++ 类 | 编辑器显示名 | 用途 |
|---------|--------|-------------|------|
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

### USNode_ShowHint — 显示弱引导提示

**分类：** `StoryDirector|UI`

| 属性 | 类型 | 说明 |
|------|------|------|
| `HintText` | `FText` | 提示文本（多行） |
| `Duration` | `float` | 持续时长（秒），默认 3.0 |

**引脚：** In → Out（立即触发，非阻塞）

**说明：** 调用 `UStoryEngineSubsystem::ExecuteStoryAction`，类型 `ShowInfoHint`。适合轻量、非打断式的引导提示。

---

### USNode_ShowTutorialPopup — 显示教程弹窗

**分类：** `StoryDirector|UI`

| 属性 | 类型 | 说明 |
|------|------|------|
| `TutorialEventId` | `FName` | 引用已有 TutorialEventId（可选，若设置则忽略 InlinePages） |
| `InlinePages` | `TArray<FTutorialPage>` | 直接写入页面内容 |
| `bPauseGame` | `bool` | 弹窗时是否暂停游戏，默认 true |

**引脚：** In → Out（非阻塞，弹窗不等待玩家关闭）

**说明：** 适合首局引导等需要打断式教程弹窗的场景。

---

### USNode_RecordProgress — 记录剧情进度

**分类：** `StoryDirector|Logic`

| 属性 | 类型 | 说明 |
|------|------|------|
| `EncounterId` | `FName` | 剧情图 ID（如 `EM_FirstRun_Tutorial`） |
| `ProgressKey` | `FName` | 进度键（如 `DummyKilled`） |

**引脚：** In → Out

**说明：** 生成 Tag `Story.Encounter.Progress.{EncounterId}.{ProgressKey}` 并通过 `SetFlag` 动作持久化存档。若 EncounterId 或 ProgressKey 为空则打印 Warning 并跳过。

---

### USNode_GiveCard — 给予卡牌

**分类：** `StoryDirector|Gameplay`

| 属性 | 类型 | 说明 |
|------|------|------|
| `CardToGive` | `URuneDataAsset*` | 要赠予的卡牌 DA |

**引脚：**

| 引脚 | 触发条件 |
|------|---------|
| Out | 卡牌成功加入牌组和背包 |
| DeckFailed | `AddCardFromRuneReward` 返回 false（牌组已满或不满足条件） |
| InventoryFailed | 无法获取玩家角色（罕见） |

**说明：** 先调用 `UCombatDeckComponent::AddCardFromRuneReward()`，成功后再调用 `AddRuneToInventory(CardToGive->CreateInstance())`。DeckFailed 时不会写入背包。

---

### USNode_EnablePortal — 激活传送门

**分类：** `StoryDirector|Level`

| 属性 | 类型 | 说明 |
|------|------|------|
| `PortalActorTag` | `FName` | 优先通过 Actor Tag 匹配（非空时优先） |
| `PortalIndex` | `int32` | 按序号匹配，-1 = 忽略 |
| `SelectedLevel` | `FName` | 目标关卡名；空 = 只启用不打开 |
| `SelectedRoom` | `URoomDataAsset*` | 目标房间；null = 只启用不打开 |

**引脚：** In → Out

**执行逻辑：**
1. `TActorIterator<APortal>` 遍历场景 Portal，匹配 Tag 或 Index
2. 调用 `EnablePortal()`（视觉激活）
3. 若 `SelectedLevel` 非空且 `SelectedRoom` 非 null，追加调用 `Open(SelectedLevel, SelectedRoom, {})`

---

### USNode_SpawnRewardPickup — 生成奖励拾取物

**分类：** `StoryDirector|Gameplay`

| 属性 | 类型 | 说明 |
|------|------|------|
| `RewardPickupClass` | `TSubclassOf<ARewardPickup>` | 拾取物蓝图类 |
| `RewardLootOptions` | `TArray<FLootOption>` | 奖励选项列表 |
| `RewardPickupCount` | `int32` | 生成数量 |
| `RewardSpawnOffset` | `FVector` | 相对源 Actor 的生成偏移 |
| `bAllowPickupOutsideArrangement` | `bool` | 允许在战斗排列外生成 |

**引脚：** In → Out

**说明：** 生成位置使用 `GetStoryProxy()->GetContextTransform()` 作为基准，加上 `RewardSpawnOffset`。适用于角色死亡后生成奖励道具的场景（如木头人死亡掉落重击卡）。

---

### USNode_ActivateTutorialSpawner — 激活教程刷怪点

**分类：** `StoryDirector|Tutorial`

| 属性 | 类型 | 说明 |
|------|------|------|
| `SpawnerActorTag` | `FName` | 目标 ATutorialMobSpawner 的 Actor Tag |

**引脚：** In → Out

**说明：** `TActorIterator<ATutorialMobSpawner>` 遍历场景，找到 Tag 匹配的 Spawner 并调用 `Activate()`。对应 LENode_ActivateTutorialSpawner 的 Story FA 版本。

---

## 典型用法

### 木头人死亡掉落重击卡

```
[Start] → [SNode_SpawnRewardPickup]
           RewardPickupClass = BP_RewardPickup
           RewardLootOptions = [{ Type=Rune, RuneAsset=DA_Rune512_Knockback }]
           RewardPickupCount = 1
           RewardSpawnOffset = (120, 0, 20)
           bAllowPickupOutsideArrangement = true
```

在 `EP_FirstRun_TrainingDummyCombo` 数据资产中：

```
Kind = Death
FirePolicy = Once
NodeEventFlow = FA_DummyDeath_DropHeavyCard  (UStoryFlowAsset)
```

### 武器拾取激活刷怪点

```
[Start] → [SNode_ActivateTutorialSpawner]
           SpawnerActorTag = TutorialDummy
```

在 `EP_FirstRun_WeaponPickupActivateDummy` 数据资产中：

```
Kind = Object
FirePolicy = Once
NodeEventFlow = FA_ActivateTutorialDummySpawner  (UStoryFlowAsset)
```

---

## 注意事项

- Story FA 的 `NodeEventFlow` 字段类型是 `UStoryFlowAsset*`，不能绑定旧的 `ULevelFlowAsset`
- 若旧 FA（如 `FA_DummyDeath_DropHeavyCard`）是 `ULevelFlowAsset` 类型，需在编辑器中手动删除后重新运行对应 Commandlet
- `AStoryFlowProxy` 在 FA 结束后自动 Destroy，不需要手动清理
- 节点在 PlayInEditor 和 Standalone 模式下均可工作；不支持服务器端调用（GetWorld 需为客户端世界）
