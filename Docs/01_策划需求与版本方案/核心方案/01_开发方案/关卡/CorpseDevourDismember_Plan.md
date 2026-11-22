# Corpse Devour / Dismember — Analysis & Plan

> 适用范围: Enemy death body, corpse interaction (Devour / Dismember), room-counted buffs, combat items
> 适用人群: Programmer, Designer
> 配套文档: [FloorGen_Guide](../../../../03_策划配置与制作手册/核心配置说明/关卡/FloorGen_Guide.md)
> 最后更新: 2026-11-22

## 1. Confirmed Design

| # | Rule |
| --- | --- |
| R1 | **Every** mob leaves its body on the ground. It stays until it is Devoured or Dismembered, or the room is left. |
| R2 | **Devour** = interact button on the body (prompt "Devour"). No popup. |
| R3 | **Dismember** = hit the body N times with melee. N defaults to **3** and is a designer value (`EnemyData.CorpseHitsToDismember`). |
| R4 | Dismember is the **only** source of gold / material / kill-reward loot. It also gives a usable **combat item**. Nothing drops on death. |
| R5 | Devour cap per room: **2** (integer, adjustable). Once reached, the remaining bodies can only be Dismembered. |
| R6 | Active Devour stacks: max **2** (integer, adjustable later by buffs). At the cap the Devour prompt is disabled. |
| R7 | A Devour gives a temporary buff. The room it was taken in does not count. Every room entered afterwards counts **1**, whether or not it has mobs. The buff covers the next **2** rooms. On leaving the second room the buff is removed and a **permanent passive** is granted for the rest of the run. |

### 1.1 Room counting example

```
Room A (combat)   devour x2       stacks: [A1:0, A2:0]   temp buff active
  door opens -> enter Room B      stacks: [A1:1, A2:1]   temp buff active
  enter Room C                    stacks: [A1:2, A2:2]   temp buff active
  enter Room D                    A1, A2 mature -> temp GE removed, 2 permanent passives granted
```

R5 and R6 are separate numbers. With both at 2 they behave as one cap. They differ only after a buff raises one of them.

## 2. Death Pose: ABP State vs Montage + Freeze

**Recommendation: keep montage + freeze (the current `GA_Dead` path), with two fixes.**

| | Montage + freeze (current) | ABP `Dead` state |
| --- | --- | --- |
| Per-enemy work | Only a death montage in `AbilityData.PassiveMap["Action.Dead"]` (already the pipeline) | Add a state + transition + `bIsDead` var to **every** enemy ABP. MCP cannot edit AnimGraphs, so all of this is by hand. |
| Already working | Yes: `Mesh->bPauseAnims = true` on corpse | No |
| Cost of ~20 bodies per room | Near zero: paused mesh skips anim evaluation | Each body keeps evaluating its ABP every frame |
| Interaction with GAS montages | Death montage owns the slot, so nothing can override it | Any later montage on the slot (e.g. a stray hit react) would pop the body up |
| Hit reaction on body (Dismember feedback) | Not animated: use FX / material flash / small scale punch | Possible (additive) |

Switch to an ABP state only if bodies later need animated reactions or ragdoll.

**Fixes needed for the montage path:**

| # | Problem | Fix |
| --- | --- | --- |
| F1 | `OnDeathMontageBlendOut` freezes the mesh when blend-out **starts**, which is `BlendOutTime` (default 0.25 s) **before** the montage's final frame. The body freezes slightly before its final pose. | Set **Blend Out Time = 0** on every death montage (content), so blend-out fires on the final frame. Add an editor validation warning for death montages with blend-out > 0. |
| F2 | A mob with **no** death montage hits `StartDeathDelay()` immediately and freezes standing in idle. | Log a warning in `GA_Dead` when a mob without a death montage becomes a corpse, and give every mob a death montage. |

## 3. Current State vs Target

