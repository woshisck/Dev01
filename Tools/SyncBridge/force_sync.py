#!/usr/bin/env python3
"""
Force Sync - 全量对比并同步两个目录，不触发编译。
用于首次对齐目录，或手动触发单次完整同步。

用法:
  python force_sync.py              # Git → P4 方向
  python force_sync.py --reverse    # P4 → Git 方向
"""

import json
import os
import sys
import stat
import shutil
import fnmatch
import subprocess
from pathlib import Path


def load_config():
    cfg_path = Path(__file__).parent / "config.json"
    if not cfg_path.exists():
        print("[错误] 找不到 config.json")
        sys.exit(1)
    with open(cfg_path, "r", encoding="utf-8") as f:
        return json.load(f)


def should_ignore(relpath, patterns):
    """支持通配符（如 *.sln）的忽略检查"""
    name = Path(relpath).name
    for pat in patterns:
        clean = pat.rstrip("/*")
        if "*" in clean or "?" in clean:
            if fnmatch.fnmatch(relpath, clean) or fnmatch.fnmatch(name, clean):
                return True
            continue
        if relpath == clean or relpath.startswith(clean + "/"):
            return True
        if clean in Path(relpath).parts:
            return True
    return False


def p4_cmd(cfg, subcmd):
    env = os.environ.copy()
    env["P4PORT"] = cfg["p4"]["port"]
    env["P4USER"] = cfg["p4"]["user"]
    env["P4CLIENT"] = cfg["p4"]["workspace"]
    if cfg["p4"].get("password"):
        env["P4PASSWD"] = cfg["p4"]["password"]
    # charset 为空时不设置，避免 Unicode 服务器报错
    if cfg["p4"].get("charset"):
        env["P4CHARSET"] = cfg["p4"]["charset"]
    elif "P4CHARSET" in env:
        del env["P4CHARSET"]

    r = subprocess.run(
        f"p4 {subcmd}", shell=True, env=env,
        capture_output=True, text=True, encoding="utf-8", errors="replace"
    )
    ok = r.returncode == 0
    out = (r.stdout or "") + (r.stderr or "")
    return ok, out


def sync_directory(src_root, dst_root, ignore_patterns):
    """全量对比同步，返回 (实际变更文件列表, 统计信息)"""
    src = Path(src_root)
    dst = Path(dst_root)
    synced_files = []
    copied, deleted, skipped, unchanged = 0, 0, 0, 0

    # 收集 src 中所有文件
    src_files = set()
    for f in src.rglob("*"):
        if not f.is_file():
            continue
        rel = str(f.relative_to(src)).replace("\\", "/")
        if should_ignore(rel, ignore_patterns):
            skipped += 1
            continue
        src_files.add(rel)

        d = dst / rel
        need_copy = True
        if d.exists():
            # 大小和时间戳相同则跳过
            if (f.stat().st_size == d.stat().st_size
                    and abs(f.stat().st_mtime - d.stat().st_mtime) < 2):
                need_copy = False
                unchanged += 1

        if need_copy:
            d.parent.mkdir(parents=True, exist_ok=True)
            if d.exists():
                d.chmod(stat.S_IWRITE | stat.S_IREAD)
            shutil.copy2(f, d)
            copied += 1
            synced_files.append(rel)

    # 删除 dst 中有但 src 没有的文件
    for f in dst.rglob("*"):
        if not f.is_file():
            continue
        rel = str(f.relative_to(dst)).replace("\\", "/")
        if should_ignore(rel, ignore_patterns):
            continue
        if rel not in src_files:
            f.chmod(stat.S_IWRITE | stat.S_IREAD)
            f.unlink()
            deleted += 1
            synced_files.append(rel)

    stats = {"copied": copied, "deleted": deleted,
             "skipped": skipped, "unchanged": unchanged}
    return synced_files, stats


