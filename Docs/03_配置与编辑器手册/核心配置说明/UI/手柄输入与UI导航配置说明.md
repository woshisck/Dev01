# 手柄输入与 UI 导航配置说明

## 目的

统一当前 核心方案的手柄操作语义，避免 Gameplay 输入和 UI 输入互相抢键。

## 默认键位

| 场景 | 手柄 | 键鼠 | 说明 |
| --- | --- | --- | --- |
| 移动 | Left Stick | WASD | 玩家移动 |
| 镜头偏移 | Right Stick | 无 | 相机偏移输入 |
| 轻攻击 | X / FaceButton_Left | Left Mouse | 普通攻击 |
| 重攻击 | Y / FaceButton_Top | Right Mouse | 重击/蓄力释放 |
| 交互 / UI 确认 | A / FaceButton_Bottom | E / Enter | 拾取、进入、确认 |
| 冲刺 / UI 返回 | B / FaceButton_Right | Space / Esc | 游戏中冲刺，UI 中返回 |
| 背包 | View / SpecialLeft | Tab | 仅 Gameplay 中打开 |
| 暂停 | Menu / SpecialRight | Esc | 无顶层 UI 时打开暂停 |
| 装填 | RB / RightShoulder | R | 远程武器装填 |
| 使用战斗道具 | LB / LeftShoulder | F | 使用当前道具 |
| 切换道具 | DPad Left / Right | Z / Q | 上一个 / 下一个道具 |

## 配置入口

- 输入映射：`/Game/Code/Core/Input/IMC_YogPlayerBase`
- 输入动作：`/Game/Code/Core/Input/Actions/`
- 玩家控制器默认值：`/Game/Code/Core/Controller/B_YogPlayerControllerBase`
- 富文本按键图标：`/Game/Docs/UI/Tutorial/BP_InputActionDecorator`

## 刷新流程

1. 编译 Editor 后先 dry-run：

   ```powershell
   DevKitEditor-Cmd.exe "X:\Project\Dev01\DevKit.uproject" -run=GamepadInputSetup -unattended -nop4
   ```

2. 确认 `Saved/GamepadInputSetupReport.md` 后应用：

   ```powershell
   DevKitEditor-Cmd.exe "X:\Project\Dev01\DevKit.uproject" -run=GamepadInputSetup -Apply -unattended -nop4
   ```

3. 检查报告中至少包含：
   - `IA_Interact` 映射到 `E, Gamepad_FaceButton_Bottom`
   - `IA_Dash` 映射到 `SpaceBar, Gamepad_FaceButton_Right`
   - `IA_Reload` 映射到 `R, Gamepad_RightShoulder`
   - `IA_UseCombatItem` / `IA_SwitchCombatItemNext` / `IA_SwitchCombatItemPrevious` 已创建或找到

## UI 验收

- 所有常用 UI 使用 `A=确认`、`B=返回`、DPad/Left Stick 导航。
- 暂停、战利品、背包、卡组编辑、商店、祭坛、净化、献祭、教程弹窗都需要手柄可操作。
- UI 打开时 Gameplay 的移动、攻击、冲刺、交互、装填、道具输入不得触发。
- 输入提示文字优先使用 `<input action="Interact"/>`、`<input action="Dash"/>`、`<input action="OpenBackpack"/>`、`<input action="UseCombatItem"/>` 等 action token。
