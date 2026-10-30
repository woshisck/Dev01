# Enemy Traits + Swarm Frenzy — Implementation Plan

| Item | Content |
| --- | --- |
| Scope | Reusable per-enemy "trait" assets configured on `UEnemyData`; first trait = Swarm Frenzy (rat) |
| Audience | Programmer (C++ steps 1-6), Designer (StateTree + asset steps 7-8) |
| Related | `EnemyData.h`, `YogAIController.h/.cpp`, `YogGameMode.h/.cpp`, `EnemyCharacterBase.cpp`, `ST_Rat` |
| Last updated | 2026-10-29 |
| Status | **C++ written (steps 1-6), not compiled, not PIE-verified.** StateTree (step 7) and content (step 8) are still to do in the editor |

## Requirement

1. Features are configured on an enemy's EnemyData (`DA_Rat` first) and are reusable on any enemy.
   One enemy can carry **several traits at once**.
2. Swarm Frenzy: every time a rat spawns or dies, count the rats alive in the level.
   - Count `>=` tier threshold → the rat gains **Frenzy**: whatever StateTree state it is in, it jumps to the
     **`Attack`** child state (`ST_Rat`: `Root > Combat > Attack`).
   - Frenzy has **tiers** (e.g. 5 rats = tier 1, 10 rats = tier 2 with a stronger buff).
   - Count drops below the lowest threshold → Frenzy is **removed**.
3. Each trait is **one asset** that can be dropped onto any enemy's EnemyData.

## Current state

| Piece | Status |
| --- | --- |
| `AYogGameMode::RegisterEnemy / UnregisterEnemy` | Every spawn path registers (`EnemyCharacterBase::BeginPlay`, `MobSpawner`, `YogLuaSpawnLibrary`, GM cheat); `Die()` unregisters. **No event is broadcast** (only `OnBossRegisteredNative`). |
| `GetAllAliveEnemies()` | Exists — usable for counting. |
| `DA_Rat.StateTree` | `/Game/Code/Enemy/AI/StateTree/ST_Rat` (not `ST_Enemy_DefaultMelee`) |
| `ST_Rat` layout | `Root > Dead / Combat (Chase, Attack) / Alert / Patrol`. Top-level states gated by `Enemy Is Dead` / `Enemy AI State Is`. `Attack` runs `Activate Ability By Tag` (`Enemy.Melee.LAtk1/LAtk2`), which has **no range gate**. |
| Blackboard `EnemyAIState` | `UpdateAwarenessBlackboard` rewrites it **every tick** from vision, so a frenzied rat must be pinned to Combat there, or it drops back to Alert and leaves the Combat subtree. |
| `UEnemyData::EnemyBuffPool` | Declared, never read at runtime — no existing per-enemy feature hook. |
| StateTree events | Engine `UStateTreeComponent::SendStateTreeEvent` is available on `AYogAIController::StateTreeComponent`; the project does not use it yet. |

## Design

### Why a DataAsset trait (not Lua / BuffFlow)

| Option | Verdict |
| --- | --- |
| Lua (`Content/Script/Maps`) | Map-scoped; must be re-wired on every map. No. |
| BuffFlow rune via `EnemyBuffPool` | Pool is dead code, and Flow has no "enemy registered / died" event source. |
| **`UEnemyTrait` DataAsset (C++ base, Blueprintable)** | One asset per trait, listed on any EnemyData. Chosen. |

### Architecture

