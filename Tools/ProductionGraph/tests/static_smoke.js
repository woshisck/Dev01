const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const { createServer } = require('../server');

function makeTempProject() {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'production-graph-static-'));
}

async function run() {
  const app = createServer({ projectRoot: makeTempProject(), port: 0 });
  const server = await app.start();
  const port = server.address().port;

  try {
    const response = await fetch(`http://127.0.0.1:${port}/`);
    const body = await response.text();
    assert.strictEqual(response.status, 200);
    assert.ok(body.includes('Production Graph'), 'home page should include app title');

    const script = await fetch(`http://127.0.0.1:${port}/app.js`);
    assert.strictEqual(script.status, 200);
  } finally {
    await app.stop();
  }

  console.log('ProductionGraph static smoke passed');
}

run().catch((error) => {
  console.error(error);
  process.exit(1);
});
