# Project Guide

This guide captures current project direction and working assumptions. `AGENTS.md` asks Codex to read this file before starting repository work.

## Current Direction

- Player combat input is now four independent actions: Attack, Dash, WeaponSkill, and Special.
- Attack and WeaponSkill combos use explicit ability tags (`PlayerState.AbilityCast.Attack.Combo1-4` and `PlayerState.AbilityCast.WeaponSkill.Combo1-4`) plus AbilityData montage maps.
- ComboGraph is no longer part of the player runtime combat path. Keep old graph fields/classes only as asset-load compatibility until legacy assets are migrated and resaved.

## Attack / WeaponSkill Refactor Follow-ups

- Build/compile was not run for the Attack / WeaponSkill / Dash / Special refactor because this guide says not to compile unless explicitly requested.
- Add Unreal `ClassRedirects` for deleted native classes if legacy Blueprint assets still reference them:
  `UGA_Player_LightAtk1-4`, `UGA_Player_HeavyAtk1-4`, `UGA_Player_DashAtk`, and possibly `UGA_PlayerMeleeAttack`.
- Asset migration/resave was not done. Existing input assets, combo graph assets, Blueprint GA assets, and UI data may still need editor-side validation after the C++ rename.
- Some compatibility names/tags were intentionally left in code/config, including deprecated controller input fallbacks, legacy `LightAtk`/`HeavyAtk` gameplay tags, `SpecialAttack` compatibility tags, and `ComboSpecialActionAbility` wrapper redirects.
- The old `Character/InputBufferComponent` legacy component still has `LightAttack`/`HeavyAttack` enum names; live player code uses `Component/BufferComponent`, but the old component was not removed.
- `UGA_RangeAttack` remains a stub; projectile/hitscan implementation is still pending.
- DevKit-only ComboGraph node fields are deprecated for player runtime combat. Prefer AbilityData montage rows and montage notifies for combo windows.

## Initial Data Assets

- Weapon combat AbilityData is merged onto runtime `CharacterData->AbilityData` in `APlayerCharacterBase::ApplyAbilityDataFromWeapon`. `WeaponDefinition.AttackAbilityData` owns attack + dash rows, `WeaponDefinition.WeaponSkillAbilityData` owns weapon skill rows, and `WeaponDefinition.SpecialAbilityData` owns special rows. The legacy all-in-one `WeaponDefinition.AbilityData` slot has been removed.

- `DA_Base_AbilitySet_Initial` (`/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial`): base `UGASTemplate` loaded by every character at `BeginPlay` via `YogCharacterBase`. Contains shared reactive GAs (`GA_Dead`, `GA_HitReaction`, `GA_Knockback`, etc.). Do **not** put weapon combat montage-routing GAs here; keep player combat grants on the player combat ability set.
- `CharacterData` GAS template (`UGASTemplate::AbilityMap`): per-character ability grants applied during `InitializeComponentsWithStats`. Logged as `"Grant ability from GAS Template: <name>"` in the output log. Same rule: no weapon combat GAs here.
- `DefaultUnarmedWeaponDef` (`APlayerCharacterBase::DefaultUnarmedWeaponDef`): a `UWeaponDefinition` asset assigned in the player character Blueprint. Auto-equipped at `BeginPlay` if `EquippedWeaponDef` is still null after all init (i.e. no weapon loaded from save). Set `DefaultUnarmedWeaponDef` on the BP so the full weapon path (ability data + deck + special attack + weapon type tag) is initialized consistently.
- `DA_DefaultCombatAbility` (`UYogAbilitySet'/Game/Docs/GlobalSet/CharacterBaseSet/DA_DefaultCombatAbility.DA_DefaultCombatAbility'`): player-only `UYogAbilitySet` assigned to `APlayerCharacterBase::DefaultCombatAbilitySet`. Granted once at `BeginPlay` (after `Super::BeginPlay`). These abilities are weapon-agnostic; the equipped weapon's AbilityData controls which montage each action plays. Do **not** grant or revoke these per weapon switch; they live for the entire play session.

## Combat Architecture

- Normal melee attacks use GAS ability tags and AbilityData montage maps.
- Weapon AbilityData assets are selected from `WeaponDefinition`.
- Player input routing starts in `YogPlayerControllerBase`.
- GAS abilities, gameplay tags, montage notifies, and data assets are all part of combat behavior; inspect all relevant pieces before changing a flow.
- Combat cards and runes use `CombatDeckComponent`, `RuneDataAsset`, and BuffFlow assets.
- Active skills use `PlayerActiveSkillComponent` and `ActiveSkillDataAsset`; prefer reusing this shape for weapon skills when practical.
- WeaponSkill input is independent from Attack, Dash, and Special. The old heavy attack input path is now the WeaponSkill path.
- Special attack montages can reuse `AN_MeleeDamage` with `GameplayEffect.DamageType.GeneralAttack`; `UGA_PlayerSpecialAttack` listens to that by default and applies the normal melee damage path.
- To chain from an action montage into the next attack or weapon skill combo, add a montage combo window that grants `PlayerState.AbilityCast.CanCombo`; input routing only allows action montage interruption during that tag window.

## Future Cleanup Markers

- `BackpackGridComponent` on `APlayerCharacterBase`: if can be cleaned.
- `CombatDeckComponent` on `APlayerCharacterBase`: if can be cleaned.
- `PlayerActiveSkillComponent` on `APlayerCharacterBase`: if can be cleaned.

## Finisher Notes

