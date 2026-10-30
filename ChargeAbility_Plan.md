# Enemy Charge Ability — Implementation Plan

## Goal
Enemy faces the player, then charges in a straight line toward the player's position.
- **Hits player** → player is stunned.
- **Misses** → enemy keeps going until it hits a wall → enemy is stunned (pinned to the wall).
- **Neither** (open space) → timeout → recovery.

---

## 1. Animation: One Montage, Multiple Sections

Use 3–5 clips in **one montage**, split into sections:

| Section | Content | Notes |
|---|---|---|
| `Windup` | Crouch/roar before charging | Telegraph for the player. Enemy rotates to face the player here. |
| `ChargeLoop` | Run cycle | In Montage Sections panel, set next section = **itself** so it loops until code jumps out. |
| `Recover` | Skid/stop after hitting player or timing out | Ends the ability. |
| `WallImpact` *(optional)* | Slam + stagger | Can be skipped if the generic stun reaction handles it. |

- Charge length is variable, so the looping middle section is required.
- Switch sections in code with `MontageJumpToSection`.
- Add an AnimNotify at the end of `Windup` that sends gameplay event `Event.Charge.Start` (or use montage section timing).

---

## 2. Movement: Root Motion Source (NOT animation root motion)

Do **not** use baked animation root motion (fixed distance, hard to interrupt, can't tune speed).

Use **`UAbilityTask_ApplyRootMotionConstantForce`**:
- **Direction:** locked once at the end of Windup → `(PlayerLoc - SelfLoc).GetSafeNormal2D()`. Locked direction = dodgeable.
- **Strength:** charge speed (expose as a tunable property).
- **Duration:** max charge time (safety cap).
- `bIsAdditive = false` so AI movement input doesn't fight it.
- Goes through CharacterMovement → collision sweeps, floor handling, and network replication come for free.

Also during the charge:
- Stop AI movement (`AIController->StopMovement()`).
- Clear AI focus so the controller doesn't steer/rotate the pawn.

---

## 3. Ability Lifecycle

```
ActivateAbility
 ├─ CommitAbility (cost/cooldown)
 ├─ Stop AI movement, add tag State.Charging
 ├─ PlayMontageAndWait (section "Windup")
 │    └─ during windup: interp-rotate to face player
 ├─ Windup end (AnimNotify / WaitGameplayEvent "Event.Charge.Start")
 │    ├─ Lock direction = (Player - Self).GetSafeNormal2D()
 │    ├─ Jump to "ChargeLoop"
 │    ├─ Start ApplyRootMotionConstantForce
 │    └─ Arm hit detection (see section 4)
 │
 ├─ OUTCOME A: hit player
 │    ├─ Apply GE_Stun to player (+ optional knockback)
 │    ├─ End root motion task, jump to "Recover"
 │    └─ On montage end → EndAbility
 │
 ├─ OUTCOME B: hit wall
 │    ├─ End root motion task
 │    ├─ Apply GE_Stun to SELF
 │    └─ EndAbility (stun system takes over)
 │
 └─ OUTCOME C: timeout / max distance (no hit)
      └─ Jump to "Recover" → EndAbility

EndAbility (always, including cancel):
  - end root motion task
  - remove State.Charging
  - disarm hit detection / restore collision
  - stop montage if still playing
```

### Stun is a GameplayEffect, NOT part of the charge ability
- `GE_Stun`: Duration X seconds, grants tag `State.Stunned`.
- Generic stun reaction (passive ability triggered by `State.Stunned`, or AnimBP state) plays the stun animation.
- Add `State.Stunned` to `ActivationBlockedTags` on all abilities (and cancel active abilities on apply).
- Same stun logic is reused by player and enemy. The charge ability stays simple: it ends on impact.
- "Pinned to the wall" = enemy's stun anim playing while it's pressed against the wall.

---

## 4. Hit Detection

### Player hit
- Add a capsule/sphere slightly larger than the body (or at the front).
- Overlap only, enabled only during `ChargeLoop`.
- On overlap with player → Outcome A.
- Use overlap (not block) so pawn-vs-pawn blocking doesn't stop the charge before detection.

### Wall hit
- Bind the capsule's `OnComponentHit` (or check CharacterMovement hit results).
- Count as wall only if:
  - `Dot(HitNormal, ChargeDir) < -0.7` (head-on, not grazing)
  - `abs(HitNormal.Z) < 0.3` (roughly vertical surface, ignores floors/slopes)
- Only arm checks once `ChargeLoop` begins (windup can't trigger them).

---

## 5. AI Side

- Behavior Tree task: `TryActivateAbilityByTag(Ability.Enemy.Charge)`, wait for ability-ended delegate before finishing the task.
- Decorator: line-of-sight + distance range check (don't charge at point-blank or too far).

---

## 6. Gameplay Tags to Create
- `Ability.Enemy.Charge`
- `State.Charging`
- `State.Stunned`
- `Event.Charge.Start`

## 7. Assets / Classes to Create
- [ ] Montage `AM_Enemy_Charge` with sections `Windup`, `ChargeLoop` (looping), `Recover`, (`WallImpact`)
- [ ] AnimNotify at end of Windup → sends `Event.Charge.Start`
- [ ] `GA_Enemy_Charge` (GameplayAbility)
- [ ] `GE_Stun` (duration GE granting `State.Stunned`)
- [ ] Stun reaction (passive GA triggered by tag, or AnimBP state)
- [ ] Charge hit collider component on enemy character (disabled by default)
- [ ] BT Task: activate ability by tag + wait for end
- [ ] BT Decorator: LOS + distance check

## 8. Tunable Properties (on GA_Enemy_Charge)
- `ChargeSpeed`
- `MaxChargeDuration`
- `WindupTurnRate`
- `WallHitDotThreshold` (default -0.7)
- `StunEffectClass` (player), `SelfStunEffectClass` (wall)
- `KnockbackStrength` (optional)
