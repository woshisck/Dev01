# GameMode Slimming: Remove Mob Spawning and Pickup Logic — Plan

| Item | Content |
| --- | --- |
| Scope | `AYogGameMode` (`Source/DevKit/Public/GameModes/YogGameMode.h`, `Private/GameModes/YogGameMode.cpp`) and every caller of its spawn / loot / pickup members |
| Audience | Programmers (implementation), designers (asset migration) |
| Related docs | [LootTypeChoicePopup_TODO.md](LootTypeChoicePopup_TODO.md), [LootSelection_Technical.md](../04_开发实现与系统文档/系统/UI/LootSelection_Technical.md) |
| Last updated | 2026-10-29 |
| Status | **C++ done, not compile-verified.** Editor asset steps (section "Asset Migration") still pending |

## Goals

1. `AYogGameMode` contains **no mob spawning logic**. Mobs come only from `AMobSpawner` + the Lua `AYogRoomDirector` (`Content/Script/Maps/RoomDirector.lua`).
2. `ARewardPickup` has **no dependency on `AYogGameMode`**: no phase gate, no loot generation, no grant, no spawn, no pickup class lookup.

## Why This Is Mostly Dead Code Already

`AYogGameMode::StartPlay` has the call to `StartLevelSpawning()` / `FallbackToPreplacedEnemies()` commented out
inside the `==SunChuankai start/end==` block. Every wave, room-resolution and room-clear path hangs off those two
calls, so at runtime today:

| Never runs | Consequence |
| --- | --- |
| Wave system (`GenerateWavePlans` → `TriggerNextWave` → `SpawnEnemyFromPool` …) | GM spawns nothing; `TotalAliveEnemies` stays 0 |
| `CheckLevelComplete` → `EnterArrangementPhase` | `CurrentPhase` stays `Combat` forever (only the cheat command moves it) |
| Room-clear `RewardPickup` spawn inside `EnterArrangementPhase` | GM-spawned room rewards never appear |
| `ActiveRoomData` resolution | `ActiveRoomData->LootPool` is never used; rune generation always falls back to `FallbackLootPool` |

So deleting the spawn system is behaviour-neutral. The pickup work is **not** neutral: pickups still call into the GM
for phase, rune generation and rune granting, and those calls must be re-homed before the GM code is removed.

> The `==SunChuankai==` block in `StartPlay` is a hand edit and must stay untouched. After this refactor it is a
> comment that names a deleted function, which is harmless.

## Target Architecture

```
Before                                         After
------                                         -----
RewardPickup ──IsPickupAllowed──▶ GM.Phase     RewardPickup ── bPickupEnabled (own state, Lua/BP toggles it)
RewardPickup ──GenerateIndependent──▶ GM       RewardPickup ──GenerateRuneOptions──▶ UYogLootSubsystem
LootSelectionWidget ──SelectLoot──▶ GM         LootSelectionWidget ──GrantLoot──▶ UYogLootSubsystem
Enemy / LENode / SNode ──RewardPickupClass──▶  Enemy / LENode / SNode ──▶ UYogSettings.DefaultRewardPickupClass
     GM                                              + ARewardPickup::SpawnWithLoot (static)
GM.EnterArrangementPhase ──spawns pickup       RoomDirector.lua ──on room clear──▶ enables / spawns pickups
GM wave system ──spawns mobs                   MobSpawner + RoomDirector.lua (unchanged)
```

New pieces:

| Piece | Kind | Owns |
| --- | --- | --- |
| `UYogLootSubsystem` | `UWorldSubsystem` (`Public/System/YogLootSubsystem.h`) | Rune option generation, per-level duplicate tracking, the single grant path for every `ELootType`, the `OnLootSelected` event |
| `URewardLootTableDataAsset` | `UPrimaryDataAsset` (`Public/Data/RewardLootTableDataAsset.h`) | `RunePool`, `DefaultOptionCount`, `SingleChoiceTag`, `SingleChoiceOptionCount` (the four fields that live on the GM today) |
| `UYogSettings` additions | Project settings | `DefaultRewardPickupClass` (soft class), `DefaultRewardLootTable` (soft object) |

## Phase A — Decouple the Pickup (do first, compiles on its own)

### A1. Loot subsystem + loot table

- [ ] Create `URewardLootTableDataAsset` with the four loot fields moved from the GM (`FallbackLootPool`, `DefaultLootOptionCount`, `SingleChoiceLootGameplayTag`, `SingleChoiceLootOptionCount`)
- [ ] Create `UYogLootSubsystem`:

