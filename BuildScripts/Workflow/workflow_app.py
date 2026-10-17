"""Dev01 workbench: UE5-neutral styling and explicit next actions."""
import argparse
import json
import os
from pathlib import Path
import queue
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from workflow_core import Workflow, save_json
from workflow_theme import COLORS as C, configure_style

ACTION_TITLES = {
    'source-editor': '打开开发副本', 'ugs-editor': '打开 UGS 编辑器', 'ugs': '打开 UGS',
    'source-build': '编译独立开发副本', 'pipeline-test': '检查引擎流水线',
    'installed-build': '生成可分发引擎', 'cloud-status': '读取云端状态',
    'cloud-build-publish': '云端构建并发布', 'pcb-verify': '校验预编译包',
}


class App:
    def __init__(self, root, project):
        self.root = root
        marker = Path(project) / 'Dev01Development.json'
        if marker.is_file():
            project = json.loads(marker.read_text(encoding='utf-8-sig'))['hostProject']
        self.workflow = Workflow(project)
        self.messages = queue.Queue()
        self.busy = False
        self.preview = None
        self.buttons, self.nav_buttons = [], []
        self.step_items = {}
        self.log_visible = False
        self.local = {}
        root.title('Dev01 工作台'); root.geometry('1200x850'); root.minsize(960, 680)
        root.configure(background=C['background'])
        configure_style(ttk.Style(root))
        header = ttk.Frame(root, padding=(22, 16)); header.pack(fill='x')
        ttk.Label(header, text='DEV01  /  工作台', font=('Microsoft YaHei UI', 18, 'bold')).pack(side='left')
        ttk.Label(header, text='开发制作  ·  构建发布  ·  数据整理', style='Muted.TLabel').pack(side='right')
        version = ttk.Frame(root, style='Panel.TFrame', padding=(16, 10)); version.pack(fill='x', padx=20, pady=(0, 14))
        self.version_title = ttk.Label(version, style='Panel.TLabel', font=('Microsoft YaHei UI', 10, 'bold'))
        self.version_title.pack(anchor='w')
        self.version_hint = ttk.Label(version, style='Panel.TLabel', foreground=C['muted'], wraplength=1080)
        self.version_hint.pack(anchor='w', pady=(5, 0))
        body = ttk.Frame(root); body.pack(fill='both', expand=True, padx=20)
        sidebar = ttk.Frame(body, padding=(0, 4, 14, 0), width=180)
        sidebar.pack(side='left', fill='y'); sidebar.pack_propagate(False)
        ttk.Label(sidebar, text='工作空间', style='Muted.TLabel').pack(anchor='w', pady=(2, 12))
        for index, (title, hint) in enumerate([
            ('开发制作', '选择工具，完成制作'), ('构建与发布', '检查版本，按步骤发布'),
            ('数据整理', '先预览，再可恢复归档'), ('连接与路径', '配置开发副本和云端'),
        ]):
            button = tk.Button(sidebar, text=title + '\n' + hint, anchor='w', justify='left',
                font=('Microsoft YaHei UI', 10), relief='flat', borderwidth=0, padx=12, pady=12,
                background=C['background'], foreground=C['text'], activebackground=C['header'],
                activeforeground=C['white'], highlightthickness=1, highlightbackground=C['background'],
                highlightcolor=C['primary'], command=lambda i=index: self.select_page(i))
            button.pack(fill='x', pady=3); self.nav_buttons.append(button)
        ttk.Label(sidebar, text='主工作区\nUGS / 已发布引擎', style='Muted.TLabel').pack(side='bottom', anchor='w', pady=16)
        self.notebook = ttk.Notebook(body, takefocus=False); self.notebook.pack(side='left', fill='both', expand=True)
        self.build_development(); self.build_release(); self.build_cleanup(); self.build_settings()
        self.notebook.bind('<<NotebookTabChanged>>', self.sync_navigation)
        self.status = tk.StringVar(value='就绪 · 选择左侧工作空间开始')
        footer = ttk.Frame(root, padding=(20, 10)); footer.pack(fill='x')
        self.status_label = ttk.Label(footer, textvariable=self.status, wraplength=760)
        self.status_label.pack(side='left', fill='x', expand=True)
        self.log_toggle = ttk.Button(footer, text='展开运行详情', command=self.toggle_log); self.log_toggle.pack(side='right')
        self.progress = ttk.Progressbar(root, mode='indeterminate'); self.progress.pack(fill='x', padx=20)
        self.log_frame = ttk.Frame(root, padding=(20, 8, 20, 12))
        self.log = tk.Text(self.log_frame, height=7, wrap='word', background=C['input'], foreground=C['text'],
            insertbackground=C['white'], font=('Consolas', 10), borderwidth=0, state='disabled')
        log_scroll = ttk.Scrollbar(self.log_frame, orient='vertical', command=self.log.yview)
        self.log.configure(yscrollcommand=log_scroll.set)
        log_scroll.pack(side='right', fill='y'); self.log.pack(side='left', fill='both', expand=True)
        root.protocol('WM_DELETE_WINDOW', self.close)
        root.bind('<Control-f>', self.focus_search); root.bind('<Configure>', self.resize_header)
        self.select_page(0); self.refresh_status(); root.after(100, self.pump)

    def resize_header(self, event):
        if event.widget == self.root:
            self.version_hint.configure(wraplength=max(300, event.width - 92))
            self.status_label.configure(wraplength=max(300, event.width - 210))

    def select_page(self, index):
        self.notebook.select(index); self.sync_navigation()

    def sync_navigation(self, event=None):
        index = self.notebook.index(self.notebook.select())
        for number, button in enumerate(self.nav_buttons):
            button.configure(background=C['header'] if number == index else C['background'],
                foreground=C['white'] if number == index else C['muted'],
                highlightbackground=C['primary'] if number == index else C['background'])

    def page(self, title, scroll=False):
        outer = ttk.Frame(self.notebook); self.notebook.add(outer, text=title)
        if not scroll:
            frame = ttk.Frame(outer, padding=(8, 0, 0, 0)); frame.pack(fill='both', expand=True)
            return frame
        canvas = tk.Canvas(outer, background=C['background'], highlightthickness=0)
        scrollbar = ttk.Scrollbar(outer, orient='vertical', command=canvas.yview)
        canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side='right', fill='y'); canvas.pack(side='left', fill='both', expand=True)
        content = ttk.Frame(canvas, padding=(8, 0, 10, 12))
        item = canvas.create_window((0, 0), window=content, anchor='nw')
        content.bind('<Configure>', lambda event: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.bind('<Configure>', lambda event: canvas.itemconfigure(item, width=event.width))
        content.workflow_canvas = canvas
        return content

    def finish_scroll_page(self, content):
        canvas = content.workflow_canvas
        def bind_children(widget):
            widget.bind('<MouseWheel>', lambda event: canvas.yview_scroll(int(-event.delta / 120), 'units'))
            if isinstance(widget, ttk.Label):
                canvas.bind('<Configure>', lambda event, label=widget: label.configure(wraplength=max(200, event.width - 65)), add='+')
            for child in widget.winfo_children(): bind_children(child)
        bind_children(content)

    def title(self, parent, title, description):
        ttk.Label(parent, text=title, font=('Microsoft YaHei UI', 15, 'bold')).pack(anchor='w')
        ttk.Label(parent, text=description, style='Muted.TLabel', wraplength=760).pack(anchor='w', pady=(6, 16))

    def card(self, parent, title, description):
        frame = ttk.Frame(parent, style='Panel.TFrame', padding=14); frame.pack(fill='x', pady=(0, 10))
        ttk.Label(frame, text=title, style='Panel.TLabel', font=('Microsoft YaHei UI', 11, 'bold')).pack(anchor='w')
        ttk.Label(frame, text=description, style='Panel.TLabel', foreground=C['muted'], wraplength=780).pack(anchor='w', pady=(6, 7))
        return frame

    def button(self, frame, text, callback, primary=False, danger=False):
        button = ttk.Button(frame, text=text, command=callback,
            style='Primary.TButton' if primary else ('Danger.TButton' if danger else 'TButton'))
        button.pack(side='left', padx=(0, 8), pady=5); self.buttons.append(button)
        return button

    def tree_scrollbars(self, parent, tree):
        vertical = ttk.Scrollbar(parent, orient='vertical', command=tree.yview)
        horizontal = ttk.Scrollbar(parent, orient='horizontal', command=tree.xview)
        tree.configure(yscrollcommand=vertical.set, xscrollcommand=horizontal.set)
        parent.rowconfigure(0, weight=1); parent.columnconfigure(0, weight=1)
        tree.grid(row=0, column=0, sticky='nsew'); vertical.grid(row=0, column=1, sticky='ns'); horizontal.grid(row=1, column=0, sticky='ew')

    def build_development(self):
        frame = self.page('开发制作')
        self.title(frame, '开发制作', '先选择制作步骤，再进入 UE 工具。输入、产出和打开方式会显示在右侧。')
        search_row = ttk.Frame(frame); search_row.pack(fill='x', pady=(0, 12))
        self.search = tk.StringVar()
        ttk.Label(search_row, text='查找工具').pack(side='left', padx=(0, 10))
        self.search_entry = ttk.Entry(search_row, textvariable=self.search); self.search_entry.pack(side='left', fill='x', expand=True)
        ttk.Label(search_row, text='Ctrl+F', style='Muted.TLabel').pack(side='left', padx=10)
        self.search.trace_add('write', lambda *args: self.populate_tools())
        self.search_entry.bind('<Escape>', lambda event: self.search.set(''))
        panes = ttk.Panedwindow(frame, orient='horizontal'); panes.pack(fill='both', expand=True)
        tree_frame = ttk.Frame(panes)
        self.tool_tree = ttk.Treeview(tree_frame, show='tree', selectmode='browse', height=8)
        self.tool_tree.column('#0', width=245, minwidth=205)
        self.tree_scrollbars(tree_frame, self.tool_tree); panes.add(tree_frame, weight=2)
        detail = ttk.Frame(panes, style='Panel.TFrame', padding=18); panes.add(detail, weight=3)
        self.step_number, self.step_title = tk.StringVar(), tk.StringVar()
        self.step_input, self.step_output, self.step_hint = tk.StringVar(), tk.StringVar(), tk.StringVar()
        ttk.Label(detail, textvariable=self.step_number, style='Panel.TLabel', foreground=C['muted']).pack(anchor='w')
        title_label = ttk.Label(detail, textvariable=self.step_title, style='Panel.TLabel', font=('Microsoft YaHei UI', 15, 'bold'), wraplength=360)
        title_label.pack(anchor='w', pady=(8, 16))
        detail.bind('<Configure>', lambda event: title_label.configure(wraplength=max(160, event.width - 40)), add='+')
        for title, value in [('准备什么', self.step_input), ('完成后得到什么', self.step_output), ('在哪里操作', self.step_hint)]:
            ttk.Label(detail, text=title, style='Panel.TLabel', foreground=C['muted']).pack(anchor='w', pady=(10, 4))
            label = ttk.Label(detail, textvariable=value, style='Panel.TLabel', wraplength=360); label.pack(anchor='w')
            detail.bind('<Configure>', lambda event, item=label: item.configure(wraplength=max(160, event.width - 40)), add='+')
        self.tool_tree.bind('<<TreeviewSelect>>', self.select_tool); self.populate_tools()
        row = ttk.Frame(frame); row.pack(fill='x', pady=(12, 0))
        self.ugs_button = self.button(row, '打开 UGS 编辑器', lambda: self.execute('ugs-editor'), primary=True)
        self.source_button = self.button(row, '打开开发副本', lambda: self.execute('source-editor'))
        self.button(row, '重新检查版本', self.refresh_status)
        ttk.Label(frame, text='新工作台尚未发布时，请使用开发副本体验；UGS 保持团队已发布版本。', style='Muted.TLabel', wraplength=780).pack(anchor='w', pady=(5, 0))

    def focus_search(self, event=None):
        self.select_page(0); self.search_entry.focus_set(); self.search_entry.select_range(0, 'end')
        return 'break'

    def populate_tools(self):
        if not hasattr(self, 'tool_tree'): return
        self.tool_tree.delete(*self.tool_tree.get_children()); self.step_items.clear()
        query = self.search.get().strip().casefold()
        for workflow in self.workflow.catalog['workflows']:
            matches = [(i, s) for i, s in enumerate(workflow['steps']) if not query or query in ' '.join([workflow['title'], s['title'], s['input'], s['output']]).casefold()]
            if not matches: continue
            parent = self.tool_tree.insert('', 'end', text=workflow['title'], open=True)
            for index, step in matches:
                item = self.tool_tree.insert(parent, 'end', text=f'{index + 1:02d}  {step["title"]}')
                self.step_items[item] = (workflow, index, step)
        if self.step_items:
            item = next(iter(self.step_items)); self.tool_tree.selection_set(item); self.select_tool()
        else:
            self.step_number.set('查找结果'); self.step_title.set('没有匹配的工具')
            self.step_input.set('试试“材质”“贴花”“关卡”或“武器”。'); self.step_output.set(''); self.step_hint.set('按 Esc 清空搜索。')

    def select_tool(self, event=None):
        selection = self.tool_tree.selection()
        if not selection or selection[0] not in self.step_items: return
        workflow, index, step = self.step_items[selection[0]]
        self.step_number.set(f'{workflow["title"]}   /   步骤 {index + 1:02d} · 共 {len(workflow["steps"])} 步')
        self.step_title.set(step['title']); self.step_input.set(step['input']); self.step_output.set(step['output'])
        self.step_hint.set('UE → YogTool → 开发工作台 → ' + workflow['title'])

    def build_release(self):
        frame = self.page('构建与发布', scroll=True); self.release_page = frame
        self.title(frame, '构建与发布', '从检查开始。只改项目代码不需要重新发布引擎；每一步显示其作用和风险。')
        for title, description, actions in [
            ('01  检查当前版本', '只读检查 P4 未提交文件、冲突、固定引擎版本及模块匹配情况。不会提交或上传。',
             [('开始只读检查', lambda: self.background('发布预检', self.workflow.preflight), 'primary'), ('检查模块版本', lambda: self.background('模块版本检查', self.ids), '')]),
            ('02  本地编译与效果确认', '只编译独立副本当前代码，不自动同步主工作区源码。编译后仍需人工确认效果。',
             [('编译开发副本', lambda: self.execute('source-build'), ''), ('检查引擎流水线', lambda: self.execute('pipeline-test'), '')]),
            ('03  引擎更新（仅引擎变更需要）', '关闭 UE 并确认效果后生成 Installed Build。Engine CL 固定与旧发布契约仍需人工审查。',
             [('生成可分发引擎', lambda: self.execute('installed-build'), ''), ('查看引擎发布说明', self.release_plan, '')]),
            ('04  云端项目与预编译包', '先配置 SSH 并读取状态。“构建并发布”会向 P4 上传，执行前必须确认。',
             [('读取云端状态', lambda: self.execute('cloud-status'), ''), ('云端构建并发布', lambda: self.execute('cloud-build-publish'), 'danger')]),
            ('05  UGS 同步与验收', '包校验通过后，在 UGS 同步对应版本。最终以编辑器成功启动和实际效果为准。',
             [('校验预编译包', lambda: self.execute('pcb-verify'), ''), ('打开 UGS', lambda: self.execute('ugs'), ''), ('查看执行记录', lambda: self.open_path(self.workflow.state / 'Runs'), '')]),
        ]:
            box = self.card(frame, title, description)
            row = ttk.Frame(box, style='Panel.TFrame'); row.pack(fill='x')
            for label, callback, emphasis in actions: self.button(row, label, callback, emphasis == 'primary', emphasis == 'danger')
        self.finish_scroll_page(frame)

    def build_cleanup(self):
        frame = self.page('数据整理')
        self.title(frame, '数据整理', '仅整理七天前、内容完全相同的生成文本。保留最新副本，归档后可以恢复。')
        self.cleanup_message = tk.StringVar(value='尚未扫描 · 点击“扫描重复文件”查看候选，不会直接改动文件。')
        ttk.Label(frame, textvariable=self.cleanup_message, foreground=C['muted'], wraplength=760).pack(anchor='w', pady=(0, 12))
        tree_frame = ttk.Frame(frame); tree_frame.pack(fill='both', expand=True)
        self.cleanup_tree = ttk.Treeview(tree_frame, columns=('size', 'keep'), show='tree headings', selectmode='extended')
        for column, title, width in [('#0', '可归档的重复文件', 300), ('size', '大小', 85), ('keep', '将保留的副本', 310)]:
            self.cleanup_tree.heading(column, text=title); self.cleanup_tree.column(column, width=width)
        self.tree_scrollbars(tree_frame, self.cleanup_tree)
        self.cleanup_tree.bind('<<TreeviewSelect>>', lambda event: self.update_buttons())
        row = ttk.Frame(frame); row.pack(fill='x', pady=(12, 0))
        self.button(row, '扫描重复文件', self.scan, primary=True)
        self.archive_button = self.button(row, '归档选中项', self.archive)
        self.button(row, '恢复归档', self.restore)
        self.button(row, '打开归档目录', lambda: self.open_path(self.workflow.state / 'Archive'))
        ttk.Label(frame, text='不会清理：正式资产、自动保存、游戏存档、崩溃恢复、引擎文件或发布包。', style='Muted.TLabel', wraplength=760).pack(anchor='w', pady=6)

    def build_settings(self):
        frame = self.page('连接与路径', scroll=True)
        self.title(frame, '连接与路径', '主工作区运行 UGS 发布版；源码开发使用独立项目目录，避免两套 DLL 互相覆盖。')
        box = self.card(frame, '独立开发副本', '选择含 DevKit.uproject 的独立目录。不能选当前主工作区或把 Binaries 链接回来。')
        self.development = tk.StringVar(value=self.workflow.settings.get('developmentProject', ''))
        ttk.Entry(box, textvariable=self.development).pack(fill='x')
        row = ttk.Frame(box, style='Panel.TFrame'); row.pack(fill='x'); self.button(row, '选择项目目录', self.choose_development)
        box = self.card(frame, '云端 SSH', '使用已配置公钥和 known_hosts 的 Host 别名或 user@host。不存储密码或票据。')
        self.ssh = tk.StringVar(value=self.workflow.settings.get('sshHost', '')); ttk.Entry(box, textvariable=self.ssh).pack(fill='x')
        row = ttk.Frame(frame); row.pack(fill='x', pady=(0, 12))
        self.button(row, '保存设置', self.save_settings, primary=True); self.button(row, '使用说明', self.release_plan)
        names = {'sourceEngine': '可编辑源码引擎', 'pipeline': '本地引擎流水线', 'installedBuild': 'Installed Build 产物',
            'publishStage': '隔离发布目录', 'gitProject': 'Git 代码副本', 'ugs': 'UGS 客户端', 'cloudRoot': '云端脚本目录'}
        box = self.card(frame, '当前路径', str(self.workflow.project))
        for key, path in self.workflow.paths.items():
            ttk.Label(box, text=f'{names.get(key, key)}\n{path}', style='Panel.TLabel', foreground=C['muted']).pack(anchor='w', pady=6)
        self.finish_scroll_page(frame)

    def choose_development(self):
        path = filedialog.askdirectory(title='选择独立开发项目目录')
        if path: self.development.set(path)

    def save_settings(self):
        previous = dict(self.workflow.settings)
        self.workflow.settings.update(sshHost=self.ssh.get().strip(), developmentProject=self.development.get().strip())
        try:
            if self.development.get().strip(): self.workflow.development_project()
            save_json(self.workflow.settings_path, self.workflow.settings)
        except Exception as exc:
            self.workflow.settings = previous; messagebox.showerror('设置未保存', str(exc)); return
        self.status.set('设置已保存 · 主工作区与开发副本保持隔离'); self.refresh_status()

    def refresh_status(self):
        self.local = self.workflow.local_status()
        cl = self.local['projectCL'] or '未读取'
        self.version_title.configure(text=f'UGS 主工作区  /  项目 CL {cl}    ·    {self.local["title"]}',
            foreground=C['success'] if self.local['ok'] else C['warning'])
        engine = (self.local.get('engineId') or '缺失')[:8]; project = (self.local.get('projectId') or '缺失')[:8]
        self.version_hint.configure(text=f'引擎模块 {engine}  /  项目模块 {project}  —  {self.local["hint"]}')
        self.update_buttons()

    def update_buttons(self):
        for button in self.buttons: button.configure(state='disabled' if self.busy else 'normal')
        if self.busy: return
        if not self.local.get('ok'): self.ugs_button.configure(state='disabled')
        if not self.local.get('developmentReady'): self.source_button.configure(state='disabled')
        if not self.preview or not self.cleanup_tree.selection(): self.archive_button.configure(state='disabled')

    def toggle_log(self):
        self.log_visible = not self.log_visible
        if self.log_visible: self.log_frame.pack(fill='x')
        else: self.log_frame.pack_forget()
        self.log_toggle.configure(text='收起运行详情' if self.log_visible else '展开运行详情')

    def emit(self, text): self.messages.put(('log', str(text)))

    def background(self, title, callback):
        if self.busy: return
        self.busy = True; self.status.set(title + ' · 正在执行，请稍候')
        self.status_label.configure(foreground=C['text']); self.progress.start(12); self.update_buttons()
        def worker():
            try:
                result = callback()
                if result is not None: self.emit(json.dumps(result, ensure_ascii=False, indent=2))
                self.messages.put(('result', (title, result)))
            except Exception as exc: self.messages.put(('error', str(exc)))
        threading.Thread(target=worker, daemon=True).start()

    def execute(self, action):
        if action in {'source-build', 'installed-build', 'cloud-build-publish'}:
            text = {
                'source-build': '仅编译设置中的独立开发副本，不覆盖 UGS 主工作区。请先保存并关闭 Unreal Editor。现在编译？',
                'installed-build': '该步骤耗时较长，并会更新 Installed Build 产物。确认已用源码引擎完成效果验收并关闭 UE？',
                'cloud-build-publish': '该步骤会在云端构建项目，并向 P4 发布预编译包。请确认提交范围和版本已经审核。现在发布？',
            }[action]
            if not messagebox.askyesno('确认：' + ACTION_TITLES[action], text): return
        self.background(ACTION_TITLES[action], lambda: self.workflow.execute(action, self.emit))

    def ids(self):
        result = self.workflow.build_ids(); self.workflow.write_report('buildids', result)
        return result

    def scan(self):
        def task():
            result = self.workflow.cleanup_scan(); self.messages.put(('preview', result))
            return {'候选数': len(result['candidates']), '扫描数': result['scanned'], '归档字节': result['candidateBytes']}
        self.background('扫描重复文件', task)

    def archive(self):
        selected = self.cleanup_tree.selection()
        if not selected or not self.preview: return
        candidates = [self.preview['candidates'][int(index)] for index in selected]
        if not messagebox.askyesno('归档重复文件', f'将 {len(candidates)} 个重复文件移动到可恢复归档，保留最新副本。继续？'): return
        self.background('归档重复文件', lambda: self.workflow.archive_duplicates(candidates))

    def restore(self):
        manifest = filedialog.askopenfilename(initialdir=self.workflow.state / 'Archive', filetypes=[('归档清单', 'manifest.json')])
        if manifest: self.background('恢复归档', lambda: self.workflow.restore(manifest))

    def release_plan(self): self.open_path(self.workflow.project / 'Docs/Dev01_Workflow_Guide.md')

    def open_path(self, path):
        path = Path(path)
        if not path.exists(): messagebox.showinfo('尚未生成记录', f'完成相应步骤后会生成此目录：\n{path}'); return
        os.startfile(path)

    def pump(self):
        while True:
            try: kind, value = self.messages.get_nowait()
            except queue.Empty: break
            if kind == 'log':
                self.log.configure(state='normal'); self.log.insert('end', value + '\n')
                count = int(self.log.index('end-1c').split('.')[0])
                if count > 2000: self.log.delete('1.0', f'{count - 2000}.0')
                self.log.see('end'); self.log.configure(state='disabled')
            elif kind == 'preview':
                self.preview = value; self.cleanup_tree.delete(*self.cleanup_tree.get_children())
                for index, item in enumerate(value['candidates']):
                    self.cleanup_tree.insert('', 'end', iid=str(index), text=item['path'], values=(f'{item["size"] / 1024:.1f} KB', item['keep']))
                count = len(value['candidates'])
                self.cleanup_message.set(f'发现 {count} 个候选 · 选择后才能归档。' if count else '扫描完成 · 没有符合规则的重复文件，无需清理。')
            else:
                self.busy = False; self.progress.stop()
                if kind == 'error':
                    self.status.set('未完成 · ' + value); self.status_label.configure(foreground=C['error']); self.emit(value)
                else:
                    title, result = value
                    failed = isinstance(result, dict) and (result.get('ok') is False or result.get('status') == 'failed')
                    if failed:
                        hint = '；'.join(result.get('blockers', [])) or '请查看详情并处理版本或执行错误。'
                        self.status.set(title + ' · 需要处理：' + hint); self.status_label.configure(foreground=C['warning'])
                    else:
                        state = (result or {}).get('status') if isinstance(result, dict) else None
                        self.status.set(title + (' · 已启动，请确认实际窗口/后台结果' if state in {'started', 'launched'} else ' · 已完成'))
                        self.status_label.configure(foreground=C['success'])
                self.refresh_status()
        self.root.after(100, self.pump)

    def close(self):
        if self.busy: messagebox.showinfo('操作进行中', '请等当前步骤完成后关闭。此操作不会中断正在写入的构建。'); return
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser()
    base = Path(sys.executable) if getattr(sys, 'frozen', False) else Path(__file__)
    default = next((str(p) for p in base.resolve().parents if (p / 'DevKit.uproject').is_file()), None)
    parser.add_argument('--project', default=default); parser.add_argument('--smoke-test', action='store_true')
    args = parser.parse_args(); root = tk.Tk()
    if not args.project:
        root.withdraw(); args.project = filedialog.askdirectory(title='选择 Dev01 主工作区')
        if not args.project: root.destroy(); return
        root.deiconify()
    try: App(root, args.project)
    except Exception as exc:
        root.withdraw(); messagebox.showerror('工作台启动失败', str(exc)); root.destroy(); return
    if args.smoke_test: root.after(600, root.destroy)
    root.mainloop()


if __name__ == '__main__': main()
