const http = require('http');
const fs = require('fs/promises');
const fsSync = require('fs');
const path = require('path');
const { spawn } = require('child_process');
const storyPipeline = require('./story_pipeline');
const storyMetadata = require('./story_metadata');

const ROOT = path.resolve(__dirname, '..', '..');
const PUBLIC_DIR = path.join(__dirname, 'public');
const DATA_DIR = path.join(ROOT, 'Docs', 'ProductionGraph');

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8'
};

function sampleGraph() {
  const timestamp = new Date().toISOString();
  return {
    manifest: {
      id: 'tutorial-opening',
      title: '开局教程制作图谱',
      updatedAt: timestamp
    },
    nodes: [
      {
        id: 'need-memory-anchor',
        type: 'need',
        title: '训练段状态需要结构化',
        description: '每个训练段需要一个稳定 ID，用于恢复敌人、门、目标提示和已完成状态。',
        status: 'open',
        owner: 'Design',
        tags: ['state'],
        position: { x: 160, y: 240 },
        fields: {},
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'solution-level-flow',
        type: 'solution',
        title: '用 LevelFlow 播放回卷演出',
        description: '失败事件广播到 StoryEngine，LevelFlow 播放画面破碎和回卷表现，然后恢复训练段状态。',
        status: 'draft',
        owner: 'Design',
        tags: ['level-flow', 'story-event'],
        position: { x: 520, y: 120 },
        fields: {},
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'task-anchor-trigger',
        type: 'task',
        title: '场景放置 Anchor Trigger',
        description: '在每个训练段入口放置锚点触发器，记录玩家当前训练段。',
        status: 'todo',
        owner: 'Level',
        tags: ['trigger'],
        position: { x: 900, y: 100 },
        fields: {},
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'blocker-save-state',
        type: 'blocker',
        title: '锚点需要保存训练段状态',
        description: '如果只保存位置，不知道应恢复哪些敌人和机关状态。',
        status: 'open',
        owner: 'Design',
        tags: ['blocker'],
        position: { x: 880, y: 380 },
        fields: {},
        createdAt: timestamp,
        updatedAt: timestamp
      }
    ],
    edges: [
      {
        id: 'edge-need-solution',
        type: 'has_solution',
        source: 'need-memory-anchor',
        target: 'solution-level-flow',
        sourcePin: 'output',
        targetPin: 'input',
        label: '产生方案',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-solution-task',
        type: 'creates_task',
        source: 'solution-level-flow',
        target: 'task-anchor-trigger',
        sourcePin: 'output',
        targetPin: 'input',
        label: '拆出任务',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-need-blocker',
        type: 'blocked_by',
        source: 'need-memory-anchor',
        target: 'blocker-save-state',
        sourcePin: 'output',
        targetPin: 'input',
        label: '阻塞',
        createdAt: timestamp,
        updatedAt: timestamp
      }
    ]
  };
}

async function ensureSampleGraph() {
  const graphPath = graphFile('tutorial-opening');
  try {
    await fs.access(graphPath);
  } catch {
    await fs.mkdir(path.dirname(graphPath), { recursive: true });
    await fs.writeFile(graphPath, JSON.stringify(sampleGraph(), null, 2), 'utf8');
  }
}

function graphFile(id) {
  return path.join(DATA_DIR, id, 'graph.json');
}

async function readGraph(id) {
  const text = await fs.readFile(graphFile(id), 'utf8');
  return JSON.parse(text);
}

async function writeGraph(id, graph) {
  await fs.mkdir(path.dirname(graphFile(id)), { recursive: true });
  await fs.writeFile(graphFile(id), JSON.stringify(graph, null, 2), 'utf8');
}

async function listGraphs() {
  await ensureSampleGraph();
  const entries = await fs.readdir(DATA_DIR, { withFileTypes: true });
  const graphs = [];
  for (const entry of entries) {
    if (!entry.isDirectory()) {
      continue;
    }
    try {
      const graph = await readGraph(entry.name);
      graphs.push({
        id: graph.manifest.id,
        title: graph.manifest.title,
        nodeCount: graph.nodes.length,
        updatedAt: graph.manifest.updatedAt
      });
    } catch {
      // Skip malformed graph folders so one bad draft does not break the tool.
    }
  }
  graphs.sort((a, b) => String(b.updatedAt || '').localeCompare(String(a.updatedAt || '')));
  return graphs;
}

