# 主 HUD 与暂停菜单配置说明

## 资产入口

- 主 HUD：`/Game/UI/Playtest_UI/HUD/WBP_HUDRoot`
- 玩家通用信息 HUD：`/Game/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud`
- 暂停菜单：`/Game/UI/Playtest_UI/Pause/WBP_PauseMenu`
- 暂停菜单纹理源文件：`SourceArt/UI/Pause/`
- 暂停菜单导入纹理：`/Game/UI/Playtest_UI/UI_Tex/Pause/`
- 生成/刷新命令let：`MainUISetupCommandlet`

## WBP_HUDRoot 命名约定

`WBP_HUDRoot` 的父类必须是 `YogHUDRootWidget`。以下控件需要保持 `Is Variable` 并使用固定名称：

- 区域容器：`TopLeftPlayerInfoRegion`、`TopRightPlayerInfoRegion`、`BossInfoRegion`、`LeftLevelInfoRegion`、`RightLevelInfoRegion`、`BottomLeftPlayerInfoRegion`、`BottomCenterCombatRegion`、`BottomRightPlayerInfoRegion`
- HUD 子控件：`PlayerHealthBar`、`EnemyArrow`、`WeaponGlassIcon`、`HeatBar`、`InfoPopup`、`CombatDeckBar`、`CurrentRoomBuffPanel`、`PlayerCommonInfoHud`

布局约定：

- `PlayerHealthBar` 放在底部左侧玩家信息区。
- `CombatDeckBar` 放在底部中央战斗信息区。
- `CurrentRoomBuffPanel` 放在左侧关卡信息区，数据来自 `YogGameMode.ActiveRoomBuffs`。
- 关卡 Buff 摘要优先读取 `RuneInfo -> RuneConfig -> HUDSummaryText`；为空时从完整 `RuneDescription` 自动压缩，避免左侧关卡信息区文本过长。
- `PlayerCommonInfoHud` 放在底部右侧玩家信息区的上沿，当前显示金币，后续用于关键道具、非战斗资源等通用数量信息。
- `UCombatItemBarWidget` 运行时放在底部右侧玩家信息区的下沿，与 `PlayerCommonInfoHud` 分层管理，避免资源信息和战斗道具混在一起。
- `BossInfoRegion` 默认隐藏，后续 Boss 血条或阶段信息放入该区域。
- 顶部左右玩家信息区继续预留给未来道具、技能或状态 UI。

## WBP_PlayerCommonInfoHud 命名约定

`WBP_PlayerCommonInfoHud` 的父类必须是 `PlayerCommonInfoWidget`。以下控件需要保持 `Is Variable`：

- `CommonInfoList`
- `GoldRow`
- `GoldIcon`
- `GoldText`

金币图标固定 24x24；金币数量只显示数字。未来关键道具等条目通过 `SetCommonInfoEntry` 添加到 `CommonInfoList`。

## WBP_PauseMenu 命名约定

`WBP_PauseMenu` 的父类必须是 `PauseMenuWidget`。以下控件需要保持 `Is Variable`：

- `BtnControl`
- `BtnDisplay`
- `BtnSound`
- `BtnSave`
- `BtnQuit`
- `DescriptionText`

菜单行为：

- `Esc` 或手柄菜单键打开暂停菜单。
- 暂停菜单打开后，`Esc`、手柄 `B`、手柄菜单键关闭菜单并回到游戏。
- 十字键上下或左摇杆上下移动焦点，手柄 `A` / `Enter` 激活当前按钮。
- `Save` 调用现有保存系统，不新增保存流程。
- `Quit` 关闭暂停并返回主菜单。

## 命令let流程

推荐执行顺序：

1. 编译 `DevKitEditor`。
2. 运行 `UnrealEditor-Cmd.exe DevKit.uproject -run=MainUISetup -HudOnly -ForceLayout -unattended -nop4 -nosplash -nullrhi` 查看 `Saved/MainUISetupReport.md`。
3. 确认报告后追加 `-Apply` 生成或刷新 HUD 资产。
4. 如果需要刷新暂停菜单，去掉 `-HudOnly` 并保留 `-ForceLayout -Apply`。

命令let会导入以下 PNG：

- `T_PausePanel_OrnateFrame.png`
- `T_PauseDivider_Ornate.png`
- `T_PauseFocusGlow.png`
- `T_GoldCoinIcon.png`
