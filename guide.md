# Project Guide

This guide captures current project direction and working assumptions. `AGENTS.md` asks Codex to read this file before starting repository work.

## Current Direction

- Heavy attack should become the primary weapon skill input.
- Finisher action is deprecated unless the user explicitly asks to work on it.
- Prefer minor, code-focused changes over large system rewrites.
- Preserve the existing combo graph system for normal weapon attack chains.
- Treat weapon skills as equipped abilities/items that can be routed through GAS.

## Combat Architecture

- Normal melee attacks use `ComboRuntimeComponent` and weapon combo graphs.
- Weapon combo graph assets/configs are selected from `WeaponDefinition`.
- Player input routing starts in `YogPlayerControllerBase`.
- GAS abilities, gameplay tags, montage notifies, and data assets are all part of combat behavior; inspect all relevant pieces before changing a flow.
- Combat cards and runes use `CombatDeckComponent`, `RuneDataAsset`, and BuffFlow assets.
- Active skills use `PlayerActiveSkillComponent` and `ActiveSkillDataAsset`; prefer reusing this shape for weapon skills when practical.
- Heavy attack routes through `PlayerSpecialAttackComponent` when a complete `SpecialAttackDataAsset` is equipped; normal heavy combo is only the fallback with no equipped special attack.
- Special attack montages can reuse `AN_MeleeDamage` with `GameplayEffect.DamageType.GeneralAttack`; `UGA_PlayerSpecialAttack` listens to that by default and applies the normal melee damage path without advancing the weapon combo graph.
- To connect a special attack into the normal light combo before it finishes, add a montage combo window that grants `PlayerState.AbilityCast.CanCombo`; `UGA_PlayerSpecialAttack` consumes buffered light input during that window and starts the weapon combo graph light root.

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

- Read relevant code and local docs before editing.
- Use existing systems, naming, and asset conventions.
- Keep changes scoped to the requested behavior.
- Do not revert unrelated user changes.
- Use `rg` for search.
- Use `apply_patch` for manual code/text edits.
- Avoid destructive commands unless explicitly requested.

## Verification

- For Unreal builds, prefer `CompileAndOpen.bat` when opening the editor is acceptable.
- For build-only checks, use the UE 5.4 `Build.bat` path available on this machine.
- If a full build is too expensive or blocked, run targeted searches/checks and clearly state what was not verified.

## Communication

- Explain gameplay changes by describing the actual trigger chain or data flow.
- For reviews, list concrete findings first with file and line references.
- Keep summaries concise: changed files, behavior change, and verification result.