async function readBody(request) {
  const chunks = [];
  for await (const chunk of request) {
    chunks.push(chunk);
  }
  return Buffer.concat(chunks).toString('utf8');
}

function findCodexCli() {
  const candidates = [
    process.env.CODEX_CLI_PATH,
    path.join(process.env.LOCALAPPDATA || '', 'OpenAI', 'Codex', 'bin', 'codex.exe'),
    path.join(
      process.env.LOCALAPPDATA || '',
      'Packages',
      'OpenAI.Codex_2p2nqsd0c76g0',
      'LocalCache',
      'Local',
      'OpenAI',
      'Codex',
      'bin',
      'codex.exe'
    ),
    'codex'
  ].filter(Boolean);
  for (const candidate of candidates) {
    if (candidate === 'codex' || fsSync.existsSync(candidate)) {
      return candidate;
    }
  }
  throw new Error('Codex CLI not found. Set CODEX_CLI_PATH to codex.exe.');
}

function findUnrealEditorCmd() {
  const candidates = [
    process.env.UNREAL_EDITOR_CMD,
    process.env.UE_EDITOR_CMD,
    path.join('Z:', 'GZA_Software', 'RealityCapture', 'UE_5.4', 'Engine', 'Binaries', 'Win64', 'UnrealEditor-Cmd.exe'),
    path.join('C:', 'Program Files', 'Epic Games', 'UE_5.4', 'Engine', 'Binaries', 'Win64', 'UnrealEditor-Cmd.exe'),
    'UnrealEditor-Cmd.exe'
  ].filter(Boolean);
  for (const candidate of candidates) {
    if (candidate === 'UnrealEditor-Cmd.exe' || fsSync.existsSync(candidate)) {
      return candidate;
    }
  }
  throw new Error('UnrealEditor-Cmd.exe not found. Set UNREAL_EDITOR_CMD to the editor commandlet executable.');
}

async function runCodexPrompt(prompt, runId) {
  const codexPath = findCodexCli();
  const runDir = path.join(ROOT, 'Docs', 'StoryPipeline', 'Runs');
  await fs.mkdir(runDir, { recursive: true });
  const outputFile = path.join(runDir, `${runId}.final.md`);
  const logFile = path.join(runDir, `${runId}.log.txt`);
  const args = [
    'exec',
    '-C',
    ROOT,
    '-s',
    'workspace-write',
    '-a',
    'never',
    '-o',
    outputFile,
    '-'
  ];

  return new Promise((resolve, reject) => {
    const child = spawn(codexPath, args, {
      cwd: ROOT,
      windowsHide: true,
      stdio: ['pipe', 'pipe', 'pipe']
    });
    let stdout = '';
    let stderr = '';
    const timer = setTimeout(() => {
      child.kill();
      reject(new Error('Codex run timed out after 10 minutes.'));
    }, 10 * 60 * 1000);
    child.stdout.on('data', (chunk) => {
      stdout += chunk.toString();
    });
    child.stderr.on('data', (chunk) => {
      stderr += chunk.toString();
    });
    child.on('error', (error) => {
      clearTimeout(timer);
      reject(error);
    });
    child.on('close', async (code) => {
      clearTimeout(timer);
      const logText = [
        `Command: ${codexPath} ${args.join(' ')}`,
        `ExitCode: ${code}`,
        '',
        '--- stdout ---',
        stdout,
        '',
        '--- stderr ---',
        stderr
      ].join('\n');
      await fs.writeFile(logFile, logText, 'utf8');
      if (code !== 0) {
        reject(new Error(`Codex exited with ${code}. See ${path.relative(ROOT, logFile)}`));
        return;
      }
      resolve({
        exitCode: code,
        outputFile: path.relative(ROOT, outputFile).replace(/\\/g, '/'),
        logFile: path.relative(ROOT, logFile).replace(/\\/g, '/')
      });
    });
    child.stdin.end(prompt);
  });
}

function uniqueLines(lines) {
  return Array.from(new Set(lines.filter(Boolean)));
}

