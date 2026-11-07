# Player Locomotion Redesign (Aim Target, 180° Reverse Pause, In-Place) — Design

| Item | Content |
| --- | --- |
| Scope | Player locomotion in the Animation Blueprint: aim target (Auto / Focus), Free / Locked frame, velocity-driven motion state (incl. pivot), in-place clips. **Dash is out of scope** (see "Dash: deferred") |
| Audience | Programmers (C++ AnimInstance, aim target), Animators / TAs (ABP, clip authoring) |
| Related docs | [InPlaceMontageMotion_TODO.md](InPlaceMontageMotion_TODO.md) (ability montages follow the same in-place rule) |
| Last updated | 2026-11-06 |
| Status | **C++ written 2026-11-04 (implementation plan steps 1, 3, 4, 5, 8). Not compiled, not PIE-verified.** ABP work (step 2), clips (step 6) and play-rate matching (step 7) are not started. See "Implementation status" below. Existing assets were read through the editor MCP on 2026-11-03 (function graphs, defaults, data assets). The AnimGraph and state machine cannot be read through MCP (see "What MCP can and cannot do") |

## Requirements

| # | Requirement | Interpretation used here |
| --- | --- | --- |
| 1 | Player aims at a target character | While a target is locked the body faces it, and movement becomes strafing relative to that facing. Target selection is a **mode**: *Focus* (player-chosen, sticky) or *Auto* (nearest) |
| 2 | "Back-front pause" | A **reverse pause**: when the player suddenly reverses direction by ~180°, a dedicated in-place animation plays (plant, brief pause, push off the other way) |
| 3 | All animation in-place, no root motion | Capsule motion comes only from `UCharacterMovementComponent`. Exceptions are listed in the register below (currently none required) |

## Decisions already made

| Topic | Decision |
| --- | --- |
| Dash | Left alone in this work. It will be built later as a **montage in a weapon ability**, not by migrating `GA_PlayerDash` |
| `SacrificeRuneComponent::NotifyDashExecuted` | Deprecated, will not be used again. No design here depends on it |
| Aim-locked dash with no input | Back-step away from the target (recorded for the future dash, see "Dash: deferred") |
| Dash steering | No free steering for now |
| Focus-mode lock / switch inputs, gamepad mapping | Mapped later. Not designed here |

## Current state (verified in code)

