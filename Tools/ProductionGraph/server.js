const http = require('http');
const fs = require('fs/promises');
const path = require('path');

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