function analyzeStoryImportOutput(stdout) {
  const lines = stdout.split(/\r?\n/);
  const storyImportLines = lines.filter((line) => line.includes('[StoryImport]') || line.includes('LogTemp: Display: - '));
  const projectErrors = uniqueLines(lines
    .filter((line) => line.includes('Error:') && !line.includes('[StoryImport]'))
    .map((line) => line.replace(/^\[[^\]]+\]\[[^\]]+\]/, '').trim()));
  const storyImportFailures = uniqueLines(storyImportLines
    .filter((line) => /\bFailed\b|\bSkipped\b|Error:/i.test(line))
    .map((line) => line.replace(/^\[[^\]]+\]\[[^\]]+\]/, '').trim()));
  const countLines = (needle) => storyImportLines.filter((line) => line.includes(needle)).length;
  const started = storyImportLines.some((line) => line.includes('[StoryImport] Mode='));
  const touchedCount =
    countLines('Would write point') +
    countLines('Would write graph') +
    countLines('Created point') +
    countLines('Created graph') +
    countLines('Updated point') +
    countLines('Updated graph');
  return {
    started,
    storyImportOk: started && storyImportFailures.length === 0 && touchedCount > 0,
    storyImportFailures,
    projectErrors,
    counts: {
      wouldWritePoints: countLines('Would write point'),
      wouldWriteGraphs: countLines('Would write graph'),
      createdPoints: countLines('Created point'),
      createdGraphs: countLines('Created graph'),
      updatedPoints: countLines('Updated point'),
      updatedGraphs: countLines('Updated graph')
    }
  };
}

async function runStoryImportCommandlet({ apply = false, manifestPath = 'Docs/StoryPipeline/StoryImportManifest.json' } = {}) {
  const editorCmd = findUnrealEditorCmd();
  const runDir = path.join(ROOT, 'Docs', 'StoryPipeline', 'Runs');
  await fs.mkdir(runDir, { recursive: true });
  const runId = `${new Date().toISOString().replace(/[:.]/g, '-')}-story-import-${apply ? 'apply' : 'dry-run'}`;
  const logFile = path.join(runDir, `${runId}.log.txt`);
  const manifestFullPath = path.resolve(ROOT, manifestPath);
  const args = [
    path.join(ROOT, 'DevKit.uproject'),
    '-run=StoryImport',
    `Manifest=${manifestFullPath}`,
    '-unattended',
    '-nop4'
  ];
  if (apply) {
    args.splice(2, 0, 'Apply');
  }

  return new Promise((resolve, reject) => {
    const child = spawn(editorCmd, args, {
      cwd: ROOT,
      windowsHide: true,
      stdio: ['ignore', 'pipe', 'pipe']
    });
    let stdout = '';
    let stderr = '';
    const timer = setTimeout(() => {
      child.kill();
      reject(new Error('StoryImport commandlet timed out after 15 minutes.'));
    }, 15 * 60 * 1000);
    child.stdout.on('data', (chunk) => {
      stdout += chunk.toString();
    });
    child.stderr.on('data', (chunk) => {
      stderr += chunk.toString();
    });
    child.on('error', (error) => {
      clearTimeout(timer);
      reject(error);
    });
    child.on('close', async (code) => {
      clearTimeout(timer);
      const analysis = analyzeStoryImportOutput(stdout);
      const logText = [
        `Command: ${editorCmd} ${args.join(' ')}`,
        `ExitCode: ${code}`,
        '',
        '--- stdout ---',
        stdout,
        '',
        '--- stderr ---',
        stderr
      ].join('\n');
      await fs.writeFile(logFile, logText, 'utf8');
      resolve({
        exitCode: code,
        ok: code === 0,
        mode: apply ? 'Apply' : 'DryRun',
        manifestFile: path.relative(ROOT, manifestFullPath).replace(/\\/g, '/'),
        logFile: path.relative(ROOT, logFile).replace(/\\/g, '/'),
        storyImportOk: analysis.storyImportOk,
        storyImportCounts: analysis.counts,
        storyImportFailures: analysis.storyImportFailures,
        projectErrors: analysis.projectErrors,
        warning: code !== 0 && analysis.storyImportOk
          ? 'StoryImport completed, but Unreal exited with project-level errors. Check projectErrors and the log.'
          : '',
        error: code === 0
          ? ''
          : analysis.storyImportOk
            ? `Unreal exited with ${code} after StoryImport completed. Check projectErrors and the log.`
            : `StoryImport exited with ${code}. Check the log for project-level errors.`
      });
    });
  });
}

