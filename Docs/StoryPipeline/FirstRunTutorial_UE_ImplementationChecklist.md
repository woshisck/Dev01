# 第一局教程 UE 手动配置步骤

本文档只写需要在 Unreal Editor 里手动设置的内容。代码、GameplayTag、Tutorial Registry、`tutorial_heavy_card`、`tutorial_finisher` 已由工程侧处理。

## 1. 主城关卡

打开地图：

- `/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom`
- 实际编辑 Gameplay 子关卡：`/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom_GamePlay`

### 1.1 移动提示 Trigger

在 `InitialRoom_GamePlay` 里放置：

- Blueprint：`/Game/Docs/Map/LevelEvent/BP_LevelEventTrigger`
- Actor Name：`Trigger_Tutorial_Move`

Details 面板填写：

- `EncounterGraph` = `/Game/Story/Encounters/Main_Tutorial_Demo/EG_FirstRun_Tutorial`
- `NodeId` = `hub_move_hint`
- `Trigger Once` = false

摆放要求：

- 放在玩家出生后必经路线。
- Trigger 范围覆盖玩家离开出生点的路径。
- 不要挡路。

### 1.2 冲刺提示 Trigger

复制 `Trigger_Tutorial_Move`：

- Actor Name：`Trigger_Tutorial_Dash`
- `NodeId` = `hub_dash_hint`
- `Trigger Once` = false

摆放要求：

- 放在移动提示之后的第二段路径。
- Trigger 范围覆盖玩家继续前进的位置。

### 1.3 首局演示武器

在 `InitialRoom_GamePlay` 里放置：

- Blueprint：`/Game/Code/Weapon/BP_WeaponSpawner`
- Actor Name：`WeaponSpawner_FirstRun_DemoSword`
- Actor Tag：`Story.FirstRun.DemoWeapon`

Details 面板填写：

- `Weapon Definition` = 首局演示武器 DA，例如 `DA_Weapon_FirstRun_DemoSword`
- `Story Encounter|Pickup`：
  - `PickupEncounterPoint` = `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy`
  - `PickupEncounterGraph` / `PickupEncounterNodeId` 留空
  - `bTriggerPickupEncounterOnce` = true

该 `PickupEncounterPoint` 只做一件事：

- `NodeEventFlow` = `/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner`
- `Actions` 留空，不要放 `ShowTutorialPopup`
- `FirePolicy` = Once

`/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner` 流程：

```text
[Start] → [ActivateTutorialSpawner (SpawnerActorTag=TutorialDummy)] → [Finish]
```

打开首局演示武器 DA：

- `InitialCombatDeck` 填两张攻击卡：
  - `DA_Rune512_Attack`
  - `DA_Rune512_Attack`

说明：

- 首次武器教程由 `TryWeaponTutorial` 自动触发。
- `PickupEncounterPoint` 只负责激活木人 Spawner；武器教程弹窗仍由 `TryWeaponTutorial` 管。
- 不要把已有的武器拾取弹窗 EP 直接绑到这里，否则可能和 `TryWeaponTutorial` 重复弹窗。

### 1.4 正常流程起始武器

在 `InitialRoom_GamePlay` 里放置或确认已有：

- Blueprint：`/Game/Code/Weapon/BP_WeaponSpawner`
- Actor Name：`WeaponSpawner_MainRun_StartWeapon`
- Actor Tag：`Story.MainRun.StartWeapon`

Details 面板填写：

- `Weapon Definition` = 正常流程起始武器 DA

正常流程武器 DA：

- `InitialCombatDeck` 暂时不要放终结技卡。当前教程版本要求终结技只在最后一关发放。
- 推荐填写：
  - `DA_Rune512_Attack`
  - `DA_Rune512_Attack`
  - 月光/连携卡

说明：

- 首局教程开始时隐藏正常流程武器。
- 首局教程完成回主城后显示正常流程武器。
- 如果正式流程后续也要让玩家开局持有终结技卡，再单独恢复该武器 DA 的 `InitialCombatDeck` 配置。

### 1.5 教程木人 Spawner

在武器区域附近放置教程专用 Spawner，而不是直接把木人桩 Actor 放进关卡：

- Blueprint：`/Game/Code/Core/System/B_TutorialMobSpawner`（基于 `ATutorialMobSpawner`）
- Actor Name：`Spawner_TutorialDummy`
- Actor Tag：`TutorialDummy`

Details 面板确认（蓝图默认值已写入，场景实例只需要按摆位微调）：

- `EnemySpawnClassis[0]` = `/Game/Code/Characters/B_EnemyDummy_Tutorial`
- `RespawnDelay` = `5.0`
- `bActivateOnBeginPlay` = false
- `OnKillEncounterPoint` = `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo`
- `SpawnRadius` = `0` 到 `100`，按场景摆位微调（蓝图默认 `0`）
- `SpawnZOffset` = `96`，除非生成高度明显不对

