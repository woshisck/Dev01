# 第一局教程流程完成结果与待做事项

## 已完成

- 新存档会写入 `Story.Flag.FirstRunTutorial.Active`，完成后可写入 `Story.Flag.FirstRunTutorial.Completed`。
- `YogGameInstanceBase` 支持 `FirstRunTutorialMap`，新存档首局教程可以加载独立教程地图；当前默认仍指向现有 `InitialRoom`。
- `RoomDataAsset` 增加教程字段：
  - `bForceSinglePortal`
  - `ForcedPortalIndex`
  - `bUseFixedRewardOptions`
  - `FixedRewardOptions`
- `RewardPickup` 支持三类奖励：
  - `Rune`：继续走现有卡牌选择 UI。
  - `Gold`：拾取后直接写入背包金币。
  - `Material`：拾取后写入指定 `MetaCurrencyTag`；未指定 tag 时按占位材料拾取处理。
- `CombatDeckComponent` 增加 `OnDeckCardsEntered`，`CombatDeckBarWidget` 已用 C++ 实现 1D 卡组进卡高亮；`BP_OnDeckCardsEntered` 只作为可选蓝图扩展点。
  - 默认时间：0.1s 渐入、0.2s 保持、0.15s 渐出。
  - 可调参数：`EntryHighlightFadeInDuration`、`EntryHighlightHoldDuration`、`EntryHighlightFadeOutDuration`、`EntryHighlightPeakScale`、`EntryHighlightPeakOpacity`。
- loading 图已改为 `imagegen` 生成的真实位图，不再使用程序绘制/SVG 风格占位。
  - 源图：`SourceArt/UI/Loading/T_FirstRunTutorial_Loading.png`
  - UE 资产：`/Game/UI/Loading/T_FirstRunTutorial_Loading`
  - 已绑定到：`/Game/UI/UI_LoadingScreen`
- 新增 `FirstRunLoadingScreenSetup` Commandlet，可重复导入 loading 图并绑定加载屏。

## 已完成（2026-05-25 补充二）

- `ATutorialMobSpawner`：关卡 Actor，调用 `Activate()` 后生成指定敌人；敌人死亡后等待 `RespawnDelay`（默认 5s）自动重新生成。
  - `EnemySpawnClassis[0]`：要生成的敌人 BP
  - `SpawnRadius` / `SpawnZOffset`：复用父类 `AMobSpawner` 的 NavMesh 生成位置逻辑；想固定在 Spawner 附近时把 `SpawnRadius` 设小或设为 0
  - `bActivateOnBeginPlay`：调试用，关卡启动即激活
  - `OnKillEncounterPoint`：击杀时触发的剧情节点（受节点 FirePolicy 控制，仅触发一次）
  - 生成后设置 `bCountsForLevelClear=false`，并从 `GameMode` 的 `AliveEnemies` 里反注册，不参与房间清怪/波次推进
  - 如果生成的是 `ATrainingDummyCharacter`，会自动关闭 `bResetOnDeath`，让它走正常死亡销毁，再由 Spawner 负责 5s 重刷
- `ULENode_ActivateTutorialSpawner`：FA 节点，按 `SpawnerActorTag`（`FName`）找到关卡中的 `ATutorialMobSpawner` 并调用 `Activate()`；分类 `LevelEvent|Tutorial`
- `FirstRunTutorialSpawnerSetupCommandlet`：自动创建/更新首局教程木人相关资产：
  - `/Game/Code/Core/System/B_TutorialMobSpawner`
  - `/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner`
  - `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy`

**教程武器拾取触发木人桩流程：**

```text
WeaponSpawner.PickupEncounterPoint = /Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy
  NodeEventFlow = /Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner
    [Start] → [ActivateTutorialSpawner (Tag: TutorialDummy)] → [Finish]

/Game/Code/Core/System/B_TutorialMobSpawner (tag: TutorialDummy, EnemySpawnClassis[0]=B_EnemyDummy_Tutorial)
  OnKillEncounterPoint = /Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo (FirePolicy=Once)
    → NodeEventFlow = /Game/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard（掉落重击卡）
  RespawnDelay = 5.0s → 无限循环生成
```

## 已完成（2026-05-25 补充）

- `ATrainingDummyCharacter` 新增：`FinishDying()` 广播 `OnCharacterDied`，支持剧情系统接收死亡事件
- `AStoryEncounterDeathListener`：关卡 Actor，无需 Overlap Box，按 `TargetActorName`/`TargetActorTag` 绑定死亡委托触发剧情
- `NodeEventFlow` 字段：`UStoryEncounterPointDA` 和 `FStoryEncounterNode` 均支持绑定 FA，Actions 完成后自动运行
- `AStoryFlowProxy`：FA 运行代理，携带 `ContextTransform`（触发时位置快照）
- `LENode_SpawnRewardPickup`：FA 内生成奖励拾取物的节点（自动读取 ContextTransform）
- `DummyDeathFlowSetupCommandlet`：自动创建 `FA_DummyDeath_DropHeavyCard` 并绑定到 `EP_FirstRun_TrainingDummyCombo`

## 待做

- 配置教程专用地图或确认 `FirstRunTutorialMap` 目标地图。
- 在教程主城中摆放并绑定：
  - 移动提示 Trigger
  - 冲刺提示 Trigger
  - 教程武器拾取点；首次拾取弹窗由 `TryWeaponTutorial` 触发，`PickupEncounterPoint` 绑定到 `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy`，`PickupEncounterGraph` / `PickupEncounterNodeId` 留空
  - 拖入 `/Game/Code/Core/System/B_TutorialMobSpawner`；场景实例设置 `Actor Tag=TutorialDummy`，并按摆位微调 `SpawnRadius` / `SpawnZOffset`
  - 教程传送门
- 配置教程武器 `WeaponDefinition.InitialCombatDeck = [攻击, 攻击]`。
- 打开 `WBP_CombatDeckBar`，确认 `DeckEntryHighlightPanel` 存在，并按观感微调 C++ 暴露的 Entry Highlight 参数。
- 配置教程房间 `RoomDataAsset`：
  - 第一战斗房：金币固定奖励。
  - 第二战斗房：固定三选一卡牌 `[攻击, 重击, 分裂]`，指定 Portal。
  - 连携卡房：特殊敌人死亡掉落 `[月光]`。
  - 过渡房：金币/材料奖励。
  - 祈祷室：献祭获得武器终结技。
- 配置特殊敌人蓝色周身雾效，若走刷怪逻辑则绑定到最后一个敌人。
- 配置无限刷敌失败房，并在玩家死亡后显示回归主城选项。
- 配置教程完成后回主城的最终卡组 `[攻击, 攻击, 月光, 武器终结技]`。
- 修复或确认项目已有缺失资源 `/Game/SlashTrailElemental/Niagara/SlashTrail/NS_SlashTrail_Dark`；它会导致 Commandlet 进程返回失败码，但不影响 loading 图导入结果。
