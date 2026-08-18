# 相机震动系统说明（Camera Shake）

> 适用范围：DevKit 战斗命中反馈中的相机震动全链路（数据、代码、资产、配置）
> 适用人群：策划 / 程序
> 配套文档：[相机管理系统设计文档](../04_开发实现与系统文档/系统/Camera/Camera_Design.md)、[命中音效分层配置说明](../03_策划配置与制作手册/核心配置说明/战斗/命中音效分层配置说明.md)
> 最后更新：2026-08-18

---

## 概述

项目中同时存在 **3 套相机震动实现**，彼此独立，只有 1 套真正在游戏里生效，且当前只在双手剑命中时触发。

本文档记录三套系统的完整链路、现有资产参数、策划配置入口，以及需要修复的已知问题。

---

## 1. 三套系统总览

| # | 系统 | 入口 | 当前状态 |
| --- | --- | --- | --- |
| A | 等级表（Level Table） | `AYogPlayerCameraManager::PlayShakeLevel` | ✅ 生效，但仅双手剑命中时触发 |
| B | 全局伤害缩放（Global Fallback） | `UYogSettings::HitShakeConfig` | ❌ 未配置，代码分支永远拿到 `nullptr` |
| C | GameplayCue | `GameplayCue.Character.CameraShake` | ❌ 资产齐全但无人触发，且震源写死世界原点 |

> 结论：目前**没有任何全局兜底的相机震动**。除双手剑外，所有命中都是零震动。

---

## 2. 系统 A：等级表（当前主路径）

### 2.1 链路

```
GA_MeleeAttack（受击方来源）          GCN_PlayerHitImpact（攻击方来源）
        │                                      │
        └──────────────┬───────────────────────┘
                       │  int32 Level
                       ▼
   AYogPlayerCameraManager::PlayShakeLevel(Level, ScaleMultiplier)
                       │
                       │  读 UYogSettings::CameraShakeLevelTable
                       ▼
        DT_CameraShakeLevels  →  遍历找 Row.Level == Level
                       │
                       │  找不到 → 静默 return（无日志、无警告）
                       ▼
   StartCameraShake(Row.ShakeClass, Row.Scale * ScaleMultiplier)
```

### 2.2 关键代码位置

| 位置 | 作用 |
| --- | --- |
| `YogPlayerCameraManager.cpp:48-69` | `PlayShakeLevel` 查表并播放 |
| `CameraShakeLevelRow.h:17-30` | `FCameraShakeLevelRow` 结构（`Level` / `ShakeClass` / `Scale`） |
| `YogSettings.h:27-30` | `CameraShakeLevelTable` 配置项声明 |
| `DefaultGame.ini:152` | 指向 `/Game/Data/Camera/DT_CameraShakeLevels` |

### 2.3 DT_CameraShakeLevels 现有内容

| RowName | Level | ShakeClass | Scale |
| --- | --- | --- | --- |
| HeavyAttack | 1 | `B_CameraShakeBase` | 1.0 |
| CriticalHit | 2 | `B_CameraShake_UltraSword` | 1.5 |

> ⚠️ **`Level` 是行键，不是强度值。** `PlayShakeLevel`（`YogPlayerCameraManager.cpp:61-68`）遍历所有行寻找 `Row.Level == Level`，找不到就直接 `return`——不报错、不打日志。填 `3` 的效果和填 `0` 完全一样，排查时极易误判。

### 2.4 两个触发来源

| 来源 | 代码位置 | 数据资产 | 作用域 |
| --- | --- | --- | --- |
| 攻击方（武器） | `GCN_PlayerHitImpact.cpp:38-56` | `UHitCueData`（`DA_HitImpact_THSword`） | 每次挥击一次 |
| 受击方（敌人） | `GA_MeleeAttack.cpp:1731-1753` | `UEnemyHitImpactData`（`DA_EnemyHitImpact`） | 取所有受击者的最大值 |

暴击提升：`GCN_PlayerHitImpact` 通过 `Ability.Event.Attack.CritHit` 标签判定暴击，把 `CameraShakeLevel` 换成 `CritCameraShakeLevel`（`GCN_PlayerHitImpact.cpp:41-45`）。

