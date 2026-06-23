# GenericGraph Dash 与旧连击 Tag 兼容说明

## 当前状态

玩家当前动作不再以 GenericGraph 连击分支作为主流程。当前运行时只暴露 Attack / Skill / WeaponSkill / Dash 四类动作输入，卡牌通过 `ECombatDeckActionSlot` 和 FlowRole 结算。

本文件仅说明旧 GenericGraph / DashSave / Combo Tag 的兼容边界。

## 不再新配的内容

- 不再手动配置旧 Combo Tag。
- 不再为新武器建立 Light / Heavy 输入分支。
- 不再把 DashSave 当作卡牌连携主条件。
- 不再在新卡牌里依赖旧 `RequiredAction = Light/Heavy`。

## 当前替代关系

| 旧配置 | 当前做法 |
| --- | --- |
| Light / Heavy 根节点 | Attack / WeaponSkill 动作槽 |
| Combo1/2/3/4 Tag | 仅作为旧蒙太奇或旧 GA 兼容上下文 |
| DashSave 保存连招节点 | Dash 单槽卡 + recovery cancel / 后摇打断奖励 |
| `CanCombo` 开窗续段 | 当前主设计不依赖连招续段 |
| Graph 节点 Link 条件 | 卡牌顺序、CardIdTag、FlowRole 或 Link 配方 |

## 旧资产兼容

如果旧武器仍依赖 GenericGraph：

1. 保留原资产，不主动删除。
2. 新增内容优先迁移到 `AttackAbilityData` / `WeaponSkillAbilityData`。
3. 只在排查旧资产时查看旧 Combo Tag。
4. 如果旧 Combo Tag 残留导致无法攻击，优先清理运行时临时 Tag，而不是继续扩展旧图。

## 验收

- 新武器不需要旧 Combo Tag 也能 Attack / WeaponSkill。
- Dash 能触发 Dash 单槽卡。
- 后摇阶段切武器能触发 recovery cancel 奖励。
- 旧资产兼容路径不会阻断当前动作槽输入。
