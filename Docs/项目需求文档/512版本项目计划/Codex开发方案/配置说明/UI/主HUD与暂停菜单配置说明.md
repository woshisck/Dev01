# 主 HUD 与暂停菜单配置说明

## 资产入口

- 主 HUD：`/Game/UI/Playtest_UI/HUD/WBP_HUDRoot`
- 暂停菜单：`/Game/UI/Playtest_UI/Pause/WBP_PauseMenu`
- 暂停菜单纹理源文件：`SourceArt/UI/Pause/`
- 暂停菜单导入纹理：`/Game/UI/Playtest_UI/UI_Tex/Pause/`
- 生成/刷新命令：`MainUISetupCommandlet`，建议先 dry-run，再使用 `-Apply`。

## WBP_HUDRoot 命名契约

`WBP_HUDRoot` 的父类必须是 `YogHUDRootWidget`。以下控件需要保持 `Is Variable` 并使用固定名称：

- 区域容器：`TopLeftPlayerInfoRegion`、`TopRightPlayerInfoRegion`、`BossInfoRegion`、`LeftLevelInfoRegion`、`RightLevelInfoRegion`、`BottomLeftPlayerInfoRegion`、`BottomCenterCombatRegion`、`BottomRightPlayerInfoRegion`
- 现有 HUD 子控件：`PlayerHealthBar`、`EnemyArrow`、`WeaponGlassIcon`、`HeatBar`、`InfoPopup`、`CombatDeckBar`、`CurrentRoomBuffPanel`

布局约定：

- `PlayerHealthBar` 放在底部左侧玩家信息区。
- `CombatDeckBar` 放在底部中央战斗信息区。
- `CurrentRoomBuffPanel` 放在左侧关卡信息区，数据仍来自 `YogGameMode.ActiveRoomBuffs`。
- `BossInfoRegion` 默认隐藏，后续 Boss 血条或阶段信息放入该区域。
- 顶部左右玩家信息区和底部右侧玩家信息区本次只作为未来道具、技能、资源 UI 的预留容器。

## WBP_PauseMenu 命名契约

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
2. 运行 `UnrealEditor-Cmd.exe DevKit.uproject -run=MainUISetup -unattended -nop4 -nosplash` 查看 `Saved/MainUISetupReport.md`。
3. 确认报告后运行 `-Apply`。
4. 如需强制刷新设计器树，额外加 `-ForceLayout`。

命令let会导入以下 PNG：

- `T_PausePanel_OrnateFrame.png`
- `T_PauseDivider_Ornate.png`
- `T_PauseFocusGlow.png`
