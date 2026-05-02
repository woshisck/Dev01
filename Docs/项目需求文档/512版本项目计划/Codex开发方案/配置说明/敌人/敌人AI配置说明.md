# 敌人 AI 配置说明

字段含义速查见：`敌人参数中文说明与功能简介.md`

## 已有功能

- `AYogAIController` 已在 C++ 中接管 Crowd PathFollowing、敌人 DA 行为树启动和移动目标计算。
- `EnemyData.BehaviorTree` 已优先生效；留空时才使用 AIController fallback 行为树。
- `BTService_UpdateEnemyCombatMove` 已实现，会写入移动目标、距离和是否进入攻击距离。
- `BTTask_EnemyAttackByProfile` 已实现，会从 `EnemyData.AttackProfile` 读取攻击距离、权重、冷却和 GA Tag。
- 老鼠和死亡守卫可通过同一套 Controller + 不同 `MovementTuning` 做出不同移动风格。

## 待做功能

- 在老鼠、死亡守卫的 `EnemyData` 中填写 `BehaviorTree`、`MovementTuning` 和 `AttackProfile`。
- 在对应行为树中挂 `BTService_UpdateEnemyCombatMove`。
- 在黑板中补齐移动和攻击判断所需 key。
- 将旧行为树中会直接 `SetActorRotation` 或强行覆盖移动目标的节点移除或停用。

## 1. EnemyData AI 参数

老鼠：

| 字段 | 值 |
|---|---:|
| `BehaviorTree` | `BT_Rat` |
| `MovementTuning.ApproachStyle` | `SwarmFlank` |
| `PreferredRange` | 180 |
| `AttackRange` | 150 |
| `AcceptanceRadius` | 60 |
| `RepathInterval` | 0.2 |
| `FlankDistance` | 160 |
| `StrafeChance` | 0.45 |
| `CrowdSeparationWeight` | 2.5 |
| `AttackProfile.Attacks` | 见下方攻击任务配置 |

死亡守卫：

| 字段                             |                                   值 |
| ------------------------------ | ----------------------------------: |
| `BehaviorTree`                 | 使用死亡守卫行为树；没有专用树时先复用 `BT_Enemy_Base` |
| `MovementTuning.ApproachStyle` |                       `BruiserHold` |
| `PreferredRange`               |                                 320 |
| `AttackRange`                  |                                 260 |
| `AcceptanceRadius`             |                                 110 |
| `RepathInterval`               |                                0.35 |
| `FlankDistance`                |                                  80 |
| `StrafeChance`                 |                                0.15 |
| `CrowdSeparationWeight`        |                                 3.5 |
| `AttackProfile.Attacks`        |                 见下方攻击任务配置 |

## 2. Blackboard

行为树使用的黑板需要准备：

| Key                  | 类型             | 用途                  |
| -------------------- | -------------- | ------------------- |
| `TargetActor`        | Object / Actor | 当前攻击目标，留空时服务会默认写入玩家 |
| `MoveTargetLocation` | Vector         | C++ 计算出的移动目标点       |
| `DistanceToTarget`   | Float          | 敌人与目标的 2D 距离        |
| `bInAttackRange`     | Bool           | 是否进入攻击距离            |
| `AcceptanceRadius`   | Float          | 推荐停距半径              |

## 3. Behavior Tree

在战斗主分支上挂 `BTService_UpdateEnemyCombatMove`。

编辑器里显示名不是 C++ 类名：

| C++ 类名                             | 行为树编辑器显示名                  | 类型      |
| ---------------------------------- | -------------------------- | ------- |
| `UBTService_UpdateEnemyCombatMove` | `Update Enemy Combat Move` | Service |
| `UBTTask_PreAttackFlash`           | `Pre Attack Flash`         | Task    |
| `UBTTask_ActivateAbilityByTag`     | `Activate Ability By Tag`  | Task    |
| `UBTTask_EnemyAttackByProfile`     | `Enemy Attack By Profile`  | Task    |

当前已有树可以保留现有结构：

