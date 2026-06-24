# LOTF Gameplay Tags — Analysis Report

_Generated 2026-06-23. Scope: all gameplay-tag sources in `LOTF2/`._

---

## 1. Sources of gameplay tags (5 authoritative)

| Source | File / Location | Count | Role |
|---|---|---|---|
| **Main tag dictionary** | `Config/DefaultGameplayTags.ini` | **6,646** tags + **186** redirects | Authored, designer-facing master list. Loaded via `ImportTagsFromConfig=True`. |
| **State/save flags** | `Config/Tags/GameplayFlags.ini` | **2,995** tags (2,993 are `GameState.Flags.*`) | Runtime/persistent world & quest flags, split into their own file. |
| **Native combat/ability tags** | `Source/LOTF2/Public/AbilitySystem/Tags/*.h` | **~705** | C++-declared tags for compile-time access in gameplay code. |
| **Native UI tags** | `Source/LOTF2/Private/UI/HexUITags.cpp` | **51** | CommonUI framework tags (layers / actions / extensions). |
| **Plugin tags** | `Plugins/GameSubtitles/Config/Tags/PluginTags.ini` | **2** | `Subtitle.TextColor.White/Yellow`. |

**Grand total ≈ 10,400 distinct gameplay tags.**

> Generated caches — `Intermediate/Config/CoalescedSourceConfigs/GameplayTags.ini` and `Saved/Config/WindowsEditor/GameplayTags*.ini` — are engine-derived, **not authoritative**. Do not edit.

---

## 2. `DefaultGameplayTags.ini` — the master dictionary

Header (`[/Script/GameplayTags.GameplayTagsSettings]`):

- `ImportTagsFromConfig=True` — the `.ini` is the source of truth.
- `FastReplication=True` — tags replicate as indices, not strings (requires **identical** tag lists on client and server).
- `WarnOnInvalidTags=True`, `ClearInvalidTags=False`, `AllowGameTagUnloading=False`.

**186 `GameplayTagRedirects`** — the tree was migrated from a *flat* scheme to a *hierarchical* one (e.g. `DodgeAbility` → `Ability.DodgeAbility`, `TargetAbility` → `Ability.TargetAbility`, `Ability_Sneak` → `SneakAbility`). Redirects keep old assets/Blueprints/saves working.

### Top-level categories (by tag count)

| Root | Count | Usage |
|---|---|---|
| Animation | 1,653 | Montages, movesets, notify-driven logic. Mostly animator-authored. |
| GameplayCue | 1,103 | Cosmetic feedback (VFX/SFX/camera shake) triggered by GAS. Presentation only. |
| Combat | 596 | Combat state machine: `State`, timing windows (`AttackWindow`/`ParryWindow`), `WeakTo`/`ResistantTo`, `DeathReason`, `Modifier`. |
| Character | 579 | Identity & state: `Player`, `Enemy`, `Boss`, `Npc`, `Summon`. Targeting/faction filters. |
| Quest | 361 | Quest/objective tracking. |
| GameplayEvent | 339 | Event signals through GAS (`Damage`, `Death`, `Interact`, `Magic`, `Input`). Pub/sub channel. |
| AI | 239 | Enemy brains: `Behavior`, `Decision`, `State`, `Actions`, `Condition`, `SM`. |
| GameState | 188 | Global session/world state. |
| UI | 180 | HUD/menu visibility, widget-driving. |
| Ability | 148 | Every granted action incl. signature `Ability.DualRealm.*`. |
| Inventory | 142 | Item categories, equipment, invasion items. |
| GameplayEffect | 126 | Tags on buffs/debuffs/damage modifiers. |
| Story | 96 | Narrative progression flags. |
| DamageDef | 94 | Damage definitions (`Player`/`Enemy`/`Weapon`). |
| AnimationMoveset | 91 | Weapon/character movesets. |
| Cooldown | 85 | Ability/effect cooldown gating. |
| Camera | 84 | `CameraVolume`, `Transition`, `Customization`. |
| BattleEffect | 65 | In-combat effects (`Curse`, `RangedAmmo`). |
| Attack | 50 | Per-attack metadata (`Damage`, `Type`, `PoiseReaction`, `Hitstop`). |
| State | 49 | High-level states incl. `State.Umbral`. |
| Combo | 49 | Combo chaining (`Sequence`, `Finisher`, `JumpAttack`). |
| _(others)_ | — | `StatusEffect`, `ItemSet`, `Boss`, `DualRealm`, `Prop`, `Movement`, `GodMode`, etc. |

### Signature mechanic — `DualRealm`

Drives the living **Axiom** vs. dead **Umbral** world. Tags coordinate Transcend (realm shifts), Shining/Materialize, Reincarnate/Resurrect, and Syphoning (e.g. `Ability.DualRealm.Transcend.AxiomToLimbo`, `Ability.DualRealm.Shining.Materialize`).

