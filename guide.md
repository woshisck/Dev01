# Project Guide

This guide captures current project direction and working assumptions. `AGENTS.md` asks Codex to read this file before starting repository work.

## Current Direction

- Player combat input is now four independent actions: Attack, Skill, WeaponSkill, and Dash.
- Skill is the player-selected active skill handled by `PlayerActiveSkillComponent` / `ActiveSkillDataAsset`; do not route it through the deprecated SpecialAttack system.
- Attack and WeaponSkill runtime input should activate the broad action tags only. Legacy Combo1-4 tags remain for montage-row compatibility and asset migration, but they should not drive card finisher logic or direct input combo chaining.
- ComboGraph is no longer part of the player runtime combat path. Keep old graph fields/classes only as asset-load compatibility until legacy assets are migrated and resaved.

## GameplayTag Cleanup Direction

- Current tag reorganization direction is documented in `Docs/04_开发实现与系统文档/标签/GameplayTag_ReorgPlan_LOTF.md`.
- Treat LOTF as an engineering-governance reference, not a root-name template: keep authoritative config dictionaries, add redirects for renames, split high-churn `GameState.Flags.*`, and use code-side tag constants/native handles for core runtime tags.
- `Buff.*` is the formal top-level system for buff/debuff/status-effect semantics and the merged card/rune effect vocabulary. Use flat tags such as `Buff.Fire`, `Buff.Poison`, `Buff.Moonlight`, `Buff.WeaponSkillFinisher`, and only add child tags when the child is a real sub-effect such as `Buff.Fire.Explode`.
- Do not introduce `Buff.Status.*`, `Buff.ID.*`, `Buff.Keyword.*`, `Buff.Binding.*`, `Rune.ID.*`, or `Rune.Effect.*` for new content. Identity, binding/action slot, trigger timing, flow role, rarity, and values belong in the DA fields/tables. GameplayTags describe runtime/query effect semantics.
- Buff/card/rune internal gameplay events use `Buff.Event.*`. `Action.Rune.*` and `Event.Rune.*` are deprecated compatibility sources only and should be migrated by `GameplayTagAssetMigrationCommandlet`.
- Buff/card/rune presentation cues use `GameplayCue.Buff.*`. `GameplayCue.Rune.*` is deprecated compatibility only and should be migrated by `GameplayTagAssetMigrationCommandlet`, including the old QTE finisher cue to `GameplayCue.Buff.FinisherCharge`.
- After migration, do not keep `Action.Rune.*` or `GameplayCue.Rune.*` as formal `Config/Tags` dictionary definitions. Keep their `GameplayTagRedirects` only as load-time compatibility until World assets and old saves no longer need them.
- Legacy sustained status tags under `Character.State.*`, such as `Character.State.Feared`, `Character.State.Frozen`, `Character.State.Stunned`, and `Character.State.SuperArmor`, are compatibility-only and should migrate to flat `Buff.*` tags.
- Player and enemy should share common runtime character states under `Character.State.*` where the state is not identity-specific, such as skill execution, death, hit reaction, knockback, and movement action states.
- Native player and Musket combat abilities should expose current action state through formal `Character.State.*` `ActivationOwnedTags`. Legacy `PlayerState.AbilityCast.*` tags may remain on `AbilityTags` for fallback activation and old asset loading, but they should not be used as the current-state source for HUD, StateConflict, AI, or debug queries.
- The merged card/rune system no longer has a formal `Rune.*` tag root. Old `Card.*`, `Rune.*`, `Buff.Status.*`, and combat-deck `Combo.*` tags are migration/compatibility sources, not new-entry roots.
- Do not introduce `Rune.Deck.*` or other Rune metadata roots as formal tag roots. Deck/CombatDeck is the runtime container; binding, activation, trigger, flow role, and identity are DA fields or enum fields.
- Player action tags are Attack, Skill, WeaponSkill, and Dash only. Do not reintroduce LightAttack/HeavyAttack as formal action tags; old LightAtk maps to Attack and old HeavyAtk maps to WeaponSkill only as asset-load compatibility.
- Compatibility tags should live with their owning system dictionary instead of staying in `Config/DefaultGameplayTags.ini`: legacy `PlayerState.*` belongs in `Config/Tags/PlayerGameplayTag.ini`, legacy `Card.*` / `Rune.*` belongs in `Config/Tags/RuneTag.ini`, and legacy sustained `Character.State.Feared/Frozen/Stunned/SuperArmor` belongs in `Config/Tags/BuffTag.ini` as migration aliases to flat `Buff.*`.
- `Config/DefaultGameplayTags.ini` is no longer the place to add system-owned gameplay tags. Keep it for global GameplayTags settings and redirects; add new tags to the owning dictionary under `Config/Tags` such as `CharacterTag.ini`, `RuneTag.ini`, `BuffTag.ini`, `GameplayCueTag.ini`, `GameplayEventTag.ini`, `CurrencyTag.ini`, `Data.ini`, `LootTag.ini`, or another clearly named owner file.
- Tutorial hint tags live in `Config/Tags/TutorialTag.ini`. `Tutorial.Hint.HeavyCard` and `Tutorial.Hint.Finisher` are compatibility-only; new first-run card tutorial logic should prefer `Tutorial.Hint.WeaponSkillFinisher` or specific non-deprecated hint tags.
- Story and tutorial orchestration should move toward a `Director.*` system. Save data records tutorial/story progress results through `GameState.Flags.*`; Director decides when to override rooms, drops, tasks, dialogue, and tutorial steps.
- Asset tag migration must go through `GameplayTagAssetMigrationCommandlet`, not manual binary edits to `.uasset` files. Run it without `-Apply` first to generate `Docs/GeneratedReports/CommandletReports/GameplayTagAssetMigrationReport.md`; only use `-Apply` after reviewing report-only hits such as deprecated Special/Finisher tags, `Rune.Card.*`, set elements, and map keys.
- `GameplayTagAssetMigrationCommandlet` now forces a synchronous AssetRegistry scan before loading assets and skips `World` assets by default. This avoids unrelated map/package load failures while migrating data assets, widgets, Blueprints, and rule assets. If map-authored GameplayTag properties must be migrated later, run a focused pass after fixing the affected map assets.
- Deprecated QTE Finisher card/effect tags such as `Card.ID.Finisher`, `Card.Effect.Finisher`, `Rune.ID.Finisher`, and `Rune.Effect.Finisher` are report-only in the migration commandlet. The old gameplay cue `GameplayCue.Rune.FinisherCharge` migrates to `GameplayCue.Buff.FinisherCharge` so assets no longer point at the Rune cue root, but this does not re-enable the old QTE Finisher runtime path.

