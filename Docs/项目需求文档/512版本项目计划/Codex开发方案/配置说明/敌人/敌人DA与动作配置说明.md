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
| `AttackProfile` | C++ 攻击任务读取的攻击列表 | 见第 2 节 |
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

老鼠推荐：

| `AttackName` | `MinRange` | `MaxRange` | `Weight` | `Cooldown` | 说明 |
|---|---:|---:|---:|---:|---|
| `QuickBite` | 0 | 170 | 4 | 0.8 | 快速咬击，命中段可挂 20%-30% 流血 |
| `LeapBite` | 120 | 260 | 1 | 2.5 | 少量跳扑，用于打断玩家站桩 |

死亡守卫推荐：

| `AttackName` | `MinRange` | `MaxRange` | `Weight` | `Cooldown` | 说明 |
|---|---:|---:|---:|---:|---|
| `Sweep` | 0 | 280 | 3 | 1.6 | 中近距离普通扫击 |
| `Heavy` | 180 | 360 | 2 | 3.5 | 重攻击，命中段可带击退，蒙太奇可挂霸体窗口 |

`AbilityTags` 填具体攻击 Tag，不要只填父级分类 Tag。代码会先调用 `AbilityData.HasAbility(Tag)` 过滤，没配蒙太奇 / GA 的 Tag 不会被激活。

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

老鼠流血建议放在快速咬击的 `AdditionalRuneEffects`，概率 0.2-0.3。死亡守卫击退放在重攻击命中段。

死亡守卫重攻击的霸体窗口用 `AnimNotifyState_AddGameplayTag` 添加 `Buff.Status.SuperArmor`，窗口结束自动移除。
