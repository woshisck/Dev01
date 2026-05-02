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

## 1. 敌人 BP

当前目标 BP：

| 敌人   | BP                                                              |
| ---- | --------------------------------------------------------------- |
| 老鼠   | `Content/Code/Enemy/Minion/Rat/BP_Enemy_Rat_01`                 |
| 死亡守卫 | `Content/Code/Enemy/Minion/RottenGuard/BP_Enemy_RottenGuard_02` |

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

如果白名单缺少某个敌人，刷怪计划会生成该敌人，但实际 Spawn 会被跳过。