---

## 3. `GameplayFlags.ini` — why it's separate

Essentially **100% `GameState.Flags.*`** (2,993 of 2,995). Persistent boolean world-state markers, e.g.:

- `GameState.Flags.Persistent.Quest.Quest_Achievement_BountyHunter.Step.0.QT_GameState.MiniBoss.Banshee.Dead`
- `GameState.Flags.HiddenQuest.GoodBoy.Petted`

Split out because the list is **high-churn and machine-touched** (every quest step, miniboss kill, door state adds an entry). Isolating it from the hand-curated dictionary avoids merge conflicts. Functionally it loads into the same global registry.

---

## 4. Native C++ tags (`AbilitySystem/Tags/`)

Declared via custom macro `HexCreateNativeGameplayTag(VarName, "Tag.String")` (defined in `GameplayTagsMacros.h`, an `FNativeGameplayTag` wrapper). Registered at module load.

| Header | Tags | Domain |
|---|---|---|
| `CombatTags.h` | 361 | `GameplayEvent.Combat.*`, parry/damage-dealt events, hit reactions |
| `CommonTags.h` | 229 | `Animation.Locomotion.*`, `CharacterTrait.*`, shared cross-system tags |
| `AbilityTags.h` | 45 | Ability identity tags |
| `DamageTags.h` | 44 | Damage typing |
| `AITags.h` | 38 | AI events/decisions |

**Why native + `.ini` coexist:** the `.ini` gives designers/Blueprints the full tree; native declarations give C++ a typed, compile-checked handle (e.g. `CombatTagsStruct::EventAttackParried`) to the *same* string tags — avoiding fragile `RequestGameplayTag(FName("..."))` lookups. Intentional mirrors, not duplicates.

---

## 5. Native UI tags (`HexUITags.cpp`, 51)

Use Epic's CommonUI macro family (`UE_DEFINE_GAMEPLAY_TAG`). Three groups:

- `UI.Layer.*` — UI stacking layers (HUD, Modal, DeathScreen, FullscreenMenu…).
- `UI.Action.*` — input actions (Confirm, Escape, Skip, PhotoMode, music-player controls…).
- `UI.Extension.*` — injection points for widgets (Notifications, Tutorials, BossStats, StatusEffects…).

---

## 6. Findings & recommendations

1. **Casing/typo tags pollute the tree** — distinct-to-the-engine variants that silently won't match canonical code. Full fix list in §7.
2. **Two naming conventions still coexist** despite 186 redirects (some flat root tags remain: `SneakAbility`, `RunAbility`).
3. **`FastReplication=True` is a hard constraint** — any tag present on one side but not the other desyncs replication. Keep native tag modules loaded symmetrically across client/server.
4. **Clean separation of concerns** — vocabulary (`DefaultGameplayTags.ini`) vs. save/quest flags (`GameplayFlags.ini`) vs. typed C++ handles (`AbilitySystem/Tags/`) is deliberate and healthy.

---

## 7. Malformed tags — fix list

Scanned 9,641 unique authored tags. **19 malformed** (casing/typo), **5 orphan flat tags**, **1 group to verify**.

### ⚠️ Critical caveat

For every casing typo, **the correctly-cased version does NOT already exist** — the typo tag is the only copy. Therefore:

- A `GameplayTagRedirect` alone is **insufficient** (it would point to an unregistered tag).
- **Correct fix = two steps:** (1) rename the `Tag="..."` string in the source `.ini` to canonical casing, **and** (2) add a redirect so existing references resolve.
- **Redirects are not cosmetic here.** Most typos live in `GameState.Flags.*` / `GameState.Boss.*`, written into **player save files**. The redirect is what keeps old saves loading after a rename.
- Best executed in the UE **Gameplay Tag Manager** (right-click → Rename), which updates the source `.ini` and adds the redirect atomically.

### Group A — Casing/typo fixes (19)

