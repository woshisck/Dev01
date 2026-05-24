const BASIC_FIELDS = [
  ['storyId', '剧情段 ID'],
  ['arc', '主线 / 支线'],
  ['chapter', '章节'],
  ['timeline', '时间线'],
  ['encounterId', 'EncounterId'],
  ['graphAsset', '流程图资产'],
  ['maps', '地图列表', 'array']
];

const BASIC_FIELD_LABELS = {
  storyId: '剧情段 ID',
  arc: '主线 / 支线',
  chapter: '章节',
  timeline: '时间线',
  encounterId: 'EncounterId / EM_',
  graphAsset: '流程图资产 / EG_',
  maps: '关卡 / RoomData 列表'
};

const FIELD_HELP = {
  '剧情段 ID': '当前剧情段的唯一 ID，用于源文件、筛选和 Codex/UE 导入定位。建议用小写英文和下划线，例如 first_run_tutorial。',
  '主线 / 支线': '剧情所属的大类。主线、支线、教程、系统剧情会影响左侧管理、文件归类和后续工作量统计。',
  '章节': '剧情所属章节或阶段，用来把同一条剧情线拆成可管理的段落，例如 first_run、prison、hub_intro。',
  '时间线': '剧情发生的时间层级，例如现世、过去、记忆、梦境。用于管理穿越、回忆和多时间线剧情。',
  'EncounterId / EM_': 'Encounter 是一次剧情遭遇或剧情段的运行时 ID，通常以 EM_ 开头。UE 导入时会用它定位或创建对应剧情遭遇。',
  '流程图资产 / EG_': 'Story Encounter Graph 资产名，通常以 EG_ 开头。它表示这一段剧情点之间的流程图。',
  '关卡 / RoomData 列表': '这一段剧情涉及的可游玩 RoomData 或关卡范围。主菜单、存档界面这类系统入口不需要放在这里。',
  '状态 / States': '剧情段使用或写入的存档状态、流程状态。用于记录是否看过、是否完成、是否解锁等。',
  '关键道具 / Key Items': '剧情关键物、教程武器、重要卡牌或会影响后续剧情判断的物品列表。',
  '工作量 JSON / Work Items': '无法自动落地到 UE 的人工任务清单，例如放置 Trigger、配置 Portal、制作演出、补 UI 或音效。',
  '扩展 JSON / Extra': '临时或未来新增字段的保留区。schema 暂时不认识的内容可以放在这里，升级字段时不会丢失。',
  '节点 ID': '当前剧情点在本剧情段内的唯一 ID。用于连接、校验和生成资产名，建议用小写英文和下划线。',
  '剧情点资产 EP_': 'Story Encounter Point DataAsset 名，通常以 EP_ 开头。UE 导入时会创建或更新这个剧情点资产。',
  '显示名称': '给人看的中文名称，方便在编辑器、清单和流程图中识别这个剧情点。',
  '剧情点类型': '这个剧情点的触发来源类型，例如区域触发、物件交互、NPC、系统事件、死亡事件或功能解锁。',
  '触发策略': '控制这个剧情点触发频率：仅一次、可重复、或每局一次。',
  '所在关卡 / RoomData': '剧情点实际发生的位置。可选 RoomData，也可以填 System、Any 等虚拟位置表示系统事件。',
  '触发器 / 事件名': '关卡内 Trigger、交互物、UI 事件或系统事件名。它会进入工作量清单，提示后续要绑定什么。',
  '触发条件': '剧情点是否需要前置条件，例如某个进度未完成、已完成，或某个功能已解锁。',
  '条件进度 Key': '触发条件要检查的进度键。只有条件类型需要进度判断时才填写。',
  '玩家看到的剧情事件': '用自然语言描述玩家实际感知到的事件，给剧情、演出、UI、任务配置和 Codex 整理使用。',
  '演出需求': '给关卡、UI、VFX、程序看的实现说明。写清楚镜头、锁门、刷怪、UI 高亮、掉落、传送门等需要如何表现。',
  '动作类型': '剧情点触发后要执行的动作类型。底部操作提示条对应游戏内底部居中的短提示，例如“按下 Space 使用冲刺”；教程弹窗、武器拾取面板和关卡奖励面板是不同 UI。',
  '标题': '动作的标题。对话可用说话者或提示框标题，任务动作可用目标标题。',
  '进度 Key': '动作要写入或记录的进度键，例如 tutorial.weapon_picked，用于后续条件判断。',
  '功能 Tag': '动作要解锁或检查的功能标签，例如背包、传送门、卡组 UI 等系统功能。',
  '任务 Tag': '动作要设置或推进的任务目标标签，用于任务系统或目标追踪。',
  '教程事件 ID': '已有教程系统里的事件 ID。需要调用现有 TutorialManager 或教程弹窗时填写。',
  '教程弹窗页': '可选。用于把一个 TutorialPopup 写成多页内容，保存到 action.tutorialPages。UE 当前仍通过教程事件 ID 查 TutorialRegistry；这些页是剧情源数据和后续生成 DA_Tutorial_* 的依据。',
  '页标题': '教程弹窗单页标题。多页时每页都可以有独立标题。',
  '页正文': '教程弹窗单页正文。会进入 YogCommonRichTextBlock，可以直接写 <input action="Interact"/> 这类输入图标标签。',
  '页副文': '可选的补充说明，也会进入 YogCommonRichTextBlock。',
  '页插图': '可选插图资产路径或占位说明。后续生成教程 DA 时用于绑定 Illustration。',
  '关卡演出 / LevelFlow': '需要播放的关卡演出、镜头、刷怪、Portal 开启或其他 LevelFlow 事件名。',
  '目标 Actor 名称': '关卡里要由剧情控制的 Actor 实例名。用于精确控制一个普通摆放对象，例如 WeaponSpawner_FirstRun_DemoSword。',
  '目标 Actor Tag': '关卡里要由剧情控制的一组 Actor Tag。适合同时显示或隐藏多个对象，例如 Story.FirstRun.DemoWeapon。',
  '启用关卡对象': '勾选表示显示并启用碰撞/Tick；取消勾选表示隐藏并禁用碰撞/Tick。用于剧情控制普通武器、门、提示物、装饰或其他关卡对象。',
  '正文 / 内容': '玩家看到的正文内容，例如底部操作提示、对话文本、提示框说明。WeakHint 会显示为底部居中的短提示条，可以写 <input action="Dash"/> 这类输入图标标签。',
  '区分键鼠/手柄正文': '默认关闭，动作只使用“正文 / 内容”。开启后才显示键鼠正文和手柄正文，并在运行时按当前输入设备选择对应正文。',
  '键鼠正文': '键鼠输入方式下显示的正文。可以写 <input action="Move"/> 这类 Yog Common UI Rich Text 输入标签，并补充 WASD 等键鼠说明。',
  '手柄正文': '手柄输入方式下显示的正文。可以写 <input action="Move"/>、<input action="CameraLook"/> 等输入标签，并补充左摇杆、右摇杆说明。',
  '前置剧情点': '用上一个剧情点的完成进度作为本剧情点的触发条件。选中后会自动写入 condition.kind 和 condition.progressKey。',
  '本点完成 Key': '本剧情点执行完后写入的进度 Key。后续剧情点可以选择它作为前置条件。',
  '本点完成说明': '给人看的进度说明，帮助在清单和校验报告里理解这个 Key 代表什么。',
  '动作节点 ID': '可选的动作稳定 ID。用于以后把动作当成可复用节点、模板或导入目标来管理。',
  '复用 Key': '可选的动作复用标识。同类提示、对话、演出可以使用同一个复用 Key，方便 Codex 归并和后续模板化。'
};

const POINT_KINDS = ['Area', 'Object', 'NPC', 'System', 'Death', 'Feature'];
const FIRE_POLICIES = ['Once', 'Repeat', 'OncePerRun'];
const ACTION_TYPES = [
  'WeakHint',
  'Dialogue',
  'RecordProgress',
  'UnlockFeature',
  'SetQuestObjective',
  'TeleportToNode',
  'PlayLevelFlow',
  'SetActorEnabled',
  'TutorialPopup'
];
const CONDITION_TYPES = ['None', 'ProgressMissing', 'ProgressCompleted', 'RunCountAtLeast', 'FeatureUnlocked'];
const POINT_KIND_LABELS = {
  Area: '区域触发',
  Object: '物件交互',
  NPC: 'NPC / 对话对象',
  System: '系统事件',
  Death: '死亡事件',
  Feature: '功能解锁'
};
const FIRE_POLICY_LABELS = {
  Once: '仅触发一次',
  Repeat: '可重复触发',
  OncePerRun: '每局一次'
};
const ACTION_TYPE_LABELS = {
  WeakHint: '底部操作提示条',
  Dialogue: '对话 / 提示框',
  RecordProgress: '记录进度',
  UnlockFeature: '解锁功能',
  SetQuestObjective: '设置任务目标',
  TeleportToNode: '传送到节点',
  PlayLevelFlow: '播放关卡演出',
  SetActorEnabled: '关卡对象启用/隐藏',
  TutorialPopup: '教程弹窗'
};
const CONDITION_TYPE_LABELS = {
  None: '无条件',
  ProgressMissing: '进度未完成',
  ProgressCompleted: '进度已完成',
  RunCountAtLeast: '至少第 N 局',
  FeatureUnlocked: '功能已解锁'
};
const ARC_PRESETS = [
  { value: 'main', label: '主线' },
  { value: 'side', label: '支线' },
  { value: 'tutorial', label: '教程' },
  { value: 'system', label: '系统' }
];
const TIMELINE_PRESETS = [
  { value: 'present', label: '当前 / 现世' },
  { value: 'past', label: '过去' },
  { value: 'memory', label: '记忆' },
  { value: 'dream', label: '梦境' }
];

const state = {
  segments: [],
  metadata: null,
  metadataFile: '',
  segment: null,
  selectedSourceFile: '',
  dirty: false,
  filter: '',
  selectedPointIndex: null,
  selectedActionIndex: null,
  collapsedPoints: new Set(),
  collapsedActions: new Set(),
  modal: {
    open: false,
    pointIndex: null,
    actionIndex: null
  },
  history: {
    undo: [],
    redo: [],
    savedSnapshot: '',
    isRestoring: false,
    max: 80
  }
};

const el = {
  segmentList: document.getElementById('segmentList'),
  filterInput: document.getElementById('filterInput'),
  refreshButton: document.getElementById('refreshButton'),
  newSegmentButton: document.getElementById('newSegmentButton'),
  saveButton: document.getElementById('saveButton'),
  undoButton: document.getElementById('undoButton'),
  redoButton: document.getElementById('redoButton'),
  syncMetadataButton: document.getElementById('syncMetadataButton'),
  syncUeMetadataButton: document.getElementById('syncUeMetadataButton'),
  codexButton: document.getElementById('codexButton'),
  runCodexButton: document.getElementById('runCodexButton'),
  exportButton: document.getElementById('exportButton'),
  ueDryRunButton: document.getElementById('ueDryRunButton'),
  ueApplyButton: document.getElementById('ueApplyButton'),
  addPointButton: document.getElementById('addPointButton'),
  autoLayoutButton: document.getElementById('autoLayoutButton'),
  reorderByCanvasButton: document.getElementById('reorderByCanvasButton'),
  clearLogButton: document.getElementById('clearLogButton'),
  segmentTitle: document.getElementById('segmentTitle'),
  segmentSubtitle: document.getElementById('segmentSubtitle'),
  saveState: document.getElementById('saveState'),
  basicForm: document.getElementById('basicForm'),
  advancedForm: document.getElementById('advancedForm'),
  pointList: document.getElementById('pointList'),
  selectionInspector: document.getElementById('selectionInspector'),
  logOutput: document.getElementById('logOutput'),
  editModal: document.getElementById('editModal'),
  editModalTitle: document.getElementById('editModalTitle'),
  editModalSubtitle: document.getElementById('editModalSubtitle'),
  editModalContent: document.getElementById('editModalContent'),
  editModalCloseButton: document.getElementById('editModalCloseButton')
};