| Function | Ported from | Notes |
| --- | --- | --- |
| `GenerateRuneOptions(const URewardLootTableDataAsset* Table)` | `GenerateLootBatch` + `GetCurrentLootOptionCount` + `ApplyLootOptionLimit` | `Table == nullptr` → `UYogSettings::DefaultRewardLootTable`; dedup set lives on the subsystem, reset in `OnWorldBeginPlay` (world = level, so per-level dedup is automatic) |
| `GrantLoot(APlayerCharacterBase*, const FLootOption&)` | `AYogGameMode::SelectLoot` + `ARewardPickup::GrantImmediateLoot` body | One switch for Rune / Gold / Material / WeaponSkill; broadcasts `Story.Event.Item.Obtained` for runes and fires `OnLootSelected` |
| `FOnLootSelected OnLootSelected` | `AYogGameMode::OnLootSelected` | Native multicast, same signature |

- [ ] Add `DefaultRewardPickupClass` and `DefaultRewardLootTable` to `UYogSettings`

### A2. RewardPickup

- [ ] Replace the GM phase gate in `IsPickupAllowed()` with pickup-owned state:
  - `UPROPERTY(EditAnywhere) bool bStartEnabled = true;`
  - `UFUNCTION(BlueprintCallable) void SetPickupEnabled(bool bEnabled);` (calls `RefreshPickupAvailability`)
- [ ] Delete `bAllowPickupOutsideArrangement` — it only exists to bypass the GM gate. Callers that set it (`SpawnRewardPickupAtLocation`, `LENode_SpawnRewardPickup`, `SNode_SpawnRewardPickup`, `StoryEncounterRuntimeSubsystem`) just drop the line
- [ ] Add `UPROPERTY(EditAnywhere) TObjectPtr<URewardLootTableDataAsset> LootTable;` (null = project default)
- [ ] `TryPickup` / `ResolveLootTypeChoice`: replace `GM->GenerateIndependentLootOptions()` with `LootSubsystem->GenerateRuneOptions(LootTable)`; drop the `GM` null check from the dependency guard
- [ ] `GrantImmediateLoot`: loop over options calling `LootSubsystem->GrantLoot`; keep the `K2_On*Granted` events and FX on the pickup
- [ ] Add `static ARewardPickup* SpawnWithLoot(UObject* WorldContext, TSubclassOf<ARewardPickup> Class, FVector Location, const TArray<FLootOption>& Options, bool bPlayFocusCue)`. Null class → `UYogSettings::DefaultRewardPickupClass`
- [ ] Remove `#include "GameModes/YogGameMode.h"` from `RewardPickup.cpp`

### A3. Callers that leave the GM

| File | Today | After |
| --- | --- | --- |
| `UI/LootSelectionWidget.cpp` `ShowLootUI` | Copies options into `GM->CurrentLootOptions` | Delete the sync block |
| `UI/LootSelectionWidget.cpp` `SelectRuneLoot` | `GM->SelectLoot(Index)` | `LootSubsystem->GrantLoot(Player, CurrentLootOptions[Index])` |
| `UI/LootSelectionWidget.cpp` `NativeConstruct` / `NativeDestruct` | Binds `GM->OnPhaseChanged` | Keep (phase stays on the GM, see Phase B) |
| `Character/EnemyCharacterBase.cpp` kill drop | `GameMode->SpawnEnemyKillRewardPickup(...)` | `ARewardPickup::SpawnWithLoot(this, nullptr, ...)` |
| `LevelFlow/Nodes/LENode_SpawnRewardPickup.cpp` | Falls back to `GM->RewardPickupClass` | Falls back to settings via `SpawnWithLoot` |
| `Story/Flow/Nodes/SNode_SpawnRewardPickup.cpp` | Same | Same |
| `Story/Encounter/StoryEncounterRuntimeSubsystem.cpp` (pickup class) | Same | Same |
| `LevelFlow/Nodes/LENode_WaitForLootSelected.cpp` | Binds `GM->OnLootSelected` | Binds `LootSubsystem->OnLootSelected` |
| `Story/Flow/Nodes/SNode_SetRoomRewardOverride.cpp`, `StoryEncounterRuntimeSubsystem.cpp` (override) | `GM->Set/ClearRoomRewardOptionsOverride` | See **Open Question 1** |

