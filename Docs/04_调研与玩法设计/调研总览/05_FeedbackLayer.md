# 05 反馈层（视听 + 体感）— 已完成功能盘点

> 范围：命中闪白 / 前摇红 / 升阶发光 / 手柄震动 / 相机系统 / 材质 .ush 体系 / 敌人预生成 FX。
> 玩家攻击的 HitStop 视觉部分本身在战斗模块 → [01_CombatCore.md](01_CombatCore.md)；UI 层暂停效果 → [06_UI_HUD.md](06_UI_HUD.md)。

---

## 功能总览

| 功能名称 | 需求简介 | 完成状态（代码）| 完成状态（编辑器）|
|---|---|---|---|
| 命中闪白 HitFlash (VFX-001) | 受击瞬间发白光 | ✅ 编译通过 | ✅ 自动触发 |
| 攻击前摇闪红 PreAttackFlash + ANS (VFX-002 / 003) | 敌人前摇预警 | ✅ 编译通过 | ⚙ 敌人攻击蒙太奇拖 ANS（VFX-004）+ 敌人 BP 填 CharacterFlashMaterial（VFX-006）|
| 热度升阶发光（玩家身体 + 武器 + 玻璃边缘炫彩）(VFX-004 / 005) | 全身亮起 + 神秘炫彩边缘 | ✅ 编译通过 | ✅ 材质 / 参数已配 |
| 霸体金光（视觉副）(COMBAT-008 副) | 6Hz 金黄正弦脉冲 | ✅ 编译通过 | ⚙ DA SuperArmor 参数（与 ENEMY-DA-002 同批）|
| 敌人预生成粒子 PreSpawnFX (FEAT-028) | 出生前播警示 FX | ✅ 编译通过 | ⚙ DA_Enemy_*.PreSpawnFX 待填（ENEMY-DA-001）|
| 热度升阶手柄震动 (FEEL-001) | 升阶瞬间震动 | ✅ 编译通过 | ✅ B_PlayerOne 已赋 FFE |
| 相机 6 状态系统 YogCameraPawn (CAM-001) | 俯视角 ARPG 镜头 + 边界约束 + 命中震动 | ✅ 编译通过 | ✅ |
| 相机平滑（VInterpTo 起点用 GetCameraLocation）(CAM-002) | 攻击 / 冲刺不僵硬 + LookAhead 可关 | ✅ 编译通过 | — 参数在 BP 调 |
| 移除鼠标偏移 + 手柄右摇杆 (CAM-003) | 不读鼠标位置 + IA_CameraLook | ✅ 编译通过 | ✅ IMC 已配 |
| 材质 .ush 体系（/Project 注册）| HLSL 函数复用 + 跨电脑可 include | ✅ 编译通过 | ✅ |

> 状态口径：✅ 完成 / ⚙ 待配置（注明待办）/ — 不涉及编辑器配置

---

## 角色身体特效

### [VFX-001] 命中闪白（HitFlash）— 自动触发
- **设计需求**：受击瞬间角色发白光，明确反馈"我被打到了"。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Character/YogCharacterBase.h` + `Private/Character/YogCharacterBase.cpp`（`StartHitFlash` / `HitFlashDuration` 默认 0.12s）
  - `HealthChanged` 血量减少时自动调用
- **设计文档**：[CharacterFlash_Technical](../../01_长期系统文档/系统/VFX/CharacterFlash_Technical.md)
- **验收方式**：玩家 / 敌人受击时身体应短暂闪白；只闪一次不会卡住

### [VFX-002 / VFX-003] 攻击前摇闪红（PreAttackFlash + ANS）
- **设计需求**：敌人攻击前摇要预警（脉冲红光），玩家有反应窗口；蒙太奇方便配置（拖 ANS 区间即可）。
- **状态**：✅ C++完成；⚙ 敌人攻击蒙太奇前摇帧拖入 ANS（VFX-004）+ 敌人 BP 填 CharacterFlashMaterial（VFX-006）
- **核心文件**：
  - `Public/Character/YogCharacterBase.h`（`StartPreAttackFlash` / `StopPreAttackFlash`）
  - `Public/Animation/ANS_PreAttackFlash.h` + `Private/Animation/ANS_PreAttackFlash.cpp`
  - `PreAttackPulseFreq` 默认 4Hz
- **设计文档**：[CharacterFlash_Technical](../../01_长期系统文档/系统/VFX/CharacterFlash_Technical.md)
- **验收方式**：敌人攻击前摇期间应红光脉冲；攻击发出 / 被打断后红光停

### [VFX-004 / VFX-005] 热度升阶发光（玩家身体 + 武器 + 玻璃边缘菲涅尔炫彩）
- **设计需求**：升阶有"全身亮起"的强反馈 — 时序：扫射 → 保持 → 淡出；边缘菲涅尔区出现神秘炫彩，alpha 高于中心，随法线方向 + 时间缓慢流动。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Character/PlayerCharacterBase.h` + `Private/Character/PlayerCharacterBase.cpp`（`PhaseUpPlayerOverlayMaterial` / `GlowSweepDuration` / `GlowHoldDuration` / `GlowFadeDuration` / `GlowIridIntensity` 默认 0.28）
  - `Public/Item/Weapon/WeaponInstance.h` + `.cpp`（武器同步）
  - `Source/DevKit/Shaders/PlayerGlowOverlay.ush`
  - `Source/DevKit/Shaders/WeaponGlowOverlay.ush`
  - `Source/DevKit/Shaders/GlassRim.ush`（`GR_HueToRGB` / `GR_Iridescence` / `GR_Fresnel` 共享函数）
  - `Public/DevKit.h` + `Private/DevKit.cpp`（`FDevKitModule::StartupModule` 调 `AddShaderSourceDirectoryMapping("/Project", Shaders/)` 让 .ush 跨电脑可用）