async function api(path, options) {
  const response = await fetch(path, options);
  const text = await response.text();
  const body = text ? JSON.parse(text) : null;
  if (!response.ok) {
    throw new Error(body?.error || `Request failed: ${response.status}`);
  }
  return body;
}

function log(value) {
  el.logOutput.textContent = typeof value === 'string' ? value : JSON.stringify(value, null, 2);
}

function markDirty() {
  pushHistorySnapshot();
  state.dirty = true;
  renderSaveState();
  renderHistoryButtons();
}

function renderSaveState() {
  el.saveState.textContent = state.dirty ? '有未保存修改' : '已保存';
  el.saveState.classList.toggle('is-dirty', state.dirty);
}

function historyPayload() {
  return {
    segment: state.segment,
    selectedSourceFile: state.selectedSourceFile,
    selectedPointIndex: state.selectedPointIndex,
    selectedActionIndex: state.selectedActionIndex
  };
}

function segmentSnapshot() {
  return JSON.stringify(state.segment || null);
}

function historySnapshot() {
  return JSON.stringify(historyPayload());
}

function recomputeDirtyFromSavedSnapshot() {
  state.dirty = segmentSnapshot() !== state.history.savedSnapshot;
}

function renderHistoryButtons() {
  if (el.undoButton) {
    el.undoButton.disabled = state.history.undo.length <= 1;
  }
  if (el.redoButton) {
    el.redoButton.disabled = state.history.redo.length === 0;
  }
}

function preservePanelScroll(panel, render) {
  const scrollTop = panel ? panel.scrollTop : 0;
  const scrollLeft = panel ? panel.scrollLeft : 0;
  render();
  if (panel) {
    panel.scrollTop = scrollTop;
    panel.scrollLeft = scrollLeft;
  }
}

function inspectorPanel() {
  return el.selectionInspector?.closest('.inspector') || null;
}

function renderPointsPreservingInspector() {
  renderPoints({ preserveInspectorScroll: true, preserveModalScroll: true });
}

function refreshModalPreservingScroll() {
  preservePanelScroll(el.editModalContent, renderEditModal);
}

function resetHistory({ saved = false } = {}) {
  const snapshot = historySnapshot();
  state.history.undo = [snapshot];
  state.history.redo = [];
  if (saved) {
    state.history.savedSnapshot = segmentSnapshot();
  } else {
    state.history.savedSnapshot = '';
  }
  renderHistoryButtons();
}

function pushHistorySnapshot() {
  if (state.history.isRestoring || !state.segment) {
    return;
  }
  const snapshot = historySnapshot();
  if (state.history.undo[state.history.undo.length - 1] === snapshot) {
    return;
  }
  state.history.undo.push(snapshot);
  if (state.history.undo.length > state.history.max) {
    state.history.undo.shift();
  }
  state.history.redo = [];
}

function restoreHistorySnapshot(snapshot) {
  const payload = JSON.parse(snapshot);
  state.history.isRestoring = true;
  state.segment = payload.segment;
  state.selectedSourceFile = payload.selectedSourceFile || state.selectedSourceFile || '';
  state.selectedPointIndex = Number.isInteger(payload.selectedPointIndex) ? payload.selectedPointIndex : null;
  state.selectedActionIndex = Number.isInteger(payload.selectedActionIndex) ? payload.selectedActionIndex : null;
  ensureSelection();
  recomputeDirtyFromSavedSnapshot();
  preservePanelScroll(el.editModalContent, () => preservePanelScroll(inspectorPanel(), renderAll));
  state.history.isRestoring = false;
  renderHistoryButtons();
}

function undoEdit() {
  if (state.history.undo.length <= 1) {
    return;
  }
  const current = state.history.undo.pop();
  state.history.redo.push(current);
  restoreHistorySnapshot(state.history.undo[state.history.undo.length - 1]);
}

function redoEdit() {
  if (state.history.redo.length === 0) {
    return;
  }
  const snapshot = state.history.redo.pop();
  state.history.undo.push(snapshot);
  restoreHistorySnapshot(snapshot);
}

function asLines(value) {
  return Array.isArray(value) ? value.join('\n') : String(value || '');
}

function fromLines(value) {
  return String(value || '')
    .split(/\r?\n|,/)
    .map((line) => line.trim())
    .filter(Boolean);
}

function unique(values) {
  return [...new Set(values.filter(Boolean).map(String))].sort((a, b) => a.localeCompare(b));
}

function assetName(value) {
  return String(value || '').split(/[\\/]/).pop();
}

function metadataEntries(key) {
  return Array.isArray(state.metadata?.[key]) ? state.metadata[key] : [];
}

function metadataValues(key, valueKey = 'path') {
  return unique(metadataEntries(key).map((entry) => entry?.[valueKey] || entry));
}

function metadataValueSet(key, valueKey = 'path') {
  return new Set(metadataValues(key, valueKey));
}

function normalizeChoiceOptions(options) {
  const byValue = new Map();
  for (const option of options || []) {
    const value = typeof option === 'string' ? option : option?.value;
    if (!value || byValue.has(value)) {
      continue;
    }
    byValue.set(value, {
      value,
      label: typeof option === 'string' ? option : (option.label || option.value),
      hideValue: typeof option !== 'string' && (option.hideValue || String(value).startsWith('__'))
    });
  }
  return [...byValue.values()].sort((a, b) => a.label.localeCompare(b.label));
}

function normalizeChoiceOptionsInOrder(options) {
  const byValue = new Map();
  for (const option of options || []) {
    const value = typeof option === 'string' ? option : option?.value;
    if (!value || byValue.has(value)) {
      continue;
    }
    byValue.set(value, {
      value,
      label: typeof option === 'string' ? option : (option.label || option.value),
      hideValue: typeof option !== 'string' && (option.hideValue || String(value).startsWith('__'))
    });
  }
  return [...byValue.values()];
}

function labeledOptions(values, labels) {
  return values.map((value) => ({ value, label: labels[value] || value }));
}

function storyTermValues(key) {
  return unique(state.metadata?.storyTerms?.[key] || []);
}

function currentSegmentValues(key) {
  return unique(state.segments.map((segment) => segment[key]));
}

function graphAssetOptions() {
  return unique([
    ...metadataEntries('storyAssets')
      .filter((entry) => /^EG_/i.test(entry.name))
      .map((entry) => entry.name),
    ...state.segments.map((segment) => segment.graphAsset)
  ]);
}

function pointAssetOptions() {
  return unique(metadataEntries('storyAssets')
    .filter((entry) => /^EP_/i.test(entry.name))
    .map((entry) => entry.name));
}

function progressKeyOptions() {
  const fromCurrent = [];
  if (state.segment) {
    for (const point of state.segment.points || []) {
      if (point.condition) {
        fromCurrent.push(point.condition.progressKey);
      }
      for (const action of point.actions || []) {
        fromCurrent.push(action.progressKey);
      }
    }
    fromCurrent.push(...(state.segment.states || []));
  }
  return unique([...storyTermValues('progressKeys'), ...fromCurrent]);
}

function roomChoiceOptions() {
  return metadataEntries('rooms').map((room) => {
    const title = [room.roomName, room.displayName].filter(Boolean).join(' / ') || room.name || room.path;
    return {
      value: room.path,
      label: `${title} - ${room.name || assetName(room.path)}`
    };
  });
}

function displayChoiceValue(value) {
  const room = metadataEntries('rooms').find((entry) =>
    entry.path === value || entry.roomName === value || entry.name === value
  );
  if (room) {
    return room.roomName || room.displayName || room.name || value;
  }
  return value;
}

function isVirtualMapValue(value) {
  return ['Any', 'Global', 'System', 'None'].includes(String(value || ''));
}

function isKnownMapValue(value) {
  if (!value || isVirtualMapValue(value)) {
    return true;
  }
  const roomPaths = metadataValueSet('rooms');
  const roomNames = metadataValueSet('rooms', 'roomName');
  const roomAssetNames = metadataValueSet('rooms', 'name');
  return (
    roomPaths.size === 0 ||
    roomPaths.has(value) ||
    roomNames.has(value) ||
    roomAssetNames.has(assetName(value))
  );
}

function field(label, input, wide = false) {
  const wrap = document.createElement('label');
  wrap.className = `form-field${wide ? ' is-wide' : ''}`;
  const caption = document.createElement('span');
  caption.className = 'field-label';
  caption.textContent = label;
  const help = FIELD_HELP[label];
  if (help) {
    caption.title = help;
    caption.setAttribute('aria-label', `${label}：${help}`);
    caption.classList.add('has-help');
  }
  wrap.append(caption, input);
  return wrap;
}

function textInput(value, onInput, placeholder = '') {
  const input = document.createElement('input');
  input.className = 'input';
  input.value = value || '';
  input.placeholder = placeholder;
  input.addEventListener('input', () => {
    onInput(input.value);
    markDirty();
    renderHeader();
  });
  return input;
}

let datalistCounter = 0;

function comboInput(value, options, onInput, placeholder = '') {
  const input = textInput(value, onInput, placeholder);
  const cleanOptions = normalizeChoiceOptions(options);
  if (cleanOptions.length === 0) {
    return input;
  }
  const listId = `story-options-${++datalistCounter}`;
  const list = document.createElement('datalist');
  list.id = listId;
  for (const optionChoice of cleanOptions) {
    const option = document.createElement('option');
    option.value = optionChoice.value;
    option.label = optionChoice.label;
    list.append(option);
  }
  input.setAttribute('list', listId);
  const wrap = document.createElement('div');
  wrap.className = 'combo-wrap';
  wrap.append(input, list);
  return wrap;
}

function managedSingleChoiceInput(value, options, onInput, placeholder = 'Custom value') {
  const normalizedOptions = options.map((option) => (
    typeof option === 'string' ? { value: option, label: option } : option
  ));
  const knownValues = new Set(normalizedOptions.map((option) => option.value));
  if (value && !knownValues.has(value)) {
    normalizedOptions.push({ value, label: value });
  }

  const wrap = document.createElement('div');
  wrap.className = 'managed-choice';
  const select = document.createElement('select');
  select.className = 'input';
  const empty = document.createElement('option');
  empty.value = '';
  empty.textContent = '选择已有项';
  select.append(empty);
  for (const option of normalizedOptions) {
    const optionEl = document.createElement('option');
    optionEl.value = option.value;
    optionEl.textContent = option.label && option.label !== option.value ? `${option.label} (${option.value})` : option.value;
    select.append(optionEl);
  }
  select.value = value || '';

  const custom = document.createElement('input');
  custom.className = 'input';
  custom.placeholder = placeholder;
  const add = document.createElement('button');
  add.type = 'button';
  add.textContent = '添加';

  const setValue = (nextValue) => {
    const cleanValue = String(nextValue || '').trim();
    if (!cleanValue) {
      return;
    }
    onInput(cleanValue);
    markDirty();
    renderHeader();
    renderAll();
  };
  select.addEventListener('change', () => setValue(select.value));
  custom.addEventListener('keydown', (event) => {
    if (event.key === 'Enter') {
      event.preventDefault();
      setValue(custom.value);
    }
  });
  add.addEventListener('click', () => setValue(custom.value));
  wrap.append(select, custom, add);
  return wrap;
}

