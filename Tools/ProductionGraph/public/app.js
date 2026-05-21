const NODE_TYPES = {
  need: { label: '需求', defaultTitle: '新的需求' },
  solution: { label: '解决方案', defaultTitle: '新的解决方案' },
  task: { label: '任务', defaultTitle: '新的制作任务' },
  blocker: { label: '阻塞 / 发现', defaultTitle: '新的阻塞' }
};

const EDGE_TYPES = {
  has_solution: '产生方案',
  creates_task: '拆出任务',
  depends_on: '依赖',
  blocked_by: '阻塞',
  becomes_need: '转为新需求',
  changes_scope: '改变范围'
};

const STATUS_OPTIONS = ['open', 'draft', 'todo', 'doing', 'blocked', 'review', 'done'];
const NODE_WIDTH = 230;
const NODE_HEIGHT = 118;
const DRAG_THRESHOLD = 2;

const state = {
  graphs: [],
  graph: null,
  selectedNodeId: null,
  selectedNodeIds: [],
  selectedEdgeId: null,
  dirty: false,
  filter: '',
  hideDone: false,
  drag: null,
  pan: null,
  connection: null,
  selectionBox: null,
  pendingConnection: null,
  viewport: { x: 0, y: 0 },
  nodeClipboard: null,
  undoStack: [],
  redoStack: [],
  historyLimit: 80,
  isRestoringHistory: false,
  focusMode: 'all',
  suppressNextContextMenu: false,
  suppressNextDocumentClick: false,
  suppressNextCanvasClick: false,
  contextMenuPosition: null
};

const el = {
  graphSelect: document.getElementById('graphSelect'),
  newGraphButton: document.getElementById('newGraphButton'),
  hideDoneToggle: document.getElementById('hideDoneToggle'),
  filterInput: document.getElementById('filterInput'),
  undoButton: document.getElementById('undoButton'),
  redoButton: document.getElementById('redoButton'),
  saveButton: document.getElementById('saveButton'),
  deleteButton: document.getElementById('deleteButton'),
  graphTitle: document.getElementById('graphTitle'),
  graphSubtitle: document.getElementById('graphSubtitle'),
  saveState: document.getElementById('saveState'),
  canvasWrap: document.getElementById('canvasWrap'),
  canvas: document.getElementById('canvas'),
  edgeLayer: document.getElementById('edgeLayer'),
  nodeLayer: document.getElementById('nodeLayer'),
  validationBar: document.getElementById('validationBar'),
  selectionType: document.getElementById('selectionType'),
  inspectorBody: document.getElementById('inspectorBody'),
  contextMenu: document.getElementById('contextMenu')
};

function now() {
  return new Date().toISOString();
}

function uniqueId(prefix) {
  return `${prefix}-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 7)}`;
}

function slugify(text) {
  const ascii = String(text || '')
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '');
  return ascii || `graph-${Date.now().toString(36)}`;
}

async function api(path, options) {
  const response = await fetch(path, options);
  const text = await response.text();
  const body = text ? JSON.parse(text) : null;
  if (!response.ok) {
    throw new Error(body?.error || `Request failed: ${response.status}`);
  }
  return body;
}

function graphHasNode(nodeId) {
  return Boolean(nodeId && state.graph?.nodes.some((node) => node.id === nodeId));
}

function graphHasEdge(edgeId) {
  return Boolean(edgeId && state.graph?.edges.some((edge) => edge.id === edgeId));
}

function selectedNode() {
  return state.graph?.nodes.find((node) => node.id === state.selectedNodeId) || null;
}

function selectedEdge() {
  return state.graph?.edges.find((edge) => edge.id === state.selectedEdgeId) || null;
}

function selectedNodeIds() {
  const ids = new Set(state.selectedNodeIds);
  if (state.selectedNodeId) {
    ids.add(state.selectedNodeId);
  }
  return [...ids].filter((nodeId) => graphHasNode(nodeId));
}

function clearSelection() {
  state.selectedNodeId = null;
  state.selectedNodeIds = [];
  state.selectedEdgeId = null;
}

function selectSingleNode(nodeId) {
  state.selectedNodeId = nodeId;
  state.selectedNodeIds = nodeId ? [nodeId] : [];
  state.selectedEdgeId = null;
}

function selectMultipleNodes(nodeIds) {
  const uniqueIds = [...new Set(nodeIds)].filter((nodeId) => graphHasNode(nodeId));
  state.selectedNodeIds = uniqueIds;
  state.selectedNodeId = uniqueIds.length === 1 ? uniqueIds[0] : null;
  state.selectedEdgeId = null;
}

function cloneGraph(graph) {
  return graph ? JSON.parse(JSON.stringify(graph)) : null;
}

function cloneValue(value) {
  return value ? JSON.parse(JSON.stringify(value)) : value;
}

function syncSelectionToGraph() {
  if (!graphHasNode(state.selectedNodeId)) {
    state.selectedNodeId = null;
  }
  state.selectedNodeIds = state.selectedNodeIds.filter((nodeId) => graphHasNode(nodeId));
  if (!graphHasEdge(state.selectedEdgeId)) {
    state.selectedEdgeId = null;
  }
}

function renderHistoryButtons() {
  el.undoButton.disabled = state.undoStack.length === 0;
  el.redoButton.disabled = state.redoStack.length === 0;
}

function resetHistory() {
  state.undoStack = [];
  state.redoStack = [];
  renderHistoryButtons();
}