- **设计文档**：[CharacterFlash_Technical](../../01_长期系统文档/系统/VFX/CharacterFlash_Technical.md) · [Material_Authoring_Guide](../../01_长期系统文档/系统/VFX/Material_Authoring_Guide.md)
- **验收方式**：
  1. 升 Phase 1 玩家身体应扫射白光 → 保持 → 淡出
  2. 武器同步 Phase 颜色（白 / 绿 / 橙黄 / 过热红）

### [COMBAT-008 副] 霸体金光
> 主条目在 [01_CombatCore.md](01_CombatCore.md) 的 [COMBAT-008]，此处只列视觉
- 连续受击 ≥ Threshold → 6Hz 金黄正弦脉冲（`FLinearColor(5,3,0)`）
- 优先级：命中白闪 > 霸体金光 > 攻击前红光
- 配 `SuperArmorPulseFreq`（蓝图 Details）/ `SuperArmorThreshold` / `SuperArmorDuration`（DA）

### [FEAT-028] 敌人预生成粒子（PreSpawnFX）
- **设计需求**：敌人出生前在地面播一段警示 FX，给玩家心理准备时间。
- **状态**：✅ C++完成；⚙ DA 待填（ENEMY-DA-001 — 给所有 DA_Enemy_* 填 `PreSpawnFX` Niagara + `PreSpawnFXDuration`）
- **核心文件**：
  - `Public/Data/EnemyData.h`（`PreSpawnFX` / `PreSpawnFXDuration`）
  - `Private/Character/EnemyCharacterBase.cpp`（自动播放）
- **验收方式**：敌人即将出生位置应先播 0.8~1.5s Niagara 警示，结束后才出现敌人

---

## 体感反馈

