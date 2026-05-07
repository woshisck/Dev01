"""
Batch operations on RuneDA / EffectDA (modifies assets, run with care).

Usage examples:
    # Set GoldCost = 99 for all RuneDA (clamped to >= 0):
    Tools/DataEditor/batch_ops.ps1 -Op set_rune_gold -Value 99

    # Set Effect Duration = 5.0 for all EffectDA:
    Tools/DataEditor/batch_ops.ps1 -Op set_effect_duration -Value 5.0

    # Set Effect MaxStack = 3:
    Tools/DataEditor/batch_ops.ps1 -Op set_effect_maxstack -Value 3

Headless operations save modified assets before the editor exits. Review the
asset diff in editor or source control; use git restore for rollback after save.
"""

import unreal
import argparse


def save_loaded_assets(label, assets):
    saved = 0
    failed = 0
    for asset in assets:
        if not asset:
            continue
        if unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=True):
            saved += 1
        else:
            failed += 1
            unreal.log_error(f"{label}: failed to save {asset.get_path_name()}")
    unreal.log(f"{label}: saved {saved} dirty assets, failed {failed}")
    if failed:
        raise RuntimeError(f"{label}: failed to save {failed} assets")
    return saved


def set_rune_gold(value):
    targets = unreal.DataEditorLibrary.get_all_rune_d_as()
    unreal.DataEditorLibrary.batch_set_rune_gold_cost(targets, int(value))
    save_loaded_assets("set_rune_gold", targets)
    unreal.log(f"set_rune_gold: {len(targets)} RuneDA -> {int(value)}")


def set_rune_rarity(value):
    targets = unreal.DataEditorLibrary.get_all_rune_d_as()
    rarity = unreal.RuneRarity[value.upper()]
    unreal.DataEditorLibrary.batch_set_rune_rarity(targets, rarity)
    save_loaded_assets("set_rune_rarity", targets)
    unreal.log(f"set_rune_rarity: {len(targets)} RuneDA -> {value}")


def set_effect_duration(value):
    targets = unreal.DataEditorLibrary.get_all_effect_d_as()
    unreal.DataEditorLibrary.batch_set_effect_duration(targets, float(value))
    save_loaded_assets("set_effect_duration", targets)
    unreal.log(f"set_effect_duration: {len(targets)} EffectDA -> {float(value)}")


def set_effect_maxstack(value):
    targets = unreal.DataEditorLibrary.get_all_effect_d_as()
    unreal.DataEditorLibrary.batch_set_effect_max_stack(targets, int(value))
    save_loaded_assets("set_effect_maxstack", targets)
    unreal.log(f"set_effect_maxstack: {len(targets)} EffectDA -> {int(value)}")


OPS = {
    "set_rune_gold":        set_rune_gold,
    "set_rune_rarity":      set_rune_rarity,
    "set_effect_duration":  set_effect_duration,
    "set_effect_maxstack":  set_effect_maxstack,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--op", choices=list(OPS.keys()), required=True)
    parser.add_argument("--value", required=True,
                        help="int / float / rarity name (Common/Rare/Epic/Legendary)")
    args, _unknown = parser.parse_known_args()
    OPS[args.op](args.value)


if __name__ == "__main__":
    main()
