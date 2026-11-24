#!/usr/bin/env python3
"""Dev01's SSH front end to its single cloud synchronization worker."""

from __future__ import annotations

import argparse
import base64
from dataclasses import dataclass, fields
import json
from pathlib import Path, PureWindowsPath
import re
import shutil
import subprocess
import sys
import time
from typing import Any, Mapping, Sequence


DEFAULT_WORKER_ROOT = r"C:\Project\Dev01-GitToP4-Auto-20261024\v3-worker"
DEFAULT_NODE = r"C:\Project\Dev01-GitToP4-Tools-20261023\node.exe"
SENSITIVE_KEY = re.compile(
    r"password|passwd|secret|token|ticket|credential|authorization|private.?key", re.I
)
PS_PREFIX = (
    "$ProgressPreference='SilentlyContinue';"
    "[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);"
    "$OutputEncoding=[Console]::OutputEncoding;"
)


def _windows_path(value: str, name: str, parent: str) -> str:
    if not isinstance(value, str) or any(ord(c) < 32 for c in value):
        raise ValueError(f"Invalid {name} path.")
    path = PureWindowsPath(value)
    if not re.fullmatch(r"[A-Za-z]:", path.drive) or not path.is_absolute():
        raise ValueError(f"{name} must be an absolute Windows drive path.")
    # Reject aliases, quoting characters and shell metacharacters before encoding.
    for part in value.replace("/", "\\").split("\\")[1:]:
        if (not part or part in {".", ".."} or part.endswith((".", " "))
                or not re.fullmatch(r"[A-Za-z0-9_-][A-Za-z0-9 _.-]*", part)):
            raise ValueError(f"Invalid {name} path segment.")
    normalized = str(path)
    if not normalized.casefold().startswith(parent.casefold() + "\\"):
        raise ValueError(f"{name} must remain under {parent}.")
    return normalized


@dataclass(frozen=True)
class Config:
    schema_version: int = 1
    host: str = "124.223.187.156"
    user: str = "Administrator"
    port: int = 22
    identity_file: str = "~/.ssh/dev01_cloud_ed25519"
    worker_root: str = DEFAULT_WORKER_ROOT
    node_executable: str = DEFAULT_NODE
    git_root: str = r"C:\Project\Dev01-GitHub"
    git_http_proxy: str | None = "http://127.0.0.1:7890"
    state_root: str = r"C:\BuildAgent\Dev01\state\git-to-p4-v3"
    timeout_seconds: int = 1800

    @classmethod
    def from_mapping(cls, value: Mapping[str, Any]) -> "Config":
        if not isinstance(value, Mapping):
            raise ValueError("Configuration must be a JSON object.")
        if set(value) - {f.name for f in fields(cls)}:
            raise ValueError("Configuration contains unsupported fields; commands and secrets are not accepted.")
        result = cls(**value)
        if type(result.schema_version) is not int or result.schema_version != 1:
            raise ValueError("Unsupported configuration schema.")
        if not isinstance(result.host, str) or not re.fullmatch(
            r"[A-Za-z0-9](?:[A-Za-z0-9.-]{0,251}[A-Za-z0-9])?", result.host
        ) or ".." in result.host:
            raise ValueError("host must be an IPv4 address or DNS name without options or shell text.")
        if not isinstance(result.user, str) or not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]*", result.user):
            raise ValueError("Invalid SSH user.")
        if type(result.port) is not int or not 1 <= result.port <= 65535:
            raise ValueError("SSH port must be an integer from 1 to 65535.")
        if type(result.timeout_seconds) is not int or not 5 <= result.timeout_seconds <= 7200:
            raise ValueError("timeout_seconds must be an integer from 5 to 7200.")
        if (not isinstance(result.identity_file, str) or not result.identity_file
                or any(ord(c) < 32 for c in result.identity_file)):
            raise ValueError("Invalid SSH identity path.")
        identity = Path(result.identity_file).expanduser()
        if not identity.is_absolute() or identity.name.lower().endswith(".pub"):
            raise ValueError("identity_file must name an absolute or ~/ private-key file path.")
        _windows_path(result.worker_root, "worker_root", r"C:\Project")
        node = _windows_path(result.node_executable, "node_executable", r"C:\Project")
        if PureWindowsPath(node).name.casefold() != "node.exe":
            raise ValueError("node_executable must name node.exe, without arguments.")
        _windows_path(result.git_root, "git_root", r"C:\Project")
        if result.git_http_proxy is not None:
            if not isinstance(result.git_http_proxy, str):
                raise ValueError("git_http_proxy must be a loopback HTTP URL or null.")
            proxy = re.fullmatch(r"http://(?:127\.0\.0\.1|localhost):([0-9]{1,5})",
                                 result.git_http_proxy, flags=re.I)
            if proxy is None or not 1 <= int(proxy.group(1)) <= 65535:
                raise ValueError("git_http_proxy requires http://127.0.0.1 or localhost and an explicit valid port.")
        _windows_path(result.state_root, "state_root", r"C:\BuildAgent\Dev01\state")
        return result

    @property
    def identity(self) -> Path:
        return Path(self.identity_file).expanduser()


