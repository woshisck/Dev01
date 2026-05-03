# 敌人 AI 配置说明

字段含义速查见：`敌人参数中文说明与功能简介.md`

## 已有功能

- `AYogAIController` 已继承 `AModularAIController`，并在项目层启用 Crowd PathFollowing、敌人 DA 行为树启动、战斗移动目标计算。
- `EnemyData.BehaviorTree` 已优先生效；留空时才使用 AIController fallback 行为树。
- `BTService_UpdateEnemyAwareness` 已实现，会维护巡逻、警戒、进攻三态。
- `BTService_UpdateEnemyCombatMove` 已实现，会写入移动目标、距离、攻击距离判断和接受半径。
- `BTTask_EnemyCombatMove` 已实现，会按 `RepathInterval` 节流 MoveTo 请求，避免玩家持续移动时行为树频繁重启 MoveTo。
- `BTTask_UpdateEnemyPatrolTarget` 已实现，无巡逻点配置时会在出生点附近随机巡逻。
- `BTTask_EnemyPatrolWait` 已实现，会按 `EnemyData.AwarenessTuning.PatrolWaitMin/Max` 随机等待。
- `BTTask_EnemyAttackByProfile` 已实现，会按 `EnemyData.AttackProfile` 的 `AttackRole`、距离、权重、冷却和 GA Tag 选招。
- 默认 Combat 分支已拆成固定优先级：技能判断 -> 特殊移动攻击判断 -> 近距离攻击判断 -> 战斗移动。
- 战斗移动已加入战斗槽位锁定、攻击距离迟滞和 `AlreadyAtGoal` 进度修正，避免老鼠在未进攻击距离时原地抖动。
- `AYogAIController` 接管敌人时会显式初始化 `Patrol` 状态、巡逻原点和非战斗黑板值。
- `EnemyAITemplateGenerator` 已实现，可直接生成默认小怪黑板和行为树资产，并把老鼠、死亡守卫 DA 指向默认行为树，同时写入默认移动、感知和攻击配置。
- 生成器会扫描敌人 `AbilityData` 里已有有效蒙太奇 / `MontageConfig` 的攻击 Tag，自动补进 `AttackProfile`；`AttackProfile` 主要管理角色、距离、权重、冷却和特殊移动参数。

## 待做功能

- 在编辑器里运行一次生成器，生成或更新默认行为树资产。
- 在 `DA_Rat`、`DA_RottenGuard` 中按敌人定位微调 `MovementTuning`、`AwarenessTuning`、`AttackProfile`；默认值由生成器先写好。
- 在攻击蒙太奇命中通知里配置老鼠流血、死亡守卫击退和霸体窗口。
- 后续如需要固定巡逻路线，再扩展 PatrolRoute 资产；当前 V1 使用出生点周围随机巡逻。

## 1. 默认小怪 AI 设计

默认小怪使用三态：

| 状态 | 行为 | 进入条件 | 退出条件 |
|---|---|---|---|
| `Patrol` 巡逻 | 在出生点周围随机找点移动，短暂停留后继续巡逻 | 默认状态，或警戒超时 | 看见玩家、收到附近敌人通知、被攻击 |
| `Alert` 警戒 | 前往最后发现玩家的位置，短暂搜索 | 发现玩家但距离较远，或附近敌人广播警戒 | 看见玩家且进入进攻半径，或警戒超时 |
| `Combat` 进攻 | 追击、停距、包围、按攻击配置选招 | 进入进攻半径，或被玩家攻击 | 超出 `CombatExitRadius` 后丢失目标，或长时间完全丢失目标后转警戒 |

关键规则：

- 敌人攻击前会显式进入 `Combat`。
- 敌人受到伤害会立刻进入 `Combat`，目标设为伤害来源。
- 一个敌人发现玩家或被攻击后，会通知 `AlertBroadcastRadius` 内的附近敌人。
- 附近敌人收到通知后进入 `Alert`；如果自己也看到玩家并进入进攻半径，会自动转 `Combat`。
- 已进入 `Combat` 后有状态粘性：短暂丢失视线、玩家刚离开 `CombatEnterRadius`、或攻击动作进行中，都不会立刻退回 `Alert`。

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

同时会给 `DA_Rat` 和 `DA_RottenGuard` 写入默认 `MovementTuning`、`AwarenessTuning`、`AttackProfile`。生成器会按 `AbilityTags` 更新默认覆盖项，并从 `AbilityData.MontageMap / MontageConfigMap` 自动补齐已有动作 Tag；其他自定义攻击项会保留。

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
| `bInAttackRange` | Bool | 是否进入可攻击距离；普通攻击用 `AttackProfile.MaxRange`，远距位移攻击会先检查内置冷却 |
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
      Enemy Attack By Profile: Skill
      Enemy Attack By Profile: SpecialMovement
      Enemy Attack By Profile: CloseMelee
      Enemy Combat Move

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