```
UEnemyData
  └─ TArray<TObjectPtr<UEnemyTrait>> Traits          DA_Rat: [DA_Trait_SwarmFrenzy, ...more traits]

AEnemyCharacterBase
  BeginPlay  → for each Trait: ActivateTrait(this)      (after GM->RegisterEnemy)
  Die/EndPlay→ for each Trait: DeactivateTrait(this)    (before GM->UnregisterEnemy)
  owns per-enemy runtime state: TMap<TObjectKey<UEnemyTrait>, FEnemyTraitRuntime>

AYogGameMode
  RegisterEnemy   → OnEnemyRegisteredNative.Broadcast(Enemy)     (new)
  UnregisterEnemy → OnEnemyUnregisteredNative.Broadcast(Enemy)   (new, only if an entry was removed)

UEnemyTrait_SwarmFrenzy::Recount(Owner)      ← on activate (next tick) + every register/unregister
  N    = alive enemies matching CountFilter
  Tier = highest tier with MinCount <= N   (or none)
  tier unchanged → nothing
  none → tier    : add tag Enemy.State.Frenzy, apply tier GE / cue
                   if tier.bForceAttackOnEnter:
                       YogAI->EnterCombat(Player, false)            ← makes Combat's enter condition pass
                       YogAI->SendStateTreeEvent(Enemy.Event.Frenzy) ← ST jumps to Combat > Attack
  tier → tier'   : swap GE / cue; force-attack again only if tier'.bForceAttackOnEnter
  tier → none    : remove tag, GE, cue

AYogAIController::UpdateAwarenessBlackboard (every tick)
  owner has Enemy.State.Frenzy && player alive → EnterCombat(Player, false); return
       (pins EnemyAIState = Combat: no vision / cone / exit radius / lose-target fallback)
```

Key rule: **trait assets are shared across every rat and must hold no per-enemy state.** Per-enemy state
(current tier, active GE handle, delegate handles) lives in `FEnemyTraitRuntime` on the enemy.

### Stacking

| Kind | How |
| --- | --- |
| Several traits on one enemy | `Traits` is an array; each trait has its own runtime slot keyed by the asset |
| Stronger Frenzy tiers | `UEnemyTrait_SwarmFrenzy::Tiers` array, highest matching tier wins, tiers swap (not add) |

## Implementation steps (C++)

### 1. Trait base — `Public/Enemy/EnemyTrait.h` (new)

- [ ] `UCLASS(Abstract, Blueprintable, BlueprintType) class UEnemyTrait : public UPrimaryDataAsset`
- [ ] `virtual void ActivateTrait(AEnemyCharacterBase* Owner) const` / `virtual void DeactivateTrait(AEnemyCharacterBase* Owner) const`
- [ ] `BlueprintImplementableEvent` `K2_OnActivated` / `K2_OnDeactivated` so simple traits can be made in BP.
- [ ] `USTRUCT FEnemyTraitRuntime` — `int32 ActiveTier = INDEX_NONE`, `FActiveGameplayEffectHandle`,
      two `FDelegateHandle`s for the GM registry events.

### 2. EnemyData field — `EnemyData.h`

- [ ] `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Traits") TArray<TObjectPtr<UEnemyTrait>> Traits;`
- [ ] Add it to the Enemy Manager property list (`SEnemyManagerWidget.cpp` ~line 1391, next to `EnemyBuffPool`).

### 3. Registry events — `YogGameMode.h/.cpp`

- [ ] `DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyRegistryChanged, AEnemyCharacterBase*)`
- [ ] `OnEnemyRegisteredNative`: broadcast at the end of `RegisterEnemy` (after the dedupe early-return).
- [ ] `OnEnemyUnregisteredNative`: broadcast in `UnregisterEnemy` only if `RemoveAllSwap` removed the given
      enemy (`MobSpawner` and `Die()` can both unregister the same enemy).

### 4. Hook traits on the enemy — `EnemyCharacterBase.h/.cpp`

- [ ] `BeginPlay`: after `GM->RegisterEnemy(this)`, call `ActivateTrait` for each entry in `EnemyData->Traits`.
- [ ] `Die()`: call `DeactivateTrait` **before** `GM->UnregisterEnemy(this)` so the dying rat neither reacts to
      its own death nor keeps its frenzy GE. Repeat in `EndPlay` (level unload / destroy without death);
      deactivation must be idempotent.
- [ ] `UPROPERTY()`-free `TMap<TObjectKey<UEnemyTrait>, FEnemyTraitRuntime> TraitRuntime;` + accessor for traits.

### 5. Swarm Frenzy trait — `UEnemyTrait_SwarmFrenzy`

| Field | Default | Meaning |
| --- | --- | --- |
| `Tiers` | `[{MinCount=5}]` | `TArray<FEnemyFrenzyTier>`, sorted by `MinCount` ascending on `PostEditChangeProperty` |
| `CountFilter` | `SameEnemyId` | `SameEnemyId` (falls back to same EnemyData asset when `EnemyId` is None) / `FactionTag` / `AnyEnemy` |
| `bCountSelf` | true | Owner included in the count |

