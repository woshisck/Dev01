"""Dev01 workflow operations. No third-party dependencies or implicit publication."""
from __future__ import annotations

import argparse
from contextlib import contextmanager, nullcontext
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import html
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import uuid


def stamp():
    return datetime.now().strftime('%Y%m%d-%H%M%S') + '-' + uuid.uuid4().hex[:6]


def save_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_name(path.name + '.tmp-' + uuid.uuid4().hex)
    temp.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    os.replace(temp, path)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def run(args, cwd, timeout=30):
    try:
        proc = subprocess.run([str(x) for x in args], cwd=str(cwd), capture_output=True,
                              encoding='utf-8', errors='replace', timeout=timeout,
                              creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        return {'ok': proc.returncode == 0, 'code': proc.returncode,
                'output': (proc.stdout + proc.stderr).strip()}
    except (OSError, subprocess.TimeoutExpired) as exc:
        return {'ok': False, 'code': -1, 'output': str(exc)}


@contextmanager
def engine_build_guard(name='Global\\Dev01BinaryReleasePipeline'):
    """Share the existing Installed Build pipeline's cross-process mutex."""
    import ctypes
    from ctypes import wintypes
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.CreateMutexW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.LPCWSTR]
    api.CreateMutexW.restype = wintypes.HANDLE
    api.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    api.WaitForSingleObject.restype = wintypes.DWORD
    api.ReleaseMutex.argtypes = [wintypes.HANDLE]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = api.CreateMutexW(None, False, name)
    if not handle: raise ctypes.WinError(ctypes.get_last_error())
    acquired = False
    try:
        result = api.WaitForSingleObject(handle, 0)
        acquired = result in (0, 0x80)  # WAIT_OBJECT_0 or WAIT_ABANDONED
        if not acquired:
            raise RuntimeError('Installed Build / 引擎发布流水线正在运行，不能同时编译源码引擎。')
        yield
    finally:
        if acquired: api.ReleaseMutex(handle)
        api.CloseHandle(handle)