```text
Main Selector
  Service: BTS_UpdateState
  Service: Update Enemy Combat Move

  Sequence: Combat
    BTT_SetMoveSpeed
    BTT_AddEnemyGPTag
    BTT_SetEnemyCombatState
    Run Behavior: SubBT_AttackModeSelect
```

`Update Enemy Combat Move` 挂在 Main 或 Combat 分支上都可以，只要它能持续 Tick。

如果要把攻击选择交给 C++ 处理，推荐攻击子树改成：

```text
Selector
  Sequence: Attack
    Decorator: Blackboard(bInAttackRange Is Set)
    Enemy Attack By Profile

  Sequence: Move
    MoveTo: MoveTargetLocation
```

`bInAttackRange == true` 不是一个单独的 Decorator 名字。编辑器里这样配置：

1. 在黑板中新建 `bInAttackRange`，类型选 `Bool`。
2. 在攻击 `Sequence` 上右键添加 Decorator，选择 UE 自带的 `Blackboard`。
3. 选中这个 Decorator，把 `Blackboard Key` 设为 `bInAttackRange`。
4. `Key Query` 选择 `Is Set`，表示 Bool 为 true。

`Enemy Attack By Profile` 已经带 `bPreAttackFlash` 配置，不需要再额外接 `Pre Attack Flash`。旧树也可以继续用 `Pre Attack Flash + Activate Ability By Tag`，但那种方式只能在行为树节点上手动固定一组 Tag，不能按距离、权重和冷却自动选招。

`MoveTo` 的目标必须使用 `MoveTargetLocation`。如果当前 MoveTo 节点不能读取黑板里的 `AcceptanceRadius`，先手动填半径：

| 敌人 | MoveTo Acceptable Radius |
|---|---:|
| 老鼠 | 60 |
| 死亡守卫 | 110 |

## 4. AttackProfile 攻击任务

推荐使用 `Enemy Attack By Profile`，它会读取当前敌人 `EnemyData.AttackProfile.Attacks`：

| 字段 | 用法 |
|---|---|
| `AttackName` | 方便策划识别的名字，不影响逻辑 |
| `AbilityTags` | 可被本攻击项随机激活的 GA Tag；必须和 `AbilityData` 里的攻击 Tag 对上 |
| `MinRange` | 距离低于该值时不选这个攻击 |
| `MaxRange` | 距离高于该值时不选这个攻击；填 0 表示不限制上限 |
| `Weight` | 同时满足条件时的随机权重 |
| `Cooldown` | 这个攻击项成功触发后的冷却时间 |
| `bPreAttackFlash` | 是否在 GA 持续期间播放攻击前摇红光 |

老鼠推荐：

| `AttackName` | 距离 | 权重 | 冷却 | 说明 |
|---|---:|---:|---:|---|
| `QuickBite` | 0-170 | 4 | 0.8 | 快速咬击，`AbilityTags` 填老鼠轻攻击 / 咬击 Tag |
| `LeapBite` | 120-260 | 1 | 2.5 | 少量跳扑，`bPreAttackFlash` 可开启 |

死亡守卫推荐：

| `AttackName` | 距离 | 权重 | 冷却 | 说明 |
|---|---:|---:|---:|---|
| `Sweep` | 0-280 | 3 | 1.6 | 普通扫击，压迫主循环 |
| `Heavy` | 180-360 | 2 | 3.5 | 重攻击，可带击退和霸体窗口，建议开启 `bPreAttackFlash` |

`AbilityTags` 不要填分类父 Tag，优先填 `AbilityData` 中确实存在的具体攻击 Tag；代码会过滤掉没有配置蒙太奇 / GA 的 Tag。

## 5. 验收

- 老鼠应该从侧向或背向接近玩家，不再直线挤成一团。
- 死亡守卫应该稳定停在中近距离读招，不频繁贴脸抖动。
- 行为树启动日志应出现 `YogAIController RunBehaviorTree(...) -> 1`。
- 如果行为树没有启动，优先检查 `EnemyData.BehaviorTree` 和敌人 BP 的 `AI Controller Class`。
