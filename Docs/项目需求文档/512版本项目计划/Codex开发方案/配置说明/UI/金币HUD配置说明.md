# 金币 HUD 配置说明

## 显示位置

- 金币显示挂在 `WBP_HUDRoot` 的 `BottomLeftPlayerInfoRegion`。
- 运行时由 `AYogHUD` 创建 `GoldPanelRuntime`，内部包含 `GoldIconRuntime` 与 `GoldTextRuntime`，放在左下角玩家 HUD 区域顶部，避免重建当前 HUD 蓝图资产。

## 图标资产

- 源图路径：`SourceArt/UI/HUD/T_GoldCoinIcon.png`。
- 导入后的 UI 纹理路径：`/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon`。
- 可通过命令let单独刷新 HUD 纹理：`MainUISetup -HudOnly -Apply`。

## 数据来源

- 当前金币读取自玩家的 `UBackpackGridComponent::Gold`。
- HUD 监听 `UBackpackGridComponent::OnGoldChanged`，金币获得、消费、跨关恢复广播后都会刷新。
- 初次绑定时会立即读取当前金币值，避免错过角色 BeginPlay 期间的恢复广播。

## 显示格式

- 当前格式为：金币图标 + `{0} G`。
- 如需替换图标，只需要覆盖源图并重新运行 `MainUISetup -HudOnly -Apply`；金币数据绑定不需要改。
