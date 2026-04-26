# 编码规范：相机系统

> **Claude 编码前必读**。修改 `YogPlayerCameraManager` / `YogCameraPawn` 或新增相机平滑 / 跟随 / 偏移逻辑时遵循。
>
> 来源：原 memory `feedback_ue5_camera_code.md`（采坑后定型，2026-04-18）。已转写到本规范。

## 关键规则：VInterpTo 起点必须用 `GetCameraLocation()`

`APlayerCameraManager::UpdateViewTarget` 中所有 `VInterpTo` 的第一个参数（起点）必须用 `GetCameraLocation()`，**不能**用 `OutVT.POV.Location`。

```cpp
// ✅ 正确
OutVT.POV.Location = FMath::VInterpTo(GetCameraLocation(), Candidate, DeltaTime, Speed);

// ❌ 错误（根运动时无平滑）
OutVT.POV.Location = FMath::VInterpTo(OutVT.POV.Location, Candidate, DeltaTime, Speed);
```

### 为什么

`Super::UpdateViewTarget()` 调用后，`OutVT.POV.Location` 已是 SpringArm 本帧 snap 到的新位置（受根运动直接跳变）。以此为起点时 VInterpTo 平滑量接近零 → 相机僵硬跟随根运动 → 攻击 / 冲刺时画面抖动明显。

`GetCameraLocation()` 返回 `CameraCache.POV.Location`（**上一帧真实输出**），以此为起点才有真正的平滑效果。

### 适用范围

所有相机跟随 / 偏移的 `VInterpTo` 调用：移动跟随、冲刺跟随、静止归位、LookAhead 偏移恢复。

## 推荐参数（在 BP_PlayerCameraManager Details 调）

| 参数 | 默认 | 说明 |
|---|---|---|
| `bEnableLookAhead` | false | 关闭后无前冲 / 回弹，消除眩晕；仅镜头行程大的关卡才开 |
| `MovingFollowSpeed` | 8 | LookAhead 关闭时的移动跟随速度 |
| `StationarySettleSpeed` | 5 | 静止归位速度（越低漂移越久，建议 5~15） |
| `DashFollowSpeed` | 18 | 冲刺高速跟随，消除单帧抖动 |

## 不要做的事

- ❌ 不要读鼠标位置做相机偏移（`bAutoReadMouseOffset` 已删，不要再加回去）
- ❌ 不要用 `LookAhead` 追敌人（与玩家朝向无关的偏移会让玩家"看不到自己"）
- ❌ 不要按速度分跟随策略（已删除速度分级 follow，统一用 `MovingFollowSpeed`）

## 输入

视角控制：`IA_CameraLook`（Axis2D）→ Gamepad Right Thumbstick 2D-Axis；不挂键鼠。

## 详细架构

→ [Camera_Design](../Systems/Camera/Camera_Design.md)