async function runStoryMetadataExportCommandlet() {
  const editorCmd = findUnrealEditorCmd();
  const runDir = path.join(ROOT, 'Docs', 'StoryPipeline', 'Runs');
  await fs.mkdir(runDir, { recursive: true });
  const runId = `${new Date().toISOString().replace(/[:.]/g, '-')}-story-metadata-export`;
  const logFile = path.join(runDir, `${runId}.log.txt`);
  const outputPath = path.join(ROOT, 'Docs', 'StoryPipeline', 'Metadata', 'ue_project_assets.json');
  const args = [
    path.join(ROOT, 'DevKit.uproject'),
    '-run=StoryMetadataExport',
    `Output=${outputPath}`,
    '-unattended',
    '-nop4'
  ];

  return new Promise((resolve, reject) => {
    const child = spawn(editorCmd, args, {
      cwd: ROOT,
      windowsHide: true,
      stdio: ['ignore', 'pipe', 'pipe']
    });
    let stdout = '';
    let stderr = '';
    const timer = setTimeout(() => {
      child.kill();
      reject(new Error('StoryMetadataExport commandlet timed out after 15 minutes.'));
    }, 15 * 60 * 1000);
    child.stdout.on('data', (chunk) => {
      stdout += chunk.toString();
    });
    child.stderr.on('data', (chunk) => {
      stderr += chunk.toString();
    });
    child.on('error', (error) => {
      clearTimeout(timer);
      reject(error);
    });
    child.on('close', async (code) => {
      clearTimeout(timer);
      const logText = [
        `Command: ${editorCmd} ${args.join(' ')}`,
        `ExitCode: ${code}`,
        '',
        '--- stdout ---',
        stdout,
        '',
        '--- stderr ---',
        stderr
      ].join('\n');
      await fs.writeFile(logFile, logText, 'utf8');
      resolve({
        exitCode: code,
        ok: code === 0,
        outputFile: path.relative(ROOT, outputPath).replace(/\\/g, '/'),
        logFile: path.relative(ROOT, logFile).replace(/\\/g, '/'),
        error: code === 0 ? '' : `StoryMetadataExport exited with ${code}. Check the log for project-level errors.`
      });
    });
  });
}

function buildStoryCodexPrompt(request) {
  return [
    '你是这个项目的剧情管线整理助手。请严格处理请求文件中的剧情源数据。',
    '',
    '目标：',
    '- 读取 sourceFiles 中列出的 .story.json。',
    '- 如果 metadataFile 存在，读取它并用于校验地图、tag、RoomData、卡牌和 Story 资产候选。',
    '- 检查并整理剧情字段、tag、剧情点、Actions、工作量。',
    '- 可以更新 Docs/StoryPipeline/StoryImportManifest.json、Docs/StoryPipeline/StoryWorkload.md、Docs/StoryPipeline/ValidationReport.md。',
    '- 不要修改 UE 地图，不要运行 StoryImport Apply，不要改无关源码。',
    '- 保留 schemaVersion、extra 和未知字段。',
    '',
    '请求：',
    JSON.stringify(request, null, 2)
  ].join('\n');
}

function send(response, status, body, type = 'application/json; charset=utf-8') {
  response.writeHead(status, { 'content-type': type });
  response.end(body);
}

async function serveStatic(request, response) {
  const url = new URL(request.url, 'http://localhost');
  const pathname = url.pathname === '/' ? '/index.html' : url.pathname;
  const filePath = path.resolve(PUBLIC_DIR, `.${pathname}`);
  if (!filePath.startsWith(PUBLIC_DIR)) {
    send(response, 403, 'Forbidden', 'text/plain; charset=utf-8');
    return;
  }
  const data = await fs.readFile(filePath);
  send(response, 200, data, MIME[path.extname(filePath)] || 'application/octet-stream');
}