class Workflow:
    CLEAN_ROOTS = ('Saved/Logs', 'Saved/Codex', 'Saved/Automation')
    TEXT_SUFFIXES = {'.log', '.txt', '.md', '.json', '.csv'}
    PROTECTED_NAMES = re.compile(r'(latest|current|ticket|secret|password|credential|lock|backup|restore|recovery|disabled)', re.I)

    def __init__(self, project):
        self.project = Path(project).resolve()
        if not (self.project / 'DevKit.uproject').is_file():
            raise ValueError(f'不是 Dev01 项目目录：{self.project}')
        self.catalog_path = self.project / 'Build/Dev01WorkflowCatalog.json'
        self.catalog = json.loads(self.catalog_path.read_text(encoding='utf-8-sig'))
        self.paths = self.catalog['paths']
        self.state = self.project / 'Saved/WorkflowHub'
        self.settings_path = self.state / 'settings.json'
        self.settings = json.loads(self.settings_path.read_text(encoding='utf-8-sig')) if self.settings_path.exists() else {}

    def p4(self, *args):
        return run(['p4', '-v', 'net.maxwait=10', *args], self.project, 22)

    def development_project(self):
        value = self.settings.get('developmentProject', '').strip()
        if not value:
            raise RuntimeError('尚未设置独立开发副本。请到“连接与路径”选择项目目录；不会在 UGS 主工作区编译源码引擎。')
        path = Path(value).resolve()
        if path == self.project or path.is_relative_to(self.project):
            raise RuntimeError('开发副本不能使用 UGS 主工作区或其子目录，请选择独立目录。')
        if not (path / 'DevKit.uproject').is_file():
            raise RuntimeError('开发副本中没有 DevKit.uproject，请先准备完整项目副本。')
        if (path / 'Binaries').exists() and (path / 'Binaries').resolve().is_relative_to(self.project):
            raise RuntimeError('开发副本的 Binaries 指向主工作区，已禁止编译。')
        for binary_path in (path / 'Plugins').glob('**/Binaries'):
            if binary_path.resolve().is_relative_to(self.project):
                raise RuntimeError('开发副本的插件 Binaries 指向主工作区，已禁止编译。')
        return path

    def local_status(self):
        manifests = self.build_ids()
        try:
            state = json.loads((self.project / '.ugs/state.json').read_text(encoding='utf-8-sig'))
        except (OSError, ValueError): state = {}
        development_ready = False
        try:
            development = str(self.development_project())
            development_ready = self.build_ids(Path(development), Path(self.paths['sourceEngine']) / 'Engine/Binaries/Win64/UnrealEditor.modules')['ok']
        except (RuntimeError, OSError, KeyError): development = None
        engine_id = manifests['records'][0]['buildId']
        project_id = next((item['buildId'] for item in manifests['records'] if item['path'].replace('\\', '/') == 'Binaries/Win64/UnrealEditor.modules'), None)
        return {'ok': manifests['ok'], 'projectCL': state.get('CurrentChangeNumber'),
                'archive': state.get('LastSyncEditorArchive'), 'engineId': engine_id, 'projectId': project_id,
                'developmentProject': development, 'developmentReady': development_ready,
                'title': 'UGS 模块匹配 · 可以启动' if manifests['ok'] else 'UGS 模块不匹配 · 暂停启动',
                'hint': '使用 UGS 引擎打开当前主工作区。' if manifests['ok'] else
                    '项目 DLL 与已发布引擎不匹配。请备份本地开发产物，再由 UGS 恢复对应预编译包；不要点击 Missing Modules 的 Yes。'}

    def build_ids(self, project=None, engine_manifest=None):
        project = Path(project or self.project)
        files = [engine_manifest or project / 'Engine/Binaries/Win64/UnrealEditor.modules',
                 project / 'Binaries/Win64/DevKitEditor.target',
                 project / 'Binaries/Win64/UnrealEditor.modules']
        files += sorted((project / 'Plugins').glob('**/Binaries/Win64/UnrealEditor.modules'))
        records = []
        for path in files:
            label = str(path.relative_to(project)) if path.is_relative_to(project) else 'SourceEngine/UnrealEditor.modules'
            try:
                value = json.loads(path.read_text(encoding='utf-8-sig'))
                if not isinstance(value, dict): raise ValueError('模块元数据必须是 JSON 对象')
                if path.suffix == '.target' and not isinstance(value.get('Version', {}), dict):
                    raise ValueError('target.Version 格式错误')
                build_id = value.get('Version', {}).get('BuildId') if path.suffix == '.target' else value.get('BuildId')
                modules = value.get('Modules', {}) if path.suffix == '.modules' else {}
                if not isinstance(modules, dict): raise ValueError('Modules 格式错误')
                missing = [name for name, binary in modules.items()
                           if not isinstance(binary, str) or not (path.parent / binary).is_file()]
                records.append({'path': label, 'buildId': build_id,
                                'ok': bool(build_id) and not missing, 'missingBinaries': missing})
            except (OSError, ValueError) as exc:
                records.append({'path': label, 'buildId': None, 'ok': False, 'error': str(exc)})
        ids = {r['buildId'] for r in records}
        return {'ok': len(ids) == 1 and all(r['ok'] for r in records), 'records': records,
                'note': '相同 BuildId 是必要条件；不替代本轮构建凭据、输出哈希和 UGS 冷启动验收。'}

    def inventory(self):
        workflows = self.catalog['workflows']
        source = (self.project / 'Source/DevKitEditor/DevKitEditor.cpp').read_text(encoding='utf-8-sig')
        tabs = sorted(set(re.findall(r'const FName \w+TabName\(TEXT\("([^"]+)"\)\)', source)))
        mapped = {s['tab'] for w in workflows for s in w['steps']}
        retained = {s['tab'] for s in self.catalog.get('retainedEntries', [])}
        directories = []
        for rel, role in [('Source/DevKitEditor/Private/Tools', '编辑器工具源码'),
                          ('Content', '正式项目资产：原路径保留'), ('Plugins', '插件源码和资产：原路径保留'),
                          ('Build', '版本与工作流配置'), ('BuildScripts', '构建与检查入口'),
                          ('Docs/GeneratedReports', '已版本化/历史报告：保留'),
                          ('Saved/Logs', '生成日志：可查重归档'), ('Saved/Autosaves', '自动保存：保留'),
                          ('Saved/SaveGames', '存档：保留'), ('Saved/Screenshots', '验收图像：保留')]:
            directories.append({'path': rel, 'role': role, 'exists': (self.project / rel).exists()})
        return {'time': datetime.now(timezone.utc).isoformat(), 'project': str(self.project),
                'workflows': workflows, 'registeredTabs': tabs,
                'unmappedTabs': sorted(set(tabs) - mapped - retained - {'DevKitWorkflow'}),
                'directories': directories,
                'paths': {k: {'path': v, 'exists': Path(v).exists()} for k, v in self.paths.items() if k != 'cloudRoot'}}

    def preflight(self):
        commands = {
            'opened': ['opened'], 'pending': ['changes', '-s', 'pending', '-m', '20'],
            'resolve': ['resolve', '-n'], 'projectHead': ['changes', '-s', 'submitted', '-m', '1', '//Dev01/main/...'],
            'engineHead': ['changes', '-s', 'submitted', '-m', '1', '//Dev01Engine/main/...'],
            'stream': ['stream', '-o', '//Dev01/main'],
            'pcb': ['files', '//Dev01Binaries/UGS/++Dev01+main-Editor.zip'],
        }
        with ThreadPoolExecutor(max_workers=4) as pool:
            pending = {key: pool.submit(self.p4, *args) for key, args in commands.items()}
            p4 = {key: future.result() for key, future in pending.items()}
        engine = Path(self.paths['sourceEngine'])
        engine_state = run(['git', '-C', engine, 'status', '--short', '--branch'], self.project)
        stream_output = p4['stream']['output']
        imports = re.findall(r'^\s*import\s+Engine/\.\.\.\s+//Dev01Engine/main/\.\.\.@(\d+)\s*$', stream_output, re.M)
        manifests = self.build_ids()
        blockers = []
        for key in ['opened', 'pending', 'resolve', 'projectHead', 'engineHead', 'stream', 'pcb']:
            item = p4[key]
            empty = key in {'opened', 'resolve'} and re.search(r'not opened|No file\(s\) to resolve|File\(s\) not opened', item['output'], re.I)
            if not item['ok'] and not empty:
                blockers.append(f'P4 {key} 检查失败')
        if re.search(r'^//.* - (edit|add|delete|move|integrate|branch)', p4['opened']['output'], re.M):
            blockers.append('工作区有 opened 文件，提交范围需明确隔离')
        if re.search(r'^Change \d+', p4['pending']['output'], re.M):
            blockers.append('已有 pending CL，先核对归属与提交状态')
        if p4['resolve']['ok'] and p4['resolve']['output'] and not re.search(r'No file\(s\) to resolve', p4['resolve']['output'], re.I):
            blockers.append('存在待 resolve 文件，不能发布')
        if not imports: blockers.append('未发现固定 Engine @CL 导入')
        if not manifests['ok']: blockers.append('本地已发布 Engine / 项目 / 插件 BuildId 不一致或文件缺失')
        result = {'time': datetime.now(timezone.utc).isoformat(), 'p4': p4, 'sourceEngineGit': engine_state,
                  'pinnedEngine': imports, 'buildIds': manifests, 'blockers': blockers,
                  'ok': not blockers, 'note': '只读预检。此结果不代表已获发布批准或云端构建成功。'}
        self.write_report('preflight', result)
        return result

    def cleanup_scan(self, minimum_age_days=7):
        groups = {}
        cutoff = time.time() - minimum_age_days * 86400
        scanned = 0
        for rel in self.CLEAN_ROOTS:
            root = self.project / rel
            if not root.is_dir() or root.is_symlink() or root.is_junction(): continue
            for folder, dirs, files in os.walk(root, followlinks=False):
                dirs[:] = [d for d in dirs if not self.PROTECTED_NAMES.search(d) and not (Path(folder) / d).is_symlink() and not (Path(folder) / d).is_junction()]
                for name in files:
                    path = Path(folder) / name
                    if path.is_symlink() or path.suffix.lower() not in self.TEXT_SUFFIXES: continue
                    if self.PROTECTED_NAMES.search(name): continue
                    try:
                        stat = path.stat()
                        if not stat.st_size or stat.st_size > 16 * 1024 * 1024: continue
                        checksum = digest(path)
                        scanned += 1
                        groups.setdefault((stat.st_size, checksum), []).append((path, stat.st_mtime))
                    except OSError:
                        continue
        candidates = []
        for (size, checksum), entries in groups.items():
            entries.sort(key=lambda item: (item[1], str(item[0])), reverse=True)
            keep = entries[0][0]
            for path, mtime in entries[1:]:
                if mtime >= cutoff: continue
                candidates.append({'path': str(path.relative_to(self.project)),
                                   'keep': str(keep.relative_to(self.project)), 'size': size,
                                   'sha256': checksum, 'mtime': mtime})
        result = {'time': datetime.now(timezone.utc).isoformat(), 'project': str(self.project),
                  'scanned': scanned, 'candidateBytes': sum(c['size'] for c in candidates),
                  'candidates': candidates, 'roots': list(self.CLEAN_ROOTS),
                  'policy': '仅归档七天前、哈希相同的生成文本。保留最新副本、资产、截图、存档、自动保存、编译输出与发布包。'}
        self.write_report('cleanup-preview', result)
        return result

    def safe_generated_path(self, relative):
        raw = Path(relative)
        if raw.is_absolute() or '..' in raw.parts:
            raise ValueError('拒绝越界路径')
        if any(self.PROTECTED_NAMES.search(part) for part in raw.parts[2:]):
            raise ValueError('备份、恢复信息和状态文件不属于自动清理范围')
        path = self.project / raw
        for ancestor in [path, *path.parents]:
            if ancestor == self.project: break
            if ancestor.is_symlink() or ancestor.is_junction(): raise ValueError('拒绝符号链接或 junction')
        resolved = path.resolve()
        if not any(resolved.is_relative_to((self.project / root).resolve()) for root in self.CLEAN_ROOTS):
            raise ValueError('路径不在生成数据白名单')
        return resolved

    def archive_duplicates(self, candidates):
        archive = self.state / 'Archive' / stamp()
        archive.mkdir(parents=True)
        journal = {'project': str(self.project), 'archive': str(archive), 'entries': []}
        journal_path = archive / 'manifest.json'
        save_json(journal_path, journal)
        for item in candidates:
            source = self.safe_generated_path(item['path'])
            keep = self.safe_generated_path(item['keep'])
            if source == keep: raise ValueError('不能归档保留副本')
            if source.suffix.lower() not in self.TEXT_SUFFIXES: raise ValueError('不支持的清理文件类型')
            if not source.is_file() or not keep.is_file(): continue
            if digest(source) != item['sha256'] or digest(keep) != item['sha256']:
                raise ValueError(f'文件已变化，停止归档：{source}')
            if source.stat().st_mtime > time.time() - 7 * 86400:
                raise ValueError(f'文件近期已变化：{source}')
            destination = archive / item['path']
            destination.parent.mkdir(parents=True, exist_ok=True)
            if destination.exists(): raise FileExistsError(destination)
            entry = {**item, 'status': 'planned'}
            journal['entries'].append(entry)
            save_json(journal_path, journal)
            shutil.move(str(source), str(destination))
            entry['status'] = 'archived'
            save_json(journal_path, journal)
        return {'manifest': str(journal_path), 'archived': len(journal['entries'])}

    def restore(self, manifest):
        manifest = Path(manifest).resolve()
        archive_root = (self.state / 'Archive').resolve()
        if not manifest.is_relative_to(archive_root) or manifest.name != 'manifest.json':
            raise ValueError('只能恢复本工作台的归档清单')
        journal = json.loads(manifest.read_text(encoding='utf-8'))
        if Path(journal['project']).resolve() != self.project: raise ValueError('归档属于其他项目')
        restored = 0
        for item in journal['entries']:
            if item['status'] == 'restored': continue
            target = self.safe_generated_path(item['path'])
            source = manifest.parent / item['path']
            if not source.resolve().is_relative_to(manifest.parent): raise ValueError('归档路径越界')
            if target.exists(): raise FileExistsError(f'原路径已有文件，拒绝覆盖：{target}')
            if not source.is_file() or digest(source) != item['sha256']: raise ValueError('归档文件缺失或哈希变化')
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(source), str(target))
            item['status'] = 'restored'
            restored += 1
            save_json(manifest, journal)
        return {'restored': restored, 'manifest': str(manifest)}

    def write_report(self, name, data):
        save_json(self.state / 'Reports' / f'{name}.json', data)
        # Local, offline report also serves as a compact UI direction reference.
        body = html.escape(json.dumps(data, ensure_ascii=False, indent=2))
        flow = ''.join(f'<section><h2>{html.escape(w["title"])}</h2><p>{html.escape(w["summary"])}</p><ol>' +
                       ''.join(f'<li>{html.escape(s["title"])}</li>' for s in w['steps']) + '</ol></section>'
                       for w in self.catalog['workflows'])
        page = f'''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dev01 工作流 · {html.escape(name)}</title><style>
body{{font:15px/1.65 system-ui,"Microsoft YaHei",sans-serif;margin:0;background:#121820;color:#e4eaf1}}main{{max-width:1180px;margin:auto;padding:36px}}
h1{{margin:0;font-size:30px}}p{{color:#a7bacd}}.grid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:16px}}section{{background:#1c2632;padding:20px;border-radius:10px}}h2{{font-size:18px;margin:0}}li{{padding:4px}}pre{{padding:20px;background:#0c1218;white-space:pre-wrap;overflow-wrap:anywhere;font-size:13px}}summary{{padding:18px;cursor:pointer}}
</style><main><h1>Dev01 工作流</h1><p>开发制作 → 本地验证 → 引擎/项目发布 → UGS 验收</p><div class="grid">{flow}</div>
<section style="margin-top:16px"><h2>构建与发布</h2><p>预检 → 源码编译 → 人工效果确认 → Installed Build → 固定 Engine CL → 云端 PCB → UGS 验收</p></section>
<details open><summary>{html.escape(name)} · 现场数据</summary><pre>{body}</pre></details></main></html>'''
        output = self.state / 'Reports' / f'{name}.html'
        output.write_text(page, encoding='utf-8')
        return output

    def commands(self, development=None):
        engine = Path(self.paths['sourceEngine'])
        pipeline = Path(self.paths['pipeline'])
        project_file = str(self.project / 'DevKit.uproject')
        development_file = str((development or self.project) / 'DevKit.uproject')
        ps = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']
        return {
            'source-build': [str(engine / 'Engine/Build/BatchFiles/Build.bat'), 'DevKitEditor', 'Win64', 'Development',
                             f'-Project={development_file}', '-WaitMutex', '-NoHotReload', '-NoUBA', '-MaxParallelActions=4', '-NoUBTMakefiles'],
            'pipeline-test': [*ps, str(pipeline / 'test_dev01_release_pipeline.ps1')],
            'installed-build': [*ps, str(pipeline / 'start_ue58_installed_build.ps1')],
            'source-editor': [str(engine / 'Engine/Binaries/Win64/UnrealEditor.exe'), development_file, '-ExecCmds=DevKit.Workflow.Open'],
            'ugs-editor': [str(self.project / 'Engine/Binaries/Win64/UnrealEditor.exe'), project_file],
            'ugs': [str(Path(self.paths['ugs']) / 'StartDev01UGS.cmd')],
            'pcb-verify': ['p4', '-v', 'net.maxwait=20', 'verify', '-q', '//Dev01Binaries/UGS/++Dev01+main-Editor.zip#head'],
        }

    def cloud_command(self, publish=False):
        host = self.settings.get('sshHost', '')
        if not re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_.@-]{0,150}', host):
            raise ValueError('请在工作台设置已配置公钥/known_hosts 的 SSH Host；不存储密码。')
        # PowerShell is encoded explicitly; the only user input is a constrained SSH host.
        import base64
        if publish:
            script = "$ErrorActionPreference='Stop'; & 'C:\\BuildAgent\\Dev01\\dev01_auto_ugs_build.ps1'; if (-not $?) { exit 1 }"
        else:
            script = """$ErrorActionPreference='Stop'
$p='C:\\BuildAgent\\Dev01'
Get-ChildItem -LiteralPath $p -Filter 'dev01_*.ps1' | Select-Object Name,LastWriteTime
Get-ScheduledTask | Where-Object TaskName -Like '*Dev01*' | Select-Object TaskName,State
$s=Join-Path $p 'state\\completed_build.json'
if(Test-Path -LiteralPath $s){Get-Content -LiteralPath $s -Raw}
Get-PSDrive C | Select-Object Used,Free
"""
        encoded = base64.b64encode(script.encode('utf-16-le')).decode('ascii')
        return ['ssh', '-o', 'BatchMode=yes', '-o', 'StrictHostKeyChecking=yes', '-o', 'ConnectTimeout=10',
                host, 'powershell.exe', '-NoProfile', '-EncodedCommand', encoded]

    def execute(self, action, emit=print):
        development = self.development_project() if action in {'source-build', 'source-editor'} else None
        if action in {'source-build', 'installed-build'}:
            active = run(['tasklist', '/FI', 'IMAGENAME eq UnrealEditor*', '/FO', 'CSV', '/NH'], self.project)
            if not active['ok'] or 'unrealeditor' in active['output'].lower():
                raise RuntimeError('请先保存并关闭 Unreal Editor，再从独立工作台构建。')
        if action == 'cloud-build-publish':
            result = self.preflight()
            if not result['ok']: raise RuntimeError('预检未通过：' + '；'.join(result['blockers']))
        if action == 'ugs-editor' and not self.build_ids()['ok']:
            raise RuntimeError('发布版本 BuildId 检查未通过，请先用 UGS 同步匹配 PCB。')
        if action == 'source-editor' and not self.build_ids(development, Path(self.paths['sourceEngine']) / 'Engine/Binaries/Win64/UnrealEditor.modules')['ok']:
            raise RuntimeError('开发副本与源码引擎模块尚未匹配。请先在“构建与发布”编译开发副本。')
        if action in {'source-editor', 'ugs-editor'}:
            # Single-instance policy applies across both engine roots for this project.
            check = run(['powershell.exe', '-NoProfile', '-Command',
                         "Get-CimInstance Win32_Process -Filter \"Name='UnrealEditor.exe'\" | Select-Object -ExpandProperty CommandLine"], self.project)
            if not check['ok']: raise RuntimeError('无法确认编辑器进程状态，停止重复启动。')
            launch_project = development if action == 'source-editor' else self.project
            if str(launch_project).lower() in check['output'].replace('/', '\\').lower():
                raise RuntimeError('此项目已有编辑器实例，请切回现有窗口。')
        args = self.cloud_command(action == 'cloud-build-publish') if action.startswith('cloud-') else self.commands(development)[action]
        self.state.mkdir(parents=True, exist_ok=True)
        lock = self.state / 'operation.lock'
        # Keep an OS lock, not a stale sentinel. Windows closes it on process termination.
        import msvcrt
        with lock.open('a+b') as handle, (engine_build_guard() if action in {'source-build', 'source-editor'} else nullcontext()):
            if handle.tell() == 0: handle.write(b'0'); handle.flush()
            handle.seek(0)
            try: msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError as exc: raise RuntimeError('另一项工作流操作正在运行。') from exc
            log = self.state / 'Runs' / f'{stamp()}-{action}.log'
            log.parent.mkdir(parents=True, exist_ok=True)
            job = {'action': action, 'status': 'running', 'started': datetime.now(timezone.utc).isoformat(), 'log': str(log)}
            save_json(log.with_suffix('.json'), job)
            emit('执行：' + subprocess.list2cmdline(args) if not action.startswith('cloud-') else '执行云端任务：' + action)
            try:
                if action in {'source-editor', 'ugs-editor', 'ugs'}:
                    subprocess.Popen(args, cwd=self.project, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                    job.update(status='launched', code=0)
                    emit('已发起启动；请在实际窗口确认启动结果。')
                else:
                    with log.open('w', encoding='utf-8') as stream:
                        proc = subprocess.Popen(args, cwd=self.project, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                                encoding='utf-8', errors='replace', creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                        for line in proc.stdout:
                            stream.write(line); stream.flush(); emit(line.rstrip())
                        code = proc.wait()
                    status = 'started' if action == 'installed-build' and code == 0 else ('succeeded' if code == 0 else 'failed')
                    job.update(status=status, code=code)
                    if status == 'started': emit('Installed Build 后台任务已开始；请查看输出中的 OutLog / ExitFile，退出码 0 才算完成。')
            except Exception as exc:
                job.update(status='failed', error=str(exc))
                raise
            finally:
                job['finished'] = datetime.now(timezone.utc).isoformat()
                save_json(log.with_suffix('.json'), job)
                handle.seek(0); msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            return job


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', required=True)
    parser.add_argument('action', choices=['inventory', 'preflight', 'buildids', 'cleanup-preview', 'archive-duplicates', 'restore'])
    parser.add_argument('--manifest')
    args = parser.parse_args()
    workflow = Workflow(args.project)
    if args.action == 'inventory': result = workflow.inventory(); workflow.write_report('inventory', result)
    elif args.action == 'preflight': result = workflow.preflight()
    elif args.action == 'buildids': result = workflow.build_ids(); workflow.write_report('buildids', result)
    elif args.action == 'cleanup-preview': result = workflow.cleanup_scan()
    elif args.action == 'archive-duplicates': result = workflow.archive_duplicates(workflow.cleanup_scan()['candidates'])
    else:
        if not args.manifest: parser.error('restore 需要 --manifest')
        result = workflow.restore(args.manifest)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 1 if result.get('ok') is False else 0


if __name__ == '__main__':
    sys.stdout.reconfigure(encoding='utf-8')
    raise SystemExit(main())
