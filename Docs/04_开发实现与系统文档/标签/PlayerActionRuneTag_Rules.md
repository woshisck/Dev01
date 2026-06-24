# Player Action / Buff-Rune Tag Rules

This document records the current tag rules after removing LightAttack/HeavyAttack and merging Rune/Card tag semantics into Buff.

## 1. Player Action Model

The player has four action channels:

| Action | Formal state tag | Notes |
| --- | --- | --- |
| Attack | `Character.State.Skill.Attack` | Normal attack. No LightAttack/HeavyAttack split. |
| Skill | `Character.State.Skill.Active` | Player-selected active skill. |
| WeaponSkill | `Character.State.Skill.WeaponSkill` | Skill provided by equipped weapon. Old HeavyAttack intent maps here only for compatibility. |
| Dash/Dodge | `Character.State.Movement.Dash` | Generic dash by default; weapons may override behavior without renaming the action channel. |

Rules:

- Do not add new `LightAttack`, `HeavyAttack`, `LightAtk`, or `HeavyAtk` tags.
- Legacy `PlayerState.AbilityCast.LightAtk*` maps to Attack only for old assets.
- Legacy `PlayerState.AbilityCast.HeavyAtk*` maps to WeaponSkill only for old assets.
- Runtime state, HUD, StateConflict, debug, AI, and new assets should read/write `Character.State.*` for action state.
- Do not create a formal top-level `Combat.*` root for these states. Combat events/cues keep their existing event/cue roots, such as `GameplayEvent.Combat.*` and `GameplayCue.Combat.*`.

## 2. Buff / Rune / Card Model

Rune and Card are merged into the Buff tag vocabulary.

Formal new tags use flat `Buff.*`:

| Intent | Formal tag example |
| --- | --- |
| Fire/burn setup or runtime burning state | `Buff.Fire` |
| Poison card/effect/runtime state | `Buff.Poison` |
| Moonlight link card/effect context | `Buff.Moonlight` |
| WeaponSkill detonation card identity | `Buff.WeaponSkillFinisher` |
| Detonation/consume effect | `Buff.Detonate` |
| Recovery cancel reward marker | `Buff.RecoveryCancelBonus` |

Do not use these for new content:

- `Rune.ID.*`
- `Rune.Effect.*`
- `Rune.Binding.*`
- `Rune.Trigger.*`
- `Rune.FlowRole.*`
- `Rune.Activation.*`
- `Card.ID.*`
- `Card.Effect.*`
- `Buff.Status.*`
- `Buff.ID.*`
- `Buff.Keyword.*`
- `Buff.Binding.*`
- `Action.Rune.*`
- `Event.Rune.*`
- `GameplayCue.Rune.*`

Those concepts now belong in DA/table fields:

| Concept | Where it belongs |
| --- | --- |
| Concrete card/rune identity | `RuneDataAsset` / combat card config field, usually with a `Buff.*` semantic tag when queryable at runtime |
| Action slot | `ECombatDeckActionSlot` or equivalent DA field |
| Trigger timing | `ECombatCardTriggerTiming` or equivalent DA field |
| Flow role | `ECombatDeckFlowRole` enum |
| Passive/active/triggered activation | DA enum/field |
| Rarity/type/element | DA fields, not gameplay tags unless queried by runtime gameplay logic |
| Numeric value and stack rule | DA table/config/GE magnitude |

Internal buff/card/rune gameplay events use `Buff.Event.*`.

Examples:

| Old event tag | Formal event tag |
| --- | --- |
| `Action.Rune.SlashWaveHit` | `Buff.Event.SlashWaveHit` |
| `Action.Rune.MoonlightBurnHit` | `Buff.Event.Moonlight.BurnHit` |
| `Action.Rune.MoonlightPoisonHit` | `Buff.Event.Moonlight.PoisonHit` |
| `Event.Rune.KnockbackApplied` | `Buff.Event.KnockbackApplied` |

