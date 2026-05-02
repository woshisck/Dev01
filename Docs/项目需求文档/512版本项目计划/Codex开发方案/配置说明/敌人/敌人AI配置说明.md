# 敌人 AI 配置说明

字段含义速查见：`敌人参数中文说明与功能简介.md`

## 已有功能

- `AYogAIController` 已继承 `AModularAIController`，并在项目层启用 Crowd PathFollowing、敌人 DA 行为树启动、战斗移动目标计算。
- `EnemyData.BehaviorTree` 已优先生效；留空时才使用 AIController fallback 行为树。
- `BTService_UpdateEnemyAwareness` 已实现，会维护巡逻、警戒、进攻三态。
- `BTService_UpdateEnemyCombatMove` 已实现，会写入移动目标、距离、攻击距离判断和接受半径。
- `BTTask_UpdateEnemyPatrolTarget` 已实现，无巡逻点配置时会在出生点附近随机巡逻。
- `BTTask_EnemyPatrolWait` 已实现，会按 `EnemyData.AwarenessTuning.PatrolWaitMin/Max` 随机等待。
- `BTTask_EnemyAttackByProfile` 已实现，会按 `EnemyData.AttackProfile` 的距离、权重、冷却和 GA Tag 选招。
- `EnemyAITemplateGenerator` 已实现，可直接生成默认小怪黑板和行为树资产，并把老鼠、死亡守卫 DA 指向默认行为树。

## 待做功能

- 在编辑器里运行一次生成器，生成或更新默认行为树资产。
- 在 `DA_Rat`、`DA_RottenGuard` 中按敌人定位填写 `MovementTuning`、`AwarenessTuning`、`AttackProfile`。
- 在攻击蒙太奇命中通知里配置老鼠流血、死亡守卫击退和霸体窗口。
- 后续如需要固定巡逻路线，再扩展 PatrolRoute 资产；当前 V1 使用出生点周围随机巡逻。

## 1. 默认小怪 AI 设计

默认小怪使用三态：

| 状态 | 行为 | 进入条件 | 退出条件 |
|---|---|---|---|
| `Patrol` 巡逻 | 在出生点周围随机找点移动，短暂停留后继续巡逻 | 默认状态，或警戒超时 | 看见玩家、收到附近敌人通知、被攻击 |
| `Alert` 警戒 | 前往最后发现玩家的位置，短暂搜索 | 发现玩家但距离较远，或附近敌人广播警戒 | 看见玩家且进入进攻半径，或警戒超时 |
| `Combat` 进攻 | 追击、停距、包围、按攻击配置选招 | 进入进攻半径，或被玩家攻击 | 丢失目标超过 `LoseTargetDelay` 后转警戒 |

关键规则：

- 敌人攻击前会显式进入 `Combat`。
- 敌人受到伤害会立刻进入 `Combat`，目标设为伤害来源。
- 一个敌人发现玩家或被攻击后，会通知 `AlertBroadcastRadius` 内的附近敌人。
- 附近敌人收到通知后进入 `Alert`；如果自己也看到玩家并进入进攻半径，会自动转 `Combat`。

## 2. 生成器命令

推荐在编译成功后用编辑器命令行生成默认资产。

DryRun 只写报告，不保存资产：

```powershell
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=EnemyAITemplateGenerator -Preset=DefaultMelee -DryRun
```

正式生成并保存资产：

```powershell
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=EnemyAITemplateGenerator -Preset=DefaultMelee
```

生成报告位置：

```text
Saved/EnemyAITemplateGeneratorReport.md
```

生成器会创建或更新：

| 资产 | 路径 |
|---|---|
| 默认黑板 | `/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee` |
| 默认行为树 | `/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee` |
| 老鼠 DA 引用 | `/Game/Docs/Data/Enemy/Rat/DA_Rat.BehaviorTree` |
| 死亡守卫 DA 引用 | `/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard.BehaviorTree` |

## 3. 默认黑板 Key

生成器会自动创建这些 Key，不需要手动建：

| Key | 类型 | 用途 |
|---|---|---|
| `EnemyAIState` | Enum `EEnemyAIState` | 当前状态：巡逻、警戒、进攻 |
| `TargetActor` | Object / Actor | 当前目标，通常是玩家 |
| `LastKnownTargetLocation` | Vector | 最后一次看见或收到通知的位置 |
| `PatrolOriginLocation` | Vector | 巡逻中心点，默认是出生位置 |
| `PatrolTargetLocation` | Vector | 本次巡逻移动目标点 |
| `MoveTargetLocation` | Vector | 战斗移动目标点，由 C++ 根据敌人移动风格计算 |
| `DistanceToTarget` | Float | 敌人与目标的 2D 距离 |
| `bInAttackRange` | Bool | 是否进入 `MovementTuning.AttackRange` |
| `AcceptanceRadius` | Float | 推荐 MoveTo 接受半径 |
| `AlertExpireTime` | Float | 警戒状态结束时间 |
| `LastSeenTargetTime` | Float | 最近一次看见目标的时间 |

## 4. 默认行为树结构

