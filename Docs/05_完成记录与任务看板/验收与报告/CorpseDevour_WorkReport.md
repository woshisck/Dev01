# Corpse Devour / Dismember — Work Report

> 适用范围: Corpse Devour / Dismember implementation progress (Phase 1 + Phase 2 of the plan)
> 适用人群: Programmer, Designer
> 配套文档: [CorpseDevourDismember_Plan](../../01_策划需求与版本方案/核心方案/01_开发方案/关卡/CorpseDevourDismember_Plan.md)
> 最后更新: 2026-11-22

## 1. Status

**Code written, not compiled, not tested in PIE.**

| Phase | Scope | Status |
| --- | --- | --- |
| 1 | Body always stays, loot only from Dismember | Code done. Death-montage content fixes (F1) and PIE check open |
| 2 | Devour: caps, room counting, Lua buff, run-state persistence | Code done. Content, prompt text, HUD open |
| 3 | Dismember combat item | Not started |
| 4 | Automation tests, designer guide | Not started |

## 2. Key Decision: Devour Buffs Are Lua, Not Runes

The original plan gave each devour a temp GE plus a hidden passive rune (BuffFlow). On 2026-11-22 this changed to a **Lua module per offer**, following the `Content/Script/Traits/SwarmFrenzy.lua` pattern for enemies.

| Owner | Responsibility |
| --- | --- |
| C++ `UDevourComponent` | Caps (R5 / R6), room counting, maturity, save / restore across rooms and save games, calling Lua hooks |
| Lua module (`Content/Script/Devour/`) | Everything the buff does: GEs, tags, attributes, combat-event reactions |

Reasons:

| | Rune / BuffFlow | Lua |
| --- | --- | --- |
| Permanent phase | `AddHiddenPassiveRune` is compiled out while the card test flag is on | Unaffected |
| Logic-heavy buffs | Awkward in a flow graph | Natural |
| Plain stat buff | GE only | GE in `EffectParams`, applied by the script |

## 3. Runtime Flow

```
Enemy dies
  └─ body stays (EnemyData.bLootableCorpse, default true; no loot roll)

Player presses Interact on body
  └─ AEnemyCharacterBase::TryInteract
       └─ CanInteract -> UDevourComponent::CanDevour  (DevoursThisRoom < MaxDevoursPerRoom
       │                                               && temp stacks < MaxDevourStacks)
       └─ ResolveDevourOffer  (EnemyData.DevourOffer, else Project Settings > Yog > DefaultDevourOffer)
       └─ UDevourComponent::TryDevour -> Lua OnTempBegin(Player, Offer, Id, 0)
       └─ CorpseDevourFX, body Destroy

Player hits body N times (EnemyData.CorpseHitsToExplode, shown as "Corpse Hits To Dismember")
  └─ DismemberCorpse -> RollKillRewards -> scatter pickups -> body Destroy (even on empty roll)

Portal -> AYogGameMode::TransitionToLevel
  └─ DevourComponent->CaptureRunState(FRunState.Devours)

New room, player respawned -> APlayerCharacterBase::RestoreRunState
  └─ DevourComponent->RestoreFromRunState
       ├─ matured                        -> Lua OnPermanentBegin
       ├─ ++RoomsEntered > DurationRooms -> mature -> Lua OnPermanentBegin
       └─ otherwise                      -> Lua OnTempBegin (re-apply on the new player)

Back to hub -> ClearRunCarriedStateForHub -> ResetForNewRun -> Lua OnRemoved
```

Room counting matches plan §1.1: the devour room does not count; with `DurationRooms = 2` the 3rd room entered matures the devour.

## 4. Lua Hooks

Every hook is optional and receives `(Player, Offer, Id[, RoomsEntered])`. `Id` is unique for the whole run and survives level changes, so scripts key state by `Player` and `Id`.

| Hook | Called when |
| --- | --- |
| `OnTempBegin(Player, Offer, Id, RoomsEntered)` | The devour, and on every respawned player while the temp phase lasts |
| `OnRoomEntered(Player, Offer, Id, RoomsEntered)` | A room entered without respawning (cheat, future streamed floors) |
| `OnTempEnd(Player, Offer, Id)` | Temp phase ends on a live player (only via `OnRoomEntered`) |
| `OnPermanentBegin(Player, Offer, Id)` | Maturity, and on every later respawned player |
| `OnRemoved(Player, Offer, Id)` | Run reset |

Every call is pcall-guarded. Errors log as `[Devour][Lua]`.

### 4.1 `UPlayerBuffLuaLibrary` (Lua: `UE.UPlayerBuffLuaLibrary`)

| Function | Use |
| --- | --- |
| `GetPlayerASC(Player)` | Bind `OnKilledTarget`, `DealtDamage`, `OnDashExecuted`, `ReceivedDamage` |
| `ApplyPlayerEffect(Player, Effect, Level)` / `RemovePlayerEffect(Player, Handle)` | GEs |
| `AddPlayerLooseTag` / `RemovePlayerLooseTag` / `HasPlayerTag` | Loose tags (counted) |
| `GetPlayerAttribute(Player, Name)` / `AddPlayerAttributeBase(Player, Name, Delta)` | Any attribute by bare property name |
| `HealPlayer(Player, Amount)` | Heal clamped to MaxHealth |