function textArea(value, onInput, placeholder = '') {
  const input = document.createElement('textarea');
  input.value = value || '';
  input.placeholder = placeholder;
  input.addEventListener('input', () => {
    onInput(input.value);
    markDirty();
  });
  return input;
}

function selectInput(value, options, onInput) {
  const input = document.createElement('select');
  input.className = 'input';
  const cleanOptions = normalizeChoiceOptionsInOrder(options);
  if (value && !cleanOptions.some((option) => option.value === value)) {
    cleanOptions.unshift({ value, label: value });
  }
  for (const optionChoice of cleanOptions) {
    const option = document.createElement('option');
    option.value = optionChoice.value;
    option.textContent = !optionChoice.hideValue && optionChoice.label && optionChoice.label !== optionChoice.value
      ? `${optionChoice.label} (${optionChoice.value})`
      : optionChoice.value;
    if (optionChoice.hideValue) {
      option.textContent = optionChoice.label;
    }
    input.append(option);
  }
  input.value = value || cleanOptions[0]?.value || '';
  input.addEventListener('change', () => {
    onInput(input.value);
    markDirty();
  });
  return input;
}

function checkboxInput(value, onChange, label) {
  const wrap = document.createElement('label');
  wrap.className = 'checkbox-row';
  const input = document.createElement('input');
  input.type = 'checkbox';
  input.checked = Boolean(value);
  const text = document.createElement('span');
  text.textContent = label;
  input.addEventListener('change', () => {
    onChange(input.checked);
    markDirty();
    renderPointsPreservingInspector();
  });
  wrap.append(input, text);
  return wrap;
}

function multiChoiceInput(values, options, onChange, placeholder = 'Custom value', config = {}) {
  const selected = Array.isArray(values) ? [...values] : [];
  const wrap = document.createElement('div');
  wrap.className = 'multi-choice';
  const chips = document.createElement('div');
  chips.className = 'multi-choice-chips';
  const controls = document.createElement('div');
  controls.className = 'multi-choice-controls';
  const select = document.createElement('select');
  select.className = 'input';
  const empty = document.createElement('option');
  empty.value = '';
  empty.textContent = '选择已有项';
  select.append(empty);
  for (const optionChoice of normalizeChoiceOptions(options)) {
    const option = document.createElement('option');
    option.value = optionChoice.value;
    option.textContent = optionChoice.label;
    select.append(option);
  }
  const custom = document.createElement('input');
  custom.className = 'input';
  custom.placeholder = placeholder;
  const addCustom = document.createElement('button');
  addCustom.type = 'button';
  addCustom.textContent = '添加';

  const commit = () => {
    const value = (custom.value || select.value || '').trim();
    if (!value || selected.includes(value)) {
      select.value = '';
      custom.value = '';
      return;
    }
    selected.push(value);
    onChange([...selected]);
    markDirty();
    renderHeader();
    renderAll();
  };

  const removeAt = (index) => {
    selected.splice(index, 1);
    onChange([...selected]);
    markDirty();
    renderHeader();
    renderAll();
  };

  select.addEventListener('change', commit);
  custom.addEventListener('keydown', (event) => {
    if (event.key === 'Enter') {
      event.preventDefault();
      commit();
    }
  });
  addCustom.addEventListener('click', commit);

  selected.forEach((value, index) => {
    const chip = document.createElement('button');
    chip.type = 'button';
    chip.className = 'choice-chip';
    if (config.isValid && !config.isValid(value)) {
      chip.classList.add('is-invalid');
    }
    const text = document.createElement('span');
    text.className = 'choice-chip-text';
    text.textContent = displayChoiceValue(value);
    const remove = document.createElement('span');
    remove.className = 'choice-chip-remove';
    remove.textContent = '×';
    chip.append(text, remove);
    chip.title = '点击移除';
    chip.addEventListener('click', () => removeAt(index));
    chips.append(chip);
  });
  if (config.isValid && selected.some((value) => !config.isValid(value))) {
    const cleanup = document.createElement('button');
    cleanup.type = 'button';
    cleanup.className = 'secondary-danger-button';
    cleanup.textContent = '清理不存在项';
    cleanup.addEventListener('click', () => {
      onChange(selected.filter((value) => config.isValid(value)));
      markDirty();
      renderHeader();
      renderAll();
    });
    chips.append(cleanup);
  }
  controls.append(select, custom, addCustom);
  wrap.append(chips, controls);
  return wrap;
}

function renderHeader() {
  if (!state.segment) {
    el.segmentTitle.textContent = '未选择剧情段';
    el.segmentSubtitle.textContent = '';
    return;
  }
  el.segmentTitle.textContent = state.segment.storyId;
  el.segmentSubtitle.textContent = `${state.segment.arc} / ${state.segment.chapter} / ${state.segment.timeline} / ${state.segment.graphAsset}`;
}

function renderSegments() {
  const filter = state.filter.toLowerCase();
  const segments = state.segments.filter((segment) => {
    const text = [
      segment.storyId,
      segment.arc,
      segment.chapter,
      segment.timeline,
      segment.encounterId,
      segment.graphAsset,
      ...(segment.maps || [])
    ].join(' ').toLowerCase();
    return !filter || text.includes(filter);
  });
  el.segmentList.replaceChildren();
  for (const segment of segments) {
    const card = document.createElement('button');
    card.className = `segment-card${segment.sourceFile === state.selectedSourceFile ? ' is-active' : ''}`;
    card.innerHTML = `<strong>${segment.storyId}</strong><span>${segment.arc} / ${segment.chapter} / ${segment.timeline}</span><span>${segment.pointCount} 个剧情点</span>`;
    card.addEventListener('click', () => loadSegment(segment.arc, segment.storyId, segment.sourceFile));
    el.segmentList.append(card);
  }
}

function renderBasicForm() {
  el.basicForm.replaceChildren();
  if (!state.segment) {
    return;
  }
  for (const [key, label, type] of BASIC_FIELDS) {
    const displayLabel = BASIC_FIELD_LABELS[key] || label;
    if (key === 'maps') {
      el.basicForm.append(field(displayLabel, multiChoiceInput(state.segment[key], roomChoiceOptions(), (value) => {
        state.segment[key] = value;
      }, 'RoomName 或 /Game/.../DA_Room', { isValid: isKnownMapValue }), true));
    } else if (key === 'arc') {
      const customArcs = unique([...storyTermValues('arcs'), ...currentSegmentValues('arc')])
        .filter((value) => !ARC_PRESETS.some((preset) => preset.value === value));
      el.basicForm.append(field(displayLabel, managedSingleChoiceInput(state.segment[key], [
        ...ARC_PRESETS,
        ...customArcs.map((value) => ({ value, label: value }))
      ], (value) => {
        state.segment[key] = value;
      }, '自定义剧情线 ID，例如 main_prison')));
    } else if (key === 'chapter') {
      el.basicForm.append(field(displayLabel, comboInput(state.segment[key], [
        ...storyTermValues('chapters'),
        ...currentSegmentValues('chapter')
      ], (value) => {
        state.segment[key] = value;
      })));
    } else if (key === 'timeline') {
      const customTimelines = unique([...storyTermValues('timelines'), ...currentSegmentValues('timeline')])
        .filter((value) => !TIMELINE_PRESETS.some((preset) => preset.value === value));
      el.basicForm.append(field(displayLabel, managedSingleChoiceInput(state.segment[key], [
        ...TIMELINE_PRESETS,
        ...customTimelines.map((value) => ({ value, label: value }))
      ], (value) => {
        state.segment[key] = value;
      }, '自定义时间线 ID，例如 past_prison')));
    } else if (key === 'graphAsset') {
      el.basicForm.append(field(displayLabel, comboInput(state.segment[key], graphAssetOptions(), (value) => {
        state.segment[key] = assetName(value);
      })));
    } else if (key === 'encounterId') {
      el.basicForm.append(field(displayLabel, comboInput(state.segment[key], state.segments.map((segment) => segment.encounterId), (value) => {
        state.segment[key] = value;
      })));
    } else if (type === 'array') {
      el.basicForm.append(field(displayLabel, textArea(asLines(state.segment[key]), (value) => {
        state.segment[key] = fromLines(value);
      }), true));
    } else {
      el.basicForm.append(field(displayLabel, textInput(state.segment[key], (value) => {
        state.segment[key] = value;
      })));
    }
  }
}

function renderAdvancedForm() {
  el.advancedForm.replaceChildren();
  if (!state.segment) {
    return;
  }
  el.advancedForm.append(
    field('状态 / States', textArea(JSON.stringify(state.segment.states || [], null, 2), (value) => {
      try {
        state.segment.states = JSON.parse(value || '[]');
      } catch {
        state.segment.extra = state.segment.extra || {};
        state.segment.extra.statesDraft = value;
      }
    }), true),
    field('关键道具 / Key Items', textArea(JSON.stringify(state.segment.keyItems || [], null, 2), (value) => {
      try {
        state.segment.keyItems = JSON.parse(value || '[]');
      } catch {
        state.segment.extra = state.segment.extra || {};
        state.segment.extra.keyItemsDraft = value;
      }
    }), true),
    field('工作量 JSON / Work Items', textArea(JSON.stringify(state.segment.workItems || [], null, 2), (value) => {
      try {
        state.segment.workItems = JSON.parse(value || '[]');
      } catch {
        // Keep the raw text in extra until it is valid JSON.
        state.segment.extra = state.segment.extra || {};
        state.segment.extra.workItemsDraft = value;
      }
    }), true),
    field('扩展 JSON / Extra', textArea(JSON.stringify(state.segment.extra || {}, null, 2), (value) => {
      try {
        state.segment.extra = JSON.parse(value || '{}');
      } catch {
        state.segment.extra = { rawDraft: value };
      }
    }), true)
  );
}

function pointCollapseKey(point, index) {
  return point.nodeId || point.asset || `point_${index}`;
}

function actionCollapseKey(point, pointIndex, actionIndex) {
  return `${pointCollapseKey(point, pointIndex)}::action_${actionIndex}`;
}

function sanitizeKeyPart(value) {
  return String(value || '')
    .trim()
    .replace(/^EP_/, '')
    .replace(/^EG_/, '')
    .replace(/[^A-Za-z0-9_]+/g, '_')
    .replace(/_+/g, '_')
    .replace(/^_|_$/g, '')
    .toLowerCase();
}

function defaultProgressKey(point) {
  const story = sanitizeKeyPart(state.segment?.storyId) || 'story';
  const node = sanitizeKeyPart(point?.nodeId || point?.asset || point?.displayName) || 'point';
  return `${story}.${node}`;
}

function recordProgressAction(point) {
  return (point?.actions || []).find((action) => action.type === 'RecordProgress' && action.progressKey);
}

