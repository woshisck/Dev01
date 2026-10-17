"""Tk layout/interaction tests for our own app; does not control other windows."""
import json
from pathlib import Path
import tempfile
import tkinter as tk
import unittest

from workflow_app import App
from workflow_core import save_json
from workflow_theme import COLORS


class WorkflowLayoutTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='dev01-workflow-ui-test-')
        project = Path(self.temp.name)
        (project / 'DevKit.uproject').write_text('{}')
        catalog = json.loads((Path(__file__).resolve().parents[2] / 'Build/Dev01WorkflowCatalog.json').read_text(encoding='utf-8'))
        self.expected_step_ids = {step['id'] for workflow in catalog['workflows'] for step in workflow['steps']}
        save_json(project / 'Build/Dev01WorkflowCatalog.json', catalog)
        self.root = tk.Tk()
        self.app = App(self.root, project)

    def tearDown(self):
        for after_id in self.root.tk.splitlist(self.root.tk.call('after', 'info')):
            self.root.after_cancel(after_id)
        self.root.destroy()
        self.temp.cleanup()

    def test_release_steps_reachable_at_minimum_size(self):
        self.root.geometry('880x630')
        self.app.notebook.select(1)
        self.root.update()
        page = self.app.release_page
        canvas = page.workflow_canvas
        self.assertGreater(canvas.winfo_height(), 100)
        self.assertGreater(page.winfo_height(), canvas.winfo_height())
        canvas.yview_moveto(1)
        self.root.update()
        last = next(button for button in self.app.buttons if button.cget('text') == '查看执行记录')
        self.assertGreaterEqual(last.winfo_rooty(), canvas.winfo_rooty())
        self.assertLessEqual(last.winfo_rooty() + last.winfo_height(), canvas.winfo_rooty() + canvas.winfo_height())

    def test_all_pages_and_readonly_scan_callback(self):
        self.root.update()
        self.assertEqual(len(self.app.notebook.tabs()), 4)
        for index in range(4):
            self.app.notebook.select(index)
            self.root.update()
        self.app.messages.put(('preview', {'candidates': [{'path': 'Saved/Logs/old.log', 'size': 12, 'keep': 'Saved/Logs/new.log'}]}))
        self.app.pump()
        self.assertEqual(len(self.app.cleanup_tree.get_children()), 1)
        self.assertEqual(self.app.cleanup_tree.item('0')['text'], 'Saved/Logs/old.log')

    def test_search_and_empty_state(self):
        self.root.update()
        self.app.search.set('贴花')
        self.root.update()
        self.assertGreater(len(self.app.step_items), 0)
        self.assertTrue(all('贴花' in ' '.join([w['title'], step['title'], step['input'], step['output']]) for w, i, step in self.app.step_items.values()))
        self.app.search.set('no_such_tool_1234')
        self.assertEqual(len(self.app.step_items), 0)
        self.assertEqual(self.app.step_title.get(), '没有匹配的工具')
        self.app.search.set('')
        self.assertEqual({step['id'] for _, _, step in self.app.step_items.values()}, self.expected_step_ids)

    def test_character_lookdev_is_searchable_in_material_workflow(self):
        self.root.update()
        self.app.search.set('LookDev')
        self.root.update()
        matches = list(self.app.step_items.values())
        self.assertEqual(len(matches), 1)
        workflow, _, step = matches[0]
        self.assertEqual(workflow['id'], 'materials')
        self.assertEqual(step['id'], 'character_lookdev')
        self.assertEqual(step['tab'], 'DevKitCharacterLookDev')

    def test_unsafe_launch_and_empty_archive_disabled(self):
        self.assertTrue(self.app.ugs_button.instate(['disabled']))
        self.assertTrue(self.app.source_button.instate(['disabled']))
        self.assertTrue(self.app.archive_button.instate(['disabled']))
        self.assertFalse(self.app.log_visible)

    def test_log_toggle_and_failure_feedback(self):
        self.root.update()
        self.app.toggle_log(); self.root.update()
        self.assertTrue(self.app.log_frame.winfo_ismapped())
        self.app.toggle_log(); self.root.update()
        self.assertFalse(self.app.log_frame.winfo_ismapped())
        self.app.messages.put(('result', ('发布检查', {'ok': False, 'blockers': ['模块不匹配']})))
        self.app.pump()
        self.assertIn('需要处理', self.app.status.get())
        self.assertNotIn('已完成', self.app.status.get())

    def test_text_contrast(self):
        def luminance(color):
            channels = [int(color[i:i + 2], 16) / 255 for i in (1, 3, 5)]
            linear = [v / 12.92 if v <= .04045 else ((v + .055) / 1.055) ** 2.4 for v in channels]
            return sum(v * w for v, w in zip(linear, (.2126, .7152, .0722)))
        for foreground, background in [('text', 'panel'), ('muted', 'panel'), ('white', 'primary')]:
            a, b = sorted([luminance(COLORS[foreground]), luminance(COLORS[background])])
            self.assertGreaterEqual((b + .05) / (a + .05), 4.5)


if __name__ == '__main__': unittest.main(verbosity=2)