> ⚠️ **两条来源都调用 `PlayShakeLevel`，同一帧可以叠加。** 三个 Shake 蓝图的 `bSingleInstance` 都是 `false`，所以两次 `StartCameraShake` 的位移会相加。若要同时启用两侧配置，需先确定由哪一侧独占。

---

## 3. 系统 B：全局伤害缩放（未启用）

设计意图：命中未配置离散等级时，按本次挥击造成的最终 HP 伤害曲线缩放震动幅度，伤害越高震动越大。

| 项目 | 内容 |
| --- | --- |
| 数据类 | `UGlobalHitShakeData`（`GlobalHitShakeData.h:17-45`） |
| 字段 | `CameraShakeClass`、`CameraShakeScale`、`DamageToShakeScale`（`FRuntimeFloatCurve`） |
| 求值 | `ResolveShakeScale(Damage) = CameraShakeScale * Curve.Eval(Damage)`，曲线为空时取 1 |
| 配置项 | `UYogSettings::HitShakeConfig`（`YogSettings.h:23-25`） |
| 调用点 | `GCN_PlayerHitImpact.cpp:57-67`，仅当 `ShakeLevel == 0` 时进入 |

**当前状态：完全未启用。**

- `Config/` 下任何 ini 都没有 `HitShakeConfig=` 这一行
- `Content/` 里不存在任何 `UGlobalHitShakeData` 资产

**后果**：`GCN_PlayerHitImpact.cpp:60` 的 `LoadSynchronous()` 永远返回 `nullptr`，`else` 分支等于空实现。**任何 `CameraShakeLevel == 0` 的命中不是"震动很弱"，而是完全没有震动。**

---

## 4. 系统 C：GameplayCue（已构建但未接通）

### 4.1 链路

```
GE_CameraShake（Instant GE）
   gameplayCues[0].gameplayCueTags = [ GameplayCue.Character.CameraShake ]
        │
        │  标签 → Notify（扫描路径 DefaultGame.ini:44
        │                 = /Game/Code/GAS/GameplayCueNotifies，资产在此路径下 ✓）
        ▼
GCN_CameraShake（AGameplayCueNotify_Actor 蓝图）
   OnExecute:
     PlayWorldCameraShake(
        WorldContext = GetSourceObjectFromGameplayCueParameters(Parameters),
        Shake        = B_GreatSwordShake_C,
        Epicenter    = (0, 0, 0),      ← 写死世界原点
        InnerRadius  = 150,
        OuterRadius  = 150,
        Falloff      = 0)
     return false
```

标签声明位置：`Config/Tags/GameplayCueTag.ini:34`。

### 4.2 三个独立失效原因

| # | 问题 | 说明 |
| --- | --- | --- |
| 1 | 无人应用 `GE_CameraShake` | `Content/` 与 `Source/` 中除自身外零引用——没有任何 GA、BuffFlow 节点或 Montage Notify 应用它，Cue 永不触发 |
| 2 | 震源写死 `(0, 0, 0)` | `PlayWorldCameraShake` 是**半径型**，只影响距震源 `OuterRadius` 内的相机。这里是世界原点而非角色或命中点，半径仅 150uu——玩家必须站在世界原点才感受得到 |
| 3 | 旋转被清零 | `B_GreatSwordShake` 的 `rotationAmplitudeMultiplier = 0`，见第 5 节 |

> 备注：`B_GreatSwordShake`、`GE_CameraShake`、`GCN_CameraShake` 三者互相引用，构成完整闭环，**不是孤立废弃资产**。缺的只是"谁来应用这个 GE"。

---

## 5. Shake 资产参数对照

