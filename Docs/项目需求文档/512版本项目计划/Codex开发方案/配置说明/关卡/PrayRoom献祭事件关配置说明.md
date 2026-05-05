# PrayRoom 献祭事件关配置说明

## 关卡目标

`PrayRoom` 暂定为战斗事件关。玩家进入后先完成普通战斗，并需要在限时内清空房间；当前配置为 90 秒。若超时后才清空房间，本次事件不激活献祭圣坛，只走普通关卡结算。

## 运行流程

1. 进入 `PrayRoom` 后按常规房间刷怪并进入战斗阶段。
2. 玩家在 `TimedClearSeconds` 内清空所有敌人后，关卡进入整理阶段。
3. 常规奖励拾取物照常掉落。
4. 献祭圣坛在奖励点附近生成，并切到可交互状态。
5. 玩家交互圣坛后打开 `WBP_SacrificeSelection`，选择一个献祭代价。
6. 支付代价成功后，直接获得献祭符文被动。该符文作为全局隐藏被动立即启动，不进入背包拖拽卡牌列表，也不加入战斗卡组。
7. 献祭成功后，圣坛关闭交互并标记为已领取，不能重复获得同一次事件奖励。

## 资产路径

| 类型 | 路径 |
| --- | --- |
| 房间 DA | `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom` |
| 房间地图 | `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom` |
| 献祭圣坛 DA | `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_Altar_PrayRoom_SacrificeEvent` |
| 献祭选择 WBP | `/Game/UI/Playtest_UI/Event/WBP_SacrificeSelection` |
| 当前接入的献祭符文 | `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/DA_Rune512_Sacrifice_MoonlightShadow` |

## DA_PrayRoom 配置

| 字段 | 当前值 |
| --- | --- |
| `RoomName` | `PrayRoom` |
| `DisplayName` | `祈祷室` |
| `RoomTags` | `Room.Type.Event`、`Room.Layer.L1` |
| `bEnableTimedClearObjective` | `true` |
| `TimedClearSeconds` | `90` |
| `SacrificeEventAltarData` | `DA_Altar_PrayRoom_SacrificeEvent` |
| `SacrificeEventWidgetClass` | `WBP_SacrificeSelection` |
| `SacrificeEventAltarSpawnOffset` | `(250, 0, 0)` |

`PrayRoom` 也加入了 C++ 地图名解析：当 RoomName 写 `PrayRoom` 或资源名为 `DA_PrayRoom` 时，会打开 `/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom`。

## 献祭三选一代价

`DA_Altar_PrayRoom_SacrificeEvent` 内当前有 3 个固定选项，三个选项授予同一张事件献祭符文，只改变玩家支付的代价。

| 代价类型 | 数值 |
| --- | --- |
| 献祭卡组中的一张卡牌 | 玩家从当前卡组列表中选择 1 张移除 |
| 攻击力增加，受到伤害增加 | 攻击力 +15%，受到伤害 +20% |
| 暴击概率降低，暴击伤害增加 | 暴击概率 -50%，暴击伤害 +50% |

属性代价会作为本轮全局状态保存到 `RunState.SacrificeOfferingCosts`，跨房间恢复。献祭卡牌代价直接修改 `CombatDeckComponent.DeckList`。

## WBP 绑定

`WBP_SacrificeSelection` 的父类是 `USacrificeSelectionWidget`。当前 Designer Tree 已包含以下可绑定控件：

| 控件名 | 类型 | 用途 |
| --- | --- | --- |
| `TitleText` | TextBlock | 标题 |
| `DescriptionText` | TextBlock | 当前献祭符文名 |
| `CostText` | TextBlock | 已选代价或失败原因 |
| `OptionBox` | VerticalBox | 三个代价按钮容器 |
| `OptionButton0/1/2` | Button | 代价三选一 |
| `DeckCardBox` | VerticalBox | 卡牌献祭选择容器 |
| `DeckButton0..7` | Button | 当前卡组前 8 张可献祭卡牌 |
| `BtnConfirm` | Button | 确认支付代价并获得符文 |
| `BtnCancel` | Button | 返回或关闭界面 |

如果该 WBP 不可用，`USacrificeSelectionWidget` 会用 C++ fallback 布局生成同名控件，保证事件流程仍可运行。

## 生成命令

后续需要重刷资产时可运行：

```powershell
& 'Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'X:\Project\Dev01\DevKit.uproject' -run=PrayRoomSacrificeEventSetup Apply ForceLayout -unattended -nop4 -nosplash -nullrhi
```

报告输出：

`Saved/PrayRoomSacrificeEventSetupReport.md`
