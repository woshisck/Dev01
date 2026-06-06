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

## Finisher Notes

- The old finisher system uses `DA_Rune_Finisher`, `GA_FinisherCharge`, `GA_ApplyFinisherMark`, `GA_Player_FinisherAttack`, finisher gameplay tags, montage notifies, and QTE HUD.
- Heavy attack is currently hijacked during `Buff.Status.FinisherExecuting` to send `Action.Finisher.Confirm`.
- If replacing heavy attack with weapon skills, avoid depending on the old finisher QTE path.

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