| Piece | Location | Note |
| --- | --- | --- |
| Move input | `AYogPlayerControllerBase::Move` | Rotates input by -45° (fixed isometric camera), calls `AddMovementInput`. The yaw-setting code is commented out, so **facing comes from CMC flags** set in Blueprint defaults. Early-returns when input is blocked or `bMovable` is false |
| Last input direction | `APlayerCharacterBase::LastInputDirection` (updated in `Move`) | Persists after the stick is released |
| AnimInstance (C++) | `UYogAnimInstance` | Only maps gameplay tags and holds an unused `GroundDistance`. **No speed / direction / aim variables exist in C++** |
| Player ABP chain (MCP) | `B_PlayerOne` -> `ABP_Character_Elian_Base` -> `ABP_YogCharacter_Base` -> `UYogAnimInstance` | `ABP_Character_Elian_Base` is nearly empty: only an empty `EventBlueprintUpdateAnimation`. **The locomotion lives in `ABP_YogCharacter_Base`** |
| ABP base users (MCP) | Referencers of `ABP_YogCharacter_Base` | `ABP_Character_Elian_Base` (player), `B_YogChar`, and a Market asset `ABP_BlueGuy`. Changing the base affects those too. `B_YogChar` has no referencers itself, so it may be unused |
| Existing locomotion (MCP) | `ABP_YogCharacter_Base` | One state machine `LocomotionSM` with a single state `Idle/Walk`, driven by the `WeaponLayer_BS_Idle/Walk` layer function (implemented by `ABP_CommonLayer` with `BS_YogCC_Elian_Common`). Speed-only, no direction |
| Existing ABP variables (MCP) | `ABP_YogCharacter_Base` | Lyra-style, computed in Blueprint: `Speed`, `HasVelocity`, `WorldVelocity`, `LocalVelocity2D`, `LocalVelocityDirectionAngle`, `WorldRotation`, `YawDeltaSpeed`, `Was Moving Last Update`, `IsFirstUpdate`. Updated from `BlueprintThreadSafeUpdateAnimation` via `UpdateLocationData / UpdateRotationData / UpdateVelocityData`. `UpdateAccelerationData`, `UpdateCharacterStateData` and `OnUpdate` are empty |
| CMC defaults on `B_PlayerOne` (MCP) | `Default__B_PlayerOne_C:CharMoveComp` | `bOrientRotationToMovement = true`, `bUseControllerDesiredRotation = false`, `bUseControllerRotationYaw = false`, `RotationRate.Yaw = 360`, `MaxWalkSpeed = 600`, `BrakingDecelerationWalking = 2048`, `MaxAcceleration = 2048`, `GroundFriction = 8`. **These are Blueprint defaults only.** `AYogCharacterBase::InitializeMovement` overwrites `MaxWalkSpeed`, braking, `MaxAcceleration` and `RotationRate` from the character-data movement row at init. The runtime values were not read |
| Player attack tags (code) | `UGA_MeleeAttack` constructor | `AbilityTags` and `ActivationOwnedTags` get `Character.State.Skill.Attack`, and `AbilityTags` also gets `PlayerState.AbilityCast.Attack`. Dash gives `Character.State.Movement.Dash` (in `ActivationOwnedTags`). Other player abilities (`BGA_PlayerSpecialAttack`, `BGA_Player_FinisherAttack`) are Blueprints and were not read |
| StateConflict (MCP) | `DA_Base_StateConflict_Initial`, loaded through `UDevAssetManager` from `DefaultGame.ini` (the player's own `ConflictTable` is `None`) | `Block.Movement` contains only `Buff.Status.Dead`. `Block.AI` contains HitReact, Knockback and Dead. Rules: HitReact blocks Dash, Dash blocks LightAtk and HeavyAtk, Dead blocks and cancels `Action` / `Enemy`, Knockback blocks and cancels `Action.Attack`. **Dash and attacks do not map to `Block.Movement`** |
| Attack-time targeting | `UANS_AutoTarget` | Nearest hostile in radius + cone, attack time only. Caches the target on the shared notify object (known issue) |
| Attack turning | `UANS_AttackRotate` | Rotates toward move input at a fixed deg/s inside an attack window |
| Dash entry | `AYogPlayerControllerBase::Dash` | Rotates the actor to `LastInputDirection` before activating the ability. Relevant here only because it is the one place the controller sets facing directly |
| Movement block | `UYogAbilitySystemComponent` (~line 695) | Hard-coded movement-block tags are HitReact, Dead and Knockback. `BlockMovementControl()` is used for HitReact, `DisableMovement()` for the rest |
| Other anim assets | `Content/Code/Characters/AnimationBP/` | `AnimEnum_Cardinal` (Forward / Backward / Left / Right, read from the asset's display-name entries), `AnimStruct_Cardinal` (four AnimSequence slots: Forward, Backward, Left, Right), `AnimEnum_RootYawOffset`, `AnimStruct_TurnInPlace`, `BS_YogCC_Elian_*`. The weapon layer ABPs are `ABP_CommonLayer`, `ABP_THSwordLayer`, `ABP_Nemesis_SwordLayer`, `ABP_THHammer` |
| Player skeleton (MCP) | `SK_Charactor_Elian` | **Has a real `root` bone** plus `ik_foot_root / ik_foot_l / ik_foot_r / ik_hand_*` and `pelvis`. IK foot bones exist, so Stride Warping is feasible |

## Design

### Overview

```
 Input (Move)        Aim target (mode: Focus / Auto)         CMC
     |                          |                              |
     |  AddMovementInput        | AimTarget (weak ptr)         | velocity, accel
     v                          v                              v
 +------------------------------------------------------------------+
 | APlayerCharacterBase                                             |
 |   Facing:   target valid -> yaw toward target (rate-limited)     |
 |             no target    -> orient to movement                   |
 |             ability owns facing (attack / dash) -> suspended     |
 |   Reverse detector: input vs velocity ~180 -> bLocoIsPivoting    |
 +------------------------------------------------------------------+
                               |  (read every frame, no state in ABP)
                               v
 +------------------------------------------------------------------+
 | UYogAnimInstance::NativeUpdateAnimation  (Loco* values)          |
 |   LocoMoveAngle, LocoMoveCardinal, bLocoIsMoving,                |
 |   bLocoHasAimTarget, bLocoIsPivoting, LocoPivotType, ...         |
 |   (Speed / HasVelocity stay in the ABP's own update functions)   |
 +------------------------------------------------------------------+
                               |
                               v
 ABP: Idle -> Start -> Loop -> Stop      Pivot (from Loop / Start) -> Start
```

Ownership rule: **C++ owns the new gameplay decisions** (aim target, facing, pivot, start / stop latches). **The ABP keeps the movement math it already has** (`Speed`, `LocalVelocityDirectionAngle`, `HasVelocity` in `ABP_YogCharacter_Base`) and reads the new C++ values. Reasons:

- `ABP_YogCharacter_Base` already computes velocity data in Blueprint on the worker thread. Re-implementing it in C++ gives two sources of truth.
- Pivot, aim and latches need gameplay state (input, target, abilities) that the ABP should not own.
- The AnimGraph and state machine cannot be read or edited through MCP, but function graphs can. So keep ABP logic to thin reads, and keep decisions in C++ where they can be tested.

**Naming rule:** `ABP_YogCharacter_Base` derives from `UYogAnimInstance`, and it already has Blueprint variables named `Speed`, `HasVelocity`, `LocalVelocityDirectionAngle`, `WorldVelocity`, `LocalVelocity2D`. A C++ `UPROPERTY` with the same name conflicts with an inherited Blueprint variable of the same name and breaks the ABP compile. New C++ members must not reuse those names. Prefix the new ones with `Loco` (for example `LocoMoveCardinal`).

### 1. Aim target (two modes)

| Aspect | Design |
| --- | --- |
| Mode | `EAimTargetMode { Auto, Focus }`, switchable at runtime |
| Storage | `TWeakObjectPtr<AActor> AimTarget` on `APlayerCharacterBase` (or a small component). `IsValid()` before use |
| Auto | Re-evaluated at ~5-10 Hz. Nearest living hostile inside `AimAcquireRadius`. Stickiness: keep the current target until it dies, leaves `AimReleaseRadius` (> acquire radius), or a closer candidate wins by a margin |
| Focus | Player locks the best candidate (nearest to the facing direction, then nearest). Target is **kept** until the player releases it, it dies, or it leaves `AimReleaseRadius`. A switch request cycles to the next candidate |
| Focus target lost | Config choice: drop to no-target, or fall back to Auto (open question 2) |
| Candidate filter | Reuse the filtering in `UANS_AutoTarget::FindBestTarget` (single source), minus the cone. Do not make a second copy |
| No target | Orient-to-movement, so direction relative to facing is always ~0 |
| Inputs | C++ exposes `SetAimMode`, `LockFocusTarget`, `ReleaseFocusTarget`, `CycleFocusTarget`. **Input mapping is deferred**, so nothing here touches IA assets or `IMC_YogPlayerBase` yet |

### 2. Facing control

| State | Who rotates the actor |
| --- | --- |
| Free movement, target locked | Locomotion facing: yaw toward target at `AimTurnRateYaw` deg/s. `bOrientRotationToMovement = false` |
| Free movement, no target | CMC `bOrientRotationToMovement = true` (current behavior) |
| Attack / skill | Ability owns facing (`ANS_AutoTarget` / `ANS_RotateOverride`). Locomotion facing suspended |
| Dash | Suspended while the dash ability is active (same rule as attacks). Dash behavior itself is deferred |
| Reverse pause | Free mode: rotates 180° over the pause (below). Aim mode: facing unchanged |
| Stunned / dead / conversation | No rotation |

"Suspended" is expressed with a gameplay-tag check on the ASC. Verified in code: melee attacks (`UGA_MeleeAttack`) own `Character.State.Skill.Attack` while active, and dash owns `Character.State.Movement.Dash`. Use `Character.State.Skill` and `Character.State.Movement.Dash` as the suspend set. The Blueprint abilities (`BGA_PlayerSpecialAttack`, `BGA_Player_FinisherAttack`) were not read, so confirm they also grant a tag under `Character.State.Skill`. If not, add the tag in their class defaults.

Note: the StateConflict table does **not** put attacks or dash under `Block.Movement`. Only `Buff.Status.Dead` does. So during an attack the player still receives movement input and `AddMovementInput` keeps working, and locomotion facing must be suspended by tag, not by a movement block.

Do **not** write `RotationRate` from here: `InitializeMovement` and `SetEnableRotationRate` already write it from data.

### 3. Direction relative to facing

What the ABP already has: `UpdateVelocityData` sets `LocalVelocity2D` (world velocity unrotated by the actor rotation, so it is **already in facing space**), `Speed` (its XY length) and `HasVelocity`. There is also a `LocalVelocityDirectionAngle` variable, but none of the function graphs that were read set it (`UpdateVelocityData` does not), so **do not rely on it** until it is found being written. A thread-safe update that sets it from `LocalVelocity2D` is the cheap fix if the ABP wants it.

What C++ adds, because the cardinal needs **state** (hysteresis, latches) and that state belongs next to the gameplay decisions. Computed in `NativeUpdateAnimation` from velocity and actor yaw, with `Loco`-prefixed names (see the naming rule):

```cpp
const FVector Velocity2D = Character->GetVelocity() * FVector(1.f, 1.f, 0.f);
const float Speed2D = Velocity2D.Size();
LocoMoveAngle = Speed2D > KINDA_SMALL_NUMBER
	? UKismetAnimationLibrary::CalculateDirection(Velocity2D, Character->GetActorRotation())
	: LocoMoveAngle; // hold the last angle at rest, do not snap to 0 (see the Stop latch below)
```

Use **velocity** for the angle (matches what the feet do). Use **acceleration** (`GetCurrentAcceleration`) for `bLocoIsMoving` so the first frame of a move is detected.

**Why the angle holds at rest:** returning 0 at near-zero speed would snap the cardinal to Front exactly when the Stop clip must be chosen. `LocoStopCardinal` is captured at the moment input is released, from the last valid cardinal, and stays until the next move starts. It is the same pattern as `LocoStartAngle`.

| Cardinal | Angle band (deg) | Clip family |
| --- | --- | --- |
| Front | -45 .. 45 | Run forward |
| Right | 45 .. 135 | Strafe right |
| Back | 135 .. 180 and -180 .. -135 | Backpedal |
| Left | -135 .. -45 | Strafe left |

Hysteresis: switch only after passing a band edge by `CardinalDeadZone` (start 15°). `AnimEnum_Cardinal` (Forward / Backward / Left / Right) and `AnimStruct_Cardinal` (four AnimSequence slots) already exist as Blueprint assets and match these four bands, but C++ cannot reference a Blueprint enum, so see the open item in "Not verified". Write the angle -> cardinal logic as one pure function and unit-test it. The future dash can reuse it.

### 4. Motion state and pivot (from CharacterMovement only)

Nothing gates input. The capsule moves exactly as CMC integrates it; `YogLocomotion::NextMotionState` only labels what it is doing, every anim update, from CMC `Velocity` and `GetCurrentAcceleration()`. Result: `LocoMotionState`.

```
            accel, speed < 90% max                  accel, speed >= 90% max
 Idle ------------------------------> Starting --------------------------> Moving
  ^  no accel, speed 0                  ^   \                                 |
  |                                     |    \ accel opposes vel (45 deg),   |
  |                                     |     \ speed >= 50% max, grounded   |
  |                                     |      v                              v
  |                                     +-- Pivoting <------------------------+
  |               vel reaches 0 or flips onto the accel side
  |
  +--- Stopping <--- no accel, still sliding (from any moving state)
```

| Transition | Condition | Why |
| --- | --- | --- |
| -> Pivoting | Acceleration within `LocoPivotAngleTolerance` (45) of opposite the velocity, speed >= `LocoPivotMinSpeedRatio` (0.5) x max speed, on ground, not suspended | CMC is braking the old direction |
| Pivoting -> Starting | Velocity reaches zero **or** its dot with acceleration turns positive | With ground friction CMC swings velocity through zero within a frame or two instead of resting at 0 |
| Pivoting -> Stopping | Input released mid-pivot | Ordinary stop |
| Pivoting -> Starting | Suspended / airborne | An ability owns movement |
| Starting -> Moving | Speed >= `LocoStartToLoopSpeedRatio` (0.9) x max speed | Loop clip takes over at running speed |

`LocoMode` decides the reference frame for **every** directional value (`LocoMoveAngle`, `LocoMoveCardinal`, `LocoStartAngle`, `LocoStopCardinal`, `LocoPivotType`):

| `LocoMode` | When | Frame (Front =) |
| --- | --- | --- |
| `Free` | No aim target | Screen up, from the player camera yaw (`GetViewYaw`, fallback `FallbackViewYaw` = -45) |
| `Locked` | Aim target held | Body facing (= the target) |

`LocoPivotType` is chosen once on entering Pivoting from the pre-reversal `LocoMoveCardinal`: `Front_To_Back` / `Back_To_Front` / `Left_To_Right` / `Right_To_Left`. Screen up -> down in Free mode is `Front_To_Back`; screen right -> left is `Right_To_Left`. `TurnAround_L` / `TurnAround_R` are no longer produced.

`LocoStartAngle` is re-captured from acceleration on every entry to Starting / Moving from Idle, Stopping **or Pivoting**, so the Start clip after a pivot faces the new direction.

Removed (2026-11-06): the input-swallowing reverse pause in `UPlayerLocomotionComponent` (`FilterMoveInput`, `PivotMaxDuration`, `PivotCooldown`, `PivotTurnDuration`, grace window, code-driven 180 turn) and the `AN_PivotEnd` end signal. They froze the player for the clip length (~1.5 s with the authored notifies). `AN_PivotEnd` is now a no-op kept only so the clips still load; remove it from the four `A_INP_*Pivot*` / `*180_Turn*` clips, then delete the class. The pivot clip is now purely cosmetic and must not be longer than the CMC braking time.

**Tuning reference (from the `B_PlayerOne` Blueprint defaults, not runtime values):**

| Quantity | Estimate | Basis |
| --- | --- | --- |
| Stop time from full speed | **~0.11 s** | `MaxWalkSpeed 600`, `BrakingDecelerationWalking 2048`, `GroundFriction 8`, `BrakingFrictionFactor 2` (read from the CMC defaults, `bUseSeparateBrakingFriction` is false). CMC braking applies friction as well as the fixed deceleration, with the friction scaled by the factor: `dv/dt = -16v - 2048`, which reaches zero from 600 in `ln(728/128)/16` s. Estimated from the CMC formula, not measured in PIE |
| 180° turn by CMC alone | ~0.5 s | `RotationRate.Yaw 360` |

What follows from that:

- Pivoting therefore lasts roughly 0.1 s at full speed. Pivot clips should be authored (or play-rate scaled) to that, and blend into Start rather than play to their end. (Older note: the natural stop (~0.11 s) is at or just under the 0.12-0.20 s window, so the capsule is already stopped when the Pivot clip is only partway through. The **braking multiplier is therefore probably unnecessary**, and the pause window is better set by the Pivot clip length than by braking. Keep the multiplier as an optional tuning hook, and add it only if PIE shows the capsule sliding after the clip starts. (Earlier chat estimates of ~0.3 s with a 2x multiplier, and then ~0.15 s, left out ground friction and the braking friction factor. ~0.11 s is the figure that matches the CMC code and the values read from the editor.)
- A 180° turn by CMC rotation takes ~0.5 s, longer than the ~0.11 s brake, so in Free mode the body is still turning when Pivoting ends and Starting begins. The Pivot / Start clips should cover that turn visually.
- `AYogCharacterBase::InitializeMovement` overwrites `MaxWalkSpeed`, braking, `MaxAcceleration` and `RotationRate` from the character-data movement row at init, so the real numbers can differ from the Blueprint defaults. Read the runtime values in PIE before tuning.

### 5. Variables exposed to the ABP

New C++ values on `UYogAnimInstance`, `BlueprintReadOnly`, written only in `NativeUpdateAnimation`. All carry a `Loco` prefix so they cannot collide with the Blueprint variables already inherited by `ABP_YogCharacter_Base`.

| Variable | Type | Meaning |
| --- | --- | --- |
| `LocoMotionState` | enum | Idle / Starting / Moving / Pivoting / Stopping, see section 4 |
| `LocoMode` | enum | Free / Locked, the frame for every directional value |
| `bLocoIsMoving` | bool | Starting, Moving or Pivoting |
| `bLocoIsStopping` | bool | `LocoMotionState == Stopping` |
| `LocoMoveAngle` | float | Velocity direction in the `LocoMode` frame, -180..180. Holds its last value at rest |
| `LocoMoveCardinal` | enum | Front / Back / Left / Right with dead zone. Live, used by Loop |
| `LocoStartAngle` | float | Acceleration angle captured on entering Starting / Moving from Idle, Stopping or Pivoting |
| `LocoStopCardinal` | enum | Cardinal captured when input is released, so the Stop clip is chosen once |
| `bLocoHasAimTarget` | bool | Aim lock active (strafe set vs plain run) |
| `LocoAimYawToTarget` | float | Signed yaw from facing to target, for an optional upper-body / head aim layer |
| `bLocoIsPivoting` | bool | `LocoMotionState == Pivoting` |
| `LocoPivotType` | enum | Chosen on entering Pivoting, see section 4 |
| `bLocoInLock` | bool | An ability owns movement / facing. ABP stays out of Start / Stop / Pivot |

Read from the ABP's existing variables instead of duplicating: `Speed` (play-rate matching, Loop blend), `HasVelocity`, `LocalVelocity2D`.

Weapon layers (`ABP_THSwordLayer`, `ABP_Nemesis_SwordLayer`, `ABP_THHammer`) go through `Anim_Layer_Interface` / `ALI_WeaponLayer`. Each must supply the same clip set or fall back to a default.

### 6. State machine

```
     bLocoIsMoving                      !bLocoIsMoving
 Idle ------------> Start ----------------------------> Stop
  ^                   |                                  |
  |                   v                                  | clip done
  |                 Loop <--(cardinal change: blend)     |
  |                   |   \                              |
  |                   |    \ bLocoIsPivoting             |
  |                   |     v                            |
  |                   |   Pivot --(vel 0/flip)--> Start  |
  +-------------------+----------------------------------+
```

| State | Content | Notes |
| --- | --- | --- |
| Idle | Idle, or aim-idle if `bLocoHasAimTarget` | No turn-in-place needed: the body already faces the target |
| Start | One clip per cardinal from `LocoStartAngle` | Short (4-8 frames). Optional |
| Loop | Blend by `LocoMoveCardinal` (4 clips, or a BlendSpace over `LocoMoveAngle` + `Speed`) | Play rate matched to capsule speed |
| Stop | One clip per cardinal, from `LocoStopCardinal` (latched at release) | Clip length should match braking time. Do not read the live cardinal here: it is meaningless at rest |
| Pivot | Clip chosen from `LocoPivotType` at entry | Exits to Start when `bLocoIsPivoting` drops (velocity hit zero), not when the clip ends |

Rules:

- Moving again during Stop interrupts it (no input lock): Stop -> Start.
- A cardinal change inside Loop blends (0.1-0.2 s). It does not go through Stop / Start, **unless** it is the 180° swap, which is Pivot.
- Abilities flip `bLocoInLock`. Attack and dash montages use a Slot after the locomotion output, which overrides it.

### 7. Foot sliding with no root motion

| Layer | What it does | Cost |
| --- | --- | --- |
| Play rate | `PlayRate = Speed / AuthoredSpeed` per clip, clamped (e.g. 0.7-1.3) | Authored speed per clip must be recorded |
| Stride Warping (Animation Warping plugin) | Adjusts stride to actual speed | Needs IK bones. Optional second pass |

Never hard-code speed: `UBaseAttributeSet` writes `MaxWalkSpeed` when MoveSpeed changes. Always read live `Speed`.

## Dash: deferred

Dash is **not** part of this work. `UGA_PlayerDash` and the controller's `Dash()` stay as they are. The plan is to author dash later as a montage inside a weapon ability. How that weapon-ability dash is wired was not looked at, so none of it is designed here.

Requirements recorded now so they are not lost when that work starts:

| Topic | Recorded decision |
| --- | --- |
| Aim-locked, no input | Back-step away from the target |
| Aim-locked, with input | Dash along the input direction while the body keeps facing the target (step aside). Facing handled by the "Dash" row in the facing table. Dash clip is picked by the dash direction's cardinal relative to facing (shared function from section 3) |
| Steering | None for now. Direction fixed at activation |
| `NotifyDashExecuted` | Deprecated, do not call from the new dash |
| In-place | Should follow the in-place plan (`ANS_MoveDistance`). That plan's direction modes (live forward, locked at begin, move input) do not cover step-aside while facing a target, because forward equals facing. A mode that takes a direction supplied by the ability would be needed. `InPlaceMontageMotion_TODO.md` was not edited |
| Interaction with locomotion | Dash suspends locomotion, which drops Pivoting. Locomotion facing is suspended while a dash ability is active |

One thing worth knowing when that work starts: `AYogPlayerControllerBase::Dash` rotates the actor to the input direction before activating anything. If the new dash is activated through the same path, that rotation would turn the body away from the target and defeat step-aside.

## Root motion register

Default is **no root motion**.

| Animation | Root motion? | Notes |
| --- | --- | --- |
| Idle, Start, Loop, Stop (all cardinals) | **No** | Capsule-driven |
| Pivot (all `LocoPivotType`) | **No** | Capsule brakes under CMC; input is never held. A baked rotating root would be root motion, so it is not used |
| Turn-in-place | **Not used** | Aim lock rotates the actor |
| Dash | **Out of scope** | Today `GA_PlayerDash` uses anim root motion. The future weapon-ability dash is intended to be in-place. Not decided in this doc |
| Player attacks | Today Motion Warping, target **No** | Covered by the in-place plan |
| Hit react / knockback | **Needs decision** | Prefer in-place clip plus displacement from the knockback system |
| Death, interact / pickup | **No** | In-place |

No locomotion animation requires root motion. If one does later (e.g. a hand-animated lunge a distance curve cannot match), add it here with the reason.

Authoring check: every clip is exported with root motion disabled, and the mesh must not drift from the capsule over the clip. The player skeleton (`SK_Charactor_Elian`) has a real `root` bone, so root motion is *possible* on it (unlike the rat), which is why the export check matters: a clip that was authored with a moving root will drift off the capsule instead of being ignored. Enable root motion on a sequence only if it is listed in the register above.

## Clip set

| Clip | Variants | Authored speed (cm/s) |
| --- | --- | --- |
| Idle / aim idle | 1 / 1 | - |
| Start | F / B / L / R (optional) | TBD |
| Loop | F / B / L / R | TBD |
| Stop | F / B / L / R | TBD |
| Pivot (both modes) | F->B, B->F, L->R, R->L | - |

Minimum viable set: Idle + Loop x4 + Pivot (free). Everything else can be added without changing the C++ interface.

## Implementation plan

| # | Step | Effort | Risk |
| --- | --- | --- | --- |
| 1 | `Loco`-prefixed variables and update math in `UYogAnimInstance`, shared cardinal function with hysteresis | 0.5 day | Low |
| 2 | Wire the new variables into `ABP_YogCharacter_Base` and build the states in `LocomotionSM` (done by hand in the ABP editor, see "ABP work") | 0.5-1 day | Medium (shared base: `B_YogChar` and Market `ABP_BlueGuy` reference it) |
| 3 | Aim target: Auto + Focus modes, shared candidate filter, C++ API only (no input mapping) | 1 day | Low |
| 4 | Locomotion facing and suspend rules (tag set verified for melee and dash, Blueprint abilities not yet) | 0.5 day | Medium |
| 5 | Motion state machine from CMC velocity / acceleration (pivot included) | 0.5 day | Low |
| 6 | Clip authoring and authored-speed table | Animator-dependent | Low |
| 7 | Play-rate matching, optional Stride Warping (IK foot bones exist) | 0.5 day | Low |
| 8 | Automation tests: cardinal function, pivot trigger math. Run through MCP `AutomationTestToolset` after the user builds | 0.5 day | Low |

### Implementation status (2026-11-04, not compiled)

| Piece | File | Note |
| --- | --- | --- |
| Enums and pure math | `Animation/PlayerLocomotionMath.h/.cpp` | `ELocoCardinal`, `ELocoPivotType`, `YogLocomotion::` angle, cardinal with hysteresis, reversal test, pivot type choice |
| `Loco*` ABP variables | `Animation/YogAnimInstance.h/.cpp` | Written in `NativeUpdateAnimation` for player owners only. `LocoCardinalDeadZone` and `LocoMinMoveSpeed` are class defaults |
| Aim target, facing, Free / Locked frame | `Component/PlayerLocomotionComponent.h/.cpp` | Added to `APlayerCharacterBase` as `LocomotionComponent`. **`bEnableLocomotion` is false by default**, so nothing changes until it is turned on |
| Shared candidate filter | `UANS_AutoTarget::GatherHostileCandidates` | `FindBestTarget` now calls it. Behavior unchanged |
| Motion state | `YogLocomotion::NextMotionState`, `UYogAnimInstance` | Pure velocity / acceleration machine. Tuning: `LocoPivotAngleTolerance`, `LocoPivotMinSpeedRatio`, `LocoStartToLoopSpeedRatio` on the anim instance |
| `AN_PivotEnd` | `Animation/AN_PivotEnd.h/.cpp` | Deprecated no-op, see section 4 |
| Debug commands (non-shipping) | `PlayerLocomotionComponent.cpp` | `Yog.Loco.Enable [0\|1]`, `Yog.Loco.Lock`, `Yog.Loco.Cycle`, `Yog.Loco.Release`, `Yog.Loco.Auto` |
| Tests | `Tests/PlayerLocomotionMathTests.cpp` | `DevKit.Locomotion.*`: angle, bands, hysteresis, reversal, pivot type, motion state, view yaw |

Not covered by tests: the component itself (aim target selection, suspend rules), because it needs a spawned player pawn. Verify in PIE.

Order: 1 -> 2 -> 6 (Loop only) first, so the character strafes with a debug target. Then 3 -> 4 -> 5. Until input mapping exists, drive Focus mode from a console command or debug key so it can be tested.

## What MCP can and cannot do here

Tested against the live editor on 2026-11-03.

| Task | MCP | Notes |
| --- | --- | --- |
| Read Blueprint class defaults and component defaults | Yes | Needs the `Default__X_C` path, e.g. `...B_PlayerOne.Default__B_PlayerOne_C:CharMoveComp` |
| Read data asset values | Yes | `ObjectTools.get_properties`. `list_properties` returns the schema, not values |
| List an ABP's variables, functions, parent class, referencers | Yes | `BlueprintTools`, `AssetTools.get_referencers` |
| Read an ABP's **function graphs** and EventGraph | Yes | `read_graph_dsl`, e.g. `UpdateVelocityData` |
| Read an ABP's **AnimGraph, state machine, state graphs** | **No** | The graph paths exist but return an empty script. `find_nodes` returns nothing. There is no AnimGraph toolset |
| Edit the state machine, transitions, Blend Poses, sync groups | **No** | Do these by hand in the ABP editor |
| Add ABP variables and function graphs | Probably | `add_variable`, `add_function_graph`, `write_graph_dsl` exist on `BlueprintTools`. **Not tried**, and `write_graph_dsl` compiles the Blueprint, so it should not be run on the shared `ABP_YogCharacter_Base` without a plan |
| Add notifies to a sequence or montage | No | No tool edits notifies, so `AN_PivotEnd` is placed by hand |
| Compile C++ | No | The user builds. Newly reflected C++ members only appear in the editor after a UBT rebuild |
| Run automation tests | Yes | `AutomationTestToolset` (DiscoverTests, RunTests), after the user builds |
| Click editor UI | Yes | `SlateInspectorToolset` |

### ABP work (manual, in the ABP editor)

Everything in this list needs the editor and cannot be done through MCP. It is ordered so the C++ lands first, because the ABP can only reference variables that exist.

1. Build the C++ from step 1 and restart the editor. Confirm the `Loco...` variables appear in `ABP_YogCharacter_Base` (they are inherited from `UYogAnimInstance`).
2. In `ABP_YogCharacter_Base`, `LocomotionSM`: add Idle, Start, Loop, Stop and Pivot states beside the existing `Idle/Walk` state. Keep `Idle/Walk` until the new set works.
3. Transitions use only plain reads of the `bLoco...` variables (the table in "State machine"). Use Inertialization blends.
4. Put the Loop clips in one sync group so a cardinal change does not pop the feet.
5. Check that the Slot node for attack montages sits **after** the state machine in the AnimGraph (not verified, because the AnimGraph cannot be read).
6. Add the new layer functions to `ALI_WeaponLayer` and implement them in `ABP_CommonLayer` first. The other layer ABPs need them before they compile cleanly.
7. Verify in PIE with `ShowDebug Animation`.

## Open questions

| # | Question | Why it matters |
| --- | --- | --- |
| 1 | Which mode is the default, and is the mode a player setting or fixed by weapon / level? | Where the config lives |
| 2 | In Focus mode, when the target dies: drop to no-target, or fall back to Auto? | Feel; avoids a dead stretch with no aim |
| 3 | Reverse pause length, and may a dash / attack cancel it (assumed yes)? | Tuning, input priority |
| 4 | Should backpedal be slower than forward (a CMC speed cap, not an ABP change)? | Gameplay speed rule |
| 5 | Multiplayer? | Facing tick and pivot would need replication thought |

Deferred, not open: Focus-mode inputs and gamepad mapping, all dash behavior.

## Not verified

| Item | Why it matters |
| --- | --- |
Resolved on 2026-11-03 and moved into "Current state": the CMC defaults on `B_PlayerOne`, the player's ABP chain, the StateConflict pairing for movement, the melee and dash ability tags, and the player skeleton's root bone.

Still unverified:

| Item | Why it matters |
| --- | --- |
| Runtime CMC values on the player | `InitializeMovement` overwrites `MaxWalkSpeed`, braking, `MaxAcceleration` and `RotationRate` from the character-data movement row. Only the Blueprint defaults were read. The ~0.15 s stop estimate depends on them |
| The AnimGraph and `LocomotionSM` layout | MCP returns nothing for them. Known only that `LocomotionSM` has an `Idle/Walk` state (from the graph list) and that the layer function `WeaponLayer_BS_Idle/Walk` exists. Needs a manual look |
| Where the Slot node sits relative to the state machine | Decides whether attack montages override locomotion |
| Whether `LocalVelocityDirectionAngle` is ever written | None of the read function graphs set it. It may be written in the AnimGraph, or be dead |
| Tags on `BGA_PlayerSpecialAttack` and `BGA_Player_FinisherAttack` | They are Blueprints. The suspend rule assumes they grant a tag under `Character.State.Skill` |
| Whether enemies (`B_YogChar`) and Market `ABP_BlueGuy` still use `ABP_YogCharacter_Base` | Editing the shared base changes them. `B_YogChar` has no referencers, so it may be unused |
| Whether to reuse `AnimEnum_Cardinal` as the C++ cardinal type | It is a Blueprint `UserDefinedEnum` with Forward / Backward / Left / Right, which C++ cannot reference. The C++ `LocoMoveCardinal` needs its own `UENUM`, and the ABP would convert or compare against it. Decide whether to keep both or retire the Blueprint enum |
| How the weapon-ability dash will be wired | Only matters when that work starts |
| Existing player locomotion clips | `Content/Code/Characters/PlayerOne/Animation` only holds a light-attack prototype. `Content/Art` is gitignored, so clips may live outside the repo |