def load_config(path: str | None = None) -> Config:
    candidate = Path(path).expanduser() if path else Path(__file__).with_name("config.json")
    if path is None and not candidate.exists():
        return Config.from_mapping({})
    try:
        value = json.loads(candidate.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError) as exc:
        raise ValueError("Cannot read a valid JSON configuration.") from exc
    return Config.from_mapping(value)


def _redact_value(value: Any) -> Any:
    if isinstance(value, dict):
        return {k: "[withheld]" if SENSITIVE_KEY.search(str(k)) else _redact_value(v)
                for k, v in value.items()}
    if isinstance(value, list):
        return [_redact_value(item) for item in value]
    if isinstance(value, str):
        return _redact_plain(value)
    return value


def _redact_plain(text: str) -> str:
    text = re.sub(r"-----BEGIN [^-]*PRIVATE KEY-----.*?-----END [^-]*PRIVATE KEY-----",
                  "[private key withheld]", text, flags=re.S)
    text = re.sub(r"(?i)(https?://)[^/@\s]+@", r"\1[withheld]@", text)
    text = re.sub(r"(?i)Bearer\s+[^\s\"']+", "Bearer [withheld]", text)
    return "\n".join("[credential-related output withheld]" if SENSITIVE_KEY.search(line)
                     else line for line in text.split("\n"))


def redact_output(text: str) -> str:
    """Keep structured operational fields while withholding credential values."""
    text = re.sub(r"-----BEGIN [^-]*PRIVATE KEY-----.*?(?:-----END [^-]*PRIVATE KEY-----|$)",
                  "[private key withheld]", text, flags=re.S)
    output = []
    for line in text.splitlines(keepends=True):
        try:
            value = json.loads(line)
        except ValueError:
            output.append(_redact_plain(line))
        else:
            output.append(json.dumps(_redact_value(value), ensure_ascii=False) + "\n")
    return "".join(output)


def _literal(value: str) -> str:
    # Values already passed path validation; quoting remains explicit.
    return "'" + value.replace("'", "''") + "'"