def p4_reconcile_files(cfg, p4_root, files, batch_size=50):
    """只对变更文件执行 reconcile"""
    ok_count, err_count = 0, 0
    for i in range(0, len(files), batch_size):
        batch = files[i:i + batch_size]
        for f in batch:
            fpath = str(Path(p4_root) / f.replace("/", os.sep))
            ok, out = p4_cmd(cfg, f'reconcile "{fpath}"')
            out_lower = out.lower()
            if ok:
                if "no file(s) to reconcile" not in out_lower and out.strip():
                    ok_count += 1
            else:
                if ("no file(s) to reconcile" not in out_lower
                        and "not under client" not in out_lower):
                    print(f"  [警告] reconcile 失败 [{f}]: {out.strip()[:150]}")
                    err_count += 1

        done = min(i + batch_size, len(files))
        print(f"  reconcile 进度: {done}/{len(files)}", end="\r")
    print()
    if err_count:
        print(f"  reconcile: {ok_count} 成功, {err_count} 失败")
    return ok_count


def main():
    reverse = "--reverse" in sys.argv
    cfg = load_config()
    ignore = cfg.get("ignore_patterns", [])

    git_path = cfg["git"]["repo_path"]
    p4_path = cfg["p4"]["workspace_path"]

    if reverse:
        src, dst = p4_path, git_path
        direction = "P4 → Git"
    else:
        src, dst = git_path, p4_path
        direction = "Git → P4"

    print(f"{'='*50}")
    print(f"  Force Sync ({direction})  [仅同步，不编译]")
    print(f"  源:   {src}")
    print(f"  目标: {dst}")
    print(f"{'='*50}")
    print()

    confirm = input("确认开始同步? (y/N): ").strip().lower()
    if confirm != "y":
        print("已取消")
        return

    print("\n正在对比文件...")
    synced_files, stats = sync_directory(src, dst, ignore)
    print(f"  复制: {stats['copied']}  删除: {stats['deleted']}  "
          f"未变: {stats['unchanged']}  忽略: {stats['skipped']}")

    if not synced_files:
        print("\n两个目录已完全同步，无需操作。")
        return

    if not reverse:
        # Git → P4
        print(f"\n正在 P4 reconcile ({len(synced_files)} 个变更文件)...")
        p4_reconcile_files(cfg, p4_path, synced_files)

        ok, opened = p4_cmd(cfg, "opened")
        if not ok:
            print(f"\n[错误] p4 opened 失败: {opened.strip()[:300]}")
            return

        opened_text = opened.strip()
        if not opened_text or "not opened" in opened_text.lower():
            print("\nP4 无变更需要提交。目录已同步。")
            return

        opened_lines = [l for l in opened_text.splitlines() if l.strip()]
        print(f"\nP4 已打开 {len(opened_lines)} 个文件")
        if len(opened_lines) <= 20:
            for l in opened_lines:
                print(f"  {l.strip()}")
        else:
            for l in opened_lines[:10]:
                print(f"  {l.strip()}")
            print(f"  ... 还有 {len(opened_lines) - 10} 个文件")

        submit = input("\n确认提交到 P4? (y/N): ").strip().lower()
        if submit == "y":
            ok, out = p4_cmd(cfg, 'submit -d "[SyncBridge] force sync from Git"')
            if ok:
                print("P4 提交成功！")
            else:
                print(f"P4 提交失败:\n  {out.strip()[:500]}")
        else:
            print("已跳过提交（文件已复制但未提交到 P4）。")
    else:
        # P4 → Git
        print("\n正在 Git add...")
        for f in synced_files:
            subprocess.run(
                f'git -c core.quotePath=false add "{f}"',
                shell=True, cwd=git_path,
                capture_output=True
            )

        r = subprocess.run(
            "git diff --cached --name-only",
            shell=True, cwd=git_path,
            capture_output=True, text=True
        )
        if not r.stdout.strip():
            print("Git 无变更。目录已同步。")
            return

        submit = input("\n确认提交并推送到 Git? (y/N): ").strip().lower()
        if submit == "y":
            subprocess.run(
                'git commit -m "[SyncBridge] force sync from P4"',
                shell=True, cwd=git_path
            )
            remote = cfg["git"].get("remote", "origin")
            branch = cfg["git"]["branch"]
            subprocess.run(f"git push {remote} {branch}", shell=True, cwd=git_path)
            print("Git 推送完成！")
        else:
            print("已跳过推送（变更已暂存但未提交）。")


if __name__ == "__main__":
    main()