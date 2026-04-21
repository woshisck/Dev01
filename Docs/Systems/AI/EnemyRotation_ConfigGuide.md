# 敌人朝向修正配置指南

> 适用范围：敌人行为树 + AnimBP 朝向修正  
> 适用人群：策划 + 程序  
> 配套文档：[行为树攻击任务配置](BT_AttackTask_ConfigGuide.md)

---

## 概述

敌人在攻击前需要面向玩家。本文档说明当前 `BTT_RotateCorrect` 的配置方法与常见错误，以及用转身动画提升视觉流畅度的方案设计（待实现）。

---

## 当前实现：BTT_RotateCorrect

### 工作原理

`BTT_RotateCorrect` 是一个 BT Task，在每次 Tick 中执行：

```
获取 Actor 当前朝向
  └─ Find Look At Rotation（当前位置 → 玩家位置）
       └─ RInterp to Constant（以 Interp Speed °/s 插值）
            └─ Set Actor Rotation
                 └─ 角度差 ≤ 15° → Finish Execute (Success)
```

### 关键配置参数

| 参数 | 推荐值 | 说明 |
|---|---|---|
| `Interp Speed` | `270.0` | 旋转速度（度/秒）。**设为 0 会瞬间 snap** |
| 完成角度阈值 | `15.0°` | 角度差小于此值时 Task 结束，进入下一步 |

> ⚠️ **常见错误：Interp Speed = 0.0**  
> UE 的 `RInterpConstantTo` 在 Speed ≤ 0 时直接返回目标值，导致每 Tick 瞬间 snap 到目标朝向，表现为敌人瞬间转身。将 `Interp Speed` 改为 `270.0` 即可修复。

### CharacterMovement 配套设置

| 属性 | 推荐值 | 说明 |
|---|---|---|
| `Rotation Rate (Yaw)` | `360.0` | 物理旋转上限，配合插值使用 |
| `Orient Rotation to Movement` | ✅ | 移动时朝向移动方向 |
| `Use Controller Desired Rotation` | ❌ | 旋转由 BT Task 手动控制，SetFocus 不介入 |

### 在行为树中的位置

```
Sequence
  ├── BTT_FindPlayerLoc
  ├── BTT_FocusToPlayer
  ├── BTT_RotateCorrect        ← 攻击前朝向修正，完成后进入下一步
  ├── BTT_MoveTo
  ├── Activate Ability By Tag  （攻击 GA）
  ├── Wait
  └── BTT_RemoveEnemyGPTag
```

---

## 方案设计：转身动画平滑旋转（待实现）

### 问题背景

当前 `BTT_RotateCorrect` 修正了物理旋转，角色不再瞬间 snap，但旋转期间没有对应的转身动画，视觉上仍然是原地滑行转身。

### 推荐方案：Blend Space 1D + 偏航角速度驱动

#### 核心原理

角色的偏航角速度（Yaw Angular Velocity）直接反映正在转身的方向和速度。用这个值驱动 Blend Space 1D，自动混合转身动画：

```
YawRate  -270°/s  →  TurnLeft  动画（满速左转）
YawRate    0°/s   →  Idle/Combat 待机动画（不转）
YawRate  +270°/s  →  TurnRight 动画（满速右转）
```

> 类比：Blend Space 就像一个音量推子面板，Left / Idle / Right 是三轨声音，角速度是推子位置，越偏哪边哪边的声音越大。

#### 实现步骤

**Step 1：准备转身动画**

| 动画资产 | 帧数 | 内容 |
|---|---|---|
| `AM_Enemy_TurnLeft` | 10~15 帧 | 向左转 90° 的上半身/全身转动 |
| `AM_Enemy_TurnRight` | 10~15 帧 | 向右转 90° 的上半身/全身转动 |
| `AM_Enemy_Idle_Combat` | 循环 | 战斗待机（中间状态，复用已有资产即可）|

**Step 2：创建 Blend Space 1D**