- `Combat` 分支按固定优先级尝试攻击：`Skill` 用于后续投掷等技能，`SpecialMovement` 用于死亡守卫 HAtk2 这类中远距离前冲，`CloseMelee` 用于普通近战。
- `SpecialMovement` 不会参与近身随机近战；距离小于普通近战范围或 `LungeStartRange` 时，死亡守卫会回到 `CloseMelee` 分支使用 HAtk1。
- `Enemy Combat Move` 不直接观察黑板重启，而是按 `RepathInterval` 节流发起 MoveTo；进入攻击距离后结束，让树重新尝试攻击。
- `Enemy Combat Move` 如果遇到 `AlreadyAtGoal` 但 `DistanceToTarget` 仍大于攻击范围，会把目标点向玩家方向推进一段距离，避免老鼠卡在“已到达但打不到”的状态。
- `Alert` 分支不负责超时判断，超时由 `Update Enemy Awareness` 写回 `Patrol`。
- `Patrol` 分支不依赖手动摆巡逻点，默认用出生点附近随机点，并按 `PatrolWaitMin/Max` 随机停留。

## 4.1 前向转弯移动

默认近战敌人不使用八方向动画。敌人在战斗移动时会先计算理想追击点，再转成角色前方的引导点，表现为“边前进边转弯”。

| 参数 | 用途 |
|---|---|
| `bUseForwardSteering` | 开启前向转弯移动 |
| `ForwardTurnLeadDistance` | 引导点在敌人前方的距离 |
| `MaxTurnYawSpeed` | 每秒最大转向角速度，同时写入 CharacterMovement 的 `RotationRate.Yaw` |
| `MoveTargetSmoothingSpeed` | 引导点位置平滑速度 |
| `SharpTurnAngle` | 大角度转向阈值，后续有转身动画时可用它切动作 |
| `MaxWalkSpeedOverride` | 单个敌人的移动速度覆盖，0 表示使用移动表原值 |
| `CombatSlotLockDuration` | 战斗槽位锁定时间，老鼠默认 1.2 秒，避免侧向点每次 Service Tick 都换 |
| `AttackRangeExitBuffer` | 攻击距离退出缓冲，默认 40；进入攻击用攻击距离，离开攻击状态用攻击距离 + 缓冲 |

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
| `CrowdSeparationWeight` | 2.4 |
| `bUseForwardSteering` | true |
| `ForwardTurnLeadDistance` | 170 |
| `MaxTurnYawSpeed` | 520 |
| `MoveTargetSmoothingSpeed` | 8 |
| `SharpTurnAngle` | 125 |
| `MaxWalkSpeedOverride` | 0 |
| `CombatSlotLockDuration` | 1.2 |
| `AttackRangeExitBuffer` | 40 |
| `AwarenessTuning.DetectionRadius` | 900 |
| `AwarenessTuning.CombatEnterRadius` | 650 |
| `AwarenessTuning.CombatExitRadius` | 1200 |
| `AwarenessTuning.AlertBroadcastRadius` | 1200 |
| `AwarenessTuning.PatrolRadius` | 600 |

攻击建议：

| 攻击项 | AttackRole | 距离 | 权重 | 冷却 | 说明 |
|---|---|---:|---:|---:|---|
| `QuickBite` | `CloseMelee` | 0-170 | 2 | 0.8 | 快速咬击，命中通知配置 20%-30% 流血 |
| `Bite` | `CloseMelee` | 0-180 | 1 | 1.1 | 普通咬击，用于丰富近身节奏 |

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
| `FlankDistance` | 120 |
| `StrafeChance` | 0.15 |
| `CrowdSeparationWeight` | 3.0 |
| `bUseForwardSteering` | true |
| `ForwardTurnLeadDistance` | 240 |
| `MaxTurnYawSpeed` | 260 |
| `MoveTargetSmoothingSpeed` | 5 |
| `SharpTurnAngle` | 105 |
| `MaxWalkSpeedOverride` | 420 |
| `CombatSlotLockDuration` | 0.4 |
| `AttackRangeExitBuffer` | 40 |
| `AwarenessTuning.DetectionRadius` | 900 |
| `AwarenessTuning.CombatEnterRadius` | 650 |
| `AwarenessTuning.CombatExitRadius` | 1200 |
| `AwarenessTuning.AlertBroadcastRadius` | 1200 |
| `AwarenessTuning.PatrolRadius` | 600 |