`B_EnemyDummy_Tutorial` 只作为 Spawner 要生成的敌人 BP：

- 父类保持 `ATrainingDummyCharacter`。
- Character/Data 资产如需手动指定，使用 `/Game/Docs/Data/Character/DA_Char_Dummy`。
- 生命值足够玩家练习 1 到 2 轮攻击。
- 不要把 `B_EnemyDummy_Tutorial` 作为场景常驻 Actor 手动摆放。

说明：

- Spawner 生成的木人不会参与房间清怪/波次推进。
- 如果生成的是 `ATrainingDummyCharacter`，Spawner 会自动关闭木人自身的死亡重置，让它正常销毁并由 Spawner 在 5 秒后重刷。

### 1.6 木人桩重击卡奖励

现在不需要在关卡里手动放 `BP_RewardPickup`，也不需要放 `AStoryEncounterDeathListener`。

打开：

- `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo`

确认：

- `FirePolicy` = Once
- `NodeEventFlow` = `FA_DummyDeath_DropHeavyCard`
- `Actions` 里不要再保留内联 `SpawnRewardPickup`，避免和 FA 重复掉卡

打开 `FA_DummyDeath_DropHeavyCard`，确认其中的 `SpawnRewardPickup` 节点：

- `RewardPickupClass` = `/Game/Code/Dungeon/BP_RewardPickup`
- `RewardLootOptions` 只包含 1 个 Rune：
  - `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback`
- `RewardPickupCount` = 1
- `RewardSpawnOffset` 建议 `(120, 0, 20)`
- `bAllowPickupOutsideArrangement` = true

木人第一次死亡时，`Spawner_TutorialDummy.OnKillEncounterPoint` 会触发 `EP_FirstRun_TrainingDummyCombo`，再由 `NodeEventFlow` 在木人死亡位置附近生成重击卡 Pickup。后续木人仍会每 5 秒重刷，但因为 `FirePolicy=Once`，不会重复掉重击卡。

说明：

- 玩家选择该卡并进入卡组后，会自动触发 `tutorial_heavy_card`。
- 不需要再额外配置重击卡教程 Story EP。
- 不要再预放 `RewardPickup_DummyHeavyCard`，否则会出现重复奖励。
- 不要再配置 `TargetActorName = B_EnemyDummy_Tutorial` 这类死亡监听配置；现在死亡事件由 `ATutorialMobSpawner` 自己管理。

### 1.7 主城 Portal

打开 RoomData：

- `/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom`

填写房间自身出口：

- `PortalDestinations` 添加或确认一项：
  - `PortalIndex` = 主城入口 Portal 的 Index
  - `RoomPool` 只放第一战斗房：
    - `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a`

配置要求：

- 主城传送门默认按普通关卡逻辑可用，不要等待木人死亡才开启。
- 教程木人不参与清怪判定，因此不要用“房间清怪后开门”来驱动主城传送门。
- `EP_FirstRun_HubPortalRewardPreview` 如需保留，只做提示/预览，不负责解锁主城出口。

## 2. 战斗房默认 RoomData 与剧情覆盖

### 2.1 第一战斗房：金币奖励

打开：

- `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a`

RoomData 只填写房间自身信息：

- 敌人池放少量基础敌人。

不要在这个 `DA_Room` 里配置教程固定奖励。

奖励在剧情点里配置：

- 打开 `EP_FirstRun_CombatRoom01Gold`
- `Actions` 添加或确认 `SetRoomRewardOverride`
- `RewardLootOptions` 只放 1 项：
  - `LootType` = `Gold`
  - `Amount` = `50`
  - `DisplayName` = `金币`
  - `Icon` = `/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon`

### 2.2 第二战斗房：Buff 与三选一卡

打开：

- `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b`

填写：

- 敌人池放基础敌人。
- Buff Pool 至少放 1 个玩家能看出来的 Buff。

不要在这个 `DA_Room` 里配置教程固定奖励。

奖励在剧情点里配置：

- 打开 `EP_FirstRun_CombatRoom02ThreeCards`
- `Actions` 添加或确认 `SetRoomRewardOverride`
- `RewardLootOptions` 放 3 张 Rune：
  - `[攻击]`：`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack`
  - `[重击]`：正式重击卡，或暂用 `DA_Rune512_Knockback`
  - `[分裂]`：`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split`

Portal 路由：

- `DA_Room` 的 `PortalDestinations` 对应 Index 的 `RoomPool` 只放：
  - `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon`
- 强制只开哪扇门不要写到 `DA_Room`：
  - 打开 `EP_FirstRun_CombatRoom02ThreeCards`
  - `Actions` 添加或确认 `SetPortalOverride`
  - `ForcedPortalIndex` = 通往下一房间的 Portal Index，例如 `0`

