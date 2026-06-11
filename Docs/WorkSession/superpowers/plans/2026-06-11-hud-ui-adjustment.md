# HUD UI Adjustment Implementation Plan - 2026-06-11

## Scope
- Keep gold permanently visible in the bottom-right common resource panel.
- Show meta/progression material only as a transient pickup/spend toast, using placeholders until final art exists.
- Add a compact player buff strip above the health bar, populated from player-owned `Buff.Status.*` gameplay tags.
- Add front/back switch presentation to combat item and active skill slots: selected slot stays bright/front, non-selected slots dim/back, selection changes roll over 0.32s.
- Update the HUD setup commandlet so generated widgets keep the new layout.
- Add/extend automation coverage for the generated HUD widget tree.

## Constraints
- Preserve the playfield: keep new UI on existing HUD edges and avoid center/lower-middle clutter.
- Do not depend on missing art. Use UMG placeholder boxes and the agreed source-art paths.
- Avoid refactoring unrelated HUD systems.
- Work in branch `codex/hud-ui-adjustment`.

## Implementation Steps
1. Add a failing automation test for `PlayerBuffBar` placement in `WBP_HUDRoot`.
2. Implement `UPlayerBuffBarWidget` as a native placeholder widget that polls the owning player's ASC tags.
3. Add `PlayerBuffBar` binding to `UYogHUDRootWidget`.
4. Update `MainUISetupCommandlet` to generate the buff bar host above `PlayerHealthHost`.
5. Adjust `UPlayerCommonInfoWidget` material row to be hidden by default and timed visible after currency changes.
6. Add lightweight switch-roll presentation to existing combat item and active skill HUD slots.
7. Refresh the design doc status and run focused validation.

## Verification Targets
- `git diff --check`
- Focused Unreal automation/build command if available locally.
- Source scan confirming no missing generated widget references.