| # | Current (wrong) | Canonical target | File |
|---|---|---|---|
| 1 | `gamestate.boss.lamphunter.Dead` | `GameState.Boss.Lamphunter.Dead` | Flags |
| 2 | `gamestate.boss.lamphunter.Dissolved` | `GameState.Boss.Lamphunter.Dissolved` | Flags |
| 3 | `gamestate.boss.lamphunter.PlayerDead.Location1` | `GameState.Boss.Lamphunter.PlayerDead.Location1` | Flags |
| 4 | `gamestate.boss.lamphunter.PlayerDead.Location2` | `…Location2` | Flags |
| 5 | `gamestate.boss.lamphunter.PlayerDead.Location3` | `…Location3` | Flags |
| 6 | `gamestate.boss.lamphunter.PlayerDead.Location4` | `…Location4` | Flags |
| 7 | `Gamestate.Boss.Lamphunter.Dead.UpperCity` | `GameState.Boss.Lamphunter.Dead.UpperCity` | Flags |
| 8 | `Gameplayevent.Enemy.Boss.UmbralAssassin.B` | `GameplayEvent.Enemy.Boss.UmbralAssassin.B` | Default |
| 9 | `Gameplayevent.Enemy.Boss.UmbralAssassin.C` | `GameplayEvent.Enemy.Boss.UmbralAssassin.C` | Default |
| 10 | `GemeplayEvent.Tutorial.Finished` | `GameplayEvent.Tutorial.Finished` | Default |
| 11 | `gameplaycue.aoe.HallowedBro.AuraDispell` | `GameplayCue.AOE.HallowedBro.AuraDispell` | Default |
| 12 | `gameplaycue.enemy.strider.ThrowDrowner` | `GameplayCue.Enemy.Strider.ThrowDrowner` | Default |
| 13 | `gameplayeffect.input.Movement.RotationRate` | `GameplayEffect.Input.Movement.RotationRate` | Default |
| 14 | `combat.state.enemy.npc.BossNotTargetable` | `Combat.State.Enemy.NPC.BossNotTargetable` | Default |
| 15 | `animation.enemy.slaveworker.idle.RummageGetUp` | `Animation.Enemy.SlaveWorker.Idle.RummageGetUp` | Default |
| 16 | `animationmoveset.instance.Boss.Isaac.StatueIntro` | `AnimationMoveset.Instance.Boss.Isaac.StatueIntro` | Default |
| 17 | `AKEventSharedNotify.VO.Battlecry.Stop` | `AkEventSharedNotify.VO.Battlecry.Stop` | Default |
| 18 | `GameplayTag Ability.BuildItem` _(literal space)_ | `Ability.BuildItem` ✅ already exists | Default |

> #18 is the one true duplicate — `Ability.BuildItem` already exists, so just delete the malformed entry + redirect.

### Ready-to-paste redirect block

Append under the existing `+GameplayTagRedirects` lines in `DefaultGameplayTags.ini`:

```ini
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.Dead",NewTagName="GameState.Boss.Lamphunter.Dead")
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.Dissolved",NewTagName="GameState.Boss.Lamphunter.Dissolved")
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.PlayerDead.Location1",NewTagName="GameState.Boss.Lamphunter.PlayerDead.Location1")
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.PlayerDead.Location2",NewTagName="GameState.Boss.Lamphunter.PlayerDead.Location2")
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.PlayerDead.Location3",NewTagName="GameState.Boss.Lamphunter.PlayerDead.Location3")
+GameplayTagRedirects=(OldTagName="gamestate.boss.lamphunter.PlayerDead.Location4",NewTagName="GameState.Boss.Lamphunter.PlayerDead.Location4")
+GameplayTagRedirects=(OldTagName="Gamestate.Boss.Lamphunter.Dead.UpperCity",NewTagName="GameState.Boss.Lamphunter.Dead.UpperCity")
+GameplayTagRedirects=(OldTagName="Gameplayevent.Enemy.Boss.UmbralAssassin.B",NewTagName="GameplayEvent.Enemy.Boss.UmbralAssassin.B")
+GameplayTagRedirects=(OldTagName="Gameplayevent.Enemy.Boss.UmbralAssassin.C",NewTagName="GameplayEvent.Enemy.Boss.UmbralAssassin.C")
+GameplayTagRedirects=(OldTagName="GemeplayEvent.Tutorial.Finished",NewTagName="GameplayEvent.Tutorial.Finished")
+GameplayTagRedirects=(OldTagName="gameplaycue.aoe.HallowedBro.AuraDispell",NewTagName="GameplayCue.AOE.HallowedBro.AuraDispell")
+GameplayTagRedirects=(OldTagName="gameplaycue.enemy.strider.ThrowDrowner",NewTagName="GameplayCue.Enemy.Strider.ThrowDrowner")
+GameplayTagRedirects=(OldTagName="gameplayeffect.input.Movement.RotationRate",NewTagName="GameplayEffect.Input.Movement.RotationRate")
+GameplayTagRedirects=(OldTagName="combat.state.enemy.npc.BossNotTargetable",NewTagName="Combat.State.Enemy.NPC.BossNotTargetable")
+GameplayTagRedirects=(OldTagName="animation.enemy.slaveworker.idle.RummageGetUp",NewTagName="Animation.Enemy.SlaveWorker.Idle.RummageGetUp")
+GameplayTagRedirects=(OldTagName="animationmoveset.instance.Boss.Isaac.StatueIntro",NewTagName="AnimationMoveset.Instance.Boss.Isaac.StatueIntro")
+GameplayTagRedirects=(OldTagName="AKEventSharedNotify.VO.Battlecry.Stop",NewTagName="AkEventSharedNotify.VO.Battlecry.Stop")
+GameplayTagRedirects=(OldTagName="GameplayTag Ability.BuildItem",NewTagName="Ability.BuildItem")
```