function ensureRecordProgressAction(point, pointIndex) {
  point.actions = Array.isArray(point.actions) ? point.actions : [];
  let action = (point.actions || []).find((item) => item.type === 'RecordProgress');
  if (!action) {
    action = {
      type: 'RecordProgress',
      progressKey: defaultProgressKey(point),
      progressLabel: point.displayName ? `已完成：${point.displayName}` : `已完成剧情点 ${pointIndex + 1}`,
      title: '',
      body: ''
    };
    point.actions.push(action);
  }
  if (!action.progressKey) {
    action.progressKey = defaultProgressKey(point);
  }
  return action;
}

function previousProgressOptions(pointIndex) {
  const points = state.segment?.points || [];
  return points.slice(0, pointIndex).map((point, index) => {
    const record = recordProgressAction(point);
    const key = record?.progressKey || defaultProgressKey(point);
    const label = point.displayName || point.nodeId || point.asset || `剧情点 ${index + 1}`;
    return { value: key, label: `${index + 1}. ${label} -> ${key}` };
  });
}

function ensureSelection() {
  const points = state.segment?.points || [];
  if (!state.segment || points.length === 0) {
    state.selectedPointIndex = null;
    state.selectedActionIndex = null;
    closeEditModal();
    return;
  }
  if (!Number.isInteger(state.selectedPointIndex) || state.selectedPointIndex < 0 || state.selectedPointIndex >= points.length) {
    state.selectedPointIndex = 0;
  }
  const actions = points[state.selectedPointIndex].actions || [];
  if (!Number.isInteger(state.selectedActionIndex) || state.selectedActionIndex < 0 || state.selectedActionIndex >= actions.length) {
    state.selectedActionIndex = null;
  }
}

function selectPoint(pointIndex, actionIndex = null) {
  state.selectedPointIndex = pointIndex;
  state.selectedActionIndex = actionIndex;
  const point = state.segment?.points?.[pointIndex];
  if (point && Number.isInteger(actionIndex)) {
    state.collapsedActions.delete(actionCollapseKey(point, pointIndex, actionIndex));
  }
  renderPointsPreservingInspector();
}

function pointDisplayTitle(point, index) {
  return point.displayName || point.asset || point.nodeId || `未命名剧情点 ${index + 1}`;
}

function actionDisplayTitle(action, actionIndex) {
  const type = ACTION_TYPE_LABELS[action?.type] || action?.type || '动作';
  return action?.title || action?.progressLabel || action?.progressKey || `${type} ${actionIndex + 1}`;
}

function actionSummary(action, actionIndex) {
  const type = ACTION_TYPE_LABELS[action?.type] || action?.type || '动作';
  const title = actionDisplayTitle(action, actionIndex);
  return `${type} · ${title}`;
}

function actionGraphDetail(action) {
  if (action?.type === 'TutorialPopup') {
    const pageCount = Array.isArray(action.tutorialPages) ? action.tutorialPages.length : 0;
    return pageCount > 0
      ? `${pageCount} 页教程弹窗`
      : (action.tutorialEventId || '未配置教程弹窗页');
  }
  if (action?.type === 'RecordProgress') {
    return action.progressKey || '未填写进度 Key';
  }
  if (action?.type === 'SetActorEnabled') {
    const target = action.targetActorName || action.targetActorTag || '未填写关卡对象';
    return `${target} -> ${action.actorEnabled === false ? '隐藏/禁用' : '显示/启用'}`;
  }
  return action?.body || action?.levelFlow || action?.tutorialEventId || '未填写动作内容';
}

function actionStatus(action) {
  if (action?.type === 'TutorialPopup') {
    const pageCount = Array.isArray(action.tutorialPages) ? action.tutorialPages.length : 0;
    if (pageCount > 0) {
      return { label: `${pageCount} 页`, className: 'is-ready' };
    }
    return { label: action.tutorialEventId ? '待补页' : '缺教程', className: 'is-warning' };
  }
  if (action?.type === 'RecordProgress') {
    return action.progressKey
      ? { label: '已记录', className: 'is-ready' }
      : { label: '缺 Key', className: 'is-warning' };
  }
  if (action?.type === 'PlayLevelFlow') {
    return action.levelFlow
      ? { label: '已配置', className: 'is-ready' }
      : { label: '缺演出', className: 'is-warning' };
  }
  if (action?.type === 'UnlockFeature') {
    return action.featureTag
      ? { label: '已配置', className: 'is-ready' }
      : { label: '缺 Tag', className: 'is-warning' };
  }
  if (action?.type === 'SetQuestObjective') {
    return action.questTaskTag
      ? { label: '已配置', className: 'is-ready' }
      : { label: '缺任务', className: 'is-warning' };
  }
  if (action?.type === 'TeleportToNode') {
    return action.targetNodeId
      ? { label: '已配置', className: 'is-ready' }
      : { label: '缺目标', className: 'is-warning' };
  }
  if (action?.type === 'SetActorEnabled') {
    return (action.targetActorName || action.targetActorTag)
      ? { label: action.actorEnabled === false ? '隐藏' : '启用', className: 'is-ready' }
      : { label: '缺对象', className: 'is-warning' };
  }
  if (usesInputTextVariants(action)) {
    return (action.inputTextVariants?.keyboardMouse || action.inputTextVariants?.gamepad)
      ? { label: '分设备', className: 'is-ready' }
      : { label: '缺正文', className: 'is-warning' };
  }
  return action?.body
    ? { label: '已填写', className: 'is-ready' }
    : { label: '缺正文', className: 'is-warning' };
}

function addActionToPoint(point, pointIndex) {
  const next = point.actions.length + 1;
  point.actions.push({
    actionId: `${sanitizeKeyPart(point.nodeId || point.asset || 'point')}_action_${String(next).padStart(2, '0')}`,
    type: 'Dialogue',
    title: '',
    body: ''
  });
  state.selectedPointIndex = pointIndex;
  state.selectedActionIndex = point.actions.length - 1;
  if (state.modal.open && state.modal.pointIndex === pointIndex) {
    state.modal.actionIndex = state.selectedActionIndex;
  }
  markDirty();
  renderPointsPreservingInspector();
}

function currentConditionSelection(point) {
  if (!point?.condition || point.condition.kind === 'None') {
    return '';
  }
  return point.condition.progressKey || '';
}

function hasInputTextVariants(action) {
  return Boolean(action?.inputTextVariants?.keyboardMouse || action?.inputTextVariants?.gamepad);
}

function usesInputTextVariants(action) {
  if (action?.useInputTextVariants !== undefined) {
    return action.useInputTextVariants === true;
  }
  return hasInputTextVariants(action);
}

function tutorialPages(action) {
  action.tutorialPages = Array.isArray(action.tutorialPages) ? action.tutorialPages : [];
  return action.tutorialPages;
}

function makeTutorialPage(index = 0, action = null) {
  return {
    title: action?.title || `第 ${index + 1} 页`,
    body: action?.body || '',
    subText: '',
    illustration: ''
  };
}

function renderTutorialPagesEditor(action) {
  const wrap = document.createElement('div');
  wrap.className = 'tutorial-pages-editor';

  const header = document.createElement('div');
  header.className = 'tutorial-pages-header';
  const title = document.createElement('strong');
  title.textContent = '教程弹窗多页';
  const add = document.createElement('button');
  add.type = 'button';
  add.textContent = '新增页';
  add.addEventListener('click', () => {
    const pages = tutorialPages(action);
    pages.push(makeTutorialPage(pages.length, pages.length === 0 ? action : null));
    markDirty();
    renderPointsPreservingInspector();
  });
  header.append(title, add);

  const note = document.createElement('p');
  note.className = 'field-note';
  note.textContent = '多页会保存到 tutorialPages。UE 当前用“教程事件 ID”查 TutorialRegistry，这些页面用于 Codex/后续 Commandlet 生成或更新 DA_Tutorial_*。';
  wrap.append(header, note);

  const pages = tutorialPages(action);
  if (pages.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'selection-empty';
    empty.textContent = '未配置页面。请点击“新增页”编写教程弹窗内容；旧版 body 字段会保留在 JSON 中，但这里不再作为教程弹窗正文编辑。';
    wrap.append(empty);
    return wrap;
  }

  pages.forEach((page, pageIndex) => {
    const pageCard = document.createElement('div');
    pageCard.className = 'tutorial-page-card';
    const pageHeader = document.createElement('div');
    pageHeader.className = 'tutorial-pages-header';
    const pageTitle = document.createElement('strong');
    pageTitle.textContent = `第 ${pageIndex + 1} 页`;
    const remove = document.createElement('button');
    remove.type = 'button';
    remove.className = 'secondary-danger-button';
    remove.textContent = '删除页';
    remove.addEventListener('click', () => {
      pages.splice(pageIndex, 1);
      markDirty();
      renderPointsPreservingInspector();
    });
    pageHeader.append(pageTitle, remove);

    const pageGrid = document.createElement('div');
    pageGrid.className = 'nested-grid';
    pageGrid.append(
      field('页标题', textInput(page.title, (value) => { page.title = value; }, '例如 武器连招')),
      field('页插图', comboInput(page.illustration, metadataValues('tutorials', 'path'), (value) => { page.illustration = value; }, '/Game/Docs/UI/TutorialTex/...')),
      field('页正文', textArea(page.body, (value) => { page.body = value; }, '支持 <input action="Interact"/> 这类 Yog Common Rich Text'), true),
      field('页副文', textArea(page.subText, (value) => { page.subText = value; }, '可选补充说明'), true)
    );
    pageCard.append(pageHeader, pageGrid);
    wrap.append(pageCard);
  });
  return wrap;
}

const STORY_GRAPH_NODE_WIDTH = 380;
const STORY_GRAPH_NODE_HEIGHT = 178;
const STORY_ACTION_NODE_WIDTH = 240;
const STORY_ACTION_NODE_HEIGHT = 58;
const STORY_ACTION_NODE_OFFSET_X = 70;
const STORY_ACTION_NODE_OFFSET_Y = 270;
const STORY_ACTION_NODE_GAP = 74;
const STORY_GRAPH_X_GAP = 460;
const STORY_GRAPH_Y_GAP = 220;
const STORY_GRAPH_PADDING = 80;

function defaultGraphPosition(index) {
  return {
    x: STORY_GRAPH_PADDING + index * STORY_GRAPH_X_GAP,
    y: STORY_GRAPH_PADDING + (index % 2) * 46
  };
}

function graphPosition(point, index) {
  const position = point?.extra?.graphPosition;
  if (position && Number.isFinite(Number(position.x)) && Number.isFinite(Number(position.y))) {
    return {
      x: Number(position.x),
      y: Number(position.y)
    };
  }
  return defaultGraphPosition(index);
}

function setGraphPosition(point, x, y) {
  point.extra = point.extra || {};
  point.extra.graphPosition = {
    x: Math.max(24, Math.round(x)),
    y: Math.max(24, Math.round(y))
  };
}