生成器会直接创建行为树，不需要手动搭节点。

```text
Root
  Selector: Main
    Service: Update Enemy Awareness
    Service: Update Enemy Combat Move

    Selector: Combat
      Decorator: EnemyAIState == Combat
      Enemy Attack By Profile
      MoveTo: MoveTargetLocation

    Sequence: Alert
      Decorator: EnemyAIState == Alert
      MoveTo: LastKnownTargetLocation
      Wait: 0.3s

    Sequence: Patrol
      Decorator: EnemyAIState == Patrol
      Update Enemy Patrol Target
      MoveTo: PatrolTargetLocation
      Enemy Patrol Wait
```

说明：

- `Combat` 分支先尝试 `Enemy Attack By Profile`；如果距离或冷却不满足，Task 会失败，行为树会继续执行 `MoveTo MoveTargetLocation`。
- `Alert` 分支不负责超时判断，超时由 `Update Enemy Awareness` 写回 `Patrol`。
- `Patrol` 分支不依赖手动摆巡逻点，默认用出生点附近随机点，并按 `PatrolWaitMin/Max` 随机停留。

## 5. 老鼠如何使用

`DA_Rat.BehaviorTree` 使用：

```text
/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee
```

推荐参数：

| 字段 | 值 |
|---|---:|
| `MovementTuning.ApproachStyle` | `SwarmFlank` |
| `PreferredRange` | 180 |
| `AttackRange` | 150 |
| `AcceptanceRadius` | 60 |
| `RepathInterval` | 0.2 |
| `FlankDistance` | 160 |
| `StrafeChance` | 0.45 |
| `CrowdSeparationWeight` | 2.5 |
| `AwarenessTuning.DetectionRadius` | 900 |
| `AwarenessTuning.CombatEnterRadius` | 650 |
| `AwarenessTuning.AlertBroadcastRadius` | 1200 |
| `AwarenessTuning.PatrolRadius` | 600 |

攻击建议：

| 攻击项 | 距离 | 权重 | 冷却 | 说明 |
|---|---:|---:|---:|---|
| `QuickBite` | 0-170 | 4 | 0.8 | 快速咬击，命中通知配置 20%-30% 流血 |
| `LeapBite` | 120-260 | 1 | 2.5 | 少量跳扑，用于补压迫 |

## 6. 死亡守卫如何使用

`DA_RottenGuard.BehaviorTree` 使用：

```text
/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee
```

推荐参数：

| 字段 | 值 |
|---|---:|
| `MovementTuning.ApproachStyle` | `BruiserHold` |
| `PreferredRange` | 320 |
| `AttackRange` | 260 |
| `AcceptanceRadius` | 110 |
| `RepathInterval` | 0.35 |
| `FlankDistance` | 80 |
| `StrafeChance` | 0.15 |
| `CrowdSeparationWeight` | 3.5 |
| `AwarenessTuning.DetectionRadius` | 900 |
| `AwarenessTuning.CombatEnterRadius` | 650 |
| `AwarenessTuning.AlertBroadcastRadius` | 1200 |
| `AwarenessTuning.PatrolRadius` | 500 |

攻击建议：

| 攻击项 | 距离 | 权重 | 冷却 | 说明 |
|---|---:|---:|---:|---|
| `Sweep` | 0-280 | 3 | 1.6 | 普通扫击 |
| `Heavy` | 180-360 | 2 | 3.5 | 重攻击，建议带前摇、击退和霸体窗口 |

## 7. BP 必要配置

敌人 BP 仍然需要这些基础项：

| 参数 | 推荐 |
|---|---|
| `AI Controller Class` | `AYogAIController` 或其蓝图子类 |
| `Auto Possess AI` | `Placed in World or Spawned` |
| `CharacterDataComponent` | 指向对应 `EnemyData` |
| `Use RVO Avoidance` | 关闭，避让交给 Crowd PathFollowing |

`AYogAIController` 的蓝图子类不是必须的。只要不需要在蓝图里加额外变量或调试显示，直接使用 C++ class 即可。

## 8. 验收流程

编译：

```powershell
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Build\BatchFiles\Build.bat DevKitEditor Win64 Development -Project="X:\Project\Dev01\DevKit.uproject" -WaitMutex -NoHotReload
```

生成器验证：

```powershell
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=EnemyAITemplateGenerator -Preset=DefaultMelee -DryRun
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -run=EnemyAITemplateGenerator -Preset=DefaultMelee
```

自动化测试：

```powershell
Z:\GZA_Software\RealityCapture\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe X:\Project\Dev01\DevKit.uproject -ExecCmds="Automation RunTests DevKit.EnemyAI.DefaultTemplate; Quit" -unattended -nop4 -nosplash -nullrhi
```

PIE 里检查：

- 未发现玩家时，小怪巡逻或原地短暂停留。
- 一个敌人发现玩家后，附近敌人进入警戒或进攻。
- 敌人受到玩家伤害后立刻进攻。
- 进入攻击距离后执行 `Enemy Attack By Profile`。
- 失去目标超过 `LoseTargetDelay + AlertDuration` 后回到巡逻。
