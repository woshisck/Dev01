# first_run_tutorial

```json
{
  "schemaVersion": 1,
  "storyId": "first_run_tutorial",
  "arc": "Main_Tutorial_Demo",
  "chapter": "first_run",
  "timeline": "present",
  "encounterId": "EM_FirstRun_Tutorial",
  "graphAsset": "EG_FirstRun_Tutorial",
  "maps": [
    "/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom",
    "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a",
    "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b",
    "/Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon",
    "/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom"
  ]
}
```

## 剧情点

| Asset | NodeId | 名称 | 类型 | 放置 |
| --- | --- | --- | --- | --- |
| `EP_FirstRun_HubMoveHint` | `hub_move_hint` | 主城移动底部操作提示条 | Area | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Trigger_Tutorial_Move |
| `EP_FirstRun_HubDashHint` | `hub_dash_hint` | 主城冲刺底部操作提示条 | Area | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Trigger_Tutorial_Dash |
| `EP_FirstRun_DeckEnterHighlight` | `deck_enter_highlight` | 1D 卡组进卡高亮 | Feature | Any / WBP_CombatDeckBar.OnDeckCardsEntered |
| `EP_FirstRun_TrainingDummyCombo` | `training_dummy_combo` | 木人桩攻击与连招练习 | Object | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / B_EnemyDummy_Tutorial |
| `EP_FirstRun_HubPortalRewardPreview` | `hub_portal_reward_preview` | 主城教程结束与传送门奖励预览 | Feature | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Portal_TutorialStart |
| `EP_FirstRun_CombatRoom01Gold` | `combat_room_01_gold` | 第一战斗房金币教学 | Area | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a / RoomStart_FirstCombatGold |
| `EP_FirstRun_CombatRoom02ThreeCards` | `combat_room_02_three_cards` | 第二战斗房敌人 Buff 与三选一卡 | Area | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b / RoomStart_BuffedThreeCards |
| `EP_FirstRun_MoonlightSpecialEnemy` | `moonlight_special_enemy` | 蓝雾特殊敌人与月光连携卡 | Area | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon / Enemy_Tutorial_MoonlightCarrier |
| `EP_FirstRun_TransitionRoom01GoldMaterial` | `transition_room_01_gold_material` | 第一过渡房记录：金币与材料 | Area | Any / RoomPool_Tutorial_Transition_01_Clear |
| `EP_FirstRun_TransitionRoom02GoldMaterial` | `transition_room_02_gold_material` | 第二过渡房记录：通往祈祷室 | Area | Any / RoomPool_Tutorial_Transition_02_Clear |
| `EP_FirstRun_ReturnHubNormalRunStart` | `return_hub_normal_run_start` | 回主城并进入后续体验 | System | System / ReturnToHub_AfterFirstRunTutorial |

## 状态与关键道具

- States: {"key":"Story.Flag.FirstRunTutorial.Active","initialValue":true,"scope":"Save","description":"新存档进入 Demo 首局教程时写入。"}, {"key":"Story.Flag.FirstRunTutorial.Completed","initialValue":false,"scope":"Save","description":"玩家死亡回主城并完成 Demo 首局教程后写入。"}, {"key":"Story.Flag.FirstRunTutorial.DemoOnly","initialValue":true,"scope":"Build","description":"Main_Tutorial_Demo 分组仅用于 Demo 阶段制作和验证。"}

## 工作量

- [ ] System: 新存档入口写入 FirstRunTutorial.Active，并加载教程 Campaign
- [ ] LevelDesign: 主城摆放移动、冲刺、武器拾取、木人桩、Portal 引导相关 Trigger / Actor；武器拾取教程由 `TryWeaponTutorial` 自动触发，不接 Story Pickup EP
- [ ] UI: 移动教学底部操作提示条正文使用输入设备分流：键鼠正文渲染 <input action="Move"/> + WASD 说明；手柄正文渲染 <input action="Move"/> 和 <input action="CameraLook"/>
- [ ] UI: CombatDeckBar 增加 OnDeckCardsEntered 渐入渐出高亮，首次入组追加 <input action="OpenBackpack"/> 打开背包底部操作提示条
- [ ] Gameplay: RewardPickup 支持 Card / Gold / Material；金币音效/VFX；材料问号 icon；卡牌紫色奖励表现
- [ ] Gameplay: Portal 支持教程强制开启指定门，并显示奖励倾向 icon
- [ ] DataAsset: 配置 Demo 教程房间序列：Hub -> 无 Buff 金币房 -> Buff 三选一卡房 -> 月光房 -> 固定 2 个过渡房 -> 祈祷室
- [ ] VFX/Gameplay: 指定敌人附加蓝色周身雾效，死亡掉落 [月光] 连携卡
- [ ] LevelActor/DataAsset: 教程完成回主城后隐藏首局演示武器，显示正常流程武器；正常武器卡组由自身 InitialCombatDeck 决定
- [ ] UI: 所有教程类按键提示统一使用 YogCommonRichTextBlock / InputActionRichTextDecorator：Move、CameraLook、Dash、Interact、OpenBackpack、MouseClick、LightAttack、HeavyAttack、ReverseCard，不写死 E/Space/Tab/H/R 或手柄键名
- [ ] Input/UI: 新增或确认反转卡牌输入 token：建议 <input action="ReverseCard"/>，用于后续月光/连携卡教程；不要在正文里写死 R 或手柄 X
