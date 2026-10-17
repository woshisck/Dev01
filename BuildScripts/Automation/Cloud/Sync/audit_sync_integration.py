"""Read-only integration invariants; inspect the index, never build output."""
import argparse
import json
import subprocess
from pathlib import Path

MAIN = '7c3c3da714abaf497d3139fdae7ce6d1e3fb08a9'
FEATURE = '4d12013c9a9da45ca036e2b16e0b0f78c5acfb85'
BASE = '291265d2eac41d3165cb4f56ae8526b02f857cee'
# Exact reviewed adaptations of imported editor code to main's removed portal model.
# A different blob still fails the preservation audit; this is not a path-wide bypass.
REVIEWED_ADAPTATIONS = {
    'Source/DevKitEditor/Private/Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.cpp':
        '5586b46265826f73d0dd2b6db5ed898fdb1140a3',
    'Source/DevKitEditor/Private/Tests/SceneRoomConfigWidgetTests.cpp':
        '909d177cd7bb160ee2fe6ca9c39cd5e16548d24b',
}


def git(repo, *args):
    return subprocess.run(['git', '-C', str(repo), *args], check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout


def changed(repo, base, tip):
    return set(git(repo, 'diff', '--name-only', '-z', base, tip).decode('utf-8').rstrip('\0').split('\0')) - {''}


def blobs(repo, ref):
    records = git(repo, 'ls-files', '--stage', '-z') if ref == ':' else git(repo, 'ls-tree', '-r', '-z', ref)
    result = {}
    for item in records.rstrip(b'\0').split(b'\0'):
        if not item:
            continue
        meta, path = item.split(b'\t', 1)
        mode, middle, last = meta.decode('ascii').split()
        if ref == ':':
            if last != '0':
                raise ValueError('Unmerged index stage')
            oid = middle
        else:
            oid = last
        result[path.decode('utf-8')] = (mode, oid)
    return result


def audit(repo):
    main, feature, index = (blobs(repo, x) for x in (MAIN, FEATURE, ':'))
    main_changed = changed(repo, BASE, MAIN)
    feature_changed = changed(repo, BASE, FEATURE)
    failures = []
    for name in sorted(main_changed - feature_changed):
        if index.get(name) != main.get(name):
            failures.append(f'Latest main change lost: {name}')
    for name in sorted(feature_changed - main_changed):
        expected = ('100644', REVIEWED_ADAPTATIONS[name]) if name in REVIEWED_ADAPTATIONS else feature.get(name)
        if index.get(name) != expected:
            failures.append(f'Reviewed P4 integration change lost: {name}')
    staged = set(git(repo, 'diff', '--cached', '--name-only', '-z', MAIN).decode('utf-8').rstrip('\0').split('\0')) - {''}
    for name in sorted(staged):
        parts = name.lower().split('/')
        ext = Path(name).suffix.lower()
        allowed_examples = name in {'Content/ActionRules/Examples/README.md',
                                    'Content/ActionRules/Examples/conditions.lua',
                                    'Content/ActionRules/Examples/player_combat.example.json'}
        if ((parts[0] in {'engine', 'content', 'binaries', 'intermediate', 'saved', 'deriveddatacache'} and not allowed_examples)
                or any(x in {'binaries', 'intermediate', 'saved', 'obj', 'node_modules'} for x in parts)
                or ext in {'.uasset', '.umap', '.dll', '.exe', '.lib', '.pdb', '.zip', '.7z', '.a', '.dylib'}):
            failures.append(f'Forbidden staged payload: {name}')
        if name in index and index[name][0] in {'120000', '160000'}:
            failures.append(f'Symlink/submodule needs separate review: {name}')
    both = sorted(main_changed & feature_changed)
    report = dict(main=MAIN, feature=FEATURE, git_merge_base=BASE,
                  latest_main_paths_verified=len(main_changed - feature_changed),
                  feature_paths_verified=len(feature_changed - main_changed),
                  exact_reviewed_portal_adaptations=REVIEWED_ADAPTATIONS,
                  both_sides_changed=both, staged_path_count=len(staged),
                  failures=failures, passed=not failures, publishes_or_mutates=False)
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = audit(args.repo)
    text = json.dumps(result, ensure_ascii=False, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + '\n', encoding='utf-8')
    print(text)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
