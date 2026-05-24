const fs = require('fs');
const path = require('path');
const storyMetadata = require('./story_metadata');

const STORY_ID_PATTERN = /^[A-Za-z0-9_.-]+$/;
const SAFE_PATH_PATTERN = /^[A-Za-z0-9_.-]+$/;
const SCHEMA_VERSION = 1;

const ACTION_TYPES = new Set([
  'WeakHint',
  'Dialogue',
  'RecordProgress',
  'UnlockFeature',
  'SetQuestObjective',
  'TeleportToNode',
  'PlayLevelFlow',
  'SetActorEnabled',
  'TutorialPopup'
]);

const NODE_KINDS = new Set(['Area', 'Object', 'NPC', 'System', 'Death', 'Feature']);
const FIRE_POLICIES = new Set(['Once', 'Repeat', 'OncePerRun']);
const CONDITION_KINDS = new Set([
  'None',
  'ProgressMissing',
  'ProgressCompleted',
  'RunCountAtLeast',
  'FeatureUnlocked'
]);

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

function writeText(filePath, value) {
  ensureDir(path.dirname(filePath));
  fs.writeFileSync(filePath, value, 'utf8');
}

function assertSafeSegment(value, label) {
  if (typeof value !== 'string' || !SAFE_PATH_PATTERN.test(value)) {
    throw new Error(`${label} must use letters, numbers, dot, underscore, or dash: ${value}`);
  }
}

function assertStoryId(value, label) {
  if (typeof value !== 'string' || !STORY_ID_PATTERN.test(value)) {
    throw new Error(`${label} must use letters, numbers, dot, underscore, or dash: ${value}`);
  }
}

function toArray(value) {
  return Array.isArray(value) ? value : [];
}

function cleanString(value, fallback = '') {
  if (value === undefined || value === null) {
    return fallback;
  }
  return String(value);
}

function cleanObject(value) {
  return value && typeof value === 'object' && !Array.isArray(value) ? value : {};
}

function cleanBool(value, fallback = false) {
  if (value === undefined || value === null) {
    return fallback;
  }
  if (typeof value === 'boolean') {
    return value;
  }
  return String(value).toLowerCase() === 'true';
}

function normalizeListEntry(value) {
  return typeof value === 'string' ? value : cleanObject(value);
}

function describeListEntry(value) {
  return typeof value === 'string' ? value : JSON.stringify(value);
}

function slugFromStoryId(storyId) {
  return cleanString(storyId, 'story_segment')
    .trim()
    .replace(/[^A-Za-z0-9_.-]+/g, '_')
    .replace(/^_+|_+$/g, '') || 'story_segment';
}

function defaultSegment() {
  return normalizeSegment({
    schemaVersion: SCHEMA_VERSION,
    storyId: 'main_prison_awaken',
    arc: 'main',
    chapter: 'prison',
    timeline: 'present',
    encounterId: 'EM_Prison_Awakening',
    graphAsset: 'EG_Prison_Awakening',
    maps: ['Prison_Lower'],
    points: [],
    states: [],
    keyItems: [],
    workItems: [],
    extra: {}
  });
}

function defaultPoint(index = 1) {
  const suffix = String(index).padStart(2, '0');
  return {
    nodeId: `point_${suffix}`,
    asset: `EP_Point_${suffix}`,
    displayName: `剧情点 ${suffix}`,
    kind: 'System',
    playerFacingEvent: '',
    firePolicy: 'Once',
    placementLevel: '',
    placementName: `Trigger_Point_${suffix}`,
    condition: { kind: 'None' },
    actions: [
      {
        type: 'Dialogue',
        title: '',
        body: ''
      }
    ],
    extra: {}
  };
}

function normalizeCondition(condition) {
  const normalized = cleanObject(condition);
  const kind = cleanString(normalized.kind || normalized.Kind, 'None');
  return {
    ...normalized,
    kind: CONDITION_KINDS.has(kind) ? kind : 'None',
    progressKey: cleanString(normalized.progressKey || normalized.ProgressKey),
    progressLabel: cleanString(normalized.progressLabel || normalized.ProgressLabel),
    featureTag: cleanString(normalized.featureTag || normalized.FeatureTag),
    runCount: Number(normalized.runCount || normalized.RunCount || 0)
  };
}