function graphCanvasSize(points) {
  const bounds = points.flatMap((point, pointIndex) => {
    const position = graphPosition(point, pointIndex);
    const actionBounds = (point.actions || []).map((action, actionIndex) => {
      const actionPosition = actionNodePosition(point, pointIndex, actionIndex);
      return {
        x: actionPosition.x + STORY_ACTION_NODE_WIDTH,
        y: actionPosition.y + STORY_ACTION_NODE_HEIGHT
      };
    });
    return [
      {
        x: position.x + STORY_GRAPH_NODE_WIDTH,
        y: position.y + STORY_GRAPH_NODE_HEIGHT
      },
      ...actionBounds
    ];
  });
  const maxX = Math.max(...bounds.map((position) => position.x), STORY_GRAPH_PADDING);
  const maxY = Math.max(...bounds.map((position) => position.y), STORY_GRAPH_PADDING);
  return {
    width: Math.max(960, maxX + STORY_GRAPH_NODE_WIDTH + STORY_GRAPH_PADDING),
    height: Math.max(680, maxY + STORY_ACTION_NODE_HEIGHT + STORY_GRAPH_PADDING)
  };
}

function edgePathBetween(sourcePosition, targetPosition) {
  const startX = sourcePosition.x + STORY_GRAPH_NODE_WIDTH;
  const startY = sourcePosition.y + 70;
  const endX = targetPosition.x;
  const endY = targetPosition.y + 70;
  const bend = Math.max(80, Math.min(180, Math.abs(endX - startX) * 0.5));
  return `M ${startX} ${startY} C ${startX + bend} ${startY}, ${endX - bend} ${endY}, ${endX} ${endY}`;
}

function actionNodePosition(point, pointIndex, actionIndex) {
  const position = graphPosition(point, pointIndex);
  return {
    x: position.x + STORY_ACTION_NODE_OFFSET_X,
    y: position.y + STORY_ACTION_NODE_OFFSET_Y + actionIndex * STORY_ACTION_NODE_GAP
  };
}

function actionEdgePath(pointPosition, actionPosition, actionIndex) {
  const startX = pointPosition.x + STORY_GRAPH_NODE_WIDTH / 2;
  const startY = pointPosition.y + STORY_GRAPH_NODE_HEIGHT;
  const endX = actionPosition.x + STORY_ACTION_NODE_WIDTH / 2;
  const endY = actionPosition.y;
  const midY = startY + Math.max(46, (endY - startY) * 0.42);
  const offsetX = actionIndex * 18;
  return `M ${startX} ${startY} C ${startX + offsetX} ${midY}, ${endX - offsetX} ${midY}, ${endX} ${endY}`;
}

function renderGraphEdges(svg, points) {
  svg.replaceChildren();
  const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
  const marker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
  marker.setAttribute('id', 'story-arrow');
  marker.setAttribute('viewBox', '0 0 10 10');
  marker.setAttribute('refX', '9');
  marker.setAttribute('refY', '5');
  marker.setAttribute('markerWidth', '7');
  marker.setAttribute('markerHeight', '7');
  marker.setAttribute('orient', 'auto-start-reverse');
  const arrow = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  arrow.setAttribute('d', 'M 0 0 L 10 5 L 0 10 z');
  arrow.setAttribute('class', 'story-edge-arrow');
  marker.append(arrow);
  defs.append(marker);

  const actionMarker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
  actionMarker.setAttribute('id', 'story-action-arrow');
  actionMarker.setAttribute('viewBox', '0 0 10 10');
  actionMarker.setAttribute('refX', '9');
  actionMarker.setAttribute('refY', '5');
  actionMarker.setAttribute('markerWidth', '8');
  actionMarker.setAttribute('markerHeight', '8');
  actionMarker.setAttribute('orient', 'auto-start-reverse');
  const actionArrow = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  actionArrow.setAttribute('d', 'M 0 0 L 10 5 L 0 10 z');
  actionArrow.setAttribute('class', 'story-action-edge-arrow');
  actionMarker.append(actionArrow);
  defs.append(actionMarker);
  svg.append(defs);

  points.forEach((point, index) => {
    if (index + 1 >= points.length) {
      return;
    }
    const next = points[index + 1];
    const sourcePosition = graphPosition(point, index);
    const targetPosition = graphPosition(next, index + 1);
    const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    path.setAttribute('class', `story-edge-path${next.condition?.kind && next.condition.kind !== 'None' ? ' is-conditioned' : ''}`);
    path.setAttribute('d', edgePathBetween(sourcePosition, targetPosition));
    path.setAttribute('marker-end', 'url(#story-arrow)');
    svg.append(path);

    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('class', 'story-edge-label');
    label.setAttribute('x', String((sourcePosition.x + targetPosition.x + STORY_GRAPH_NODE_WIDTH) / 2));
    label.setAttribute('y', String((sourcePosition.y + targetPosition.y) / 2 + 64));
    label.textContent = next.condition?.kind && next.condition.kind !== 'None' ? '前置完成' : '下一步';
    svg.append(label);

    (point.actions || []).forEach((action, actionIndex) => {
      const actionPosition = actionNodePosition(point, index, actionIndex);
      const actionPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      actionPath.setAttribute('class', `story-action-edge-path${action.type === 'RecordProgress' ? ' is-record' : ''}`);
      actionPath.setAttribute('d', actionEdgePath(sourcePosition, actionPosition, actionIndex));
      actionPath.setAttribute('marker-end', 'url(#story-action-arrow)');
      svg.append(actionPath);
    });
  });

  const lastPoint = points[points.length - 1];
  if (lastPoint) {
    const lastIndex = points.length - 1;
    const sourcePosition = graphPosition(lastPoint, lastIndex);
    (lastPoint.actions || []).forEach((action, actionIndex) => {
      const actionPosition = actionNodePosition(lastPoint, lastIndex, actionIndex);
      const actionPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      actionPath.setAttribute('class', `story-action-edge-path${action.type === 'RecordProgress' ? ' is-record' : ''}`);
      actionPath.setAttribute('d', actionEdgePath(sourcePosition, actionPosition, actionIndex));
      actionPath.setAttribute('marker-end', 'url(#story-action-arrow)');
      svg.append(actionPath);
    });
  }
}

function startStoryCanvasPan(event) {
  if (event.button !== 1 && event.button !== 2) {
    return;
  }
  const wrap = event.currentTarget;
  event.preventDefault();
  const pan = {
    startX: event.clientX,
    startY: event.clientY,
    startScrollLeft: wrap.scrollLeft,
    startScrollTop: wrap.scrollTop
  };
  wrap.setPointerCapture(event.pointerId);
  wrap.classList.add('is-panning');

  const move = (moveEvent) => {
    wrap.scrollLeft = pan.startScrollLeft - (moveEvent.clientX - pan.startX);
    wrap.scrollTop = pan.startScrollTop - (moveEvent.clientY - pan.startY);
  };

  const finish = () => {
    wrap.classList.remove('is-panning');
    wrap.removeEventListener('pointermove', move);
    wrap.removeEventListener('pointerup', finish);
    wrap.removeEventListener('pointercancel', finish);
  };

  wrap.addEventListener('pointermove', move);
  wrap.addEventListener('pointerup', finish);
  wrap.addEventListener('pointercancel', finish);
}

function focusStoryGraphNodes(pointIndexes) {
  const wrap = el.pointList.querySelector('.story-canvas-wrap');
  if (!wrap || !state.segment) {
    return;
  }
  const points = state.segment.points || [];
  const indexes = pointIndexes.length ? pointIndexes : points.map((point, index) => index);
  const boxes = indexes.map((index) => {
    const point = points[index];
    const position = graphPosition(point, index);
    const actionCount = (point.actions || []).length;
    return {
      left: position.x,
      top: position.y,
      right: Math.max(position.x + STORY_GRAPH_NODE_WIDTH, position.x + STORY_ACTION_NODE_OFFSET_X + STORY_ACTION_NODE_WIDTH),
      bottom: position.y + STORY_GRAPH_NODE_HEIGHT + (actionCount ? STORY_ACTION_NODE_OFFSET_Y + actionCount * STORY_ACTION_NODE_GAP : 0)
    };
  });
  if (boxes.length === 0) {
    return;
  }
  const left = Math.min(...boxes.map((box) => box.left));
  const top = Math.min(...boxes.map((box) => box.top));
  const right = Math.max(...boxes.map((box) => box.right));
  const bottom = Math.max(...boxes.map((box) => box.bottom));
  wrap.scrollTo({
    left: Math.max(0, left + (right - left) / 2 - wrap.clientWidth / 2),
    top: Math.max(0, top + (bottom - top) / 2 - wrap.clientHeight / 2),
    behavior: 'smooth'
  });
}

function focusSelectedOrAllStoryNodes() {
  if (Number.isInteger(state.selectedPointIndex)) {
    focusStoryGraphNodes([state.selectedPointIndex]);
    return;
  }
  focusStoryGraphNodes([]);
}

function isTypingTarget(target) {
  return ['INPUT', 'TEXTAREA', 'SELECT'].includes(target?.tagName) || target?.isContentEditable;
}

function actionNodeKindClass(action) {
  if (action.type === 'RecordProgress') {
    return ' is-record';
  }
  if (action.type === 'PlayLevelFlow') {
    return ' is-level-flow';
  }
  if (action.type === 'WeakHint' || action.type === 'TutorialPopup') {
    return ' is-ui';
  }
  return '';
}

function actionNodeLabel(action) {
  if (action.type === 'RecordProgress') {
    return '进度记录';
  }
  return ACTION_TYPE_LABELS[action.type] || action.type || '动作节点';
}

function makeGraphEditButton(label, onOpen) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'node-edit-button';
  button.title = label;
  button.setAttribute('aria-label', label);
  button.addEventListener('pointerdown', (event) => {
    event.stopPropagation();
  });
  button.addEventListener('click', (event) => {
    event.preventDefault();
    event.stopPropagation();
    onOpen();
  });
  return button;
}

function startGraphNodeDrag(event, pointIndex, node) {
  if (event.button !== 0 || event.target.closest('.story-action-node, .node-edit-button')) {
    return;
  }
  const point = state.segment?.points?.[pointIndex];
  if (!point) {
    return;
  }
  const start = graphPosition(point, pointIndex);
  const drag = {
    startClientX: event.clientX,
    startClientY: event.clientY,
    startX: start.x,
    startY: start.y,
    moved: false
  };
  node.setPointerCapture(event.pointerId);
  node.classList.add('is-dragging');

  const move = (moveEvent) => {
    const dx = moveEvent.clientX - drag.startClientX;
    const dy = moveEvent.clientY - drag.startClientY;
    if (Math.abs(dx) > 2 || Math.abs(dy) > 2) {
      drag.moved = true;
    }
    const nextX = Math.max(24, drag.startX + dx);
    const nextY = Math.max(24, drag.startY + dy);
    setGraphPosition(point, nextX, nextY);
    node.style.left = `${point.extra.graphPosition.x}px`;
    node.style.top = `${point.extra.graphPosition.y}px`;
    const svg = el.pointList.querySelector('.story-edges');
    if (svg) {
      renderGraphEdges(svg, state.segment.points || []);
    }
  };

  const finish = () => {
    node.classList.remove('is-dragging');
    node.removeEventListener('pointermove', move);
    node.removeEventListener('pointerup', finish);
    node.removeEventListener('pointercancel', finish);
    if (drag.moved) {
      markDirty();
      renderStoryGraph();
    } else {
      selectPoint(pointIndex);
    }
  };

  node.addEventListener('pointermove', move);
  node.addEventListener('pointerup', finish);
  node.addEventListener('pointercancel', finish);
}

function autoLayoutStoryGraph() {
  if (!state.segment) {
    return;
  }
  (state.segment.points || []).forEach((point, index) => {
    setGraphPosition(point, STORY_GRAPH_PADDING + index * STORY_GRAPH_X_GAP, STORY_GRAPH_PADDING + (index % 2) * 46);
  });
  markDirty();
  renderPointsPreservingInspector();
}

