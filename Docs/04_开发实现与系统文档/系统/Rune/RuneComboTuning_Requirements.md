# Rune Editor 旧连招奖励需求废弃说明

更新时间：2026-06-22

## 当前状态

旧“ComboIndex 驱动符文数值奖励”方案已废弃。当前玩家没有轻/重攻击连招主流程，卡牌构筑强度由动作槽、卡牌顺序、FlowRole、Link Recipe 和 Passive Flow 表达。

## 废弃字段

- `CombatCard.bUseComboEffectScaling`
- `CombatCard.ComboScalarPerIndex`
- `CombatCard.MaxComboScalar`
- `FRuneComboBonusConfig`
- 投射物节点上的 `Add Combo Stacks To Projectile Count`

这些字段保留给旧资产加载；当前运行时不会让它们改变卡牌倍率或投射物数量。

## 替代方案

| 旧需求 | 当前做法 |
| --- | --- |
| 随连招提升伤害 | 用 Link Recipe、Matched Flow 或 Catalyst Buff |
| 随连招增加投射物 | 在具体 Base / Link Flow 中直接配置投射物数量 |
| 末段触发爆发 | 用 `RequiredFlowRole = Finisher` 或 `RequiredActionSlot = WeaponSkill` |
| 多卡前后联动 | 用相邻卡 `CardIdTag`、`CardEffectTags`、FlowRole 条件 |

## 验收

1. 旧 Combo Scaling 字段开启时，运行时倍率仍为 1.0。
2. 旧投射物 Combo stacks 字段开启时，不会因为 ComboIndex 增加投射物。
3. 新卡牌说明和新符文配置不再使用“连招奖励”作为构筑入口。
