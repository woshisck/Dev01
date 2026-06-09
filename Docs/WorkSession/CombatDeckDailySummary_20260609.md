# Combat Deck / ComboGraph Work Summary - 2026-06-09

## Scope

Branch: `ComboGraphRemoveDash`

This note summarizes the current branch work that is directly related to the combat input, ComboGraph, special attack, weapon skill, dash, and combat deck refactor. It separates code already landed from asset/design work that still needs follow-up in Unreal Editor.

## Build Verification

Command:

```powershell
& 'D:\UE\UE_5.4\Engine\Build\BatchFiles\Build.bat' DevKitEditor Win64 Development -Project='D:\Self\GItGame\Dev01\DevKit.uproject' -WaitMutex
```

Result:

- Build passed with exit code `0`.
- UnrealHeaderTool generated reflection code successfully.
- `DevKitEditor` and `DevKitEditor` modules linked successfully.
- Remaining output is warnings only:
  - Visual Studio 2022 compiler is not the preferred UE version.
  - Existing deprecated UE API warnings around `bIsFocusable` / `IsFocusable`.
  - Existing deprecated GameplayEffect tag container warnings.

## Implemented Code Changes

### Input Naming And Routing

- Player input now uses the new naming layer:
  - `NormalAttack`
  - `SpecialAttack`
  - `WeaponSkill`
- Deprecated input fields are kept as compatibility fallbacks:
  - `Input_LightAttack`
  - `Input_HeavyAttack`
  - `Input_Dash`
- `YogPlayerControllerBase` routes:
  - `NormalAttack` to normal weapon/unarmed combo attack.
  - `SpecialAttack` to `PlayerSpecialAttackComponent` when equipped, otherwise legacy heavy fallback.
  - `WeaponSkill` first to weapon ComboGraph `WeaponSkill` node, then to dash fallback.
- `BufferComponent` records `NormalAttack`, `SpecialAttack`, and `WeaponSkill` instead of the old light/heavy/dash names.

### ComboGraph Changes

- `EYogComboGraphInputAction` keeps old numeric compatibility but exposes new display intent:
  - `Light` -> `NormalAttack`
  - `Heavy` -> `SpecialAttack`
  - `WeaponSkill`
  - legacy `Dash` is hidden and normalized to `WeaponSkill`
- ComboGraph lookup now normalizes legacy `Dash` nodes/edges to `WeaponSkill`.
- Invalid graph input values no longer accidentally match normal attack.
- ComboGraph exposes its supported node class set for editor/tooling.
- Graph validation catches duplicate child inputs under the same parent.

### Weapon And Special Attack Graph Separation

- `ComboRuntimeComponent` now tracks:
  - weapon combo graph
  - special attack combo graph
  - currently active graph
- Weapon graph is used by normal attack and weapon skill.
- Special attack graph is started only from the special attack input path.
- `SpecialAttackDataAsset` can carry its own `ComboGraph`.
- `PlayerCharacterBase` loads both the equipped weapon graph and the equipped special attack graph from `WeaponDefinition`.
- Resetting to unarmed clears the equipped special attack graph and restores the default unarmed combo graph.

### Combat Deck Action Slot / Flow Role

Combat card resolution now carries a richer action context:

- `ECombatDeckActionSlot`
  - `Attack`
  - `Skill`
  - `WeaponSkill`
  - `Dash`
  - `Any`
- `ECombatDeckFlowRole`
  - `Starter`
  - `Catalyst`
  - `Finisher`
  - `Any`
- `ECombatCardReleaseMode`
  - `Normal`
  - `Finisher`

Rune card configs now support:

- `RequiredActionSlot`
- `RequiredFlowRole`

Link conditions also support:

- `RequiredActionSlot`
- `RequiredFlowRole`

BuffFlow combat-card context now exposes:

- action slot
- flow role
- combo index
- combo node id
- combo tags
- applied multiplier
- finisher flags

### Attack Deck Versus Single Action Slots

The deck is now split at runtime:

- `Attack` cards remain in the ordered attack sequence.
- `Skill`, `WeaponSkill`, and `Dash` each have one single card slot.
- Single-slot cards do not enter the attack sequence.
- Single-slot cards do not advance `CurrentIndex`.
- Single-slot cards do not trigger attack deck shuffle.
- Single-slot cards can trigger repeatedly on each matching action.
- `GetDeckSourceAssets()` includes attack sequence cards and single-slot cards, so save/restore keeps the slot cards.
- `GetFullDeckSnapshot()` includes attack sequence plus single-slot cards for full inspection.
- `GetActionSlotCardSnapshot()` exposes the currently equipped single-slot card for UI/debugging.

Current gameplay interpretation:

