# 战斗系统 — Combat

> 攻击判定、连击框架、闪避、架势（Poise）、技能蓄力、终结技。

## 架构说明 & 使用指南

| 文档 | 内容 |
| --- | --- |
| [AttackDamage_ConfigGuide.md](AttackDamage_ConfigGuide.md) | 攻击伤害配置 |
| [EnemyCombo_ConfigGuide.md](EnemyCombo_ConfigGuide.md) | 敌人连击编辑器配置 |
| [MeleeCombo_NotifyRune_Guide.md](MeleeCombo_NotifyRune_Guide.md) | 近战 AnimNotify 钩子 + 符文联动 |
| [Dash_Design.md](Dash_Design.md) | 闪避系统设计与碰撞通道 |
| [PoiseSystem_ConfigGuide.md](PoiseSystem_ConfigGuide.md) | 架势（韧性）系统配置 |
| [SkillCharge_Guide.md](SkillCharge_Guide.md) | 技能蓄力配置 |
| [FinisherCard_*.md](.) | 终结技卡相关文档 |

## 编码前必读

- [../../编码规范/GAS.md](../../编码规范/GAS.md) — GA/GE/AttributeSet 规范
- [../../编码规范/AnimNotify.md](../../编码规范/AnimNotify.md) — AN/ANS 类规范

## 关联系统

- [../Rune/](../Rune/) — Buff 效果通过 FlowGraph 实现，战斗 AN 触发符文钩子
- [../Weapon/WeaponSystem_Technical.md](../Weapon/WeaponSystem_Technical.md) — 武器参数层
- [../../标签/GA_TagFields_Guide.md](../../标签/GA_TagFields_Guide.md) — GA 的 5 字段 Tag 配置规则
