# GameplayTag Redirect Cleanup

## Current Rule

Old runtime tag definitions should be removed from `Config/Tags` after assets have been migrated to the formal tag. Keep `Config/DefaultGameplayTags.ini` redirects until map-authored tags and legacy saves have also been verified.

## Cleaned Definitions

The following old names are no longer formal tag dictionary entries, or are typo aliases that should not be authored directly. They remain only as redirects:

| Old Tag | New Tag |
| --- | --- |
| `GameplayCue.Character. SuperResist` | `GameplayCue.Character.SuperResist` |
| `Action.Rune.KnockbackApplied` | `Buff.Event.KnockbackApplied` |
| `Action.Rune.SlashWaveHit` | `Buff.Event.SlashWaveHit` |
| `Action.Rune.MoonlightBurnHit` | `Buff.Event.Moonlight.BurnHit` |
| `Action.Rune.MoonlightPoisonHit` | `Buff.Event.Moonlight.PoisonHit` |
| `Action.Rune.MoonlightPoisonExpired` | `Buff.Event.Moonlight.PoisonExpired` |
| `Action.Rune.MoonlightShield` | `Buff.Event.Moonlight.Shield` |
| `Action.Rune.MoonlightSlash` | `Buff.Event.Moonlight.Slash` |
| `Action.Rune.MoonlightSplit` | `Buff.Event.Moonlight.Split` |
| `Action.Rune.ShadowMarkDetonate` | `Buff.Event.ShadowMarkDetonate` |
| `Event.Rune.KnockbackApplied` | `Buff.Event.KnockbackApplied` |
| `Event.Rune.BleedApplied` | `Buff.Event.Bleed` |
| `Event.Rune.KillConfirmed` | `Buff.Event.KillConfirmed` |
| `GameplayCue.Rune` | `GameplayCue.Buff` |
| `GameplayCue.Rune.AtkUp` | `GameplayCue.Buff.AttackUp` |
| `GameplayCue.Rune.Burn` | `GameplayCue.Buff.Fire` |
| `GameplayCue.Rune.Burn.Vfx` | `GameplayCue.Buff.Fire.Vfx` |
| `GameplayCue.Rune.Cursed` | `GameplayCue.Buff.Curse` |
| `GameplayCue.Rune.DeathPoison` | `GameplayCue.Buff.DeathPoison` |
| `GameplayCue.Rune.Fearless` | `GameplayCue.Buff.Fearless` |
| `GameplayCue.Rune.FinisherCharge` | `GameplayCue.Buff.FinisherCharge` |
| `GameplayCue.Rune.KillExplosion` | `GameplayCue.Buff.KillExplosion` |
| `GameplayCue.Rune.MoonlightSlash.Fire` | `GameplayCue.Buff.MoonlightSlash.Fire` |
| `GameplayCue.Rune.MoonlightSlash.Hit` | `GameplayCue.Buff.MoonlightSlash.Hit` |
| `GameplayCue.Rune.MoonlightSlash.Pierce` | `GameplayCue.Buff.MoonlightSlash.Pierce` |
| `GameplayCue.Rune.MoonlightSlash.Split` | `GameplayCue.Buff.MoonlightSlash.Split` |
| `GameplayCue.Rune.Poison.Vfx` | `GameplayCue.Buff.Poison.Vfx` |
| `GameplayCue.Rune.Shield` | `GameplayCue.Buff.Shield` |

## Redirect Removal Gate

Do not delete these `GameplayTagRedirects` yet. They can be removed only after all of the following are true:

- `GameplayTagAssetMigrationCommandlet` dry-run reports `Tags would change: 0`.
- A focused World/map migration pass has been run, because the current commandlet skips `World` assets.
- Legacy save/load compatibility is no longer required, or old saves have an explicit save migration path.
- Static scans show no non-documentation references to the old tag names.

Until then, redirects are not new authoring entries; they are load-time compatibility only.
