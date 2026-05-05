# 玩家通用信息 HUD 配置说明

## 资产入口

- 通用信息 HUD：`/Game/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud`
- 挂载位置：`/Game/UI/Playtest_UI/HUD/WBP_HUDRoot` 的 `BottomRightPlayerInfoRegion`
- C++ 父类：`UPlayerCommonInfoWidget`
- HUDRoot 绑定控件名：`PlayerCommonInfoHud`

## 设计定位

- 该控件不再是金币专用 WBP，而是玩家通用信息 HUD。
- 当前默认显示金币，后续关键道具、非战斗使用类资源、关卡携带物数量等，都应优先扩展到这个 WBP 的 `CommonInfoList` 中。
- 战斗道具栏仍由 `UCombatItemBarWidget` 管理，不混入这里。

## 命名约定

`WBP_PlayerCommonInfoHud` 内部需要保留以下控件名：

- `CommonInfoList`：通用信息条目列表。
- `GoldRow`：金币条目行。
- `GoldIcon`：金币图标。
- `GoldText`：金币数量文本。

金币图标固定以 24x24 显示，避免源图尺寸直接撑大 HUD。

## 数据来源

- 金币读取自玩家 `UBackpackGridComponent::Gold`。
- `UPlayerCommonInfoWidget` 监听 `UBackpackGridComponent::OnGoldChanged`，金币获得、消耗、读档恢复后都会刷新。
- 未来通用条目可调用 `SetCommonInfoEntry(EntryId, Label, Count, IconTexture)` 添加或刷新；`Count <= 0` 时会移除该条目。

## 刷新命令

推荐先编译 `DevKitEditor`，再运行：

```text
UnrealEditor-Cmd.exe DevKit.uproject -run=MainUISetup -HudOnly -ForceLayout -Apply -unattended -nop4 -nosplash -nullrhi
```
