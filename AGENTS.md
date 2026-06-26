# Repository Instructions

This file applies to the whole repository.

## Startup Context

- Before starting analysis, implementation, or review work, read `guide.md` in the repository root if it exists.
- If `guide.md` is missing, continue with the request and mention only if the missing guide affects the task.
- Treat newer user instructions in the current thread as higher priority than this file or `guide.md`.

## Project Shape

- This is an Unreal Engine 5.8 project: `DevKit.uproject`.
- Main runtime code lives under `Source/DevKit`.
- Editor automation and commandlets live under `Source/DevKitEditor`.
- Gameplay tags are configured under `Config/Tags` and `Config/DefaultGameplayTags.ini`.
- Content assets live under `Content`; avoid editing generated assets or commandlet-created assets unless the task asks for it.
- Project docs live under `Docs`.

## Working Rules

- Read the relevant code and local docs before changing behavior.
- Prefer existing systems and naming patterns over new abstractions.
- Keep changes scoped to the requested gameplay/system behavior.
- Do not revert unrelated user changes.
- Use `rg` for search.
- Use `apply_patch` for manual text/code edits.
- Avoid destructive commands unless the user explicitly asks for them.

## Unreal And Gameplay Notes

- For combat work, check the relevant gameplay tags, GAS ability classes, montage notifies, and data assets before changing input or ability flow.
- For combo work, inspect `ComboRuntimeComponent`, `GameplayAbilityComboGraph`, and weapon combo graph assets/config references.
- For weapon work, inspect `WeaponDefinition`, `WeaponInstance`, weapon-granted abilities, and current equipped weapon state.
- For combat-card/rune work, inspect `CombatDeckComponent`, `RuneDataAsset`, BuffFlow assets, and related gameplay tags.

## Verification

- When a build is needed, prefer `CompileAndOpen.bat` or the Unreal 5.8 `Build.bat` path available on this machine.
- If a test/build cannot be run because Unreal, assets, or permissions are unavailable, say so clearly.
- For small code-only changes, at minimum run targeted search/compile checks when practical.

## Communication

- Explain the actual trigger chain or data flow when changing gameplay behavior.
- For reviews, lead with concrete findings and file/line references.
- Keep final summaries concise: what changed, where, and what was verified.
