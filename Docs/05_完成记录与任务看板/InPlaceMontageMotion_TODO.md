# In-Place Montage Motion (Move Distance + Rotation Notify States) — Plan

| Item | Content |
| --- | --- |
| Scope | Ability montage movement: replace anim root motion / Motion Warping with in-place anims + two notify states |
| Audience | Programmers (implementation), Animators / designers (montage authoring) |
| Related docs | [EnemyLootableCorpse_TODO.md](EnemyLootableCorpse_TODO.md) |
| Last updated | 2026-11-03 |
| Status | **Plan only. No code written, nothing compiled or PIE-verified.** Open questions at the bottom. |

## Decision

All ability montages become **in-place** animations. Displacement and turning are authored per montage with two
`AnimNotifyState`s instead of baked root motion:

| Notify state | Controls |
| --- | --- |
| `ANS_MoveDistance` | How far the character travels inside the window, and the shape of that travel (curve) |
| `ANS_RotateOverride` | How fast and toward what the character turns inside the window |

Why not root motion + Motion Warping: anim root motion overwrites `Velocity` and skips `PhysicsRotation` unless
`bAllowPhysicsRotationDuringAnimRootMotion` is set (engine `CharacterMovementComponent.cpp`, `PerformMovement`), so
player input cannot steer a swing, and skeletons without a real root bone (e.g. the rat) cannot use it at all.

## Current state (verified in code)

| Piece | Location | Note |
| --- | --- | --- |
| Motion Warping redirect | `ANS_AttackMotionWarpRedirect`, `AYogPlayerControllerBase::SetupAttackMotionWarpTarget` | Sets `LastInputDirection` as a side effect — keep that part |
| Curve-driven root motion source | `FYogRootMotionSource_MontageCurveMove` | Samples a `MoveDistance` curve on the **montage asset** by playback position; direction re-read from actor forward each tick |
| Source applied only for enemies | `UGA_MeleeAttack::TryStartEnemyMontageCurveMove` / `StopEnemyCurveMove` | ID stored on the ability |
| Existing rotate notify | `UANS_AttackRotate` | Stateless, rotates toward last move input at a fixed deg/s via `SetActorRotation` |
| Custom CMC | `UYogCharacterMovementComponent` | Overrides `GetMaxSpeed` / `GetDeltaRotation` but both just call `Super` |
| Movement block | `UYogAbilitySystemComponent` (~line 700) | `Block.Movement` calls `Owner->DisableMovement()`, except HitReact which calls `BlockMovementControl()` |

## Q: Can a notify state read a curve?

Yes. Three options:

| Option | How | Good for | Catch |
| --- | --- | --- | --- |
| A. `FRuntimeFloatCurve` on the notify | UPROPERTY on the ANS, inline curve editor in Details | A curve unique to one notify instance | Not network-serializable as data |
| B. `UCurveFloat` asset reference | `TObjectPtr<UCurveFloat>` on the ANS | One curve shared across many montages | One extra asset per curve shape |
| C. Curve on the montage | `Montage->EvaluateCurveData(Name, Context)` | Curves keyed to montage time | One curve per montage, not per window; must live on the montage itself, not inner sequences |