攻击建议：

| 攻击项 | AttackRole | 距离 | 权重 | 冷却 | 说明 |
|---|---|---:|---:|---:|---|
| `Sweep` | `CloseMelee` | 0-290 | 2 | 1.6 | 普通扫击 |
| `Heavy` | `SpecialMovement` | 160-650 | 1 | 2.2 | 中远距离重攻击，距离大于 300 时会代码前冲，建议带前摇、击退和霸体窗口 |
| `LAtk*` | `CloseMelee` | 0-260 | 2.5 | 0.9 | 由 `AbilityData` 中有效的 `Enemy.Melee.LAtk*` 自动补齐，用于近身丰富普通攻击池 |

`Heavy` 默认前冲参数：

| 字段 | 值 |
|---|---:|
| `AttackMovementMode` | `RadialLunge` |
| `LungeStartRange` | 300 |
| `LungeDistance` | 280 |
| `LungeDuration` | 0.35 |
| `LungeStopDistance` | 170 |
| `MovementAttackRangeMultiplier` | 2.5 |
| `MovementAttackCooldown` | 10.0 |

说明：

- `Heavy` 的远距位移攻击范围默认按 `MovementTuning.AttackRange * MovementAttackRangeMultiplier` 计算，死亡守卫即 `260 * 2.5 = 650`。
- `Heavy` 只在 `SpecialMovement` 分支里判断；进入普通近战距离后，默认不再抢普通近战分支。
- 远距位移攻击成功发动后，会进入 `MovementAttackCooldown` 秒内置冷却；冷却期间如果玩家仍在中远距离，敌人会继续 `Enemy Combat Move` 追近，不会在攻击 Task 和 MoveTo 之间反复切换。
- 如果玩家离开远距位移攻击范围，内置冷却会重置，重新进入范围后可再次发动一次远距位移攻击。

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

PIE 中需要检查敌人移动抖动时，可以临时开启移动平滑日志：

```text
DevKit.EnemyAI.MoveSmoothLog 1
```

如果还需要看每次 `MoveTo` 重发原因，使用：

```text
DevKit.EnemyAI.MoveSmoothLog 2
```

需要判断敌人是否切错状态，开启状态日志：

```text
DevKit.EnemyAI.StateLog 1
```

如果要同时看感知采样，也就是敌人是否还看见玩家、距离是否超过进攻半径：

```text
DevKit.EnemyAI.StateLog 2
```

需要判断“为什么不攻击”，开启攻击决策日志：

```text
DevKit.EnemyAI.AttackDecisionLog 1
```

需要单独看远距位移攻击冷却、重置和范围门控：

```text
DevKit.EnemyAI.MovementAttackLog 1
```

常见排查：

- `[EnemyMovementAttack] Event=RangeIgnored Reason=NoValidAbilityTag` 表示 `AttackProfile` 里配置了远距离位移攻击，但当前敌人的 `AbilityData` 没有给该 `AbilityTags` 配有效蒙太奇或 `MontageConfig`。这种情况下远距离攻击不会再把 `bInAttackRange` 拉成 true，敌人会继续靠近到其他有效攻击范围。
- `[EnemyAIAttack] Reason=NoValidAbilityTag ConfiguredTags=...` 表示攻击任务已经进入选招，但对应 Tag 不可用。死亡守卫如果要使用中距前冲，优先检查 `DA_RottenGuard.AbilityData` 是否配置了 `Enemy.Melee.HAtk2`；如果暂时没有 HAtk2 动作，就把 `Heavy.AbilityTags` 改成一个已配置的有效攻击 Tag，或暂时禁用 `Heavy`。
- `[EnemyMoveSmooth] AttackRange=650 InRange=1` 但随后一直 `NoValidAbilityTag`，一般就是远距离攻击 Tag 没配对。修正后如果 Tag 仍无效，`AttackRange` 会回落到有效近战范围，AI 会继续 `Enemy Combat Move`。
- `[EnemyMoveSmooth] MoveAttack{Ready=0,AttackCooldown=...}` 表示远距位移攻击的普通攻击冷却还没结束；此时 650 远距不会继续把敌人判定为可攻击，AI 会追近到普通近战范围。
- 带 `RadialLunge` 的攻击只在 `DistanceToTarget >= LungeStartRange` 时参与选招；`LungeStartRange` 内由普通近战攻击负责，避免 HAtk2 和 HAtk1 在近距离来回抢分支。
- 攻击 GA 激活期间，Controller 会冻结 combat move 目标并停止当前路径移动；这段时间日志里的 `MoveTarget` 会保持在敌人当前位置附近，不应再因为玩家移动大幅跳变。
- 如果 `PreferredRange` 大于当前有效攻击距离，C++ 会按 `EffectiveAttackRange - AcceptanceRadius` 动态收缩战斗移动停距，避免死亡守卫停在 320 左右但只有 290 扫击范围可用。
- 前向转弯移动时，`AcceptanceRadius` 仍作为攻击停距/动态停距计算参考，但 `Enemy Combat Move` 的实际 MoveTo 接受半径会临时收缩到约 `30-55`，避免过程引导点被过大的接受半径提前判定 `AlreadyAtGoal`。