### 4.2 Sample: `Content/Script/Devour/Default.lua`

| Param | Type | Default | Meaning |
| --- | --- | --- | --- |
| `TempEffect` | EffectParams | — | Infinite GE during the temp phase |
| `PermanentEffect` | EffectParams | — | Infinite GE during the permanent phase |
| `TempEffectLevel` / `PermanentEffectLevel` | NumberParams | 1 | GE level |
| `TempHealOnKill` / `PermanentHealOnKill` | NumberParams | 0 | Health per kill in that phase |

## 5. Data

| Asset / setting | Field | Notes |
| --- | --- | --- |
| `UDevourOfferDataAsset` | `LuaModule`, `NumberParams`, `EffectParams`, `DurationRooms` (2), `DisplayName`, `Description`, `Icon` | Old `TempEffect` / `PermanentPassive` fields removed |
| `UEnemyData` | `bLootableCorpse` (true), `CorpseHitsToExplode` (3), `DevourOffer`, `CorpseDevourFX` | |
| `UYogSettings` | `DefaultDevourOffer` | Not set yet in any `Config/*.ini` |
| `UPlayerAttributeSet` | `MaxDevoursPerRoom` (2), `MaxDevourStacks` (2) | Buff GEs can raise them; clamped ≥ 0 |
| `FRunState` / `FRunCheckpointData` | `Devours` (`FDevourRunEntry` / `FDevourCheckpointEntry`) | Offer, Id, RoomsEntered, bMatured |

## 6. Files Changed

| File | Change |
| --- | --- |
| `Character/EnemyCharacterBase.h/.cpp` | Body always stays; `DismemberCorpse`; Devour interact; `ResolveDevourOffer` |
| `Data/EnemyData.h` | `DevourOffer`, `CorpseDevourFX`, Dismember display name |
| `Data/DevourOfferDataAsset.h` (new) | Offer asset + run / checkpoint entry structs |
| `Component/DevourComponent.h/.cpp` (new) | Rules, room counting, persistence, Lua bridge |
| `Character/PlayerBuffLuaLibrary.h/.cpp` (new) | Player-side Lua library |
| `Character/PlayerCharacterBase.h/.cpp` | `DevourComponent`; restore in `RestoreRunState`; reset in `ClearRunCarriedStateForHub` |
| `AbilitySystem/Attribute/PlayerAttributeSet.h/.cpp` | Cap attributes |
| `System/YogSettings.h` | `DefaultDevourOffer` |
| `System/YogGameInstanceBase.h` | `FRunState.Devours` |
| `GameModes/YogGameMode.cpp` | Capture in `TransitionToLevel` |
| `SaveGame/YogSaveGame.h`, `SaveGame/YogSaveSubsystem.cpp` | Checkpoint mirror |
| `DevKitEditor/Tools/UMetaNodeEditProxy.h/.cpp` | Save-slot editor mirror |
| `Cheater/Cheater.h/.cpp` | `Yog_DevourNextRoom`, `Yog_PrintDevours` |
| `Content/Script/Devour/Default.lua` (new) | Sample buff |

## 7. Editor Setup Still Needed

| # | Step |
| --- | --- |
| 1 | Create `DA_DevourOffer_Default`: `LuaModule = Devour.Default`, `EffectParams.TempEffect = GE_Devour_Default_Temp`, optional `PermanentEffect` and heal params |
| 2 | Project Settings > Yog > **Default Devour Offer** = `DA_DevourOffer_Default` |
| 3 | Delete `Content/Code/Devour/DA_Rune_Devour_Default_passive` (unused now) |
| 4 | Set **Blend Out Time = 0** on every death montage (plan §2, F1) |

## 8. Test Checklist

| # | Check | Expected |
| --- | --- | --- |
| 1 | Kill several mob types | Bodies stay, nothing drops, portal still opens |
| 2 | Hit a body 3 times | Dismember FX, loot scatters, body removed |
| 3 | Devour 2 bodies, try a 3rd | 3rd shows no prompt / does nothing |
| 4 | Devour, then `Yog_DevourNextRoom` ×3 with `Yog_PrintDevours` | temp (1/2), temp (2/2), PERMANENT |
| 5 | Devour, walk through 3 real portals | Temp GE present in rooms 2–3, permanent from room 4 |
| 6 | Save checkpoint, quit, continue | Devours restored with the same state |
| 7 | Return to hub | All devour effects removed |
| 8 | Heal-on-kill phase end | Heals stop after the phase ends. **Unverified risk:** `OnKilledTarget:Remove` with a closure; check here first if heals continue |

## 9. Open Items

| Item | Phase |
| --- | --- |
| Interact prompt text "Devour" + disabled text at cap; refresh prompts on other bodies after a devour | 2.4 |
| HUD stack icons (bind `OnDevourStacksChanged` / `OnDevourMatured`) | 2.6 |
| Content assets (§7) | 2.7 |
| Combat item loot type, `UCombatItemComponent::AddItem`, persistence | 3 |
| Automation tests, `CorpseDevour_Guide.md` | 4 |
| FloorGen streamed rooms: call `UDevourComponent::OnRoomEntered()` from the floor director instead of relying on respawn | Out of scope (M1) |
