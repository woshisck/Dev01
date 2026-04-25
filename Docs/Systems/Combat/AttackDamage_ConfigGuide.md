# 攻击伤害配置指南

> 适用范围：为玩家或敌人配置近战攻击伤害判定  
> 适用人群：策划  
> 配套文档：[技术文档](../Systems/AttackDamage_Technical.md)  
> 最后更新：2026-04-10（C++ 迁移后新版）

---

## 概述

近战攻击伤害已完全迁移到 C++，策划**无需改任何代码或蓝图逻辑**。配置分为两层：

| 层级 | 配置位置 | 配置内容 | 频率 |
|---|---|---|---|
| **角色层** | 角色父类 Blueprint | 伤害 GE + 目标检测类型 | 每个角色族配置一次 |
| **技能层** | `DA_AbilityData` 资产 | 蒙太奇 + 命中框形状 + 攻击参数 | 每个攻击段单独配置 |

---

## 一、角色父类 Blueprint 配置（每族配置一次）

打开角色父类 Blueprint（如 `B_EnemyBase`、`B_PlayerOne`），在 **Class Defaults** 中找到 **Combat | Melee** 分类：

| 字段 | 敌人填写 | 玩家填写 |
|---|---|---|
| **Default Melee Target Type** | `UYogTargetType_Enemy`（C++ 类） | `UYogTargetType_Player`（C++ 类） |
| **Default Melee Damage Effect** | 敌人伤害 GE（如 `GE_EnemyMelee_Damage`） | 玩家伤害 GE |

> ✅ 配置一次，所有子类 Blueprint 自动继承，无需每个敌人单独配置。

---

## 二、DA_AbilityData 资产配置（每个攻击段）

打开对应角色的 `DA_AbilityData` 资产（如 `DA_Ability_Rat`），在 **Ability Map** 中为每个攻击段填写：

### 基础字段

| 字段 | 说明 | 示例值 |
|---|---|---|
| **Key（Tag）** | 攻击技能的 GameplayTag，与 GA 类的 AbilityTags 一致 | `Enemy.Melee.LAtk1` |
| **Montage** | 该段攻击的动画蒙太奇资产 | `AM_Enemy_Rat_Attack_01` |
| **Act Range** | 攻击范围半径（同时作为命中框的外圆半径） | `400.0` |
| **Act Damage** | 攻击力加成值（传给 GE SetByCaller） | `20.0` |
| **Act Resilience** | 韧性加成值（攻击前摇期间临时提升） | `20.0` |
| **Act Dmg Reduce** | 受伤减免加成值（攻击前摇期间临时） | `0.0` |

### 命中框（Hitbox Types）配置

命中框决定攻击能击中哪个角度/范围的目标。不填则退化为全向球形检测（半径 = Act Range）。

#### 环形扇区（Annulus）— 推荐用于大多数近战

| 字段 | 说明 | 示例 |
|---|---|---|
| **Hitbox Type** | 选择 `Annulus` | — |
| **Inner Radius** | 内圆半径（小于此距离不命中）| `100` |
| **Degree** | 扇形总角度（左右各半）| `45`（即左右各 22.5°）|
| **Offset Degree** | 扇形中心偏转角（0 = 正前方）| `0` |

> 示例：内圆 100、外圆 400（Act Range）、角度 45° = 正前方 45° 扇形，最近 100 单位内免疫。

#### 三角扇面（Triangle）— 用于不规则形状

`Hitbox Triangles` 是一组角度点（Degree），相邻两点构成一个扇形片：
- 至少填 2 个点才能形成有效区域
- 角度以**角色正前方为 0°**，顺时针为正

> 示例：`[-30, 0, 30]` → 正前方左右各 30° 的扇面

---

## 三、蒙太奇配置

在攻击蒙太奇的**攻击判定帧**处添加 AnimNotify，类型选择 **`AN_MeleeDamage`**（C++ 类，无需设置 EventTag）。

> ⚠️ 不要使用旧的 `AN_Dmg_GeneralAttack`（Blueprint 版本），否则 EventTag 不匹配。

### AN_MeleeDamage 可选功能

除攻击参数外，`AN_MeleeDamage` 还支持以下可选配置：

#### HitStop（命中停顿）