function normalizeTutorialPage(page, index = 0) {
  const normalized = cleanObject(page);
  return {
    ...normalized,
    title: cleanString(normalized.title || normalized.Title, `第 ${index + 1} 页`),
    body: cleanString(normalized.body || normalized.Body),
    subText: cleanString(normalized.subText || normalized.SubText),
    illustration: cleanString(normalized.illustration || normalized.Illustration)
  };
}

function normalizeAction(action) {
  const normalized = cleanObject(action);
  const type = cleanString(normalized.type || normalized.Kind || normalized.kind, 'WeakHint');
  const actionType = ACTION_TYPES.has(type) ? type : 'WeakHint';
  const inputTextVariants = cleanObject(normalized.inputTextVariants || normalized.InputTextVariants);
  const keyboardMouseText = cleanString(inputTextVariants.keyboardMouse || inputTextVariants.KeyboardMouse || normalized.keyboardMouseBody || normalized.KeyboardMouseBody);
  const gamepadText = cleanString(inputTextVariants.gamepad || inputTextVariants.Gamepad || normalized.gamepadBody || normalized.GamepadBody);
  const hasInputTextVariants = keyboardMouseText || gamepadText || Object.keys(inputTextVariants).length > 0;
  const rawUseInputTextVariants = normalized.useInputTextVariants ?? normalized.UseInputTextVariants ?? normalized.bUseInputTextVariants;
  const useInputTextVariants = cleanBool(rawUseInputTextVariants, Boolean(hasInputTextVariants));
  const targetActorName = cleanString(normalized.targetActorName || normalized.TargetActorName);
  const targetActorTag = cleanString(normalized.targetActorTag || normalized.TargetActorTag);
  const rawActorEnabled = normalized.actorEnabled ?? normalized.ActorEnabled ?? normalized.bActorEnabled;
  const hasActorControlFields = actionType === 'SetActorEnabled' || targetActorName || targetActorTag || rawActorEnabled !== undefined;
  return {
    ...normalized,
    actionId: cleanString(normalized.actionId || normalized.ActionId),
    reuseKey: cleanString(normalized.reuseKey || normalized.ReuseKey),
    type: actionType,
    title: cleanString(normalized.title || normalized.Title),
    body: cleanString(normalized.body || normalized.Body),
    ...(useInputTextVariants || rawUseInputTextVariants !== undefined ? { useInputTextVariants } : {}),
    ...(hasInputTextVariants ? { inputTextVariants: {
      ...inputTextVariants,
      keyboardMouse: keyboardMouseText,
      gamepad: gamepadText
    } } : {}),
    tutorialPages: toArray(normalized.tutorialPages || normalized.TutorialPages).map(normalizeTutorialPage),
    tutorialEventId: cleanString(normalized.tutorialEventId || normalized.TutorialEventId),
    pauseGame: normalized.pauseGame === undefined ? normalized.bPauseGame !== false : normalized.pauseGame !== false,
    progressKey: cleanString(normalized.progressKey || normalized.ProgressKey),
    progressLabel: cleanString(normalized.progressLabel || normalized.ProgressLabel),
    featureTag: cleanString(normalized.featureTag || normalized.FeatureTag),
    questTaskTag: cleanString(normalized.questTaskTag || normalized.QuestTaskTag),
    targetNodeId: cleanString(normalized.targetNodeId || normalized.TargetNodeId),
    ...(hasActorControlFields ? {
      targetActorName,
      targetActorTag,
      actorEnabled: cleanBool(rawActorEnabled, true)
    } : {}),
    levelFlow: cleanString(normalized.levelFlow || normalized.LevelFlow)
  };
}