### A4. Remove pickup / loot members from the GM

| Remove | Kind |
| --- | --- |
| `RewardPickupClass`, `FallbackLootPool`, `DefaultLootOptionCount`, `SingleChoiceLootGameplayTag`, `SingleChoiceLootOptionCount` | Config (migrate values first, see Phase C) |
| `CurrentLootOptions`, `bLootOptionsPending`, `LootAssignedThisLevel`, `LastEnemyKillLocation` | State |
| `OnLootGenerated`, `OnLootSelected` + their delegate declarations | Events |
| `SelectLoot`, `ShowLootOptions`, `GenerateLootOptions`, `GenerateIndependentLootOptions`, `GenerateLootBatch`, `GetCurrentLootOptionCount`, `ApplyLootOptionLimit` | Loot |
| `SpawnRewardPickupAtLocation`, `SpawnEnemyKillRewardPickup`, `SpawnStorySpecialRewardPickup`, `FindLootSpawnLocation` | Pickup spawn |
| `ShouldAllowExtraRewardPickupForRoom`, `bEnableExtraRewardPickups`, `SacrificeDropChance`, extra-pickup block in `EnterArrangementPhase` | Extra pickup roll |
| Room-clear pickup block in `EnterArrangementPhase`, `bSuppressRoomClearRewardPickup` | Room-clear pickup |
| `DescribeGameModeLootOptionsForRewardDebug` + all `[StoryRewardDebug] GM …` logs | Debug |
| `#include "Map/RewardPickup.h"` | Include |

## Phase B — Remove Mob Spawning from the GM

### B1. Delete outright

| Remove | Notes |
| --- | --- |
| `StartLevelSpawning` | Whole function (~370 lines). Also does room resolution, campaign stage broadcast, hub / shop / event room branches and run-state restore — all already dead, see **Open Question 3** |
| `GenerateWavePlans`, `BuildWavePlan`, `TriggerNextWave`, `SpawnNextOneByOne`, `SetupWaveTrigger`, `OnWaveTriggerFired`, `CheckWaveTrigger`, `CheckDemandSpawn` | Wave system |
| `FallbackToPreplacedEnemies`, `CheckLevelComplete` | Room-clear detection (the Lua director owns this now) |
| `SpawnEnemyFromPool`, `BeginSpawnEnemyFromPool`, `FinishSpawnFromPool`, `HandleLifecycleEnemySpawned`, `HandleLifecycleEnemySpawnFailed` | Spawn pipeline |
| `MarkStorySpecialRewardEnemy`, `bStorySpecialRewardEnemyEnabled`, `StorySpecialRewardEnemyLootOptions`, `StorySpecialRewardEnemyAuraFX` | Story special-reward enemy |
| `SelectRoomBuffs`, `ActiveRoomBuffs`, `GetActiveRoomBuffs` | Only applied to GM-spawned enemies (check `HUD` room-buff display first, see B3) |
| `HandleTimedClearObjectiveExpired`, `TimedClearObjectiveTimer`, `bTimedClearObjective*` | Started only in `StartLevelSpawning` |
| `FPlannedEnemy`, `FWavePlan`, `WavePlans`, `CurrentWaveIndex`, `TotalAliveEnemies`, `bAllWavesSpawned`, `PendingSpawnCount`, `OneByOneSpawnQueue`, `OneByOneSpawnIndex`, `bWaveStaggerMode`, `TotalLevelPlannedEnemies`, all wave `FTimerHandle`s | State |
| `InitialSpawnDelay`, `LowDifficultyScoreMax`, `HighDifficultyScoreMin`, `SpawnStaggerMin/Max`, `OneByOneDefaultInterval`, `ActiveGoldMin/Max`, `ActiveBuffCount` | Config |
| `ShouldSkipCombatForRoom` + its test cases in `Tests/GameModeRoomFlowTests.cpp` | Only used by `StartLevelSpawning` |

### B2. Move out

| Member | Caller | Move to |
| --- | --- | --- |
| `SpawnGMEnemyNearPlayer` | `System/YogRuntimeGMSubsystem.cpp` (debug panel) | `UYogRuntimeGMSubsystem`; drop `bCountForLevelClear` / `bApplyRoomBuffs` params |
| `HandleLifecycleEnemySpawned/Failed` calls | `BuffFlow/Nodes/BFNode_SpawnEnemyFromContext.cpp` | Remove the calls. No content asset references this node (searched `Content/`), so delete the node too if nothing else uses `FBuffFlowLifecycleContext` |

