# WBP_CurrentRoomBuffPanel 配置说明

## 作用

显示当前关卡实际施加给敌人的符文 Buff。数据来自 `YogGameMode.ActiveRoomBuffs`，不是 `DA_Room.BuffPool` 候选池。

这和传送门下一关预览的 `PreRolledBuffs` 是同一批结果：玩家进入该关后，HUD 会把这批符文作为“当前关卡敌人符文”显示出来。

## 当前状态

- 已生成 WBP 资产：`Content/UI/Playtest_UI/HUD/WBP_CurrentRoomBuffPanel.uasset`。
- WBP 父类：`CurrentRoomBuffWidget`。
- `YogHUD` 未手动配置 `CurrentRoomBuffWidgetClass` 时，会自动尝试加载 `/Game/UI/Playtest_UI/HUD/WBP_CurrentRoomBuffPanel`。
- 如果 WBP 资产不存在，`YogHUD` 会回退到 `CurrentRoomBuffWidget` 的 C++ 默认布局，不会阻塞游玩。

## WBP 层级

当前自动生成的 Designer 层级：

```text
RootCanvas
  PanelSizeBox
    OuterBorder
      PanelStack
        TitleText
        RoomNameText
        BuffListBox
        EmptyText
```

关键控件：

| 控件名 | 类型 | 用途 |
| --- | --- | --- |
| `OuterBorder` | Border | 面板背景 |
| `TitleText` | TextBlock | 标题 |
| `RoomNameText` | TextBlock | 当前房间名 |
| `BuffListBox` | VerticalBox | C++ 动态填充符文行 |
| `EmptyText` | TextBlock | 无 Buff 时提示 |

这些控件都是 `BindWidgetOptional`。如果后续重做样式，只要保留 `BuffListBox`，C++ 就会继续填充符文图标、名称、摘要和通用效果文本。

符文摘要优先读取 `RuneInfo -> RuneConfig -> HUDSummaryText`。该字段用于当前关卡面板和传送门关卡预览，建议手写 1-2 行；为空时会从 `RuneDescription` 自动截取短摘要，完整描述仍保留给背包详情、符文信息卡等大面板使用。

## 挂载方式

推荐保持自动加载即可，不需要额外蓝图逻辑。

可选手动挂载：

1. 在 `WBP_HUDRoot` 中拖入 `WBP_CurrentRoomBuffPanel`，变量名设为 `CurrentRoomBuffPanel`。
2. 或在 `BP_YogHUD -> Room Buff -> CurrentRoomBuffWidgetClass` 中指定 `WBP_CurrentRoomBuffPanel`。

两者都不配置时，HUD 会创建当前 WBP；如果 WBP 加载失败，才使用 C++ 默认样式。

## 重新生成 WBP

如果该 WBP 被误删，可以运行：

```powershell
& 'Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'X:\Project\Dev01\DevKit.uproject' -run=CurrentRoomBuffWidgetSetup -Apply -unattended -nop4 -nosplash
```

生成报告位置：

```text
Saved/CurrentRoomBuffWidgetSetupReport.md
```

## 验收

1. `DA_Room.BuffPool` 中配置敌人符文 Buff，优先使用 `Content/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/` 下的 512 敌人/关卡专用符文。
2. 当前难度档位的 `BuffCount` 大于 0。
3. 进入关卡后，HUD 左上区域显示本关实际抽中的敌人符文。
4. 面板内容与进入该关前传送门预览的 Buff 一致。
5. Hub 房间或无 Buff 房间不显示面板。
