# Player Action + Combat Deck Current Setup

本文档记录当前玩家动作与卡牌系统的制作口径，用于替代旧的轻/重攻击、SpecialAttack、消耗式卡组理解。

## 当前动作口径

| 玩家动作 | 运行时含义 | 卡牌槽位 | 默认流程角色 |
| --- | --- | --- | --- |
| Attack | 玩家普通攻击，不再区分轻/重攻击 | Attack 序列槽 | 由卡牌决定 Starter / Catalyst / Any |
| Skill | 玩家选择的主动技能 | Skill 单槽 | Catalyst |
| WeaponSkill | 武器提供的战技 | WeaponSkill 单槽 | Finisher |
| Dash | 通用冲刺/闪避动作 | Dash 单槽 | Catalyst |

旧 `LightAttack` / `HeavyAttack` / `SpecialAttack` / `Special` 只作为资产重定向、旧蓝图加载和命名兼容存在，新制作不要再以它们作为设计入口。

## 卡牌结算口径

- 卡牌现在是 `Resolve`，不是 `Consume`。攻击触发当前卡牌效果，但不会从 `DeckList` 中移除这张卡。
- `ConsumedCard`、`OnCardConsumed`、`Card_Consume` 等旧名字只保留兼容旧蓝图、旧日志和旧资产。
- 新逻辑应使用 `ResolvedCard`、`OnCardResolved`、`Card_Resolve`。
- Attack 槽是顺序出牌：按 `ActiveSequence` 与 `CurrentIndex` 依次结算。
- Skill / WeaponSkill / Dash 各自只有一个单槽卡，不参与 Attack 序列推进。
- 卡牌的 Passive Flow 独立于出牌结算，属于装备后持续生效的被动效果。

## Build 流程角色

当前构筑循环按以下角色理解：

| 角色 | 用途 | 建议动作 |
| --- | --- | --- |
| Starter | 启动卡，上 Buff 或创建可被后续读取的状态 | Attack |
| Catalyst | 催化卡，强化、延长或转化已有 Buff | Attack / Skill / Dash |
| Finisher | 终结卡，引爆或结算前置状态 | WeaponSkill |

战技默认视为引爆逻辑。攻击和主动技能可以通过卡牌配置成为 Starter 或 Catalyst。

## 制作配置建议

- 新攻击序列卡：`RequiredActionSlot = Attack`，`RequiredAction = Any`。
- 新技能卡：`RequiredActionSlot = Skill`，通常 `RequiredFlowRole = Catalyst`。
- 新战技卡：`RequiredActionSlot = WeaponSkill`，通常 `RequiredFlowRole = Finisher`。
- 新冲刺卡：`RequiredActionSlot = Dash`，通常 `RequiredFlowRole = Catalyst`。
- 需要顺序连携时，用卡牌顺序、Link 方向、前后卡 `CardIdTag` 或 FlowRole 条件表达，不要依赖轻/重攻击分支。
- 旧 `RequiredAction = Light/Heavy` 会被兼容为新 Attack 上下文可匹配，但新卡不要再这样配置。
- 如果只是常驻加成，优先放在 Passive Flow；如果需要出牌顺序影响结果，放在 Base / Link / Matched Flow。

## 运行时入口

- `YogPlayerControllerBase` 只绑定 Attack / WeaponSkill / Skill / Dash。
- `GA_MeleeAttack` 负责 Attack 槽卡牌结算。
- `UPlayerActiveSkillComponent` 负责玩家选择的 Skill，并触发 Skill 单槽卡。
- `GA_WeaponSkill` / `GA_PlayMontage` 负责 WeaponSkill 单槽卡与共享冷却。
- `GA_PlayerDash` 负责 Dash 单槽卡。
- Skill 与 WeaponSkill 使用 `PlayerState.Cooldown.SkillShared` 共享冷却锁。
- 后摇阶段切武器会触发 recovery cancel 奖励，并清理 Skill / WeaponSkill 冷却。

## 废弃兼容入口

以下类只为旧资产加载保留，不用于新制作：

- `UComboRuntimeComponent`
- `UPlayerSpecialAttackComponent`
- `USpecialAttackDataAsset`
- `UGA_PlayerSpecialAttack`
- `UGA_Special`

新功能应接入 `UPlayerActiveSkillComponent`、`UCombatDeckComponent` 和四个动作槽位。

## 验收清单

- 新卡牌说明里不出现“消耗卡牌”。
- 新玩家动作说明里不出现“轻攻击/重攻击切换”。
- 战技卡默认承担 Finisher / detonate 语义。
- 主动技能和战技互相进入共享冷却。
- 攻击卡能按顺序结算，并能通过前后顺序触发 Link。
- 被动构筑效果放在 Passive Flow，而不是伪装成一次性出牌。
