const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { createServer } = require('../server');

function makeTempProject() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'production-graph-server-'));
}

async function requestJson(url, options) {
  const response = await fetch(url, options);
  const body = await response.text();
  let json = null;
  if (body) {
    json = JSON.parse(body);
  }
  return { response, json };
}

async function run() {
  const projectRoot = makeTempProject();
  const app = createServer({ projectRoot, port: 0 });
  const server = await app.start();
  const port = server.address().port;
  const baseUrl = `http://127.0.0.1:${port}`;

  try {
    const listed = await requestJson(`${baseUrl}/api/graphs`);
    assert.strictEqual(listed.response.status, 200);
    assert.ok(Array.isArray(listed.json.graphs));
    assert.ok(listed.json.graphs.some((graph) => graph.id === 'tutorial-opening'));

    const graph = await requestJson(`${baseUrl}/api/graphs/tutorial-opening`);
    assert.strictEqual(graph.response.status, 200);
    assert.ok(graph.json.nodes.length >= 1);
    assert.ok(graph.json.edges.length >= 1);

    graph.json.nodes[0].title = 'Edited through API';
    const saved = await requestJson(`${baseUrl}/api/graphs/tutorial-opening`, {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify(graph.json)
    });
    assert.strictEqual(saved.response.status, 200);
    assert.strictEqual(saved.json.ok, true);

    const reloaded = await requestJson(`${baseUrl}/api/graphs/tutorial-opening`);
    assert.strictEqual(reloaded.json.nodes[0].title, 'Edited through API');
  } finally {
    await app.stop();
  }

  console.log('ProductionGraph server smoke passed');
}

run().catch((error) => {
  console.error(error);
  process.exit(1);
});