function normalizePoint(point, index = 0) {
  const normalized = cleanObject(point);
  const fallback = defaultPoint(index + 1);
  const kind = cleanString(normalized.kind || normalized.Kind, fallback.kind);
  const firePolicy = cleanString(normalized.firePolicy || normalized.FirePolicy, fallback.firePolicy);
  return {
    ...normalized,
    nodeId: cleanString(normalized.nodeId || normalized.NodeId, fallback.nodeId),
    asset: cleanString(normalized.asset || normalized.Asset, fallback.asset),
    displayName: cleanString(normalized.displayName || normalized.DisplayName, fallback.displayName),
    kind: NODE_KINDS.has(kind) ? kind : fallback.kind,
    playerFacingEvent: cleanString(normalized.playerFacingEvent || normalized.PlayerFacingEvent),
    firePolicy: FIRE_POLICIES.has(firePolicy) ? firePolicy : fallback.firePolicy,
    placementLevel: cleanString(normalized.placementLevel || normalized.PlacementLevel),
    placementName: cleanString(normalized.placementName || normalized.PlacementName, fallback.placementName),
    condition: normalizeCondition(normalized.condition || normalized.Condition),
    actions: toArray(normalized.actions || normalized.Actions).map(normalizeAction),
    extra: cleanObject(normalized.extra)
  };
}

function normalizeSegment(segment) {
  const normalized = cleanObject(segment);
  const storyId = cleanString(normalized.storyId || normalized.story_id, 'main_prison_awaken');
  const arc = cleanString(normalized.arc, 'main');
  const chapter = cleanString(normalized.chapter, 'chapter');
  const timeline = cleanString(normalized.timeline, 'present');
  return {
    ...normalized,
    schemaVersion: Number(normalized.schemaVersion || normalized.schema_version || SCHEMA_VERSION),
    storyId,
    arc,
    chapter,
    timeline,
    encounterId: cleanString(normalized.encounterId || normalized.encounter_id, `EM_${slugFromStoryId(storyId)}`),
    graphAsset: cleanString(normalized.graphAsset || normalized.graph_asset, `EG_${slugFromStoryId(storyId)}`),
    maps: toArray(normalized.maps).map(String),
    points: toArray(normalized.points).map(normalizePoint),
    states: toArray(normalized.states).map(normalizeListEntry),
    keyItems: toArray(normalized.keyItems || normalized.key_items).map(normalizeListEntry),
    workItems: toArray(normalized.workItems || normalized.work_items).map((item) =>
      typeof item === 'string' ? { type: 'task', title: item, status: 'todo' } : cleanObject(item)
    ),
    extra: cleanObject(normalized.extra)
  };
}

function segmentFile(rootDir, arc, storyId) {
  assertSafeSegment(arc, 'arc');
  assertStoryId(storyId, 'storyId');
  return path.join(rootDir, 'Docs', 'StorySource', arc, `${storyId}.story.json`);
}

function markdownFile(rootDir, segment) {
  return path.join(rootDir, 'Docs', 'StorySource', segment.arc, `${segment.storyId}.story.md`);
}

function listSegmentFiles(rootDir) {
  const sourceRoot = path.join(rootDir, 'Docs', 'StorySource');
  if (!fs.existsSync(sourceRoot)) {
    return [];
  }
  const out = [];
  const walk = (dirPath) => {
    for (const entry of fs.readdirSync(dirPath, { withFileTypes: true })) {
      const fullPath = path.join(dirPath, entry.name);
      if (entry.isDirectory()) {
        walk(fullPath);
      } else if (entry.isFile() && entry.name.endsWith('.story.json')) {
        out.push(fullPath);
      }
    }
  };
  walk(sourceRoot);
  return out.sort();
}

function listSegments(rootDir) {
  return listSegmentFiles(rootDir).map((filePath) => {
    const segment = normalizeSegment(readJson(filePath));
    return {
      storyId: segment.storyId,
      arc: segment.arc,
      chapter: segment.chapter,
      timeline: segment.timeline,
      encounterId: segment.encounterId,
      graphAsset: segment.graphAsset,
      maps: segment.maps,
      pointCount: segment.points.length,
      sourceFile: path.relative(rootDir, filePath).replace(/\\/g, '/')
    };
  });
}

function readSegment(rootDir, arc, storyId) {
  return normalizeSegment(readJson(segmentFile(rootDir, arc, storyId)));
}