### B3. Keep (not spawning, still has live callers)

| Member | Why it stays |
| --- | --- |
| `RegisterEnemy`, `UnregisterEnemy`, `HasAliveEnemies`, `GetEnemyCentroid`, `GetNearestEnemyDirection`, `GetNearbyEnemies`, `GetAllAliveEnemies` | Enemy registry used by camera, AI controller, HUD arrows, `AMobSpawner`, `YogLuaSpawnLibrary`. Moving it to a world subsystem is a separate follow-up |
| `UpdateFinishLevel` | Called by `EnemyCharacterBase`, `TrainingDummyCharacter` and `B_EnemyDummy` (BP). Keep kill count + save stats, delete the wave block |
| `CurrentPhase`, `OnPhaseChanged`, `EnterArrangementPhase` | Read by backpack, camera, altar, save, cheat, `B_GM_LootOnly` (BP). Add `UFUNCTION(BlueprintCallable) SetLevelPhase(ELevelPhase)` so `RoomDirector.lua` drives it, see **Open Question 2** |
| `EnterArrangementPhase` body | Keep: set phase + broadcast, level-end effect, `LevelClear` lifecycle event, run-state snapshot, checkpoint. Delete: gold reward (reads dead `ActiveGold*`), loot spawn, extra pickup, altar spawn (Open Question 3) |
| Player death / revive, transition, campaign data, story events, lifecycle flow | Unrelated to this refactor |

### B4. Data types (keep for now)

`SpawnTypes.h`, the enemy pool fields on `URoomDataAsset` / `UCampaignDataAsset` FloorTable, and the `BuildWavePlan`
comment in `EnemyData.h` stay untouched in this pass. Removing serialized struct fields drops the data in every asset
silently, and `AMobSpawner` still includes `SpawnTypes.h`. Mark the fields `DeprecatedProperty` in a follow-up once
nothing reads them.

## Phase C — Asset Migration (editor, before Phase A4 compiles)

Values on GM properties are lost the moment the property is deleted. Copy them first.

- [ ] Open the GameMode BP (`BP_GameMode_Default` and any other `AYogGameMode` children) and note `RewardPickupClass`, `FallbackLootPool`, `DefaultLootOptionCount`, `SingleChoiceLootGameplayTag`, `SingleChoiceLootOptionCount`
- [ ] Create `DA_RewardLootTable_Default` with those loot values
- [ ] Project Settings → Yog: set `DefaultRewardPickupClass` = `BP_RewardPickup`, `DefaultRewardLootTable` = `DA_RewardLootTable_Default`
- [ ] Placed `BP_RewardPickup` instances that had `bAllowPickupOutsideArrangement` ticked: nothing to do (`bStartEnabled` defaults to true). Instances that must wait for room clear: untick `bStartEnabled` and let `RoomDirector.lua` call `SetPickupEnabled(true)`
- [ ] `RoomDirector.lua`: in `OpenPortals` (room cleared), enable every `ARewardPickup` in the map, and call `SetLevelPhase(Arrangement)` if Open Question 2 is accepted
- [ ] Re-test assets that touch the moved APIs:

| Asset | Uses |
| --- | --- |
| `Content/Code/Characters/B_EnemyDummy` | `UpdateFinishLevel` (kept) |
| `Content/Developers/sunchuankai/LootTest/B_GM_LootOnly` | `EnterArrangementPhase` (kept) |
| `Content/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard` | `SNode_SpawnRewardPickup` (pickup class now from settings) |

## Phase D — Tests and Docs

- [ ] `Tests/RewardPickupTests.cpp`: delete the `ShouldAllowExtraRewardPickupForRoom` cases; keep `ShouldGrantLootImmediatelyForOptions` cases
- [ ] `Tests/GameModeRoomFlowTests.cpp`: delete `ShouldSkipCombatForRoom` cases; keep `FindRoomDataForLoadedMap` / `ShouldPreserveCurrentMapForEditorPlay`
- [ ] `Tests/StoryRewardOverrideTests.cpp`: follow Open Question 1
- [ ] Update `LootSelection_Technical.md` (GM no longer in the grant path) and the GameMode / wave spawning docs under `Docs/`
- [ ] Update the Level Flow / Wave Spawning paragraphs in `CLAUDE.md`

## Suggested Order