### Group B — Orphan flat tags (no namespace, cannot auto-redirect)

No canonical target; each needs a human decision (reclassify or delete if unused):

| Tag | Likely intent |
|---|---|
| `Prority` | Typo of "Priority"; no `Priority` tree exists — stray test tag. **Delete candidate.** |
| `AirDevastatingImpact` | Probably belongs under `Attack.*` or an ability namespace. |
| `Shoot` | Probably `Ability.*` or `GameplayEvent.*`. |
| `Gauntlet` | Ambiguous — weapon? automation/test map? |
| `LegArmor` | Probably `Inventory.Category.*` or an item-slot tag. |

Before deleting, check references via the editor **Reference Viewer** / "Find tag usages" — Blueprints are binary `.uasset`, so a config grep won't catch BP references.

### Group C — Verify, probably intentional (not auto-fixing)

- **`Anim.UI.*` (7 tags)** — `Anim.UI.MainMenu`, `Anim.UI.ClassSelection`, etc. A consistent group, likely a deliberate namespace distinct from `Animation.*` (menu animation states). Confirm with UI/anim owner before merging.
- **`Thud.Pole` (1)** — possibly a legit physics/SFX root.

---

## 8. How tags are created (conventions)

Three creation paths, chosen by who owns the tag.

### A. Designer/data tags → the `.ini` files

Authored through the UE **Gameplay Tag Manager**, which writes `Config/DefaultGameplayTags.ini`. Rule of thumb: anything Blueprints/data assets reference lives here. The high-churn save/quest tags (`GameState.Flags.*`) are deliberately siphoned into the second file `Config/Tags/GameplayFlags.ini` to keep the main dictionary diff-clean.

### B. Code-owned tags → native C++ macro

Tags that C++ needs by name are declared with the project macro `HexCreateNativeGameplayTag` (defined in `GameplayTagsMacros.h`):

```cpp
// CombatTags.h
struct CombatTagsStruct
{
    HexCreateNativeGameplayTag(EventAttackBlocked,      "GameplayEvent.Combat.AttackBlocked");
    HexCreateNativeGameplayTag(EventAttackBlockedHeavy, "GameplayEvent.Combat.AttackBlocked.Heavy");
    // ...
    static const CombatTagsStruct& Get() { static const CombatTagsStruct tags; return tags; }
};
#define GCombatTags CombatTagsStruct::Get()   // global accessor
```

Conventions baked into this:

- **Two arguments**: a C++ variable name + the dotted string. The variable name is the PascalCase concatenation of the path (`GameplayEvent.Combat.AttackBlocked.Heavy` → `EventAttackBlockedHeavy`).
- **Dotted hierarchy, PascalCase segments**, root = owning system (`GameplayEvent`, `Combat`, `Ability`, `DamageDef`, `AI`…).
- **One struct per domain**, each with a `Get()` singleton and a `G<Domain>Tags` `#define`. Five exist: `GCombatTags` (361 tags), `GCommonTags` (229), `GAbilityTags` (45), `GDamageTags` (44), `GAITags` (38).
- Registered automatically at module load via `FNativeGameplayTag`.

### C. UI tags → CommonUI macro

`HexUITags.cpp` uses Epic's `UE_DEFINE_GAMEPLAY_TAG` (51 tags) because CommonUI's layer/action/extension system expects that form.

### Cross-cutting rules

