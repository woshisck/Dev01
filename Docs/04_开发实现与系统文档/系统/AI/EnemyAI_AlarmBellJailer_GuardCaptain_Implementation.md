# 哨铃狱卒与看守队长 AI 实现记录

日期：2026-06-22
范围：敌人 AI、EnemyData 生成器、AttackProfile、后撤逻辑、后续 BP 制作清单

## 1. 本次已落地内容

本次将哨铃狱卒 `AlarmBellJailer` 和看守队长 `GuardCaptain` 接入现有敌人 AI 配置链路。当前阶段不依赖模型和动作，可先生成基础数据资产，后续再补 BP、AnimBP、蒙太奇和实际技能效果。

已修改的主要系统：

| 模块 | 内容 |
|---|---|
| `EnemyData.AttackProfile` | 新增血量门槛 `MinHealthPercent / MaxHealthPercent`，新增攻击结束后请求后撤 `bRequestRepositionOnResolve` |
| `AYogAIController` | 攻击结束时写入 `bLastAttackWhiffed`、`bPostAttackReposition`、`LastWhiffTime`、`LastRepositionRequestTime` |
| `BTTask_EnemyAttackByProfile` | 选招时支持血量门槛，支持 `AttackRole=Reposition`，并输出对应诊断日志 |
| `BTDecorator_EnemyPostAttackReposition` | 新增行为树 Decorator，用于判断是否进入攻击后后撤分支 |
| `GA_MeleeAttack` | 攻击结束时把打空结果和强制后撤请求通知给 AIController |
| `GA_EnemyMeleeAttacks` | 新增 `Enemy.Range.LAtk1-4`、`Enemy.Range.HAtk1-4` 敌人远程攻击 GA 槽位 |
| `EnemyAITemplateGenerator` | 生成 / 更新哨铃狱卒、看守队长、共享敌人 GAS 模板和默认行为树结构 |

## 2. 行为树数据流

```text
EnemyData.AttackProfile
  -> BTTask_EnemyAttackByProfile 按 Role / 距离 / 冷却 / 权重 / 血量筛选
  -> 激活 AbilityTags 对应的 GA
  -> GA 结束时通知 AYogAIController.NotifyAttackResolved
  -> 写入 bPostAttackReposition
  -> 默认行为树优先执行 AttackRole=Reposition 的后撤项
```

默认 Combat 分支顺序：

```text
PostAttackReposition
-> Skill
-> SpecialMovement
-> CloseMelee
-> MoveToCombatSlot
```

## 3. 生成器会处理的资产

| 资产 | 路径 |
|---|---|
| 默认黑板 | `/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee` |
| 默认行为树 | `/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee` |
| 共享敌人 GAS 模板 | `/Game/Docs/Data/Enemy/DA_Enemy_GASTemplate` |
| 哨铃狱卒 DA | `/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AlarmBellJailer` |
| 哨铃狱卒 AbilityData | `/Game/Docs/Data/Enemy/AlarmBellJailer/DA_AbilityMontage_AlarmBellJailer_01` |
| 看守队长 DA | `/Game/Docs/Data/Enemy/GuardCaptain/DA_GuardCaptain` |
| 看守队长 AbilityData | `/Game/Docs/Data/Enemy/GuardCaptain/DA_AbilityMontage_GuardCaptain_01` |

注意：哨铃狱卒和看守队长的 `EnemyClass` 会保持空值，等 BP 做好后再手动填写。

## 4. 当前默认攻击配置

哨铃狱卒：

| 攻击项 | Tag | Role | 目的 |
|---|---|---|---|
| `BellAlarm` | `Enemy.Skill.Skill1` | `Skill` | 摇铃警报 / 召唤入口 |
| `BellShock` | `Enemy.Skill.Skill2` | `Skill` | 近中距离铃声震荡 |
| `BatonShove` | `Enemy.Melee.LAtk1` | `CloseMelee` | 近身推击，结束后请求后撤 |
| `PanicRetreat` | `Enemy.Melee.LAtk2` | `Reposition` | 被贴身或攻击后拉开距离 |

看守队长：

| 攻击项 | Tag | Role | 目的 |
|---|---|---|---|
| `ExecutionShot` | `Enemy.Range.LAtk1` | `Skill` | 常规远程压制 |
| `SuppressiveShot` | `Enemy.Range.HAtk1` | `Skill` | 长冷却远程重压制 |
| `SummonJailer` | `Enemy.Skill.Skill1` | `Skill` | 召唤狱卒 |
| `CommandBuff` | `Enemy.Skill.Skill2` | `Skill` | 强化附近狱卒 |
| `CaptainPush` | `Enemy.Melee.HAtk1` | `CloseMelee` | 近身重击推开玩家，结束后请求后撤 |
| `TacticalRetreat` | `Enemy.Melee.HAtk2` | `Reposition` | 后撤到中远距离 |
| `FlameIgnition` | `Enemy.Skill.Skill4` | `Skill` | 35% 血量以下点燃头部火炬，进入火焰阶段 |

## 5. 后续制作清单

必须制作：

| 内容 | 哨铃狱卒 | 看守队长 |
|---|---|---|
| 敌人 BP | `BP_Enemy_AlarmBellJailer` | `BP_Enemy_GuardCaptain` |
| `EnemyClass` | 填入 `DA_AlarmBellJailer` | 填入 `DA_GuardCaptain` |
| 模型 / 骨骼 | 需要 | 需要 |
| AnimBP | 需要 | 需要 |
| 移动 / 受击 / 死亡动作 | 需要 | 需要 |
| 攻击蒙太奇 | Skill1、Skill2、LAtk1、LAtk2 | RangeLAtk1、RangeHAtk1、Skill1、Skill2、HAtk1、HAtk2、Skill4 |
| 命中 Notify | 推击、铃声震荡 | 远程判定、推击、火焰附加 |
| VFX / SFX | 摇铃、铃声震荡、召唤 | 枪击、号令、推击、头部火焰 |

技能效果待做：

| 技能 | 后续实现 |
|---|---|
| `BellAlarm` | 召唤腐鼠或通知附近敌人；需要同屏召唤上限 |
| `BellShock` | 小范围硬直、减速或轻击退 |
| `SummonJailer` | 召唤狱卒类敌人；需要可用出生点、失败保护、同屏上限 |
| `CommandBuff` | 给附近狱卒施加增益；没有目标时不释放或降低权重 |
| `FlameIgnition` | 点燃头部火炬、切换材质 / VFX、给后续攻击附加火焰伤害 |

关卡 / 刷怪待做：

- 将两个 BP 加入合适房间的 `MobSpawner.EnemySpawnClassis` 白名单。
- 在房间 `EnemyPool` 中加入对应 `EnemyData`。
- 看守队长建议作为精英房核心敌人，默认单只出现。
- 哨铃狱卒建议少量出现，避免多个召唤源同时拖慢战斗。

验证待做：

- 编译后运行 `EnemyAITemplateGenerator -Preset=DefaultMelee`。
- 打开生成报告 `Saved/EnemyAITemplateGeneratorReport.md`。
- 检查 `DA_AlarmBellJailer` 和 `DA_GuardCaptain` 是否指向默认行为树。
- 补 BP 后 PIE 检查巡逻、发现玩家、选招、近身推击后撤、低血火焰阶段入口。

## 6. 当前未验证项

本次没有编译，也没有运行 Unreal Editor commandlet。项目 `guide.md` 要求除非明确要求，否则不编译。当前修改已经做过静态检查，资产生成和自动化测试需要在后续编译后进行。