| 参数 | `B_CameraShakeBase` | `B_CameraShake_UltraSword` | `B_GreatSwordShake` |
| --- | --- | --- | --- |
| Pattern 类型 | Perlin Noise | Perlin Noise | **Wave Oscillator** |
| 使用方 | DT 等级 1 | DT 等级 2 | GCN_CameraShake |
| Loc X（幅度/频率） | 1 / 1 | 2 / 10 | 20 / 30 |
| Loc Y | 1 / 1 | 1 / 10 | 20 / 30 |
| Loc Z | 1 / 1 | 2 / 10 | 10 / 30 |
| Location 幅度倍率 | 1 | 1 | 1 |
| **Rotation 幅度倍率** | **0** | **0** | **0** |
| Pitch | 1 / 1 | 10 / 12 | 2 / 20 |
| Yaw | 1 / 1 | 1 / 9 | 2 / 20 |
| Roll | 1 / 1 | 0.6 / 7 | 5 / 20 |
| FOV | 0 | 2.5 / 8 | 0 |
| Duration | 1.0 s | 0.142 s | 2.0 s |
| BlendIn / BlendOut | 0.2 / 0.2 | 0 / 0.2 | 0.05 / 0.05 |
| bSingleInstance | false | false | false |

### 5.1 参数层面的问题

| # | 问题 | 影响 |
| --- | --- | --- |
| 1 | **三个资产的 `rotationAmplitudeMultiplier` 全为 0** | 所有已配置的 Pitch / Yaw / Roll 被整体清零。`UltraSword` 的 Pitch 幅度 10、`GreatSword` 的 Roll 幅度 5 都是白配。这是"震动感觉不到"的最主要原因，每个资产只需改 1 个值 |
| 2 | `B_CameraShakeBase` 是未调参的占位值 | 幅度和频率全为 1、时长 1 秒，即 1uu 位移以 1Hz 抖动，人眼不可见 |
| 3 | `B_CameraShake_UltraSword` 的 BlendOut > Duration | Duration 0.142s 但 BlendOut 0.2s，淡出时间长于总时长 |

---

## 6. 策划配置指南

### 6.1 受击方：`DA_EnemyHitImpact`

资产路径：`/Game/Data/Combat/DA_EnemyHitImpact`（已由 `DefaultGame.ini:153` 接入，无需再指向）。

字段位置（声明于 `EnemyHitImpactData.h:59-61`）：

```
DA_EnemyHitImpact
└─ Hit Impact
   └─ Entries
      └─ [0]   Match Tag = HitReact.Material.Soft, Priority = 10
         ├─ Match Tag
         ├─ Priority
         ├─ Levels
         │   └─ [0]  Sound = S_FleshCut, VFX = None
         └─ Camera Shake Level   ← 在这里填，当前为 0
```

可填值：

| 值 | 结果 |
| --- | --- |
| 0 | 不产生震动（当前值） |
| 1 | `B_CameraShakeBase`，Scale 1.0 |
| 2 | `B_CameraShake_UltraSword`，Scale 1.5 |
| 3 及以上 | **静默无效**，需先在 `DT_CameraShakeLevels` 补行 |

**两个实用规则：**

1. **可以配置"只有震动"的条目。** 震动读取（`EnemyHitImpactData.cpp:37-40`）发生在 `Levels.Num() == 0` 的提前 `continue`（`:42-45`）**之前**。所以可以新增一个只填 Match Tag、Priority、Camera Shake Level，`Levels` 留空的条目——它只贡献震动，不带 VFX / SFX。给重甲、金属类敌人加重震动时不必重复配一遍特效音效。

2. **级联规则是"按 Priority 降序取第一个非零值"**（`if (Result.CameraShakeLevel == 0)`），同 Priority 按数组顺序（`StableSort`）。所以高优先级的状态条目（如 `Buff.Status.*`）可以覆盖材质条目的震动等级，与 `bSoundSet` / `bVFXSet` 是同一套模式。

### 6.2 攻击方：`UHitCueData`

当前项目中唯一的 `UHitCueData` 资产是 `/Game/Code/Weapon/TwoHandedSword/DA_HitImpact_THSword`。

| 字段 | 当前值 | 说明 |
| --- | --- | --- |
| `CameraShakeLevel` | 1 | 普通命中的等级 |
| `CritCameraShakeLevel` | 2 | 暴击时替换上面的值；填 0 表示沿用 `CameraShakeLevel` |

字段声明：`HitCueData.h:25-33`。资产通过 Montage Notify 的 `HitImpactCueData` 字段传入（`AN_MeleeDamage.h:182`、`YogAnimNotifyState_Damage.h:106`）。