```
A1 subsystem + table ─▶ A2 pickup ─▶ A3 callers ─▶ build + PIE test (GM members still present, unused)
   ─▶ C asset migration ─▶ A4 delete GM loot members ─▶ build
   ─▶ B1/B2 delete spawn system ─▶ build ─▶ D tests + docs
```

Each arrow ending in "build" is a safe commit point.

## Decisions Taken (2026-10-29)

| Question | Decision | Effect |
| --- | --- | --- |
| Story reward override | Remove completely, including its encounter action, flow node and next-room plan | `SNode_SetRoomRewardOverride`, `SNode_SetNextRoomPlan`, `FStoryNextRoomPlan`, `EStoryRewardOverrideTarget`, the GameInstance pending override and plan, and their tests are deleted. The action kind keeps a hidden `Deprecated_SetRoomRewardOverride` slot, and an enum redirect in `DefaultEngine.ini` maps the old name onto it so existing encounter assets still load |
| Who drives the phase | Nobody; remove | `ELevelPhase`, `CurrentPhase`, `OnPhaseChanged`, `EnterArrangementPhase` and the `Yog_SetLevelEnded` cheat are deleted. The camera and backpack lock now ask `HasAliveEnemies()`; the altar is active until consumed |
| Shop, altar, hub terminal, gold rewards | Delete now | Gone with `StartLevelSpawning` and `EnterArrangementPhase`. `AShopActor` and `AAltarActor` classes stay as placed actors |
| Spawn data structs | Delete now | `FRoomDifficultyTier`, `FFloorConfig`, the spawn enums, and every `URoomDataAsset` / `UCampaignDataAsset` field that fed the wave system are deleted. `FEnemyEntry` stays because `AMobSpawner::MobPool` uses it |

## What Changed

| Area | Result |
| --- | --- |
| `AYogGameMode` | 3566 lines down to 733. Keeps player spawn, death / revive, kill count, enemy registry, `TransitionToLevel`, lifecycle events |
| `ARewardPickup` | No GameMode reference. `bPickupEnabled` replaces the phase gate and `bAllowPickupOutsideArrangement`. `SpawnWithLoot` is the single spawn helper |
| `UYogLootSubsystem` (new) | Rune option generation with per-world dedup, plus `GrantLoot` for every `ELootType` |
| `URewardLootTableDataAsset` (new) | Rune pool and card count; project default in Project Settings > Yog |
| `URoomDataAsset` | Now only `LevelAsset`, `RoomName`, `DisplayName`, `RoomTags` |
| `UCampaignDataAsset` | Now only `LayerTag`, `RoomPool`, `DefaultStartingRoom` |
| `RoomDirector.lua` | On room clear also enables pickups that were placed with `bPickupEnabled` off |
| Debug panel enemy spawn | Moved to `UYogRuntimeGMSubsystem`; room-buff and level-clear options removed |

## Asset Migration (editor, still to do)

- [ ] Project Settings > Yog: create `DA_RewardLootTable_Default` (Reward Loot Table), fill `RunePool` with the runes that used to sit in the GameMode's `FallbackLootPool`, then assign it to `DefaultRewardLootTable`. `DefaultRewardPickupClass` is already set to `BP_RewardPickup` in `DefaultGame.ini`
- [ ] Open `B_GameMode`, `BP_GameMode_TwoLevelTest` and `B_GM_LootOnly`, compile, and confirm no node is left in an error state (they may reference deleted properties such as `RewardPickupClass`)
- [ ] Open `DA_Campaign_MainRun`, `DA_Campaign_LootOnly` and every `DA_Room_*`: removed fields are dropped silently on load, so re-save them
- [x] The first-run tutorial (encounter points, flows, rule set, tags, docs) was deleted entirely on 2026-10-29, so the reward override clean-up there no longer applies
- [ ] `BP_RewardPickup`: check Class Defaults under Reward | Pickup (`bPickupEnabled`, `LootTable`) and that the old Reward | Story box is gone
- [ ] Map scripts: a pickup that should wait for room clear needs `bPickupEnabled` unticked; `RoomDirector.lua` reveals it

## Not Done

| Item | Why |
| --- | --- |
| Compile | Not run; the project owner builds |
| `FFloorConfig`-style floor sequencing | Removed with the wave system. Reintroduce per map in Lua if a run structure is needed again |
| `AYogHUD::TriggerLevelEndEffect` | Kept as a HUD feature, but nothing calls it now |
