# 攻击伤害技术说明

## 当前玩家入口

玩家战斗入口已经收敛为 Attack / Skill / WeaponSkill / Dash 四类动作槽。

| 动作槽 | 主要运行时 |
| --- | --- |
| Attack | `UGA_MeleeAttack` / 武器 Attack AbilityData |
| Skill | `UPlayerActiveSkillComponent` |
| WeaponSkill | `UGA_WeaponSkill`、`UGA_PlayMontage` 或武器专属战技 GA |
| Dash | `UGA_PlayerDash` |

玩家卡牌上下文不再使用旧 LightAtk / HeavyAtk 连招段数。旧 Combo1/2/3/4 Tag 只可作为旧蒙太奇选择和资产兼容信息。

## 伤害与命中

- 命中窗口来自 `MontageConfigDA.Entries.Hit Detection Window`。
- 伤害、距离、判定框、HitStop 来自 `MontageAttackDataAsset`。
- 卡牌通过 `FCombatDeckActionContext.ActionSlot`、`FlowRole`、卡牌顺序和 Link Recipe 参与效果。
- `CombatCard` 结算后不会从卡组移除；Attack 序列只推进当前索引。

## 旧兼容

- 旧 `GA_Player_LightAtk*` 重定向到当前 Attack / MeleeAttack 兼容路径。
- 旧 `GA_Player_HeavyAtk*` 重定向到 WeaponSkill 兼容路径。
- 旧 ComboGraph / WeaponComboConfig 不再作为新玩家武器配置入口。
- 旧 Combo Scaling、DashSave combo tag 注入和 ComboIndex 卡牌倍率均已禁用或标记废弃。