function applyLinearConditionsFromOrder() {
  const points = state.segment?.points || [];
  points.forEach((point, index) => {
    point.condition = point.condition || { kind: 'None' };
    if (index === 0) {
      point.condition.kind = 'None';
      point.condition.progressKey = '';
      point.condition.progressLabel = '';
      return;
    }
    if (!['None', 'ProgressCompleted', 'ProgressMissing'].includes(point.condition.kind)) {
      return;
    }
    const previous = points[index - 1];
    const record = ensureRecordProgressAction(previous, index - 1);
    point.condition.kind = 'ProgressCompleted';
    point.condition.progressKey = record.progressKey;
    point.condition.progressLabel = previous.displayName || previous.nodeId || previous.asset || '';
  });
}

function reorderPointsByCanvas() {
  if (!state.segment) {
    return;
  }
  const selectedPoint = state.segment.points?.[state.selectedPointIndex] || null;
  state.segment.points = (state.segment.points || [])
    .map((point, index) => ({ point, index, position: graphPosition(point, index) }))
    .sort((a, b) => a.position.x - b.position.x || a.position.y - b.position.y || a.index - b.index)
    .map((entry) => entry.point);
  applyLinearConditionsFromOrder();
  state.selectedPointIndex = selectedPoint ? state.segment.points.indexOf(selectedPoint) : null;
  state.selectedActionIndex = null;
  markDirty();
  renderAll();
}

function toggleCollapsed(set, key) {
  if (set.has(key)) {
    set.delete(key);
  } else {
    set.add(key);
  }
}

function collapsedSummary(text) {
  const summary = document.createElement('div');
  summary.className = 'collapsed-summary';
  summary.textContent = text || '暂无摘要';
  return summary;
}

function setCollapseButtonState(button, isCollapsed) {
  button.title = isCollapsed ? '展开' : '折叠';
  button.setAttribute('aria-label', button.title);
  button.setAttribute('aria-expanded', String(!isCollapsed));
}

function makeCollapseButton(isCollapsed, onToggle) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'collapse-button';
  setCollapseButtonState(button, isCollapsed);
  button.addEventListener('click', () => onToggle(button));
  return button;
}

function renderAction(point, action, actionIndex, pointIndex, isSelected = false) {
  const collapseKey = actionCollapseKey(point, pointIndex, actionIndex);
  const isCollapsed = state.collapsedActions.has(collapseKey);
  const card = document.createElement('div');
  card.className = `action-card${isCollapsed ? ' is-collapsed' : ''}${isSelected ? ' is-selected' : ''}`;
  const header = document.createElement('div');
  header.className = 'action-card-header';
  header.innerHTML = `<strong>动作 ${actionIndex + 1}</strong>`;
  const actions = document.createElement('div');
  actions.className = 'action-actions';
  const toggle = makeCollapseButton(isCollapsed, (button) => {
    toggleCollapsed(state.collapsedActions, collapseKey);
    const nextCollapsed = state.collapsedActions.has(collapseKey);
    card.classList.toggle('is-collapsed', nextCollapsed);
    setCollapseButtonState(button, nextCollapsed);
  });
  const remove = document.createElement('button');
  remove.className = 'danger-button';
  remove.textContent = '删除';
  remove.addEventListener('click', () => {
    point.actions.splice(actionIndex, 1);
    if (state.selectedPointIndex === pointIndex && state.selectedActionIndex === actionIndex) {
      state.selectedActionIndex = null;
    }
    if (state.modal.open && state.modal.pointIndex === pointIndex && state.modal.actionIndex === actionIndex) {
      state.modal.actionIndex = null;
    }
    markDirty();
    renderPointsPreservingInspector();
  });
  actions.append(toggle, remove);
  header.append(actions);
  const typeLabel = ACTION_TYPE_LABELS[action.type] || action.type || '动作';
  const summary = collapsedSummary(`${typeLabel}${action.title ? ` · ${action.title}` : ''} · ${actionGraphDetail(action)}`);

  const grid = document.createElement('div');
  grid.className = 'nested-grid collapsible-body';
  const useInputVariants = usesInputTextVariants(action);
  grid.append(
    field('动作节点 ID', textInput(action.actionId, (value) => { action.actionId = value; }, '例如 action_hub_move_hint')),
    field('复用 Key', textInput(action.reuseKey, (value) => { action.reuseKey = value; }, '例如 Hint.Move.Basic')),
    field('动作类型', selectInput(action.type, labeledOptions(ACTION_TYPES, ACTION_TYPE_LABELS), (value) => {
      action.type = value;
      if (value === 'TutorialPopup') {
        action.tutorialPages = Array.isArray(action.tutorialPages) ? action.tutorialPages : [];
      }
      if (value === 'SetActorEnabled' && action.actorEnabled === undefined) {
        action.actorEnabled = true;
      }
      setTimeout(renderPointsPreservingInspector, 0);
    })),
    field('标题', textInput(action.title, (value) => { action.title = value; }, '对话标题、提示标题或任务标题')),
    field('进度 Key', comboInput(action.progressKey, progressKeyOptions(), (value) => { action.progressKey = value; }, '例如 tutorial.weapon_picked')),
    field('功能 Tag', comboInput(action.featureTag, state.metadata?.tags || [], (value) => { action.featureTag = value; }, '例如 Feature.Inventory')),
    field('任务 Tag', comboInput(action.questTaskTag, state.metadata?.tags || [], (value) => { action.questTaskTag = value; }, '例如 Quest.FirstRun.OpenPortal')),
    field('教程事件 ID', comboInput(action.tutorialEventId, metadataValues('tutorials', 'name'), (value) => { action.tutorialEventId = value; }, '选择或填写教程事件 ID')),
    field('关卡演出 / LevelFlow', comboInput(action.levelFlow, metadataValues('levelFlows'), (value) => { action.levelFlow = value; }, '需要播放的 LevelFlow 资产或事件名'))
  );
  if (action.type === 'SetActorEnabled') {
    grid.append(
      field('目标 Actor 名称', textInput(action.targetActorName, (value) => { action.targetActorName = value; }, '例如 WeaponSpawner_FirstRun_DemoSword')),
      field('目标 Actor Tag', textInput(action.targetActorTag, (value) => { action.targetActorTag = value; }, '例如 Story.FirstRun.DemoWeapon')),
      field('启用关卡对象', checkboxInput(action.actorEnabled !== false, (checked) => { action.actorEnabled = checked; }, '勾选显示/启用；取消勾选隐藏/禁用'), true)
    );
  }
  if (action.type !== 'TutorialPopup' && action.type !== 'SetActorEnabled') {
    grid.append(
      field('正文 / 内容', textArea(action.body, (value) => { action.body = value; }, '玩家看到的对话、浮窗、提示框正文'), true),
      field('区分键鼠/手柄正文', checkboxInput(useInputVariants, (checked) => {
      action.useInputTextVariants = checked;
      if (checked) {
        action.inputTextVariants = action.inputTextVariants || {};
      }
      }, '开启后按当前输入设备显示键鼠正文或手柄正文'), true)
    );
  }
  if (action.type !== 'TutorialPopup' && action.type !== 'SetActorEnabled' && useInputVariants) {
    grid.append(
      field('键鼠正文', textArea(action.inputTextVariants?.keyboardMouse, (value) => {
        action.inputTextVariants = action.inputTextVariants || {};
        action.inputTextVariants.keyboardMouse = value;
      }, '键鼠输入方式显示，例如 <input action="Move"/> 移动角色。键鼠使用 WASD。'), true),
      field('手柄正文', textArea(action.inputTextVariants?.gamepad, (value) => {
        action.inputTextVariants = action.inputTextVariants || {};
        action.inputTextVariants.gamepad = value;
      }, '手柄输入方式显示，例如 <input action="Move"/> 左摇杆移动，<input action="CameraLook"/> 右摇杆调整镜头。'), true)
    );
  }
  if (action.type === 'TutorialPopup') {
    grid.append(field('教程弹窗页', renderTutorialPagesEditor(action), true));
  }
  card.append(header, summary, grid);
  return card;
}

function renderStoryGraph() {
  const previousWrap = el.pointList.querySelector('.story-canvas-wrap');
  const previousScroll = previousWrap ? { left: previousWrap.scrollLeft, top: previousWrap.scrollTop } : null;
  el.pointList.replaceChildren();
  if (!state.segment) {
    return;
  }
  const points = state.segment.points || [];
  if (points.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'selection-empty';
    empty.textContent = '还没有剧情点。点击右上角“新增剧情点”开始。';
    el.pointList.append(empty);
    return;
  }

  const canvasSize = graphCanvasSize(points);
  const graph = document.createElement('div');
  graph.className = 'story-canvas-wrap story-flow';
  graph.addEventListener('pointerdown', startStoryCanvasPan);
  graph.addEventListener('contextmenu', (event) => event.preventDefault());
  const canvas = document.createElement('div');
  canvas.className = 'story-canvas';
  canvas.style.width = `${canvasSize.width}px`;
  canvas.style.height = `${canvasSize.height}px`;

  const edges = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
  edges.classList.add('story-edges');
  edges.setAttribute('width', String(canvasSize.width));
  edges.setAttribute('height', String(canvasSize.height));
  renderGraphEdges(edges, points);
  canvas.append(edges);

  points.forEach((point, index) => {
    point.actions = Array.isArray(point.actions) ? point.actions : [];
    point.condition = point.condition || { kind: 'None' };
    const position = graphPosition(point, index);

    const node = document.createElement('div');
    node.className = `story-graph-node story-flow-node${state.selectedPointIndex === index && state.selectedActionIndex === null ? ' is-selected' : ''}`;
    node.style.left = `${position.x}px`;
    node.style.top = `${position.y}px`;
    node.setAttribute('role', 'button');
    node.setAttribute('tabindex', '0');
    node.addEventListener('pointerdown', (event) => startGraphNodeDrag(event, index, node));
    node.addEventListener('dblclick', (event) => {
      event.preventDefault();
      event.stopPropagation();
      openEditModal(index, null);
    });
    node.addEventListener('keydown', (event) => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        selectPoint(index);
      }
    });

    const header = document.createElement('div');
    header.className = 'story-node-header';
    const number = document.createElement('span');
    number.className = 'story-node-number';
    number.textContent = String(index + 1).padStart(2, '0');
    const title = document.createElement('strong');
    title.className = 'story-node-title';
    title.textContent = pointDisplayTitle(point, index);
    const meta = document.createElement('span');
    meta.className = 'story-node-meta';
    const condition = point.condition.kind && point.condition.kind !== 'None'
      ? `${CONDITION_TYPE_LABELS[point.condition.kind] || point.condition.kind}: ${point.condition.progressKey || '-'}`
      : '无前置条件';
    const kindLabel = POINT_KIND_LABELS[point.kind] || point.kind || '剧情点';
    const levelLabel = displayChoiceValue(point.placementLevel || 'System');
    meta.textContent = `${kindLabel} · ${levelLabel}`;
    node.title = [
      pointDisplayTitle(point, index),
      `条件：${condition}`,
      point.placementName ? `触发：${point.placementName}` : ''
    ].filter(Boolean).join('\n');
    const titleWrap = document.createElement('div');
    titleWrap.className = 'story-node-title-wrap';
    titleWrap.append(title, meta);
    header.append(number, titleWrap);
    node.append(makeGraphEditButton('编辑剧情点', () => openEditModal(index, null)));

    const summary = document.createElement('span');
    summary.className = 'story-node-event';
    summary.textContent = point.playerFacingEvent || point.placementName || '未填写玩家看到的剧情事件';

    const count = document.createElement('span');
    count.className = 'story-node-action-count';
    count.textContent = `${point.actions.length} 个动作节点`;

    node.append(header, summary, count);
    canvas.append(node);

    point.actions.forEach((action, actionIndex) => {
      const actionPosition = actionNodePosition(point, index, actionIndex);
      const actionNode = document.createElement('div');
      actionNode.className = `story-action-node${actionNodeKindClass(action)}${state.selectedPointIndex === index && state.selectedActionIndex === actionIndex ? ' is-selected' : ''}`;
      actionNode.style.left = `${actionPosition.x}px`;
      actionNode.style.top = `${actionPosition.y}px`;
      actionNode.setAttribute('role', 'button');
      actionNode.setAttribute('tabindex', '0');
      actionNode.title = action.body || action.progressKey || '';
      actionNode.addEventListener('click', (event) => {
        event.stopPropagation();
        selectPoint(index, actionIndex);
      });
      actionNode.addEventListener('dblclick', (event) => {
        event.preventDefault();
        event.stopPropagation();
        openEditModal(index, actionIndex);
      });
      actionNode.addEventListener('keydown', (event) => {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          selectPoint(index, actionIndex);
        }
      });
      const kind = document.createElement('span');
      kind.className = 'story-action-node-kind';
      kind.textContent = actionNodeLabel(action);
      const actionTitle = document.createElement('strong');
      actionTitle.textContent = actionDisplayTitle(action, actionIndex);
      const detail = document.createElement('span');
      detail.className = 'story-action-node-detail';
      detail.textContent = actionGraphDetail(action);

      actionNode.append(kind, actionTitle, detail, makeGraphEditButton('编辑动作节点', () => openEditModal(index, actionIndex)));
      canvas.append(actionNode);
    });
  });
  graph.append(canvas);
  el.pointList.append(graph);
  if (previousScroll) {
    graph.scrollLeft = previousScroll.left;
    graph.scrollTop = previousScroll.top;
  }
}