- Attack deck = ordered play sequence.
- Skill slot = one player-selected skill card.
- WeaponSkill slot = one weapon-provided finisher/detonate card.
- Dash slot = one mobility/cancel card.

### Active Skill Integration

- `ActiveSkillDataAsset` can opt into combat deck resolution:
  - `bResolveCombatDeckOnUse`
  - `CombatDeckActionSlot`
  - `CombatDeckFlowRole`
  - `CombatDeckTriggerTiming`
- Default active skill deck role is `Skill / Catalyst`.
- `PlayerActiveSkillComponent` resolves the skill slot when the skill is configured to do so.

### Weapon Combo Node Context

- `WeaponComboNodeConfig` now carries:
  - combat deck action slot
  - combat deck flow role
- Runtime combo node activation writes the correct deck context:
  - normal attack -> `Attack / Starter`
  - weapon skill path -> currently `Dash / Catalyst`
  - special attack combo -> `WeaponSkill / Finisher`
- Combo finishers still map to finisher release mode, but this is separate from the deprecated QTE finisher system.

### Ranged / Projectile Propagation

- Musket and projectile paths propagate combat deck action slot and flow role.
- Spawned ranged projectiles can preserve the originating combat-card context.

### BuffFlow Node Support

- `BFNode_CombatCardContext` exposes action slot and flow role pins.
- The node can branch/filter by required action slot and flow role.
- This enables BuffFlow assets to react differently to starter, catalyst, and finisher contexts.

## Test Coverage Added Or Updated

Automation tests were added or updated around:

- normal release holding finisher-release cards
- finisher release holding normal cards
- action slot routing
- single skill slot repeated trigger
- flow role mismatch holding until correct role
- source asset routing into attack/skill/weapon skill/dash slots
- active skill combat deck defaults
- ComboGraph named inputs
- legacy Dash input routing to WeaponSkill
- invalid ComboGraph input not matching normal attack
- special attack data asset carrying ComboGraph
- player loading weapon graph and special attack graph separately
- BuffFlow storing action slot and flow role in combat-card context

Note: build passed, but UE automation tests were not rerun in this final pass.

## Editor / Asset Test Checklist

### No Weapon State

- Player spawns with no weapon.
- Left mouse / `NormalAttack` should trigger default unarmed attack graph, not dash.
- `WeaponSkill` input should trigger dash fallback.
- Confirm default unarmed combo graph points to:
  - `/Game/Code/Weapon/Disarm/GA_ComboGraph_Disarm`

### Two-Handed Sword Pickup

For `DA_WPN_THSword` and `CG_THSword_Test`:

- `WeaponDefinition.GameplayAbilityComboGraph` references the intended sword graph.
- Normal attack root is `NormalAttack` / legacy `Light`.
- Weapon skill root is `WeaponSkill`.
- Nodes have valid montage or montage config.
- If special attack is equipped, `DefaultSpecialAttack` references a `SpecialAttackDataAsset`.
- If that special attack needs graph logic, the special attack asset has `Config.ComboGraph`.

### Input Assets

Confirm Enhanced Input references:

- `IA_NormalAttack`
- `IA_SpecialAttack`
- `IA_WeaponSkill`
- `IMC_YogPlayerBase` maps them to the intended keyboard/mouse/gamepad buttons.
- `B_YogPlayerControllerBase` assigns the new input action fields.

### Combat Card Assets

For each combat card/rune DA:

- Attack sequence cards:
  - `RequiredActionSlot = Attack`
  - `RequiredFlowRole = Starter` or `Catalyst`
- Active skill cards:
  - `RequiredActionSlot = Skill`
  - usually `RequiredFlowRole = Catalyst`
- Weapon skill / detonate cards:
  - `RequiredActionSlot = WeaponSkill`
  - usually `RequiredFlowRole = Finisher`
- Dash/cancel cards:
  - `RequiredActionSlot = Dash`
  - usually `RequiredFlowRole = Catalyst`
- Link recipes should set `RequiredActionSlot` and `RequiredFlowRole` when the link should only trigger in a specific action context.

### BuffFlow Assets

For BuffFlow graphs that depend on combat context:

- Use `BFNode_CombatCardContext`.
- Read `ActionSlot` and `FlowRole`.
- Branch starter/catalyst/finisher effects explicitly.
- Use `RequiredActionSlot` and `RequiredFlowRole` on context checks when an effect must be slot-specific.

## Remaining Development Direction

### 1. Weapon Skill Versus Dash Naming

Current code path:

- `WeaponSkill` input first tries weapon ComboGraph `WeaponSkill`.
- If no graph node activates, it falls back to `PlayerState.AbilityCast.Dash`.
- The deck context for the graph-based weapon skill path is currently `Dash / Catalyst` in `TryActivateWeaponSkill`.

