const assert = require('assert');
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'public', 'index.html'), 'utf8');
const css = fs.readFileSync(path.join(root, 'public', 'styles.css'), 'utf8');
const js = fs.readFileSync(path.join(root, 'public', 'app.js'), 'utf8');

assert.ok(html.includes('id="contextMenu"'), 'page should include a custom right-click node menu');
assert.ok(js.includes('showContextMenu'), 'app should open the node menu from right click');
assert.ok(js.includes('startCanvasPan'), 'app should support middle/right mouse canvas panning');
assert.ok(js.includes('centerCanvasView'), 'app should center the canvas view so panning is not clamped at origin');
assert.ok(js.includes('applyCanvasPan'), 'app should use a dedicated canvas pan helper');
assert.ok(js.includes("event.key === 'Backspace'") && js.includes("event.key === 'Delete'"), 'app should delete selected items with Backspace/Delete');
assert.ok(js.includes('undoStack') && js.includes('redoStack'), 'app should maintain undo and redo stacks');
assert.ok(js.includes('undoGraphChange') && js.includes('redoGraphChange'), 'app should expose undo and redo actions');
assert.ok(js.includes("event.key.toLowerCase() === 'z'") && js.includes("event.key.toLowerCase() === 'y'"), 'app should support keyboard undo and redo');
assert.ok(html.includes('id="undoButton"') && html.includes('id="redoButton"'), 'page should include undo and redo buttons');
assert.ok(!html.includes('id="edgeModeButton"'), 'left sidebar should not require a start-connection mode button');
assert.ok(js.includes('startPinConnection') && js.includes('finishPinConnection'), 'nodes should connect by dragging pins');
assert.ok(js.includes('showConnectionMenu'), 'dropping a pin connection on empty canvas should open the node menu');
assert.ok(js.includes('connectionPreview'), 'pin dragging should render a preview connection');
assert.ok(js.includes('focusSelectedOrAllNodes') && js.includes('frameNodes'), 'F should focus selected nodes and all nodes');
assert.ok(js.includes("event.key.toLowerCase() === 'f'"), 'app should handle F as a focus shortcut');
assert.ok(!js.includes('Math.max(0, state.drag.originalX'), 'node dragging should allow negative coordinates on a free canvas');
assert.ok(js.includes('node-delete-button'), 'nodes should include an inline red delete button');
assert.ok(js.includes('node-pin'), 'nodes should include connection pins');
assert.ok(css.includes('.context-menu'), 'context menu should have styles');
assert.ok(css.includes('.node-delete-button'), 'node delete button should have styles');
assert.ok(css.includes('.node-pin'), 'node pins should have styles');
assert.ok(css.includes('.connection-preview'), 'connection preview should have styles');
assert.ok(css.includes('.canvas-wrap.is-panning'), 'canvas panning should have visual cursor feedback');
assert.ok(css.includes('.canvas-wrap.is-connecting'), 'pin connection should have visual cursor feedback');
assert.ok(css.includes('.history-row'), 'undo and redo controls should have styles');

console.log('ProductionGraph UI contract smoke passed');