def remote_script(operation: str, config: Config) -> str:
    worker = str(PureWindowsPath(config.worker_root) / "auto-sync.mjs")
    wrapper = str(PureWindowsPath(config.worker_root) / "run-scheduled.ps1")
    if operation == "preview":
        return (PS_PREFIX + "$ErrorActionPreference='Stop';"
                f"if(-not(Test-Path -LiteralPath {_literal(wrapper)} -PathType Leaf)){{exit 127}};"
                f"& {_literal(wrapper)} -DryRun;exit $LASTEXITCODE")
    if operation == "once":
        # Bypass the notification wrapper, never the worker's own guards.
        return (PS_PREFIX +
                f"if(-not(Test-Path -LiteralPath {_literal(config.node_executable)} -PathType Leaf)"
                f" -or -not(Test-Path -LiteralPath {_literal(worker)} -PathType Leaf)){{exit 127}};"
                "$ErrorActionPreference='Continue';"
                f"& {_literal(config.node_executable)} {_literal(worker)} '--apply';"
                "$code=$LASTEXITCODE;if($null -eq $code){exit 127};exit $code")
    if operation == "check":
        return (PS_PREFIX + "$ErrorActionPreference='Stop';try{$r=[ordered]@{"
                "Identity=((& whoami.exe)-join '');Host=((& hostname.exe)-join '');"
                f"Worker=(Test-Path -LiteralPath {_literal(worker)} -PathType Leaf);"
                f"Wrapper=(Test-Path -LiteralPath {_literal(wrapper)} -PathType Leaf);"
                f"Node=(Test-Path -LiteralPath {_literal(config.node_executable)} -PathType Leaf);"
                f"GitRoot=(Test-Path -LiteralPath {_literal(config.git_root)} -PathType Container);"
                f"StateRoot=(Test-Path -LiteralPath {_literal(config.state_root)} -PathType Container)"
                "};$r|ConvertTo-Json -Compress;"
                "if($r.Worker -and $r.Wrapper -and $r.Node -and $r.GitRoot -and $r.StateRoot){exit 0};exit 2"
                "}catch{@{Error='RemoteCheckFailed';Type=$_.Exception.GetType().Name}|ConvertTo-Json -Compress;exit 1}")
    if operation != "status":
        raise ValueError("Unsupported operation.")
    git = "& git -C $g -c " + _literal("http.proxy=" + (config.git_http_proxy or ""))
    # Fixed commands and selected state fields only. No fetch, journal write,
    # task start, P4 command, or credential-file read is part of this probe.
    return (PS_PREFIX + "$ErrorActionPreference='Stop';try{$code=0;"
            f"$g={_literal(config.git_root)};$s={_literal(config.state_root)};"
            "$env:GIT_TERMINAL_PROMPT='0';$env:GCM_INTERACTIVE='Never';"
            f"$h=(({git} rev-parse HEAD 2>$null)-join '').Trim();if($LASTEXITCODE){{$code=1}};"
            f"$d=@({git} status --porcelain=v1 2>$null);if($LASTEXITCODE){{$code=1}};"
            f"$o=(({git} remote get-url origin 2>$null)-join '').Trim();if($LASTEXITCODE){{$code=1}};"
            f"$a=(({git} ls-remote origin refs/heads/main 2>$null)-join '').Trim();"
            "$remote=$null;if($LASTEXITCODE -eq 0 -and $a -match '^([0-9a-f]{40})\\s'){$remote=$Matches[1]}else{$code=1};"
            "$r=[ordered]@{Utc=[DateTime]::UtcNow.ToString('o');"
            "Git=@{Head=$h;AdvertisedMain=$remote;MatchesRemote=($h -eq $remote);DirtyCount=$d.Count;"
            "OriginMatchesExpected=($o -eq 'https://github.com/woshisck/Dev01.git')};"
            "ActiveTransaction=(Test-Path -LiteralPath ($s+'\\active.json'))};"
            "foreach($n in @('checkpoint','status')){$p=$s+'\\'+$n+'.json';"
            "if(Test-Path -LiteralPath $p){$r[$n]=Get-Content -LiteralPath $p -Raw -Encoding UTF8|ConvertFrom-Json|"
            "Select-Object schema,status,reason,mode,timestamp,gitCommit,p4Change,codeCL,engineCL,buildId,archiveRevision,archiveChange,lastDisposition,updatedAt,WritesToGitOrP4}"
            "else{$r[$n]=$null;$code=1}};"
            "$r.Tasks=@(foreach($n in @('Dev01 Code Mirror','Dev01 UGS PCB Build')){try{"
            "$t=Get-ScheduledTask -TaskName $n -ErrorAction Stop;$i=Get-ScheduledTaskInfo -TaskName $n -ErrorAction Stop;"
            "@{Name=$n;State=[string]$t.State;Enabled=$t.Settings.Enabled;LastResult=$i.LastTaskResult;"
            "LastRun=$i.LastRunTime.ToString('o');NextRun=$i.NextRunTime.ToString('o')}}"
            "catch{$code=1;@{Name=$n;ReadError=$_.Exception.GetType().Name}}});"
            "$r|ConvertTo-Json -Depth 6 -Compress;exit $code"
            "}catch{@{Error='RemoteStatusFailed';Type=$_.Exception.GetType().Name}|ConvertTo-Json -Compress;exit 1}")