function pushUndoSnapshot() {
  if (!state.graph || state.isRestoringHistory) {
    return;
  }
  state.undoStack.push(cloneGraph(state.graph));
  if (state.undoStack.length > state.historyLimit) {
    state.undoStack.shift();
  }
  state.redoStack = [];
  renderHistoryButtons();
}

function restoreGraphSnapshot(snapshot) {
  if (!snapshot) {
    return;
  }
  state.isRestoringHistory = true;
  state.graph = cloneGraph(snapshot);
  syncSelectionToGraph();
  state.connection = null;
  state.selectionBox = null;
  state.pendingConnection = null;
  state.dirty = true;
  state.isRestoringHistory = false;
  render();
}

function undoGraphChange() {
  const previous = state.undoStack.pop();
  if (!previous || !state.graph) {
    renderHistoryButtons();
    return;
  }
  state.redoStack.push(cloneGraph(state.graph));
  restoreGraphSnapshot(previous);
}

function redoGraphChange() {
  const next = state.redoStack.pop();
  if (!next || !state.graph) {
    renderHistoryButtons();
    return;
  }
  state.undoStack.push(cloneGraph(state.graph));
  restoreGraphSnapshot(next);
}

function captureFieldHistory(input) {
  if (input.dataset.historyCaptured === 'true') {
    return;
  }
  pushUndoSnapshot();
  input.dataset.historyCaptured = 'true';
}

function markDirty() {
  state.dirty = true;
  renderSaveState();
}

function renderSaveState() {
  el.saveState.textContent = state.dirty ? '有未保存修改' : '已保存';
  el.saveState.classList.toggle('is-dirty', state.dirty);
}

function graphMatchesFilter(node) {
  const text = [node.title, node.description, node.status, node.owner, ...(node.tags || [])]
    .join(' ')
    .toLowerCase();
  return text.includes(state.filter.toLowerCase());
}

function visibleNodes() {
  if (!state.graph) {
    return [];
  }
  return state.graph.nodes.filter((node) => {
    if (state.hideDone && node.status === 'done') {
      return false;
    }
    return !state.filter || graphMatchesFilter(node);
  });
}

function visibleNodeIds() {
  return new Set(visibleNodes().map((node) => node.id));
}

function clientToGraphPoint(clientX, clientY) {
  const rect = el.canvas.getBoundingClientRect();
  return {
    x: clientX - rect.left,
    y: clientY - rect.top
  };
}

function pinPoint(node, role) {
  const x = Number(node.position?.x || 0);
  const y = Number(node.position?.y || 0);
  return {
    x: role === 'input' ? x : x + NODE_WIDTH,
    y: y + NODE_HEIGHT / 2
  };
}

function inferredPinRoles(source, target) {
  return Number(source.position?.x || 0) <= Number(target.position?.x || 0)
    ? { sourcePin: 'output', targetPin: 'input' }
    : { sourcePin: 'input', targetPin: 'output' };
}

function edgeEndpointPoints(source, target, edge) {
  const fallback = inferredPinRoles(source, target);
  return {
    start: pinPoint(source, edge?.sourcePin || fallback.sourcePin),
    end: pinPoint(target, edge?.targetPin || fallback.targetPin)
  };
}

function makePathBetweenPoints(start, end) {
  const dx = Math.max(120, Math.abs(end.x - start.x) * 0.45);
  return `M ${start.x} ${start.y} C ${start.x + dx} ${start.y}, ${end.x - dx} ${end.y}, ${end.x} ${end.y}`;
}

function makeEdgePath(source, target, edge) {
  const points = edgeEndpointPoints(source, target, edge);
  return makePathBetweenPoints(points.start, points.end);
}

function normalizedSelectionBox(box) {
  if (!box) {
    return null;
  }
  return {
    x: Math.min(box.start.x, box.current.x),
    y: Math.min(box.start.y, box.current.y),
    width: Math.abs(box.current.x - box.start.x),
    height: Math.abs(box.current.y - box.start.y)
  };
}

function nodeIntersectsBox(node, box) {
  const x = Number(node.position?.x || 0);
  const y = Number(node.position?.y || 0);
  return x < box.x + box.width &&
    x + NODE_WIDTH > box.x &&
    y < box.y + box.height &&
    y + NODE_HEIGHT > box.y;
}

function nodesInsideSelectionBox(box = normalizedSelectionBox(state.selectionBox)) {
  if (!box || !state.graph) {
    return [];
  }
  return state.graph.nodes
    .filter((node) => nodeIntersectsBox(node, box))
    .map((node) => node.id);
}

function frameNodes(nodes) {
  if (!nodes || nodes.length === 0) {
    return;
  }
  const bounds = nodes.reduce((result, node) => {
    const x = Number(node.position?.x || 0);
    const y = Number(node.position?.y || 0);
    return {
      minX: Math.min(result.minX, x),
      minY: Math.min(result.minY, y),
      maxX: Math.max(result.maxX, x + NODE_WIDTH),
      maxY: Math.max(result.maxY, y + NODE_HEIGHT)
    };
  }, { minX: Infinity, minY: Infinity, maxX: -Infinity, maxY: -Infinity });
  state.viewport = {
    x: Math.round(el.canvasWrap.clientWidth / 2 - (bounds.minX + bounds.maxX) / 2),
    y: Math.round(el.canvasWrap.clientHeight / 2 - (bounds.minY + bounds.maxY) / 2)
  };
  applyCanvasTransform();
}

