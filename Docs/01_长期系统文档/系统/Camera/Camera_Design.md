# 相机管理系统设计文档

**版本**：v2.1
**日期**：2026-04-16
**状态**：已实现

---

## 1. 系统概述

相机管理系统以 **AYogPlayerCameraManager** 为核心，通过重写 `UpdateViewTarget()` 在 UE 标准 SpringArm + Camera 渲染管线之上叠加智能偏移。这是 UE 推荐的相机定制方式，无需额外 Pawn Actor。

### 架构对比

| 旧方案（v1.x）| 新方案（v2.0）|
|--------------|--------------|
| 独立 AYogCameraPawn（额外 Actor） | 无额外 Actor，逻辑在 CameraManager |
| 多边形 CameraConstraintActor | UE 原生 AYogCameraVolume（Brush） |
| Pawn::Tick 驱动 | CameraManager::UpdateViewTarget 驱动 |
| SetViewTarget 切换视角 | 角色自带 Camera，视角天然正确 |

---

## 2. 架构

```
APlayerCharacterBase (角色蓝图)
  ├── UYogSpringArmComponent (CameraBoom)   ← 臂长、碰撞避让、关闭自带 Lag
  └── UCameraComponent                      ← 实际相机，视角来源

AYogPlayerCameraManager                     ← 核心：重写 UpdateViewTarget()
  │  PlayerController 构造函数自动注册
  │
  ├── UpdateViewTarget(OutVT, DeltaTime)
  │     Step 1  Super() → SpringArm + Camera 写入基础 POV
  │     Step 2  获取玩家位置、速度
  │     Step 3  更新 MovingTime / LookAheadAlpha
  │     Step 4  DetermineState()           ← 优先级状态决策
  │     Step 5  ComputeStateOffset()       ← XY 偏移
  │     Step 6  InputOffset（手柄/鼠标）
  │     Step 7  合成候选位置
  │     Step 8  VolumeClamp               ← EncompassesPoint 约束
  │     Step 9  应用（Dash 直接赋值，其余 VInterpTo）
  │
  └── API：SetDashMode / SetCameraInputAxis / NotifyHeavyHit / NotifyCritHit
           SetConstraintVolume

AYogCameraVolume (Level 中放置的 Brush Volume)
  ├── OnOverlapBegin(Player) → CameraManager->SetConstraintVolume(this)
  └── OnOverlapEnd(Player)   → CameraManager->SetConstraintVolume(nullptr)

AYogGameMode (战斗感知数据源，不变)
  ├── RegisterEnemy / UnregisterEnemy
  ├── GetEnemyCentroid / GetNearestEnemyDirection
  └── GetNearbyEnemies
```

---

## 3. 相机状态优先级

```
Priority 1 ▶ Dash          冲刺：直接赋值，无任何延迟
Priority 2 ▶ CombatFocus   战斗中 + 附近有敌：向质心偏移
Priority 3 ▶ CombatSearch  战斗中 + 敌人超出半径：向最近敌人方向偏移
Priority 4 ▶ PickupFocus   整理阶段 + 场景有拾取物：向拾取物偏移
Priority 5 ▶ LookAhead     移动中：前瞻偏移（Alpha 随时间 0→1）
Priority 6 ▶ FocusCharacter 静止：慢速回归中心
```

> 状态 2/3 在 `GameMode.CurrentPhase == Combat` 时激活；
> 状态 4 在 `GameMode.CurrentPhase == Arrangement` 且有 `ARewardPickup` 时激活。

---

## 4. 前瞻系统（LookAhead）

> **默认关闭**（`bEnableLookAhead = false`）。开启后相机在移动时向前领先，停下时回弹，适合镜头行程较大的关卡。关闭时使用纯跟随模式（见第 4.1 节）。

| 阶段 | 时间 | 行为 |
|------|------|------|
| 初期落后 | 0 → `LookAheadBuildupTime` | Alpha=0，慢速跟随（InitialFollowLerpSpeed） |
| 前瞻增强 | `LookAheadBuildupTime` 后 | Alpha→1，领先移动方向（LookAheadLerpSpeed） |

