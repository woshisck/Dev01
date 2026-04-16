# 相机管理系统设计文档

**版本**：v1.2  
**日期**：2026-04-16  
**状态**：已实现，配置指南见第 11 节

---

## 1. 系统概述

相机管理系统以 **AYogCameraPawn** 为核心，作为独立 Pawn 控制相机位置，通过 `SetViewTarget` 成为玩家视角。Owner 设置为玩家角色，每帧跟随其位置并叠加多层偏移。

### 设计目标

| 目标 | 效果 |
|------|------|
| 静止自然聚焦 | 玩家静止时相机缓慢回到角色中心，感觉有呼吸感 |
| 移动前瞻视野 | 玩家移动初期稍有落后，持续移动后领先于移动方向，给予更多前方视野 |
| 冲刺冲击感 | 冲刺时相机 1:1 无延迟跟随，强调位移力度 |
| 战斗感知 | 相机向屏幕上敌人质心偏移，协助玩家感知战场 |
| 整理阶段引导 | 消灭全部敌人后，相机向拾取物偏移，引导玩家走向奖励 |
| 边界约束 | 通过多边形 Constraint Actor 防止相机越出地图 |
| 输入微偏 | 手柄右摇杆 / 鼠标引起细微偏移，增强控制感 |
| 特殊事件反馈 | 暴击、重伤时触发可配置的相机震动 |

---

## 2. 架构

```
AYogCameraPawn
│
├── UpdateCameraLoc(DeltaTime)         ← 每帧入口
│   ├── 获取玩家速度，判断是否移动
│   ├── 更新 MovingTime / LookAheadAlpha
│   ├── DetermineState()               ← 优先级状态决策
│   ├── ComputeStateOffset()           ← 计算 XY 偏移
│   ├── 更新 ActiveAxis（手柄优先，否则鼠标）
│   ├── 平滑 CurrentInputOffset
│   ├── 合成 TargetXY = PlayerXY + StateOffset + InputOffset
│   ├── ConstrainPosition()            ← 多边形边界约束
│   └── VInterpTo / SetActorLocation   ← 位置应用
│
├── ACameraConstraintActor             ← 场景中放置的边界 Actor
│   └── BoundaryPoints[]               ← 美术在 Details 中编辑顶点
│
└── AYogGameMode（战斗感知数据源）
    ├── AliveEnemies[]
    ├── GetEnemyCentroid()
    ├── GetNearestEnemyDirection()
    └── GetNearbyEnemies()
```

---

## 3. 相机状态优先级

```
Priority 1 ▶ Dash          冲刺：无延迟 1:1 跟随
Priority 2 ▶ CombatFocus   战斗中 + 附近有敌：向质心偏移
Priority 3 ▶ CombatSearch  战斗中 + 敌人不在范围：向最近敌人方向偏移
Priority 4 ▶ PickupFocus   整理阶段 + 拾取物存在：向拾取物偏移
Priority 5 ▶ LookAhead     玩家移动：前瞻偏移（随时间增强）
Priority 6 ▶ FocusCharacter 静止：慢速回归玩家中心
```

> 状态 2/3 在 `GameMode.CurrentPhase == Combat` 时才激活；  
> 状态 4 在 `GameMode.CurrentPhase == Arrangement` 且场景中有 `ARewardPickup` 时激活。

---

## 4. 前瞻系统（LookAhead）

| 阶段 | 时间 | 相机行为 |
|------|------|----------|
| 初期落后 | 0 → `LookAheadBuildupTime` | Alpha=0，慢速跟随，产生轻微落后感 |
| 前瞻增强 | `LookAheadBuildupTime` 后 | Alpha→1，逐步移到移动方向前方 |

- `LookAheadAlpha = Clamp(MovingTime / LookAheadBuildupTime, 0, 1)`
- 目标偏移：`LastMovementDir × LookAheadDistance × CurrentLookAheadAlpha`
- 跟随速度：从 `InitialFollowLerpSpeed` 插值到 `LookAheadLerpSpeed`
- 停止后 Alpha 以 `LookAheadAlphaDecaySpeed` 快速衰减

---

## 5. 冲刺模式（Dash）

- `SetActorLocation(PlayerPos)` 直接设置，**不经过 VInterpTo**
- 进入时 `LookAheadAlpha` 重置为 0，避免残余偏移
- **接入**：已在 `GA_PlayerDash.cpp` 中自动调用，无需额外配置

```
ActivateAbility → Player->GetOwnCamera()->SetDashMode(true)
EndAbility      → Player->GetOwnCamera()->SetDashMode(false)
```

---

## 6. 边界约束（ACameraConstraintActor）