**Chosen: B (`UCurveFloat` asset reference) only.** Multiplayer may be needed later, and B is the only option that
serializes cheaply: `NetSerialize` writes one `UCurveFloat*` and a saved `Content/` asset resolves to the same object
on both machines (the engine's `FRootMotionSource_RadialForce` / `MoveToDynamicForce` do the same). A would require
serializing every key of an inline curve. C is network-safe but gives one curve per montage, so two windows in the
same montage could not have different shapes.

X axis is normalized 0-1 over the notify window, Y axis is the fraction of total distance, so one curve shape works
for windows of any length.

Rules that come with B:

- Curves must be saved assets under `Content/`. A runtime-created or transient curve will not resolve on the other
  machine.
- Share curves by shape and name them accordingly (e.g. `CV_Move_EaseOut`, `CV_Move_Lunge`), so the asset count stays
  small.
- Server and client must build the source from the same notify inputs (distance, curve asset, direction mode).
  Under CMC prediction, any mismatch shows up as a correction, not just a wrong curve.

Window progress comes from `EventReference.GetNotify()` (`FAnimNotifyEvent::GetTriggerTime()`, `GetEndTriggerTime()`,
`GetDuration()`) and the montage playback position. Do **not** accumulate `FrameDeltaTime`: reading the montage
position keeps motion locked to the animation under HitStop and attack-speed changes.

**Notify state objects are shared per asset, not per actor.** No per-run state as members on the ANS (the existing
`ANS_AutoTarget::CachedTarget` has this problem). Runtime state lives in the root motion source or on the movement
component.

## Work breakdown

### 1. Generalize the root motion source (~0.5-1 day, low risk)

Extend `FYogRootMotionSource_MontageCurveMove` (or add a sibling) with:

| Field | Purpose |
| --- | --- |
| `WindowStart`, `WindowDuration` | Map montage position to normalized progress |
| `TotalDistance` | Scale for the curve's Y fraction |
| `TObjectPtr<UCurveFloat>` curve (option B) | Shape of travel |
| Direction mode | Live actor forward / locked at begin / move input |

- Keep the existing `MoveDistance` montage-curve path working so enemy lunges do not regress.
- Mark the source `MarkedForRemoval` when the montage instance is gone or progress reaches 1. Today it emits zero
  velocity until something removes it.
- `NetSerialize` must carry the new fields (window, distance, direction mode, curve asset pointer) so the source is
  ready for multiplayer even though nothing replicates yet. Also update `Clone`, `Matches` and `UpdateStateFrom...`
  overrides to cover them.

### 2. `ANS_MoveDistance` (~0.5 day, low risk, depends on step 1)

| Property | Notes |
| --- | --- |
| Distance (cm) | Negative allowed for retreats |
| Curve | `UCurveFloat` asset (option B); empty means linear |
| Direction mode | Default: live actor forward, so it composes with the rotation notify |

- `NotifyBegin`: build the source, give it a unique `InstanceName`, apply it.
- `NotifyEnd`: remove it **by name**, not by stored ID (the ANS is shared).
- Safety net: also remove in the ability's `EndAbility`. Whether `NotifyEnd` fires reliably on an interrupted montage
  is **not verified** — check before relying on it.

### 3. `ANS_RotateOverride` (~0.25 day, low risk)

| Property | Notes |
| --- | --- |
| Yaw speed (deg/s) | 0 locks facing |
| Optional speed curve | Over normalized window time |
| Facing source | Move input (what `ANS_AttackRotate` does), mouse aim, or locked |

- Implement in `NotifyTick` with `SetActorRotation`, like `ANS_AttackRotate`. Stateless, so no shared-object issue.
- Supersedes `ANS_AttackRotate`. Keep the old class until existing montages are migrated, then delete it.

### 4. Movement-mode policy (size unknown, **high risk**)

Root motion sources only run in modes that execute physics. `DisableMovement()` sets `MOVE_None`, where
`ANS_MoveDistance` would silently do nothing. The comment at `YogCharacterBase.cpp` `UpdateCharacterMovement` already
records that disabling movement also disables montage root motion.

**Not yet checked:** which state tags the player's attack abilities apply, and whether any of them resolve to the
`Block.Movement` -> `DisableMovement()` branch. If they do, route attack states through `BlockMovementControl()`
(keeps the movement mode, ignores input only). That touches shared StateConflict logic, so read the tag setup first.

Also check `bMovable`: `AYogPlayerControllerBase::Move` returns early when it is false. It only affects the
walk-input path, not root motion sources.

### 5. Montage migration

- Re-export or flag each attack anim as in-place. `Content/Art` is gitignored, so these changes never appear in
  `git status`.
- Add both notify states to every montage by hand (no MCP tool edits notifies).
- Retire `UMotionWarpingComponent` usage and `ANS_AttackMotionWarpRedirect` after migration. Keep the
  `LastInputDirection` assignment from `SetupAttackMotionWarpTarget`: `ResolveAttackDirection` and dash read it.

### 6. Tests

Add automation tests for the source's progress math: window mapping, curve endpoints, rewound position, removal on
completion.

## Effort summary

| Step | Effort | Risk |
| --- | --- | --- |
| 1. Source generalization | 0.5-1 day | Low |
| 2. `ANS_MoveDistance` | 0.5 day | Low |
| 3. `ANS_RotateOverride` | 0.25 day | Low |
| 4. Movement-mode policy | Unknown until tags are checked | **High** |
| 5. Montage migration | Scales with montage count | Manual, low risk |
| 6. Tests | 0.5 day | Low |

## Open questions

| Question | Why it matters |
| --- | --- |
| Is multiplayer required? | Answered for the curve: B is chosen now so multiplayer stays possible. Still open: whether the notify must run on both server and client, and whether prediction corrections are acceptable |
| Which tags do player attack abilities apply? | Step 4; could change the approach |
| Does the player skeleton matter? | Not for this plan (in-place), but relevant if any attack keeps root motion |
| Does `NotifyEnd` fire on interrupted montages? | Decides how much the `EndAbility` safety net must carry |

## Pitfalls to remember

- `UBaseAttributeSet` writes `MaxWalkSpeed` whenever the MoveSpeed attribute changes. Any speed override must be a
  multiplier or a counted stack, never a cached-and-restored value, or a buff landing mid-window goes stale.
- Overlapping notify windows need a counted stack, not a single saved value.
- Enemies set their own turn rate and speed in `YogAIController.cpp` (~lines 518-525). Either skip AI characters in
  the new states or share one override stack so those settings restore correctly.
