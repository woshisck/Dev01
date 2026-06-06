# 代码功能目录

> **面向程序**。每个条目描述一个已实现的可复用系统/组件，标注标签和接入方式。  
> 新增系统时必须在此登记。

---

## 标签说明

| 标签 | 含义 |
|---|---|
| `#可复用` | 可以直接用于新角色/新武器/新场景 |
| `#DA配置` | 策划通过 DataAsset 配置，无需改代码 |
| `#蒙太奇驱动` | 通过拖放 AnimNotify 到蒙太奇时间轴激活 |
| `#Subsystem` | 全局单例，任何地方可访问 |
| `#GAS` | 基于 Gameplay Ability System |
| `#符文联动` | 支持符文/BuffFlow 扩展 |
| `#UI` | 纯 UI 组件 |

---

## 战斗手感

### HitStopManager
`#Subsystem` `#可复用` `#蒙太奇驱动`  
**文件**：`AbilitySystem/HitStopManager.h/.cpp`  
**用途**：命中停顿（冻帧）+ 全局时间缩放（慢动作），走实时时钟，TimeDilation=0时仍正常运行。  
**接入**：攻击蒙太奇命中帧放 `AN Hit Stop`，配置 FrozenDuration / SlowDuration / SlowTimeDilation。  
**参考值**：轻攻 F=50ms；重攻 F=80ms/S=120ms@25%；暴击 F=60ms/S=150ms@20%

---

### ANS_AutoTarget
`#可复用` `#蒙太奇驱动` `#DA配置`  
**文件**：`Animation/ANS_AutoTarget.h/.cpp`  
**用途**：攻击区间内自动旋转角色朝向最近活着的敌人，支持锥角过滤和持续跟踪。  
**接入**：攻击蒙太奇挥击区间拖入 `ANS Auto Target`。  
**配置**：SearchRadius（默认 300）/ SearchHalfAngleDeg / bContinuousTracking

---

### ANS_PreAttackFlash
`#可复用` `#蒙太奇驱动`  
**文件**：`Animation/ANS_PreAttackFlash.h/.cpp`  
**用途**：敌人攻击前摇期间触发角色发红光（预警玩家），Begin=StartPreAttackFlash，End=StopPreAttackFlash。  
**接入**：敌人攻击蒙太奇**前摇帧**到命中帧，拖入 `ANS Pre Attack Flash`。  
**前提**：敌人 BP Details → CharacterFlashMaterial 已填入 Overlay 材质

---

### AN_HitStop
`#可复用` `#蒙太奇驱动`  
**文件**：`Animation/AN_HitStop.h/.cpp`  
**用途**：触发 HitStopManager 的冻帧 + 慢动作。  
**接入**：玩家攻击蒙太奇**命中帧**放置，填写参数。

---

### ANS_AttackRotate
`#可复用` `#蒙太奇驱动`  
**文件**：`Animation/ANS_AttackRotate.h/.cpp`  
**用途**：攻击区间内朝摇杆/移动方向持续旋转角色。  
**接入**：攻击蒙太奇挥击区间拖入。

---

## 技能与能力

### SkillChargeComponent
`#可复用` `#DA配置` `#符文联动`  
**文件**：`Component/SkillChargeComponent.h/.cpp`  
**用途**：多段充能 + 冷却，替代 GAS 自带的单段 CD 系统。支持符文修改 CD。  
**接入**：挂在 Character 上，GA 的 CanActivateAbility 调 HasCharge()，ActivateAbility 调 ConsumeCharge()。  
**配置**：MaxCharge（最大充能次数）/ CooldownDuration（单次恢复时间）

---

### GA_PlayerDash
`#GAS` `#可复用`  
**文件**：`AbilitySystem/Abilities/GA_PlayerDash.h/.cpp`  
**用途**：玩家冲刺，含越障（台阶/坡度）、穿透敌人、无敌帧、根运动驱动。  
**调试**：控制台 `Dash.DebugTrace 1` 绘制扫描路径和命中信息。  
**配置**：DashMaxDistance / DashCapsuleRadius / DashMontageRootMotionLength

---

## 背包 / 符文 / 热度

### BackpackGridComponent
`#可复用` `#DA配置` `#符文联动`  
**文件**：`Component/BackpackGridComponent.h/.cpp`  
**用途**：符文背包格管理、4 阶热度计算、激活区动态扩展、符文激活/升级（Lv.I/II/III）、Producer/Consumer 链路传导。  
**接入**：挂在 PlayerCharacterBase 上。  
**文档**：[BackpackSystem_Technical.md](../../99_归档/旧方案/2D背包方案/BackpackSystem_Technical.md)

---