class CloudClient:
    def __init__(self, config: Config):
        self.config = Config.from_mapping({field.name: getattr(config, field.name) for field in fields(Config)})

    def argv(self, operation: str) -> list[str]:
        ssh = shutil.which("ssh")
        if not ssh:
            raise ValueError("OpenSSH ssh is not available on PATH.")
        if not self.config.identity.is_file():
            raise ValueError("The configured SSH identity file does not exist.")
        script = remote_script(operation, self.config)
        encoded = base64.b64encode(script.encode("utf-16-le")).decode("ascii")
        command = "powershell.exe -NoLogo -NoProfile -NonInteractive -OutputFormat Text -EncodedCommand " + encoded
        if len(command) > 7800:
            raise ValueError("Remote probe exceeds the Windows command-line limit.")
        return [ssh, "-i", str(self.config.identity), "-p", str(self.config.port),
                "-o", "BatchMode=yes", "-o", "IdentitiesOnly=yes",
                "-o", "StrictHostKeyChecking=yes", "-o", "ConnectTimeout=15",
                f"{self.config.user}@{self.config.host}", command]

    def call(self, operation: str) -> int:
        argv = self.argv(operation)
        attempts = 3 if operation in {"check", "status"} else 1
        for attempt in range(attempts):
            try:
                result = subprocess.run(argv, shell=False, capture_output=True, text=True,
                                        encoding="utf-8", errors="replace",
                                        timeout=min(self.config.timeout_seconds, 120)
                                        if operation in {"check", "status"} else self.config.timeout_seconds)
            except subprocess.TimeoutExpired:
                print("SSH request timed out; inspect cloud status before retrying a write.", file=sys.stderr)
                return 124
            except OSError:
                print("Unable to start the SSH client.", file=sys.stderr)
                return 127
            preauthentication_failure = (result.returncode == 255 and not result.stdout.strip()
                and re.search(r"(?im)^(?:(?:kex_exchange_identification:|banner exchange:)[^\r\n]*"
                              r"(?:Connection reset|Connection refused|Connection timed out|Exceeded MaxStartups)|"
                              r"ssh: connect to host [^\r\n]+ port \d+: Connection (?:refused|timed out)"
                              r"|Connection timed out during banner exchange)", result.stderr))
            if attempt + 1 < attempts and preauthentication_failure:
                print(f"SSH failed before authentication; retrying read-only probe ({attempt + 2}/{attempts}).",
                      file=sys.stderr)
                time.sleep(2)
                continue
            break
        sys.stdout.write(redact_output(result.stdout))
        sys.stderr.write(redact_output(result.stderr))
        return result.returncode

    def observe(self, interval: int, count: int) -> int:
        completed = 0
        while count == 0 or completed < count:
            code = self.call("status")
            if code:
                return code
            completed += 1
            if count == 0 or completed < count:
                time.sleep(interval)
        return 0


def _positive(value: str) -> int:
    number = int(value)
    if not 1 <= number <= 3600:
        raise argparse.ArgumentTypeError("Value must be from 1 to 3600.")
    return number


def _count(value: str) -> int:
    number = int(value)
    if number < 0:
        raise argparse.ArgumentTypeError("count must be nonnegative; 0 observes until Ctrl+C.")
    return number


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="Dev01 cloud sync front end. One cloud worker owns all Git/P4 writes.",
        epilog="No force mode, local mirror, binary submit, or notification sender is included.",
    )
    result.add_argument("-c", "--config", help="Optional nonsecret JSON configuration.")
    commands = result.add_subparsers(dest="command")
    commands.add_parser("check", help="Check SSH identity and required cloud paths.")
    commands.add_parser("status", help="Read cloud Git, advertised main, checkpoint and tasks.")
    commands.add_parser("preview", help="Run the official cloud wrapper with -DryRun.")
    commands.add_parser("once", help="Run the guarded cloud worker once with --apply.")
    watch = commands.add_parser("run", help="Observe existing cloud tasks; never start a sync loop.")
    watch.add_argument("--interval", type=_positive, default=30, help="Seconds between reads (default: 30).")
    watch.add_argument("--count", type=_count, default=0, help="Read count; 0 means until Ctrl+C.")
    return result


def menu() -> str | None:
    print("Dev01 云同步：1 检查连接  2 状态  3 预检  4 执行一次  5 只读观察  q 退出")
    return {"1": "check", "2": "status", "3": "preview", "4": "once", "5": "run"}.get(input("> ").strip().lower())


def main(argv: Sequence[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        command = args.command or menu()
        if command is None:
            return 0
        client = CloudClient(load_config(args.config))
        if command == "run":
            return client.observe(getattr(args, "interval", 30), getattr(args, "count", 0))
        return client.call(command)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2
    except OSError:
        print("Unavailable local configuration or SSH prerequisite.", file=sys.stderr)
        return 2
    except (KeyboardInterrupt, EOFError):
        return 130


if __name__ == "__main__":
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    raise SystemExit(main())