function focusSelectedOrAllNodes() {
  const multiSelection = selectedNodeIds();
  if (multiSelection.length > 1 && state.focusMode !== 'selected') {
    frameNodes(state.graph.nodes.filter((node) => multiSelection.includes(node.id)));
    state.focusMode = 'selected';
    return;
  }
  const node = selectedNode();
  if (node && state.focusMode !== 'selected') {
    frameNodes([node]);
    state.focusMode = 'selected';
    return;
  }
  frameNodes(visibleNodes());
  state.focusMode = 'all';
}

function applyCanvasTransform() {
  el.canvas.style.transform = `translate(${state.viewport.x}px, ${state.viewport.y}px)`;
}

function applyCanvasPan(dx, dy) {
  if (!state.pan) {
    return;
  }
  state.viewport = {
    x: state.pan.startOffsetX + dx,
    y: state.pan.startOffsetY + dy
  };
  applyCanvasTransform();
}

function centerCanvasView() {
  requestAnimationFrame(() => frameNodes(state.graph?.nodes || []));
}

function renderGraphSelect() {
  el.graphSelect.innerHTML = '';
  for (const graph of state.graphs) {
    const option = document.createElement('option');
    option.value = graph.id;
    option.textContent = `${graph.title} (${graph.nodeCount})`;
    el.graphSelect.appendChild(option);
  }
  if (state.graph) {
    el.graphSelect.value = state.graph.manifest.id;
  }
}

function renderHeader() {
  if (!state.graph) {
    el.graphTitle.textContent = '未加载图谱';
    el.graphSubtitle.textContent = '';
    return;
  }
  el.graphTitle.textContent = state.graph.manifest.title;
  el.graphSubtitle.textContent = `${state.graph.nodes.length} 个节点 / ${state.graph.edges.length} 条连线`;
}

function renderEdges() {
  if (!state.graph) {
    el.edgeLayer.innerHTML = '';
    return;
  }
  const nodeById = new Map(state.graph.nodes.map((node) => [node.id, node]));
  const visible = visibleNodeIds();
  el.edgeLayer.innerHTML = `
    <defs>
      <marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="3" orient="auto">
        <path d="M0,0 L0,6 L7,3 z" fill="#64748b"></path>
      </marker>
      <marker id="arrow-warn" markerWidth="8" markerHeight="8" refX="7" refY="3" orient="auto">
        <path d="M0,0 L0,6 L7,3 z" fill="#b45309"></path>
      </marker>
    </defs>`;

  for (const edge of state.graph.edges) {
    const source = nodeById.get(edge.source);
    const target = nodeById.get(edge.target);
    if (!source || !target || !visible.has(source.id) || !visible.has(target.id)) {
      continue;
    }
    const pathData = makeEdgePath(source, target, edge);
    const group = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    const line = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    line.setAttribute('d', pathData);
    line.setAttribute('class', `edge-line ${edge.type}`);
    line.setAttribute('marker-end', edge.type === 'blocked_by' || edge.type === 'becomes_need' ? 'url(#arrow-warn)' : 'url(#arrow)');

    const hit = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    hit.setAttribute('d', pathData);
    hit.setAttribute('class', 'edge-hit');
    hit.addEventListener('click', (event) => {
      event.stopPropagation();
      state.selectedEdgeId = edge.id;
      state.selectedNodeId = null;
      state.selectedNodeIds = [];
      render();
    });

    const points = edgeEndpointPoints(source, target, edge);
    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('x', (points.start.x + points.end.x) / 2);
    label.setAttribute('y', (points.start.y + points.end.y) / 2 - 8);
    label.setAttribute('class', 'edge-label');
    label.textContent = edge.label || EDGE_TYPES[edge.type] || edge.type;

    group.appendChild(line);
    group.appendChild(hit);
    group.appendChild(label);
    el.edgeLayer.appendChild(group);
  }
  renderConnectionPreview();
}

function renderConnectionPreview() {
  if (!state.connection) {
    return;
  }
  const preview = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  preview.setAttribute('d', makePathBetweenPoints(state.connection.start, state.connection.current));
  preview.setAttribute('class', 'connection-preview');
  el.edgeLayer.appendChild(preview);
}

function renderSelectionBox() {
  const box = normalizedSelectionBox(state.selectionBox);
  if (!box || !state.selectionBox?.moved) {
    return;
  }
  const item = document.createElement('div');
  item.className = 'selection-box';
  item.style.left = `${box.x}px`;
  item.style.top = `${box.y}px`;
  item.style.width = `${box.width}px`;
  item.style.height = `${box.height}px`;
  el.nodeLayer.appendChild(item);
}

function renderNodes() {
  if (!state.graph) {
    el.nodeLayer.innerHTML = '';
    return;
  }
  el.nodeLayer.innerHTML = '';
  const selection = new Set(selectedNodeIds());
  for (const node of visibleNodes()) {
    const item = document.createElement('div');
    item.className = `node ${node.type}`;
    item.classList.toggle('is-selected', selection.has(node.id));
    item.style.left = `${Number(node.position?.x || 0)}px`;
    item.style.top = `${Number(node.position?.y || 0)}px`;
    item.dataset.nodeId = node.id;
    const tags = (node.tags || []).slice(0, 3).map((tag) => `<span class="tag">${escapeHtml(tag)}</span>`).join('');
    item.innerHTML = `
      <button class="node-delete-button" title="删除节点" aria-label="删除节点"></button>
      <button class="node-pin input" data-pin-role="input" title="拖动连线输入引脚" aria-label="输入引脚"></button>
      <button class="node-pin output" data-pin-role="output" title="拖动连线输出引脚" aria-label="输出引脚"></button>
      <div class="node-inner">
        <div class="node-type">${NODE_TYPES[node.type]?.label || node.type}</div>
        <div class="node-title">${escapeHtml(node.title)}</div>
        <div class="node-desc">${escapeHtml(node.description).slice(0, 120)}</div>
        <div class="node-meta">
          <span class="tag">${escapeHtml(node.status || 'open')}</span>
          ${node.owner ? `<span class="tag">${escapeHtml(node.owner)}</span>` : ''}
          ${tags}
        </div>
      </div>`;
    item.querySelector('.node-delete-button').addEventListener('click', (event) => {
      event.stopPropagation();
      deleteNode(node.id);
    });
    item.querySelectorAll('.node-pin').forEach((pin) => {
      pin.addEventListener('pointerdown', (event) => startPinConnection(event, node, pin.dataset.pinRole));
      pin.addEventListener('click', (event) => event.stopPropagation());
    });
    item.addEventListener('pointerdown', (event) => startDrag(event, node));
    item.addEventListener('click', (event) => {
      event.stopPropagation();
      selectSingleNode(node.id);
      state.focusMode = 'all';
      render();
    });
    el.nodeLayer.appendChild(item);
  }
  renderSelectionBox();
}

