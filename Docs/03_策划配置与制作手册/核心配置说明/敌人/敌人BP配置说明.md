# 敌人 BP 配置说明

字段含义速查见：`敌人参数中文说明与功能简介.md`

## 已有功能

- 敌人 BP 仍使用项目层 `AYogAIController` / 其蓝图子类，不需要修改引擎。
- `AYogAIController` 已接入 Crowd PathFollowing，用于更好的寻路和避让。
- 敌人生成会读取 `EnemyData.EnemyClass`，并在生成后自动 Spawn Default Controller。
- 敌人 `BeginPlay` 已从 `EnemyData` 推送 `SuperArmorThreshold` / `SuperArmorDuration` 到 ASC。

## 待做功能

- 在编辑器中确认老鼠、死亡守卫 BP 的 `AI Controller Class` 都指向 `AYogAIController` 的蓝图子类。
- 确认敌人 BP 的 `CharacterDataComponent` 指向正确的 `EnemyData`。
- 确认关卡 `MobSpawner.EnemySpawnClassis` 白名单包含对应敌人 BP。
- 新建哨铃狱卒和看守队长 BP，并分别填入 `DA_AlarmBellJailer.EnemyClass`、`DA_GuardCaptain.EnemyClass`。
- 给哨铃狱卒和看守队长补模型、AnimBP、动作蒙太奇、命中通知和技能效果蓝图。

## 1. 敌人 BP

当前目标 BP：

| 敌人   | BP                                                              |
| ---- | --------------------------------------------------------------- |
| 老鼠   | `Content/Code/Enemy/Minion/Rat/BP_Enemy_Rat_01`                 |
| 死亡守卫 | `Content/Code/Enemy/Minion/RottenGuard/BP_Enemy_RottenGuard_02` |
| 哨铃狱卒 | 待制作，建议 `Content/Code/Enemy/Minion/AlarmBellJailer/BP_Enemy_AlarmBellJailer` |
| 看守队长 | 待制作，建议 `Content/Code/Enemy/Elite/GuardCaptain/BP_Enemy_GuardCaptain` |

打开敌人 BP，在 Class Defaults 中检查：

| 字段                            | 配置                                                   |
| ----------------------------- | ---------------------------------------------------- |
| `AI Controller Class`         | `BP_AIControllerBase` 或其他继承自 `AYogAIController` 的蓝图类 |
| `Auto Possess AI`             | `Placed in World or Spawned`                         |
| `Use Controller Rotation Yaw` | 关闭                                                   |

如果敌人使用了其他 AIController 类，这次新增的 Crowd 避让、DA 行为树启动、C++ 移动目标计算都不会生效。

## 2. CharacterDataComponent

在敌人 BP 的 `CharacterDataComponent` 中配置：

| 敌人 | Character Data |
|---|---|
| 老鼠 | `Content/Docs/Data/Enemy/Rat/DA_Rat` |
| 死亡守卫 | `Content/Docs/Data/Enemy/RottenGuard/DA_RottenGuard` |
| 哨铃狱卒 | `Content/Docs/Data/Enemy/AlarmBellJailer/DA_AlarmBellJailer` |
| 看守队长 | `Content/Docs/Data/Enemy/GuardCaptain/DA_GuardCaptain` |

`EnemyData` 里再配置 `EnemyClass`、`BehaviorTree`、`MovementTuning`、霸体参数和敌人专属 Buff。

## 3. CharacterMovement

敌人移动建议：

| 字段                                | 配置       |
| --------------------------------- | -------- |
| `Orient Rotation to Movement`     | 开启       |
| `Use Controller Desired Rotation` | 关闭       |
| `Rotation Rate.Yaw`               | 360 左右起调 |
| `Use RVO Avoidance`               | 关闭       |

避让由 `AYogAIController` 的 Crowd PathFollowing 处理，不要再叠加 RVO。

## 4. Spawner 白名单

关卡中的 `MobSpawner.EnemySpawnClassis` 需要包含本房间允许刷出的敌人 BP。

| 房间需求 | 白名单建议 |
|---|---|
| 老鼠群 | 加入 `BP_Enemy_Rat_01` |
| 死亡守卫房 | 加入 `BP_Enemy_RottenGuard_02` |
| 混合房 | 两者都加入 |
| 哨铃狱卒房 | 加入 `BP_Enemy_AlarmBellJailer`，并搭配腐鼠、腐化看守或弱囚犯 |
| 看守队长精英房 | 加入 `BP_Enemy_GuardCaptain`，并搭配 1-2 个狱卒类敌人 |

如果白名单缺少某个敌人，刷怪计划会生成该敌人，但实际 Spawn 会被跳过。

## 5. 哨铃狱卒与看守队长 BP 待制作清单

哨铃狱卒：

| 内容 | 要求 |
|---|---|
| 敌人 BP | 继承现有敌人 Character 基类，`AI Controller Class` 使用 `AYogAIController` 或其 BP 子类 |
| CharacterDataComponent | 指向 `DA_AlarmBellJailer` |
| Skeletal Mesh / AnimBP | 后续按模型制作；没有模型前可临时用占位 Mesh 验证 AI |
| AbilityData | 在 `DA_AbilityMontage_AlarmBellJailer_01` 中补 `Enemy.Skill.Skill1`、`Enemy.Skill.Skill2`、`Enemy.Melee.LAtk1`、`Enemy.Melee.LAtk2` |
| 摇铃召唤 | `Enemy.Skill.Skill1` 成功时召唤腐鼠或通知附近敌人；需要同屏召唤上限 |
| 铃声震荡 | `Enemy.Skill.Skill2` 做小范围硬直、减速或轻击退 |
| 推击与后撤 | `Enemy.Melee.LAtk1` 命中段配置轻击退；`Enemy.Melee.LAtk2` 可做无命中后撤动作 |

看守队长：

| 内容 | 要求 |
|---|---|
| 敌人 BP | 继承现有敌人 Character 基类，`AI Controller Class` 使用 `AYogAIController` 或其 BP 子类 |
| CharacterDataComponent | 指向 `DA_GuardCaptain` |
| Skeletal Mesh / AnimBP | 后续按铁笼头盔 / 远程武器 / 队长外套制作；没有模型前可临时用占位 Mesh 验证 AI |
| AbilityData | 在 `DA_AbilityMontage_GuardCaptain_01` 中补 `Enemy.Range.LAtk1`、`Enemy.Range.HAtk1`、`Enemy.Skill.Skill1`、`Enemy.Skill.Skill2`、`Enemy.Melee.HAtk1`、`Enemy.Melee.HAtk2`、`Enemy.Skill.Skill4` |
| 远程攻击 | Range 槽位需要在 BP / Notify 中实现投射物或射线判定 |
| 召唤狱卒 | `Enemy.Skill.Skill1` 召唤狱卒类敌人；需要同屏召唤上限、出生点选择和失败保护 |
| 队长号令 Buff | `Enemy.Skill.Skill2` 给附近狱卒施加增益；建议没有可强化目标时不释放 |
| 近身推击 | `Enemy.Melee.HAtk1` 配置重击退，攻击结束后 AI 会自动请求后撤 |
| 战术后撤 | `Enemy.Melee.HAtk2` 可做无命中后撤动作 |
| 低血火焰阶段 | `Enemy.Skill.Skill4` 在 35% 血量以下可用；需要点燃头部火炬、切换材质 / VFX，并让后续攻击附带火焰伤害 |