function renderMarkdown(segment) {
  const pointLines = segment.points.map((point) =>
    `| \`${point.asset}\` | \`${point.nodeId}\` | ${point.displayName} | ${point.kind} | ${point.placementLevel || '-'} / ${point.placementName || '-'} |`
  );
  const workLines = segment.workItems.map((item) =>
    `- [${item.status === 'done' ? 'x' : ' '}] ${item.type || 'task'}: ${item.title || item.description || JSON.stringify(item)}`
  );
  return [
    `# ${segment.storyId}`,
    '',
    '```json',
    JSON.stringify({
      schemaVersion: segment.schemaVersion,
      storyId: segment.storyId,
      arc: segment.arc,
      chapter: segment.chapter,
      timeline: segment.timeline,
      encounterId: segment.encounterId,
      graphAsset: segment.graphAsset,
      maps: segment.maps
    }, null, 2),
    '```',
    '',
    '## 剧情点',
    '',
    '| Asset | NodeId | 名称 | 类型 | 放置 |',
    '| --- | --- | --- | --- | --- |',
    ...(pointLines.length ? pointLines : ['| - | - | - | - | - |']),
    '',
    '## 状态与关键道具',
    '',
    `- States: ${segment.states.map(describeListEntry).join(', ') || '-'}`,
    `- KeyItems: ${segment.keyItems.map(describeListEntry).join(', ') || '-'}`,
    '',
    '## 工作量',
    '',
    ...(workLines.length ? workLines : ['- [ ] 暂无']),
    ''
  ].join('\n');
}

function writeSegment(rootDir, segment) {
  const normalized = normalizeSegment(segment);
  assertSafeSegment(normalized.arc, 'arc');
  assertStoryId(normalized.storyId, 'storyId');
  const jsonPath = segmentFile(rootDir, normalized.arc, normalized.storyId);
  writeJson(jsonPath, normalized);
  writeText(markdownFile(rootDir, normalized), renderMarkdown(normalized));
  return {
    segment: normalized,
    sourceFile: path.relative(rootDir, jsonPath).replace(/\\/g, '/')
  };
}

function collectSegments(rootDir, sourceFiles = []) {
  if (!sourceFiles || sourceFiles.length === 0) {
    return listSegmentFiles(rootDir).map((filePath) => normalizeSegment(readJson(filePath)));
  }
  return sourceFiles.map((sourceFile) => {
    const filePath = path.resolve(rootDir, sourceFile);
    if (!filePath.startsWith(rootDir)) {
      throw new Error(`Source file escapes project root: ${sourceFile}`);
    }
    return normalizeSegment(readJson(filePath));
  });
}

function asMetadataSet(metadata, key, valueKey = 'path') {
  if (!metadata || !Array.isArray(metadata[key])) {
    return new Set();
  }
  return new Set(metadata[key].map((entry) => String(entry?.[valueKey] || entry)).filter(Boolean));
}

function assetName(value) {
  return String(value || '').split(/[\\/]/).pop();
}

function isVirtualPlacementLevel(value) {
  return ['Any', 'Global', 'System', 'None'].includes(String(value || ''));
}

function isKnownMapOrRoom(value, sets) {
  if (!value || isVirtualPlacementLevel(value)) {
    return true;
  }
  return (
    sets.knownMaps.has(value) ||
    sets.knownMapNames.has(assetName(value)) ||
    sets.knownRooms.has(value) ||
    sets.knownRoomNames.has(value) ||
    sets.knownRoomAssetNames.has(assetName(value))
  );
}

