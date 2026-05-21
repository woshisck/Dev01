const fs = require('fs');
const path = require('path');

const GRAPH_ID_PATTERN = /^[a-z0-9][a-z0-9-]*$/;
const NODE_TYPES = new Set(['need', 'solution', 'task', 'blocker']);
const EDGE_TYPES = new Set([
  'has_solution',
  'creates_task',
  'depends_on',
  'blocked_by',
  'becomes_need',
  'changes_scope'
]);

function assertId(id, label) {
  if (typeof id !== 'string' || !GRAPH_ID_PATTERN.test(id)) {
    throw new Error(`${label} must use lowercase letters, numbers, and hyphens: ${id}`);
  }
}

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function writeJson(filePath, value) {
  ensureDir(path.dirname(filePath));
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`, 'utf8');
}

function listJson(dirPath) {
  if (!fs.existsSync(dirPath)) {
    return [];
  }
  return fs
    .readdirSync(dirPath)
    .filter((name) => name.endsWith('.json'))
    .sort()
    .map((name) => path.join(dirPath, name));
}

function removeStaleJson(dirPath, expectedIds) {
  if (!fs.existsSync(dirPath)) {
    return;
  }
  const expectedFiles = new Set([...expectedIds].map((id) => `${id}.json`));
  for (const fileName of fs.readdirSync(dirPath)) {
    if (fileName.endsWith('.json') && !expectedFiles.has(fileName)) {
      fs.unlinkSync(path.join(dirPath, fileName));
    }
  }
}

function normalizeNode(node) {
  assertId(node.id, 'node id');
  if (!NODE_TYPES.has(node.type)) {
    throw new Error(`Unknown node type for ${node.id}: ${node.type}`);
  }
  return {
    id: node.id,
    type: node.type,
    title: String(node.title || 'Untitled node'),
    description: String(node.description || ''),
    status: String(node.status || 'open'),
    owner: String(node.owner || ''),
    tags: Array.isArray(node.tags) ? node.tags.map(String) : [],
    position: {
      x: Number(node.position?.x || 0),
      y: Number(node.position?.y || 0)
    },
    fields: node.fields && typeof node.fields === 'object' ? node.fields : {},
    createdAt: String(node.createdAt || new Date().toISOString()),
    updatedAt: String(node.updatedAt || new Date().toISOString())
  };
}

function normalizeEdge(edge, nodeIds) {
  assertId(edge.id, 'edge id');
  if (!EDGE_TYPES.has(edge.type)) {
    throw new Error(`Unknown edge type for ${edge.id}: ${edge.type}`);
  }
  assertId(edge.source, 'edge source');
  assertId(edge.target, 'edge target');
  if (!nodeIds.has(edge.source)) {
    throw new Error(`Edge ${edge.id} source does not exist: ${edge.source}`);
  }
  if (!nodeIds.has(edge.target)) {
    throw new Error(`Edge ${edge.id} target does not exist: ${edge.target}`);
  }
  return {
    id: edge.id,
    type: edge.type,
    source: edge.source,
    target: edge.target,
    label: String(edge.label || ''),
    createdAt: String(edge.createdAt || new Date().toISOString()),
    updatedAt: String(edge.updatedAt || new Date().toISOString())
  };
}

function normalizeGraph(id, graph) {
  assertId(id, 'graph id');
  const now = new Date().toISOString();
  const nodes = Array.isArray(graph.nodes) ? graph.nodes.map(normalizeNode) : [];
  const nodeIds = new Set(nodes.map((node) => node.id));
  if (nodeIds.size !== nodes.length) {
    throw new Error('Graph contains duplicate node ids');
  }
  const edges = Array.isArray(graph.edges)
    ? graph.edges.map((edge) => normalizeEdge(edge, nodeIds))
    : [];
  const edgeIds = new Set(edges.map((edge) => edge.id));
  if (edgeIds.size !== edges.length) {
    throw new Error('Graph contains duplicate edge ids');
  }
  const manifest = {
    id,
    title: String(graph.manifest?.title || id),
    description: String(graph.manifest?.description || ''),
    version: Number(graph.manifest?.version || 1),
    createdAt: String(graph.manifest?.createdAt || now),
    updatedAt: String(graph.manifest?.updatedAt || now),
    viewport: {
      x: Number(graph.manifest?.viewport?.x || 0),
      y: Number(graph.manifest?.viewport?.y || 0),
      zoom: Number(graph.manifest?.viewport?.zoom || 1)
    }
  };
  return { manifest, nodes, edges };
}

function createSampleGraph() {
  const timestamp = '2026-05-21T00:00:00.000Z';
  return {
    manifest: {
      id: 'tutorial-opening',
      title: '开局教程制作图',
      description: '记忆碎片教程从需求到任务、阻塞和新需求的示例图。',
      version: 1,
      createdAt: timestamp,
      updatedAt: timestamp,
      viewport: { x: 0, y: 0, zoom: 1 }
    },
    nodes: [
      {
        id: 'need-memory-anchor',
        type: 'need',
        title: '失败回到记忆锚点',
        description: '玩家在记忆碎片教程普通失败后，应回到最近的训练段锚点，而不是进入正式死亡循环。',
        status: 'open',
        owner: 'Design',
        tags: ['tutorial', 'memory-fragment'],
        position: { x: 120, y: 140 },
        fields: { priority: 'high' },
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'solution-level-flow-rewind',
        type: 'solution',
        title: '用 LevelFlow 播放回卷演出',
        description: '失败事件广播到 StoryEngine，LevelFlow 播放画面破碎和回卷表现，然后恢复训练段状态。',
        status: 'draft',
        owner: 'Design',
        tags: ['level-flow', 'story-event'],
        position: { x: 420, y: 100 },
        fields: { storyEvent: 'Story.Event.MemoryTutorial.PlayerFailed' },
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'task-place-anchor-trigger',
        type: 'task',
        title: '场景放置 Anchor Trigger',
        description: '在每个训练段入口放置锚点触发器，记录玩家当前训练段。',
        status: 'todo',
        owner: 'Level',
        tags: ['trigger', 'level'],
        position: { x: 740, y: 80 },
        fields: { workType: 'Scene Trigger', verification: 'PIE enter trigger and fail once' },
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'task-config-story-rule',
        type: 'task',
        title: '配置失败回退 StoryRule',
        description: '配置 PlayerFailed 事件命中教程失败回退规则，并触发 LevelFlow。',
        status: 'todo',
        owner: 'Tech Design',
        tags: ['story-rule'],
        position: { x: 720, y: 280 },
        fields: { workType: 'StoryRule', storyEvent: 'Story.Event.MemoryTutorial.PlayerFailed' },
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'blocker-anchor-state',
        type: 'blocker',
        title: '锚点需要保存训练段状态',
        description: '如果只保存位置，不知道应恢复哪组敌人和哪些门状态。',
        status: 'open',
        owner: 'Design',
        tags: ['blocker', 'state'],
        position: { x: 450, y: 420 },
        fields: { discoveredFrom: 'task-place-anchor-trigger' },
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'need-training-state',
        type: 'need',
        title: '训练段状态需要结构化',
        description: '每个训练段需要一个稳定 ID，用于恢复敌人、门、目标提示和已完成状态。',
        status: 'open',
        owner: 'Design',
        tags: ['follow-up', 'state'],
        position: { x: 120, y: 430 },
        fields: { generatedFrom: 'blocker-anchor-state' },
        createdAt: timestamp,
        updatedAt: timestamp
      }
    ],
    edges: [
      {
        id: 'edge-anchor-solution',
        type: 'has_solution',
        source: 'need-memory-anchor',
        target: 'solution-level-flow-rewind',
        label: '解决方案',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-solution-trigger-task',
        type: 'creates_task',
        source: 'solution-level-flow-rewind',
        target: 'task-place-anchor-trigger',
        label: '拆出任务',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-solution-rule-task',
        type: 'creates_task',
        source: 'solution-level-flow-rewind',
        target: 'task-config-story-rule',
        label: '拆出任务',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-trigger-blocker',
        type: 'blocked_by',
        source: 'task-place-anchor-trigger',
        target: 'blocker-anchor-state',
        label: '被阻塞',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-blocker-new-need',
        type: 'becomes_need',
        source: 'blocker-anchor-state',
        target: 'need-training-state',
        label: '转成新需求',
        createdAt: timestamp,
        updatedAt: timestamp
      },
      {
        id: 'edge-state-changes-rule',
        type: 'changes_scope',
        source: 'need-training-state',
        target: 'task-config-story-rule',
        label: '改变范围',
        createdAt: timestamp,
        updatedAt: timestamp
      }
    ]
  };
}

function createStorage(projectRoot) {
  const root = path.resolve(projectRoot);
  const graphRoot = path.join(root, 'Docs', 'ProductionGraph');

  function graphDir(id) {
    assertId(id, 'graph id');
    return path.join(graphRoot, id);
  }

  function loadGraph(id) {
    const dir = graphDir(id);
    const manifestPath = path.join(dir, 'manifest.json');
    if (!fs.existsSync(manifestPath)) {
      throw new Error(`Graph not found: ${id}`);
    }
    return {
      manifest: readJson(manifestPath),
      nodes: listJson(path.join(dir, 'nodes')).map(readJson),
      edges: listJson(path.join(dir, 'edges')).map(readJson)
    };
  }

  function saveGraph(id, graph) {
    const normalized = normalizeGraph(id, graph);
    const dir = graphDir(id);
    const nodeDir = path.join(dir, 'nodes');
    const edgeDir = path.join(dir, 'edges');

    ensureDir(nodeDir);
    ensureDir(edgeDir);
    writeJson(path.join(dir, 'manifest.json'), normalized.manifest);

    const nodeIds = new Set();
    for (const node of normalized.nodes) {
      nodeIds.add(node.id);
      writeJson(path.join(nodeDir, `${node.id}.json`), node);
    }

    const edgeIds = new Set();
    for (const edge of normalized.edges) {
      edgeIds.add(edge.id);
      writeJson(path.join(edgeDir, `${edge.id}.json`), edge);
    }

    removeStaleJson(nodeDir, nodeIds);
    removeStaleJson(edgeDir, edgeIds);
    return normalized;
  }

  function listGraphs() {
    if (!fs.existsSync(graphRoot)) {
      return [];
    }
    return fs
      .readdirSync(graphRoot, { withFileTypes: true })
      .filter((entry) => entry.isDirectory())
      .map((entry) => entry.name)
      .filter((id) => GRAPH_ID_PATTERN.test(id))
      .filter((id) => fs.existsSync(path.join(graphRoot, id, 'manifest.json')))
      .map((id) => {
        const graph = loadGraph(id);
        return {
          id,
          title: graph.manifest.title,
          description: graph.manifest.description,
          updatedAt: graph.manifest.updatedAt,
          nodeCount: graph.nodes.length,
          edgeCount: graph.edges.length
        };
      })
      .sort((a, b) => a.title.localeCompare(b.title));
  }

  function ensureSampleGraph() {
    const sampleId = 'tutorial-opening';
    if (!fs.existsSync(path.join(graphRoot, sampleId, 'manifest.json'))) {
      saveGraph(sampleId, createSampleGraph());
    }
  }

  return {
    graphRoot,
    listGraphs,
    loadGraph,
    saveGraph,
    ensureSampleGraph
  };
}

module.exports = {
  createStorage,
  createSampleGraph,
  NODE_TYPES,
  EDGE_TYPES
};
