# 第一局教程 UE 配置与检查流程

本文档按当前剧情源 `Docs/StorySource/Main_Tutorial_Demo/first_run_tutorial.story.json` 拆分，目标是把 `EG_FirstRun_Tutorial` 的剧情点真正落到 UE 关卡、RoomData、Reward、Portal、UI 与保存流程中。

## 0. 开始前确认

1. 先同步并编译当前工程。
   - 目标：`DevKitEditor Win64 Development`
   - 命令：
     ```powershell
     & 'Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Build\BatchFiles\Build.bat' DevKitEditor Win64 Development -Project="X:\Project\Dev01\DevKit.uproject" -WaitMutex -NoHotReload
     ```
2. 确认 Finisher QTE 崩溃修复已在本地。
   - 打开 `Content/UI/Playtest_UI/HUD/WBP_FinisherQTEPrompt`
   - `KeyText` 应为 `YogCommonRichTextBlock`，文本为 `<input action="HeavyAttack"/>`
   - 如果不是，运行：
     ```powershell
     & 'Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'X:\Project\Dev01\DevKit.uproject' -run=FinisherQTEWidgetSetup Apply ForceLayout -unattended -nop4 -nosplash -NullRHI -log
     ```
3. 本次剧情资产只使用这套：
   - 流程图：`/Game/Story/Encounters/Main_Tutorial_Demo/EG_FirstRun_Tutorial`
   - 剧情点目录：`/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/`
   - 不要使用旧的 `/Game/Story/Encounters/Tutorial/EG_FirstRun_Tutorial`，避免同名旧资产混淆。

## 1. 入口与 Campaign

### 1.1 主界面入口

已完成，只需要检查：

1. 打开 `Config/DefaultEngine.ini`
   - `GameDefaultMap=/Game/Maps/L_EntryMenu.L_EntryMenu`
   - `ServerDefaultMap=/Game/Maps/L_EntryMenu.L_EntryMenu`
2. 打开 `Config/DefaultGame.ini`
   - `FrontendMap=/Game/Maps/L_EntryMenu.L_EntryMenu`
   - `EntryMenuClass=/Game/UI/Frontend/WBP_SlotSelectWidget.WBP_SlotSelectWidget_C`
   - `MainGameMap=/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom.InitialRoom`
   - `FirstRunTutorialMap=/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom.InitialRoom`
3. 验收：
   - 新建存档后 Save 内有 `Story.Flag.FirstRunTutorial.Active`
   - 未完成首局教程且无 checkpoint 时，选档界面仍允许继续

### 1.2 教程 Campaign

打开：`/Game/Docs/Map/DA_Campaign_Tutorial`

注意：`DA_Campaign_Tutorial` 当前只有全局 `RoomPool`、`FloorTable`、`DefaultStartingRoom` 等字段，没有“每一层固定目标 RoomData”的表格字段。下面这张表是首局教程的目标路线，不是要求你在 Campaign DA 里逐行填写目标 RoomData。

建议把 Demo 首局教程目标路线整理成以下顺序：

