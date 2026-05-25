# 第一局教程流程完成结果与待做事项

## 已完成（按时间顺序）

### 基础系统层（2026-05 早期）

- 新存档写入 `Story.Flag.FirstRunTutorial.Active`；教程完成后可写 `Story.Flag.FirstRunTutorial.Completed`
- `YogGameInstanceBase` 支持 `FirstRunTutorialMap`；当前默认仍指向现有 `InitialRoom`
- `RoomDataAsset` 增加教程字段：`bForceSinglePortal`、`ForcedPortalIndex`、`bUseFixedRewardOptions`、`FixedRewardOptions`
- `RewardPickup` 支持 Rune / Gold / Material 三类奖励
- `CombatDeckComponent` 增加 `OnDeckCardsEntered`，进卡高亮动画 C++ 实现（渐入 0.1s / 保持 0.2s / 渐出 0.15s）
- Loading 图改为真实位图，`FirstRunLoadingScreenSetup` Commandlet 可重复导入

### 教程木人链路（2026-05-25）

- `ATrainingDummyCharacter`：`FinishDying()` 广播 `OnCharacterDied`，支持故事系统接收死亡事件
- `AStoryEncounterDeathListener`：关卡 Actor，按 Tag/Name 绑定死亡委托触发剧情
- `ATutorialMobSpawner`：调用 `Activate()` 后生成指定敌人，死亡后 5s 重刷；`OnKillEncounterPoint` 控制掉卡（FirePolicy=Once）；生成的敌人不参与清怪统计
- `FA_DummyDeath_DropHeavyCard`（Story Director Flow）：木头人死亡掉落重击卡
- `FA_ActivateTutorialDummySpawner`（Story Director Flow）：拾取武器后激活木人桩 Spawner
- `EP_FirstRun_WeaponPickupActivateDummy`：武器拾取触发点，NodeEventFlow 绑定激活 FA
- `EP_FirstRun_TrainingDummyCombo`：木人死亡触发点，NodeEventFlow 绑定掉卡 FA
- 移动提示 Trigger：已摆放并配置
- 冲刺提示 Trigger：已摆放并配置
- 武器相关 Actor Tag：已配置

### Story Director FA 体系（2026-05-25 ～ 26）

- `UStoryFlowAsset`：独立 FA 类型，与关卡 FA / 符文 FA 严格隔离
- `USNode_Base`：SNode 基类，`AllowedAssetClasses = { UStoryFlowAsset }` 强制隔离
- 首批 SNode（7 个）：ShowHint、ShowTutorialPopup、RecordProgress、GiveCard、EnablePortal、SpawnRewardPickup、ActivateTutorialSpawner
- `NodeEventFlow` 字段升级为 `UStoryFlowAsset*`
- `BFNode_Base` 的 `DeniedAssetClasses` 新增 `UStoryFlowAsset`，防止 Buff 节点污染 Story FA 编辑器
- Commandlet 全面迁移到 Story FA
- `StoryFlowAssetFactory`：编辑器内右键创建 Story Director Flow
- `FirstRunTutorialSpawnerSetupCommandlet`：已升级为自动生成
  `[Start] -> SNode_SetActorEnabled(Story.FirstRun.DemoWeapon, true) -> SNode_ActivateTutorialSpawner(TutorialDummy)`
- `ATutorialMobSpawner`：激活后直接按 Spawner 自身位置调用 `SpawnMobAtLocation()`，不再依赖 NavMesh 随机投影；日志会提示是否找到 Spawner、是否生成成功。

### 新增 SNode（2026-05-26）

以下节点覆盖了 DA `Actions[]` 中剩余的主要 ActionKind，完成了从 DA 到 Story FA 的迁移体系：

| 节点 | 功能 | 分类 |
| --- | --- | --- |
| `SNode_SetActorEnabled` | 按 Name/Tag 显示/隐藏关卡 Actor | `StoryDirector\|Level` |
| `SNode_UnlockFeature` | 解锁游戏功能（冲刺、背包等） | `StoryDirector\|Progress` |
| `SNode_SetRoomRewardOverride` | 覆盖/清除当前房间奖励池 | `StoryDirector\|Level` |
| `SNode_SetPortalOverride` | 强制传送门目的地 / 清除覆盖 | `StoryDirector\|Level` |
| `SNode_SetQuestObjective` | 新增/更新任务目标 | `StoryDirector\|Quest` |
| `SNode_TutorialAreaHint` | 常驻区域提示（Duration=0） | `StoryDirector\|UI` |
| `SNode_ShowHint`（扩展） | 新增可选 HintTitle 字段 | `StoryDirector\|UI` |