- **Renames never delete — they redirect.** 186 `+GameplayTagRedirects` entries preserve old asset/Blueprint/**save-file** references. (This is why the typo cleanup in §7 needs redirects, not just edits.)
- **`FastReplication=True`** means tags replicate as indices, so the tag list must be **identical client/server** — native tag modules must load symmetrically.
- **Native C++ and `.ini` mirror the same strings on purpose**: the `.ini` exposes the full tree to designers; the native handle gives C++ compile-time safety (`GCombatTags.EventAttackBlocked` instead of a fragile string lookup).

---

## 9. How tags are used (consumption)

C++ never types the string; it goes through the accessor, e.g. `GCombatTags.EventAttackBlocked`. Accessor usage across `Source/`: `GCombatTags` 626 refs / 137 files, `GCommonTags` 434 / 133, `GDamageTags` 134 / 18, `GAbilityTags` 88 / 45, `GAITags` 75 / 32.

### GAS consumption idioms (call-site counts)

| Idiom | Count | What it does |
|---|---|---|
| `HasMatchingGameplayTag` / `HasAnyMatching…` | 247 / 21 | **State checks** — "is this character blocking / immune / knocked down?" The dominant pattern. |
| `HandleGameplayEvent` | 109 | Routes a gameplay event into the ability system. |
| `RequestGameplayTag` | 66 | Dynamic string→tag lookup (data-driven cases that can't use the native handle). |
| `AddLooseGameplayTag` / `RemoveLooseGameplayTag` | 35 / 31 | Transient runtime state not backed by a GameplayEffect. |
| `SendGameplayEventToActor` | 34 | Fires a `GameplayEvent.*` tag to trigger reactions on another actor. |
| `ExecuteGameplayCue` | 29 | Plays a `GameplayCue.*` (VFX/SFX/camera shake) — cosmetic only. |

### Three roles tags play

1. **Event bus** — `GameplayEvent.Combat.*` tags fired by one system, awaited by abilities.
   Example: `GA_BlockAbility.cpp:50` waits on `GCombatTags.EventAttackBlocked` via `WaitGameplayEvent`; `AnathemaHitReactAbility.cpp:278` matches the same tag with `EventTag.MatchesTag(...)` to select a hit reaction. The block ability raises the event; the hit-react ability consumes it.
2. **State machine** — `Combat.State.*`, `Character.State.*`, `State.Umbral`, etc. set/queried via loose tags and effect-granted tags. Hierarchy matching matters: `MatchesTag` means `Combat.State.Immune.HealthDamage` also satisfies a query for `Combat.State.Immune`.
3. **Persistence / world state** — `GameState.Flags.*` are gameplay tags used as save-game booleans (miniboss-dead, quest-step-done, door-open). The quest system writes them (`QuestTask` → `SetCustomOnCompleteGameStateFlag`) and checks them the same way (`HasMatchingGameplayTag`) to gate doors/quests. This is why the redirects are save-critical.

---

## 10. Worked examples by system area

Real call sites (file:line) showing exactly how a tag is used in each part of the game.

### 10.1 Player / character state

State is a tag the character carries; everything else queries it. The same `Combat.State.*` tag is read by gameplay, AI, and UI — that single source of truth is the whole point.

```cpp
// LOTF2Character.cpp:1128 — the canonical "am I knocked down?" query
bool ALOTF2Character::IsKnockedDown() const
{
    return HasMatchingGameplayTag(GCombatTags.StateCombatKnockdown);
}

// AIActionApproachPlayer.cpp:42 — AI won't path to a knocked-down target
if (Character->HasMatchingGameplayTag(GCombatTags.StateCombatKnockdown)) { /* skip approach */ }

// AimTargetWidget.cpp:76 — the HUD reads the SAME state to update the reticle
if (abilitySystemComponent->HasMatchingGameplayTag(GCombatTags.StateCombatStaggered)) { /* show staggered reticle */ }
```

**Areas touched by state tags:** gameplay abilities, AI (Behavior Tree / Utility actions), HUD widgets, hit detection. One tag, many readers.

### 10.2 Buffs & status effects (granted tags)

Buffs are not booleans — they are **GameplayEffects that grant tags**. Equip an item → the effect grants its tag container; the buff "ends" by removing effects that granted a tag.

```cpp
// EquipementData.cpp:210 — equipping grants the item's tags as a buff
GETagsSpecHandle.Data->DynamicGrantedTags = OnEquippedGameplayAbilitiesConfig.GetGameplayTags();

// LOTF2Character.cpp:998 — a character's innate tags are granted the same way at spawn
initialTagsSpecHandle.Data->DynamicGrantedTags = pCharacterData->GetGameplayAbilitiesConfig().GetGameplayTags();

// HealthComponent.cpp:382 — "revive" = strip every effect that granted the Dead tag
abilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GCombatTags.CombatStateDead));