| 字段 | 说明 |
|---|---|
| **HitStopMode** | `None`（默认）/ `Freeze` / `Slow` — 选择后对应参数自动显示 |
| **HitStopFrozenDuration** | Freeze 模式：冻结帧时长（默认 0.06s，上限 0.3s） |
| **HitStopSlowDuration** | Slow 模式：延缓帧时长（默认 0.12s，上限 0.5s） |
| **HitStopSlowRate** | Slow 模式：蒙太奇减速倍率（默认 0.3，越小越慢） |
| **HitStopCatchUpRate** | Slow 模式：追帧加速倍率（默认 2.0） |

> 配置 HitStopMode 后无需在 FA 中写 HitStop Tag，AN 自包含触发。详见 [HitStop 设计文档](../../Design/Combat/HitStop_Design.md)。

#### OnHitEventTags（命中事件广播）

| 字段 | 说明 |
|---|---|
| **OnHitEventTags** | 命中目标时向攻击者广播的 GameplayTag 数组 |

典型用途：暴击镜头抖动、击退音效、重击屏幕特效等。下游系统通过 `WaitGameplayEvent` 监听对应 Tag 响应。

#### AdditionalRuneEffects（附加符文效果）

| 字段 | 说明 |
|---|---|
| **AdditionalRuneEffects** | 命中目标时额外触发的 `URuneDataAsset` 列表（施加到目标） |

---

## 四、GASTemplate 配置（将 GA 类赋给角色）

在角色的 `CharacterData` 资产 → `GASTemplate` → `AbilityMap` 中添加对应的 C++ GA 类：

| 角色类型 | 攻击 GA 类 |
|---|---|
| 敌人第 1 段轻攻击 | `GA_Enemy_LAtk1` |
| 敌人第 2 段轻攻击 | `GA_Enemy_LAtk2` |
| 玩家轻攻击第 1 段 | `GA_Player_LightAtk1` |
| 玩家重攻击第 1 段 | `GA_Player_HeavyAtk1` |
| 玩家冲刺攻击 | `GA_Player_DashAtk` |

---

## 完整示例：配置一个鼠类敌人（两段连击）

**步骤 1**：`B_EnemyBase` Class Defaults
```
Default Melee Target Type  = UYogTargetType_Enemy
Default Melee Damage Effect = GE_EnemyMelee_Damage
```

**步骤 2**：`DA_Ability_Rat` → Ability Map

```
[Enemy.Melee.LAtk1]
  Montage       = AM_Enemy_Rat_Attack_01
  Act Range     = 400
  Act Damage    = 20
  Hitbox Types  = [Annulus: inner=100, degree=90, offset=0]

[Enemy.Melee.LAtk2]
  Montage       = AM_Enemy_Rat_Attack_02
  Act Range     = 350
  Act Damage    = 25
  Hitbox Types  = [Annulus: inner=80, degree=120, offset=0]
```

**步骤 3**：蒙太奇 `AM_Enemy_Rat_Attack_01` 攻击帧放 `AN_MeleeDamage`

**步骤 4**：角色 `CharacterData` → `GASTemplate` → AbilityMap 添加 `GA_Enemy_LAtk1`、`GA_Enemy_LAtk2`

---

## 常见问题

**Q：攻击动画播放正常，但目标不掉血？**  
排查顺序：
1. 角色父类 BP 是否设置了 `DefaultMeleeDamageEffect`？
2. `DA_AbilityData` 对应行是否有 `Montage` 填写？
3. 蒙太奇攻击帧是否有 `AN_MeleeDamage` 通知？
4. Output Log 里是否有 `[GA_MeleeAttack] OnEventReceived` 日志？（有=通知触发正常，问题在 GE；没有=通知未触发）

**Q：Debug 攻击范围显示为圆圈，不是扇形？**  
`DA_AbilityData` 对应行的 `Hitbox Types` 未填写，填入 Annulus/Triangle 数据后即可显示正确形状。

**Q：敌人攻击 Debug 显示橙色，玩家显示黄色，正常吗？**  
正常。两种颜色用于区分检测类型（橙色=敌人打玩家，黄色=玩家打敌人）。

**Q：玩家连击打到第几段不触发了？**  
检查蒙太奇的 `CanCombo` AnimNotify 窗口是否正确添加（在攻击帧之后、动画结束之前的时间段）。

**Q：无需改代码就能增加新的攻击 GA 吗？**  
敌人：使用现有 `GA_Enemy_LAtk1~4` / `HAtk1~4`，直接在 AbilityData 里增加攻击行即可。  
玩家：使用现有 `GA_Player_LightAtk1~4` / `HeavyAtk1~4` / `DashAtk`，同上。  
如需新的 GA 类型（如远程攻击），需程序新增 C++ 类。
