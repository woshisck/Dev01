import json
import os
from pathlib import Path
import tempfile
import threading
import time
import unittest
import uuid
from unittest.mock import patch

from workflow_core import Workflow, digest, save_json, engine_build_guard


class WorkflowTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='dev01-workflow-test-')
        self.root = Path(self.temp.name)
        (self.root / 'DevKit.uproject').write_text('{}')
        save_json(self.root / 'Build/Dev01WorkflowCatalog.json', {'paths': {}, 'workflows': []})
        self.workflow = Workflow(self.root)

    def tearDown(self):
        self.temp.cleanup()

    def file(self, name, contents='sample', days=10):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents)
        old = time.time() - days * 86400
        os.utime(path, (old, old))
        return path

    def test_archive_restore_roundtrip(self):
        old = self.file('Saved/Logs/old.log', days=10)
        keep = self.file('Saved/Logs/new.log', days=1)
        scan = self.workflow.cleanup_scan()
        self.assertEqual(len(scan['candidates']), 1)
        result = self.workflow.archive_duplicates(scan['candidates'])
        self.assertFalse(old.exists())
        self.assertTrue(keep.exists())
        self.assertEqual(self.workflow.restore(result['manifest'])['restored'], 1)
        self.assertEqual(digest(old), digest(keep))

    def test_changed_keep_blocks_archive(self):
        old = self.file('Saved/Logs/old.log')
        keep = self.file('Saved/Logs/new.log', days=1)
        scan = self.workflow.cleanup_scan()
        keep.write_text('changed')
        with self.assertRaises(ValueError): self.workflow.archive_duplicates(scan['candidates'])
        self.assertTrue(old.exists())

    def test_restore_never_overwrites(self):
        old = self.file('Saved/Logs/old.log')
        self.file('Saved/Logs/new.log', days=1)
        result = self.workflow.archive_duplicates(self.workflow.cleanup_scan()['candidates'])
        old.write_text('new user data')
        with self.assertRaises(FileExistsError): self.workflow.restore(result['manifest'])
        self.assertEqual(old.read_text(), 'new user data')

    def test_protected_data_excluded(self):
        self.file('Saved/Logs/new.log', days=1)
        self.file('Saved/Logs/Latest.log')
        self.file('Saved/Codex/CrashRecovery/data.json')
        self.file('Saved/Codex/PackageRestoreData.disabled.json')
        self.file('Content/user.uasset')
        self.file('Saved/SaveGames/save.json')
        self.file('Saved/Autosaves/a.json')
        self.file('Saved/Logs/recent.log', days=1)
        self.assertEqual(self.workflow.cleanup_scan()['candidates'], [])

    def test_traversal_rejected(self):
        for path in ['../outside.txt', 'Content/example.uasset', 'Saved/Logs/../../Content/example.txt', 'Saved/Codex/Recovery/foo.json']:
            with self.assertRaises(ValueError): self.workflow.safe_generated_path(path)

    def test_target_buildid_is_nested(self):
        save_json(self.root / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'engine-id'})
        save_json(self.root / 'Binaries/Win64/DevKitEditor.target', {'Version': {'BuildId': 'engine-id'}})
        save_json(self.root / 'Binaries/Win64/UnrealEditor.modules', {'BuildId': 'engine-id'})
        self.assertTrue(self.workflow.build_ids()['ok'])
        save_json(self.root / 'Plugins/Demo/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'wrong'})
        self.assertFalse(self.workflow.build_ids()['ok'])

    def test_missing_manifest_is_failure(self):
        self.assertFalse(self.workflow.build_ids()['ok'])

    def test_matching_ids_cannot_hide_missing_binary(self):
        save_json(self.root / 'Engine/Binaries/Win64/UnrealEditor.modules', {'BuildId': 'engine-id'})
        save_json(self.root / 'Binaries/Win64/DevKitEditor.target', {'Version': {'BuildId': 'engine-id'}})
        save_json(self.root / 'Binaries/Win64/UnrealEditor.modules',
                  {'BuildId': 'engine-id', 'Modules': {'DevKitEditor': 'UnrealEditor-DevKitEditor.dll'}})
        result = self.workflow.build_ids()
        self.assertFalse(result['ok'])
        self.assertEqual(result['records'][2]['missingBinaries'], ['DevKitEditor'])

    def test_main_workspace_cannot_be_development_target(self):
        for value in ['', str(self.root), str(self.root / 'Saved/DevCopy')]:
            self.workflow.settings['developmentProject'] = value
            with self.assertRaises(RuntimeError): self.workflow.development_project()

    def test_development_commands_are_isolated(self):
        with tempfile.TemporaryDirectory(prefix='dev01-isolated-test-') as folder:
            separate = Path(folder)
            (separate / 'DevKit.uproject').write_text('{}')
            self.workflow.settings['developmentProject'] = str(separate)
            self.workflow.paths.update(sourceEngine='X:/SourceEngine', pipeline='X:/Pipeline', ugs='X:/UGS')
            result = self.workflow.commands(self.workflow.development_project())
            self.assertIn(f'-Project={separate / "DevKit.uproject"}', result['source-build'])
            self.assertEqual(result['source-editor'][1], str(separate / 'DevKit.uproject'))
            self.assertEqual(result['ugs-editor'][1], str(self.root / 'DevKit.uproject'))

    def test_invalid_ssh_host_rejected(self):
        for host in ['', '-oProxyCommand=cmd', 'user@host;evil', 'host example']:
            self.workflow.settings['sshHost'] = host
            with self.assertRaises(ValueError): self.workflow.cloud_command()

    def test_pending_resolve_blocks_publish(self):
        self.workflow.paths['sourceEngine'] = str(self.root)
        def p4(*args):
            output = ''
            if args[0] == 'stream': output = ' import Engine/... //Dev01Engine/main/...@76'
            if args[0] == 'resolve': output = '//Dev01/main/Source/Test.cpp - merging //Dev01/main/Source/Test.cpp#2'
            return {'ok': True, 'code': 0, 'output': output}
        with patch.object(self.workflow, 'p4', side_effect=p4), patch.object(self.workflow, 'build_ids', return_value={'ok': True}), patch('workflow_core.run', return_value={'ok': True}):
            result = self.workflow.preflight()
        self.assertFalse(result['ok'])
        self.assertTrue(any('resolve' in item for item in result['blockers']))

    @unittest.skipUnless(os.name == 'nt', 'Windows named mutex')
    def test_shared_engine_pipeline_mutex(self):
        mutex_name = 'Local\\Dev01WorkflowTest-' + uuid.uuid4().hex
        acquired = threading.Event()
        release = threading.Event()
        failures = []
        def owner():
            try:
                with engine_build_guard(mutex_name):
                    acquired.set()
                    release.wait(10)
            except Exception as exc: failures.append(exc); acquired.set()
        thread = threading.Thread(target=owner)
        thread.start()
        try:
            self.assertTrue(acquired.wait(5))
            self.assertEqual(failures, [])
            with self.assertRaises(RuntimeError):
                with engine_build_guard(mutex_name): pass
        finally:
            release.set(); thread.join(5)
        with engine_build_guard(mutex_name): pass


if __name__ == '__main__': unittest.main(verbosity=2)