1. 将 `ACameraConstraintActor`（或其蓝图子类）拖入关卡（位置任意）
2. Details > **Camera Constraint** > **Boundary Points** 添加顶点（或在视口直接拖拽黄色控制柄）
3. 顶点 Z 值无关紧要，系统只取 XY 坐标
4. 非 Shipping 版本下自动绘制**青色连线 + 黄色球标**
5. 每个关卡放**一个**即可，CameraPawn 自动查找并缓存

> **坐标系说明**：BoundaryPoints 存储的是 Actor 本地坐标，运行时自动通过 `GetActorTransform().TransformPosition()` 转换为世界坐标参与计算和调试绘制。Actor 可放在关卡任意位置，边界会正确跟随。

算法：**射线法**判断点是否在多边形内，在外则返回距最近边的投影点（`FMath::ClosestPointOnSegment2D`）。

---

## 7. 战斗感知偏移

**数据流**：
```
AEnemyCharacterBase::BeginPlay  → GameMode.RegisterEnemy()
AEnemyCharacterBase::Die        → GameMode.UnregisterEnemy()
AYogCameraPawn::Tick            → GameMode.GetEnemyCentroid() / GetNearestEnemyDirection()
```

| 状态 | 触发条件 | 偏移计算 |
|------|----------|----------|
| CombatFocus | `CombatSearchRadius` 内有存活敌人 | 玩家 → 质心方向 × `CombatBiasDistance` |
| CombatSearch | 有存活敌人但超出搜索半径 | 玩家 → 最近敌人方向 × `CombatSearchOffset` |

---

## 8. 输入偏移（手柄右摇杆 / 鼠标）

### 逻辑

每帧根据 `GamepadInputAxis.SizeSquared() > 0.01f` 判断优先级：

```
手柄激活 → ActiveAxis = GamepadInputAxis（由 SetCameraInputAxis 写入）
手柄未激活 + bAutoReadMouseOffset=true
         → ActiveAxis = 鼠标归一化坐标（在 CameraPawn Tick 中自动读取）
最终偏移  = ActiveAxis × MaxInputOffset（经 InputOffsetLerpSpeed 平滑）
```

### 接入情况

| 输入源 | 接入方式 | 状态 |
|--------|----------|------|
| 鼠标 | CameraPawn Tick 自动读取，无需额外配置 | ✅ 已完成 |
| 手柄右摇杆 | PlayerController 绑定 `Input_CameraLook` | ✅ 已完成（需编辑器配置，见第 11 节）|

---

## 9. 相机震动（特殊事件）

在 **CameraPawn 蓝图 Details** 中配置：

| 属性 | 说明 |
|------|------|
| `HeavyHitShakeClass` | 重伤时播放的 CameraShake 蓝图类 |
| `HeavyHitShakeScale` | 震动强度（默认 1.0） |
| `CritHitShakeClass` | 暴击时的 CameraShake 蓝图类 |
| `CritHitShakeScale` | 震动强度（默认 1.0） |

调用方式（GE / GA / AnimNotify 中）：
```
PlayerCharacter → GetOwnCamera → NotifyHeavyHit()
PlayerCharacter → GetOwnCamera → NotifyCritHit()
```

---

## 10. 可调参数汇总

> 所有参数均可在 **CameraPawn 蓝图 Details** 中调整，无需改代码。

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `LookAheadBuildupTime` | 0.5s | 达到最大前瞻偏移所需移动时间 |
| `LookAheadDistance` | 280 uu | 最大前瞻偏移距离 |
| `LookAheadLerpSpeed` | 4.0 | 前瞻状态跟随速度 |
| `InitialFollowLerpSpeed` | 2.0 | 起步时跟随速度（产生落后感） |
| `LookAheadAlphaDecaySpeed` | 5.0 | 停止后前瞻衰减速度 |
| `FocusLerpSpeed` | 1.5 | 静止时回归中心的速度 |
| `CombatSearchRadius` | 1200 uu | 视为"附近有敌人"的半径 |
| `CombatBiasDistance` | 220 uu | CombatFocus 最大偏移距离 |
| `CombatSearchOffset` | 160 uu | CombatSearch 偏移距离 |
| `CombatLerpSpeed` | 2.5 | 战斗状态跟随速度 |
| `PickupBiasDistance` | 150 uu | 整理阶段拾取物偏移距离 |
| `PickupLerpSpeed` | 2.0 | 拾取物状态跟随速度 |
| `MaxInputOffset` | 200 uu | 手柄/鼠标最大偏移量 |
| `InputOffsetLerpSpeed` | 8.0 | 输入偏移平滑速度 |
| `bAutoReadMouseOffset` | true | 手柄未激活时是否自动读取鼠标偏移 |
| `MovingSpeedThreshold` | 10 uu/s | 速度超过此值视为"移动中" |

