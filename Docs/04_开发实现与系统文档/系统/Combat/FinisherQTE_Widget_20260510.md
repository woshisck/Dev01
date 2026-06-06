# Finisher QTE Window and HUD

## Runtime

- `ANS_FinisherTimeDilation` opens `Buff.Status.FinisherQTEOpen` at NotifyBegin and clears it at NotifyEnd.
- During `Buff.Status.FinisherExecuting`, heavy attack input only sends `Action.Finisher.Confirm` while `Buff.Status.FinisherQTEOpen` exists.
- `GA_Player_FinisherAttack` ignores confirm events that arrive outside the QTE window.
- Confirming clears `Buff.Status.FinisherQTEOpen`, restores time dilation, hides the QTE prompt, and keeps `EventMagnitude = ConfirmedDamageMultiplier` for mark detonation.

## HUD

- Widget class: `UFinisherQTEWidget`
- Generated WBP: `/Game/UI/Playtest_UI/HUD/WBP_FinisherQTEPrompt`
- HUD default field: `AYogHUD::FinisherQTEWidgetClass`
- Current generated assignment: `/Game/UI/B_HUD_Intro` -> `WBP_FinisherQTEPrompt`

`AYogHUD` creates the prompt at viewport Z `30`. The widget is hidden by default, shown by `ANS_FinisherTimeDilation`, and forcibly hidden by `GA_Player_FinisherAttack` on confirm/end.

## Montage Setup

In `AM_Player_FinisherAttack`:

1. Place `ANS_FinisherTimeDilation` over the valid QTE input frames.
2. Set `SlowDilation` to the desired slow-motion value, usually `0.15`.
3. Put the hit-frame notify after the input window and set its event tag to `Ability.Event.Finisher.HitFrame`.

Only heavy input inside the ANS range counts as QTE confirm.
