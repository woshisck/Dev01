#!/usr/bin/env python3
"""
SyncBridge - Git ↔ P4 双向同步 + UE5 自动编译
独立开发者用，简单可靠。

用法:
  python sync_bridge.py                # 持续循环同步
  python sync_bridge.py --once         # 只执行一次
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
import argparse
from pathlib import Path
from datetime import datetime

SYNC_MARKER = "[SyncBridge]"
VERSION = "1.0.0"


class SyncBridge:
    def __init__(self, config_path):
        self.base_dir = Path(__file__).parent
        self.config = self._load_json(config_path)
        self.state_file = self.base_dir / "sync_state.json"
        self.lock_file = self.base_dir / ".sync_lock"
        self.state = self._load_json(self.state_file, default={})
        self._setup_logging()

    # ── 基础工具 ──────────────────────────────────

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
        logging.basicConfig(
            level=level,
            format=fmt,
            datefmt="%Y-%m-%d %H:%M:%S",
            handlers=[
                logging.FileHandler(log_file, encoding="utf-8"),
                logging.StreamHandler(sys.stdout),
            ],
        )
        self.log = logging.getLogger("SyncBridge")

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
            out = r.stdout or ""
            err = r.stderr or ""
            if r.returncode != 0:
                self.log.debug(f"CMD 返回码 {r.returncode}")
                self.log.debug(f"stderr: {err[:500]}")
                return False, err + out
            return True, out
        except subprocess.TimeoutExpired:
            self.log.error(f"CMD 超时({timeout}s): {cmd}")
            return False, "TIMEOUT"
        except Exception as e:
            self.log.error(f"CMD 异常: {e}")
            return False, str(e)

    def _should_ignore(self, relpath):
        """检查文件是否在忽略列表中"""
        for pat in self.config.get("ignore_patterns", []):
            clean = pat.rstrip("/*")
            if relpath == clean or relpath.startswith(clean + "/"):
                return True
            if clean in Path(relpath).parts:
                return True
        return False

    # ── 首次运行初始化 ────────────────────────────

    def _ensure_initialized(self):
        """首次运行时，记录当前 Git/P4 位置，避免全量同步"""
        if self.state.get("initialized"):
            return

        self.log.info("=" * 50)
        self.log.info("首次运行，正在初始化同步基线...")

        # 记录 Git 当前 HEAD
        ok, h = self._git("rev-parse HEAD")
        if ok:
            self.state["last_git_commit"] = h.strip()

        # 记录 P4 当前最新 CL
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
        self.log.info("下一次轮询开始检测新变更。")
        self.log.info("=" * 50)

    # ── Git 操作 ──────────────────────────────────

    def _git(self, subcmd, timeout=120):
        return self._run(f"git {subcmd}", cwd=self.config["git"]["repo_path"], timeout=timeout)

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

        if local.strip() == remote_h.strip():
            return False

        # 反循环：如果远端最新提交是我们自己的同步提交，跳过
        ok, msg = self._git(f"log {remote}/{branch} -1 --format=%s")
        if ok and SYNC_MARKER in msg:
            self.log.info("Git: 远端最新是同步提交，拉取但跳过反向同步")
            self._git(f"pull {remote} {branch}")
            self.state["last_git_commit"] = remote_h.strip()
            self._save_state()
            return False

        self.log.info(f"Git: 检测到远端更新 → {remote_h.strip()[:8]}")
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

        ok, _ = self._git(f"pull {remote} {branch}")
        if not ok:
            self.log.error("git pull 失败！")
            return []

        ok, after = self._git("rev-parse HEAD")
        if not ok:
            return []
        after = after.strip()

        ok, diff = self._git(f"diff --name-only {before} {after}")
        changed = [f for f in (diff or "").strip().splitlines() if f and not self._should_ignore(f)]

        self.state["last_git_commit"] = after
        self._save_state()
        self.log.info(f"git pull 完成: {len(changed)} 个文件变更")
        return changed

    def _git_commit_push(self, files, desc):
        """提交并推送到 GitHub"""
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
        ok, _ = self._git(f'commit -m "{msg}"')
        if not ok:
            return False

        ok, _ = self._git(f"push {remote} {branch}")
        if not ok:
            self.log.error("git push 失败！")
            return False

        ok, h = self._git("rev-parse HEAD")
        if ok:
            self.state["last_git_commit"] = h.strip()
            self._save_state()

        self.log.info("git push 完成")
        return True

    # ── P4 操作 ───────────────────────────────────

    def _p4(self, subcmd, timeout=120):
        """执行 p4 命令（自动注入连接参数）"""
        cfg = self.config["p4"]
        env = os.environ.copy()
        env["P4PORT"] = cfg["port"]
        env["P4USER"] = cfg["user"]
        env["P4CLIENT"] = cfg["workspace"]
        if cfg.get("password"):
            env["P4PASSWD"] = cfg["password"]
        if cfg.get("charset"):
            env["P4CHARSET"] = cfg["charset"]
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

        # 反循环检查
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
        if not (out or "").strip():
            return []

        # 解析 p4 sync 输出，提取本地文件路径
        # 格式: //depot/path#rev - updating D:\local\path
        #   或: //depot/path#rev - added as D:\local\path
        changed = []
        for line in (out or "").strip().splitlines():
            m = re.search(r" - \w+(?: as)? (.+)$", line.strip())
            if m:
                local = m.group(1).strip()
                try:
                    rel = str(Path(local).relative_to(ws_path)).replace("\\", "/")
                    if not self._should_ignore(rel):
                        changed.append(rel)
                except ValueError:
                    pass

        # 更新 CL 状态
        depot = cfg.get("depot_path", "//...")
        ok2, cl_out = self._p4(f"changes -m1 -s submitted {depot}")
        if ok2 and (cl_out or "").strip():
            try:
                self.state["last_p4_changelist"] = int(cl_out.strip().split()[1])
                self._save_state()
            except (IndexError, ValueError):
                pass

        self.log.info(f"p4 sync 完成: {len(changed)} 个文件")
        return changed

    def _p4_reconcile_submit(self, files, desc):
        """将变更提交到 P4"""
        cfg = self.config["p4"]
        ws = cfg["workspace_path"]

        # 逐文件 reconcile（更安全，不会误提交无关文件）
        reconcile_errors = []
        for f in files:
            fpath = str(Path(ws) / f.replace("/", os.sep))
            ok, out = self._p4(f'reconcile "{fpath}"')
            if not ok and "no file(s) to reconcile" not in (out or "").lower():
                reconcile_errors.append(f"  {f}: {out.strip()[:200]}")
            elif ok and (out or "").strip():
                self.log.info(f"P4 reconcile: {out.strip()}")

        if reconcile_errors:
            self.log.warning(f"P4 reconcile 部分文件失败:\n" + "\n".join(reconcile_errors))

        # 检查是否有文件被打开
        ok, opened = self._p4("opened")
        self.log.info(f"P4 opened 状态: {(opened or '').strip()[:500]}")
        opened_text = (opened or "").strip().lower()
        if not opened_text or "not opened" in opened_text or "file(s) not opened" in opened_text:
            self.log.info("P4: 无变更需要提交")
            return True

        msg = f"{SYNC_MARKER} {desc}"
        ok, out = self._p4(f'submit -d "{msg}"', timeout=300)
        if not ok:
            self.log.error(f"p4 submit 失败！错误信息:\n{(out or '').strip()}")
            return False

        # 更新 CL 状态
        depot = cfg.get("depot_path", "//...")
        ok, cl_out = self._p4(f"changes -m1 -s submitted {depot}")
        if ok and (cl_out or "").strip():
            try:
                self.state["last_p4_changelist"] = int(cl_out.strip().split()[1])
                self._save_state()
            except (IndexError, ValueError):
                pass

        self.log.info("p4 submit 完成")
        return True

    # ── UE5 编译 ──────────────────────────────────

    def _needs_compile(self, files):
        """检查变更文件是否包含需要编译的源码"""
        ue = self.config.get("ue5", {})
        if not ue.get("enabled"):
            return False
        triggers = ue.get("compile_triggers", [".cpp", ".h", ".cs"])
        return any(any(f.endswith(ext) for ext in triggers) for f in files)

    def _compile_ue5(self):
        """调用 UnrealBuildTool 编译项目"""
        ue = self.config["ue5"]
        engine = ue["engine_path"]

        # 构建编译命令
        dotnet_ver = ue.get("dotnet_version", "6.0.302")
        dotnet_dir = os.path.join(engine, "Engine", "Binaries", "ThirdParty", "DotNet", dotnet_ver, "windows")
        ubt = os.path.join(engine, "Engine", "Binaries", "DotNET", "UnrealBuildTool", "UnrealBuildTool.dll")
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
        else:
            self.log.error("UE5 编译失败！请检查日志。")
        return ok

    # ── 文件同步 ──────────────────────────────────

    def _sync_files(self, src_root, dst_root, files):
        """将变更文件从源目录同步到目标目录（支持增删改）"""
        src = Path(src_root)
        dst = Path(dst_root)
        copied, deleted = 0, 0

        for f in files:
            s = src / f
            d = dst / f
            if s.exists():
                d.parent.mkdir(parents=True, exist_ok=True)
                # P4 文件默认只读，复制前先解除只读属性
                if d.exists():
                    d.chmod(stat.S_IWRITE | stat.S_IREAD)
                shutil.copy2(s, d)
                copied += 1
            else:
                # 源文件不存在 = 被删除了
                if d.exists():
                    d.chmod(stat.S_IWRITE | stat.S_IREAD)
                    d.unlink()
                    deleted += 1

        self.log.info(f"文件同步: 复制 {copied}, 删除 {deleted} ({src_root} → {dst_root})")

    # ── 主流程 ────────────────────────────────────

    def _flow_git_to_p4(self):
        """Git → P4 方向同步"""
        self.log.info("── 检查 Git → P4 ──")

        if not self._git_has_updates():
            return

        # 拉取变更
        changed = self._git_pull()
        if not changed:
            return

        self.log.info(f"Git 变更文件: {', '.join(changed[:10])}{'...' if len(changed) > 10 else ''}")

        # 源码变更时编译 UE5
        if self._needs_compile(changed):
            if not self._compile_ue5():
                self.log.error("编译失败，本轮跳过 P4 同步")
                return
            # 把编译产物也加入同步列表
            bin_dir = Path(self.config["git"]["repo_path"]) / "Binaries"
            if bin_dir.exists():
                for bf in bin_dir.rglob("*"):
                    if bf.is_file():
                        rel = str(bf.relative_to(Path(self.config["git"]["repo_path"]))).replace("\\", "/")
                        if rel not in changed:
                            changed.append(rel)

        # 同步文件到 P4 工作区
        self._sync_files(
            self.config["git"]["repo_path"],
            self.config["p4"]["workspace_path"],
            changed,
        )

        # 提交到 P4
        self._p4_reconcile_submit(changed, f"Git同步 {len(changed)}文件")

    def _flow_p4_to_git(self):
        """P4 → Git 方向同步"""
        self.log.info("── 检查 P4 → Git ──")

        if not self._p4_has_updates():
            return

        # 同步 P4
        changed = self._p4_sync_and_get_changes()
        if not changed:
            return

        self.log.info(f"P4 变更文件: {', '.join(changed[:10])}{'...' if len(changed) > 10 else ''}")

        # 同步文件到 Git 目录
        self._sync_files(
            self.config["p4"]["workspace_path"],
            self.config["git"]["repo_path"],
            changed,
        )

        # 提交并推送到 GitHub
        self._git_commit_push(changed, f"P4同步 {len(changed)}文件")

    def run_once(self):
        """执行一轮同步"""
        if self.lock_file.exists():
            # 检查锁文件是否过期（超过 2 小时视为僵尸锁）
            age = time.time() - self.lock_file.stat().st_mtime
            if age < 7200:
                self.log.warning("另一个同步进程正在运行，跳过")
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
        self.log.info(f"  冲突: Git 为主")
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
    parser.add_argument("-c", "--config", default="config.json", help="配置文件路径 (默认: config.json)")
    parser.add_argument("--once", action="store_true", help="只执行一次同步（不循环）")
    args = parser.parse_args()

    bridge = SyncBridge(args.config)
    if args.once:
        bridge.run_once()
    else:
        bridge.run_loop()


if __name__ == "__main__":
    main()
