import base64
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import syncbridge


class SyncBridgeTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.key = Path(self.directory.name) / "identity"
        self.key.write_text("test placeholder", encoding="utf-8")
        self.config = syncbridge.Config.from_mapping({"identity_file": str(self.key)})

    def script_from(self, argv):
        encoded = argv[-1].rsplit(" ", 1)[1]
        return base64.b64decode(encoded).decode("utf-16-le")

    def client_call(self, operation, returncode=0, stdout="", stderr=""):
        result = subprocess.CompletedProcess([], returncode, stdout, stderr)
        with patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
            syncbridge.subprocess, "run", return_value=result
        ) as run, redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            code = syncbridge.CloudClient(self.config).call(operation)
        return code, run.call_args

    def test_preview_uses_official_dry_run_and_no_local_source_writes(self):
        source = Path(self.directory.name) / "source"
        source.mkdir()
        (source / "source.cpp").write_text("preserve source\n", encoding="utf-8")
        before = {str(path): path.read_bytes() for path in Path(self.directory.name).rglob("*") if path.is_file()}
        code, call = self.client_call("preview", returncode=2)
        self.assertEqual(code, 2)
        self.assertFalse(call.kwargs["shell"])
        self.assertEqual(call.kwargs["encoding"], "utf-8")
        argv = call.args[0]
        script = self.script_from(argv)
        self.assertIn("run-scheduled.ps1' -DryRun", script)
        for forbidden in ("--apply", "Copy-Item", "Remove-Item", "Set-Content", "p4 ", "git "):
            self.assertNotIn(forbidden, script)
        after = {str(path): path.read_bytes() for path in Path(self.directory.name).rglob("*") if path.is_file()}
        self.assertEqual(before, after)
        self.assertIn("StrictHostKeyChecking=yes", argv)
        self.assertIn("IdentitiesOnly=yes", argv)

    def test_once_calls_only_guarded_node_worker_and_propagates_exit(self):
        code, call = self.client_call("once", returncode=37)
        self.assertEqual(code, 37)
        script = self.script_from(call.args[0])
        self.assertIn("node.exe'", script)
        self.assertIn("auto-sync.mjs' '--apply'", script)
        self.assertIn("exit $code", script)
        self.assertNotIn("run-scheduled.ps1", script)
        self.assertNotIn("Feishu", script)
        self.assertNotIn("force", script)

    def test_status_reads_both_heads_and_tasks_without_fetch_or_start(self):
        code, call = self.client_call("status")
        self.assertEqual(code, 0)
        script = self.script_from(call.args[0])
        self.assertIn("rev-parse HEAD", script)
        self.assertIn("ls-remote origin refs/heads/main", script)
        self.assertIn("Get-ScheduledTask", script)
        self.assertIn("checkpoint", script)
        self.assertEqual(script.count("-c 'http.proxy=http://127.0.0.1:7890'"), 4)
        self.assertNotIn("git config", script)
        self.assertIn("$s+'\\active.json'", script)
        self.assertIn("$p=$s+'\\'+$n+'.json'", script)
        for forbidden in (" fetch ", " pull ", "--apply", "Start-ScheduledTask", "Set-Content", "p4 "):
            self.assertNotIn(forbidden, script)
        self.assertLess(len(call.args[0][-1]), 7801)

    def test_observer_only_reads_status(self):
        client = syncbridge.CloudClient(self.config)
        with patch.object(client, "call", return_value=0) as call, patch.object(syncbridge.time, "sleep") as sleep:
            self.assertEqual(client.observe(30, 2), 0)
        self.assertEqual([x.args for x in call.call_args_list], [("status",), ("status",)])
        sleep.assert_called_once_with(30)

    def test_observer_propagates_read_failure(self):
        with patch.object(syncbridge.CloudClient, "call", return_value=255):
            self.assertEqual(syncbridge.CloudClient(self.config).observe(30, 2), 255)

    def test_cli_propagates_cloud_exit(self):
        with patch.object(syncbridge, "load_config", return_value=self.config), patch.object(
            syncbridge.CloudClient, "call", return_value=23
        ) as call:
            self.assertEqual(syncbridge.main(["once"]), 23)
        call.assert_called_once_with("once")

    def test_no_force_command_or_shell_override(self):
        with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as exc:
            syncbridge.main(["force", "--yes"])
        self.assertEqual(exc.exception.code, 2)
        for mapping in ({"remote_command": "whoami"}, {"password": "not accepted"}, {"ssh_executable": "cmd.exe"}):
            with self.subTest(mapping=list(mapping)), self.assertRaises(ValueError):
                syncbridge.Config.from_mapping(mapping)

    def test_config_rejects_injected_hosts_users_and_paths(self):
        bad = [
            {"host": "host; whoami"}, {"host": "-oProxyCommand=bad"},
            {"host": "host\nother"}, {"user": "Administrator & whoami"}, {"user": "-v"},
            {"node_executable": r"C:\Project\node.exe --eval bad"},
            {"worker_root": r"C:\Project\..\Windows"},
            {"worker_root": r"C:\Project\worker';whoami;#"},
            {"worker_root": r"\\host\share\worker"},
            {"worker_root": r"C:\Windows\worker"},
            {"state_root": r"C:\BuildAgent\Dev01\state\..\secrets"},
            {"port": True}, {"timeout_seconds": "300"},
            {"identity_file": "relative-key"}, {"identity_file": str(self.key) + ".pub"},
        ]
        for mapping in bad:
            with self.subTest(mapping=list(mapping)), self.assertRaises(ValueError):
                syncbridge.Config.from_mapping(mapping)

    def test_config_example_and_config_file_parse_without_execution(self):
        source = Path(syncbridge.__file__).with_name("config.example.json")
        config = syncbridge.Config.from_mapping(json.loads(source.read_text("utf-8")))
        self.assertEqual(config.worker_root, syncbridge.DEFAULT_WORKER_ROOT)
        path = Path(self.directory.name) / "config.json"
        path.write_text(json.dumps({"identity_file": str(self.key)}), encoding="utf-8")
        with patch.object(syncbridge.subprocess, "run") as run:
            self.assertEqual(syncbridge.load_config(str(path)).identity, self.key)
        run.assert_not_called()

    def test_git_proxy_accepts_only_explicit_loopback_http_ports_or_null(self):
        for proxy in ("http://127.0.0.1:7890", "http://localhost:8080", None):
            with self.subTest(proxy=proxy):
                config = syncbridge.Config.from_mapping({"git_http_proxy": proxy})
                script = syncbridge.remote_script("status", config)
                self.assertEqual(script.count("-c 'http.proxy=" + (proxy or "") + "'"), 4)
        for proxy in ("http://proxy.example:7890", "http://127.0.0.1", "https://127.0.0.1:7890",
                      "http://user:pass@127.0.0.1:7890", "http://127.0.0.1:7890/",
                      "http://localhost:7890?token=value", "http://localhost:0",
                      "http://localhost:65536", "http://localhost:7890';whoami;#", False):
            with self.subTest(proxy_type=type(proxy).__name__), self.assertRaises(ValueError):
                syncbridge.Config.from_mapping({"git_http_proxy": proxy})

    def test_read_only_probe_retries_only_clear_preauthentication_failures_up_to_three_times(self):
        failed = subprocess.CompletedProcess([], 255, "", "kex_exchange_identification: read: Connection reset by peer\n")
        passed = subprocess.CompletedProcess([], 0, '{"status":"ok"}\n', "")
        with patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
            syncbridge.subprocess, "run", side_effect=[failed, failed, passed]
        ) as run, patch.object(syncbridge.time, "sleep") as sleep, redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            self.assertEqual(syncbridge.CloudClient(self.config).call("status"), 0)
        self.assertEqual(run.call_count, 3)
        self.assertEqual([call.args for call in sleep.call_args_list], [(2,), (2,)])

    def test_read_only_retry_stops_after_third_failure(self):
        failed = subprocess.CompletedProcess([], 255, "", "ssh: connect to host 127.0.0.1 port 22: Connection refused\n")
        with patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
            syncbridge.subprocess, "run", return_value=failed
        ) as run, patch.object(syncbridge.time, "sleep"), redirect_stderr(io.StringIO()):
            self.assertEqual(syncbridge.CloudClient(self.config).call("check"), 255)
        self.assertEqual(run.call_count, 3)

    def test_never_retries_writes_authentication_failure_or_unknown_disconnect(self):
        cases = [("once", "", "kex_exchange_identification: read: Connection reset by peer\n"),
                 ("preview", "", "banner exchange: Connection reset by peer\n"),
                 ("status", "", "Permission denied (publickey).\n"),
                 ("status", "", "kex_exchange_identification: invalid protocol identifier\n"),
                 ("status", "", "Connection to server closed.\n"),
                 ("status", '{"Status":"started"}\n', "kex_exchange_identification: read: Connection reset by peer\n")]
        for operation, stdout, stderr in cases:
            with self.subTest(operation=operation, stdout=bool(stdout)):
                failed = subprocess.CompletedProcess([], 255, stdout, stderr)
                with patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
                    syncbridge.subprocess, "run", return_value=failed
                ) as run, patch.object(syncbridge.time, "sleep") as sleep, redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                    self.assertEqual(syncbridge.CloudClient(self.config).call(operation), 255)
                run.assert_called_once()
                sleep.assert_not_called()

    def test_missing_identity_fails_before_connecting(self):
        self.key.unlink()
        with patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
            syncbridge.subprocess, "run"
        ) as run, self.assertRaises(ValueError):
            syncbridge.CloudClient(self.config).call("status")
        run.assert_not_called()

    def test_timeout_is_unknown_result_not_success(self):
        for operation in ("check", "status", "preview", "once"):
            with self.subTest(operation=operation), patch.object(syncbridge.shutil, "which", return_value="ssh.exe"), patch.object(
                syncbridge.subprocess, "run", side_effect=subprocess.TimeoutExpired([], 10)
            ) as run, patch.object(syncbridge.time, "sleep") as sleep, redirect_stderr(io.StringIO()):
                self.assertEqual(syncbridge.CloudClient(self.config).call(operation), 124)
            run.assert_called_once()
            sleep.assert_not_called()

    def test_secret_values_are_withheld_from_structured_and_plain_output(self):
        raw = '{"Reason":"PriorPCBNotVerified","token":"token-value","nested":{"password":"password-value"}}\n'
        redacted = syncbridge.redact_output(raw)
        self.assertIn("PriorPCBNotVerified", redacted)
        self.assertNotIn("token-value", redacted)
        self.assertNotIn("password-value", redacted)
        raw = "Authorization: Bearer bearer-value\nhttps://user:pass@github.com/org/repo\n"
        redacted = syncbridge.redact_output(raw)
        self.assertNotIn("bearer-value", redacted)
        self.assertNotIn("user:pass", redacted)

    def test_private_key_blocks_are_withheld_including_truncated_output(self):
        for raw in ("-----BEGIN OPENSSH PRIVATE KEY-----\nkey-body-value\n-----END OPENSSH PRIVATE KEY-----\n",
                    "-----BEGIN OPENSSH PRIVATE KEY-----\nkey-body-value\n"):
            with self.subTest(complete="END" in raw):
                self.assertNotIn("key-body-value", syncbridge.redact_output(raw))

    def test_check_does_not_read_key_or_start_tasks(self):
        script = syncbridge.remote_script("check", self.config)
        self.assertIn("whoami.exe", script)
        self.assertNotIn(str(self.key), script)
        self.assertNotIn("Start-ScheduledTask", script)
        self.assertNotIn("Get-Content", script)

    def test_generated_scripts_have_no_control_characters(self):
        for operation in ("check", "status", "preview", "once"):
            with self.subTest(operation=operation):
                script = syncbridge.remote_script(operation, self.config)
                self.assertFalse(any(ord(char) < 32 for char in script))

    def test_generated_scripts_parse_in_available_local_powershell_without_execution(self):
        powershell = shutil.which("powershell.exe") or shutil.which("pwsh")
        if powershell is None:
            self.skipTest("A local PowerShell parser is not installed.")
        cases = json.dumps([syncbridge.remote_script(operation, self.config)
                            for operation in ("check", "status", "preview", "once")])
        encoded_cases = base64.b64encode(cases.encode("utf-8")).decode("ascii")
        probe = ("$cases=[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String('"
                 + encoded_cases + "'))|ConvertFrom-Json;"
                 "foreach($case in $cases){$t=$null;$e=$null;"
                 "[void][System.Management.Automation.Language.Parser]::ParseInput($case,[ref]$t,[ref]$e);"
                 "if($e){$e|ForEach-Object {$_.Message};exit 1}};exit 0")
        encoded_probe = base64.b64encode(probe.encode("utf-16-le")).decode("ascii")
        result = subprocess.run([powershell, "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded_probe],
                                shell=False, capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