function renderProgressHelper(point, pointIndex) {
  const helper = document.createElement('div');
  helper.className = 'progress-helper help-card';
  const title = document.createElement('strong');
  title.textContent = '进度 / 条件辅助';
  const note = document.createElement('p');
  note.textContent = '建议：每个剧情点结束时记录一个“本点完成 Key”，下一个剧情点用“前置剧情点”选择它。这样你不需要手写 condition 的 KEY。';
  const previousOptions = [
    { value: '__none__', label: pointIndex === 0 ? '无前置 / 第一剧情点' : '无前置', hideValue: true },
    ...previousProgressOptions(pointIndex)
  ];
  const record = recordProgressAction(point);
  const fields = document.createElement('div');
  fields.className = 'nested-grid';
  fields.append(
    field('前置剧情点', selectInput(currentConditionSelection(point) || '__none__', previousOptions, (value) => {
      point.condition = point.condition || {};
      if (value === '__none__') {
        point.condition.kind = 'None';
        point.condition.progressKey = '';
        point.condition.progressLabel = '';
        return;
      }
      const selected = previousOptions.find((option) => option.value === value);
      const previousPoint = (state.segment.points || [])
        .slice(0, pointIndex)
        .find((candidate) => (recordProgressAction(candidate)?.progressKey || defaultProgressKey(candidate)) === value);
      if (previousPoint && !recordProgressAction(previousPoint)) {
        ensureRecordProgressAction(previousPoint, state.segment.points.indexOf(previousPoint));
      }
      point.condition.kind = 'ProgressCompleted';
      point.condition.progressKey = value;
      point.condition.progressLabel = selected?.label || '';
    })),
    field('本点完成 Key', comboInput(record?.progressKey || defaultProgressKey(point), progressKeyOptions(), (value) => {
      const action = ensureRecordProgressAction(point, pointIndex);
      action.progressKey = value;
    }, '例如 first_run.hub_move_hint')),
    field('本点完成说明', textInput(record?.progressLabel || '', (value) => {
      const action = ensureRecordProgressAction(point, pointIndex);
      action.progressLabel = value;
    }, '例如 已提示移动'))
  );
  const fix = document.createElement('button');
  fix.type = 'button';
  fix.textContent = record ? '已存在 RecordProgress' : '添加缺失的 RecordProgress';
  fix.disabled = Boolean(record);
  fix.addEventListener('click', () => {
    ensureRecordProgressAction(point, pointIndex);
    markDirty();
    renderPointsPreservingInspector();
  });
  helper.append(title, note, fields, fix);
  return helper;
}

function renderPointDetailForm(point, pointIndex) {
  const card = document.createElement('div');
  card.className = 'detail-card';
  const header = document.createElement('div');
  header.className = 'detail-card-header';
  const title = document.createElement('strong');
  title.textContent = `${pointIndex + 1}. ${pointDisplayTitle(point, pointIndex)}`;
  const removePoint = document.createElement('button');
  removePoint.type = 'button';
  removePoint.className = 'danger-button';
  removePoint.textContent = '删除剧情点';
  removePoint.addEventListener('click', () => {
    state.segment.points.splice(pointIndex, 1);
    state.selectedPointIndex = Math.min(pointIndex, (state.segment.points || []).length - 1);
    state.selectedActionIndex = null;
    closeEditModal();
    markDirty();
    renderPointsPreservingInspector();
  });
  header.append(title, removePoint);

  const mapOptions = [
    ...(state.segment.maps || []).map((value) => ({ value, label: displayChoiceValue(value) })),
    ...roomChoiceOptions()
  ];
  const grid = document.createElement('div');
  grid.className = 'nested-grid';
  grid.append(
    field('节点 ID', textInput(point.nodeId, (value) => { point.nodeId = value; }, '例如 hub_move_hint')),
    field('剧情点资产 EP_', comboInput(point.asset, pointAssetOptions(), (value) => { point.asset = assetName(value); }, '例如 EP_FirstRun_HubMoveHint')),
    field('显示名称', textInput(point.displayName, (value) => { point.displayName = value; }, '给自己看的中文名称')),
    field('剧情点类型', selectInput(point.kind, labeledOptions(POINT_KINDS, POINT_KIND_LABELS), (value) => { point.kind = value; })),
    field('触发策略', selectInput(point.firePolicy, labeledOptions(FIRE_POLICIES, FIRE_POLICY_LABELS), (value) => { point.firePolicy = value; })),
    field('所在关卡 / RoomData', comboInput(point.placementLevel, mapOptions, (value) => { point.placementLevel = value; }, '选择 RoomData，或填 System / Any')),
    field('触发器 / 事件名', textInput(point.placementName, (value) => { point.placementName = value; }, '例如 Trigger_Tutorial_Move 或 NewSaveCreated')),
    field('触发条件', selectInput(point.condition.kind, labeledOptions(CONDITION_TYPES, CONDITION_TYPE_LABELS), (value) => { point.condition.kind = value; })),
    field('条件进度 Key', comboInput(point.condition.progressKey, progressKeyOptions(), (value) => { point.condition.progressKey = value; }, '条件需要检查的进度 Key')),
    field('玩家看到的剧情事件', textArea(point.playerFacingEvent, (value) => { point.playerFacingEvent = value; }, '用中文描述玩家实际看到、听到或完成的剧情事件'), true),
    field('演出需求', textArea(point.extra?.stageDirection, (value) => {
      point.extra = point.extra || {};
      point.extra.stageDirection = value;
    }, '给关卡、UI、VFX、程序看的实现说明'), true)
  );
  card.append(header, grid);
  return card;
}

function renderActionInspector(point, pointIndex) {
  point.actions = Array.isArray(point.actions) ? point.actions : [];
  const wrap = document.createElement('div');
  wrap.className = 'detail-card';
  const header = document.createElement('div');
  header.className = 'detail-card-header';
  const title = document.createElement('strong');
  title.textContent = state.selectedActionIndex === null ? '动作节点状态' : `动作节点：${actionDisplayTitle(point.actions[state.selectedActionIndex], state.selectedActionIndex)}`;
  const actions = document.createElement('div');
  actions.className = 'point-actions';
  if (state.selectedActionIndex !== null) {
    const showAll = document.createElement('button');
    showAll.type = 'button';
    showAll.textContent = '返回动作列表';
    showAll.addEventListener('click', () => {
      state.selectedActionIndex = null;
      if (state.modal.open && state.modal.pointIndex === pointIndex) {
        state.modal.actionIndex = null;
      }
      renderPointsPreservingInspector();
    });
    actions.append(showAll);
  }
  const addAction = document.createElement('button');
  addAction.type = 'button';
  addAction.textContent = '新增动作节点';
  addAction.addEventListener('click', () => addActionToPoint(point, pointIndex));
  actions.append(addAction);
  header.append(title, actions);

  const actionList = document.createElement('div');
  actionList.className = 'action-list';
  if (point.actions.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'selection-empty';
    empty.textContent = '这个剧情点还没有动作。';
    actionList.append(empty);
  } else if (state.selectedActionIndex !== null) {
    actionList.append(renderAction(point, point.actions[state.selectedActionIndex], state.selectedActionIndex, pointIndex, true));
  } else {
    const note = document.createElement('p');
    note.className = 'action-status-note';
    note.textContent = '点击动作状态可进入该动作的详细字段。';
    actionList.append(note);
    const statusList = document.createElement('div');
    statusList.className = 'action-status-list';
    point.actions.forEach((action, actionIndex) => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'action-status-row';
      row.addEventListener('click', () => {
        state.selectedPointIndex = pointIndex;
        state.selectedActionIndex = actionIndex;
        if (state.modal.open && state.modal.pointIndex === pointIndex) {
          state.modal.actionIndex = actionIndex;
        }
        renderPointsPreservingInspector();
      });
      const index = document.createElement('span');
      index.className = 'action-status-index';
      index.textContent = String(actionIndex + 1).padStart(2, '0');
      const main = document.createElement('span');
      main.className = 'action-status-main';
      const name = document.createElement('strong');
      name.textContent = actionDisplayTitle(action, actionIndex);
      const detail = document.createElement('span');
      detail.textContent = `${actionNodeLabel(action)} · ${actionGraphDetail(action)}`;
      main.append(name, detail);
      const status = actionStatus(action);
      const badge = document.createElement('span');
      badge.className = `action-status-badge ${status.className}`;
      badge.textContent = status.label;
      row.append(index, main, badge);
      statusList.append(row);
    });
    actionList.append(statusList);
  }
  wrap.append(header, actionList);
  return wrap;
}

function closeEditModal() {
  state.modal.open = false;
  state.modal.pointIndex = null;
  state.modal.actionIndex = null;
  if (el.editModal) {
    el.editModal.hidden = true;
  }
}

function openEditModal(pointIndex, actionIndex = null) {
  state.selectedPointIndex = pointIndex;
  state.selectedActionIndex = Number.isInteger(actionIndex) ? actionIndex : null;
  state.modal.open = true;
  state.modal.pointIndex = pointIndex;
  state.modal.actionIndex = Number.isInteger(actionIndex) ? actionIndex : null;
  renderPointsPreservingInspector();
}