// ANS_UnholsterLanternCosmetic.cpp:19 — transient (loose) tag, not effect-backed
pASC->AddLooseGameplayTag(GCombatTags.CombatStateCosmeticLantern);
```

**Two flavors:** *effect-granted* tags (`DynamicGrantedTags`, removed via `RemoveActiveEffectsWithGrantedTags`) for real buffs/debuffs (`StatusEffect.Positive/Negative`, equipment bonuses); *loose* tags (`AddLooseGameplayTag`) for momentary, non-persistent state.

### 10.3 Damage & detection

Tags are the **keys** that carry damage numbers and gate hit detection. Weapon damage is fully data-driven by `DamageDef`/`GameplayEffect.Input.*` tags via `SetByCaller` magnitudes.

```cpp
// WeaponData.cpp:36 — per damage-type magnitude is keyed by a damage tag
weaponStatsEffecthandle.Data->SetSetByCallerMagnitude(
    GDamageTags.GameplayEffectInputCombatAttribute##DamageType##Damage,   // e.g. ...Physical/Fire/Holy
    GET_STAT_AT_LEVEL(pWeaponStats, Damage##DamageType, level));

// HitBoxesActor.cpp:183 — detection gated by state: a disabled dealer can't grab
bCanBeGrabbed &= (gameplayTagAssetInterface->HasMatchingGameplayTag(
    GCombatTags.CombatStateDamageDealersDisabled) == false);
```

The flow: weapon stats → `GameplayEffect.Input.*` tags carry the numbers → execution applies them → `WeakTo`/`ResistantTo` tags on the target modulate the result → on hit, a `GameplayEvent.Combat.DamageDealt.*` tag is broadcast (see §9, the block/hit-react event bus).

### 10.4 UI

UI tags are **routing addresses**, not state. `UI.Layer.*` says *which stack* a widget goes on; `UI.Action.*`/`UI.Extension.*` name input actions and injection points (CommonUI).

```cpp
// AnchorMenuWidget.cpp:83 — push a widget onto a named layer
UHexUIExtensions::PushAsyncContentToLayer_ForPlayer(localPlayer, TAG_UI_LAYER_INGAMEMENU, Online_Lobby_Widget);

// HexActivatableWidget.cpp:245 — clear a layer by its tag
UHexUIExtensions::ResetLayer(localPlayer, TAG_UI_LAYER_INGAMEMENU);
```

### 10.5 Animation

Animation uses tags **two ways**: as keys to *look up* the right montage/sequence, and (via anim notifies) as a way for animation to *push state back* onto the character.

```cpp
// BTTask_HexDodge.cpp:24 — directional dodge selects its montage by tag
pMontageToPlay = aiCharacter.GetCharacterConfigComponent()->GetAnimMontageByTag(GCommonTags.AnimationCoreStepForward);
// ...StepBack / StepLeft / StepRight for the other directions

// HexSoulFlayCharacterSoul.cpp:431 — look up a sequence by locomotion tag
sequenceToPlay = character->GetCharacterConfigComponent()->GetAnimationByTag(GCommonTags.AnimationLocomotionFalling);

// ANS_UnholsterLanternCosmetic.cpp:23 — an anim notify writes a gameplay state tag mid-montage
pASC->AddLooseGameplayTag(GCombatTags.CombatStateBlockLanternCamera);
```

This is why `Animation.*` is the largest category (1,653 tags): every montage/sequence is addressable by tag through `GetAnimMontageByTag` / `GetAnimationByTag` on the character config component.

### Summary — who uses tags for what

| Area | Tag families | Primary idiom |
|---|---|---|
| Player/character state | `Combat.State.*`, `Character.State.*` | `HasMatchingGameplayTag` (query) |
| Buffs / status effects | `StatusEffect.*`, equipment tags, `Combat.State.Dead` | `DynamicGrantedTags` / `RemoveActiveEffectsWithGrantedTags` |
| Transient state | combat/cosmetic states | `AddLooseGameplayTag` / `RemoveLooseGameplayTag` |
| Damage & detection | `DamageDef.*`, `GameplayEffect.Input.*`, `WeakTo`/`ResistantTo` | `SetSetByCallerMagnitude` + state gates |
| Combat events | `GameplayEvent.Combat.*` | `SendGameplayEventToActor` / `WaitGameplayEvent` |
| Cosmetic feedback | `GameplayCue.*` | `ExecuteGameplayCue` |
| UI | `UI.Layer/Action/Extension.*` | `PushContentToLayer` / `ResetLayer` |
| Animation | `Animation.*`, `AnimationMoveset.*` | `GetAnimMontageByTag` / `GetAnimationByTag` |
| Persistence | `GameState.Flags.*` | written by quests, read via `HasMatchingGameplayTag` |

---

## 11. Reproduction kit

Everything below is **verbatim** from the project — enough to stand up the same *code-side* tag infrastructure in a new project. The data-side (binary `.uasset`) pieces can only be listed as a checklist (§11.5); they cannot be embedded in markdown.

### 11.1 Build dependencies

In `<Module>.Build.cs`, add to `PublicDependencyModuleNames` (or Private):

```csharp
"GameplayTasks",
"GameplayTags",
"GameplayAbilities",
"CommonUI",       // only if you replicate the UI.Layer/Action/Extension tags
```

> `FNativeGameplayTag` registers each native tag under the declaring module's `UE_PLUGIN_NAME` / `UE_MODULE_NAME`. Declare native tags in the module that should own them; declaring the same string in two modules registers two distinct native entries.

### 11.2 Config — `Config/DefaultGameplayTags.ini` header

```ini
[/Script/GameplayTags.GameplayTagsSettings]
ImportTagsFromConfig=True
WarnOnInvalidTags=True
ClearInvalidTags=False
AllowEditorTagUnloading=True
AllowGameTagUnloading=False
FastReplication=True
InvalidTagCharacters=
```

Designer/data tags are appended below this as `GameplayTagList=(Tag="...",DevComment="...")`. Renames are recorded as `+GameplayTagRedirects=(OldTagName="...",NewTagName="...")`. Split high-churn save/quest tags into a second file (`Config/Tags/GameplayFlags.ini`) with the same `[/Script/GameplayTags.GameplayTagsList]` section header.

> **`FastReplication=True` requires identical tag lists on client and server.** All native tag modules must load symmetrically, or replication desyncs.

### 11.3 `GameplayTagsMacros.h` (verbatim)

```cpp
// Copyright (C), HEXWORKS by CIGames, 2020-2022.
#pragma once

#include "GameplayTags.h"
#include "NativeGameplayTags.h"

//-------------------------------------------------------
#define EXPAND(x) x
#define CAT( A, B ) A ## B
#define SELECT( NAME, NUM ) CAT( NAME ## _, NUM )
#define GET_COUNT( _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, COUNT, ... ) COUNT
#define VA_SIZE( ... ) EXPAND(GET_COUNT( __VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 ))
#define VA_SELECT( NAME,macro, ... ) EXPAND(SELECT( NAME, VA_SIZE(__VA_ARGS__) )(macro, __VA_ARGS__))

#define FOR_EACH( macro, ... ) VA_SELECT( FOR_EACHArg, macro,  __VA_ARGS__ )
#define FOR_EACHArg_1(macro, _obj1) macro(_obj1)
#define FOR_EACHArg_2(macro, _obj1, _obj2) FOR_EACHArg_1(macro, _obj1) macro(_obj2)
#define FOR_EACHArg_3(macro, _obj1, _obj2, _obj3) FOR_EACHArg_2(macro, _obj1, _obj2) macro(_obj3)
#define FOR_EACHArg_4(macro, _obj1, _obj2, _obj3, _obj4) FOR_EACHArg_3(macro, _obj1, _obj2, _obj3) macro(_obj4)
#define FOR_EACHArg_5(macro, _obj1, _obj2, _obj3, _obj4, _obj5) FOR_EACHArg_4(macro, _obj1, _obj2, _obj3, _obj4) macro(_obj5)
#define FOR_EACHArg_6(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6) FOR_EACHArg_5(macro, _obj1, _obj2, _obj3, _obj4, _obj5) macro(_obj6)
#define FOR_EACHArg_7(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7) FOR_EACHArg_6(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6) macro(_obj7)
#define FOR_EACHArg_8(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7, _obj8) FOR_EACHArg_7(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7) macro(_obj8)
#define FOR_EACHArg_9(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7, _obj8, _obj9) FOR_EACHArg_8(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7, _obj8) macro(_obj9)
#define FOR_EACHArg_10(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7, _obj8, _obj9, _obj10) FOR_EACHArg_9(macro, _obj1, _obj2, _obj3, _obj4, _obj5, _obj6, _obj7, _obj8, _obj9) macro(_obj10)

//-------------------------------------------------------
// Native gameplay tag as a struct member
#define HexCreateNativeGameplayTag(TagName, TagString)  \
const FNativeGameplayTag TagName = { UE_PLUGIN_NAME, UE_MODULE_NAME, FName(TagString), TEXT(""), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD };

#define HexCreateNativeGameplayTagWithComment(TagName, TagString, TagDevComment)  \
const FNativeGameplayTag TagName = { UE_PLUGIN_NAME, UE_MODULE_NAME, FName(TagString), TEXT(TagDevComment), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD };

#define HexCreateGameplayTagArray(TagArrayName, ... ) \
const TArray<FGameplayTag> TagArrayName = { __VA_ARGS__ };

#define HexCreateGameplayTagContainer(TagContainerName, ... ) \
const FGameplayTagContainer TagContainerName = FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>({__VA_ARGS__}));
```

### 11.4 The per-domain tag struct pattern

Each domain header (`CombatTags.h`, `CommonTags.h`, …) follows this exact shape:

```cpp
#pragma once
#include "GameplayTagsMacros.h"

struct CombatTagsStruct
{
    HexCreateNativeGameplayTag(EventAttackBlocked,      "GameplayEvent.Combat.AttackBlocked");
    HexCreateNativeGameplayTag(EventAttackBlockedHeavy, "GameplayEvent.Combat.AttackBlocked.Heavy");
    // ... one line per tag, grouped by // section comments ...

    static const CombatTagsStruct& Get()
    {
        static const CombatTagsStruct tags;   // lazy-init singleton
        return tags;
    }
};

#define GCombatTags CombatTagsStruct::Get()    // call sites use GCombatTags.EventAttackBlocked
```

**Convention:** variable name = PascalCase concatenation of the dotted path. One struct per domain, one `G<Domain>Tags` accessor each.

### 11.5 Damage ↔ attribute bridge — `LOTF2AttributeSetMacros.h` (verbatim)

This is the glue that makes `GameplayEffect.Input.*` / `BattleEffect.Attribute.*` tags resolve to actual attributes. The `ATTRIBUTE_ACCESSORS` macro auto-creates a matching native tag (`BattleEffectInput<Class><Property>`) alongside the attribute's getters/setters:

```cpp
#pragma once
#include "AttributeSet.h"
#include "NativeGameplayTags.h"
#include "AbilitySystemGlobals.h"

#define HEX_CREATE_BATTLE_EFFECT_TAG(ClassName, PropertyName)  "BattleEffect.Attribute." #ClassName "." #PropertyName

#define HEX_CREATE_BATTLE_EFFECT_INPUT_ATTRIBUTE_TAG(ClassName, PropertyName)  \
static inline FNativeGameplayTag BattleEffectInput##ClassName##PropertyName = {UE_PLUGIN_NAME, UE_MODULE_NAME, HEX_CREATE_BATTLE_EFFECT_TAG(ClassName, PropertyName ), TEXT(""), ENativeGameplayTagToken::PRIVATE_USE_MACRO_INSTEAD};

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName) \
    HEX_CREATE_BATTLE_EFFECT_INPUT_ATTRIBUTE_TAG(ClassName, PropertyName)

