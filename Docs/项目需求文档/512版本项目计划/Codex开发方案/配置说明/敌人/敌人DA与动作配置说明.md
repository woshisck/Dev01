# 敌人 DA 与动作配置说明

字段含义速查见：`敌人参数中文说明与功能简介.md`

## 已有功能

- `EnemyData` 已支持刷怪分数、行为树、移动调参、攻击配置、敌人专属 Buff、预生成 FX 和霸体参数。
- 敌人攻击命中已通过 `AN_MeleeDamage` 配置伤害、动作韧性、命中事件和附加效果。
- 敌人 BP 配置已拆到《敌人BP配置说明》，AI 行为树和黑板配置已拆到《敌人AI配置说明》。

## 待做功能

- 在老鼠和死亡守卫的 `EnemyData` 中按本文填写数值，尤其是 `AttackProfile`。
- 在攻击蒙太奇中补齐流血、击退、霸体窗口等动作通知。
- 确认 BP 和 AI 配置分别按独立文档完成。

面向：策划 / 关卡设计  
适用范围：512 敌人 AI、动作命中、霸体和敌人专属 Buff

## 1. 敌人基础 DA

打开对应 `EnemyData` 数据资产，重点配置：

| 字段 | 用法 | 推荐 |
|---|---|---|
| `EnemyClass` | 敌人蓝图类 | 指向对应 BP |
| `DifficultyScore` | 刷怪难度分，分越高越优先刷 | 老鼠 2；死亡守卫 7-9 |
| `BehaviorTree` | 此敌人的行为树 | 必填优先；留空才用 AIController fallback |
| `AttackProfile` | C++ 攻击任务读取的攻击列表 | 见第 2 节；先按 `AttackRole` 分支，再按距离、冷却、权重选招 |
| `SuperArmorThreshold` | 连续受击几次后触发霸体，0 为不触发 | 老鼠 0；死亡守卫 2-3 |
| `SuperArmorDuration` | 霸体持续秒数 | 死亡守卫 1.5-2.0 |

AI 移动和行为树配置见：

- `敌人AI配置说明.md`
- `敌人BP配置说明.md`

## 2. AttackProfile 攻击配置

`AttackProfile.Attacks` 的每个条目代表一类攻击。行为树中使用 `Enemy Attack By Profile` 后，代码会按距离、冷却和权重自动选择一条攻击。

| 字段 | 用法 |
|---|---|
| `AttackName` | 配置备注名，例如 `QuickBite`、`Heavy` |
| `AbilityTags` | 本攻击可激活的 GA Tag 列表，必须来自该敌人 `AbilityData` |
| `MinRange` | 最近可用距离 |
| `MaxRange` | 最远可用距离，0 为无限远 |
| `Weight` | 多个攻击同时可用时的随机权重 |
| `Cooldown` | 该攻击成功触发后的冷却 |
| `bPreAttackFlash` | 是否播放前摇红光 |
| `AttackRole` | 行为树攻击分支：`Skill`、`SpecialMovement`、`CloseMelee` |
| `AttackMovementMode` | 攻击激活时是否附带程序位移，默认 `None` |
| `LungeStartRange` | `RadialLunge` 的触发距离，敌人距离目标大于等于该值才前冲 |
| `LungeDistance` | `RadialLunge` 最大前冲距离 |
| `LungeDuration` | `RadialLunge` 前冲持续时间 |
| `LungeStopDistance` | `RadialLunge` 尽量停在目标外侧的距离 |
| `MovementAttackRangeMultiplier` | 远距位移攻击范围倍率，默认按基础 `AttackRange * 2.5` |
| `MovementAttackCooldown` | 远距位移攻击内置冷却，默认 10 秒 |

老鼠推荐：

| `AttackName` | `AttackRole` | `MinRange` | `MaxRange` | `Weight` | `Cooldown` | 说明 |
|---|---|---:|---:|---:|---:|---|
| `QuickBite` | `CloseMelee` | 0 | 170 | 2 | 0.8 | 快速咬击，建议绑定流血 |
| `Bite` | `CloseMelee` | 0 | 180 | 1 | 1.1 | 第二套咬击，建议绑定撕裂 |

死亡守卫推荐：

| `AttackName` | `AttackRole` | `MinRange` | `MaxRange` | `Weight` | `Cooldown` | 程序位移 | 说明 |
|---|---|---:|---:|---:|---:|---|---|
| `Sweep` | `CloseMelee` | 0 | 290 | 2 | 1.6 | `None` | 中近距离普通扫击 |
| `Heavy` | `SpecialMovement` | 160 | 650 | 1 | 2.2 | `RadialLunge` | 对应 `Enemy.Melee.HAtk2`，距离大于 300 时前冲，命中段可带击退和霸体窗口 |