Buff/card/rune presentation uses `GameplayCue.Buff.*`, for example `GameplayCue.Buff.Fire`, `GameplayCue.Buff.Poison.Vfx`, and `GameplayCue.Buff.MoonlightSlash.Hit`.
Old `GameplayCue.Rune.FinisherCharge` also migrates to `GameplayCue.Buff.FinisherCharge`; it remains a deprecated visual compatibility cue and does not restore the old QTE Finisher runtime.

## 3. Deck Loop

The combat deck is a build loop, not a light/heavy combo tree.

Design flow:

| Flow role | Meaning | Stored as |
| --- | --- | --- |
| Starter | Applies or opens a buff/link context | DA enum field |
| Catalyst | Strengthens, extends, converts, or advances the context | DA enum field |
| Finisher | Consumes/explodes/finalizes the context. Not deprecated QTE Finisher. | DA enum field |

Examples:

| Intent | Tags + fields |
| --- | --- |
| Attack starts fire setup | `CardEffectTags = Buff.Fire`, `RequiredActionSlot = Attack`, `FlowRole = Starter` |
| Skill strengthens fire setup | `CardEffectTags = Buff.Fire`, `RequiredActionSlot = Skill`, `FlowRole = Catalyst` |
| WeaponSkill detonates setup | `CardIdTag = Buff.WeaponSkillFinisher`, `CardEffectTags = Buff.Detonate`, `RequiredActionSlot = WeaponSkill`, `FlowRole = Finisher` |
| Always-on stat upgrade | DA passive fields + concrete runtime/query `Buff.*` only if needed |

## 4. Cancel / Recovery Rules

Post-attack cancel is a window rule, not a light/heavy action identity.

Use:

- `Character.State.Window.PostAttackRecovery` for the recovery window.
- `Character.State.Window.CanCombo` only as a generic action-interruption/cancel window.
- `Buff.RecoveryCancelBonus` for the temporary reward marker after a successful recovery cancel.

Weapon switching can use `Character.State.Equipment.SwitchWeapon`. If switching during recovery resets Skill/WeaponSkill cooldowns, the reset logic should key from the recovery window and reward marker, not from old LightAttack/HeavyAttack tags.

## 5. Deprecated Compatibility

The following families are migration-only:

- `PlayerState.AbilityCast.LightAtk*`
- `PlayerState.AbilityCast.HeavyAtk*`
- `Card.ID.*`
- `Card.Effect.*`
- `Rune.ID.*`
- `Rune.Effect.*`
- `Rune.Card.*`
- `Buff.Status.*`
- `Combo.CombatDeck.*`
- `Action.Rune.*`
- `Event.Rune.*`
- `GameplayCue.Rune.*`

Keep them loadable until assets are migrated and resaved. Do not use them as new authoring entries.

## 6. Config Ownership

`Config/DefaultGameplayTags.ini` should contain global GameplayTags settings and redirects only.

| Tag family | Owner file |
| --- | --- |
| `Character.State.*`, legacy `State.Musket.*` | `Config/Tags/CharacterTag.ini` |
| `Buff.*`, legacy `Buff.Status.*`, legacy sustained `Character.State.*` status aliases | `Config/Tags/BuffTag.ini` |
| Legacy `Rune.*`, legacy `Card.*` | `Config/Tags/RuneTag.ini` |
| `Action.*`, legacy `PlayerState.*` | `Config/Tags/PlayerGameplayTag.ini` |
| `GameplayCue.*` | `Config/Tags/GameplayCueTag.ini` |
| `GameplayEvent.*` | `Config/Tags/GameplayEventTag.ini` |
| `Tutorial.Hint.*` | `Config/Tags/TutorialTag.ini` |
| `Currency.*` | `Config/Tags/CurrencyTag.ini` |
| `Data.*` SetByCaller slots | `Config/Tags/Data.ini` |
| `Loot.*` | `Config/Tags/LootTag.ini` |

When a root is unclear, choose the system that owns the runtime behavior, not the system that displays it. HUD may show Buff icons, but Buff tags are still authored under `Buff.*` and read by UI as presentation data.