#define LOTF2_GAMEPLAYATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue) \
{ \
    static FProperty* ThisProperty = FindFieldChecked<FProperty>(ClassName::StaticClass(), GET_MEMBER_NAME_CHECKED(ClassName, PropertyName)); \
    UAbilitySystemComponent* abilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwningActor(), true);\
    if (ensureAlwaysMsgf(abilitySystem, TEXT("Actor %s does not seem to have AbilitySystem"), *GetOwningActor()->GetName())) \
    {\
        abilitySystem->SetBaseAttributeValueFromReplication(FGameplayAttribute(ThisProperty), PropertyName, OldValue); \
    }\
}
```

### 11.6 Minimal end-to-end wiring (the path a value travels)

```
Weapon stat (DataAsset)
  → SetSetByCallerMagnitude(GDamageTags.GameplayEffectInput...<Type>Damage, value)   // WeaponData.cpp
  → GameplayEffect spec carries the value keyed by the tag
  → AttributeSet (ATTRIBUTE_ACCESSORS) receives it via the matching BattleEffect.Attribute.* tag
  → execution modulates by target's WeakTo / ResistantTo tags
  → on hit: SendGameplayEventToActor(GameplayEvent.Combat.DamageDealt.*)
  → passive ability WaitGameplayEvent / MatchesTag reacts (hit react)   // AnathemaHitReactAbility.cpp
  → ExecuteGameplayCue(GameplayCue.*) plays VFX/SFX