function renderInspector() {
  const multiSelection = selectedNodeIds();
  const node = selectedNode();
  const edge = selectedEdge();
  if (multiSelection.length > 1) {
    el.selectionType.textContent = '多选';
    el.inspectorBody.classList.remove('empty');
    el.inspectorBody.innerHTML = `
      <div class="form-stack">
        <div class="field-label">已选中 ${multiSelection.length} 个节点</div>
        <button id="deleteMultiSelectionButton" class="danger-button" type="button">删除选中节点</button>
      </div>`;
    document.getElementById('deleteMultiSelectionButton')?.addEventListener('click', deleteSelected);
    return;
  }
  if (node) {
    el.selectionType.textContent = NODE_TYPES[node.type]?.label || node.type;
    el.inspectorBody.classList.remove('empty');
    el.inspectorBody.innerHTML = nodeInspectorHtml(node);
    bindNodeInspector(node);
    return;
  }
  if (edge) {
    el.selectionType.textContent = EDGE_TYPES[edge.type] || edge.type;
    el.inspectorBody.classList.remove('empty');
    el.inspectorBody.innerHTML = edgeInspectorHtml(edge);
    bindEdgeInspector(edge);
    return;
  }
  el.selectionType.textContent = '未选择';
  el.inspectorBody.classList.add('empty');
  el.inspectorBody.textContent = '选择一个节点或连线开始编辑。';
}

function nodeInspectorHtml(node) {
  return `
    <div class="form-stack">
      <label><span class="field-label">标题</span><input class="input" data-node-field="title" value="${escapeAttr(node.title)}"></label>
      <label><span class="field-label">描述</span><textarea data-node-field="description">${escapeHtml(node.description)}</textarea></label>
      <div class="mini-row">
        <label><span class="field-label">类型</span>${selectHtml('type', NODE_TYPES, node.type, 'data-node-field')}</label>
        <label><span class="field-label">状态</span>${statusSelectHtml(node.status)}</label>
      </div>
      <div class="mini-row">
        <label><span class="field-label">负责人</span><input class="input" data-node-field="owner" value="${escapeAttr(node.owner)}"></label>
        <label><span class="field-label">标签</span><input class="input" data-node-field="tags" value="${escapeAttr((node.tags || []).join(', '))}"></label>
      </div>
      <div class="divider"></div>
      <label><span class="field-label">工作分类</span><input class="input" data-node-field-extra="workType" value="${escapeAttr(node.fields?.workType || '')}"></label>
      <label><span class="field-label">StoryEvent</span><input class="input" data-node-field-extra="storyEvent" value="${escapeAttr(node.fields?.storyEvent || '')}"></label>
      <label><span class="field-label">LevelFlow / 资产</span><input class="input" data-node-field-extra="levelFlow" value="${escapeAttr(node.fields?.levelFlow || '')}"></label>
      <label><span class="field-label">验证方式</span><textarea data-node-field-extra="verification">${escapeHtml(node.fields?.verification || '')}</textarea></label>
    </div>`;
}

function edgeInspectorHtml(edge) {
  return `
    <div class="form-stack">
      <label><span class="field-label">关系类型</span>${selectHtml('type', EDGE_TYPES, edge.type, 'data-edge-field')}</label>
      <label><span class="field-label">标签</span><input class="input" data-edge-field="label" value="${escapeAttr(edge.label || '')}"></label>
    </div>`;
}

function selectHtml(name, options, selected, attrName) {
  return `<select class="input" ${attrName}="${name}">` + Object.entries(options)
    .map(([value, config]) => {
      const label = typeof config === 'string' ? config : config.label;
      return `<option value="${value}" ${value === selected ? 'selected' : ''}>${escapeHtml(label)}</option>`;
    })
    .join('') + '</select>';
}

function statusSelectHtml(selected) {
  return `<select class="input" data-node-field="status">` + STATUS_OPTIONS
    .map((value) => `<option value="${value}" ${value === selected ? 'selected' : ''}>${value}</option>`)
    .join('') + '</select>';
}

function bindNodeInspector(node) {
  el.inspectorBody.querySelectorAll('[data-node-field]').forEach((input) => {
    input.addEventListener('input', () => {
      captureFieldHistory(input);
      const field = input.dataset.nodeField;
      node[field] = field === 'tags'
        ? input.value.split(',').map((item) => item.trim()).filter(Boolean)
        : input.value;
      node.updatedAt = now();
      markDirty();
      renderHeader();
      renderNodes();
      renderValidation();
    });
  });
  el.inspectorBody.querySelectorAll('[data-node-field-extra]').forEach((input) => {
    input.addEventListener('input', () => {
      captureFieldHistory(input);
      node.fields = node.fields || {};
      node.fields[input.dataset.nodeFieldExtra] = input.value;
      node.updatedAt = now();
      markDirty();
      renderNodes();
    });
  });
}