function validateSegments(segments, metadata = null) {
  const errors = [];
  const warnings = [];
  const missingRooms = new Set();
  const storyIds = new Set();
  const knownSets = {
    knownMaps: asMetadataSet(metadata, 'maps'),
    knownMapNames: asMetadataSet(metadata, 'maps', 'name'),
    knownRooms: asMetadataSet(metadata, 'rooms'),
    knownRoomNames: asMetadataSet(metadata, 'rooms', 'roomName'),
    knownRoomAssetNames: asMetadataSet(metadata, 'rooms', 'name')
  };
  const knownTags = new Set(Array.isArray(metadata?.tags) ? metadata.tags : []);
  const knownLevelFlows = asMetadataSet(metadata, 'levelFlows');
  const knownLevelFlowNames = asMetadataSet(metadata, 'levelFlows', 'name');
  for (const segment of segments) {
    if (storyIds.has(segment.storyId)) {
      errors.push(`Duplicate storyId: ${segment.storyId}`);
    }
    storyIds.add(segment.storyId);
    if (!segment.encounterId) {
      errors.push(`${segment.storyId}: encounterId is required`);
    }
    if (!segment.graphAsset) {
      errors.push(`${segment.storyId}: graphAsset is required`);
    }
    for (const map of segment.maps) {
      if (knownSets.knownMaps.size + knownSets.knownRooms.size > 0 && !isKnownMapOrRoom(map, knownSets)) {
        warnings.push(`${segment.storyId}: room/map is not in synced RoomData or map metadata: ${map}`);
        missingRooms.add(map);
      }
    }
    const nodeIds = new Set();
    for (const point of segment.points) {
      if (!point.nodeId) {
        errors.push(`${segment.storyId}: point is missing nodeId`);
      }
      if (nodeIds.has(point.nodeId)) {
        errors.push(`${segment.storyId}: duplicate nodeId ${point.nodeId}`);
      }
      nodeIds.add(point.nodeId);
      if (!point.asset) {
        errors.push(`${segment.storyId}.${point.nodeId}: asset is required`);
      }
      if (point.actions.length === 0) {
        warnings.push(`${segment.storyId}.${point.nodeId}: no actions configured`);
      }
      for (const action of point.actions) {
        if (!ACTION_TYPES.has(action.type)) {
          errors.push(`${segment.storyId}.${point.nodeId}: unknown action ${action.type}`);
        }
        if (action.type === 'RecordProgress' && !action.progressKey) {
          warnings.push(`${segment.storyId}.${point.nodeId}: RecordProgress missing progressKey`);
        }
        if (action.type === 'PlayLevelFlow' && !action.levelFlow) {
          warnings.push(`${segment.storyId}.${point.nodeId}: PlayLevelFlow missing levelFlow asset path`);
        } else if (
          action.type === 'PlayLevelFlow' &&
          action.levelFlow &&
          knownLevelFlows.size > 0 &&
          !knownLevelFlows.has(action.levelFlow) &&
          !knownLevelFlowNames.has(assetName(action.levelFlow))
        ) {
          warnings.push(`${segment.storyId}.${point.nodeId}: LevelFlow is not in synced metadata: ${action.levelFlow}`);
        }
        if (action.type === 'SetQuestObjective' && !action.questTaskTag) {
          warnings.push(`${segment.storyId}.${point.nodeId}: SetQuestObjective missing questTaskTag`);
        }
        if (action.type === 'SetActorEnabled' && !action.targetActorName && !action.targetActorTag) {
          warnings.push(`${segment.storyId}.${point.nodeId}: SetActorEnabled missing targetActorName or targetActorTag`);
        }
        if (action.type === 'TutorialPopup' && !action.tutorialEventId) {
          warnings.push(`${segment.storyId}.${point.nodeId}: TutorialPopup missing tutorialEventId; UE runtime looks up pages through TutorialRegistry`);
        }
        for (const [pageIndex, page] of toArray(action.tutorialPages).entries()) {
          if (!page.body) {
            warnings.push(`${segment.storyId}.${point.nodeId}: TutorialPopup page ${pageIndex + 1} missing body`);
          }
        }
        if (action.featureTag && knownTags.size > 0 && !knownTags.has(action.featureTag)) {
          warnings.push(`${segment.storyId}.${point.nodeId}: featureTag is not in synced gameplay tags: ${action.featureTag}`);
        }
        if (action.questTaskTag && knownTags.size > 0 && !knownTags.has(action.questTaskTag)) {
          warnings.push(`${segment.storyId}.${point.nodeId}: questTaskTag is not in synced gameplay tags: ${action.questTaskTag}`);
        }
      }
      if (!point.placementLevel || !point.placementName) {
        warnings.push(`${segment.storyId}.${point.nodeId}: trigger placement is incomplete`);
      } else if (
        knownSets.knownMaps.size + knownSets.knownRooms.size > 0 &&
        !isVirtualPlacementLevel(point.placementLevel) &&
        !isKnownMapOrRoom(point.placementLevel, knownSets)
      ) {
        warnings.push(`${segment.storyId}.${point.nodeId}: placementLevel is not in synced RoomData or map metadata: ${point.placementLevel}`);
        missingRooms.add(point.placementLevel);
      }
    }
  }
  return { errors, warnings, missingRooms: [...missingRooms].sort((a, b) => a.localeCompare(b)) };
}

