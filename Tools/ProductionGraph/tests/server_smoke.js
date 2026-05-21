const assert = require('assert');
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');

assert.ok(fs.existsSync(path.join(root, 'server.js')), 'server.js should exist');
assert.ok(fs.existsSync(path.join(root, 'public', 'index.html')), 'index.html should exist');
assert.ok(fs.existsSync(path.join(root, 'public', 'app.js')), 'app.js should exist');
assert.ok(fs.existsSync(path.join(root, 'public', 'styles.css')), 'styles.css should exist');

console.log('ProductionGraph server smoke passed');