## Attack / WeaponSkill Refactor Follow-ups

- Build/compile was not run for the Attack / WeaponSkill / Dash / Skill refactor because this guide says not to compile unless explicitly requested.
- Add Unreal `ClassRedirects` for deleted native classes if legacy Blueprint assets still reference them:
  `UGA_Player_LightAtk1-4`, `UGA_Player_HeavyAtk1-4`, `UGA_Player_DashAtk`, and possibly `UGA_PlayerMeleeAttack`.
- Asset migration/resave was not done. Existing input assets, combo graph assets, Blueprint GA assets, and UI data may still need editor-side validation after the C++ rename.
- Some compatibility names/tags were intentionally left in config, including legacy `LightAtk`/`HeavyAtk` gameplay tags, `Special`/`SpecialAttack` compatibility tags, and `ComboSpecialActionAbility` wrapper redirects.
- The old `Character/InputBufferComponent` legacy component is now only an asset-load compatibility shell; live player code uses `Component/BufferComponent`.
- `UGA_RangeAttack` remains a stub; projectile/hitscan implementation is still pending.
- DevKit-only ComboGraph node fields are deprecated for player runtime combat. Prefer AbilityData montage rows and montage notifies for combo windows.

## Initial Data Assets

- Weapon combat AbilityData is merged onto runtime `CharacterData->AbilityData` in `APlayerCharacterBase::ApplyAbilityDataFromWeapon`. `WeaponDefinition.AttackAbilityData` owns attack + dash rows, `WeaponDefinition.WeaponSkillAbilityData` owns weapon skill rows, and `WeaponDefinition.PassiveAbilityData` owns weapon-specific reaction/death passive rows such as `Action.HitReact.Front`, `Action.HitReact.Back`, and `Action.Dead`. `WeaponDefinition.SpecialAbilityData` is deprecated compatibility data and is not merged into runtime combat. The legacy all-in-one `WeaponDefinition.AbilityData` slot has been removed.