### 6.3 新增震动等级

1. 在 `Content/Code/GAS/CameraShake/` 下新建 CameraShake 蓝图，配好 Pattern 参数（**记得把 `Rotation Amplitude Multiplier` 设为非 0**）
2. 打开 `DT_CameraShakeLevels`，新增一行：`Level` 填新的整数、`ShakeClass` 指向新蓝图、`Scale` 填基础倍率
3. 在 `DA_EnemyHitImpact` 或 `UHitCueData` 中填入该 `Level`

---

## 7. 数据流全景

```
Montage Notify (HitImpactCueData: UHitCueData)
        │
        ▼
GA_MeleeAttack
        │  WeaponDef->HitImpactLevel  （GA_MeleeAttack.cpp:1728）
        ▼
UHitImpactVisualComponent::PlayHitFeedback(HitLocation, Level)
        │                                （HitImpactVisualComponent.cpp:108）
        ▼
UEnemyHitImpactData::Resolve(VictimTags, Level)   （EnemyHitImpactData.cpp:8-68）
        │
        ├─ Levels[Clamp(Level-1, ...)] → Sound / VFX   ← 随武器等级变化
        └─ Entry->CameraShakeLevel      → 震动等级     ← 不随武器等级变化
        │
        ▼  返回 CameraShakeLevel（HitImpactVisualComponent.cpp:137）
GA_MeleeAttack 取所有受击者 max() → PlayShakeLevel  （GA_MeleeAttack.cpp:1738-1753）
```

> ⚠️ **结构不对称：** `Sound` / `VFX` 位于 `FHitImpactFX`（`EnemyHitImpactData.h:15-36`）、随 `Levels` 数组按武器 `HitImpactLevel` 取值；而 `CameraShakeLevel` 位于外层的 `FHitImpactEntry`（`:43-62`）。因此**轻武器和重武器打同一个敌人，特效和音效不同，但震动完全相同**。若要让震动成为 VFX / SFX 的真正对等项，需要把该字段下移进 `FHitImpactFX`。

---

## 8. 已知问题清单

| # | 问题 | 位置 | 影响 |
| --- | --- | --- | --- |
| 1 | 三个 Shake 资产 `rotationAmplitudeMultiplier` 均为 0 | 三个 Shake 蓝图 | 所有旋转震动被清零，是"感觉不到震动"的首因 |
| 2 | `HitShakeConfig` 未配置且资产不存在 | `DefaultGame.ini` / `Content/` | 全局伤害缩放兜底完全失效，等级为 0 的命中零震动 |
| 3 | `DA_EnemyHitImpact` 唯一条目 `cameraShakeLevel = 0` | `DA_EnemyHitImpact` | 受击方路径从不产生震动 |
| 4 | 除双手剑外没有任何 `UHitCueData` 资产 | `Content/` | 其余武器命中零震动 |
| 5 | `GCN_CameraShake` 震源写死 `(0,0,0)`、半径 150 | `GCN_CameraShake` OnExecute | 即便接通 GE 也几乎不可能触发 |
| 6 | 无人应用 `GE_CameraShake` | — | 系统 C 整体空转 |
| 7 | `B_CameraShakeBase` 参数为占位默认值 | 该蓝图 | 等级 1 的震动人眼不可见 |
| 8 | 震动等级填错不报错 | `YogPlayerCameraManager.cpp:61-68` | 查不到行时静默 return，排查困难 |
| 9 | 双侧配置会叠加 | `GA_MeleeAttack` + `GCN_PlayerHitImpact` | 同帧两次 `StartCameraShake`，位移相加 |

---

## 9. 文档同步提醒

`Camera_Design.md` 第 9 节与第 10.4 节仍在描述 `HeavyHitShakeClass`、`CritHitShakeClass` 与 `NotifyHeavyHit()` 三个 API——**这些在代码中均已不存在**，早已被本文档描述的 DataTable 等级表方案取代。修改相机震动时请以本文档为准，并在方案确定后回头更新 `Camera_Design.md`。