function bindEdgeInspector(edge) {
  el.inspectorBody.querySelectorAll('[data-edge-field]').forEach((input) => {
    input.addEventListener('input', () => {
      captureFieldHistory(input);
      edge[input.dataset.edgeField] = input.value;
      edge.updatedAt = now();
      markDirty();
      renderEdges();
      renderValidation();
    });
  });
}

function renderValidation() {
  const messages = validateGraph();
  el.validationBar.classList.toggle('has-errors', messages.some((item) => item.kind === 'error'));
  el.validationBar.textContent = messages.length === 0
    ? '校验通过：没有断线、重复 ID 或孤立节点。'
    : messages.map((item) => item.text).join(' | ');
}

function validateGraph() {
  if (!state.graph) {
    return [];
  }
  const messages = [];
  const ids = new Set(state.graph.nodes.map((node) => node.id));
  for (const edge of state.graph.edges) {
    if (!ids.has(edge.source) || !ids.has(edge.target)) {
      messages.push({ kind: 'error', text: `连线 ${edge.id} 缺少节点` });
    }
  }
  if (state.dirty) {
    messages.push({ kind: 'info', text: '存在未保存修改' });
  }
  return messages;
}

function render() {
  renderGraphSelect();
  renderHeader();
  renderSaveState();
  renderHistoryButtons();
  renderEdges();
  renderNodes();
  renderInspector();
  renderValidation();
}

function startPinConnection(event, node, role) {
  if (event.button !== 0) {
    return;
  }
  event.preventDefault();
  event.stopPropagation();
  hideContextMenu();
  const start = pinPoint(node, role);
  state.connection = {
    sourceNodeId: node.id,
    sourceRole: role,
    start,
    current: start,
    moved: false
  };
  event.currentTarget.setPointerCapture(event.pointerId);
  el.canvasWrap.classList.add('is-connecting');
  renderEdges();
}

function updateConnectionPreview(event) {
  const point = clientToGraphPoint(event.clientX, event.clientY);
  state.connection.current = point;
  state.connection.moved = Math.abs(point.x - state.connection.start.x) > DRAG_THRESHOLD ||
    Math.abs(point.y - state.connection.start.y) > DRAG_THRESHOLD;
  renderEdges();
}

function pinFromPoint(clientX, clientY) {
  return document.elementFromPoint(clientX, clientY)?.closest('.node-pin') || null;
}

function resolveConnectionTarget(start, targetPin) {
  const targetNodeId = targetPin.closest('.node')?.dataset.nodeId;
  if (!targetNodeId || targetNodeId === start.sourceNodeId) {
    return null;
  }
  const targetRole = targetPin.dataset.pinRole;
  if (start.sourceRole === 'input' && targetRole === 'output') {
    return { source: targetNodeId, target: start.sourceNodeId, sourcePin: 'output', targetPin: 'input' };
  }
  return { source: start.sourceNodeId, target: targetNodeId, sourcePin: start.sourceRole, targetPin: targetRole };
}

function finishPinConnection(event) {
  if (!state.connection) {
    return;
  }
  const connection = state.connection;
  const targetPin = pinFromPoint(event.clientX, event.clientY);
  state.connection = null;
  el.canvasWrap.classList.remove('is-connecting');
  const target = targetPin ? resolveConnectionTarget(connection, targetPin) : null;
  if (target) {
    createEdge(target.source, target.target, null, {
      sourcePin: target.sourcePin,
      targetPin: target.targetPin
    });
    render();
    return;
  }
  if (connection.moved) {
    showConnectionMenu(event, connection);
    state.suppressNextDocumentClick = true;
    setTimeout(() => {
      state.suppressNextDocumentClick = false;
    }, 250);
  }
  renderEdges();
}

function shouldStartBoxSelection(event) {
  if (event.button !== 0) {
    return false;
  }
  return !event.target.closest('.node') &&
    !event.target.closest('.node-pin') &&
    !event.target.closest('.node-delete-button') &&
    !event.target.closest('.context-menu') &&
    !event.target.closest('.edge-hit');
}

function startBoxSelection(event) {
  if (!shouldStartBoxSelection(event)) {
    return;
  }
  hideContextMenu();
  const start = clientToGraphPoint(event.clientX, event.clientY);
  state.selectionBox = { start, current: start, moved: false };
  el.canvasWrap.setPointerCapture(event.pointerId);
}

function updateBoxSelection(event) {
  const current = clientToGraphPoint(event.clientX, event.clientY);
  state.selectionBox.current = current;
  state.selectionBox.moved = Math.abs(current.x - state.selectionBox.start.x) > DRAG_THRESHOLD ||
    Math.abs(current.y - state.selectionBox.start.y) > DRAG_THRESHOLD;
  if (state.selectionBox.moved) {
    el.canvasWrap.classList.add('is-selecting');
    renderNodes();
  }
}

function finishBoxSelection(event) {
  if (!state.selectionBox) {
    return;
  }
  updateBoxSelection(event);
  const wasSelecting = state.selectionBox.moved;
  const selectedIds = wasSelecting ? nodesInsideSelectionBox() : [];
  state.selectionBox = null;
  el.canvasWrap.classList.remove('is-selecting');
  if (wasSelecting) {
    selectMultipleNodes(selectedIds);
    state.suppressNextCanvasClick = true;
    setTimeout(() => {
      state.suppressNextCanvasClick = false;
    }, 250);
    render();
  } else {
    renderNodes();
  }
}

