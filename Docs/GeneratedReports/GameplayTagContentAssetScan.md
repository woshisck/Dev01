# GameplayTag Content Asset Scan

Generated after the 2026-06-24 GameplayTag cleanup pass.

This report now treats `GameplayTagAssetMigrationCommandlet` as the authoritative asset check. Earlier static byte-scan counts were useful as a migration queue, but they were not reliable proof of live GameplayTag properties because binary assets can retain old strings in names, imports, or historical serialized data.

## Final Commandlet Dry-Run

- Mode: `DryRun`
- Root: `/Game`
- IncludeDeprecated: `false`
- Assets visited: `6525`
- Assets with migration hits: `1`
- Tags would change: `0`
- Tags changed: `0`
- Report-only hits: `2`
- Skipped World assets: `177`

## Remaining Report-Only Hits

These are intentionally not auto-migrated because the old QTE Finisher path is deprecated compatibility data and must not become a formal active Rune runtime entry.

| Asset | Tag | Decision |
| --- | --- | --- |
| `/Game/YogRuneEditor/Runes/DA_Rune_Finisher` | `Card.ID.Finisher` | Keep as report-only compatibility data. |
| `/Game/YogRuneEditor/Runes/DA_Rune_Finisher` | `Card.Effect.Finisher` | Keep as report-only compatibility data. |

## Applied Asset Migrations

The cleanup pass migrated the safe asset references to the current tag model:

| Asset | Result |
| --- | --- |
| `/Game/Story/Rules/SR_FirstRun` | `first_run.heavy_card_obtained` migrated to `first_run.weapon_skill_finisher_obtained`. |
| `/Game/YogRuneEditor/Runes/DA_Rune_Burn` | `Card.ID.Burn` / `Card.Effect.Burn` migrated to `Rune.ID.Burn` / `Rune.Effect.Burn`. |
| `/Game/YogRuneEditor/Runes/DA_Rune_Poison` | `Card.ID.Poison` / `Card.Effect.Poison` migrated to `Rune.ID.Poison` / `Rune.Effect.Poison`. |
| `/Game/YogRuneEditor/Runes/DA_Rune_Moonlight` | `Card.ID.Moonlight`, `Card.Effect.Moonlight`, and link-recipe neighbor effects migrated to `Rune.*`. |

## Verification Notes

- `Config/DefaultGameplayTags.ini` has no `GameplayTagList` declarations.
- `Config/Tags/*.ini` has no duplicate gameplay tag declarations.
- `Rune.Deck.*` remains disallowed as a formal tag root.
- Legacy Heavy/Light/Card tags can still appear in config or docs only as compatibility tags, migration sources, or historical notes.
- World assets were skipped by the commandlet because full world loading can trip unrelated map/package issues. If world-authored GameplayTag properties become relevant, run a focused root scan after repairing the affected map assets.