| 顺序  | 目标 RoomData                                                  | 目的    | 检查点               |
| --- | ------------------------------------------------------------ | ----- | ----------------- |
| 0   | `/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom`           | 主城教程  | Hub、武器、木人桩、Portal |
| 1   | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a`          | 第一战斗房 | 无 Buff、金币奖励       |
| 2   | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b`          | 第二战斗房 | 敌人 Buff、三选一卡      |
| 3   | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon`          | 月光房   | 蓝雾敌人、月光掉落         |
| 4   | 任意普通房 A                                                      | 过渡房 1 | 金币/材料，不弹提示        |
| 5   | 任意普通房 B                                                      | 过渡房 2 | 金币/材料，结束后进祈祷室     |
| 6   | `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom` | 祈祷室   | 献祭、终结技、无限刷敌       |

当前代码已支持运行时 Campaign 选择：`B_GameMode.CampaignData` 指向 `/Game/Docs/Map/DA_Campaign_MainRun`，`B_GameMode.FirstRunTutorialCampaignData` 指向 `/Game/Docs/Map/DA_Campaign_Tutorial`。首局教程 Active 时自动使用 Tutorial，完成后回到 MainRun。`DA_Campaign_Tutorial` 只负责本教程可用的全局房间池和阶段参数；固定进入下一间房的逻辑要配置在当前 RoomData 的 `PortalDestinations` / `bForceSinglePortal` 上。

### 1.3 Campaign 路由检查

打开：`/Game/Code/Core/GameMode/B_GameMode`

必须保持：

- `CampaignData` = `/Game/Docs/Map/DA_Campaign_MainRun`
- `FirstRunTutorialCampaignData` = `/Game/Docs/Map/DA_Campaign_Tutorial`
- `StoryEventRegistry` = `/Game/Data/Story/DA_StoryEventRegistry_Tutorial`

不要把 `CampaignData` 改回 `DA_Campaign_Tutorial`。`CampaignData` 是玩家正常随机流程；`FirstRunTutorialCampaignData` 只在 `Story.Flag.FirstRunTutorial.Active` 有效时被 `AYogGameMode` 临时使用。

如果需要重新写入配置，运行：

```powershell
& 'Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'X:\Project\Dev01\DevKit.uproject' -ExecutePythonScript='X:\Project\Dev01\Docs\WorkSession\configure_campaign_routing.py' -unattended -nop4 -nosplash -log
```

验收：

- 新存档 / 首局教程 Active：日志应出现 `Source=FirstRunTutorialCampaignData`，使用 `DA_Campaign_Tutorial`
- 教程完成 / 普通流程：日志应出现 `Source=CampaignData`，使用 `DA_Campaign_MainRun`
- 后续剧情强控：剧情或蓝图调用 `SetCampaignOverride(DA_Campaign_Story_xxx)`，日志应出现 `Source=GameInstance.CampaignOverrideData`

### 1.4 武器卡组切换方案

当前采用“不同普通武器 DA + 剧情控制关卡对象出现”的方案。武器 DA 只使用原本的 `InitialCombatDeck`，不再给武器增加首局教程卡组覆盖字段。

运行时路径：
1. 新存档会写入 `Story.Flag.FirstRunTutorial.Active`。
2. 玩家进入 `hub_dash_hint` 区域时，剧情动作 `SetActorEnabled` 显示主城里正常摆放的 `BP_WeaponSpawner`：`WeaponSpawner_FirstRun_DemoSword`。
3. 这个 Spawner 指向首局演示武器 DA，例如 `DA_Weapon_FirstRun_DemoSword`；该武器自己的 `InitialCombatDeck` 填 `[攻击][攻击]`。
4. 玩家拾取武器时，仍走普通 `AWeaponSpawner` / `UWeaponDefinition::SetupWeaponToCharacter` 流程，读取武器自身 `InitialCombatDeck`。
5. 教程完成后会移除 `Story.Flag.FirstRunTutorial.Active` 并写入 `Story.Flag.FirstRunTutorial.Completed`。
6. 回主城后，剧情动作隐藏首局演示武器，并显示正常流程武器放置点 `WeaponSpawner_MainRun_StartWeapon`。
7. 后续进入正常流程时，`B_GameMode.CampaignData` 使用 `DA_Campaign_MainRun`；正常流程武器仍读取它自己的 `InitialCombatDeck`。

建议配置方式：
- 首局演示武器 DA：`InitialCombatDeck = [攻击][攻击]`。
- 正常流程起始武器 DA：`InitialCombatDeck = [攻击][攻击][月光][武器终结技]`，或使用正式设计需要的默认卡组。
- 关卡里两个武器都是普通 `BP_WeaponSpawner`，通过剧情 `SetActorEnabled` 控制显示/隐藏。
- 不要使用教程专用 WeaponSpawner，不要在 `WeaponDefinition` 里添加首局教程专用卡组列表。

## 2. 主城教程配置

地图：`/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom`

建议实际编辑 Gameplay 子关卡：
`/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom_GamePlay`

RoomData：
`/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom`

### 2.1 移动提示 Trigger

> 注意：场景里使用现有 `/Game/Docs/Map/LevelEvent/BP_LevelEventTrigger`。该 BP 的父类 `ALevelEventTrigger` 现在只保留 `EncounterGraph + NodeId` 这一种 Story Encounter 配置方式，不需要另找 `StoryEncounterTrigger` BP，也不需要再填写 `EncounterPoint`、旧的 `EncounterMap` 或直接挂载 `Level Flow`。

放置：
1. 打开 `InitialRoom_GamePlay`
2. 放置现有蓝图：`/Game/Docs/Map/LevelEvent/BP_LevelEventTrigger`
3. 命名：`Trigger_Tutorial_Move`
4. 放在出生点前方，玩家必经但不挡路的位置
5. 调整 `TriggerVolume` 范围，覆盖玩家离开出生点的路径

填写：
- `EncounterGraph` = `/Game/Story/Encounters/Main_Tutorial_Demo/EG_FirstRun_Tutorial`
- `NodeId` = `hub_move_hint`
- `bTriggerOnce` / `Trigger Once` = false

检查：
- PIE 新档进入主城，走过 Trigger 后出现底部提示条
- 文本必须使用输入富文本：
  - 键鼠：`<input action="Move"/> 移动角色。`
  - 手柄：`<input action="Move"/> 移动角色。<input action="CameraLook"/> 调整镜头。`
- 这是 `TutorialAreaHint`，不写入进度、不作为后续剧情条件。玩家进入框体 0.15s 淡入，停留时保持显示，离开框体 0.15s 淡出；重新进入会再次显示。

### 2.2 冲刺提示 Trigger

放置：
1. 复制 `Trigger_Tutorial_Move`
2. 命名：`Trigger_Tutorial_Dash`
3. 放在移动提示之后的第二段路径

填写：
- `EncounterGraph` = `EG_FirstRun_Tutorial`
- `NodeId` = `hub_dash_hint`
- `bTriggerOnce` / `Trigger Once` = false

检查：
- 玩家穿过后出现：`按下 <input action="Dash"/> 使用冲刺。`
- 不要写死 Space / 手柄键名
- 这是 `TutorialAreaHint`，不写入进度，也不依赖移动提示。玩家在框体内保持显示，离开 0.15s 淡出；重新进入会再次显示。

### 2.3 首局演示武器拾取点

放置：
1. 打开 `InitialRoom_GamePlay`
2. 放置 `Content/Code/Weapon/BP_WeaponSpawner`
3. 命名：`WeaponSpawner_FirstRun_DemoSword`
4. 推荐添加 Actor Tag：`Story.FirstRun.DemoWeapon`，也可以只依赖唯一 Actor Name
5. 放在武器中心视觉明显位置
6. 初始可以隐藏；玩家进入冲刺提示区域时由 `SetActorEnabled` 显示

填写：
- Weapon Definition 指向首局演示武器 DA，例如 `DA_Weapon_FirstRun_DemoSword`
- 该武器 DA 的普通 `InitialCombatDeck` 填：
  - `DA_Rune512_Attack`
  - `DA_Rune512_Attack`
- 拾取提示使用：`<input action="Interact"/>`

当前 C++ 行为：
- `BP_WeaponSpawner` 和 `WeaponDefinition` 不再有首局教程专用卡组逻辑。
- 拾取任何武器时都读取该武器 DA 自己的 `InitialCombatDeck`，为空时回退到 `InitialRunes`。
- 首局教程要换武器时，通过剧情动作 `SetActorEnabled` 控制普通 Spawner 的显示/隐藏。

剧情绑定：
- `hub_dash_hint` 的动作节点在玩家进入冲刺提示区域时显示 `WeaponSpawner_FirstRun_DemoSword`
- `weapon_pickup_prompt` 的 `placementName` = `WeaponSpawner_FirstRun_DemoSword`
- 首次武器拾取由 `AWeaponSpawner::TryPickupWeapon` 调用 `UTutorialManager::TryWeaponTutorial` 触发，不需要在首局演示武器实例上额外绑定 `Story Encounter|Pickup`，否则可能和旧状态机路径重复。
- `Story Encounter|Pickup` 保留给非 `NeedWeaponTutorial` 状态下的剧情拾取点：可填写 `PickupEncounterPoint`，或填写 `PickupEncounterGraph` + `PickupEncounterNodeId`。

检查：
- 靠近武器出现拾取提示
- 拾取后下方 1D 卡组只出现该武器 `InitialCombatDeck` 里的 `[攻击][攻击]`
- 教程完成后，首局演示武器被隐藏，正常流程武器可出现
- 触发 `first_run.weapon_picked`

### 2.4 1D 卡组进卡高亮

资产：
- `WBP_CombatDeckBar`
- C++ 已绑定 `CombatDeckComponent.OnDeckCardsEntered`
- C++ 已实现 `CombatDeckBarWidget.PlayDeckCardsEnteredHighlight`
- `BP_OnDeckCardsEntered` 只作为可选蓝图扩展点，不再要求蓝图里做基础动效

配置：
1. 打开 `WBP_CombatDeckBar`
2. 确认存在可选绑定控件：`DeckEntryHighlightPanel`
3. 基础动效由 C++ 驱动：
   - 下方卡组外框亮起
   - `EntryHighlightFadeInDuration` 默认 `0.1`
   - `EntryHighlightHoldDuration` 默认 `0.2`
   - `EntryHighlightFadeOutDuration` 默认 `0.15`
   - `EntryHighlightPeakScale` 默认 `1.035`
   - `EntryHighlightPeakOpacity` 默认 `1.0`
4. 这些参数在 `WBP_CombatDeckBar` 的 Class Defaults 里可调
5. 每次有卡进入 1D 卡组都播放

剧情绑定：
- 事件源：`WBP_CombatDeckBar.OnDeckCardsEntered`
- 剧情点：`EP_FirstRun_DeckEnterHighlight`
- `NodeId` = `deck_enter_highlight`

检查：
- 拾取武器后 `[攻击][攻击]` 入组，高亮一次
- 后续获得 `[重击]`、`[月光]`、终结技卡也高亮
- 底部提示使用：`<input action="OpenBackpack"/> 打开背包查看卡牌效果`

### 2.5 背包首次打开教程

触发：
- 玩家第一次打开背包时触发，不需要在地图里放实体 Trigger

需要实现或检查：
1. 背包 UI 打开入口处判断：
   - 首局教程 Active
   - `first_run.backpack_rules_seen` 未完成
2. 调用 `EP_FirstRun_BackpackCardRules`
3. `NodeId` = `backpack_card_rules`

弹窗页建议：
- 第 1 页：武器连招
- 第 2 页：卡牌会改变攻击结果
- 第 3 页：可以拖拽或按操作提示调整卡牌顺序

按键文本：
- 鼠标拖拽：`<input action="MouseClick"/>`
- 反转卡牌：`<input action="ReverseCard"/>`
- 不提前讲连携卡，只讲基础卡牌规则和排序

检查：
- 第一次打开背包弹出教程
- 第二次打开不重复弹
- 背包 UI 内的连招标正常显示

### 2.6 木人桩

放置：
1. 在武器中心放一个训练木人桩 Actor
2. 命名：`BP_TutorialTrainingDummy`
3. 生命值足够玩家练习 1-2 轮连招

剧情绑定：
- 玩家第一次攻击或进入木人桩区域时触发 `EP_FirstRun_TrainingDummyCombo`
- `NodeId` = `training_dummy_combo`

检查：
- 右上角显示武器连招列表
- 提示文本使用：
  - `<input action="LightAttack"/>`
  - `<input action="HeavyAttack"/>`
- 木人桩死亡后进入奖励掉落流程

### 2.7 木人桩掉落重击卡

目标奖励：
- 只出现一张卡，位于三选一 UI 的中间卡位
- 卡牌：`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback` 或正式 `[重击]` DA

注意：
- C++ 已给 `ARewardPickup` 增加固定奖励字段，不需要再做教程专用子类才能固定掉落。
- 放置或 Spawn `BP_RewardPickup` 后直接填写：
  - `bUseFixedLootOptions` = true
  - `FixedLootOptions` 只填一项 Rune：`[重击]`
- 如果木人桩死亡时动态 Spawn，也可以 Spawn 后继续调用 `AssignLoot`；但静态配置优先用 `FixedLootOptions`，更好检查。

放置/生成后命名：
- `RewardPickup_DummyHeavyCard`

剧情绑定：
- 拾取成功后触发 `EP_FirstRun_DummyDropHeavyCard`
- `NodeId` = `dummy_drop_heavy_card`

检查：
- 木人桩死亡后出现奖励拾取物
- 三选一 UI 只展示中间一张 `[重击]`
- 拾取后触发重击教程弹窗
- 触发 `first_run.heavy_card_obtained`

### 2.8 主城 Portal 与奖励预览

使用现有场景 Portal。

检查 Portal：
1. 找到主城入口 Portal Actor
2. 命名或备注：`Portal_TutorialStart`
3. 确认 `Index` 与 `DA_HubRoom_InitialRoom.PortalDestinations` 对应

RoomData 填写：
打开 `/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom`
- `PortalDestinations[0].PortalIndex` = 场景 Portal 的 `Index`
- `PortalDestinations[0].RoomPool` 只放：
  - `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a`
- `bForceSinglePortal` = true
- `ForcedPortalIndex` = 上面 Portal 的 `Index`

剧情绑定：
- 拾取 `[重击]` 后触发 `EP_FirstRun_HubPortalRewardPreview`
- `NodeId` = `hub_portal_reward_preview`

检查：
- 未拾取重击前 Portal 不高亮或不可用
- 拾取重击后 Portal 高亮
- Portal 预览显示金币奖励 icon
- 进入后必到 `DA_Room_CL_corridor_01a`

## 3. 第一战斗房：金币教学

RoomData：
`/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a`

配置：
1. `RoomName` = `L1_CommonLevel_Corridor_01a`
2. 敌人池放少量基础敌人
3. 不给本房敌人配置教程专用 Buff
4. `bUseFixedRewardOptions` = true
5. `FixedRewardOptions` 填一项：
   - `LootType` = `Gold`
   - `Amount` = 建议 50
   - `DisplayName` = `金币`
   - `Icon` = 项目已有金币 icon

剧情点：
- `EP_FirstRun_CombatRoom01Gold`
- `NodeId` = `combat_room_01_gold`
- 触发点：房间开始或清场后记录均可；推荐清场后记录

检查：
- 进房后少量敌人刷新
- 清场后出现金币拾取物
- 拾取后播放金币音效/VFX
- 右下角金币数字增长
- 写入 `first_run.combat_room_01_cleared`

## 4. 第二战斗房：Buff 与三选一卡

RoomData：
`/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b`

配置：
1. 敌人池放基础敌人
2. Buff Pool 至少有 1 个明确可见 Buff
3. `bUseFixedRewardOptions` = true
4. `FixedRewardOptions` 填三项 Rune：
   - `[攻击]`：`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack`
   - `[重击]`：正式重击 DA，若暂用击退卡则用 `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback`
   - `[分裂]`：`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split`
5. 奖励 UI 的卡牌 icon 使用卡牌自身 icon
6. 本房结束后强制打开通往月光房的 Portal：
   - `bForceSinglePortal` = true
   - `ForcedPortalIndex` = 需要打开的 Portal Index
   - `PortalDestinations` 的该 Index 只放 `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon`

剧情点：
- `EP_FirstRun_CombatRoom02ThreeCards`
- `NodeId` = `combat_room_02_three_cards`

检查：
- 进入房间时提示“敌人获得了关卡强化”
- 敌人确实带 Buff
- 清场后只出现 `[攻击][重击][分裂]`
- 下一扇门必定去 `DA_Room_CL_WaterDungeon`

## 5. 月光连携卡房

RoomData：
`/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon`

配置：
1. 选择一个特殊敌人，或刷怪逻辑里的最后一个敌人
2. 给该敌人加蓝色周身雾效
3. 这个敌人死亡后掉落 `[月光]`
   - 优先使用 `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward`
   - 如当前系统使用双手剑专属卡，也可用 `/Game/Code/Weapon/TwoHandedSword/CombatCards/DA_Rune512_THSword_Moonlight`
4. 掉落方式同重击卡：
   - 教程专用 Pickup，或死亡后 Spawn 并调用 `AssignLoot`

剧情点：
- `EP_FirstRun_MoonlightSpecialEnemy`
- `NodeId` = `moonlight_special_enemy`
- `placementName` = `Enemy_Tutorial_MoonlightCarrier`

弹窗检查：
- 第 1 页：介绍连携卡是特殊卡，拥有基础结算
- 第 2 页：介绍正向/反向连携，隐晦提示可反转
- 按键使用 `<input action="ReverseCard"/>`

检查：
- 特殊敌人蓝雾可见
- 击杀后掉落 `[月光]`
- 拾取后弹出连携卡教程
- 写入 `first_run.moonlight_obtained`

## 6. 两个过渡房

目标：
- 玩家继续打 2 个房间
- 只掉金币和材料
- 不弹底部提示条
- 第二个过渡房结束后必定进入祈祷室

配置：
1. 选择两个普通 RoomData，或复制现有 Corridor RoomData 作为教程过渡房
2. 两个房间都设置：
   - `bUseFixedRewardOptions` = true
   - `FixedRewardOptions` = Gold + Material
   - 材料 `Icon` 暂用问号 icon
3. 第 1 个过渡房结束写入：
   - `EP_FirstRun_TransitionRoom01GoldMaterial`
   - `first_run.transition_room_01_cleared`
4. 第 2 个过渡房结束写入：
   - `EP_FirstRun_TransitionRoom02GoldMaterial`
   - `first_run.transition_room_02_cleared`
   - `first_run.transition_rooms_finished`
5. 第 2 个过渡房的 Portal 强制指向：
   - `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom`

检查：
- 两个过渡房没有弱提示/底部提示条
- 奖励只有金币/材料
- 第二个过渡房结束后下一关必进祈祷室

## 7. 祈祷室献祭与终结技

地图：
`/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom`

RoomData：
`/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom`

已有配置参考：
`Docs/03_配置与编辑器手册/核心配置说明/关卡/PrayRoom献祭事件关配置说明.md`

### 7.1 祭坛

检查：
1. 打开 `DA_PrayRoom`
2. 确认：
   - `RoomName` = `PrayRoom`
   - `Room Tags` 包含 `Room.Type.Event`
   - 祭坛相关 DA 指向 `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_Altar_PrayRoom_SacrificeEvent`
3. 打开祈祷室地图
4. 放置或确认 `BP_Altar_PrayerSacrifice`
5. 命名或备注：`Altar_Tutorial_Finisher`

剧情点：
- 献祭完成并获得终结技后触发 `EP_FirstRun_PrayerSacrificeFinisher`
- `NodeId` = `prayer_sacrifice_finisher`

检查：
- 玩家能献祭一张卡
- 获得武器终结技卡牌
- 弹出终结技通用教程
- 写入 `first_run.finisher_obtained`

### 7.2 终结技后无限刷敌

放置/配置：
1. 在祈祷室放置无限刷敌 Spawner
2. 命名：`Spawner_Tutorial_Endless`
3. 默认禁用
4. 获得终结技后延迟约 1 秒启用
5. 敌人总量无上限，难度保证玩家最终死亡
6. 禁止普通通关出口

剧情点：
- `EP_FirstRun_ScriptedDefeatReturnHub`
- `NodeId` = `scripted_defeat_return_hub`

死亡 UI：
- 玩家死亡后只显示“回归主城”
- 不显示普通复活/继续选项

检查：
- 获得终结技后开始无限刷敌
- 死亡后出现回主城选项
- 写入 `first_run.scripted_defeat_seen`

## 8. 回主城与教程完成

触发：
- 死亡 UI 选择“回归主城”后触发系统事件
- `placementName` = `ReturnToHub_AfterFirstRunTutorial`

剧情点：
- `EP_FirstRun_ReturnHubNormalRunStart`
- `NodeId` = `return_hub_normal_run_start`

保存/关卡对象切换：
1. 移除 `Story.Flag.FirstRunTutorial.Active`
2. 写入 `Story.Flag.FirstRunTutorial.Completed`
3. 隐藏 `WeaponSpawner_FirstRun_DemoSword`
4. 显示 `WeaponSpawner_MainRun_StartWeapon`
5. `WeaponSpawner_MainRun_StartWeapon` 指向正常流程武器 DA；该武器自己的 `InitialCombatDeck` 决定后续默认卡组
6. 清理首局教程临时房间状态
7. 回到 `InitialRoom`

检查：
- 再次从主菜单进入同一存档，不再走首局教程入口
- 回主城后，正常流程武器出现；拾取后卡组来自该武器自身 `InitialCombatDeck`
- 可以进入正常 Run

## 9. 最终全流程验收

1. 删除或新建一个空存档槽。
2. 主菜单选择新存档。
3. 确认进入 `InitialRoom`，且 Save 有 `Story.Flag.FirstRunTutorial.Active`。
4. 依次验证：
   - 移动提示
   - 冲刺提示
   - 拾取武器
   - 1D 卡组高亮
   - 打开背包教程
   - 木人桩练习
   - 木人桩掉落 `[重击]`
   - Portal 奖励预览
   - 第一战斗房金币
   - 第二战斗房 Buff 与三选一卡
   - 月光特殊敌人
   - 两个过渡房
   - 祈祷室献祭
   - 终结技教程
   - 无限刷敌死亡
   - 回主城
5. 回主城后检查：
   - `Story.Flag.FirstRunTutorial.Active` 不存在或为 false
   - `Story.Flag.FirstRunTutorial.Completed` 为 true
   - 首局演示武器已隐藏，正常流程武器出现；拾取后卡组来自正常武器自身 `InitialCombatDeck`
6. 回主菜单再进入该存档，确认不会重走首局教程。

## 10. 当前需要开发补齐的点

### 10.1 本轮 C++ 已补齐

1. 武器卡组方案已回到普通路径：
   - `UWeaponDefinition` 不再持有首局教程专用卡组列表。
   - `AWeaponSpawner` 和 `UWeaponDefinition::SetupWeaponToCharacter` 只读取武器自身 `InitialCombatDeck` / `InitialRunes`。
2. 剧情控制关卡对象：
   - Story Encounter 新增动作 `SetActorEnabled`。
   - 可通过 `TargetActorName` 或 `TargetActorTag` 显示/隐藏普通关卡 Actor。
   - 剧情编辑器与 UE 导入管线已支持该动作，并会把目标 Actor 缺失列入工作量清单。
3. 固定奖励拾取物：
   - `ARewardPickup` 增加 `bUseFixedLootOptions` 和 `FixedLootOptions`。
   - 静态放置或动态 Spawn 的奖励都可以直接指定固定 Loot。
4. 切关恢复卡组：
   - `APlayerCharacterBase::RestoreRunStateFromGI` 现在使用精确恢复，不会在教程跨房间时额外塞入临时终结技卡。

### 10.2 仍需配置或蓝图补齐

1. 创建或打开首局演示武器 DA，例如 `DA_Weapon_FirstRun_DemoSword`：
   - 普通 `InitialCombatDeck = [攻击][攻击]`
   - 不填写任何首局教程专用卡组覆盖字段
2. 在 `InitialRoom_GamePlay` 放置普通 `BP_WeaponSpawner`：
   - Actor Name = `WeaponSpawner_FirstRun_DemoSword`
   - Actor Tag = `Story.FirstRun.DemoWeapon`（推荐，用于 `SetActorEnabled` 批量或稳定查找；如果只控制单个对象，也可以用 Actor Name）
   - Weapon Definition = `DA_Weapon_FirstRun_DemoSword`
3. 放置正常流程起始武器 Spawner：
   - Actor Name = `WeaponSpawner_MainRun_StartWeapon`
   - Actor Tag = `Story.MainRun.StartWeapon`（推荐；非强制）
   - Weapon Definition 指向正常流程武器 DA
   - 正常流程武器 DA 的 `InitialCombatDeck` 由正式设计决定，例如 `[攻击][攻击][月光][武器终结技]`
4. 首次武器拾取弹窗由 `TryWeaponTutorial` 触发；不要在 `WeaponSpawner_FirstRun_DemoSword` 上重复填写 `Story Encounter|Pickup`。非首次剧情拾取点可使用该通用钩子。
5. 背包首次打开触发 `EP_FirstRun_BackpackCardRules` 仍需检查 UI 入口是否已绑定。
6. 木人桩死亡事件仍需生成或启用固定 `[重击]` 奖励拾取物。
7. 月光特殊敌人仍需在目标 RoomData / 刷怪逻辑中配置特殊敌人 BP、蓝雾 FX 和 `[月光]` 固定掉落。
8. Portal 预览奖励 icon 的金币/卡牌/材料表现仍需逐项检查。
9. 两个过渡房结束后强制进入祈祷室仍需在 RoomData `PortalDestinations` / `bForceSinglePortal` 中配置。
10. 祈祷室获得终结技后启动无限刷敌仍需关卡 BP 或 Room Event 绑定。
11. 死亡界面“只显示回归主城”仍需按首局教程 Active/Completed 状态做 UI 分支验收。
12. 全流程 PIE 验收仍未完成：新存档从主菜单进入、首局教程、死亡回主城、再进入正常 MainRun。