function renderEditModal() {
  if (!el.editModal || !el.editModalContent) {
    return;
  }
  if (!state.modal.open) {
    el.editModal.hidden = true;
    el.editModalContent.replaceChildren();
    return;
  }
  const points = state.segment?.points || [];
  const point = points[state.modal.pointIndex];
  if (!point) {
    closeEditModal();
    return;
  }
  point.actions = Array.isArray(point.actions) ? point.actions : [];
  point.condition = point.condition || { kind: 'None' };
  state.selectedPointIndex = state.modal.pointIndex;

  let actionIndex = Number.isInteger(state.modal.actionIndex) ? state.modal.actionIndex : null;
  if (actionIndex !== null && !point.actions[actionIndex]) {
    actionIndex = null;
    state.modal.actionIndex = null;
  }
  state.selectedActionIndex = actionIndex;

  el.editModal.hidden = false;
  el.editModalTitle.textContent = actionIndex === null
    ? `编辑剧情点：${pointDisplayTitle(point, state.modal.pointIndex)}`
    : `编辑动作：${actionDisplayTitle(point.actions[actionIndex], actionIndex)}`;
  el.editModalSubtitle.textContent = actionIndex === null
    ? '双击剧情点或点击节点右上角图标可再次打开。'
    : `${pointDisplayTitle(point, state.modal.pointIndex)} / 动作 ${actionIndex + 1}`;

  const stack = document.createElement('div');
  stack.className = 'modal-editor-stack';
  if (actionIndex === null) {
    stack.append(
      renderProgressHelper(point, state.modal.pointIndex),
      renderPointDetailForm(point, state.modal.pointIndex),
      renderActionInspector(point, state.modal.pointIndex)
    );
  } else {
    stack.append(renderActionInspector(point, state.modal.pointIndex));
  }
  el.editModalContent.replaceChildren(stack);
}

function renderSelectedPointInspector(options = {}) {
  const render = () => {
    el.selectionInspector.replaceChildren();
    if (!state.segment) {
      el.selectionInspector.textContent = '先在左侧选择或新建一个剧情段。';
      return;
    }
    ensureSelection();
    if (state.selectedPointIndex === null) {
      el.selectionInspector.textContent = '当前剧情段还没有剧情点。';
      return;
    }
    const point = state.segment.points[state.selectedPointIndex];
    point.actions = Array.isArray(point.actions) ? point.actions : [];
    point.condition = point.condition || { kind: 'None' };
    el.selectionInspector.append(
      renderProgressHelper(point, state.selectedPointIndex),
      renderPointDetailForm(point, state.selectedPointIndex),
      renderActionInspector(point, state.selectedPointIndex)
    );
  };
  if (options.preserveScroll) {
    preservePanelScroll(inspectorPanel(), render);
    return;
  }
  render();
}

function renderPoints(options = {}) {
  ensureSelection();
  renderStoryGraph();
  renderSelectedPointInspector(options);
  if (options.preserveModalScroll) {
    preservePanelScroll(el.editModalContent, renderEditModal);
  } else {
    renderEditModal();
  }
}

function renderAll() {
  renderHeader();
  renderSaveState();
  renderHistoryButtons();
  renderSegments();
  renderBasicForm();
  renderAdvancedForm();
  renderPoints();
}

async function refreshSegments() {
  const body = await api('/api/story/segments');
  state.segments = body.segments || [];
  renderSegments();
}

async function refreshMetadata() {
  const body = await api('/api/story/metadata');
  state.metadata = body.metadata || null;
  state.metadataFile = body.metadataFile || '';
  return body;
}

async function syncMetadata() {
  log('正在同步项目地图和资产列表...');
  const body = await api('/api/story/sync-metadata', { method: 'POST' });
  state.metadata = body.metadata || null;
  state.metadataFile = body.metadataFile || '';
  log({
    metadataFile: body.metadataFile,
    summary: body.metadata?.summary || {}
  });
  renderAll();
}

async function syncUeMetadata() {
  log('正在启动 UE 读取 RoomData / Campaign DA，请稍等...');
  const body = await api('/api/story/sync-ue-metadata', { method: 'POST' });
  state.metadata = body.metadata || null;
  state.metadataFile = body.metadataFile || '';
  log({
    commandlet: body.commandlet,
    metadataFile: body.metadataFile,
    summary: body.metadata?.summary || {},
    ueMetadataFile: body.metadata?.ueMetadataFile || ''
  });
  renderAll();
}

async function loadSegment(arc, storyId, sourceFile) {
  if (state.dirty && !confirm('当前剧情段有未保存修改，确定切换吗？')) {
    return;
  }
  const body = await api(`/api/story/segment?arc=${encodeURIComponent(arc)}&storyId=${encodeURIComponent(storyId)}`);
  state.segment = body.segment;
  state.selectedSourceFile = sourceFile;
  state.dirty = false;
  state.selectedPointIndex = (state.segment.points || []).length ? 0 : null;
  state.selectedActionIndex = null;
  resetHistory({ saved: true });
  renderAll();
}

async function newSegment() {
  const body = await api('/api/story/default');
  state.segment = body.segment;
  state.selectedSourceFile = '';
  state.dirty = true;
  state.selectedPointIndex = (state.segment.points || []).length ? 0 : null;
  state.selectedActionIndex = null;
  resetHistory({ saved: false });
  renderAll();
}

async function saveSegment() {
  if (!state.segment) {
    return;
  }
  const body = await api('/api/story/segment', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({ segment: state.segment })
  });
  state.segment = body.segment;
  state.selectedSourceFile = body.sourceFile;
  state.dirty = false;
  ensureSelection();
  resetHistory({ saved: true });
  log(body);
  await refreshSegments();
  renderAll();
}

async function createCodexRequest() {
  if (!state.selectedSourceFile) {
    await saveSegment();
  }
  const body = await api('/api/story/codex-request', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({
      mode: '导出引擎manifest',
      sourceFiles: state.selectedSourceFile ? [state.selectedSourceFile] : [],
      rules: '整理剧情字段，检查 tag/ID/工作量，输出 StoryImportManifest.json、StoryWorkload.md 和校验报告。不要修改 UE 地图。'
    })
  });
  log(body);
}

async function runCodexCleanup() {
  if (state.dirty || !state.selectedSourceFile) {
    await saveSegment();
  }
  log('Codex 正在整理剧情源文件，请等待...');
  const body = await api('/api/story/run-codex', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({
      mode: '整理剧情字段',
      sourceFiles: state.selectedSourceFile ? [state.selectedSourceFile] : [],
      rules: [
        '整理剧情字段，补全明显缺失的工作量。',
        '检查 EncounterId、NodeId、EP 资产名、ProgressKey、FeatureTag、QuestTaskTag。',
        '生成或更新 StoryImportManifest.json、StoryWorkload.md、ValidationReport.md。',
        '保留 schemaVersion、extra 和未知字段。',
        '不要修改 UE 地图，不要执行 StoryImport Apply。'
      ].join('\n')
    })
  });
  log(body);
  await refreshSegments();
}

async function exportManifest() {
  if (state.dirty) {
    await saveSegment();
  }
  const body = await api('/api/story/export', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({
      sourceFiles: state.selectedSourceFile ? [state.selectedSourceFile] : []
    })
  });
  log(body);
  return body;
}

async function runStoryImport(apply) {
  if (state.dirty) {
    await saveSegment();
  }
  const exported = await exportManifest();
  log(`正在执行 StoryImport ${apply ? 'Apply' : 'Dry Run'}...`);
  const body = await api('/api/story/import-ue', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify({
      apply,
      manifestPath: exported.manifestFile
    })
  });
  log(body);
}

el.refreshButton.addEventListener('click', refreshSegments);
el.newSegmentButton.addEventListener('click', newSegment);
el.saveButton.addEventListener('click', saveSegment);
el.undoButton.addEventListener('click', undoEdit);
el.redoButton.addEventListener('click', redoEdit);
el.editModalCloseButton.addEventListener('click', closeEditModal);
el.editModal.addEventListener('pointerdown', (event) => {
  if (event.target === el.editModal) {
    closeEditModal();
  }
});
el.syncMetadataButton.addEventListener('click', syncMetadata);
el.syncUeMetadataButton.addEventListener('click', syncUeMetadata);
el.codexButton.addEventListener('click', createCodexRequest);
el.runCodexButton.addEventListener('click', runCodexCleanup);
el.exportButton.addEventListener('click', exportManifest);
el.ueDryRunButton.addEventListener('click', () => runStoryImport(false));
el.ueApplyButton.addEventListener('click', () => {
  if (confirm('Apply 会写入或更新 UE Story Encounter 资产，但不会修改地图。确定继续吗？')) {
    runStoryImport(true);
  }
});
el.autoLayoutButton.addEventListener('click', autoLayoutStoryGraph);
el.reorderByCanvasButton.addEventListener('click', reorderPointsByCanvas);
el.addPointButton.addEventListener('click', () => {
  if (!state.segment) {
    return;
  }
  const next = state.segment.points.length + 1;
  const nodeId = `point_${String(next).padStart(2, '0')}`;
  const previous = state.segment.points[next - 2];
  const previousKey = previous ? (recordProgressAction(previous)?.progressKey || defaultProgressKey(previous)) : '';
  state.segment.points.push({
    nodeId,
    asset: `EP_${state.segment.graphAsset.replace(/^EG_/, '')}_Point_${String(next).padStart(2, '0')}`,
    displayName: `剧情点 ${next}`,
    kind: 'System',
    playerFacingEvent: '',
    firePolicy: 'Once',
    placementLevel: state.segment.maps?.[0] || '',
    placementName: `Trigger_${String(next).padStart(2, '0')}`,
    condition: previousKey ? { kind: 'ProgressCompleted', progressKey: previousKey } : { kind: 'None' },
    actions: [
      { type: 'Dialogue', actionId: `${nodeId}_dialogue`, title: '', body: '' },
      { type: 'RecordProgress', actionId: `${nodeId}_record`, progressKey: `${sanitizeKeyPart(state.segment.storyId) || 'story'}.${nodeId}`, progressLabel: `已完成剧情点 ${next}` }
    ],
    extra: {}
  });
  state.selectedPointIndex = state.segment.points.length - 1;
  state.selectedActionIndex = null;
  markDirty();
  renderPointsPreservingInspector();
});
el.clearLogButton.addEventListener('click', () => log(''));
el.filterInput.addEventListener('input', () => {
  state.filter = el.filterInput.value;
  renderSegments();
});
document.addEventListener('keydown', (event) => {
  if (event.key === 'Escape' && state.modal.open) {
    event.preventDefault();
    closeEditModal();
    return;
  }
  if ((event.ctrlKey || event.metaKey) && !event.altKey) {
    const key = event.key.toLowerCase();
    if (key === 'z' && !event.shiftKey) {
      event.preventDefault();
      undoEdit();
      return;
    }
    if (key === 'y' || (key === 'z' && event.shiftKey)) {
      event.preventDefault();
      redoEdit();
      return;
    }
  }
  if (isTypingTarget(event.target) || event.ctrlKey || event.metaKey || event.altKey) {
    return;
  }
  if (event.key.toLowerCase() === 'f') {
    event.preventDefault();
    focusSelectedOrAllStoryNodes();
  }
});

Promise.all([refreshSegments(), refreshMetadata()]).then(renderAll).catch((error) => log(error.stack || error.message));