---

## 待做（优先级排序）

### P0：编译与资产迁移（必须先做）

- [x] 关闭 UE 编辑器，编译 `DevKitEditor` 目标，确认无报错
- [x] 确认旧 ULevelFlowAsset 资产无需再手动删除；当前 commandlet 可加载现有 Story Director Flow：
  - `/Game/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard`
  - `/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner`
- [x] 重新运行两个 Commandlet 重建/刷新 Story Director Flow 类型：
  - `DummyDeathFlowSetupCommandlet`
  - `FirstRunTutorialSpawnerSetupCommandlet`
- [x] 验证 `FA_ActivateTutorialDummySpawner` 包含：
  - `SNode_SetActorEnabled(TargetActorTag=Story.FirstRun.DemoWeapon, bEnabled=true)`
  - `SNode_ActivateTutorialSpawner(SpawnerActorTag=TutorialDummy)`
- [x] `EP_FirstRun_HubMoveHint` 已由 commandlet 写入隐藏正式武器动作：
  - `SetActorEnabled(TargetActorTag=Story.MainRun.StartWeapon, bActorEnabled=false)`

### P1：武器显示由故事引擎控制

- [ ] 找到正常流程起始武器 Spawner Actor，确认 Actor Tag（或设置 Tag 为 `Story.MainRun.StartWeapon`）
- [x] 在最早必然触发的教程入口 Encounter（当前为 `EP_FirstRun_HubMoveHint`）中加入：
  - `SetActorEnabled(TargetActorTag=Story.MainRun.StartWeapon, bActorEnabled=false)`
- [ ] 找到教程演示武器 Spawner Actor，确认 Tag（或设置为 `Story.FirstRun.DemoWeapon`）；默认在关卡中 Hidden In Game = true
- [x] 在武器拾取故事点 `EP_FirstRun_WeaponPickupActivateDummy` 的 NodeEventFlow FA 中加入：
  - `SNode_SetActorEnabled(TargetActorTag=Story.FirstRun.DemoWeapon, bEnabled=true)`
  - `SNode_ActivateTutorialSpawner(SpawnerActorTag=TutorialDummy)`
- [ ] 如果你希望“冲刺提示完成后”就显示教程武器，则在 `EP_FirstRun_HubDashHint` 的 NodeEventFlow FA 中也加入：
  - `SNode_SetActorEnabled(TargetActorTag=Story.FirstRun.DemoWeapon, bEnabled=true)`
- [ ] 在教程完成回主城 Encounter `EP_FirstRun_ReturnHubNormalRunStart` 的 NodeEventFlow FA 中加入：
  - `SNode_SetActorEnabled(TargetActorTag=Story.FirstRun.DemoWeapon, bEnabled=false)`
  - `SNode_SetActorEnabled(TargetActorTag=Story.MainRun.StartWeapon, bEnabled=true)`
  - `SNode_RecordProgress(EncounterId=EM_FirstRun_Tutorial, ProgressKey=Completed)`

### P2：教程关卡编辑器配置

- [ ] 摆放 `B_TutorialMobSpawner`：Tag=`TutorialDummy`，`EnemySpawnClassis[0]=B_EnemyDummy_Tutorial`，`OnKillEncounterPoint=EP_FirstRun_TrainingDummyCombo`
- [ ] 摆放教程传送门，只配置 `Index`；`SelectedLevel` / `SelectedRoom` 是运行时字段，灰色不可填是正常的
- [ ] 在当前 Hub 的 `RoomDataAsset.PortalDestinations` 里配置传送目标：`PortalIndex` 对应传送门 Actor 的 `Index`，`RoomPool` 放可进入的目标 RoomDataAsset
- [ ] 配置教程武器 `WeaponDefinition.InitialCombatDeck = [攻击, 攻击]`

### P3：房间 RoomDataAsset 配置

- [ ] 第一战斗房：金币固定奖励
- [ ] 第二战斗房：固定三选一卡牌 `[攻击, 重击, 分裂]`，指定 Portal
- [ ] 连携卡房：特殊敌人死亡掉落月光卡，配置蓝色周身雾效
- [ ] 过渡房：金币/材料奖励
- [ ] 祈祷室：献祭获得武器终结技
- [ ] 无限刷敌失败房：玩家死亡后显示回归主城选项

### P4：验收

- [ ] 完整 PIE 验证第一局教程链路（见 `FirstRunTutorial_UE_ImplementationChecklist.md` 第 9 节）
- [ ] 修复或确认 `/Game/SlashTrailElemental/Niagara/SlashTrail/NS_SlashTrail_Dark` 缺失资源不影响流程
