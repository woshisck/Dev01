# 第一局教程 UE 实施 Checklist

本文档按当前状态整理，用来指导剩余的 UE 编辑器配置。已经完成的项不要重复做，重点处理“教程流程由故事引擎控制显示/隐藏”和剩余房间配置。

## 0. 当前状态

已完成：

- 移动提示 Trigger 已摆放并配置。
- 冲刺提示 Trigger 已摆放并配置。
- 武器相关 Actor Tag 已配置。
- 教程木头人生成逻辑已走 `B_TutorialMobSpawner` / `ATutorialMobSpawner`。
- `FA_ActivateTutorialDummySpawner` 和 `FA_DummyDeath_DropHeavyCard` 已重建为 Story Director Flow 类型。

仍需在 UE 中完成：

- 正常流程起始武器在教程中不应默认显示，需要由 Story Encounter 动作隐藏，再在教程完成后显示。
- 首局教程演示武器也应由 Story Encounter 动作显示/隐藏。
- 摆放并配置 `B_TutorialMobSpawner`。
- 配置教程传送门、房间奖励、月光房、过渡房、祈愿房。
- 完整 PIE 验证第一局教程链路。

核心原则：

- 不通过删除 Actor 或永久改默认隐藏来解决教程/正常流程切换。
- 教程期间哪些 Actor 可见，应由故事引擎的 `SetActorEnabled` 控制。
- Actor Name 和 Actor Tag 要与故事动作配置一致，避免动作找不到目标。

## 1. 主城配置

### 1.1 移动提示 Trigger

状态：已完成。

仅复查：

- Trigger 能触发移动提示 Encounter。
- 如果该 Encounter 是教程最早稳定触发点，可以顺便承担“隐藏正常流程起始武器”的动作，见 1.4。

### 1.2 冲刺提示 Trigger

状态：已完成。

仅复查：

- Trigger 能触发冲刺提示 Encounter。
- 冲刺提示完成后应显示首局教程演示武器，见 1.3。

### 1.3 首局教程演示武器

用途：教程中玩家拾取的临时武器。

场景 Actor：

- 建议 Actor Name：`WeaponSpawner_FirstRun_DemoSword`
- Actor Tag：`Story.FirstRun.DemoWeapon`
- Pickup Encounter Point：`/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy`

故事动作配置：

- 在 `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubDashHint` 中确认有动作：
  - Action Type：`SetActorEnabled`
  - Target Actor Name：`WeaponSpawner_FirstRun_DemoSword`
  - Target Actor Tag：`Story.FirstRun.DemoWeapon`
  - Actor Enabled：`true`
  - 建议 Action Id：`show_first_run_demo_weapon`

建议初始状态：

- 首局教程演示武器可以在关卡里默认隐藏/禁用，由冲刺提示后的 `SetActorEnabled(true)` 显示。
- 如果你希望一进主城就能看到教程武器，也可以默认显示，但不要影响正常流程起始武器的隐藏逻辑。

武器配置：

- Weapon Definition 的 `InitialCombatDeck` 配置为：
  - 攻击
  - 攻击

验收：

- 新存档进入教程时，玩家不会同时看到“教程演示武器”和“正常流程起始武器”造成困惑。
- 玩家完成冲刺提示后，教程演示武器按预期可见并可拾取。
- 拾取教程演示武器后，能触发 `EP_FirstRun_WeaponPickupActivateDummy`。

### 1.4 正常流程起始武器

当前问题：教程期间正常流程起始武器仍在显示。这个不应通过手动删除或永久隐藏解决，而应纳入故事引擎控制。

场景 Actor：

- 建议 Actor Name：`WeaponSpawner_MainRun_StartWeapon`
- Actor Tag：`Story.MainRun.StartWeapon`
- 这个 Actor 保留在主城中，供教程结束后的正常流程使用。

教程开始时隐藏：

- 优先放在最早必然触发的教程入口 Encounter 中。
- 如果当前没有更早的教程入口 Encounter，可先放在 `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubMoveHint`。
- 注意：如果放在移动提示 Trigger 中，玩家触发移动提示前它不会被隐藏；如果出生点能看到它，后续最好补一个真正的教程入口/Hub Start Encounter。

新增或确认动作：

- Action Type：`SetActorEnabled`
- Target Actor Name：`WeaponSpawner_MainRun_StartWeapon`
- Target Actor Tag：`Story.MainRun.StartWeapon`
- Actor Enabled：`false`
- 建议 Action Id：`hide_main_run_start_weapon_during_first_run`
- 建议 Reuse Key：`LevelActor.MainRun.StartWeapon.HideDuringFirstRun`

教程完成后显示：

- 在 `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_ReturnHubNormalRunStart` 中确认有动作：
  - Action Type：`SetActorEnabled`
  - Target Actor Name：`WeaponSpawner_MainRun_StartWeapon`
  - Target Actor Tag：`Story.MainRun.StartWeapon`
  - Actor Enabled：`true`
  - 建议 Action Id：`show_main_run_start_weapon_after_first_run`

同时确认该 Encounter 里还有：

- 隐藏首局教程演示武器：
  - Target Actor Name：`WeaponSpawner_FirstRun_DemoSword`
  - Target Actor Tag：`Story.FirstRun.DemoWeapon`
  - Actor Enabled：`false`
- 写入教程完成进度：
  - `Story.Flag.FirstRunTutorial.Completed = true`
  - 清理或关闭 `Story.Flag.FirstRunTutorial.Active`

验收：

