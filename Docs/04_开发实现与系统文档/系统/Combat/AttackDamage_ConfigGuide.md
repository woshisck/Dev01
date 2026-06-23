# 攻击伤害配置指南

> 当前玩家动作配置以 [PlayerActionCombatDeck_Current.md](PlayerActionCombatDeck_Current.md) 和策划手册中的“武器动作与攻击参数配置说明”为准。旧 LightAtk / HeavyAtk 连招示例已废弃。

## 当前玩家配置

| 内容 | 配置位置 |
| --- | --- |
| Attack 蒙太奇与命中窗口 | `WeaponDefinition.AttackAbilityData` -> `AbilityData.MontageConfigMap` |
| WeaponSkill 蒙太奇与命中窗口 | `WeaponDefinition.WeaponSkillAbilityData` -> `AbilityData.MontageConfigMap` |
| 命中窗口 | `MontageConfigDA.Entries.Hit Detection Window` |
| 攻击参数 | `MontageAttackDataAsset` |
| 卡牌触发 | `CombatCard.RequiredActionSlot` + `RequiredFlowRole` |

## 敌人配置

敌人仍可使用敌人侧 LAtk / HAtk 命名和 AI 攻击配置；这属于敌人动作分类，不代表玩家还有轻/重攻击输入。

## 旧内容处理

- `GA_Player_LightAtk*`、`GA_Player_HeavyAtk*`、旧 ComboGraph 只作资产兼容。
- 新玩家武器不要按旧“每段连招一个 GA”配置。
- 卡牌构筑不要依赖旧攻击段数，使用动作槽、流程角色、卡牌顺序和 Link Recipe。