```
LookAheadAlpha = Clamp(MovingTime / LookAheadBuildupTime, 0, 1)
XY偏移 = LastMovementDir.XY × LookAheadDistance × LookAheadAlpha
停止后 Alpha 以 LookAheadAlphaDecaySpeed 快速衰减
```

### 4.1 纯跟随模式（LookAhead 关闭，默认）

关闭 `bEnableLookAhead` 后：

- 移动时：以 `MovingFollowSpeed`（默认 8）平滑追随，不产生领先偏移
- 静止时：以 `StationarySettleSpeed`（默认 5）缓速归位，残留漂移迅速消失
- 状态机始终在 `FocusCharacter`（跳过 LookAhead 状态）

```
移动  → VInterpTo(GetCameraLocation(), SpringArmPos, DeltaTime, MovingFollowSpeed)
静止  → VInterpTo(GetCameraLocation(), SpringArmPos, DeltaTime, StationarySettleSpeed)
冲刺  → VInterpTo(GetCameraLocation(), SpringArmPos, DeltaTime, DashFollowSpeed)
```

> **关键**：起点使用 `GetCameraLocation()`（上一帧输出），而非 `OutVT.POV.Location`（当帧 SpringArm snap 位置）。
> 这样根运动造成的角色位移会被 VInterpTo 平滑，消除攻击/冲刺时镜头直接"贴身"的僵硬感。

---

## 5. 冲刺模式（Dash）

- `UpdateViewTarget` 中对 `OutVT.POV.Location` **直接赋值**，不经过 VInterpTo
- 进入时 `LookAheadAlpha` 重置为 0，避免残余偏移
- **接入**：`GA_PlayerDash` 在 Activate/End 调用 `GetPlayerCameraManager(0)->SetDashMode(true/false)`

---

## 6. 边界约束（AYogCameraVolume）

### 原理

`AVolume::EncompassesPoint(FVector)` 是 UE 内置方法，直接检测 3D 点是否在 Brush 几何体内。
CameraManager 将候选相机位置（投影到玩家 Z 高度）传入检测，若超出则 XY 退回玩家位置。

```cpp
const FVector CheckPt(Candidate.X, Candidate.Y, PlayerPos.Z);
if (!ConstraintVolume->EncompassesPoint(CheckPt))
{
    Candidate.X = PlayerPos.X;
    Candidate.Y = PlayerPos.Y;
}
```

### 关卡配置

1. 关卡编辑器 > Place Actors 搜索 `YogCameraVolume` 拖入关卡
2. 用 **Geometry Edit 模式**（快捷键 Shift+5）拖拽顶点围住可玩区域
3. 可以是任意形状，高度建议覆盖 `-1000 ~ +1000`（EncompassesPoint 是 3D 检测）
4. 玩家走进 Volume 时自动注册，走出时注销
5. 每个关卡可放多个 Volume（嵌套区域），但只有最后触发的生效

---

## 7. 战斗感知偏移

**数据流**（同 v1.x，不变）：
```
EnemyCharacterBase::BeginPlay  → GameMode.RegisterEnemy()
EnemyCharacterBase::Die        → GameMode.UnregisterEnemy()
AYogPlayerCameraManager::UpdateViewTarget → GameMode.GetEnemyCentroid() / GetNearestEnemyDirection()
```

---

## 8. 输入偏移（手柄右摇杆 / 鼠标）

```
GamepadInputAxis.SizeSquared() > 0.01f
  → 使用 GamepadInputAxis（由 PlayerController.CameraLook 写入）
否则 + bAutoReadMouseOffset=true
  → 自动读取 GetMousePosition，归一化到 [-1,1]
最终偏移 = ActiveAxis × MaxInputOffset（VInterpTo 平滑）
```

- **鼠标**：CameraManager `UpdateViewTarget` 中自动读取，无需额外配置
- **手柄**：PlayerController `CameraLook` 回调 → `CameraManager->SetCameraInputAxis()`

---

## 9. 相机震动

CameraManager 自身就是 `APlayerCameraManager`，直接调用内置 `StartCameraShake()`，无需中转。

