"""
Smoke test for the DataEditor toolkit (read-only).

Run via PowerShell wrapper:
    Tools/DataEditor/run_smoke_test.ps1

What it does (no asset modification):
    1. Lists all RuneDA / EffectDA via UDataEditorLibrary
    2. Runs UDataValidator::ValidateAll
    3. Runs UDataEditorLibrary::VerifyAccessorParity
    4. Exports both DA types to CSV under Saved/Balance/

Output:
    - Console / Output Log (LogPython, LogDataEditor, LogDataValidator)
    - Saved/Balance/SmokeTest_<timestamp>/report.txt  -- machine-readable summary
"""

import unreal
import os
import datetime
import traceback


def banner(text):
    bar = "=" * 78
    line = f"\n{bar}\n  {text}\n{bar}"
    unreal.log(line)
    print(line)
    return line


def write_report(report_dir, sections):
    os.makedirs(report_dir, exist_ok=True)
    report_path = os.path.join(report_dir, "report.txt")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("Data Editor Smoke Test Report\n")
        f.write(f"Generated: {datetime.datetime.now().isoformat()}\n")
        f.write("=" * 78 + "\n\n")
        for title, body in sections:
            f.write(f"[ {title} ]\n")
            f.write(str(body) + "\n\n")
    unreal.log(f"Report written to: {report_path}")
    return report_path


def main():
    sections = []
    overall_pass = True

    # ── 1. Asset collection ──
    banner("1. Collecting RuneDA / EffectDA")
    try:
        rune_das = unreal.DataEditorLibrary.get_all_rune_d_as()
        effect_das = unreal.DataEditorLibrary.get_all_effect_d_as()
        character_das = unreal.DataEditorLibrary.get_all_character_d_as()
        montage_config_das = unreal.DataEditorLibrary.get_all_montage_config_d_as()
        montage_attack_das = unreal.DataEditorLibrary.get_all_montage_attack_data_d_as()
        musket_tuning_das = unreal.DataEditorLibrary.get_all_musket_action_tuning_d_as()
        msg = (
            f"RuneDA count: {len(rune_das)}\n"
            f"EffectDA count: {len(effect_das)}\n"
            f"CharacterData count: {len(character_das)}\n"
            f"MontageConfigDA count: {len(montage_config_das)}\n"
            f"MontageAttackDataAsset count: {len(montage_attack_das)}\n"
            f"MusketActionTuningDataAsset count: {len(musket_tuning_das)}"
        )
        unreal.log(msg)
        sections.append(("Asset Collection", msg))
        if len(character_das) == 0 or len(montage_config_das) == 0:
            overall_pass = False
    except Exception as e:
        overall_pass = False
        err = traceback.format_exc()
        unreal.log_error(err)
        sections.append(("Asset Collection", f"FAILED: {e}\n{err}"))

    # ── 2. Validate All ──
    banner("2. UDataValidator::ValidateAll")
    try:
        report = unreal.DataValidator.validate_all()
        msg = (
            f"Scanned: {report.scanned_count}\n"
            f"Errors:  {report.error_count}\n"
            f"Warnings:{report.warning_count}\n"
            f"Summary: {report.summary}"
        )
        unreal.log(msg)
        sections.append(("Validation", msg))
        if report.error_count > 0:
            overall_pass = False
    except Exception as e:
        overall_pass = False
        err = traceback.format_exc()
        unreal.log_error(err)
        sections.append(("Validation", f"FAILED: {e}\n{err}"))

    # ── 3. Accessor Parity ──
    banner("3. UDataEditorLibrary::VerifyAccessorParity")
    try:
        diff = unreal.DataEditorLibrary.verify_accessor_parity()
        msg = f"Accessor/field diff count: {diff} (expected 0)"
        unreal.log(msg)
        sections.append(("Accessor Parity", msg))
        if diff != 0:
            overall_pass = False
    except Exception as e:
        overall_pass = False
        err = traceback.format_exc()
        unreal.log_error(err)
        sections.append(("Accessor Parity", f"FAILED: {e}\n{err}"))

    # ── 4. CSV export ──
    banner("4. CSV Export")
    try:
        rune_csv = unreal.DataEditorLibrary.export_rune_d_as_to_csv("")
        effect_csv = unreal.DataEditorLibrary.export_effect_d_as_to_csv("")
        msg = f"Rune CSV:   {rune_csv}\nEffect CSV: {effect_csv}"
        unreal.log(msg)
        sections.append(("CSV Export", msg))
        if not rune_csv or not effect_csv:
            overall_pass = False
    except Exception as e:
        overall_pass = False
        err = traceback.format_exc()
        unreal.log_error(err)
        sections.append(("CSV Export", f"FAILED: {e}\n{err}"))

    # ── Final summary ──
    banner("SMOKE TEST RESULT: " + ("PASS" if overall_pass else "FAIL"))
    sections.append(("Overall", "PASS" if overall_pass else "FAIL"))

    project_dir = unreal.SystemLibrary.get_project_directory()
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    report_dir = os.path.join(project_dir, "Saved", "Balance", f"SmokeTest_{stamp}")
    write_report(report_dir, sections)

    return overall_pass


if __name__ == "__main__":
    success = main()
    if not success:
        unreal.log_error("Smoke test FAILED. See report and Output Log.")
        raise RuntimeError("DataEditor smoke test failed")
