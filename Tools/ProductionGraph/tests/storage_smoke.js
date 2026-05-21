const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { createStorage } = require('../storage');

function makeTempProject() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'production-graph-'));
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function run() {
  const projectRoot = makeTempProject();
  const storage = createStorage(projectRoot);

  const graph = {
    manifest: {
      id: 'tutorial-opening',
      title: 'Tutorial Opening',
      description: 'Smoke test graph',
      version: 1,
      createdAt: '2026-05-21T00:00:00.000Z',
      updatedAt: '2026-05-21T00:00:00.000Z',
      viewport: { x: 0, y: 0, zoom: 1 }
    },
    nodes: [
      {
        id: 'need-memory-anchor',
        type: 'need',
        title: 'Memory anchor rewind',
        description: 'Failure should rewind to the latest anchor.',
        status: 'open',
        owner: 'Design',
        tags: ['tutorial'],
        position: { x: 120, y: 160 },
        fields: {},
        createdAt: '2026-05-21T00:00:00.000Z',
        updatedAt: '2026-05-21T00:00:00.000Z'
      },
      {
        id: 'task-place-trigger',
        type: 'task',
        title: 'Place anchor trigger',
        description: 'Add trigger to the tutorial level.',
        status: 'todo',
        owner: 'Level',
        tags: ['trigger'],
        position: { x: 420, y: 160 },
        fields: { workType: 'Trigger' },
        createdAt: '2026-05-21T00:00:00.000Z',
        updatedAt: '2026-05-21T00:00:00.000Z'
      }
    ],
    edges: [
      {
        id: 'edge-need-task',
        type: 'creates_task',
        source: 'need-memory-anchor',
        target: 'task-place-trigger',
        label: 'creates task',
        createdAt: '2026-05-21T00:00:00.000Z',
        updatedAt: '2026-05-21T00:00:00.000Z'
      }
    ]
  };

  storage.saveGraph('tutorial-opening', graph);

  const loaded = storage.loadGraph('tutorial-opening');
  assert.strictEqual(loaded.manifest.id, 'tutorial-opening');
  assert.strictEqual(loaded.nodes.length, 2);
  assert.strictEqual(loaded.edges.length, 1);

  const nodePath = path.join(projectRoot, 'Docs', 'ProductionGraph', 'tutorial-opening', 'nodes', 'task-place-trigger.json');
  assert.ok(fs.existsSync(nodePath), 'node file should be written separately');
  assert.strictEqual(readJson(nodePath).title, 'Place anchor trigger');

  storage.saveGraph('tutorial-opening', {
    ...graph,
    nodes: graph.nodes.slice(0, 1),
    edges: []
  });

  const trimmed = storage.loadGraph('tutorial-opening');
  assert.strictEqual(trimmed.nodes.length, 1);
  assert.strictEqual(trimmed.edges.length, 0);
  assert.ok(!fs.existsSync(nodePath), 'removed nodes should remove stale files');

  storage.ensureSampleGraph();
  const graphs = storage.listGraphs();
  assert.ok(graphs.some((item) => item.id === 'tutorial-opening'), 'sample graph should be listed');

  console.log('ProductionGraph storage smoke passed');
}

run();