- 新存档教程期间，正常流程起始武器不可见、不可交互。
- 教程完成回主城后，首局教程演示武器隐藏，正常流程起始武器显示。
- 非教程流程不受影响，正常流程起始武器仍可正常出现。

## 2. 教程木头人

### 2.1 摆放 Spawner

在教程主城中摆放：

- Blueprint：`/Game/Code/Core/System/B_TutorialMobSpawner`
- Actor Tag：`TutorialDummy`
- `EnemySpawnClassis[0]`：教程木头人 BP，例如 `B_EnemyDummy_Tutorial`
- `OnKillEncounterPoint`：`/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo`
- 生成位置面向玩家，避免刷在墙体、不可达区域或传送门旁边。

行为预期：

- 玩家拾取教程武器后，通过 `FA_ActivateTutorialDummySpawner` 激活。
- 生成逻辑仍复用父类 `AMobSpawner::SpawnMobAtLocation()`。
- 生成的木头人不参与房间清怪统计。
- 木头人死亡后 5 秒再次刷新。

### 2.2 击杀奖励

确认 `EP_FirstRun_TrainingDummyCombo` 接到：

- `/Game/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard`

奖励行为：

- 第一次击杀木头人时掉落重攻击卡。
- 掉卡应是一次性，后续 5 秒刷新出来的木头人不重复掉同一张教程卡。

验收：

- 拾取教程武器后，木头人出现。
- 击杀后第一次掉重攻击卡。
- 5 秒后木头人再次出现。
- 再次击杀不会重复触发一次性掉卡。

## 3. 教程传送门

在主城摆放教程传送门。

需要配置：

- Selected Level：教程战斗 Level。
- Selected Room：教程战斗 Room。
- 传送门开启逻辑按现有清怪/进度逻辑走，不需要被教程木头人阻塞。

验收：

- 教程木头人存在或死亡刷新中，都不会阻止主城传送门按预期开启。
- 玩家能通过该传送门进入教程战斗房。

## 4. 教程战斗房

需要配置教程房间的 RoomDataAsset。

至少确认：

- 教程战斗房的敌人组合。
- 金币奖励。
- 卡牌奖励。
- 材料奖励。
- 房间完成后的出口或传送逻辑。

如果某些奖励要由故事引擎覆盖，使用对应 Encounter 的奖励 Override，而不是直接写死到通用房间配置里。

验收：

- 房间能正常开始、清怪、结算。
- 奖励符合第一局教程预期。
- 离开房间后能进入下一段教程链路。

## 5. 月光房

需要配置：

- RoomDataAsset。
- 月光/神龛/交互物 Actor。
- 对应 Encounter Point 或 Story Flow。
- 奖励或状态写入。

验收：

- 玩家进入后能看到并完成月光房核心交互。
- 交互完成后能进入后续房间。

## 6. 过渡房

需要配置：

- 每个过渡房的 RoomDataAsset。
- 出入口连接。
- 是否有奖励。
- 是否需要 Encounter 提示。

验收：

- 房间连接不丢失。
- 玩家不会卡在无出口状态。
- 如果是纯过渡房，不应错误触发战斗结算奖励。

## 7. 祈愿房

需要配置：

- RoomDataAsset。
- 祈愿交互物。
- 对应 Encounter Point 或 Story Flow。
- 奖励、消耗、可选项。

验收：

- 玩家能完成祈愿交互。
- 交互结果正确写入。
- 房间完成后能继续进入后续流程。

## 8. 教程完成回主城

确认教程最终回到主城时触发：

- `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_ReturnHubNormalRunStart`

该 Encounter 应负责：

- 标记第一局教程完成。
- 关闭第一局教程 Active 状态。
- 隐藏首局教程演示武器。
- 显示正常流程起始武器。
- 开启或恢复正常流程传送门/入口。

验收：

- 完成教程后重新回到主城，场景进入正常流程状态。
- 再次进入主城不会重复显示首局教程专用武器。
- 正常流程起始武器可见并可交互。

## 9. 最终 PIE 验证

用新存档验证：

1. 进入主城。
2. 移动提示正常触发。
3. 冲刺提示正常触发。
4. 正常流程起始武器在教程期间不可见。
5. 教程演示武器按预期显示。
6. 拾取教程演示武器。
7. 木头人生成。
8. 击杀木头人，第一次掉重攻击卡。
9. 5 秒后木头人再次刷新。
10. 主城传送门不被木头人阻塞。
11. 进入教程战斗房并完成。
12. 完成月光房、过渡房、祈愿房。
13. 教程结束回主城。
14. 教程演示武器隐藏。
15. 正常流程起始武器显示。

快速排查：

- 正常流程起始武器仍显示：检查 `EP_FirstRun_HubMoveHint` 或更早教程入口 Encounter 是否有 `SetActorEnabled(false)`。
- `SetActorEnabled` 无效：检查 Actor Name 和 Actor Tag 是否与动作配置完全一致。
- 教程武器不显示：检查 `EP_FirstRun_HubDashHint` 是否触发，以及 `SetActorEnabled(true)` 目标是否正确。
- 木头人不刷：检查 Spawner 的 Actor Tag 是否为 `TutorialDummy`，以及拾取武器 Encounter 是否执行 `FA_ActivateTutorialDummySpawner`。
- 木头人刷了但影响清怪：检查 `ATutorialMobSpawner` 是否正确 unregister enemy。
- 掉卡重复：检查 `FA_DummyDeath_DropHeavyCard` / 对应 Encounter 的一次性策略。
