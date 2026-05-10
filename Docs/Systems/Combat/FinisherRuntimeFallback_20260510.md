# Finisher Runtime Fallback and Aura Setup

## Runtime behavior

- `BGA_Player_FinisherAttack` still prefers montage notifies:
  - `ANS_FinisherTimeDilation` opens the QTE slow-motion window.
  - `AN_MeleeDamage` with `EventTag = Ability.Event.Finisher.HitFrame` opens the finisher hit frame.
- If either notify is missing from `AM_Player_FinisherAttack`, the GA now schedules a fallback:
  - QTE window starts at `FallbackQTEWindowStartTime`, lasts `FallbackQTEWindowDuration`, and uses `FallbackQTESlowDilation`.
  - Hit frame fires at `FallbackHitFrameTime`, then applies the default melee hitbox and detonates finisher marks.

## Damage box

- The fallback hit frame uses the owner character's `DefaultMeleeTargetType` and `DefaultMeleeDamageEffect`.
- If the montage has `AN_MeleeDamage`, the notify's `ActRange` and `HitboxTypes` are used.
- If the notify is missing, `FallbackFinisherActionData` is used. Current default:
  - `ActDamage = 60`
  - `ActRange = 480`
  - annulus hitbox, `degree = 115`

## QTE

- Heavy attack input only confirms while `Buff.Status.FinisherQTEOpen` is present.
- Confirming inside the window restores time dilation early and marks the finisher as confirmed.
- The mark detonation receives `EventMagnitude = ConfirmedDamageMultiplier` when confirmed.

## Pre-finisher aura

- The GA calls `StartPreFinisherAura()` when the finisher ability starts.
- The character plays a separate gold overlay through `StartFinisherAuraFlash()` / `StopFinisherAuraFlash()`.
- Optional Niagara is exposed on `BGA_Player_FinisherAttack`:
  - `PreFinisherAuraNiagara`
  - `PreFinisherAuraAttachSocketName`
  - `PreFinisherAuraLocationOffset`
  - `PreFinisherAuraRotationOffset`
  - `PreFinisherAuraScale`
  - `PreFinisherAuraDuration`
- Blueprint extension points:
  - `BP_OnPreFinisherAuraStarted`
  - `BP_OnPreFinisherAuraEnded`

## Time dilation desaturation

- All gameplay time dilation paths now call `TimeDilationVisualSubsystem`.
- The subsystem spawns an unbound transient post-process volume and drives screen saturation toward gray while slow motion is active.
- When slow motion ends, saturation restores quickly.
- Current connected paths:
  - `ANS_FinisherTimeDilation`
  - `BGA_Player_FinisherAttack` fallback QTE window
  - player damage time dilation
  - `LENode_TimeDilation`
  - weapon tutorial slow-motion intro
  - level-end slow motion

## Combo requirement hint

- `BGA_FinisherCharge` shows the existing lightweight `InfoPopup` HUD prompt when the finisher card is played.
- Default text: `在强化时间内打出 H -> H -> H，最后一击后会自动触发终结技。`
- Configurable fields:
  - `bShowComboHintOnActivate`
  - `ComboHintPopup`
  - `ComboHintTitle`
  - `ComboHintBody`
  - `ComboHintDuration`

## Temporary initial unlock gate

- The runtime deck component automatically adds `/Game/YogRuneEditor/Runes/DA_Rune_Finisher` through `TemporaryInitialFinisherRune`.
- Do not place the temporary finisher manually in the weapon `InitialCombatDeck`; the component removes duplicates and inserts it into the last visible active slot.
- Packaged builds use `TemporaryFinisherUnlockCompletedBattles = 3`.
- Before the unlock count is reached, the card is visible as `LOCK current/required`. If consumed while locked, it advances the deck but does not execute the card flow.
- Completed combat clears are counted in `AYogGameMode::CompletedCombatBattleCount` and persisted in `FRunState::CompletedCombatBattleCount`.
- Editor-only GM command: `Yog_UnlockFinisher`.

## Recommended asset setup

For exact animation timing, still add these notifies to `AM_Player_FinisherAttack`:

1. Add `ANS_FinisherTimeDilation` over the QTE window.
2. Add `AN_MeleeDamage` on the strike frame.
3. Set the `AN_MeleeDamage` event tag to `Ability.Event.Finisher.HitFrame`.
4. Tune `ActRange` and `HitboxTypes` on the notify for the final strike.

The fallback exists to keep the finisher functional when the notifies are missing or temporarily misconfigured.