| 属性 | 说明 |
|------|------|
| `HeavyHitShakeClass` | 重伤时的 CameraShake 蓝图 |
| `CritHitShakeClass` | 暴击时的 CameraShake 蓝图 |

调用：`Cast<AYogPlayerCameraManager>(GetPlayerCameraManager(0))->NotifyHeavyHit()`

---

## 10. 可调参数汇总

> 参数在 **BP_PlayerCameraManager 蓝图 Details** 中调整，无需重新编译。

### 10.1 跟随行为（Camera|Movement / Camera|LookAhead）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `bEnableLookAhead` | **false** | 是否启用前瞻偏移；关闭时相机只跟随不领先 |
| `MovingFollowSpeed` | **8.0** | LookAhead 关闭时移动跟随速度（推荐 6~12） |
| `StationarySettleSpeed` | **5.0** | 静止时归位速度（越大越快；5=缓慢，15=快速） |
| `DashFollowSpeed` | **18.0** | 冲刺期间跟随速度（0=直接 snap） |
| `MovingSpeedThreshold` | 10 uu/s | 速度超过此值视为移动中 |

### 10.2 前瞻系统（仅 bEnableLookAhead=true 时生效）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `LookAheadBuildupTime` | 0.5s | 达到最大前瞻偏移所需移动时间 |
| `LookAheadDistance` | 280 uu | 最大前瞻偏移距离 |
| `LookAheadLerpSpeed` | 4.0 | 前瞻阶段跟随速度 |
| `InitialFollowLerpSpeed` | 2.0 | 起步时跟随速度（产生落后感） |
| `LookAheadAlphaDecaySpeed` | 5.0 | 停止后前瞻衰减速度 |
| `FocusLerpSpeed` | 1.5 | 静止时回归中心的速度（LookAhead 开启时） |