function startDrag(event, node) {
  if (event.button !== 0 || event.target.closest('.node-delete-button') || event.target.closest('.node-pin')) {
    return;
  }
  event.currentTarget.setPointerCapture(event.pointerId);
  state.drag = {
    node,
    startX: event.clientX,
    startY: event.clientY,
    originalX: Number(node.position?.x || 0),
    originalY: Number(node.position?.y || 0),
    historyCaptured: false
  };
}

function startCanvasPan(event) {
  if (event.button !== 1 && event.button !== 2) {
    return;
  }
  hideContextMenu();
  event.preventDefault();
  el.canvasWrap.setPointerCapture(event.pointerId);
  state.pan = {
    button: event.button,
    startX: event.clientX,
    startY: event.clientY,
    startOffsetX: state.viewport.x,
    startOffsetY: state.viewport.y,
    moved: false
  };
}

function onPointerMove(event) {
  if (state.connection) {
    updateConnectionPreview(event);
    return;
  }
  if (state.selectionBox) {
    updateBoxSelection(event);
    return;
  }
  if (state.pan) {
    const dx = event.clientX - state.pan.startX;
    const dy = event.clientY - state.pan.startY;
    if (Math.abs(dx) > DRAG_THRESHOLD || Math.abs(dy) > DRAG_THRESHOLD) {
      state.pan.moved = true;
      el.canvasWrap.classList.add('is-panning');
      applyCanvasPan(dx, dy);
    }
    return;
  }
  if (!state.drag) {
    return;
  }
  const dx = event.clientX - state.drag.startX;
  const dy = event.clientY - state.drag.startY;
  if (!state.drag.historyCaptured && (dx !== 0 || dy !== 0)) {
    pushUndoSnapshot();
    state.drag.historyCaptured = true;
  }
  state.drag.node.position = {
    x: state.drag.originalX + dx,
    y: state.drag.originalY + dy
  };
  state.drag.node.updatedAt = now();
  markDirty();
  renderEdges();
  renderNodes();
}

function onPointerUp(event) {
  if (state.connection) {
    finishPinConnection(event);
    return;
  }
  if (state.selectionBox) {
    finishBoxSelection(event);
    return;
  }
  if (state.pan) {
    state.suppressNextContextMenu = state.pan.button === 2 && state.pan.moved;
    state.pan = null;
    el.canvasWrap.classList.remove('is-panning');
    return;
  }
  state.drag = null;
}

function createNode(type, position, options = {}) {
  if (!state.graph) {
    return null;
  }
  if (options.captureHistory !== false) {
    pushUndoSnapshot();
  }
  const index = state.graph.nodes.length;
  const timestamp = now();
  const node = {
    id: uniqueId(type),
    type,
    title: NODE_TYPES[type].defaultTitle,
    description: '',
    status: type === 'task' ? 'todo' : 'open',
    owner: '',
    tags: [],
    position: position || { x: 120 + (index % 4) * 260, y: 120 + Math.floor(index / 4) * 180 },
    fields: {},
    createdAt: timestamp,
    updatedAt: timestamp
  };
  state.graph.nodes.push(node);
  selectSingleNode(node.id);
  markDirty();
  render();
  return node;
}

function inferEdgeType(source, target) {
  const targetNode = state.graph?.nodes.find((node) => node.id === target);
  if (targetNode?.type === 'solution') return 'has_solution';
  if (targetNode?.type === 'task') return 'creates_task';
  if (targetNode?.type === 'blocker') return 'blocked_by';
  if (targetNode?.type === 'need') return 'changes_scope';
  return 'depends_on';
}

function createEdge(source, target, type, options = {}) {
  if (!state.graph) {
    return null;
  }
  if (options.captureHistory !== false) {
    pushUndoSnapshot();
  }
  const edgeType = type || inferEdgeType(source, target);
  const edge = {
    id: uniqueId('edge'),
    type: edgeType,
    source,
    target,
    sourcePin: options.sourcePin,
    targetPin: options.targetPin,
    label: EDGE_TYPES[edgeType],
    createdAt: now(),
    updatedAt: now()
  };
  state.graph.edges.push(edge);
  state.selectedEdgeId = edge.id;
  state.selectedNodeId = null;
  state.selectedNodeIds = [];
  markDirty();
  return edge;
}

function createConnectedNode(type) {
  if (!state.graph || !state.pendingConnection || !state.contextMenuPosition) {
    return;
  }
  pushUndoSnapshot();
  const startNodeId = state.pendingConnection.sourceNodeId;
  const startRole = state.pendingConnection.sourceRole;
  const drop = state.contextMenuPosition;
  const position = {
    x: startRole === 'input' ? drop.x - NODE_WIDTH - 48 : drop.x + 48,
    y: drop.y - NODE_HEIGHT / 2
  };
  const node = createNode(type, position, { captureHistory: false });
  if (!node) {
    return;
  }
  if (startRole === 'input') {
    createEdge(node.id, startNodeId, null, { captureHistory: false, sourcePin: 'output', targetPin: 'input' });
  } else {
    createEdge(startNodeId, node.id, null, { captureHistory: false, sourcePin: 'output', targetPin: 'input' });
  }
  selectSingleNode(node.id);
  markDirty();
  render();
}

