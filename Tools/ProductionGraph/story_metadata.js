const fs = require('fs');
const path = require('path');

const METADATA_RELATIVE_PATH = path.join('Docs', 'StoryPipeline', 'Metadata', 'project_assets.json');
const UE_METADATA_RELATIVE_PATH = path.join('Docs', 'StoryPipeline', 'Metadata', 'ue_project_assets.json');

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function readJsonAnyEncoding(filePath) {
  const bytes = fs.readFileSync(filePath);
  if (bytes.length >= 2 && bytes[0] === 0xff && bytes[1] === 0xfe) {
    return JSON.parse(bytes.slice(2).toString('utf16le'));
  }
  if (bytes.length >= 2 && bytes[0] === 0xfe && bytes[1] === 0xff) {
    return JSON.parse(Buffer.from(bytes.slice(2)).swap16().toString('utf16le'));
  }
  return JSON.parse(bytes.toString('utf8').replace(/^\uFEFF/, ''));
}

function writeJson(filePath, value) {
  ensureDir(path.dirname(filePath));
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`, 'utf8');
}

function metadataPath(rootDir) {
  return path.join(rootDir, METADATA_RELATIVE_PATH);
}

function ueMetadataPath(rootDir) {
  return path.join(rootDir, UE_METADATA_RELATIVE_PATH);
}

function toPosix(value) {
  return String(value || '').replace(/\\/g, '/');
}

function toGamePath(rootDir, filePath) {
  const contentDir = path.join(rootDir, 'Content');
  const relative = path.relative(contentDir, filePath);
  const withoutExt = relative.replace(/\.[^.]+$/, '');
  return `/Game/${toPosix(withoutExt)}`;
}

function assetNameFromPath(gamePath) {
  return path.posix.basename(toPosix(gamePath));
}

function assetEntry(rootDir, filePath) {
  const gamePath = toGamePath(rootDir, filePath);
  return {
    name: assetNameFromPath(gamePath),
    path: gamePath,
    directory: path.posix.dirname(toPosix(gamePath))
  };
}

function walkFiles(dirPath, predicate, out = []) {
  if (!fs.existsSync(dirPath)) {
    return out;
  }
  for (const entry of fs.readdirSync(dirPath, { withFileTypes: true })) {
    const fullPath = path.join(dirPath, entry.name);
    if (entry.isDirectory()) {
      walkFiles(fullPath, predicate, out);
    } else if (entry.isFile() && predicate(fullPath, entry.name)) {
      out.push(fullPath);
    }
  }
  return out;
}

function sortEntries(entries) {
  return entries.sort((a, b) => a.path.localeCompare(b.path));
}

function uniqueStrings(values) {
  return [...new Set(values.filter(Boolean).map(String))].sort((a, b) => a.localeCompare(b));
}

function scanGameplayTags(rootDir) {
  const configDir = path.join(rootDir, 'Config');
  const tags = [];
  const iniFiles = walkFiles(configDir, (filePath, name) => name.endsWith('.ini'));
  const pattern = /GameplayTagList=\(Tag="([^"]+)"/g;
  for (const filePath of iniFiles) {
    const text = fs.readFileSync(filePath, 'utf8');
    let match = pattern.exec(text);
    while (match) {
      tags.push(match[1]);
      match = pattern.exec(text);
    }
  }
  return uniqueStrings(tags);
}

function scanStorySourceTerms(rootDir) {
  const sourceRoot = path.join(rootDir, 'Docs', 'StorySource');
  const arcs = [];
  const chapters = [];
  const timelines = [];
  const progressKeys = [];
  const storyFiles = walkFiles(sourceRoot, (filePath, name) => name.endsWith('.story.json'));
  for (const filePath of storyFiles) {
    try {
      const segment = readJson(filePath);
      arcs.push(segment.arc);
      chapters.push(segment.chapter);
      timelines.push(segment.timeline);
      for (const point of Array.isArray(segment.points) ? segment.points : []) {
        for (const action of Array.isArray(point.actions) ? point.actions : []) {
          progressKeys.push(action.progressKey, action.ProgressKey);
        }
        if (point.condition) {
          progressKeys.push(point.condition.progressKey, point.condition.ProgressKey);
        }
      }
      for (const state of Array.isArray(segment.states) ? segment.states : []) {
        progressKeys.push(state);
      }
    } catch {
      // A malformed draft should not block metadata sync.
    }
  }
  return {
    arcs: uniqueStrings(['main', 'side', 'tutorial', 'system', ...arcs]),
    chapters: uniqueStrings(chapters),
    timelines: uniqueStrings(['present', 'past', 'memory', 'dream', ...timelines]),
    progressKeys: uniqueStrings(progressKeys)
  };
}

function readUeMetadata(rootDir) {
  const filePath = ueMetadataPath(rootDir);
  if (!fs.existsSync(filePath)) {
    return null;
  }
  try {
    return readJsonAnyEncoding(filePath);
  } catch {
    return null;
  }
}

function normalizeUeRoom(entry) {
  return {
    name: entry.name || assetNameFromPath(entry.path),
    path: entry.path,
    directory: path.posix.dirname(toPosix(entry.path || '')),
    roomName: entry.roomName || entry.name || '',
    displayName: entry.displayName || entry.roomName || entry.name || '',
    tags: Array.isArray(entry.tags) ? entry.tags : [],
    isHubRoom: entry.isHubRoom === true,
    enemyCount: Number(entry.enemyCount || 0),
    buffCount: Number(entry.buffCount || 0),
    lootCount: Number(entry.lootCount || 0),
    portalCount: Number(entry.portalCount || 0),
    forceSinglePortal: entry.forceSinglePortal === true,
    forcedPortalIndex: Number(entry.forcedPortalIndex || 0),
    useFixedRewardOptions: entry.useFixedRewardOptions === true,
    fixedRewardCount: Number(entry.fixedRewardCount || 0)
  };
}

function isLikelyRoomDataAsset(entry) {
  const name = entry.name || assetNameFromPath(entry.path);
  const pathValue = entry.path || '';
  if (/^DA_Rune/i.test(name) || /AbilitySet|StateConflict|Tutorial|Altar|PortalData|LevelSample/i.test(name)) {
    return false;
  }
  if (!/\/Map(\/|$)/i.test(pathValue) && !/\/LootTest\//i.test(pathValue)) {
    return false;
  }
  return /^(DA_Room_|DA_HubRoom_|DA_CL_|DA_PrayRoom$|DA_EmbalmingChamber$|DA_LootOnly$)/i.test(name);
}

function mergeRoomData(fastRoomData, ueMetadata) {
  const ueRooms = (Array.isArray(ueMetadata?.roomData) ? ueMetadata.roomData : [])
    .filter((entry) => entry.path)
    .map(normalizeUeRoom);
  if (ueRooms.length > 0) {
    return sortEntries(ueRooms);
  }
  return sortEntries(fastRoomData.filter(isLikelyRoomDataAsset));
}

function mergeCampaigns(fastCampaigns, ueMetadata) {
  const byPath = new Map(fastCampaigns.map((entry) => [entry.path, entry]));
  for (const entry of Array.isArray(ueMetadata?.campaigns) ? ueMetadata.campaigns : []) {
    if (entry.path) {
      byPath.set(entry.path, {
        ...byPath.get(entry.path),
        ...entry,
        name: entry.name || assetNameFromPath(entry.path),
        directory: path.posix.dirname(toPosix(entry.path || ''))
      });
    }
  }
  return sortEntries([...byPath.values()]);
}

function scanProjectAssets(rootDir) {
  const contentDir = path.join(rootDir, 'Content');
  const files = walkFiles(contentDir, (filePath, name) => name.endsWith('.umap') || name.endsWith('.uasset'));
  const maps = [];
  const roomData = [];
  const campaigns = [];
  const runes = [];
  const tutorials = [];
  const storyAssets = [];
  const levelFlows = [];

  for (const filePath of files) {
    const ext = path.extname(filePath).toLowerCase();
    const entry = assetEntry(rootDir, filePath);
    const name = entry.name;
    const lowerPath = entry.path.toLowerCase();
    if (ext === '.umap') {
      maps.push(entry);
      continue;
    }
    if (ext !== '.uasset') {
      continue;
    }
    if (/^DA_Campaign/i.test(name)) {
      campaigns.push(entry);
    }
    if (isLikelyRoomDataAsset(entry)) {
      roomData.push(entry);
    }
    if (/^DA_Rune/i.test(name)) {
      runes.push(entry);
    }
    if (/^DA_Tutorial/i.test(name)) {
      tutorials.push(entry);
    }
    if (/Story|Encounter|\/EG_|\/EP_|\/Story\//i.test(entry.path)) {
      storyAssets.push(entry);
    }
    if (/LevelFlow|\/Flow|Flow_/i.test(entry.path)) {
      levelFlows.push(entry);
    }
  }

  const storyTerms = scanStorySourceTerms(rootDir);
  const tags = scanGameplayTags(rootDir);
  const ueMetadata = readUeMetadata(rootDir);
  const mergedRoomData = mergeRoomData(roomData, ueMetadata);
  const mergedCampaigns = mergeCampaigns(campaigns, ueMetadata);
  const now = new Date().toISOString();
  return {
    schemaVersion: 1,
    generatedAt: now,
    source: 'fast-filesystem-scan',
    ueMetadataFile: ueMetadata ? UE_METADATA_RELATIVE_PATH.replace(/\\/g, '/') : '',
    ueMetadataGeneratedAt: ueMetadata?.generatedAt || '',
    maps: sortEntries(maps),
    roomData: mergedRoomData,
    rooms: mergedRoomData,
    campaigns: mergedCampaigns,
    runes: sortEntries(runes),
    tutorials: sortEntries(tutorials),
    storyAssets: sortEntries(storyAssets),
    levelFlows: sortEntries(levelFlows),
    tags,
    storyTerms,
    summary: {
      maps: maps.length,
      roomData: mergedRoomData.length,
      rooms: mergedRoomData.length,
      campaigns: mergedCampaigns.length,
      runes: runes.length,
      tutorials: tutorials.length,
      storyAssets: storyAssets.length,
      levelFlows: levelFlows.length,
      tags: tags.length
    }
  };
}

function writeMetadata(rootDir, metadata) {
  const filePath = metadataPath(rootDir);
  writeJson(filePath, metadata);
  return {
    metadata,
    metadataFile: path.relative(rootDir, filePath).replace(/\\/g, '/')
  };
}

function syncMetadata(rootDir) {
  return writeMetadata(rootDir, scanProjectAssets(rootDir));
}

function readMetadata(rootDir) {
  const filePath = metadataPath(rootDir);
  if (!fs.existsSync(filePath)) {
    return null;
  }
  return readJson(filePath);
}

function metadataFileForRequest(rootDir) {
  const filePath = metadataPath(rootDir);
  return fs.existsSync(filePath) ? path.relative(rootDir, filePath).replace(/\\/g, '/') : '';
}

module.exports = {
  METADATA_RELATIVE_PATH,
  UE_METADATA_RELATIVE_PATH,
  metadataPath,
  ueMetadataPath,
  scanProjectAssets,
  syncMetadata,
  readMetadata,
  metadataFileForRequest
};
