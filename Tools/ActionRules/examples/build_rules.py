"""An ordinary developer-owned Python authoring script, never evaluated from Excel."""
import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from action_rules import build_document, write_document


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()
    identities = {"Attack": "Character.State.Skill.Attack", "Dash": "Character.State.Movement.Dash",
                  "WeaponSkill": "Character.State.Skill.WeaponSkill"}
    actions = [{"id": name, "displayName": name, "abilityTags": [tag], "stateTags": [tag]}
               for name, tag in identities.items()]
    # These disabled examples do not change the live project policy.
    relations = [("Dash", "Attack", "block"), ("Attack", "Dash", "interrupt"),
                 ("Attack", "WeaponSkill", "interrupt"), ("Dash", "WeaponSkill", "block")]
    rules = [{"id": f"R{index:03}", "enabled": False, "subject": "player", "current": current,
              "request": request, "effect": effect, "condition": "", "reason": "Disabled authoring example"}
             for index, (current, request, effect) in enumerate(relations, 1)]
    write_document(build_document("PlayerCombat.Example", actions, rules), args.output, force=args.force)
    print("Wrote example only. Not mounted or published.")


if __name__ == "__main__":
    main()