function copyInternalEdges(nodeIdSet) {
  if (!state.graph) {
    return [];
  }
  return state.graph.edges
    .filter((edge) => nodeIdSet.has(edge.source) && nodeIdSet.has(edge.target))
    .map((edge) => cloneValue(edge));
}

function buildNodeClipboardFromSelection() {
  if (!state.graph) {
    return null;
  }
  const ids = selectedNodeIds();
  if (ids.length === 0) {
    return null;
  }
  const idSet = new Set(ids);
  const nodes = state.graph.nodes
    .filter((node) => idSet.has(node.id))
    .map((node) => cloneValue(node));
  return {
    nodes,
    edges: copyInternalEdges(idSet),
    pasteCount: 0
  };
}

function copySelectedNodes() {
  const clipboard = buildNodeClipboardFromSelection();
  if (!clipboard) {
    return false;
  }
  state.nodeClipboard = clipboard;
  return true;
}

function pasteNodeClipboard(clipboard) {
  if (!state.graph || !clipboard || clipboard.nodes.length === 0) {
    return false;
  }
  pushUndoSnapshot();
  const timestamp = now();
  const offset = 40 * ((clipboard.pasteCount || 0) + 1);
  const idMap = new Map();
  const newNodeIds = [];
  for (const sourceNode of clipboard.nodes) {
    const node = cloneValue(sourceNode);
    const oldId = node.id;
    node.id = uniqueId(node.type || 'node');
    node.position = {
      x: Number(node.position?.x || 0) + offset,
      y: Number(node.position?.y || 0) + offset
    };
    node.createdAt = timestamp;
    node.updatedAt = timestamp;
    idMap.set(oldId, node.id);
    newNodeIds.push(node.id);
    state.graph.nodes.push(node);
  }
  for (const sourceEdge of clipboard.edges || []) {
    const source = idMap.get(sourceEdge.source);
    const target = idMap.get(sourceEdge.target);
    if (!source || !target) {
      continue;
    }
    const edge = cloneValue(sourceEdge);
    edge.id = uniqueId('edge');
    edge.source = source;
    edge.target = target;
    edge.createdAt = timestamp;
    edge.updatedAt = timestamp;
    state.graph.edges.push(edge);
  }
  clipboard.pasteCount = (clipboard.pasteCount || 0) + 1;
  selectMultipleNodes(newNodeIds);
  markDirty();
  render();
  return true;
}

function pasteCopiedNodes() {
  return pasteNodeClipboard(state.nodeClipboard);
}

function duplicateSelectedNodes() {
  const clipboard = buildNodeClipboardFromSelection();
  return pasteNodeClipboard(clipboard);
}

function deleteSelected() {
  if (!state.graph) {
    return;
  }
  const nodeIds = selectedNodeIds();
  if (nodeIds.length > 1) {
    deleteNodes(nodeIds);
    return;
  }
  if (state.selectedNodeId) {
    deleteNode(state.selectedNodeId);
    return;
  }
  if (state.selectedEdgeId) {
    deleteEdge(state.selectedEdgeId);
  }
}

function deleteNodes(nodeIds) {
  const ids = new Set(nodeIds.filter((nodeId) => graphHasNode(nodeId)));
  if (ids.size === 0) {
    return;
  }
  pushUndoSnapshot();
  state.graph.nodes = state.graph.nodes.filter((node) => !ids.has(node.id));
  state.graph.edges = state.graph.edges.filter((edge) => !ids.has(edge.source) && !ids.has(edge.target));
  clearSelection();
  markDirty();
  render();
}

function deleteNode(nodeId) {
  if (!state.graph || !graphHasNode(nodeId)) {
    return;
  }
  pushUndoSnapshot();
  state.graph.nodes = state.graph.nodes.filter((node) => node.id !== nodeId);
  state.graph.edges = state.graph.edges.filter((edge) => edge.source !== nodeId && edge.target !== nodeId);
  if (state.selectedNodeId === nodeId) {
    state.selectedNodeId = null;
  }
  state.selectedNodeIds = state.selectedNodeIds.filter((id) => id !== nodeId);
  state.selectedEdgeId = null;
  markDirty();
  render();
}

function deleteEdge(edgeId) {
  if (!state.graph || !graphHasEdge(edgeId)) {
    return;
  }
  pushUndoSnapshot();
  state.graph.edges = state.graph.edges.filter((edge) => edge.id !== edgeId);
  state.selectedEdgeId = null;
  markDirty();
  render();
}

function setContextMenuTitle(text) {
  el.contextMenu.querySelector('.context-menu-title').textContent = text;
}

function placeContextMenu(clientX, clientY) {
  el.contextMenu.style.left = `${clientX}px`;
  el.contextMenu.style.top = `${clientY}px`;
  el.contextMenu.hidden = false;
}

function showContextMenu(event) {
  event.preventDefault();
  if (state.suppressNextContextMenu) {
    state.suppressNextContextMenu = false;
    hideContextMenu();
    return;
  }
  state.pendingConnection = null;
  state.contextMenuPosition = clientToGraphPoint(event.clientX, event.clientY);
  setContextMenuTitle('新增节点');
  placeContextMenu(event.clientX, event.clientY);
}

function showConnectionMenu(event, connection) {
  state.pendingConnection = {
    sourceNodeId: connection.sourceNodeId,
    sourceRole: connection.sourceRole
  };
  state.contextMenuPosition = clientToGraphPoint(event.clientX, event.clientY);
  setContextMenuTitle('创建并连接');
  placeContextMenu(event.clientX, event.clientY);
}