function toImportManifest(rootDir, sourceFiles = [], metadata = storyMetadata.readMetadata(rootDir)) {
  const segments = collectSegments(rootDir, sourceFiles);
  const validation = validateSegments(segments, metadata);
  return {
    schemaVersion: SCHEMA_VERSION,
    generatedAt: new Date().toISOString(),
    metadataFile: storyMetadata.metadataFileForRequest(rootDir),
    segments,
    validation
  };
}

function buildWorkloadMarkdown(manifest) {
  const lines = ['# Story Pipeline Workload', '', `Generated: ${manifest.generatedAt}`, ''];
  for (const segment of manifest.segments) {
    lines.push(`## ${segment.storyId}`, '');
    lines.push(`- Arc: ${segment.arc}`);
    lines.push(`- Chapter: ${segment.chapter}`);
    lines.push(`- Timeline: ${segment.timeline}`);
    lines.push(`- Graph: ${segment.graphAsset}`);
    lines.push('');
    lines.push('| Work | Detail | Owner | Status |');
    lines.push('| --- | --- | --- | --- |');
    for (const point of segment.points) {
      if (isVirtualPlacementLevel(point.placementLevel)) {
        lines.push(`| SystemEvent | ${point.placementLevel || '-'} / ${point.placementName || '-'} -> ${point.asset} | Tech Design | todo |`);
      } else {
        lines.push(`| Trigger | ${point.placementLevel || '-'} / ${point.placementName || '-'} -> ${point.asset} | Level | todo |`);
      }
      for (const action of point.actions) {
        if (action.type === 'PlayLevelFlow') {
          lines.push(`| LevelFlow | ${action.levelFlow || `需要为 ${point.asset} 配置 LevelFlow`} | Tech Design | todo |`);
        }
        if (action.type === 'Dialogue') {
          lines.push(`| Dialogue | ${point.asset}: ${action.title || point.displayName} | Narrative/UI | todo |`);
        }
        if (action.type === 'SetQuestObjective') {
          lines.push(`| QuestTask | ${action.questTaskTag || `需要为 ${point.asset} 配置 QuestTaskTag`} | Narrative | todo |`);
        }
        if (action.type === 'SetActorEnabled') {
          const target = action.targetActorName || action.targetActorTag || `需要为 ${point.asset} 配置 TargetActorName/TargetActorTag`;
          lines.push(`| LevelActor | ${target} -> ${action.actorEnabled ? 'Enable/Show' : 'Disable/Hide'} | Level/Tech Design | todo |`);
        }
      }
    }
    for (const item of segment.workItems) {
      lines.push(`| ${item.type || 'Task'} | ${item.title || item.description || JSON.stringify(item)} | ${item.owner || '-'} | ${item.status || 'todo'} |`);
    }
    lines.push('');
  }
  if (manifest.validation.missingRooms?.length) {
    lines.push('## Missing RoomData / Campaign Tasks', '');
    lines.push('| Work | Detail | Owner | Status |');
    lines.push('| --- | --- | --- | --- |');
    for (const roomName of manifest.validation.missingRooms) {
      lines.push(`| RoomData | Create or sync RoomData for \`${roomName}\`, set RoomName/DisplayName/RoomTags, bind map resolver if needed, add to DA_Campaign_MainRun or DA_Campaign_Tutorial RoomPool, then configure Portal/reward rules. | Level Design | todo |`);
    }
    lines.push('');
  }
  if (manifest.validation.errors.length || manifest.validation.warnings.length) {
    lines.push('## Validation', '');
    for (const error of manifest.validation.errors) {
      lines.push(`- ERROR: ${error}`);
    }
    for (const warning of manifest.validation.warnings) {
      lines.push(`- WARN: ${warning}`);
    }
    lines.push('');
  }
  return lines.join('\n');
}

