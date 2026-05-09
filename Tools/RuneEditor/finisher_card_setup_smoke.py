#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def read_text(rel_path: str) -> str:
    return (ROOT / rel_path).read_text(encoding="utf-8")


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"{label}: missing {needle!r}")


def assert_not_contains(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"{label}: still contains {needle!r}")


def main() -> None:
    header = read_text("Source/DevKitEditor/Rune/FinisherCardSetupCommandlet.h")
    source = read_text("Source/DevKitEditor/Rune/FinisherCardSetupCommandlet.cpp")

    assert_contains(header, "UFinisherCardSetupCommandlet", "commandlet class")
    assert_contains(source, "/Game/YogRuneEditor/Runes/DA_Rune_Finisher", "finisher rune path")
    assert_contains(source, "/Game/YogRuneEditor/Flows/FA_FinisherCard_BaseEffect", "base flow path")
    assert_contains(source, "/Game/YogRuneEditor/Flows/FA_Finisher_ChargeHit", "charge hit flow path")
    assert_contains(source, "/Game/YogRuneEditor/Flows/FA_Finisher_Detonate", "detonate flow path")
    assert_contains(source, "/Game/Code/GAS/Abilities/Finisher/BGA_FinisherCharge", "charge ability path")
    assert_contains(source, "/Game/Code/GAS/Abilities/Finisher/BGA_Player_FinisherAttack", "attack ability path")
    assert_contains(source, "/Game/Code/GAS/Abilities/Finisher/GE_FinisherCharge", "charge GE path")
    assert_contains(source, "/Game/Code/GAS/Abilities/Finisher/GE_Mark_Finisher", "finisher mark GE path")
    assert_contains(source, "/Game/Code/GAS/Abilities/Finisher/GE_FinisherDamage", "damage GE path")
    assert_contains(source, "DetonationDamage", "detonation tuning")
    assert_contains(source, "KnockbackDistance", "knockback tuning")
    assert_contains(source, "Buff.Status.FinisherCharge", "charge granted tag")
    assert_contains(source, "Buff.Status.Mark.Finisher", "finisher mark granted tag")
    assert_contains(source, "Buff.Status.FinisherWindowOpen", "required window tag")
    assert_contains(source, "PlayerState.AbilityCast.FinisherCharge", "charge ability tag")
    assert_contains(source, "PlayerState.AbilityCast.Finisher", "finisher ability tag")
    assert_contains(source, "Action.FinisherCharge.ChargeConsumed", "charge consumed event")
    assert_contains(source, "Action.Mark.Apply.Finisher", "apply finisher mark event")
    assert_contains(source, "Action.Mark.Detonate.Finisher", "detonate finisher mark event")
    assert_not_contains(source, "Action.ApplyFinisherMark", "legacy apply mark event")
    assert_not_contains(source, "Action.FinisherAttack.DetonateTarget", "legacy detonate event")
    assert_contains(source, "FinisherCardSetupReport.md", "report file")

    print("Finisher card setup smoke checks passed.")


if __name__ == "__main__":
    main()