Recommended next decision:

- If weapon skill means weapon-provided detonate/finisher, change graph weapon skill context to `WeaponSkill / Finisher`.
- Keep fallback dash as `Dash / Catalyst`.
- This will make the user-facing model cleaner:
  - attack = ordered starter/catalyst cards
  - skill = player-selected catalyst
  - weapon skill = detonate/finisher
  - dash = mobility/cancel catalyst

### 2. Heavy Weapon Dodge Override

Not fully implementable yet from current code/data because:

- `EWeaponType` only has `Melee` and `Ranged`.
- There is no separate `Heavy` weapon type.
- There is no dedicated native `Dodge` ability class or `PlayerState.AbilityCast.Dodge` tag path.

Recommended implementation:

- Add weapon mobility mode data to `WeaponDefinition`, for example:
  - `DefaultDash`
  - `Dodge`
  - `WeaponOverrideAbility`
- Add a dedicated dodge gameplay tag and optional GA.
- Let `YogPlayerControllerBase::WeaponSkill` choose mobility fallback based on equipped weapon data.

### 3. Post-Attack Cancel Strengthening

Code has a post-attack window concept, but the strengthening rules need asset/design support.

Recommended implementation:

- Define gameplay tags for cancel windows:
  - post-attack cancel open
  - cancel performed in recovery
  - cancel bonus active
- Montage assets need notify windows for valid cancel timing.
- BuffFlow cards can then check `FlowRole` plus cancel tags to apply bonuses.

### 4. Two-Weapon Heat Switch

This is a larger feature and should stay separate from the current card slot refactor.

Recommended data/code pieces:

- second equipped weapon slot
- per-weapon combat deck save data
- heat/resource requirement
- switch activation ability
- parry/imbalance gameplay events
- cooldown reset rule for skill and weapon skill when switching during recovery cancel

### 5. Deprecated Finisher Cleanup

Old QTE finisher code is already guarded/removed in native code, but many assets/tags remain for compatibility.

Recommended cleanup order:

1. Keep deprecated tags until old saves/assets are audited.
2. Remove or archive deprecated finisher tutorial/UI/VFX assets only after references are confirmed gone.
3. Do not remove generic combo-final-hit fields such as `bIsComboFinisher`; those are still used by normal combo/card logic.

## Asset Requirement List

### Required To Test Current Branch

- `IA_NormalAttack`
- `IA_SpecialAttack`
- `IA_WeaponSkill`
- updated `IMC_YogPlayerBase`
- updated `B_YogPlayerControllerBase`
- default unarmed ComboGraph asset
- weapon ComboGraph assets with valid `NormalAttack` roots
- weapon ComboGraph assets with valid `WeaponSkill` roots where weapon skill behavior is expected
- valid montages or montage configs on all active graph nodes

### Required For Special Attack

- `SpecialAttackDataAsset` for each special attack
- ability class or granted ability config on special attack asset
- optional special attack ComboGraph
- special attack montage assets
- combo cancel window notifies if normal attack should interrupt/connect from special attack

### Required For Deck Build System

- attack-slot rune cards
- skill-slot rune cards
- weapon-skill-slot rune cards
- dash-slot rune cards
- BuffFlow assets for starter/catalyst/finisher roles
- link recipes with action slot / flow role conditions
- UI support to display:
  - attack ordered deck
  - skill single slot
  - weapon skill single slot
  - dash single slot

### Required For Heavy Weapon Dodge

- weapon classification beyond current `Melee/Ranged`, or a dedicated mobility mode field
- dodge gameplay tag
- dodge GA or dash variant ability
- dodge montage
- heavy weapon DA configured to use dodge fallback or override ability

### Required For Two-Weapon Switch

- secondary weapon slot assets/UI
- per-weapon deck storage
- heat/resource UI
- weapon switch GA
- parry/imbalance gameplay cue/effect assets
- cooldown reset policy data

## Suggested Next Work Order

1. Open editor and validate input assets/controller assignment.
2. Validate no-weapon normal attack and weapon-skill dash fallback.
3. Validate `DA_WPN_THSword`:
   - normal attack root works
   - weapon skill root works or intentionally falls back
   - special attack asset/graph is assigned if needed
4. Configure several test rune cards:
   - Attack / Starter
   - Attack / Catalyst
   - Skill / Catalyst
   - WeaponSkill / Finisher
   - Dash / Catalyst
5. Test attack ordered card sequence and single-slot repeat trigger in PIE.
6. Decide whether graph-based weapon skill should be `WeaponSkill / Finisher` instead of current `Dash / Catalyst`.
7. Only after the above is stable, start heavy weapon dodge and two-weapon switch as separate feature work.