function hideContextMenu() {
  el.contextMenu.hidden = true;
  state.pendingConnection = null;
}

function isTypingTarget(target) {
  const tagName = target?.tagName;
  return target?.isContentEditable || tagName === 'INPUT' || tagName === 'TEXTAREA' || tagName === 'SELECT';
}

function handleKeyboardDelete(event) {
  if (isTypingTarget(event.target)) {
    return;
  }
  const usesShortcutModifier = event.ctrlKey || event.metaKey;
  if (usesShortcutModifier && event.key.toLowerCase() === 'z' && !event.shiftKey) {
    event.preventDefault();
    undoGraphChange();
    return;
  }
  if (usesShortcutModifier && (event.key.toLowerCase() === 'y' || (event.key.toLowerCase() === 'z' && event.shiftKey))) {
    event.preventDefault();
    redoGraphChange();
    return;
  }
  if (usesShortcutModifier && event.key.toLowerCase() === 'c') {
    event.preventDefault();
    copySelectedNodes();
    return;
  }
  if (usesShortcutModifier && event.key.toLowerCase() === 'v') {
    event.preventDefault();
    pasteCopiedNodes();
    return;
  }
  if (usesShortcutModifier && event.key.toLowerCase() === 'd') {
    event.preventDefault();
    duplicateSelectedNodes();
    return;
  }
  if (!usesShortcutModifier && event.key.toLowerCase() === 'f') {
    event.preventDefault();
    focusSelectedOrAllNodes();
    return;
  }
  if ((event.key === 'Backspace' || event.key === 'Delete') && (selectedNodeIds().length > 0 || state.selectedEdgeId)) {
    event.preventDefault();
    deleteSelected();
  }
}

async function loadGraphs() {
  const data = await api('/api/graphs');
  state.graphs = data.graphs;
  renderGraphSelect();
}

async function loadGraph(id) {
  state.graph = await api(`/api/graphs/${id}`);
  selectSingleNode(state.graph.nodes[0]?.id || null);
  state.dirty = false;
  state.focusMode = 'all';
  resetHistory();
  render();
  centerCanvasView();
}

async function saveGraph() {
  if (!state.graph) {
    return;
  }
  state.graph.manifest.updatedAt = now();
  const result = await api(`/api/graphs/${state.graph.manifest.id}`, {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(state.graph)
  });
  state.graph = result.graph;
  state.dirty = false;
  await loadGraphs();
  render();
}

async function createNewGraph() {
  const title = prompt('图谱名称，例如：黑夜少女首次主城对话');
  if (!title) {
    return;
  }
  const id = slugify(prompt('英文 ID，用于文件夹名，例如：night-girl-hub-intro') || title);
  await api('/api/graphs', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ id, title })
  });
  await loadGraphs();
  await loadGraph(id);
}

function escapeHtml(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

function escapeAttr(value) {
  return escapeHtml(value).replace(/\n/g, '&#10;');
}

function bindEvents() {
  document.querySelectorAll('[data-add-node]').forEach((button) => {
    button.addEventListener('click', () => createNode(button.dataset.addNode));
  });
  document.querySelectorAll('[data-context-node]').forEach((button) => {
    button.addEventListener('click', () => {
      if (state.pendingConnection) {
        createConnectedNode(button.dataset.contextNode);
      } else if (state.contextMenuPosition) {
        createNode(button.dataset.contextNode, state.contextMenuPosition);
      }
      hideContextMenu();
    });
  });
  el.graphSelect.addEventListener('change', () => loadGraph(el.graphSelect.value));
  el.newGraphButton.addEventListener('click', createNewGraph);
  el.saveButton.addEventListener('click', saveGraph);
  el.deleteButton.addEventListener('click', deleteSelected);
  el.undoButton.addEventListener('click', undoGraphChange);
  el.redoButton.addEventListener('click', redoGraphChange);
  el.hideDoneToggle.addEventListener('change', () => {
    state.hideDone = el.hideDoneToggle.checked;
    render();
  });
  el.filterInput.addEventListener('input', () => {
    state.filter = el.filterInput.value;
    render();
  });
  el.canvas.addEventListener('click', () => {
    if (state.suppressNextCanvasClick) {
      state.suppressNextCanvasClick = false;
      return;
    }
    clearSelection();
    state.focusMode = 'all';
    render();
  });
  el.canvasWrap.addEventListener('pointerdown', startBoxSelection);
  el.canvasWrap.addEventListener('pointerdown', startCanvasPan);
  el.canvasWrap.addEventListener('auxclick', (event) => {
    if (event.button === 1) {
      event.preventDefault();
    }
  });
  el.canvasWrap.addEventListener('contextmenu', showContextMenu);
  document.addEventListener('click', (event) => {
    if (state.suppressNextDocumentClick) {
      state.suppressNextDocumentClick = false;
      return;
    }
    if (!event.target.closest('.context-menu')) {
      hideContextMenu();
    }
  });
  document.addEventListener('keydown', handleKeyboardDelete);
  document.addEventListener('pointermove', onPointerMove);
  document.addEventListener('pointerup', onPointerUp);
  window.addEventListener('beforeunload', (event) => {
    if (state.dirty) {
      event.preventDefault();
      event.returnValue = '';
    }
  });
  window.addEventListener('resize', applyCanvasTransform);
}

async function init() {
  bindEvents();
  await loadGraphs();
  if (state.graphs.length > 0) {
    await loadGraph(state.graphs[0].id);
  } else {
    render();
  }
}

init().catch((error) => {
  el.validationBar.classList.add('has-errors');
  el.validationBar.textContent = error.message;
});