### [FEEL-001] 热度升阶手柄震动
- **设计需求**：升阶瞬间震一下，体感强化"一个阶段过去了"。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Character/PlayerCharacterBase.h` + `Private/Character/PlayerCharacterBase.cpp`（`ClientPlayForceFeedback(PhaseUpForceFeedback)`，监听 `Buff.Status.Heat.Phase.1/2/3` Tag 新增）
  - 资产：`Content/Code/Core/ForceFeedbackEffect/FFE_HeatPhaseUp` + `FFE_HeatPhaseUp_ExternalCurve`
- **配置入口**：`B_PlayerOne` Details → Heat\|Feedback → `Phase Up Force Feedback`
- **验收方式**：用手柄玩，升 Phase 1 / 2 / 3 时手柄应震动

---

## 相机系统（俯视角 ARPG 镜头）

### [CAM-001] YogCameraPawn 6 状态相机系统
- **设计需求**：俯视角 ARPG 相机要应对多种状态：Dash / 战斗聚焦 / 战斗搜索 / 拾取聚焦 / 前瞻 / 角色聚焦；关卡有边界约束；命中可触发震动。
- **状态**：✅ C++完成；⚙ `BP_YogPlayerController` Details `Camera Pawn Class = BP_YogCameraPawn`
- **核心文件**：
  - `Public/Camera/YogCameraPawn.h` + `Private/Camera/YogCameraPawn.cpp`（`EYogCameraStates`：Dash / CombatFocus / CombatSearch / PickupFocus / LookAhead / FocusCharacter）
  - `Public/Camera/CameraConstraintActor.h` + `Private/Camera/CameraConstraintActor.cpp`（多边形顶点约束）
  - `Public/Camera/YogPlayerCameraManager.h` + `.cpp`
- **设计文档**：[Camera_Design](../../01_长期系统文档/系统/Camera/Camera_Design.md)
- **验收方式**：
  1. 冲刺时相机切到 Dash 状态（更高跟随速度）
  2. 走到关卡边界，相机应被 ConstraintActor 多边形限制
  3. 调 `GetOwnCamera->NotifyHeavyHit()` 应触发屏幕震动

### [CAM-002] 相机平滑（VInterpTo 起点用 GetCameraLocation）
- **设计需求**：攻击 / 冲刺时相机不僵硬 snap；可关 LookAhead 消除眩晕；不同状态用不同跟随速度。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/Camera/YogPlayerCameraManager.cpp`（VInterpTo 起点改为 `GetCameraLocation()`，不能用 `OutVT.POV.Location`）
  - 参数：`bEnableLookAhead`（默认 false）/ `MovingFollowSpeed=8` / `StationarySettleSpeed=5` / `DashFollowSpeed=18`
- **设计文档**：memory: `feedback_ue5_camera_code.md`（**待转写**为 `Docs/Conventions/Camera.md`）
- **验收方式**：攻击 / 冲刺时相机应平滑跟随而非瞬移；LookAhead 关闭时应无前冲 / 回弹

### [CAM-003] 移除鼠标偏移 + 手柄右摇杆生效
- **设计需求**：不要鼠标偏移视角；右摇杆控制视角；背包键加手柄按键映射。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/Camera/YogPlayerCameraManager.cpp`（删除 `bAutoReadMouseOffset` 与读鼠标位置逻辑）
  - `Content/Input/IMC_YogPlayerBase.uasset`（`IA_CameraLook` Axis2D 绑 Gamepad Right Thumbstick；`IA_OpenBackpack` 加 Gamepad Special Left）
- **验收方式**：手柄右摇杆应转动视角；鼠标移动应不影响相机偏移

---

## 材质 .ush 体系（VFX 底层）

### [材质 .ush 体系] Custom Node + .ush 跨电脑可 include
- **设计需求**：复用 HLSL 函数（菲涅尔 / 炫彩 / SDF）；团队成员路径一致 — 不能依赖各自盘符。
- **状态**：✅ C++完成
- **核心文件**：
  - `Source/DevKit/Public/DevKit.h` + `Private/DevKit.cpp`（`FDevKitModule::StartupModule` 注册 `/Project` → `Source/DevKit/Shaders/`）
  - .ush 清单：`GlassRim.ush` / `PlayerGlowOverlay.ush` / `WeaponGlowOverlay.ush` / `GlassFrameUI.ush` / `GlassBlurMask.ush` / `WeaponTrail.ush` / `LiquidHealthBar.ush` / `LevelEndReveal.ush`（位于 `Source/DevKit/Shaders/`）
- **设计文档**：[Material_Authoring_Guide](../../01_长期系统文档/系统/VFX/Material_Authoring_Guide.md)（详细规范）· memory: `reference_material_authoring.md`（已有正式文档不需转写）
- **验收方式**：新建 Material → Custom Node → Include File Paths 填 `/Project/GlassRim.ush` → 编译应通过

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| ENEMY-DA-001 | 所有 `DA_Enemy_*` → Enemy\|Spawn → 填 `PreSpawnFX` (NS_EnemySpawn) + `PreSpawnFXDuration`(0.8~1.5s) | [TASKS](../../05_项目管理与进度/PM/TASKS.md) |
| VFX-006 | 所有敌人 BP Details → Combat\|Visual → `CharacterFlashMaterial` 填入 Overlay 材质 | [CharacterFlash_Technical](../../01_长期系统文档/系统/VFX/CharacterFlash_Technical.md) |
| VFX-004 | 所有敌人攻击蒙太奇前摇帧拖入 `ANS Pre Attack Flash` 区间 | 同上 |