---

## 11. 编辑器配置指南

### 11.1 首次接入流程（按顺序操作）

**Step 1 — 创建 CameraPawn 蓝图**

> 如果已有蓝图子类，跳过此步。

1. 内容浏览器 > 右键 > Blueprint Class
2. 父类选 `YogCameraPawn`
3. 命名为 `BP_YogCameraPawn`

**Step 2 — 配置 PlayerController 蓝图**

打开 `BP_YogPlayerController`（或你的 PlayerController 蓝图子类），在 **Details** 面板找到：

| 属性 | 填写内容 |
|------|----------|
| Camera Setting > **Camera Pawn Class** | `BP_YogCameraPawn` |
| Input > **Input Camera Look** | `IA_CameraLook`（见 Step 3） |

**Step 3 — 创建 IA_CameraLook InputAction 资产**

1. 内容浏览器 > 右键 > Input > **Input Action**
2. 命名为 `IA_CameraLook`
3. 打开，将 **Value Type** 设为 **Axis2D (Vector2D)**
4. 保存

**Step 4 — 在 IMC 中绑定右摇杆**

1. 打开你的 **Input Mapping Context**（`IMC_Default` 或类似名称）
2. 点击 `+`，选择 `IA_CameraLook`
3. 添加映射：**Gamepad Right Thumbstick 2D-Axis**
4. 无需额外修改器（Enhanced Input 会自动归一化）
5. 保存 IMC

---

### 11.2 边界约束配置

1. 内容浏览器搜索 `CameraConstraintActor`，拖入关卡
2. 选中该 Actor，Details > **Camera Constraint** > **Boundary Points**
3. 点击 `+` 添加顶点，或在视口中拖拽顶点（`MakeEditWidget` 已启用）
4. 通常放 4~6 个顶点围住地图可玩区域

> 每个关卡只需放**一个** CameraConstraintActor，系统自动查找并缓存。

---

### 11.3 相机震动配置（可选）

1. 内容浏览器 > 右键 > Blueprint Class > 父类选 `CameraShakeBase`（或 `MatineeCameraShake`）
2. 配置震动参数（幅度、频率、持续时间）
3. 打开 `BP_YogCameraPawn` > Details：
   - `Heavy Hit Shake Class` → 你的重伤震动蓝图
   - `Crit Hit Shake Class` → 你的暴击震动蓝图
4. 在受击逻辑（GE / AnimNotify）中调用：
   ```
   PlayerCharacter → GetOwnCamera → NotifyHeavyHit / NotifyCritHit
   ```

---

### 11.4 手动 Spawn CameraPawn（如未自动创建）

如果关卡开始后没有相机，检查 PlayerController 蓝图的 `BeginPlay`：

```
Event BeginPlay → SpawnCameraPawn(GetControlledCharacter())
```

> `SpawnCameraPawn` 会自动设置 Owner、调用 `SetOwnCamera`、并执行 `SetViewTarget`。

---

## 12. 相关文件

| 文件 | 说明 |
|------|------|
| [YogCameraPawn.h](../../../Source/DevKit/Public/Camera/YogCameraPawn.h) | 相机 Pawn 声明 |
| [YogCameraPawn.cpp](../../../Source/DevKit/Private/Camera/YogCameraPawn.cpp) | 相机 Pawn 实现 |
| [CameraConstraintActor.h](../../../Source/DevKit/Public/Camera/CameraConstraintActor.h) | 边界约束 Actor |
| [YogPlayerControllerBase.h](../../../Source/DevKit/Public/Character/YogPlayerControllerBase.h) | 输入绑定（含 Input_CameraLook） |
| [YogPlayerControllerBase.cpp](../../../Source/DevKit/Private/Character/YogPlayerControllerBase.cpp) | CameraLook / CameraLookReleased 实现 |
| [GA_PlayerDash.cpp](../../../Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayerDash.cpp) | SetDashMode 调用 |
| [YogGameMode.h](../../../Source/DevKit/Public/GameModes/YogGameMode.h) | 敌人注册接口 |
| [EnemyCharacterBase.cpp](../../../Source/DevKit/Private/Character/EnemyCharacterBase.cpp) | 注册/注销钩子 |

---

## 13. 待办

- [ ] 创建 CameraShake 蓝图资产并分配到 CameraPawn Details
- [ ] 关卡中放置 ACameraConstraintActor 并配置多边形顶点
- [ ] 重伤/暴击逻辑接入 `NotifyHeavyHit / NotifyCritHit`