`FEnemyFrenzyTier`:

| Field | Meaning |
| --- | --- |
| `MinCount` | Tier active when count `>=` this |
| `Effect` | `TSubclassOf<UGameplayEffect>`, Infinite, applied while the tier is active (speed / damage) |
| `CueTag` | Optional GameplayCue for visual feedback (red eyes, aura) |
| `bForceAttackOnEnter` | Send the Frenzy StateTree event when this tier is entered |

- [ ] `ActivateTrait`: bind both GM delegates (store handles in runtime), then
      `GetWorldTimerManager().SetTimerForNextTick` → `Recount`. The deferral matters: on `BeginPlay` the
      StateTree may not have started yet, and events sent before `StartLogic` are dropped.
- [ ] `Recount`: iterate `GM->GetAllAliveEnemies()`, skip `!IsAlive()`, apply filter, pick tier, apply the
      edge logic from the diagram. O(N) per event per rat — fine at room scale.
- [ ] `DeactivateTrait`: unbind delegates, remove GE / tag / cue, erase runtime slot.

### 6. AI side — `YogAIController.h/.cpp`

- [ ] `void SendStateTreeEvent(FGameplayTag EventTag)` — guards `StateTreeComponent`, forwards to the engine.
- [ ] Top of `UpdateAwarenessBlackboard` (after the pawn / BB guard): if the pawn ASC has `Enemy.State.Frenzy`
      and the player pawn exists → `EnterCombat(PlayerPawn, false)` and `return true`.
      `static const FGameplayTag` for the tag.
- [ ] Conversation hold still wins: the brain is paused, so neither the evaluator nor the event fires. Events
      sent during hold are lost, but the awareness pin keeps the rat in Combat when the hold ends — acceptable.
- [ ] Add tags to `Config/Tags/EnemyTag.ini`:
      `Enemy.State.Frenzy` (loose tag while frenzied), `Enemy.Event.Frenzy` (StateTree event).

## Lua-scripted traits (optional layer on top of the C++ trait)

Designers who prefer scripting can use `UEnemyTrait_Lua` instead of `UEnemyTrait_SwarmFrenzy`. It is the same
kind of asset, listed on `UEnemyData.Traits` the same way, but the logic is a Lua module. The C++ Swarm
Frenzy trait stays as the reference implementation; use one or the other on an enemy, not both.

| Piece | File |
| --- | --- |
| Lua trait asset | `Public/Data/EnemyTrait_Lua.h`, `Private/Data/EnemyTrait_Lua.cpp` |
| Engine calls Lua can reach | `Public/Data/EnemyTraitLuaLibrary.h` (`UEnemyTraitLuaLibrary`) |
| Swarm Frenzy script | `Content/Script/Traits/SwarmFrenzy.lua` |

Module contract (all functions optional; arguments are `Owner`, `Trait`):

| Function | When |
| --- | --- |
| `OnActivate` | Enemy begins play |
| `OnRosterChanged` | One tick after any enemy registers / unregisters (coalesced), and once one tick after activation |
| `OnDeactivate` | Enemy dies or leaves the world |

Rules for script authors:

