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

建议把 Demo 首局教程整理成以下顺序：

| 顺序 | 目标 RoomData | 目的 | 检查点 |
| --- | --- | --- | --- |
| 0 | `/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom` | 主城教程 | Hub、武器、木人桩、Portal |
| 1 | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a` | 第一战斗房 | 无 Buff、金币奖励 |
| 2 | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b` | 第二战斗房 | 敌人 Buff、三选一卡 |
| 3 | `/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon` | 月光房 | 蓝雾敌人、月光掉落 |
| 4 | 任意普通房 A | 过渡房 1 | 金币/材料，不弹提示 |
| 5 | 任意普通房 B | 过渡房 2 | 金币/材料，结束后进祈祷室 |
| 6 | `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom` | 祈祷室 | 献祭、终结技、无限刷敌 |

当前代码已支持运行时 Campaign 选择：`B_GameMode.CampaignData` 指向 `/Game/Docs/Map/DA_Campaign_MainRun`，`B_GameMode.FirstRunTutorialCampaignData` 指向 `/Game/Docs/Map/DA_Campaign_Tutorial`。首局教程 Active 时自动使用 Tutorial，完成后回到 MainRun。`DA_Campaign_Tutorial` 仍需要检查是否继续复用旧阶段 Tag，或改成上面这条 Demo 首局链路；房间顺序优先用 RoomData 的 `PortalDestinations` 和 `bForceSinglePortal` 串起来。

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

## 2. 主城教程配置

地图：`/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom`

建议实际编辑 Gameplay 子关卡：
`/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom_GamePlay`

RoomData：
`/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom`

### 2.1 移动提示 Trigger

放置：
1. 打开 `InitialRoom_GamePlay`
2. 放置 C++ Actor：`StoryEncounterTrigger`
3. 命名：`Trigger_Tutorial_Move`
4. 放在出生点前方，玩家必经但不挡路的位置
5. 调整 `TriggerVolume` 范围，覆盖玩家离开出生点的路径

填写：
- `EncounterPoint` = `/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubMoveHint`
- `EncounterMap` = `/Game/Story/Encounters/Main_Tutorial_Demo/EG_FirstRun_Tutorial`
- `NodeId` = `hub_move_hint`

检查：
- PIE 新档进入主城，走过 Trigger 后出现底部提示条
- 文本必须使用输入富文本：
  - 键鼠：`<input action="Move"/> 移动角色。`
  - 手柄：`<input action="Move"/> 移动角色。<input action="CameraLook"/> 调整镜头。`
- 触发后保存进度：`Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.hub_move_hint`

### 2.2 冲刺提示 Trigger

放置：
1. 复制 `Trigger_Tutorial_Move`
2. 命名：`Trigger_Tutorial_Dash`
3. 放在移动提示之后的第二段路径

填写：
- `EncounterPoint` = `EP_FirstRun_HubDashHint`
- `EncounterMap` = `EG_FirstRun_Tutorial`
- `NodeId` = `hub_dash_hint`

检查：
- 玩家穿过后出现：`按下 <input action="Dash"/> 使用冲刺。`
- 不要写死 Space / 手柄键名
- 触发一次后不重复弹

### 2.3 教程武器拾取点

放置：
1. 打开 `InitialRoom_GamePlay`
2. 放置 `Content/Code/Weapon/BP_WeaponSpawner`
3. 命名：`WeaponSpawner_Tutorial`
4. 放在武器中心视觉明显位置

填写：
- 武器 Definition 指向教程双手剑或当前 Demo 默认武器
- 武器自身 `InitialCombatDeck` 填：
  - `DA_Rune512_Attack`
  - `DA_Rune512_Attack`
- 拾取提示使用：`<input action="Interact"/>`

剧情绑定：
- 需要在武器拾取成功事件后触发 `EP_FirstRun_WeaponPickupPrompt`
- 若当前 `BP_WeaponSpawner` 没有 Story Encounter 触发字段，需要加一个蓝图事件或 Level Blueprint 调用 `StoryEncounterRuntimeSubsystem.TriggerEncounterPoint`

检查：
- 靠近武器出现拾取提示
- 拾取后获得武器
- 下方 1D 卡组出现 `[攻击][攻击]`
- 触发 `first_run.weapon_picked`

### 2.4 1D 卡组进卡高亮

资产：
- `WBP_CombatDeckBar`
- C++ 已有 `CombatDeckComponent.OnDeckCardsEntered`
- C++ 已有 `CombatDeckBarWidget.BP_OnDeckCardsEntered`

配置：
1. 打开 `WBP_CombatDeckBar`
2. 实现 `BP_OnDeckCardsEntered`
3. 做一个短动画：
   - 下方卡组外框亮起
   - 0.15s 渐入
   - 0.6s 保持
   - 0.25s 渐出
4. 每次有卡进入 1D 卡组都播放

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
当前 `ARewardPickup` 的 `AssignedLoot` 是运行时由 `AssignLoot` 写入，手动放一个 `BP_RewardPickup` 不会自动带固定卡牌。因此这里需要二选一：

方案 A：做一个教程专用 BP。
- 新建 `BP_TutorialRewardPickup_HeavyCard` 继承 `BP_RewardPickup`
- 暴露 `FixedLootOptions`
- BeginPlay 调用 `AssignLoot(FixedLootOptions)`
- `FixedLootOptions` 只填一项 Rune：`[重击]`

方案 B：木人桩死亡时由 Level Blueprint / 木人桩 BP Spawn `BP_RewardPickup`。
- Spawn 后立即调用 `AssignLoot`
- 传入只含 `[重击]` 的 `FLootOption`

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

保存/卡组写入：
1. 移除 `Story.Flag.FirstRunTutorial.Active`
2. 写入 `Story.Flag.FirstRunTutorial.Completed`
3. 设置新默认卡组：
   - `[攻击]`
   - `[攻击]`
   - `[月光]`
   - `[武器终结技]`
4. 清理首局教程临时房间状态
5. 回到 `InitialRoom`

检查：
- 再次从主菜单进入同一存档，不再走首局教程入口
- 回主城后武器卡组为 `[攻击][攻击][月光][武器终结技]`
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
   - 初始卡组正确
6. 回主菜单再进入该存档，确认不会重走首局教程。

## 10. 当前需要开发补齐的点

以下不是单纯填 DA 就能完成，需要补蓝图或 C++：

1. `BP_WeaponSpawner` 拾取成功后触发 Story Encounter。
2. 背包首次打开触发 `EP_FirstRun_BackpackCardRules`。
3. 教程专用 RewardPickup：支持手动/脚本指定固定 Loot，并能只显示中间一张卡。
4. 木人桩死亡事件生成固定 `[重击]` 奖励。
5. 月光敌人死亡事件生成固定 `[月光]` 奖励。
6. Portal 预览奖励 icon 的金币/卡牌/材料表现。
7. 两个过渡房结束后强制进入祈祷室。
8. 祈祷室获得终结技后启动无限刷敌。
9. 死亡界面“只显示回归主城”。
10. 回主城后写入 Completed 并重置默认卡组。
