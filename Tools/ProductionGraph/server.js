const fs = require('fs');
const http = require('http');
const path = require('path');
const { createStorage, createSampleGraph } = require('./storage');

const DEFAULT_PORT = 4783;
const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml; charset=utf-8'
};

function parseArgs(argv) {
  const result = { port: DEFAULT_PORT, projectRoot: process.cwd() };
  for (let i = 0; i < argv.length; i += 1) {
    const item = argv[i];
    if (item === '--port') {
      result.port = Number(argv[i + 1] || DEFAULT_PORT);
      i += 1;
    } else if (item === '--project-root') {
      result.projectRoot = path.resolve(argv[i + 1] || process.cwd());
      i += 1;
    }
  }
  return result;
}

function sendJson(response, status, payload) {
  response.writeHead(status, { 'content-type': 'application/json; charset=utf-8' });
  response.end(`${JSON.stringify(payload, null, 2)}\n`);
}

function sendText(response, status, text) {
  response.writeHead(status, { 'content-type': 'text/plain; charset=utf-8' });
  response.end(text);
}

function readRequestBody(request) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    request.on('data', (chunk) => chunks.push(chunk));
    request.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
    request.on('error', reject);
  });
}

function createServer(options = {}) {
  const projectRoot = path.resolve(options.projectRoot || process.cwd());
  const port = Number(options.port ?? DEFAULT_PORT);
  const publicDir = path.join(__dirname, 'public');
  const storage = createStorage(projectRoot);
  let server = null;

  async function handleApi(request, response, url) {
    storage.ensureSampleGraph();
    const parts = url.pathname.split('/').filter(Boolean);

    if (request.method === 'GET' && url.pathname === '/api/graphs') {
      sendJson(response, 200, { graphs: storage.listGraphs() });
      return;
    }

    if (request.method === 'POST' && url.pathname === '/api/graphs') {
      const body = JSON.parse(await readRequestBody(request) || '{}');
      const graph = createSampleGraph();
      graph.manifest.id = body.id;
      graph.manifest.title = body.title || body.id;
      graph.manifest.description = body.description || '';
      graph.nodes = [];
      graph.edges = [];
      storage.saveGraph(body.id, graph);
      sendJson(response, 201, { ok: true, graph: storage.loadGraph(body.id) });
      return;
    }

    if (parts.length === 3 && parts[0] === 'api' && parts[1] === 'graphs') {
      const graphId = parts[2];
      if (request.method === 'GET') {
        sendJson(response, 200, storage.loadGraph(graphId));
        return;
      }
      if (request.method === 'POST') {
        const body = JSON.parse(await readRequestBody(request) || '{}');
        const graph = storage.saveGraph(graphId, body);
        sendJson(response, 200, { ok: true, graph });
        return;
      }
    }

    sendJson(response, 404, { error: 'API route not found' });
  }

  function serveStatic(request, response, url) {
    const relativePath = url.pathname === '/' ? 'index.html' : decodeURIComponent(url.pathname.slice(1));
    const filePath = path.resolve(publicDir, relativePath);
    if (!filePath.startsWith(publicDir)) {
      sendText(response, 403, 'Forbidden');
      return;
    }
    if (!fs.existsSync(filePath) || !fs.statSync(filePath).isFile()) {
      sendText(response, 404, 'Not found');
      return;
    }
    const ext = path.extname(filePath).toLowerCase();
    response.writeHead(200, {
      'content-type': MIME_TYPES[ext] || 'application/octet-stream',
      'cache-control': 'no-store'
    });
    response.end(fs.readFileSync(filePath));
  }

  async function handleRequest(request, response) {
    const url = new URL(request.url, 'http://127.0.0.1');
    try {
      if (url.pathname.startsWith('/api/')) {
        await handleApi(request, response, url);
        return;
      }
      serveStatic(request, response, url);
    } catch (error) {
      sendJson(response, 500, { error: error.message });
    }
  }

  function start() {
    return new Promise((resolve) => {
      server = http.createServer(handleRequest);
      server.listen(port, '127.0.0.1', () => resolve(server));
    });
  }

  function stop() {
    return new Promise((resolve, reject) => {
      if (!server) {
        resolve();
        return;
      }
      server.close((error) => {
        if (error) {
          reject(error);
        } else {
          resolve();
        }
      });
    });
  }

  return { start, stop, storage };
}

if (require.main === module) {
  const args = parseArgs(process.argv.slice(2));
  const app = createServer(args);
  app.start().then((server) => {
    const address = server.address();
    console.log(`Production Graph Tool running at http://localhost:${address.port}`);
    console.log(`Project root: ${path.resolve(args.projectRoot)}`);
  });
}

module.exports = {
  createServer,
  parseArgs
};
