# Loot Type Choice Popup — Pending Plan

| Item | Content |
| --- | --- |
| Scope | Reward pickup flow (`ARewardPickup::TryPickup` → `AYogHUD::QueueLootSelection` → `ULootSelectionWidget`) |
| Audience | Programmers (implementation), designers (configuration), UI (WBP) |
| Related docs | [LootSelection_Technical.md](../04_开发实现与系统文档/系统/UI/LootSelection_Technical.md), [EnemyLootableCorpse_TODO.md](EnemyLootableCorpse_TODO.md) |
| Last updated | 2026-10-27 |
| Status | **C++ done, not compile-verified** — editor asset setup (section 5) still pending |

## Requirement

After the player holds E on a reward pickup, show a popup that lets them choose which **type** of loot
they get (e.g. Rune / Gold / Material), then run the matching grant flow.

## Current Behavior

```
Hold E → ARewardPickup::TryPickup()
   ├─ all options are Gold / Material / WeaponSkill → GrantImmediateLoot() → destroyed
   └─ any Rune option → AYogHUD::QueueLootSelection() → ULootSelectionWidget (3 rune cards)
```

The player never picks the type: it comes from `AssignedLoot` / `GenerateIndependentLootOptions()`,
and the latter only ever produces runes.

| Location | Notes |
| --- | --- |
| `RewardPickup.cpp` `TryPickup()` | Interact entry; decides immediate grant vs. rune cards |
| `RewardPickup.cpp` `ShouldGrantLootImmediatelyForOptions()` | Grants immediately when no Rune option exists |
| `RewardPickup.cpp` `GrantImmediateLoot()` / `ResetForSkip()` / `ConsumeAndDestroy()` | Grant / reset on skip / consume |
| `YogGameMode.cpp` `GenerateIndependentLootOptions()` | Generates the rune 3-choice batch |
| `YogHUD.cpp` `QueueLootSelection()` | Rune card popup queue |
| `LootSelectionWidget.cpp` `SkipSelection()` | Reference for closing + resetting the pickup |
| `ShopActor.cpp` `TryInteract()` / `OnPlayerEndOverlap()` | Reference for `EnsureWidget` → `Setup` → `PushScreen`, and `PopScreen` on leaving range |
| `YogUIRegistry.h` `EYogUIScreenId` | Screen ID enum (serialized as uint8 — append only) |

## Target Flow

```
Hold E → ARewardPickup::TryPickup()
   └─ bOfferLootTypeChoice → UIManager->PushScreen(LootTypeChoice)
            ┌──────────────────────────────────┐
            │  Choose your reward              │
            │  [ Rune ]  [ Gold x50 ]  [ Mat ] │
            └──────────────────────────────────┘
        ├─ Rune     → GenerateIndependentLootOptions() → existing QueueLootSelection (3 cards)
        ├─ Gold/Mat → GrantImmediateLoot()             → ConsumeAndDestroy()
        └─ Back     → ResetForSkip() (pickup stays, can re-interact)
```

## Implementation Steps

### 1. Choice struct (`Public/GameModes/LevelFlowTypes.h`)

- [x] Add `USTRUCT(BlueprintType) FLootTypeChoice`

| Field | Type | Notes |
| --- | --- | --- |
| `LootType` | `ELootType` | Choice type |
| `Amount` | `int32` | Gold / Material amount |
| `MetaCurrencyTag` | `FGameplayTag` | Currency tag for Material |
| `DisplayName` | `FText` | Button label |
| `Icon` | `TObjectPtr<UTexture2D>` | Button icon |

### 2. Screen ID (`Public/UI/YogUIRegistry.h`)

- [x] Append `LootTypeChoice` to the end of `EYogUIScreenId` (after `BubbleMessage`)
      — the enum is serialized as uint8 inside the registry asset, **never insert in the middle**

### 3. Pickup (`Public/Map/RewardPickup.h` / `Private/Map/RewardPickup.cpp`)

- [x] New config (same pattern as `bUseFixedLootOptions` / `FixedLootOptions`):
  - `UPROPERTY(EditAnywhere) bool bOfferLootTypeChoice = false;`
  - `UPROPERTY(EditAnywhere, meta = (EditCondition = "bOfferLootTypeChoice")) TArray<FLootTypeChoice> LootTypeChoices;`
- [x] At the top of `TryPickup()`: when the flag is on and `LootTypeChoices` is not empty,
      `EnsureWidget` → `Setup` → `PushScreen(LootTypeChoice)`, then return
- [x] Add `ResolveLootTypeChoice(int32 Index, APlayerCharacterBase* Player)`:

| Choice | Handling |
| --- | --- |
| `Rune` | `GM->GenerateIndependentLootOptions()` → `HUD->QueueLootSelection(Options, this)` |
| `Gold` / `Material` / `WeaponSkill` | Build a single `FLootOption` → `GrantImmediateLoot()` → `ConsumeAndDestroy()` on success |

- [x] `OnPlayerLeaveRange()`: `PopScreen(LootTypeChoice)` if the popup is open (same as `AShopActor::OnPlayerEndOverlap`)

### 4. Popup widget (new `Public/UI/LootTypeChoiceWidget.h` / `Private/UI/LootTypeChoiceWidget.cpp`)

- [x] Derive from `UCommonActivatableWidget`
- [x] `Setup(ARewardPickup* Pickup, APlayerCharacterBase* Player)`; hold the pickup as `TWeakObjectPtr`
- [x] `BindWidgetOptional` container; build one button per entry in `LootTypeChoices`
- [x] `GetDesiredFocusTarget()` returns the first button (gamepad focus)
- [x] Button click → `Pickup->ResolveLootTypeChoice(Index, Player)` → close
- [x] Back / Esc → `Pickup->ResetForSkip(Player)` → close
- [x] Close via `UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::LootTypeChoice)`,
      falling back to `DeactivateWidget()` (same as `ULootSelectionWidget::SkipSelection`)

### 5. Content setup (editor)

- [ ] Create `WBP_LootTypeChoice` with parent `ULootTypeChoiceWidget`
- [ ] Add a row to the UI Registry DA:

| Field | Value |
| --- | --- |
| ScreenId | `LootTypeChoice` |
| WidgetClass | `WBP_LootTypeChoice` |
| Layer | Modal |
| `bPauseGame` | true |
| `bDisablePawnInput` | true |
| `bShowMouseCursor` | true |

- [ ] Enable `bOfferLootTypeChoice` and fill in `LootTypeChoices` on the reward pickup BPs / instances that need it

### 6. Docs

- [x] Add the type-choice step to `LootSelection_Technical.md`

## Caveats

| Item | Notes |
| --- | --- |
| WeaponSkill | Only works on the immediate-grant path; `SelectLoot` and the rune card widget don't support it, so never put it in the 3-card list |
| Lootable corpses | In `EnemyLootableCorpse_TODO.md` the corpse calls `TryPickup` on a spawned pickup; if `RewardPickupClass` has this flag on, kill drops will show this popup too — decide whether they should skip it |
| Off by default | The flag defaults to false, so existing room rewards and kill drops are unchanged |

## Open Questions

- [ ] Which types the popup offers (Rune / Gold / Material / other)
- [ ] Enabled for every room reward, or only per pickup by designers (default: per pickup)