### 10.3 战斗 / 拾取感知

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CombatSearchRadius` | 1200 uu | 视为"附近有敌人"的半径 |
| `CombatBiasDistance` | 220 uu | CombatFocus 最大偏移 |
| `CombatSearchOffset` | 160 uu | CombatSearch 偏移 |
| `CombatLerpSpeed` | 2.5 | 战斗状态跟随速度 |
| `PickupBiasDistance` | 150 uu | 整理阶段拾取物偏移 |
| `PickupLerpSpeed` | 2.0 | 拾取物状态跟随速度 |

### 10.4 输入偏移 / 震动

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `MaxInputOffset` | 200 uu | 手柄/鼠标最大偏移 |
| `InputOffsetLerpSpeed` | 8.0 | 输入偏移平滑速度 |
| `bAutoReadMouseOffset` | true | 手柄未激活时自动读取鼠标 |
| `HeavyHitShakeClass` | — | 重伤相机震动蓝图 |
| `CritHitShakeClass` | — | 暴击相机震动蓝图 |

---

## 11. 编辑器配置指南

### 11.1 角色蓝图（一次性）

打开玩家角色蓝图 > Components 面板：

1. 添加 `YogSpringArmComponent`，命名 `CameraBoom`
2. 在 CameraBoom 下添加 `CameraComponent`
3. 选中 CameraBoom，在 Details 中：
   - `Target Arm Length` → 调整到俯视距离（约 1200~1800）
   - `Socket Offset` → Z 值控制相机高度（约 600~900）
   - `Rotation` → X=-60（俯角），Y=0，Z=0（或按需调整）
   - **关闭** `Enable Camera Lag`（由 CameraManager 统一处理偏移）
   - **关闭** `Enable Camera Rotation Lag`
   - `bUsePawnControlRotation = false`

### 11.2 PlayerController 蓝图（自动生效）

C++ 构造函数已设置 `PlayerCameraManagerClass = AYogPlayerCameraManager::StaticClass()`，蓝图子类自动继承，**无需手动配置**。

如需在蓝图中覆盖参数（如调整 LookAheadDistance），可在 PlayerController 蓝图中：
Details > Player Camera Manager > 找到相关参数直接修改。

### 11.3 关卡边界 Volume

1. Place Actors 面板搜索 `Yog Camera Volume`，拖入关卡
2. 选中后按 **Shift+5** 进入 Geometry Edit 模式，拖拽顶点
3. 或在 Details > Brush Settings 直接输入尺寸（矩形地图用这个最快）
4. Volume 高度需覆盖玩家可能到达的 Z 范围

### 11.4 相机震动（可选）

1. 创建 `CameraShakeBase` 蓝图子类，配置震动参数
2. 在 PlayerController 蓝图 Details > Player Camera Manager > `Heavy Hit Shake Class` / `Crit Hit Shake Class` 中指定
3. 受击逻辑中调用：
   ```
   Cast<AYogPlayerCameraManager>(GetPlayerCameraManager()) → NotifyHeavyHit / NotifyCritHit
   ```

---

## 12. 相关文件

| 文件 | 说明 |
|------|------|
| [YogPlayerCameraManager.h](../../../../Source/DevKit/Public/Camera/YogPlayerCameraManager.h) | CameraManager 声明（状态枚举、参数、API）|
| [YogPlayerCameraManager.cpp](../../../../Source/DevKit/Private/Camera/YogPlayerCameraManager.cpp) | UpdateViewTarget 完整实现 |
| [YogSpringArmComponent.h](../../../../Source/DevKit/Public/Camera/YogSpringArmComponent.h) | SpringArm 组件（挂在角色蓝图上）|
| [YogCameraVolume.h](../../../../Source/DevKit/Public/Volume/YogCameraVolume.h) | 边界 Volume |
| [YogCameraVolume.cpp](../../../../Source/DevKit/Private/Volume/YogCameraVolume.cpp) | Overlap → SetConstraintVolume |
| [YogPlayerControllerBase.cpp](../../../../Source/DevKit/Private/Character/YogPlayerControllerBase.cpp) | PlayerCameraManagerClass 注册、CameraLook 路由 |
| [GA_PlayerDash.cpp](../../../../Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayerDash.cpp) | SetDashMode 调用 |
| [YogGameMode.h](../../../../Source/DevKit/Public/GameModes/YogGameMode.h) | 敌人注册接口 |

### 已废弃（保留但不再使用）

| 文件 | 原因 |
|------|------|
| `YogCameraPawn.h/.cpp` | 被 CameraManager 方案替代 |
| `CameraConstraintActor.h/.cpp` | 被 YogCameraVolume 替代 |

---

## 13. 待办

- [ ] 角色蓝图添加 YogSpringArmComponent + CameraComponent 并调整参数
- [ ] 关卡中放置 AYogCameraVolume 围住可玩区域
- [ ] 创建 CameraShake 蓝图资产并分配
- [ ] 受击逻辑接入 NotifyHeavyHit / NotifyCritHit

---

## ⚠️ Claude 编写注意事项

- **禁止 LookAhead**：不要添加任何前瞻偏移逻辑（根据速度方向预测摄像机位置），已确认不符合本项目手感要求
- **插值起点必须用 GetCameraLocation()**：`VInterpTo` 的当前值必须传 `SpringArmComponent->GetComponentLocation()`（实际摄像机位置），不能用 `TargetLocation`（会导致插值抖动）
- **禁止追敌摄像机**：摄像机不跟踪敌人，只跟随玩家角色，不要加任何基于敌人位置的偏移逻辑
- **6 状态优先级**：普通(0) < 冲刺(1) < 战斗(2) < 背包(3) < 引导(4) < 边界(5)，高优先级状态覆盖低优先级，C++ 里用整型优先级比较，不要用枚举位运算
- **多边形边界约束**：摄像机位置限制在 `PolygonBoundaryPoints` 定义的多边形内，用射线投影到最近边的方式修正，不要用 AABB（矩形边界太生硬）
- **分速度插值**：普通跟随用慢速（建议 InterpSpeed=3.0），冲刺状态用快速（InterpSpeed=8.0），状态切换时立即切换速度，不要对速度本身插值
- **禁止鼠标偏移**：摄像机位置与鼠标位置无关，不要读取鼠标坐标影响摄像机