1. Content Browser → 右键 → `Animation → Blend Space 1D`
2. Axis 配置：
   - Name: `YawRate`
   - Min: `-270.0`，Max: `270.0`
3. 在轴上放置三个动画：

| 轴位置 | 动画 |
|---|---|
| `-270.0` | `AM_Enemy_TurnLeft` |
| `0.0` | `AM_Enemy_Idle_Combat` |
| `+270.0` | `AM_Enemy_TurnRight` |

**Step 3：AnimBP 中计算角速度**

在 AnimBP 的 Event Graph 中，每帧计算偏航角速度并存入变量：

```
Event Blueprint Update Animation
  └─ Get Owning Actor
       └─ GetComponentByClass (CharacterMovementComponent)
            └─ GetAngularVelocity → Get Z
                 └─ SET "YawRate" (float 变量)
```

**Step 4：在 AnimBP State Machine 中使用**

在 `Combat Idle` 状态中，将待机动画替换为 Blend Space，输入 `YawRate` 变量：

```
[Combat Idle State]
  └─ BS_Enemy_Turn (YawRate → AnimBP 中的 YawRate 变量)
```

#### 效果预期

| 场景 | 视觉效果 |
|---|---|
| 正对玩家（差 < 15°） | 播放 Idle 待机，无转身 |
| 小幅转向（15°~90°） | 平滑混入轻微转身动作 |
| 大幅转向（> 90°） | 明显转身动画，旋转完成后自动切回 Idle |

#### 注意事项

| 情况 | 处理方式 |
|---|---|
| 移动中同时转向 | 移动状态使用移动方向 BS，`CombatIdle` 状态才使用转身 BS，两套互不干扰 |
| 攻击中强制追踪 | 攻击蒙太奇播放期间应锁定转身 BS，避免与攻击动画冲突 |
| 转身动画过长 | Blend Space 开启 Loop，让动画持续播放直到转完为止 |
| 没有现成转身动画 | 可暂时复用 Idle 资产（效果差但不会报错），待有专用资产再替换 |

---

## 常见问题

**Q：BTT_RotateCorrect 修正了角度但还是感觉突兀？**  
A：检查 `Interp Speed` 是否 > 0（推荐 270.0）。若已正确设置但仍突兀，说明缺少转身动画，参考上方"转身动画平滑旋转"方案。

**Q：转身 Blend Space 和移动 Blend Space 会冲突吗？**  
A：不会冲突，因为两者在 AnimBP State Machine 的不同状态下使用：移动状态用移动方向 BS，CombatIdle 状态用转身 BS。

**Q：能不能在攻击动画中也加转身追踪？**  
A：可以通过 `AbilityData` 的 `ActRotateSpeed` 字段配置攻击时的追踪速度，但通常建议攻击中锁定朝向（打击感更强），攻击前由 `BTT_RotateCorrect` 修正好方向。

---

## ⚠️ Claude 编写注意事项

- **朝向控制禁止用 SetActorRotation**：敌人攻击朝向通过 `UCharacterMovementComponent::bOrientRotationToMovement` + 移动输入方向控制，不要在 BT Task 里直接调 `SetActorRotation`（会与 CMC 冲突）
- **BTTask 激活 Ability 走 Tag**：`BTTask_ActivateAbilityByTag` 只传 GameplayTag，不持有 GA 指针，Tag 必须是 `Ability.Enemy.*` 命名空间下已注册的 Tag
- **BT 访问 ASC 路径**：从 `UAIController` → `GetPawn()` → `Cast<AYogCharacterBase>` → `GetAbilitySystemComponent()`，不要用 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor`（在 AIController 上调返回 nullptr）
- **EnemyCombo 依赖 UAN_EnemyComboSection**：多段连击的段落衔接通过 `UAN_EnemyComboSection` AnimNotify 触发下一段，BT 只负责启动第一段 GA，后续连击在 GA 内部靠 Notify 推进