## 3. 月光连携卡房

打开：

- `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon`

配置目标：

- 选择一个特殊敌人，或把最后一个敌人作为月光掉落敌人。
- 给该敌人加蓝色周身雾效。
- 该敌人死亡后固定掉落 `[月光]` 或当前正式连携卡。

推荐卡牌：

- `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward`
- 如果当前双手剑使用专属月光卡，则使用正式专属卡。

说明：

- 连携卡进入卡组后，会自动弹 `tutorial_card_link`。
- 不需要在敌人死亡事件上额外配置连携卡教程弹窗。

## 4. 两个过渡房

选择两个普通 RoomData 作为过渡房，或复制现有 Corridor RoomData。

每个过渡房的 `DA_Room` 只填写房间自身信息：

- 不配置教程提示

如果过渡房也需要固定 Gold / Material 奖励，不要写到 `DA_Room`，在对应剧情点里加 `SetRoomRewardOverride`，把 `RewardLootOptions` 配成 Gold / Material。

第二个过渡房额外填写：

- `DA_Room` 的 `PortalDestinations` 对应 Index 的 `RoomPool` 只放：
  - `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom`
- 强制只开哪扇门不要写到 `DA_Room`，在对应剧情点里加 `SetPortalOverride`，填写通往祈祷室的 `ForcedPortalIndex`

## 5. 祈祷室

打开地图：

- `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom`

打开 RoomData：

- `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom`

### 5.1 祭坛

确认或放置：

- `BP_Altar_PrayerSacrifice`
- Actor Name：`Altar_Tutorial_Finisher`

RoomData / 祭坛配置：

- `Room Tags` 包含 `Room.Type.Event`
- 祭坛相关 DA 指向：
  - `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_Altar_PrayRoom_SacrificeEvent`

奖励结果：

- 献祭完成后给玩家武器终结技卡。
- 这张卡必须通过祭坛奖励/关卡蓝图/Room Event 明确加入玩家卡组，不再由双手剑武器自动携带。
- 终结技卡不需要 3 场战斗解锁，进入卡组后即可使用。

说明：

- 终结技卡进入卡组后，会自动弹 `tutorial_finisher`。
- 不需要在祭坛事件上额外配置终结技教程弹窗。

### 5.2 无限刷敌

在祈祷室放置无限刷敌 Spawner：

- Actor Name：`Spawner_Tutorial_Endless`
- 初始禁用

配置要求：

- 玩家获得终结技卡后延迟约 1 秒启用。
- 敌人总量不设上限，难度保证玩家最终死亡。
- 禁止普通通关出口。

死亡后 UI：

- 首局教程 Active 时，死亡界面只显示“回归主城”。
- 不显示普通复活/继续选项。

## 6. 教程完成回主城

回主城流程需要做到：

- 移除 `Story.Flag.FirstRunTutorial.Active`
- 写入 `Story.Flag.FirstRunTutorial.Completed`
- 隐藏 `WeaponSpawner_FirstRun_DemoSword`
- 显示 `WeaponSpawner_MainRun_StartWeapon`
- 回到 `/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom`

## 7. 教程弹窗资产

打开：

- `/Game/Code/Core/GameMode/BP_HUD`

Details 面板确认：

- `TutorialPopupClass` = `/Game/UI/Playtest_UI/WBP_TutorialPopup`
- `TutorialRegistry` = `/Game/Docs/UI/Tutorial/DA_TutorialRegistry`

如果 `TutorialRegistry` 为空，武器拾取、背包、重击卡、连携卡、终结技这些 EventID 都会找不到内容，日志会出现 `TutorialRegistry is not set on BP_HUD`，教程弹窗会被跳过。

打开：

- `/Game/Docs/UI/Tutorial/DA_TutorialRegistry`

确认五个 EventID 映射：

- `tutorial_weapon_pickup` -> `DA_Tutorial_WeaponPickup`
- `tutorial_backpack` -> `DA_Tutorial_Backpack`
- `tutorial_card_link` -> `DA_Tutorial_CardLink`
- `tutorial_heavy_card` -> `DA_Tutorial_HeavyCard`
- `tutorial_finisher` -> `DA_Tutorial_Finisher`

打开这五个 Tutorial DA，文案里的按键必须写成：

- `<input action="Interact"/>`
- `<input action="OpenBackpack"/>`
- `<input action="MouseClick"/>`
- `<input action="Accept"/>`
- `<input action="Move"/>`
- `<input action="ReverseCard"/>`
- `<input action="LightAttack"/>`
- `<input action="HeavyAttack"/>`

不要写：

- `[E]`
- `Space`
- 鼠标左键
- 手柄 A/B/X/Y
- 十字键文字