- The module table is shared by every enemy using it. Keep per-enemy state in a weak-keyed table keyed by `Owner`.
- Read tuning from `Trait.NumberParams` and `Trait.EffectParams`, authored on the asset; the names are up to the script.
- Calls are `pcall`-guarded; a Lua error is logged and skipped.
- The enemy must be able to resolve a Lua env (UnLua env on the owner's world); a missing env logs a warning and skips.

Stacking parameters read by `SwarmFrenzy.lua` (names are the script's own; any other script can define its own):

| Param | Where | Default | Meaning |
| --- | --- | --- | --- |
| `MinCount` | NumberParams | 5 | Matching enemies needed for the first stack |
| `CountPerStack` | NumberParams | 0 | Extra enemies per additional stack; 0 = never more than 1 stack |
| `MaxStacks` | NumberParams | 0 | Stack cap; 0 = uncapped |
| `CountSelf` | NumberParams | 1 | 0 leaves the owner out of the count |
| `ReattackPerStack` | NumberParams | 0 | 1 = every gained stack re-sends `Enemy.Event.Frenzy`, not only the first |
| `StackEffect` | EffectParams | none | Infinite, Stack Limit 1 GE, applied once at **Level = stack count** |

`Stacks = 1 + floor((Count - MinCount) / CountPerStack)` once `Count >= MinCount`, then capped. With
`MinCount 5, CountPerStack 5`: 5 rats = 1 stack, 10 = 2, 15 = 3. Because the effect's level is the stack
count, scale its magnitudes with a curve table or scalable float; a constant magnitude will not grow with stacks.
The stack effect is removed and reapplied whenever the stack count changes (one GE per rat, never two).

Content for the Lua version of frenzy: create a `UEnemyTrait_Lua` asset with `LuaModule = Traits.SwarmFrenzy`,
`NumberParams { MinCount = 5, CountPerStack = 5, MaxStacks = 4 }`, `EffectParams { StackEffect = GE_RatFrenzy }`,
and add it to `DA_Rat.Traits`. The StateTree setup below is identical.

## StateTree setup (`ST_Rat`, done in editor by the designer)

Goal: from `Patrol`, `Alert` or `Chase`, receiving `Enemy.Event.Frenzy` jumps straight into `Combat > Attack`.
`Dead` and `Attack` itself are deliberately left out (a dead rat must stay dead; a rat already swinging
should not restart its swing).

### 7. Steps

1. Build C++ first, so `Enemy.Event.Frenzy` exists in the tag picker.
2. Open `/Game/Code/Enemy/AI/StateTree/ST_Rat`.
3. Select the **`Patrol`** state → Details → **Transitions** → **+** and set:

   | Field | Value |
   | --- | --- |
   | Trigger | `On Event` |
   | Required Event → Tag | `Enemy.Event.Frenzy` |
   | Transition To | `Attack` (pick `Combat > Attack`, not `Combat`) |
   | Priority | `High` |
   | Consume Event on Select | ✔ |
   | Delay | off |

4. Add the **same transition** to **`Alert`** and to **`Chase`** (Combat's other child).
5. Do **not** add it to `Root` (it would also fire from `Dead` and `Attack`) or to `Dead` / `Attack`.
6. Check `Attack`'s **Enter Conditions**. If it has a range condition (e.g. `Target Within 2D Distance`), a
   frenzied rat far from the player will fail the jump and fall back to `Chase` — that is fine (it still
   charges, then attacks in range). Remove the condition only if you want a literal out-of-range lunge.
7. **Compile** the StateTree (toolbar) and **Save**. An uncompiled edit never reaches runtime.
8. Any state added to `ST_Rat` later that should be interruptible by Frenzy needs this transition too.
   Other enemies using Frenzy need the same edit on their own tree.

Why the jump succeeds: transitioning into `Attack` re-tests `Combat`'s enter condition
(`Enemy AI State Is = Combat`). The trait calls `EnterCombat` right before sending the event, so it passes.
After the swing ends, the awareness pin keeps `EnemyAIState = Combat`, so the rat stays in `Chase ↔ Attack`
until Frenzy is removed.

### 8. Content

- [ ] Create `DA_Trait_SwarmFrenzy` (suggested folder `Content/Docs/Data/Enemy/Traits/`):
      Tier 0 `MinCount = 5, bForceAttackOnEnter = true`; Tier 1 `MinCount = 10, Effect = GE_RatFrenzy_T2`.
- [ ] Create the tier GEs (Infinite, Stack Limit 1).
- [ ] `DA_Rat.Traits = [DA_Trait_SwarmFrenzy]`. Optionally `DA_Rat_UnArm`.

## Verification

| Test | Expected |
| --- | --- |
| 4 rats placed, player hidden far away | All `Patrol` |
| Spawn 5th rat | All 5 jump to `Attack` (log `EnemyAIState` cvar = 1 shows `-> Combat`), then chase the player without having seen them |
| Rat mid-swing when the 5th spawns | Swing finishes normally (no restart) |
| Reach 10 rats | Tier 2 GE replaces tier 1 GE on every rat (one GE per rat, not two) |
| Kill down to 4 | Frenzy tag / GE / cue removed; normal awareness resumes |
| 4 rats + 3 other enemies, `SameEnemyId` | No Frenzy |
| Conversation mode while frenzied | Rats hold; resume Combat on exit |
| Level transition with frenzied rats | No crash, no dangling GM delegates (EndPlay deactivates) |