```

### 11.7 Data-side checklist (NOT capturable in markdown — binary `.uasset`)

To get a *working* clone you must also author/migrate these in-editor:

- [ ] **AttributeSet classes** (the `UAttributeSet` subclasses using `ATTRIBUTE_ACCESSORS`) — code, but you need the actual attribute list
- [ ] **GameplayEffect assets** — `DynamicGrantedTags`, `SetByCaller` magnitude tags, granted/ongoing/removal tag containers
- [ ] **GameplayAbility assets/classes** — `AbilityTags`, `ActivationOwnedTags`, `ActivationBlockedTags`, event triggers
- [ ] **GameplayCue notifies/assets** — one per `GameplayCue.*` branch
- [ ] **CharacterConfigComponent / CharacterAnimationsConfig** data — the tag→montage/sequence map behind `GetAnimMontageByTag` / `GetAnimationByTag`
- [ ] **Montage/moveset DataTables** keyed by `Animation.*` / `AnimationMoveset.*`
- [ ] **GameState flag / save subsystem** — persists `GameState.Flags.*` and the quest write path (`SetCustomOnCompleteGameStateFlag`)
- [ ] **CommonUI layer registry + HexUIExtensions** — backs `PushContentToLayer` / `ResetLayer` for `UI.Layer.*`
- [ ] **ASC setup** on the character/pawn (grant initial tags via `DynamicGrantedTags`)
- [ ] The actual **tag entries** themselves — copy the `.ini` files, or regenerate them

> Reproducing §11.1–11.6 gives you the *engine* of the tag system. §11.7 is the *content* that gives the tags meaning — budget most of the effort there.
