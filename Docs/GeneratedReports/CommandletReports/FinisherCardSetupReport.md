# Finisher Card Setup Report
- Mode: Apply

## GameplayEffect assets
- Found `/Game/Code/GAS/Abilities/Finisher/GE_Mark_Finisher`.
- Found `/Game/Code/GAS/Abilities/Finisher/GE_FinisherCharge`.
- Configured `/Game/Code/GAS/Abilities/Finisher/GE_FinisherCharge`: duration=8.0s stackLimit=5 grantedTag=Buff.Status.FinisherCharge.
- Found `/Game/Code/GAS/Abilities/Finisher/GE_Mark_Finisher`.
- Configured `/Game/Code/GAS/Abilities/Finisher/GE_Mark_Finisher`: duration=12.0s stackLimit=1 grantedTag=Buff.Status.Mark.Finisher.
- Found `/Game/Code/GAS/Abilities/Finisher/GE_FinisherDamage`.
- `GE_FinisherDamage` duplicated from project SetByCaller damage template; FA uses `Data.Damage`.

## GameplayAbility Blueprints
- Found `/Game/Code/GAS/Abilities/Finisher/BGA_ApplyMark_Finisher`.
- Found `/Game/Code/GAS/Abilities/Finisher/BGA_FinisherCharge`.
- Found `/Game/Code/GAS/Abilities/Finisher/BGA_Player_FinisherAttack`.
- Found `/Game/Code/GAS/Abilities/Finisher/BGA_ApplyMark_Finisher`.
- Found `/Game/Code/GAS/Abilities/Finisher/AM_Player_FinisherAttack`.
- Configured `BGA_FinisherCharge`.
- Configured `BGA_Player_FinisherAttack`; CancelAbilitiesWithTag excludes FinisherCharge.
- Configured `BGA_ApplyMark_Finisher`: trigger Action.Mark.Apply.Finisher -> `GE_Mark_Finisher`.

## Rune and Flow assets
- Found `/Game/YogRuneEditor/Flows/FA_FinisherCard_ChargeHit`.
- Found `/Game/YogRuneEditor/Flows/FA_FinisherCard_Detonate`.
- Found `/Game/YogRuneEditor/Flows/FA_FinisherCard_BaseEffect`.
- Found `/Game/YogRuneEditor/Flows/FA_FinisherCard_ChargeHit`.
- Found `/Game/YogRuneEditor/Flows/FA_FinisherCard_Detonate`.
- Configured `FA_FinisherCard_BaseEffect`.
- Configured `FA_FinisherCard_ChargeHit`.
- Configured `FA_FinisherCard_Detonate`.
- Found `/Game/YogRuneEditor/Runes/DA_Rune_Finisher`.
- Configured `DA_Rune_Finisher` tuning rows and CombatCard BaseFlow.

## Ability set
- `DA_Base_AbilitySet_Initial` already has `BGA_FinisherCharge_C`, or no supported ability list was found.
- `DA_Base_AbilitySet_Initial` already has `BGA_Player_FinisherAttack_C`, or no supported ability list was found.
- `DA_Base_AbilitySet_Initial` already has `BGA_ApplyMark_Finisher_C`, or no supported ability list was found.