死亡守卫 `Heavy` 的前冲参数推荐：

| 字段 | 推荐 |
|---|---:|
| `LungeStartRange` | 300 |
| `LungeDistance` | 280 |
| `LungeDuration` | 0.35 |
| `LungeStopDistance` | 170 |
| `MovementAttackRangeMultiplier` | 2.5 |
| `MovementAttackCooldown` | 10.0 |

远距位移攻击的节奏规则：

- 距离小于 `LungeStartRange` 时，`AttackRole=SpecialMovement` 的 `Heavy` 不参与选招；死亡守卫会优先使用 `AttackRole=CloseMelee` 的 `Sweep / Enemy.Melee.HAtk1`。
- 距离大于等于 `LungeStartRange` 时，`Heavy` 作为远距位移攻击选中；成功发动后进入 `MovementAttackCooldown` 秒内置冷却。
- `Heavy.Cooldown` 和 `MovementAttackCooldown` 期间，玩家如果仍在中远距离，敌人会继续追近，不会反复停在原地尝试攻击。
- 玩家离开远距位移攻击范围后，内置冷却会重置。

`AbilityTags` 填具体攻击 Tag，不要只填父级分类 Tag。代码会先调用 `AbilityData.HasAbility(Tag)` 过滤，没配蒙太奇 / GA 的 Tag 不会被激活。

如果日志出现 `[EnemyAIAttack] Reason=NoValidAbilityTag ConfiguredTags=Enemy.Melee.HAtk2`，说明 `AttackProfile.Heavy` 已配置，但死亡守卫的 `AbilityData` 里没有给 `Enemy.Melee.HAtk2` 配有效蒙太奇或 `MontageConfig`。此时远距前冲不会再占用 650 攻击范围，敌人会继续靠近并尝试其他有效攻击。要启用死亡守卫中距前冲，必须补齐 `Enemy.Melee.HAtk2`；如果暂时没有该动作，请把 `Heavy.AbilityTags` 改成已有有效攻击 Tag，或把 `Heavy.Weight` 设为 0。

## 3. 敌人专属 Buff

`EnemyBuffPool` 的每个条目使用：

| 字段 | 用法 |
|---|---|
| `RuneDA` | 要施加给敌人的符文 / Buff |
| `DifficultyScore` | 该 Buff 生效时额外消耗的难度分 |
| `ApplyChance` | 生效概率，1 为必定生效 |

死亡守卫的概率护甲建议放在 `EnemyBuffPool`：

| 字段 | 推荐 |
|---|---|
| `RuneDA` | `Content/Docs/BuffDocs/Playtest_GA/IronArmor/DA_Rune_IronArmor` |
| `DifficultyScore` | 2-3 |
| `ApplyChance` | 0.3-0.4 |

当前未看到专门命名为 `DA_EnemyBuff_IronArmor` 的资产；现有可直接使用的是 `DA_Rune_IronArmor`。

## 4. 动作命中配置

在攻击蒙太奇的 `AN_MeleeDamage` 中配置：

| 字段 | 用法 |
|---|---|
| `ActDamage` | 本段命中伤害 |
| `ActResilience` | 攻击动作韧性，越高越不容易被同级打断 |
| `OnHitEventTags` | 命中后广播事件 |
| `AdditionalRuneEffects` | 命中附加效果 |

老鼠的被动效果不要写在 AI Controller 里，按动作命中段绑定：

| 动作 / Tag | 建议附加效果 |
|---|---|
| `Enemy.Melee.LAtk1` / `QuickBite` | `AdditionalRuneEffects` 绑定流血，概率 0.2-0.3 |
| `Enemy.Melee.LAtk2` / `Bite` | `AdditionalRuneEffects` 绑定撕裂，概率和持续时间按手感配置 |

死亡守卫的 `Enemy.Melee.HAtk2` 同时承担中距前冲攻击。`AttackProfile` 只负责选中并写入前冲上下文；实际命中效果仍放在蒙太奇 `AN_MeleeDamage`：

| 动作 / Tag | 建议附加效果 |
|---|---|
| `Enemy.Melee.HAtk1` / `Sweep` | 普通高伤害或小击退 |
| `Enemy.Melee.HAtk2` / `Heavy` | `AdditionalRuneEffects` 或 `OnHitEventTags` 绑定击退，蒙太奇前摇可挂霸体窗口 |

死亡守卫重攻击的霸体窗口用 `AnimNotifyState_AddGameplayTag` 添加 `Buff.Status.SuperArmor`，窗口结束自动移除。

死亡守卫“投掷小型敌人”本轮不实现。后续默认设计为：死亡守卫消耗附近小型敌人，投向玩家当前位置，落点有圆圈预警，落地范围伤害和减速，被投掷小怪命中或落地后死亡 / 隐藏。
