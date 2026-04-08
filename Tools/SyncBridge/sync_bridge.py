#!/usr/bin/env python3
"""
SyncBridge v1.1 - Git ↔ P4 双向同步 + UE5 自动编译

用法:
  python sync_bridge.py                # 持续循环同步
  python sync_bridge.py --once         # 只执行一次
  python sync_bridge.py --compile-sync # 编译后同步到 P4（不循环）
  python sync_bridge.py -c my.json     # 指定配置文件
"""

import subprocess
import json
import os
import sys
import stat
import shutil
import time
import logging
import re
import fnmatch
import argparse
from pathlib import Path

SYNC_MARKER = "[SyncBridge]"
VERSION = "1.1.0"


class SyncBridge:
    def __init__(self, config_path):
        self.base_dir = Path(__file__).parent
        self.config = self._load_json(config_path)
        self.state_file = self.base_dir / "sync_state.json"
        self.lock_file = self.base_dir / ".sync_lock"
        self.state = self._load_json(self.state_file, default={})
        self._setup_logging()

    # ── 基础工具 ──────────────────────────────────────────────

    def _load_json(self, path, default=None):
        p = Path(path)
        if not p.is_absolute():
            p = self.base_dir / p
        if p.exists():
            with open(p, "r", encoding="utf-8") as f:
                return json.load(f)
        if default is not None:
            return default
        print(f"[错误] 找不到配置文件: {p}")
        print("请复制 config.example.json 为 config.json 并填写配置。")
        sys.exit(1)

    def _save_state(self):
        with open(self.state_file, "w", encoding="utf-8") as f:
            json.dump(self.state, f, indent=2, ensure_ascii=False)

    def _setup_logging(self):
        log_file = self.base_dir / self.config.get("log_file", "sync_bridge.log")
        level = getattr(logging, self.config.get("log_level", "INFO").upper(), logging.INFO)
        fmt = "%(asctime)s [%(levelname)s] %(message)s"
        # 避免重复添加 handler
        logger = logging.getLogger("SyncBridge")
        if not logger.handlers:
            logging.basicConfig(
                level=level,
                format=fmt,
                datefmt="%Y-%m-%d %H:%M:%S",
                handlers=[
                    logging.FileHandler(log_file, encoding="utf-8"),
                    logging.StreamHandler(sys.stdout),
                ],
            )
        self.log = logger

    def _run(self, cmd, cwd=None, env=None, timeout=300):
        """执行命令，返回 (成功, 输出文本)"""
        self.log.debug(f"CMD: {cmd}")
        try:
            r = subprocess.run(
                cmd,
                shell=True,
                cwd=cwd,
                env=env,
                capture_output=True,
                text=True,
                timeout=timeout,
                encoding="utf-8",
                errors="replace",
            )
            out = (r.stdout or "") + (r.stderr or "")
            if r.returncode != 0:
                self.log.debug(f"CMD 返回码 {r.returncode}: {out[:300]}")
                return False, out
            return True, r.stdout or ""
        except subprocess.TimeoutExpired:
            self.log.error(f"CMD 超时 ({timeout}s): {cmd}")
            return False, "TIMEOUT"
        except Exception as e:
            self.log.error(f"CMD 异常: {e}")
            return False, str(e)

    def _should_ignore(self, relpath):
        """检查文件是否在忽略列表中，支持通配符（*.sln 等）"""
        name = Path(relpath).name
        for pat in self.config.get("ignore_patterns", []):
            clean = pat.rstrip("/*")
            if "*" in clean or "?" in clean:
                # 通配符：匹配完整相对路径或仅文件名
                if fnmatch.fnmatch(relpath, clean) or fnmatch.fnmatch(name, clean):
                    return True
                continue
            # 精确路径匹配
            if relpath == clean or relpath.startswith(clean + "/"):
                return True
            if clean in Path(relpath).parts:
                return True
        return False

    # ── 首次运行初始化 ─────────────────────────────────────────

    def _ensure_initialized(self):
        """首次运行时记录当前 Git/P4 位置，避免全量同步"""
        if self.state.get("initialized"):
            return

        self.log.info("=" * 50)
        self.log.info("首次运行，正在初始化同步基线...")

        ok, h = self._git("rev-parse HEAD")
        if ok:
            self.state["last_git_commit"] = h.strip()

        cfg = self.config.get("p4", {})
        depot = cfg.get("depot_path", "//...")
        ok, out = self._p4(f"changes -m1 -s submitted {depot}")
        if ok and (out or "").strip():
            try:
                self.state["last_p4_changelist"] = int(out.strip().split()[1])
            except (IndexError, ValueError):
                self.state["last_p4_changelist"] = 0

        self.state["initialized"] = True
        self._save_state()
        self.log.info(
            f"基线已设定: Git={self.state.get('last_git_commit', '?')[:8]}, "
            f"P4=CL#{self.state.get('last_p4_changelist', 0)}"
        )
        self.log.info("=" * 50)

    # ── Git 操作 ───────────────────────────────────────────────

    def _git(self, subcmd, timeout=120):
        return self._run(f"git -c core.quotePath=false {subcmd}",
                         cwd=self.config["git"]["repo_path"], timeout=timeout)

    def _git_has_updates(self):
        """检查 GitHub 是否有新提交"""
        cfg = self.config["git"]
        remote = cfg.get("remote", "origin")
        branch = cfg["branch"]

        ok, _ = self._git(f"fetch {remote}")
        if not ok:
            self.log.error("git fetch 失败")
            return False

        ok, local = self._git("rev-parse HEAD")
        ok2, remote_h = self._git(f"rev-parse {remote}/{branch}")
        if not (ok and ok2):
            return False

        local, remote_h = local.strip(), remote_h.strip()
        if local == remote_h:
            return False

        # 反循环保护：远端最新提交是同步提交则跳过
        ok, msg = self._git(f"log {remote}/{branch} -1 --format=%s")
        if ok and SYNC_MARKER in (msg or ""):
            self.log.info("Git: 远端最新是同步提交，拉取并跳过")
            self._git(f"pull {remote} {branch}")
            self.state["last_git_commit"] = remote_h
            self._save_state()
            return False

        self.log.info(f"Git: 检测到远端更新 → {remote_h[:8]}")
        return True

    def _git_pull(self):
        """拉取更新，返回变更文件相对路径列表"""
        cfg = self.config["git"]
        remote = cfg.get("remote", "origin")
        branch = cfg["branch"]

        ok, before = self._git("rev-parse HEAD")
        if not ok:
            return []
        before = before.strip()

        ok, out = self._git(f"pull {remote} {branch}")
        if not ok:
            self.log.error(f"git pull 失败: {out.strip()[:300]}")
            return []

        ok, after = self._git("rev-parse HEAD")
        if not ok:
            return []
        after = after.strip()

        if before == after:
            self.log.info("git pull: 无新提交")
            return []

        ok, diff = self._git(f"diff --name-only {before} {after}")
        changed = [f for f in (diff or "").strip().splitlines()
                   if f and not self._should_ignore(f)]

        self.state["last_git_commit"] = after
        self._save_state()
        self.log.info(f"git pull 完成: {len(changed)} 个文件变更")
        return changed

    def _git_commit_push(self, files, desc):
        """暂存、提交并推送到 GitHub"""
        cfg = self.config["git"]
        remote = cfg.get("remote", "origin")
        branch = cfg["branch"]

        for f in files:
            self._git(f'add "{f}"')

        ok, staged = self._git("diff --cached --name-only")
        if not ok or not (staged or "").strip():
            self.log.info("Git: 无变更需要提交")
            return True

        msg = f"{SYNC_MARKER} {desc}"
        ok, out = self._git(f'commit -m "{msg}"')
        if not ok:
            self.log.error(f"git commit 失败: {out.strip()[:300]}")
            return False

        ok, out = self._git(f"push {remote} {branch}")
        if not ok:
            self.log.error(f"git push 失败: {out.strip()[:300]}")
            return False

        ok, h = self._git("rev-parse HEAD")
        if ok:
            self.state["last_git_commit"] = h.strip()
            self._save_state()

        self.log.info("git push 完成")
        return True

    # ── P4 操作 ────────────────────────────────────────────────

    def _p4(self, subcmd, timeout=120):
        """执行 p4 命令（自动注入连接参数）"""
        cfg = self.config["p4"]
        env = os.environ.copy()
        env["P4PORT"] = cfg["port"]
        env["P4USER"] = cfg["user"]
        env["P4CLIENT"] = cfg["workspace"]
        if cfg.get("password"):
            env["P4PASSWD"] = cfg["password"]
        # charset 只在非空时设置，避免 "Unicode clients require a unicode enabled server"
        if cfg.get("charset"):
            env["P4CHARSET"] = cfg["charset"]
        elif "P4CHARSET" in env:
            del env["P4CHARSET"]
        return self._run(f"p4 {subcmd}", env=env, timeout=timeout)

    def _p4_has_updates(self):
        """检查 P4 是否有新提交"""
        cfg = self.config["p4"]
        depot = cfg.get("depot_path", "//...")

        ok, out = self._p4(f"changes -m1 -s submitted {depot}")
        if not ok or not (out or "").strip():
            return False

        try:
            latest = int(out.strip().split()[1])
        except (IndexError, ValueError):
            return False

        last = self.state.get("last_p4_changelist", 0)
        if latest <= last:
            return False

        # 反循环保护
        ok, desc = self._p4(f"describe -s {latest}")
        if ok and SYNC_MARKER in (desc or ""):
            self.log.info("P4: 最新变更是同步提交，跳过")
            self.state["last_p4_changelist"] = latest
            self._save_state()
            return False

        self.log.info(f"P4: 新变更 CL#{latest} (上次: CL#{last})")
        return True

    def _p4_sync_and_get_changes(self):
        """同步 P4 工作区，返回变更文件的相对路径列表"""
        cfg = self.config["p4"]
        ws_path = Path(cfg["workspace_path"])

        ok, out = self._p4("sync", timeout=600)
        if not ok:
            self.log.error(f"p4 sync 失败: {(out or '').strip()[:300]}")
            return []

        out_text = (out or "").strip()

        # "up-to-date" 或无输出表示无变更
        if not out_text or "up-to-date" in out_text.lower():
            self._p4_update_cl_state()
            return []

        changed = []
        for line in out_text.splitlines():
            line = line.strip()
            if not line:
                continue
            # 提取 Windows 绝对路径（兼容各种语言环境的 P4 输出）
            m = re.search(r"([A-Za-z]:[/\\][^\n]+)$", line)
            if m:
                local = m.group(1).strip().strip('"')
                try:
                    rel = str(Path(local).relative_to(ws_path)).replace("\\", "/")
                    if not self._should_ignore(rel):
                        changed.append(rel)
                except ValueError:
                    self.log.debug(f"P4 sync: 路径不在工作区内，跳过: {local}")

        self._p4_update_cl_state()
        self.log.info(f"p4 sync 完成: {len(changed)} 个文件")
        return changed

    def _p4_update_cl_state(self):
        """更新保存的 P4 最新 CL 编号"""
        cfg = self.config["p4"]
        depot = cfg.get("depot_path", "//...")
        ok, out = self._p4(f"changes -m1 -s submitted {depot}")
        if ok and (out or "").strip():
            try:
                self.state["last_p4_changelist"] = int(out.strip().split()[1])
                self._save_state()
            except (IndexError, ValueError):
                pass

    def _p4_reconcile_submit(self, files, desc):
        """将变更文件提交到 P4"""
        cfg = self.config["p4"]
        ws = cfg["workspace_path"]

        # 逐文件 reconcile
        ok_count, err_count = 0, 0
        for f in files:
            fpath = str(Path(ws) / f.replace("/", os.sep))
            ok, out = self._p4(f'reconcile "{fpath}"')
            out_lower = (out or "").lower()
            if ok:
                if "no file(s) to reconcile" not in out_lower and out.strip():
                    ok_count += 1
            else:
                if ("no file(s) to reconcile" not in out_lower
                        and "not under client" not in out_lower):
                    self.log.warning(f"P4 reconcile 失败 [{f}]: {out.strip()[:200]}")
                    err_count += 1

        if err_count:
            self.log.warning(f"P4 reconcile: {ok_count} 成功, {err_count} 失败")

        # 检查是否有文件被打开
        ok, opened = self._p4("opened")
        if not ok:
            self.log.error(f"p4 opened 查询失败: {(opened or '').strip()[:300]}")
            return False

        opened_text = (opened or "").strip()
        if not opened_text or "not opened" in opened_text.lower():
            self.log.info("P4: 无变更需要提交")
            return True

        opened_count = len([l for l in opened_text.splitlines() if l.strip()])
        self.log.info(f"P4: {opened_count} 个文件待提交")

        msg = f"{SYNC_MARKER} {desc}"
        ok, out = self._p4(f'submit -d "{msg}"', timeout=300)
        if not ok:
            self.log.error(f"p4 submit 失败:\n{(out or '').strip()[:1000]}")
            # 回退未变更的已打开文件，清理状态
            self._p4("revert -a", timeout=60)
            return False

        self._p4_update_cl_state()
        self.log.info("p4 submit 完成")

        # 提交后将文件恢复为只读（P4 标准状态：未 checkout 的文件应为只读）
        restored = 0
        for f in files:
            fpath = Path(ws) / f.replace("/", os.sep)
            if fpath.exists():
                try:
                    fpath.chmod(stat.S_IREAD)
                    restored += 1
                except Exception:
                    pass
        if restored:
            self.log.info(f"已将 {restored} 个文件恢复为只读")

        return True

    # ── UE5 编译 ───────────────────────────────────────────────

    def _needs_compile(self, files):
        """检查变更文件是否包含需要编译的源码"""
        ue = self.config.get("ue5", {})
        if not ue.get("enabled"):
            return False
        triggers = ue.get("compile_triggers", [".cpp", ".h", ".cs"])
        return any(any(f.endswith(ext) for ext in triggers) for f in files)

    def _close_ue_editor(self):
        """关闭 UE 编辑器（释放 DLL 锁）。返回编辑器是否原本在运行"""
        ok, out = self._run('tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH', timeout=10)
        if not ok or "UnrealEditor.exe" not in (out or ""):
            return False

        self.log.info("检测到 UE 编辑器正在运行，正在关闭...")
        self._run('taskkill /IM UnrealEditor.exe', timeout=30)

        # 最多等待 60 秒
        for _ in range(30):
            time.sleep(2)
            ok, out = self._run('tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH', timeout=10)
            if "UnrealEditor.exe" not in (out or ""):
                self.log.info("UE 编辑器已关闭")
                return True

        # 超时强制终止
        self.log.warning("UE 编辑器关闭超时，强制终止...")
        self._run('taskkill /F /IM UnrealEditor.exe', timeout=15)
        time.sleep(5)
        return True

    def _snapshot_binaries(self):
        """记录编译前二进制目录的文件 mtime 快照"""
        git_root = Path(self.config["git"]["repo_path"])
        ue = self.config.get("ue5", {})
        watch_dirs = ue.get("compile_output_dirs", ["Binaries"])
        snap = {}
        for d in watch_dirs:
            dpath = git_root / d
            if dpath.exists():
                for f in dpath.rglob("*"):
                    if f.is_file():
                        snap[str(f)] = f.stat().st_mtime
        return snap

    def _diff_binaries(self, snap_before):
        """对比快照，返回编译后新增或修改的文件相对路径列表"""
        git_root = Path(self.config["git"]["repo_path"])
        ue = self.config.get("ue5", {})
        watch_dirs = ue.get("compile_output_dirs", ["Binaries"])
        changed = []
        for d in watch_dirs:
            dpath = git_root / d
            if not dpath.exists():
                continue
            for f in dpath.rglob("*"):
                if not f.is_file():
                    continue
                fstr = str(f)
                mtime = f.stat().st_mtime
                old_mtime = snap_before.get(fstr, -1)
                if mtime > old_mtime + 1:  # 1 秒容差
                    rel = str(f.relative_to(git_root)).replace("\\", "/")
                    if not self._should_ignore(rel):
                        changed.append(rel)
        self.log.info(f"编译产物: 检测到 {len(changed)} 个新增/修改文件")
        return changed

    def _compile_ue5(self):
        """关闭编辑器 → 编译。返回 (ok: bool, changed_binaries: list)"""
        ue = self.config["ue5"]
        engine = ue["engine_path"]

        self._close_ue_editor()

        # 编译前快照
        snap_before = self._snapshot_binaries()

        # 构建编译命令
        dotnet_ver = ue.get("dotnet_version", "6.0.302")
        dotnet_dir = os.path.join(
            engine, "Engine", "Binaries", "ThirdParty", "DotNet", dotnet_ver, "windows"
        )
        ubt = os.path.join(
            engine, "Engine", "Binaries", "DotNET", "UnrealBuildTool", "UnrealBuildTool.dll"
        )
        project = os.path.join(self.config["git"]["repo_path"], ue["project_file"])
        target = ue.get("target", "DevKitEditor")
        platform = ue.get("platform", "Win64")
        configuration = ue.get("configuration", "Development")

        env = os.environ.copy()
        env["DOTNET_ROOT"] = dotnet_dir

        cmd = (
            f'"{os.path.join(dotnet_dir, "dotnet.exe")}" "{ubt}" '
            f"{target} {platform} {configuration} "
            f'-Project="{project}" -WaitMutex'
        )

        self.log.info("开始 UE5 编译...")
        self.log.info(f"  目标: {target} | {platform} | {configuration}")
        ok, out = self._run(cmd, env=env, timeout=3600)

        if ok:
            self.log.info("UE5 编译成功")
            return True, self._diff_binaries(snap_before)
        else:
            self.log.error(f"UE5 编译失败！\n{(out or '').strip()[-2000:]}")
            return False, []

    # ── 文件同步 ───────────────────────────────────────────────

    def _sync_files(self, src_root, dst_root, files):
        """将变更文件从源目录同步到目标目录（支持增删改）"""
        src = Path(src_root)
        dst = Path(dst_root)
        copied, deleted, errors = 0, 0, 0

        for f in files:
            s = src / f
            d = dst / f
            try:
                if s.exists():
                    d.parent.mkdir(parents=True, exist_ok=True)
                    if d.exists():
                        d.chmod(stat.S_IWRITE | stat.S_IREAD)
                    shutil.copy2(s, d)
                    copied += 1
                else:
                    # 源不存在 = 已被删除
                    if d.exists():
                        d.chmod(stat.S_IWRITE | stat.S_IREAD)
                        d.unlink()
                        deleted += 1
            except Exception as e:
                self.log.warning(f"文件同步失败 [{f}]: {e}")
                errors += 1

        self.log.info(
            f"文件同步: 复制 {copied}, 删除 {deleted}"
            + (f", 失败 {errors}" if errors else "")
            + f"  ({src_root} → {dst_root})"
        )

    # ── 主流程 ─────────────────────────────────────────────────

    def _flow_git_to_p4(self):
        """Git → P4 方向同步"""
        self.log.info("── 检查 Git → P4 ──")

        if not self._git_has_updates():
            return

        changed = self._git_pull()
        if not changed:
            return

        self.log.info(
            f"Git 变更 ({len(changed)}): "
            + ", ".join(changed[:5])
            + ("..." if len(changed) > 5 else "")
        )

        # 有源码变更时编译
        if self._needs_compile(changed):
            ok, bin_files = self._compile_ue5()
            if not ok:
                self.log.error("编译失败，本轮跳过 P4 同步")
                return
            # 合并编译产物（去重）
            changed_set = set(changed)
            for f in bin_files:
                if f not in changed_set:
                    changed.append(f)
                    changed_set.add(f)
            self.log.info(f"合并后同步文件数: {len(changed)}")

        self._sync_files(
            self.config["git"]["repo_path"],
            self.config["p4"]["workspace_path"],
            changed,
        )
        self._p4_reconcile_submit(changed, f"sync {len(changed)} files from Git")

    def _flow_p4_to_git(self):
        """P4 → Git 方向同步"""
        self.log.info("── 检查 P4 → Git ──")

        if not self._p4_has_updates():
            return

        changed = self._p4_sync_and_get_changes()
        if not changed:
            return

        self.log.info(
            f"P4 变更 ({len(changed)}): "
            + ", ".join(changed[:5])
            + ("..." if len(changed) > 5 else "")
        )

        self._sync_files(
            self.config["p4"]["workspace_path"],
            self.config["git"]["repo_path"],
            changed,
        )
        self._git_commit_push(changed, f"sync {len(changed)} files from P4")

    def compile_and_sync(self):
        """手动触发：编译 UE5 → 将编译产物同步到 P4"""
        ue = self.config.get("ue5", {})
        if not ue.get("enabled"):
            self.log.error("ue5.enabled 为 false，请先在 config.json 中启用")
            return False

        self.log.info("=" * 50)
        self.log.info("Compile & Sync 开始")
        self.log.info("=" * 50)

        ok, bin_files = self._compile_ue5()
        if not ok:
            self.log.error("编译失败，退出")
            return False

        if not bin_files:
            self.log.info("没有检测到新的编译产物，跳过 P4 同步")
            return True

        self.log.info(f"正在同步 {len(bin_files)} 个编译产物到 P4...")
        self._sync_files(
            self.config["git"]["repo_path"],
            self.config["p4"]["workspace_path"],
            bin_files,
        )

        ok = self._p4_reconcile_submit(bin_files, f"compiled binaries ({len(bin_files)} files)")
        if ok:
            self.log.info("Compile & Sync 完成")
        return ok

    def run_once(self):
        """执行一轮同步（有锁保护）"""
        if self.lock_file.exists():
            age = time.time() - self.lock_file.stat().st_mtime
            if age < 7200:
                self.log.warning("另一个同步进程正在运行，跳过本轮")
                return
            self.log.warning("检测到僵尸锁文件（>2h），强制清除")

        try:
            self.lock_file.write_text(str(os.getpid()))
            self._ensure_initialized()
            self._flow_git_to_p4()
            self._flow_p4_to_git()
        except Exception as e:
            self.log.error(f"同步异常: {e}", exc_info=True)
        finally:
            if self.lock_file.exists():
                self.lock_file.unlink()

    def run_loop(self):
        """持续循环同步"""
        interval = self.config.get("poll_interval_minutes", 10)
        self.log.info("=" * 50)
        self.log.info(f"SyncBridge v{VERSION} 启动")
        self.log.info(f"  Git:  {self.config['git']['repo_path']}")
        self.log.info(f"  P4:   {self.config['p4']['workspace_path']}")
        self.log.info(f"  间隔: {interval} 分钟")
        self.log.info("=" * 50)

        while True:
            try:
                self.run_once()
            except KeyboardInterrupt:
                self.log.info("用户中断，退出")
                break
            self.log.info(f"等待 {interval} 分钟...\n")
            try:
                time.sleep(interval * 60)
            except KeyboardInterrupt:
                self.log.info("用户中断，退出")
                break


def main():
    parser = argparse.ArgumentParser(description="SyncBridge - Git 与 P4 双向同步工具")
    parser.add_argument("-c", "--config", default="config.json", help="配置文件路径")
    parser.add_argument("--once", action="store_true", help="只执行一次同步（不循环）")
    parser.add_argument("--compile-sync", action="store_true", help="编译后同步到 P4（不循环）")
    args = parser.parse_args()

    bridge = SyncBridge(args.config)
    if args.compile_sync:
        sys.exit(0 if bridge.compile_and_sync() else 1)
    elif args.once:
        bridge.run_once()
    else:
        bridge.run_loop()


if __name__ == "__main__":
    main()