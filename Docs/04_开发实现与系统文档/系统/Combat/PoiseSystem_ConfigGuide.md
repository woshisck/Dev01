# 韧性系统配置指南 — Poise System

> 适用范围：战斗手感调整，控制受击频率与霸体节奏  
> 适用人群：策划 + 程序  
> 相关文档：[攻击伤害设计文档](../../../00_入口与规范/缺失引用记录.md) · [AttackDamage 技术文档](AttackDamage_Technical.md)  
> 更新：2026-04-17

---

## 功能概述

韧性系统控制"一次攻击能否打出受击动画"。核心规则：

> **攻击方韧性 > 防御方韧性 → 触发受击；否则被防御方硬抗。**

攻击方韧性 = `Resilience 属性` + `动作韧性（ActResilience）`  
防御方韧性 = `Resilience 属性`（可因连续受击临时叠加）

非玩家角色连续被打出受击 N 次后进入**霸体**，短暂免疫受击动画。

---

## 配置项速览

| 配置项 | 位置 | 说明 |
| --- | --- | --- |
| `Resilience` 初始值 | 角色属性数据表 | 角色基础韧性，越高越难被打出受击 |
| `AN_MeleeDamage.ActResilience` | 蒙太奇 Notify | 本次攻击的动作韧性加成 |
| `SuperArmorThreshold` | 敌人蓝图 ASC CDO | 连续受击多少次后进入霸体（默认 3） |
| `SuperArmorDuration` | 敌人蓝图 ASC CDO | 霸体持续时间（默认 2.0s） |

---

## 一、Resilience 属性（基础韧性）

在角色属性数据表（`CharacterData` → `BaseAttributeData`）的 `Resilience` 字段填写初始值。

推荐基准值：

| 角色类型 | 推荐值 | 效果 |
| --- | --- | --- |
| 玩家 | 100 | 基准，轻击（20+100=120）可对普通敌打出受击 |
| 普通敌人 | 50 | 被玩家轻击即触发受击 |
| 精英敌人 | 120 | 需要重击（ActResilience 50+100=150 > 120）才触发受击 |
| BOSS | 200 | 玩家轻击/重击均无法打出受击，需配合特殊状态 |

---

## 二、动作韧性（ActResilience）

在蒙太奇的 `AN_MeleeDamage` Notify 上设置 `Act Resilience` 字段，表示"这一刀挥出时攻击者额外的韧性"。

| 攻击类型 | 推荐值 | 备注 |
| --- | --- | --- |
| 轻击 | 20（默认） | 配合玩家 Resilience=100，合计 120 |
| 重击 / 终结技 | 50 | 合计 150，可打穿精英敌人 |
| 符文追加打击 | 0~30 | 视效果强度决定 |

`GA_MeleeAttack::OnEventReceived` 在命中 Notify 触发时自动写入 `CurrentActionPoiseBonus`，`ReceiveDamage` 读取后立即清零，无需手动维护。

---

## 三、霸体配置（SuperArmor）

霸体仅对**非玩家角色**生效，在敌人蓝图的 `YogAbilitySystemComponent` 组件 CDO 上配置：

| 属性 | 默认值 | 说明 |
| --- | --- | --- |
| `Super Armor Threshold` | 3 | 连续 N 次受击后进入霸体 |
| `Super Armor Duration` | 2.0s | 霸体持续时间，到期自动解除 |

霸体期间 ASC 持有 `Buff.Status.SuperArmor` Tag，可在蓝图/FA 中监听该 Tag 做视觉反馈（如发光、震动等）。

计数规则：
- 每次成功触发受击动画 → `PoiseHitCount++`
- 5 秒内无新受击 → `PoiseHitCount` 归零
- 达到阈值 → 添加霸体 Tag + 计数归零 + 开始倒计时

---

## 四、配置示例

### 普通敌人（容易被打出受击，3 次后短暂霸体）

```
角色数据表 Resilience     = 50
AN_MeleeDamage ActResilience（轻击）= 20
SuperArmorThreshold       = 3
SuperArmorDuration         = 2.0s
```

效果：玩家轻击（20+100=120 > 50）每次都打出受击；连续 3 次后霸体 2 秒。

### 精英敌人（轻击无效，重击才触发）

```
角色数据表 Resilience     = 120
AN_MeleeDamage ActResilience（轻击）= 20  → 120 ≤ 120，不触发受击
AN_MeleeDamage ActResilience（重击）= 50  → 150 > 120，触发受击
SuperArmorThreshold       = 5
SuperArmorDuration         = 3.0s
```

---

## 五、运行时调试

| 方式 | 说明 |
| --- | --- |
| Output Log `[Poise]` 前缀 | PoiseHitCount 重置、SuperArmor 触发/过期均有日志 |
| `Buff.Status.SuperArmor` Tag | 可在蓝图 `HasMatchingGameplayTag` 判断霸体状态 |
| `CurrentActionPoiseBonus` | 可在 YogASC 上 `BlueprintReadWrite`，PIE 中 Watch Variables 观察 |

---

## 六、当前限制

- 敌人 GA 尚未写入 `CurrentActionPoiseBonus`（敌人攻击玩家时动作韧性为 0），后续扩展敌人 GA 时自行设置
- 玩家无霸体逻辑（被连击不触发霸体），如需扩展去掉 `ReceiveDamage` 中的 `!IsPlayerControlled()` 判断