| Area | Today | Target |
| --- | --- | --- |
| Body stays | Only if the mob **rolled a kill reward** (`RollKillRewards` → `bLootableCorpse`); only `DA_Rat` has rewards | Always (`EnemyData.bLootableCorpse` stays as an opt-out for bosses / dummies, default true) |
| Rewards on death | Rolled on death; spawned on death for non-corpse mobs | Nothing on death. Rolled on Dismember. |
| Interact (E) on body | Spawns reward pickups → grants → destroys body | **Devour** |
| N hits on body | `ExplodeCorpse` → scatter pickups | **Dismember** → scatter gold / material / item pickups |
| Hit count value | `EnemyData.CorpseHitsToExplode = 3` | Same property, display name "Corpse Hits To Dismember" (keep the C++ name so `DA_Rat`'s saved value survives) |
| Combat items at runtime | `UCombatItemComponent` only has `DefaultItemAssets`, no add API, no persistence | `AddItem`, charges, saved in `FRunState` + save game, droppable via loot |
| Room counter | Nothing. `CompletedCombatBattleCount` only changes via cheat | Not needed: Devour counts **room entries**, which is already a run-state restore |
| Temp buff across levels | GEs are not carried by `FRunState` | `FRunState.ActiveDevours` re-applies the GE on restore |
| Permanent passive | `AddHiddenPassiveRune` + `FRunState.HiddenPassiveRuneInstances` exist | Reused as is |

## 4. Target Architecture

```
UEnemyData
  ├─ bLootableCorpse          (default true, opt-out)
  ├─ CorpseHitsToExplode      (DisplayName "Corpse Hits To Dismember", default 3)
  ├─ KillRewards              (now = Dismember loot: gold / material / skill / combat item)
  └─ DevourOffer : UDevourOfferDataAsset   (null -> UYogSettings::DefaultDevourOffer)
        ├─ LuaModule     "Devour.Default"  (Content/Script/Devour/Default.lua)
        ├─ NumberParams / EffectParams     (tuning read by the script)
        ├─ DurationRooms = 2
        └─ DisplayName / Icon / Description

Die() -> body always stays (no reward roll)
  ├─ E interact   -> UDevourComponent::TryDevour(Offer) ─ cap checks R5/R6 ─> Lua OnTempBegin
  │                   -> body Destroy (+ devour FX)
  └─ N melee hits -> RollKillRewards() -> SpawnKillRewardPickups(scatter) -> body Destroy

UDevourComponent (player)  — rules only, no buff content
  Devours: TArray<FActiveDevour{ Offer, Id, RoomsEntered, bMatured }>
  DevoursThisRoom (resets with the level, the player is respawned per room)

TransitionToLevel     -> CaptureRunState -> FRunState.Devours (+ FRunCheckpointData.Devours in the save)
RestoreRunState       -> RestoreFromRunState on the new player:
                           matured                         -> Lua OnPermanentBegin
                           ++RoomsEntered >  DurationRooms -> mature, Lua OnPermanentBegin
                           otherwise                       -> Lua OnTempBegin (re-apply on new player)
OnRoomEntered (same player, cheat / streamed floors) -> ++, Lua OnRoomEntered or OnTempEnd + OnPermanentBegin
ClearRunCarriedStateForHub -> ResetForNewRun -> Lua OnRemoved

Lua side: UPlayerBuffLuaLibrary (ApplyPlayerEffect / RemovePlayerEffect / loose tags / attributes /
HealPlayer / GetPlayerASC for binding OnKilledTarget, DealtDamage, OnDashExecuted).
Caps R5 / R6 are player attributes (`MaxDevoursPerRoom`, `MaxDevourStacks`, base 2), so a future buff GE can raise them without code.

## 5. Implementation Plan

### Phase 1 — Body always stays, rewards move to Dismember (R1, R3, R4)

| Step | Change | File(s) |
| --- | --- | --- |
| 1.1 | `Die()`: stop calling `RollKillRewards()`. `bLootableCorpse = EnemyData->bLootableCorpse` (no loot requirement). | `EnemyCharacterBase.cpp` |
| 1.2 | `FinishDying()` non-corpse path: remove `SpawnKillRewardPickups` (R4). | `EnemyCharacterBase.cpp` |
| 1.3 | `ExplodeCorpse()` → `DismemberCorpse()`: roll `RollKillRewards()` there, scatter pickups, destroy. Destroy even when the roll is empty (today it returns early and the body becomes unkillable). | `EnemyCharacterBase.cpp/.h` |
| 1.4 | `CorpseHitsToExplode`: change DisplayName to "Corpse Hits To Dismember" and its comment; keep the property name. | `EnemyData.h` |
| 1.5 | F1 / F2 from §2. | Death montage assets, `GA_Dead.cpp` |
| 1.6 | PIE: kill several mob types, bodies stay, 3 hits dismember and drop gold, portal still opens. | — |

### Phase 2 — Devour (R2, R5, R6, R7)

| Step | Change | File(s) |
| --- | --- | --- |
| 2.1 | New `UDevourOfferDataAsset` + `UYogSettings::DefaultDevourOffer` + `UEnemyData::DevourOffer` override. | `Data/DevourOfferDataAsset.h` (new), `YogSettings.h`, `EnemyData.h` |
| 2.2 | Attributes `MaxDevoursPerRoom`, `MaxDevourStacks` (base 2) on `UPlayerAttributeSet`. | `PlayerAttributeSet.h/.cpp` |
| 2.3 | New `UDevourComponent` on `APlayerCharacterBase`: `CanDevour()`, `TryDevour(Offer)`, stacks, `DevoursThisRoom`, delegates `OnDevourStacksChanged`, `OnDevourMatured`. | `Component/DevourComponent.h/.cpp` (new), `PlayerCharacterBase.h/.cpp` |
| 2.4 | `AEnemyCharacterBase::TryInteract` → `DevourComponent->TryDevour`; on success destroy the body. `CanInteract` / prompt reflect `CanDevour()` (prompt shows "Devour"; disabled text at cap). | `EnemyCharacterBase.cpp`, `InteractPromptComponent` text |
| 2.5 | Run state: `FRunState.ActiveDevours`, captured in `AYogGameMode::TransitionToLevel` (next to `CaptureCombatLoadoutForRunState`), restored in `RestoreRunStateFromGI` with the ++/mature logic from §4. Mirror in `YogSaveGame` / `YogSaveSubsystem` and `UMetaNodeEditProxy`. | `YogGameInstanceBase.h`, `YogGameMode.cpp`, `PlayerCharacterBase.cpp`, save files |
| 2.6 | HUD: stack icons with "rooms left" (bind to delegates). | WBP |
| 2.7 | Content: `DA_DevourOffer_Default` (LuaModule `Devour.Default`), `GE_Devour_Default_Temp`, a permanent GE. Buff logic is Lua, not a BuffFlow rune. | Content, `Content/Script/Devour/` |

### Phase 3 — Dismember combat item (R4)

| Step | Change | File(s) |
| --- | --- | --- |
| 3.1 | `ELootType::CombatItem` + `FLootOption::CombatItemAsset`, `Amount` = charges. Granted by `UYogLootSubsystem::GrantLoot` → `UCombatItemComponent::AddItem`. Immediate-grant path only (like WeaponSkill). | `LevelFlowTypes.h`, `YogLootSubsystem.cpp`, `RewardPickup.cpp` (`GrantImmediateLoot` switch) |
| 3.2 | `UCombatItemComponent::AddItem(Asset, Charges)`; `FRuntimeSlot.Charges`; stack the same `ItemId`; decrement on use instead of `RemoveAt`; max slot count (designer value). | `CombatItemComponent.h/.cpp` |
| 3.3 | Persist: `FRunState.CombatItems`, capture / restore, save-game mirror. | same files as 2.5 |
| 3.4 | Content: one item (e.g. `DA_CombatItem_RatBone` + GA on its `ActivationTag`), added to `DA_Rat.KillRewards`. | Content |

### Phase 4 — Tests & docs

| Step | Change |
| --- | --- |
| 4.1 | Automation tests (next to `Tests/RewardPickupTests.cpp`): body stays without kill rewards; dismember after N hits; per-room and stack caps; room-entry counting matures on the 3rd entry; run-state round trip for devours and combat items. |
| 4.2 | Designer guide `CorpseDevour_Guide.md` under `03_策划配置与制作手册/核心配置说明/关卡/` + INDEX entry. |

## 6. Order & Risk

| Order | Deliverable | Risk |
| --- | --- | --- |
| 1 | Phase 1 | Low. Every mob now keeps a body, so check frame cost with ~20 paused bodies, and check any Lua / Story listener that expected a death drop. |
| 2 | Phase 2 | Medium: GE re-application on restore must run after the ASC is initialized, and bodies left in a room must not be devourable after the portal transition starts. |
| 3 | Phase 3 | Medium: touches loot types and save code. |
| 4 | Phase 4 | — |

## 7. Out of Scope

- FloorGen streamed rooms (M1). Rooms stop using `OpenLevel`, so "room entered" must come from the floor director instead of run-state restore. Put the ++/mature logic in `UDevourComponent::OnRoomEntered()` so only the caller changes.
- Devour / Dismember montages, VFX polish, ragdoll bodies.