async function handleApi(request, response) {
  const url = new URL(request.url, 'http://localhost');
  const parts = url.pathname.split('/').filter(Boolean);
  if (request.method === 'GET' && url.pathname === '/api/story/default') {
    send(response, 200, JSON.stringify({ segment: storyPipeline.defaultSegment() }));
    return;
  }
  if (request.method === 'GET' && url.pathname === '/api/story/metadata') {
    const metadata = storyMetadata.readMetadata(ROOT);
    send(response, 200, JSON.stringify({
      metadata,
      metadataFile: storyMetadata.metadataFileForRequest(ROOT),
      exists: Boolean(metadata)
    }));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/sync-metadata') {
    send(response, 200, JSON.stringify(storyMetadata.syncMetadata(ROOT)));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/sync-ue-metadata') {
    const commandlet = await runStoryMetadataExportCommandlet();
    const metadata = storyMetadata.syncMetadata(ROOT);
    send(response, 200, JSON.stringify({ commandlet, ...metadata }));
    return;
  }
  if (request.method === 'GET' && url.pathname === '/api/story/segments') {
    send(response, 200, JSON.stringify({ segments: storyPipeline.listSegments(ROOT) }));
    return;
  }
  if (request.method === 'GET' && url.pathname === '/api/story/segment') {
    const arc = url.searchParams.get('arc');
    const storyId = url.searchParams.get('storyId');
    send(response, 200, JSON.stringify({ segment: storyPipeline.readSegment(ROOT, arc, storyId) }));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/segment') {
    const body = JSON.parse(await readBody(request));
    send(response, 200, JSON.stringify(storyPipeline.writeSegment(ROOT, body.segment || body)));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/codex-request') {
    const body = JSON.parse(await readBody(request));
    send(response, 200, JSON.stringify(storyPipeline.createCodexRequest(ROOT, body)));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/run-codex') {
    const body = JSON.parse(await readBody(request));
    const requestResult = storyPipeline.createCodexRequest(ROOT, body);
    const runId = requestResult.request.id;
    const runResult = await runCodexPrompt(buildStoryCodexPrompt(requestResult.request), runId);
    send(response, 200, JSON.stringify({ ...requestResult, run: runResult }));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/export') {
    const body = JSON.parse(await readBody(request));
    send(response, 200, JSON.stringify(storyPipeline.exportPipeline(ROOT, body.sourceFiles || [])));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/story/import-ue') {
    const body = JSON.parse(await readBody(request));
    const result = await runStoryImportCommandlet({
      apply: body.apply === true,
      manifestPath: body.manifestPath || 'Docs/StoryPipeline/StoryImportManifest.json'
    });
    send(response, 200, JSON.stringify({ import: result }));
    return;
  }
  if (request.method === 'GET' && url.pathname === '/api/graphs') {
    send(response, 200, JSON.stringify({ graphs: await listGraphs() }));
    return;
  }
  if (request.method === 'POST' && url.pathname === '/api/graphs') {
    const body = JSON.parse(await readBody(request));
    const id = body.id;
    const timestamp = new Date().toISOString();
    const graph = {
      manifest: { id, title: body.title || id, updatedAt: timestamp },
      nodes: [],
      edges: []
    };
    await writeGraph(id, graph);
    send(response, 200, JSON.stringify({ graph }));
    return;
  }
  if (parts[0] === 'api' && parts[1] === 'graphs' && parts[2]) {
    const id = parts[2];
    if (request.method === 'GET') {
      send(response, 200, JSON.stringify(await readGraph(id)));
      return;
    }
    if (request.method === 'POST') {
      const graph = JSON.parse(await readBody(request));
      await writeGraph(id, graph);
      send(response, 200, JSON.stringify({ graph }));
      return;
    }
  }
  send(response, 404, JSON.stringify({ error: 'Not found' }));
}

async function handle(request, response) {
  try {
    if (request.url.startsWith('/api/')) {
      await handleApi(request, response);
      return;
    }
    await serveStatic(request, response);
  } catch (error) {
    send(response, 500, JSON.stringify({ error: error.message }));
  }
}

const portArgIndex = process.argv.indexOf('--port');
const port = portArgIndex >= 0 ? Number(process.argv[portArgIndex + 1]) : 4783;

http.createServer(handle).listen(port, '127.0.0.1', () => {
  console.log(`ProductionGraph running at http://localhost:${port}`);
});