- `DA_Base_AbilitySet_Initial` (`/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial`): base `UGASTemplate` loaded by every character at `BeginPlay` via `YogCharacterBase`. Contains shared reactive GAs (`GA_Dead`, `GA_HitReaction`, `GA_Knockback`, etc.). Do **not** put weapon combat montage-routing GAs here; keep player combat grants on the player combat ability set.
- `CharacterData` GAS template (`UGASTemplate::AbilityMap`): per-character ability grants applied during `InitializeComponentsWithStats`. Logged as `"Grant ability from GAS Template: <name>"` in the output log. Same rule: no weapon combat GAs here.
- `DefaultUnarmedWeaponDef` (`APlayerCharacterBase::DefaultUnarmedWeaponDef`): a `UWeaponDefinition` asset assigned in the player character Blueprint. Auto-equipped at `BeginPlay` if `EquippedWeaponDef` is still null after all init (i.e. no weapon loaded from save). Set `DefaultUnarmedWeaponDef` on the BP so the full weapon path (ability data + deck + weapon type tag) is initialized consistently.
- `DA_DefaultCombatAbility` (`UYogAbilitySet'/Game/Docs/GlobalSet/CharacterBaseSet/DA_DefaultCombatAbility.DA_DefaultCombatAbility'`): player-only `UYogAbilitySet` assigned to `APlayerCharacterBase::DefaultCombatAbilitySet`. Granted once at `BeginPlay` (after `Super::BeginPlay`). These abilities are weapon-agnostic; the equipped weapon's AbilityData controls which montage each action plays. Do **not** grant or revoke these per weapon switch; they live for the entire play session.

## Combat Architecture

- Normal melee attacks use GAS ability tags and AbilityData montage maps.
- Weapon AbilityData assets are selected from `WeaponDefinition`.
- Enemy weapon attacks/skills use `EnemyWeaponDefinition`, not player `WeaponDefinition`: set `EnemyData.DefaultWeaponDefinition` or `AllowedWeaponDefinitions`, assign the enemy weapon `AbilityData`, and configure montage rows keyed by `Enemy.Melee.*` and `Enemy.Skill.Skill1-4`.
- For enemy weapon skills, also grant the matching `GA_Enemy_Skill1-4` classes in the enemy `GASTemplate.AbilityMap`; `EnemyWeaponDefinition.AttackProfile` must include entries with matching `AbilityTags` and `AttackRole = Skill` so `BTTask_EnemyAttackByProfile` can choose and activate them.
- Player input routing starts in `YogPlayerControllerBase`.
- GAS abilities, gameplay tags, montage notifies, and data assets are all part of combat behavior; inspect all relevant pieces before changing a flow.
- Combat cards and runes use `CombatDeckComponent`, `RuneDataAsset`, and BuffFlow assets.
- Active skills use `PlayerActiveSkillComponent` and `ActiveSkillDataAsset`; this is the runtime path for the player's selected Skill action.
- WeaponSkill input is independent from Attack, Dash, and Skill. The old heavy attack input path is now the WeaponSkill path.
- Skill and WeaponSkill share `PlayerState.Cooldown.SkillShared`. Active skills start the shared cooldown from their selected skill config; WeaponSkill starts it from GAS cooldown time or the montage ability fallback duration, and recovery-cancel weapon switching clears both.
- The old SpecialAttack data/ability/component classes are deprecated compatibility shells only. New skill behavior belongs in active skill assets/abilities.
- `PlayerState.AbilityCast.CanCombo` is now treated as an action-interruption/cancel window. Do not use it to advance Attack/WeaponSkill Combo1-4 chains in player runtime input.

## Future Cleanup Markers

- `BackpackGridComponent` on `APlayerCharacterBase`: if can be cleaned.
- `CombatDeckComponent` on `APlayerCharacterBase`: if can be cleaned.
- `PlayerActiveSkillComponent` on `APlayerCharacterBase`: if can be cleaned.

## Finisher Notes

- The old finisher system is deprecated; native finisher GAS classes, montage notifies, and QTE HUD code have been removed while legacy finisher tags/assets are kept only for compatibility unless explicitly needed.
- Legacy finisher assets may remain in source control for old asset/save loading, but they must not be enabled as gameplay runtime entry points. Any remaining code path must either be guarded by `DevKit::Combat::IsFinisherAbilityDeprecated()`, prune/ignore deprecated finisher cards, or no-op immediately. Do not add new tutorial, card, input, UI, or reward flow that depends on `Card.ID.Finisher`, `Card.Effect.Finisher`, `PlayerState.AbilityCast.Finisher*`, or `Action.Finisher*`.
- `AYogGameMode::bCountCombatClearsForTemporaryFinisherUnlock` is deprecated compatibility data. Even if an older Blueprint has it enabled, runtime finisher unlock counting is ignored while `IsFinisherAbilityDeprecated()` is true.
- Heavy attack was previously hijacked during `Buff.FinisherExecuting`; that route is deprecated and heavy attack should stay available for the weapon skill path.
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
  - `GameplayCue.Buff.FinisherCharge`
  - `Rune.Library.Finisher`
  - `Tutorial.Hint.Finisher`
  - `Rune.ID.Finisher`
  - `Buff.FinisherCharge`
  - `Buff.FinisherExecuting`
  - `Buff.Mark.Finisher`
  - `Buff.FinisherWindowOpen`
  - `Buff.FinisherQTEOpen`
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
