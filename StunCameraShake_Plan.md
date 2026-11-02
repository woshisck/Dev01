# GE_Stun Camera Shake — Implementation Plan

| Item | Value |
|------|-------|
| Scope | `UGE_Stun` presentation: camera shake when a stun starts |
| Audience | Programmer + designer |
| Related | `ChargeAbility_Plan.md`, `Content/Script/Skills/Charge.lua` |
| Last updated | 2026-11-01 |

---

## 1. Current State

`GE_Stun` has **no GameplayCue** attached.

| Where | Finding |
|-------|---------|
| `Source/DevKit/Private/AbilitySystem/GameplayEffect/GE_Stun.cpp` | Constructor sets duration (1.5 s), grants `Buff.Status.Stunned`, stacking rules. `GameplayCues` is never set. |
| Blueprint children | None. Only `Skill_Charge.uasset` references `GE_Stun`. |
| `Content/Script/Skills/Charge.lua:53` | Applies the **native** class: `Asset.EffectParams:Find("TargetEffect") or UE.UGE_Stun`. Used on the player (charge hit) and on the enemy itself (wall hit, via `SelfEffect`). |
| `Config/Tags/GameplayCueTag.ini` | No `GameplayCue.*Stun*` tag. |
| `Content/Code/GAS/GameplayCueNotifies/` | No stun GCN. |

### Why not reuse `GCN_CameraShake`

| Asset | Tag | Event implemented |
|-------|-----|-------------------|
| `GCN_CameraShake` (parent `AGCN_VisualBase`) | `GameplayCue.Character.CameraShake` | **OnExecute** only (plays `B_GreatSwordShake`) |
| `GE_CameraShake` | same tag | Instant-style cue trigger |

`GE_Stun` is a **HasDuration** effect, so its cues fire `OnActive` / `WhileActive` / `OnRemove`. `OnExecute` never runs, so `GCN_CameraShake` would do nothing.

---

## 2. Event Flow

```
Charge.lua ApplyGameplayEffect(UGE_Stun)
        │
        ▼
UGE_Stun (HasDuration 1.5 s)
  ├─ TargetTags: Buff.Status.Stunned  ──► movement lock / input block
  └─ GameplayCues: GameplayCue.Character.Stun
        │
        ▼
GCN_Stun (AGCN_VisualBase BP)
  ├─ OnActive   ──► target is player? ──► play camera shake
  └─ OnRemove   ──► (auto destroy)
```

---

## 3. Steps

### Step 1 — Add the cue tag

File: `Config/Tags/GameplayCueTag.ini` (keep alphabetical, after `GameplayCue.Character.Poison`):

```ini
GameplayTagList=(Tag="GameplayCue.Character.Stun",DevComment="Stun presentation: camera shake on stun start. Driven by UGE_Stun.")
```

### Step 2 — Create `GCN_Stun` (editor)

Path: `Content/Code/GAS/GameplayCueNotifies/GCN_Stun`

| Setting | Value |
|---------|-------|
| Parent class | `AGCN_VisualBase` (same as `GCN_CameraShake`) |
| Gameplay Cue Tag | `GameplayCue.Character.Stun` |
| Auto Destroy On Remove | `true` |
| Event to override | **OnActive** (not OnExecute) |

`OnActive` graph:

1. Check `MyTarget` is the player (cast to `PlayerCharacterBase`, or `IsPlayerControlled`). The enemy also stuns itself on a wall hit; skip this check only if that should shake too.
2. Call `PlayWorldCameraShake` (epicenter = target location) or `ClientStartCameraShake` on the player controller.
3. Shake class: pick or create one under `Content/Code/GAS/CameraShake/` (e.g. a child of `B_CameraShakeBase`).

### Step 3 — Attach the cue to `UGE_Stun` (C++)

`Charge.lua` uses the native class, so a Blueprint child of `GE_Stun` would not be picked up. Add to the end of `UGE_Stun::UGE_Stun()`:

```cpp
if (const FGameplayTag StunCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Character.Stun"), false);
	StunCueTag.IsValid())
{
	GameplayCues.Add(FGameplayEffectCue(StunCueTag, 0.f, 1.f));
}
```

Requires a C++ rebuild (constructor change on a reflected class).

---

## 4. Verification

| Check | Expected |
|-------|----------|
| An enemy charge (`Skill_Charge`) hits the player | Camera shakes once at stun start |
| Charge hits a wall (enemy self-stun) | No shake (if the player check is in place) |
| Stun re-applied during an active stun | Duration refreshes; no extra GCN actor left behind |
| `showdebug abilitysystem` during stun | `GameplayCue.Character.Stun` listed while active |