### BuffFlow / FA 系统
`#可复用` `#DA配置` `#符文联动`  
**文件**：`BuffFlow/` 目录  
**用途**：所有 Buff/符文效果的可视化节点图（Flow Asset），无需 C++ 即可组合效果。  
**接入**：创建 FA（Flow Asset），连接 BFNode 节点，在 RuneDA 里引用 FA。  
**文档**：[BuffFlow_ProgrammerGuide.md](Rune/BuffFlow_ProgrammerGuide.md)

---

## 武器系统

### WeaponSpawner / WeaponInstance / WeaponDefinition
`#可复用` `#DA配置`  
**文件**：`Weapon/WeaponSpawner.h/.cpp`、`Weapon/WeaponInstance.h/.cpp`  
**用途**：武器拾取交互、装备、热度发光联动、切关状态恢复。  
**配置**：DA_Weapon_Xxx（WeaponDefinition）填写武器属性。  
**文档**：[WeaponSystem_Technical.md](Weapon/WeaponSystem_Technical.md)

---

### 火绳枪 GA 套件
`#GAS` `#DA配置`  
**文件**：`AbilitySystem/Abilities/Musket/` 目录（6 个 GA）  
**GA 列表**：LightAttack / HeavyAttack / SprintAttack / Reload_Single / Reload_All / SprintReload  
**文档**：[Musket_System_Guide.md](Weapon/Musket_System_Guide.md)

---

## 关卡 / 循环

### LevelFlow（LevelFlowAsset + LevelEventTrigger）
`#可复用` `#DA配置`  
**文件**：`LevelFlow/LevelFlowAsset.h/.cpp`、`LevelFlow/LevelEventTrigger.h/.cpp`  
**用途**：关卡内时间膨胀、延迟执行、引导弹窗按时间轴编排的可视化节点系统。  
**接入**：创建 DA_LevelEvent_Xxx，连节点，放 BP_LevelEventTrigger 在关卡中。  
**文档**：[EditorSetup_LevelFlow_Tutorial_WeaponSpawner.md](../../99_归档/TODO/EditorSetup_LevelFlow_Tutorial_WeaponSpawner.md)

---

### TutorialManager
`#可复用` `#DA配置`  
**文件**：`Tutorial/TutorialManager.h/.cpp`  
**用途**：两段引导（武器拾取后 + 战斗后），存档记录已显示状态，配合 LevelFlow 节点触发。

---

### YogSaveSubsystem
`#Subsystem` `#可复用`  
**文件**：`Subsystem/YogSaveSubsystem.h/.cpp`  
**用途**：HP / 金币 / 背包符文跨关卡持久化存储和恢复。

---

## 特效 / 视觉

### CharacterFlash（YogCharacterBase）
`#可复用`  
**文件**：`Character/YogCharacterBase.h/.cpp`  
**接口**：`StartHitFlash()`（受伤自动调）/ `StartPreAttackFlash()` / `StopPreAttackFlash()`  
**用途**：命中闪白（0.12s 线性淡出）+ 前摇闪红（sin 脉冲）。  
**前提**：角色 BP Details → CharacterFlashMaterial 填入含 FlashColor/FlashAlpha/Power 参数的 Overlay 材质

---

### 热度升阶发光（PlayerCharacterBase + WeaponInstance）
`#DA配置`  
**文件**：`Character/PlayerCharacterBase.h/.cpp`、`Weapon/WeaponInstance.h/.cpp`  
**用途**：热度 Phase1/2/3 触发玩家身体 + 武器发光，颜色从 DA_BackpackStyle 读取。

---

## UI 组件

### GlassFrameWidget
`#UI` `#可复用` `#DA配置`  
**文件**：`UI/GlassFrameWidget.h/.cpp`  
**用途**：液态玻璃框，双层模糊 + SDF 圆角边框 + 底边炫彩。  
**文档**：[GlassFrame_Technical.md](UI/GlassFrame_Technical.md)

---

### AmmoCounter（WBP_AmmoCounter）
`#UI` `#GAS`  
**文件**：`UI/AmmoCounter.h/.cpp`  
**用途**：图标式横排弹药显示，监听 GAS Ammo Attribute 变化自动刷新。

---

### LootSelectionWidget
`#UI` `#可复用`  
**文件**：`UI/LootSelectionWidget.h/.cpp`  
**用途**：三选一符文奖励界面，结束后自动打开背包。

---

## 相机

### YogCameraPawn
`#可复用` `#DA配置`  
**文件**：`Camera/YogCameraPawn.h/.cpp`  
**用途**：6 状态优先级相机（普通/冲刺/战斗/背包/引导/边界），前瞻跟随，多边形边界约束，无 LookAhead。  
**文档**：[Camera_Design.md](Camera/Camera_Design.md)
