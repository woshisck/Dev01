# Enemy AI Debug Commands

Console variables registered in `Source/DevKit/Private/Controller/YogAIController.cpp`. Type them in the in-game console (`~`); run a command with no value to print its current value.

## Awareness

| Command | Default | Description |
|---|---|---|
| `DevKit.EnemyAI.DrawAwareness <0\|1>` | `0` | `1` draws every enemy's detection annulus and view cone in red `(1,0,0)`. Compiled out when `ENABLE_DRAW_DEBUG` is off (shipping). |
| `DevKit.EnemyAI.StateLog <0\|1\|2>` | `0` | `0` off, `1` state transitions, `2` state transitions plus periodic awareness samples. |
| `DevKit.EnemyAI.StateLogInterval <seconds>` | `0.5` | Seconds between awareness sample logs per AI. Only used when `StateLog >= 2`. |

### DrawAwareness shape

Source fields live in `FEnemyAIAwarenessTuning` (`Source/DevKit/Public/Data/EnemyData.h`), edited on the EnemyData asset under `AI|Awareness`.

| Part | Drawn from | Condition |
|---|---|---|
| Outer arc | `DetectionRadius` | always |
| Inner arc | `DetectionInnerRadius` | only when above 0 |
| Cone edges | both radii at `±DetectionHalfAngle` | only when `DetectionHalfAngle < 180` |

Notes:
- The shape is drawn from `UpdateAwarenessBlackboard`, so it only appears while the awareness evaluator is ticking for that enemy. Lines live 0.1 s.
- It shows the acquisition zone. An enemy already in Combat ignores the inner radius and the cone.

## Movement smoothing

| Command | Default | Description |
|---|---|---|
| `DevKit.EnemyAI.MoveSmoothLog <0\|1\|2>` | `0` | `0` off, `1` summary, `2` summary plus MoveTo requests. |
| `DevKit.EnemyAI.MoveSmoothLogInterval <seconds>` | `0.35` | Seconds between summary logs per AI. |
| `DevKit.EnemyAI.MoveSmoothWarnTargetJump <units>` | `140` | Move target delta that flags a sample as unstable. |
| `DevKit.EnemyAI.MoveSmoothWarnYawDelta <degrees>` | `75` | Desired yaw delta that flags a sample as unstable. |
| `DevKit.EnemyAI.MoveSmoothWarnYawRate <deg/s>` | `540` | Actor yaw rate that flags a sample as unstable. |

## Log categories

| Category | Used by |
|---|---|
| `LogEnemyAIState` | `StateLog` output |
| `LogEnemyMoveSmooth` | `MoveSmoothLog` output |
