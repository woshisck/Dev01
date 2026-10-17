# 动作规则示例

所有行均禁用；导出不会挂载，也不会改变正式冲刺关系。多个 Tag 用分号分隔。

## Actions

| ID | DisplayName | AbilityTags | StateTags |
| --- | --- | --- | --- |
| Attack | 普攻 | Character.State.Skill.Attack | Character.State.Skill.Attack |
| Dash | 冲刺 | Character.State.Movement.Dash | Character.State.Movement.Dash |
| WeaponSkill | 战技 | Character.State.Skill.WeaponSkill | Character.State.Skill.WeaponSkill |

## Rules

| ID | Enabled | Subject | Current | Request | Effect | Condition | Reason |
| --- | --- | --- | --- | --- | --- | --- | --- |
| R001 | FALSE | player | Dash | Attack | block | | 冲刺期间拒绝普攻请求（示例） |
| R002 | FALSE | player | Attack | Dash | interrupt | | 冲刺请求可打断普攻（示例） |
| R003 | FALSE | player | Attack | WeaponSkill | interrupt | | 战技请求可打断普攻（示例） |
| R004 | FALSE | player | Dash | WeaponSkill | block | | 冲刺期间拒绝战技请求（示例） |
