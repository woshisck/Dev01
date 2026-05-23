# Story Pipeline Workload

Generated: 2026-05-23T14:41:12.651Z

## first_run_tutorial

- Arc: Main_Tutorial_Demo
- Chapter: first_run
- Timeline: present
- Graph: EG_FirstRun_Tutorial

| Work | Detail | Owner | Status |
| --- | --- | --- | --- |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Trigger_Tutorial_Move -> EP_FirstRun_HubMoveHint | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Trigger_Tutorial_Dash -> EP_FirstRun_HubDashHint | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / WeaponSpawner_Tutorial -> EP_FirstRun_WeaponPickupPrompt | Level | todo |
| SystemEvent | Any / WBP_CombatDeckBar.OnDeckCardsEntered -> EP_FirstRun_DeckEnterHighlight | Tech Design | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Backpack_FirstOpen -> EP_FirstRun_BackpackCardRules | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / BP_TutorialTrainingDummy -> EP_FirstRun_TrainingDummyCombo | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / RewardPickup_DummyHeavyCard -> EP_FirstRun_DummyDropHeavyCard | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom / Portal_TutorialStart -> EP_FirstRun_HubPortalRewardPreview | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a / RoomStart_FirstCombatGold -> EP_FirstRun_CombatRoom01Gold | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b / RoomStart_BuffedThreeCards -> EP_FirstRun_CombatRoom02ThreeCards | Level | todo |
| Trigger | /Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon / Enemy_Tutorial_MoonlightCarrier -> EP_FirstRun_MoonlightSpecialEnemy | Level | todo |
| SystemEvent | Any / RoomPool_Tutorial_Transition_01_Clear -> EP_FirstRun_TransitionRoom01GoldMaterial | Tech Design | todo |
| SystemEvent | Any / RoomPool_Tutorial_Transition_02_Clear -> EP_FirstRun_TransitionRoom02GoldMaterial | Tech Design | todo |
| Trigger | /Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom / Altar_Tutorial_Finisher -> EP_FirstRun_PrayerSacrificeFinisher | Level | todo |
| Trigger | /Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom / Spawner_Tutorial_Endless -> EP_FirstRun_ScriptedDefeatReturnHub | Level | todo |
| Dialogue | EP_FirstRun_ScriptedDefeatReturnHub: 遗圣目录 | Narrative/UI | todo |
| SystemEvent | System / ReturnToHub_AfterFirstRunTutorial -> EP_FirstRun_ReturnHubNormalRunStart | Tech Design | todo |
| System | 新存档入口写入 FirstRunTutorial.Active，并加载教程 Campaign | Tech Design | todo |
| LevelDesign | 主城摆放移动、冲刺、武器拾取、木人桩、Portal 引导相关 Trigger / Actor | Level Design | todo |
| UI/Narrative | 配置教程弹窗内容：武器、背包卡牌规则、重击、月光、终结技、教程完成；正文/副正文统一使用 YogCommonRichTextBlock + InputActionRichTextDecorator | UI/Narrative | todo |
| UI | 移动教学底部操作提示条正文使用输入设备分流：键鼠正文渲染 <input action="Move"/> + WASD 说明；手柄正文渲染 <input action="Move"/> 和 <input action="CameraLook"/> | UI | todo |
| UI | CombatDeckBar 增加 OnDeckCardsEntered 渐入渐出高亮，首次入组追加 <input action="OpenBackpack"/> 打开背包底部操作提示条 | UI | partial |
| Gameplay | RewardPickup 支持 Card / Gold / Material；金币音效/VFX；材料问号 icon；卡牌紫色奖励表现 | Gameplay/UI | todo |
| Gameplay | Portal 支持教程强制开启指定门，并显示奖励倾向 icon | Gameplay | todo |
| DataAsset | 配置 Demo 教程房间序列：Hub -> 无 Buff 金币房 -> Buff 三选一卡房 -> 月光房 -> 固定 2 个过渡房 -> 祈祷室 | Level Design | todo |
| VFX/Gameplay | 指定敌人附加蓝色周身雾效，死亡掉落 [月光] 连携卡 | VFX/Gameplay | todo |
| Gameplay | 祈祷室祭坛支持献祭一张卡并授予 [武器终结技] | Gameplay | todo |
| LevelFlow | 终结技教程后启动无限刷敌、锁定失败流程，死亡后只显示“回归主城”选项 | Tech Design | todo |
| Save/Deck | 教程完成回主城后设置默认卡组 [攻击][攻击][月光][武器终结技]，并写入 Completed | Gameplay | todo |
| UI | 所有教程类按键提示统一使用 YogCommonRichTextBlock / InputActionRichTextDecorator：Move、CameraLook、Dash、Interact、OpenBackpack、MouseClick、LightAttack、HeavyAttack、ReverseCard，不写死 E/Space/Tab/H/R 或手柄键名 | UI | done |
| UI | WBP_FinisherQTEPrompt 的输入提示改为 YogCommonRichTextBlock / InputActionRichTextDecorator，终结技确认显示 <input action="HeavyAttack"/>，不要写死 H | UI | done |
| Input/UI | 新增或确认反转卡牌输入 token：建议 <input action="ReverseCard"/>，用于后续月光/连携卡教程；不要在正文里写死 R 或手柄 X | Input/UI | done |