function buildValidationMarkdown(manifest) {
  const lines = ['# Story Pipeline Validation', '', `Generated: ${manifest.generatedAt}`, ''];
  if (manifest.metadataFile) {
    lines.push(`Metadata: ${manifest.metadataFile}`, '');
  }
  if (!manifest.validation.errors.length && !manifest.validation.warnings.length) {
    lines.push('No validation issues.', '');
    return lines.join('\n');
  }
  if (manifest.validation.errors.length) {
    lines.push('## Errors', '');
    for (const error of manifest.validation.errors) {
      lines.push(`- ${error}`);
    }
    lines.push('');
  }
  if (manifest.validation.warnings.length) {
    lines.push('## Warnings', '');
    for (const warning of manifest.validation.warnings) {
      lines.push(`- ${warning}`);
    }
    lines.push('');
  }
  return lines.join('\n');
}

function exportPipeline(rootDir, sourceFiles = []) {
  const manifest = toImportManifest(rootDir, sourceFiles);
  const manifestPath = path.join(rootDir, 'Docs', 'StoryPipeline', 'StoryImportManifest.json');
  const workloadPath = path.join(rootDir, 'Docs', 'StoryPipeline', 'StoryWorkload.md');
  const validationPath = path.join(rootDir, 'Docs', 'StoryPipeline', 'ValidationReport.md');
  writeJson(manifestPath, manifest);
  writeText(workloadPath, buildWorkloadMarkdown(manifest));
  writeText(validationPath, buildValidationMarkdown(manifest));
  return {
    manifest,
    manifestFile: path.relative(rootDir, manifestPath).replace(/\\/g, '/'),
    workloadFile: path.relative(rootDir, workloadPath).replace(/\\/g, '/'),
    validationFile: path.relative(rootDir, validationPath).replace(/\\/g, '/')
  };
}

function createCodexRequest(rootDir, request) {
  const mode = cleanString(request.mode, '整理剧情字段');
  const sourceFiles = toArray(request.sourceFiles).map(String);
  const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
  const id = `${timestamp}-${mode.replace(/[^A-Za-z0-9]+/g, '_')}`;
  const requestData = {
    id,
    mode,
    sourceFiles,
    metadataFile: storyMetadata.metadataFileForRequest(rootDir),
    rules: cleanString(request.rules, [
      '读取剧情源文件并保持 schemaVersion/extra 字段。',
      '检查 EncounterId + NodeId 唯一性。',
      '补全缺失工作量：Trigger、LevelFlow、Dialogue、QuestTask、KeyItem。',
      '输出 StoryImportManifest.json、StoryWorkload.md 和校验报告。',
      '不要修改 UE 地图。'
    ].join('\n')),
    outputs: toArray(request.outputs).length ? request.outputs : [
      'Docs/StoryPipeline/StoryImportManifest.json',
      'Docs/StoryPipeline/StoryWorkload.md',
      'Docs/StoryPipeline/ValidationReport.md'
    ],
    createdAt: new Date().toISOString()
  };
  const jsonPath = path.join(rootDir, 'Docs', 'StoryPipeline', 'Requests', `${id}.json`);
  const mdPath = path.join(rootDir, 'Docs', 'StoryPipeline', 'Requests', `${id}.md`);
  writeJson(jsonPath, requestData);
  writeText(mdPath, [
    `# Codex Story Pipeline Request: ${mode}`,
    '',
    `Created: ${requestData.createdAt}`,
    '',
    '## Source Files',
    '',
    ...sourceFiles.map((file) => `- ${file}`),
    '',
    '## Metadata',
    '',
    requestData.metadataFile || '- No synced metadata file yet. Run sync metadata if dropdown validation is needed.',
    '',
    '## Rules',
    '',
    requestData.rules,
    '',
    '## Outputs',
    '',
    ...requestData.outputs.map((file) => `- ${file}`),
    ''
  ].join('\n'));
  return {
    request: requestData,
    requestFile: path.relative(rootDir, jsonPath).replace(/\\/g, '/'),
    markdownFile: path.relative(rootDir, mdPath).replace(/\\/g, '/')
  };
}

module.exports = {
  ACTION_TYPES,
  NODE_KINDS,
  FIRE_POLICIES,
  CONDITION_KINDS,
  defaultSegment,
  defaultPoint,
  normalizeSegment,
  listSegments,
  readSegment,
  writeSegment,
  validateSegments,
  toImportManifest,
  exportPipeline,
  createCodexRequest
};