可调日志参数：

| CVar | 默认 | 用途 |
|---|---:|---|
| `DevKit.EnemyAI.MoveSmoothLog` | 1 | 0 关闭；1 输出汇总；2 额外输出每次 MoveTo 请求 |
| `DevKit.EnemyAI.MoveSmoothLogInterval` | 0.35 | 单个 AI 汇总日志间隔 |
| `DevKit.EnemyAI.MoveSmoothWarnTargetJump` | 140 | `MoveTargetLocation` 单次跳变超过此值时标记 `Unstable` |
| `DevKit.EnemyAI.MoveSmoothWarnYawDelta` | 75 | 期望转向角过大时标记 `Unstable` |
| `DevKit.EnemyAI.MoveSmoothWarnYawRate` | 540 | Actor 实际转向角速度过高时标记 `Unstable` |
| `DevKit.EnemyAI.StateLog` | 1 | 0 关闭；1 输出状态切换；2 额外输出感知采样 |
| `DevKit.EnemyAI.StateLogInterval` | 0.5 | 单个 AI 感知采样日志间隔 |
| `DevKit.EnemyAI.AttackDecisionLog` | 1 | 0 关闭；1 输出攻击选择和失败原因 |
| `DevKit.EnemyAI.MovementAttackLog` | 1 | 0 关闭；1 输出远距位移攻击冷却启动和重置 |

日志重点看这些字段：

| 字段 | 判断方式 |
|---|---|
| `MoveTargetDelta` 很大 | C++ 目标点在跳，优先调高 `MoveTargetSmoothingSpeed` 或降低绕侧幅度 |
| `DesiredYawDelta` 长期很大 | 敌人一直在急转，优先降低侧向点变化或提高 `ForwardTurnLeadDistance` |
| `ActorYawRate` 很大 | 角色旋转过快，优先降低 `MaxTurnYawSpeed` 或检查动画蓝图旋转 |
| `MoveReq` 很高 / `LastReqInterval` 很小 | MoveTo 重发过密，优先提高 `RepathInterval` 或 `TargetRefreshDistance` |
| `Path` 在 `Idle/Moving` 间频繁跳 | 行为树或攻击距离反复中断移动，需要看 `InRange` 和攻击冷却 |
| `MoveAttack.ValidAbility=0` | 远距位移攻击的 Tag 没有在当前 `AbilityData` 中配有效蒙太奇或 `MontageConfig`，不会扩展攻击距离 |
| `Acceptance` / `BBAcc` | `Acceptance` 是最近一次 MoveTo 使用的接受半径，`BBAcc` 是当前黑板写入值；前向转弯移动时通常应小于 DA 里的 `AcceptanceRadius` |

状态日志重点看：

| 日志 | 判断方式 |
|---|---|
| `[EnemyAIState] ... -> Patrol` | 敌人是否因为警戒超时或初始化回到巡逻 |
| `[EnemyAIState] ... -> Alert` | 敌人是否因为丢失目标从进攻退回警戒 |
| `[EnemyAIState:Awareness] CanSee=0` | 视线检测失败，优先检查遮挡、检测半径或 LineOfSight |
| `[EnemyAIAttack] Reason=NoCandidate` | 攻击距离布尔为 true，但具体招式因距离、冷却或 Tag 不可用 |
| `[EnemyAIAttack] SetInRangeFalse=1` | 敌人当前离普通近战过远，本帧会把 `bInAttackRange` 拉回 false，让 `Enemy Combat Move` 继续追近 |
| `[EnemyMovementAttack] Event=Activated` | 远距位移攻击成功发动，并开始 10 秒内置冷却 |
| `[EnemyMovementAttack] Event=ResetCooldown` | 玩家离开远距位移攻击范围，冷却被重置 |

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