- The old finisher system is deprecated; native finisher GAS classes, montage notifies, and QTE HUD code have been removed while legacy finisher tags/assets are kept only for compatibility unless explicitly needed.
- Heavy attack was previously hijacked during `Buff.Status.FinisherExecuting`; that route is deprecated and heavy attack should stay available for the weapon skill path.
- If replacing heavy attack with weapon skills, avoid depending on the old finisher QTE path.
- Deprecated native code removed/marked deleted:
  - `Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp`
  - `Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp`
  - `Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp`
  - `Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h`
  - `Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h`
  - `Source/DevKit/Public/AbilitySystem/Abilities/GA_Player_FinisherAttack.h`
  - `Source/DevKit/Private/Animation/ANS_FinisherTimeDilation.cpp`
  - `Source/DevKit/Private/Animation/AN_TriggerFinisherAbility.cpp`
  - `Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h`
  - `Source/DevKit/Public/Animation/AN_TriggerFinisherAbility.h`
  - `Source/DevKit/Private/UI/FinisherQTEWidget.cpp`
  - `Source/DevKit/Public/UI/FinisherQTEWidget.h`
  - `Source/DevKitEditor/Rune/FinisherCardSetupCommandlet.*`
  - `Source/DevKitEditor/UI/FinisherQTEWidgetSetupCommandlet.*`
- Deprecated finisher assets still present/tracked:
  - `Content/Code/GAS/Abilities/Finisher/*`
  - `Content/Code/GAS/GameplayCueNotifies/GCN_FinisherCharge.uasset`
  - `Content/Code/Weapon/TwoHandedSword/GeneratedMontages/*_Finisher.uasset`
  - `Content/YogRuneEditor/Runes/DA_Rune_Finisher.uasset`
  - `Content/YogRuneEditor/Flows/FA_FinisherCard_*.uasset`
  - `Content/UI/Playtest_UI/HUD/WBP_FinisherQTEPrompt.uasset`
  - `Content/Docs/UI/Tutorial/DA_Tutorial_Finisher.uasset`
  - `Content/Docs/UI/TutorialTex/512/T_Tutorial512_Finisher.uasset`
  - `Content/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_PrayerSacrificeFinisher.uasset`
  - `Content/VFX/Misc/*Finisher*.uasset`
  - `Content/VFX/Trail/Material/MI_Finisher_Fame.uasset`
  - `Plugins/YogAnimSource/Content/ElianAnim/TwoHandedSword/*FinisherAtk*.uasset`
  - `SourceArt/**/*Finisher*.png`
- Deprecated finisher tags retained only for legacy asset/save compatibility:
  - `Card.Effect.Finisher`
  - `Card.ID.Finisher`
  - `GameplayCue.Rune.FinisherCharge`
  - `Rune.Library.Finisher`
  - `Tutorial.Hint.Finisher`
  - `Rune.ID.Finisher`
  - `Buff.Status.FinisherCharge`
  - `Buff.Status.FinisherExecuting`
  - `Buff.Status.Mark.Finisher`
  - `Buff.Status.FinisherWindowOpen`
  - `Buff.Status.FinisherQTEOpen`
  - `Action.Mark.Apply.Finisher`
  - `Action.Mark.Detonate.Finisher`
  - `Action.Finisher.Confirm`
  - `Action.FinisherCharge.Activate`
  - `Action.FinisherCharge.ChargeConsumed`
  - `Ability.Event.Finisher.HitFrame`
  - `Action.Player.FinisherAttack`
  - `PlayerState.AbilityCast.Finisher`
  - `PlayerState.AbilityCast.FinisherCharge`
  - `Level.Stage.Finisher`
  - `Tutorial.Finisher`
  - `Story.Event.FirstRun.FinisherObtained`
  - `Story.Flag.FirstRun.FinisherObtained`
  - `Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.finisher_obtained`
- Remaining guarded/deprecation references:
  - `Source/DevKit/Public/Combat/FinisherDeprecation.h`
  - `Source/DevKit/Private/Cheater/Cheater.cpp`
  - `Source/DevKit/Private/Story/FirstRunTutorialDirectorSubsystem.cpp`
  - `Source/DevKit/Private/Character/PlayerCharacterBase.cpp`
  - `Source/DevKit/Private/Component/CombatDeckComponent.cpp`
  - `Source/DevKit/Public/Component/CombatDeckComponent.h`
  - `Tools/RuneEditor/finisher_card_setup_smoke.py`
- Do not confuse deprecated QTE finisher move code with generic combo-final-hit naming such as `bIsComboFinisher`, `IsCombatDeckComboFinisher`, or combo graph nodes marked as finishers. Those are normal combo/deck concepts unless the task explicitly removes combo-final-hit behavior.

## Unreal Project Notes

- Project: `DevKit.uproject`
- Engine: Unreal Engine 5.4
- Runtime code: `Source/DevKit`
- Editor commandlets/tools: `Source/DevKitEditor`
- Gameplay tags: `Config/Tags` and `Config/DefaultGameplayTags.ini`
- Main assets: `Content`
- Documentation: `Docs`

## Working Rules

- DO NOT COMPILE THE CODE UNLESS THE USER EXPLICITLY TELLS YOU TO DO SO.
- Read relevant code and local docs before editing.
- Use existing systems, naming, and asset conventions.
- Keep changes scoped to the requested behavior.
- Do not revert unrelated user changes.
- Use `rg` for search.
- Use `apply_patch` for manual code/text edits.
- Avoid destructive commands unless explicitly requested.

## Verification

- Do not compile code unless the user explicitly asks for compilation in the current thread.
- For Unreal builds, prefer `CompileAndOpen.bat` when opening the editor is acceptable.
- For build-only checks, use the UE 5.4 `Build.bat` path available on this machine.
- If a full build is too expensive or blocked, run targeted searches/checks and clearly state what was not verified.

## Communication

- Explain gameplay changes by describing the actual trigger chain or data flow.
- For reviews, list concrete findings first with file and line references.
- Keep summaries concise: changed files, behavior change, and verification result.
