import { createHash } from 'node:crypto';
import { spawnSync } from 'node:child_process';
import { lstatSync, readFileSync, realpathSync, writeFileSync } from 'node:fs';
import { basename, dirname, isAbsolute, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = dirname(fileURLToPath(import.meta.url));
const POLICY_PATH = join(HERE, 'policy.json');
const CODE_EXTENSIONS = new Set([
  '.h', '.hpp', '.hxx', '.c', '.cc', '.cpp', '.cxx', '.inl', '.inc',
  '.cs', '.py', '.js', '.mjs', '.ts', '.tsx', '.usf', '.ush',
  '.json', '.ini', '.xml', '.yml', '.yaml', '.txt', '.md',
  '.ps1', '.bat', '.cmd', '.sh',
]);
const CONFIG_EXTENSIONS = new Set(['.ini', '.json', '.xml', '.yml', '.yaml', '.txt']);
const PROTECTED_SEGMENTS = new Set([
  'engine', 'binaries', 'intermediate', 'saved', 'deriveddatacache',
  '.git', '.p4', 'sourceart', 'resources',
  'obj', 'bin', '.vs', '.idea', 'node_modules', 'dist', 'out',
  '__pycache__', '.pytest_cache', '.mypy_cache', '.cache',
]);
const PROTECTED_EXTENSIONS = new Set([
  '.uasset', '.umap', '.uexp', '.ubulk', '.uptnl', '.dll', '.exe', '.lib',
  '.pdb', '.zip', '.7z', '.pak', '.a', '.dylib', '.so', '.cache',
  '.png', '.jpg', '.jpeg', '.tga', '.wav', '.mp3', '.fbx', '.blend',
]);
const UTF8 = new TextDecoder('utf-8', { fatal: true });
const GBK = new TextDecoder('gbk', { fatal: true });
const UTF16LE = new TextDecoder('utf-16le', { fatal: true });
const UTF16BE = new TextDecoder('utf-16be', { fatal: true });

function digest(data, algorithm = 'sha256') {
  return createHash(algorithm).update(data).digest('hex');
}

export function textEncoding(bytes, allowUtf16 = false) {
  if (allowUtf16 && bytes.length >= 2 && bytes.length % 2 === 0) {
    if (bytes[0] === 0xff && bytes[1] === 0xfe) {
      try { UTF16LE.decode(bytes); return 'utf16le'; } catch { return null; }
    }
    if (bytes[0] === 0xfe && bytes[1] === 0xff) {
      try { UTF16BE.decode(bytes); return 'utf16be'; } catch { return null; }
    }
  }
  if (bytes.includes(0)) return null;
  try { UTF8.decode(bytes); return 'utf8'; }
  catch {
    try { GBK.decode(bytes); return 'gbk'; }
    catch { return null; }
  }
}

function extension(path) {
  const leaf = path.slice(path.lastIndexOf('/') + 1);
  const dot = leaf.lastIndexOf('.');
  return dot < 0 ? '' : leaf.slice(dot).toLowerCase();
}

function validPath(path) {
  return typeof path === 'string' && path.length > 0 && !path.includes('\\') &&
    !path.includes('\0') && !path.startsWith('/') && !path.endsWith('/') &&
    path.split('/').every((part) => part && part !== '.' && part !== '..' &&
      !/[. ]$/.test(part) && !/[:\x00-\x1f]/.test(part) &&
      !/^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])(?:\..*)?$/i.test(part));
}

export function classifyPath(path) {
  if (!validPath(path)) return { eligible: false, reason: 'invalid-path' };
  const parts = path.split('/');
  const lower = parts.map((part) => part.toLowerCase());
  const ext = extension(path);
  // The reviewed Docs policy is an entire-root mirror, including PNG/log files.
  // Keep this positive root before extension/segment exclusions, which govern code trees.
  if (lower[0] === 'docs' && parts.length >= 2) return { eligible: true, kind: 'docs' };
  if (lower.some((part) => PROTECTED_SEGMENTS.has(part))) {
    return { eligible: false, reason: 'protected-segment' };
  }
  if (PROTECTED_EXTENSIONS.has(ext)) return { eligible: false, reason: 'binary-or-asset-extension' };
  if (lower[0] === 'build' || lower[0] === 'buildscripts') {
    return { eligible: false, reason: 'distribution-build-protected' };
  }
  if (lower[0] === 'tools') {
    return { eligible: false, reason: 'not-explicitly-approved-text' };
  }
  if (path === 'DevKit.uproject') return { eligible: true, kind: 'project-descriptor' };
  if (lower[0] === 'source' && parts.length >= 2 && CODE_EXTENSIONS.has(ext)) {
    return { eligible: true, kind: 'project-source' };
  }
  if (lower[0] === 'config' && parts.length >= 2 && CONFIG_EXTENSIONS.has(ext)) {
    return { eligible: true, kind: 'project-config' };
  }
  if (lower[0] === 'content' && lower[1] === 'script' && parts.length >= 3 && ext === '.lua') {
    return { eligible: true, kind: 'lua-source' };
  }
  if (lower[0] === 'plugins' && parts.length >= 3) {
    if (lower.includes('content')) return { eligible: false, reason: 'plugin-content-protected' };
    const sourceIndex = lower.indexOf('source', 2);
    const configIndex = lower.indexOf('config', 2);
    if (sourceIndex >= 2 && sourceIndex < parts.length - 1 && CODE_EXTENSIONS.has(ext)) {
      return { eligible: true, kind: 'plugin-source' };
    }
    if (configIndex >= 2 && configIndex < parts.length - 1 && CONFIG_EXTENSIONS.has(ext)) {
      return { eligible: true, kind: 'plugin-config' };
    }
    if (ext === '.uplugin' && !lower.includes('content')) {
      return { eligible: true, kind: 'plugin-descriptor' };
    }
  }
  return { eligible: false, reason: 'outside-positive-allowlist' };
}

function limitForPath(path, maxTextBytes, maxDocsBytes) {
  return classifyPath(path).kind === 'docs' ? maxDocsBytes : maxTextBytes;
}

function run(binary, args, options = {}) {
  const result = spawnSync(binary, args, {
    cwd: options.cwd,
    input: options.input,
    encoding: null,
    maxBuffer: 256 * 1024 * 1024,
    windowsHide: true,
  });
  if (result.error) throw result.error;
  if (result.status !== 0 && !options.allowMissing) {
    throw new Error(`${binary} ${args.join(' ')} failed (${result.status}): ${result.stderr.toString('utf8').trim()}`);
  }
  return result;
}

function parseGitTree(raw) {
  const entries = new Map();
  for (const record of raw.subarray(0, raw.length - 1).toString('binary').split('\0')) {
    if (!record) continue;
    const tab = record.indexOf('\t');
    if (tab < 0) throw new Error('Malformed git ls-tree record');
    const meta = record.slice(0, tab).trim().split(/\s+/);
    if (meta.length !== 4) throw new Error('Malformed git ls-tree metadata');
    const path = UTF8.decode(Buffer.from(record.slice(tab + 1), 'binary'));
    if (entries.has(path)) throw new Error(`Duplicate Git path: ${path}`);
    entries.set(path, { path, mode: meta[0], type: meta[1], oid: meta[2], size: Number(meta[3]) });
  }
  return entries;
}

function parseP4Tags(raw) {
  const records = [];
  let current = {};
  let decoded;
  try { decoded = UTF8.decode(raw); } catch { decoded = GBK.decode(raw); }
  for (const line of decoded.split(/\r?\n/)) {
    if (!line) {
      if (Object.keys(current).length) records.push(current);
      current = {};
      continue;
    }
    const match = /^\.\.\. ([^ ]+) ?(.*)$/.exec(line);
    if (!match) continue;
    if (Object.hasOwn(current, match[1])) throw new Error(`Duplicate P4 tag ${match[1]}`);
    current[match[1]] = match[2];
  }
  if (Object.keys(current).length) records.push(current);
  return records;
}

function p4Read(args, allowNoFiles = false) {
  const result = run('p4', ['-ztag', ...args], { allowMissing: allowNoFiles });
  if (result.status !== 0) {
    const message = `${result.stdout.toString('utf8')}\n${result.stderr.toString('utf8')}`;
    if (allowNoFiles && /no such file\(s\)|file\(s\) not opened/i.test(message)) return [];
    throw new Error(`p4 ${args.join(' ')} failed (${result.status}): ${message.trim()}`);
  }
  const records = parseP4Tags(result.stdout);
  const errors = records.filter((item) => item.code === 'error');
  if (errors.length) {
    if (allowNoFiles && errors.length === records.length &&
        errors.every((item) => /no such file\(s\)|file\(s\) not opened/i.test(item.data ?? ''))) return [];
    throw new Error(`P4 server error: ${errors.map((item) => item.data).join('; ').slice(0, 500)}`);
  }
  return records;
}

function hydrateGitDigests(gitRepo, entries, maxTextBytes, maxDocsBytes) {
  const selected = [...entries.values()].filter((entry) => classifyPath(entry.path).eligible &&
    entry.type === 'blob' && entry.mode === '100644' && Number.isSafeInteger(entry.size) &&
    entry.size <= limitForPath(entry.path, maxTextBytes, maxDocsBytes));
  if (!selected.length) return;
  const input = Buffer.from(selected.map((entry) => entry.oid).join('\n') + '\n', 'ascii');
  const result = run('git', ['-C', gitRepo, 'cat-file', '--batch'], { input });
  const output = result.stdout;
  let offset = 0;
  for (const entry of selected) {
    const newline = output.indexOf(10, offset);
    if (newline < 0) throw new Error('Truncated git cat-file header');
    const header = output.subarray(offset, newline).toString('ascii');
    const match = /^([0-9a-f]{40,64}) blob (\d+)$/.exec(header);
    if (!match || match[1] !== entry.oid || Number(match[2]) !== entry.size) {
      throw new Error(`Unexpected git cat-file object for ${entry.path}`);
    }
    const dataStart = newline + 1;
    const dataEnd = dataStart + entry.size;
    if (dataEnd >= output.length || output[dataEnd] !== 10) throw new Error(`Truncated git blob ${entry.path}`);
    const bytes = output.subarray(dataStart, dataEnd);
    entry.md5 = digest(bytes, 'md5');
    entry.sha256 = digest(bytes);
    entry.textEncoding = textEncoding(bytes, classifyPath(entry.path).kind === 'docs');
    entry.textValid = !!entry.textEncoding;
    entry.contentKind = entry.textValid ? 'text' : 'binary';
    if (entry.contentKind === 'binary') entry.textEncoding = 'binary';
    offset = dataEnd + 1;
  }
  if (offset !== output.length) throw new Error('Unexpected trailing git cat-file data');
}

function readGit(gitRepo, gitSha, maxTextBytes, maxDocsBytes) {
  const commit = run('git', ['-C', gitRepo, 'rev-parse', `${gitSha}^{commit}`]).stdout.toString('ascii').trim();
  if (commit !== gitSha) throw new Error(`Git SHA mismatch: expected ${gitSha}, found ${commit}`);
  const tree = run('git', ['-C', gitRepo, 'ls-tree', '-rlz', gitSha, '--']).stdout;
  if (!tree.length || tree[tree.length - 1] !== 0) throw new Error('Empty or malformed git tree');
  const entries = parseGitTree(tree);
  hydrateGitDigests(gitRepo, entries, maxTextBytes, maxDocsBytes);
  return entries;
}

function p4Patterns(depot, cl) {
  return [
    `${depot}/Source/...@${cl}`,
    `${depot}/Config/...@${cl}`,
    `${depot}/Plugins/.../Source/...@${cl}`,
    `${depot}/Plugins/.../Config/...@${cl}`,
    `${depot}/Plugins/.../*.uplugin@${cl}`,
    `${depot}/Content/Script/...@${cl}`,
    `${depot}/DevKit.uproject@${cl}`,
    `${depot}/Docs/...@${cl}`,
  ];
}

function readP4(depot, cl) {
  const entries = new Map();
  const fields = 'depotFile,headRev,headAction,digest,fileSize,headType';
  for (const pattern of p4Patterns(depot, cl)) {
    for (const item of p4Read(['fstat', '-Ol', '-T', fields, pattern], true)) {
      if (!item.depotFile) continue;
      if (!item.depotFile.startsWith(`${depot}/`)) throw new Error(`Unexpected depot path: ${item.depotFile}`);
      const path = item.depotFile.slice(depot.length + 1);
      if (item.headAction === 'delete' || item.headAction === 'move/delete') continue;
      const entry = { path, depotFile: item.depotFile, rev: Number(item.headRev),
        action: item.headAction, type: item.headType, size: Number(item.fileSize),
        md5: item.digest?.toLowerCase() };
      const prior = entries.get(path);
      if (prior && JSON.stringify(prior) !== JSON.stringify(entry)) throw new Error(`Conflicting P4 metadata: ${path}`);
      entries.set(path, entry);
    }
  }
  return entries;
}

function readOpened(depot) {
  const opened = new Map();
  for (const item of p4Read(['opened', '-a', `${depot}/...`], true)) {
    if (!item.depotFile?.startsWith(`${depot}/`)) continue;
    const path = item.depotFile.slice(depot.length + 1);
    const list = opened.get(path) ?? [];
    list.push({ clientFile: item.clientFile, action: item.action, change: item.change,
      user: item.user });
    opened.set(path, list);
  }
  return opened;
}

function inspectOpenedWorkingTree(opened, gitEntries, depot, maxTextBytes, maxDocsBytes) {
  const info = p4Read(['info']);
  const client = info[0]?.clientName;
  const rootMapping = p4Read(['where', `${depot}/DevKit.uproject`]);
  const rootPath = rootMapping.find((item) => item.path)?.path;
  if (!client || !rootPath) throw new Error('Could not verify current P4 client/root for opened audit');
  const workspaceRoot = realpathSync(dirname(rootPath)).toLowerCase();
  for (const [path, records] of opened) {
    if (!classifyPath(path).eligible) continue;
    for (const record of records) {
      record.workingDigestMatchesGit = null;
      if (!record.clientFile?.startsWith(`//${client}/`)) {
        record.workingInspection = 'other-client-unavailable';
        continue;
      }
      const mapping = p4Read(['where', `${depot}/${path}`]);
      const localPath = mapping.find((item) => item.depotFile === `${depot}/${path}`)?.path;
      if (!localPath) { record.workingInspection = 'unmapped'; continue; }
      try {
        if (!lstatSync(localPath).isFile()) { record.workingInspection = 'not-regular-file'; continue; }
        const realPath = realpathSync(localPath).toLowerCase();
        if (!realPath.startsWith(`${workspaceRoot}\\`)) {
          record.workingInspection = 'outside-verified-workspace-root';
          continue;
        }
        const bytes = readFileSync(localPath);
        if (bytes.length > limitForPath(path, maxTextBytes, maxDocsBytes)) {
          record.workingInspection = 'too-large'; continue;
        }
        record.workingInspection = 'hashed-current-client-file';
        record.workingSha256 = digest(bytes);
        record.workingMd5 = digest(bytes, 'md5');
        const git = gitEntries.get(path);
        if (git?.md5) record.workingDigestMatchesGit = record.workingMd5 === git.md5;
      } catch (error) {
        if (error.code === 'ENOENT') record.workingInspection = 'missing-local-file';
        else throw error;
      }
    }
  }
  return opened;
}

function safeP4(entry, kind, limit) {
  const allowedType = kind === 'docs' ? /^(?:text|unicode|utf8|utf16|binary)$/i :
    /^(?:text|unicode|utf8|utf16)(?:\+[A-Za-z0-9]+)?$/i;
  return !!entry && allowedType.test(entry.type ?? '') &&
    /^[0-9a-f]{32}$/.test(entry.md5 ?? '') && Number.isSafeInteger(entry.rev) && entry.rev > 0 &&
    Number.isSafeInteger(entry.size) && entry.size >= 0 && entry.size <= limit;
}

function caseCollisions(paths) {
  const lower = new Map();
  const collisions = [];
  for (const path of paths) {
    const key = path.toLowerCase();
    const prior = lower.get(key);
    if (prior && prior !== path) collisions.push([prior, path]);
    else lower.set(key, path);
  }
  return collisions;
}

export function buildPlan({ gitEntries, p4Entries, opened = new Map(), gitSha, p4Cl,
  depot, policySha256, maxTextBytes, maxDocsBytes = 16777216 }) {
  const allPaths = [...new Set([...gitEntries.keys(), ...p4Entries.keys()])].sort();
  const operations = [];
  const blocked = [];
  const openedEligible = [];
  const openedOnly = [];
  const excludedGitCounts = {};
  const excludedP4Counts = {};
  let unchangedCount = 0;
  for (const path of gitEntries.keys()) {
    const category = classifyPath(path);
    if (!category.eligible) excludedGitCounts[category.reason] = (excludedGitCounts[category.reason] ?? 0) + 1;
  }
  for (const path of p4Entries.keys()) {
    const category = classifyPath(path);
    if (!category.eligible) excludedP4Counts[category.reason] = (excludedP4Counts[category.reason] ?? 0) + 1;
  }
  const eligiblePaths = allPaths.filter((path) => classifyPath(path).eligible);
  for (const path of allPaths) {
    if (/^docs(?:\/|$)/i.test(path) && !classifyPath(path).eligible) {
      blocked.push({ path, reason: 'docs-unsafe-path', detail: classifyPath(path).reason });
    }
  }
  for (const [path, records] of opened) {
    if (classifyPath(path).eligible && !gitEntries.has(path) && !p4Entries.has(path)) {
      openedOnly.push({ path, kind: classifyPath(path).kind,
        reason: 'opened-path-absent-from-both-fixed-snapshots', opened: records });
    }
  }
  openedOnly.sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
  const collisions = caseCollisions(eligiblePaths);
  if (collisions.length) blocked.push(...collisions.map((pair) => ({ reason: 'case-collision', paths: pair })));
  for (const path of eligiblePaths) {
    const git = gitEntries.get(path);
    const p4 = p4Entries.get(path);
    const openedForPath = opened.get(path) ?? [];
    const kind = classifyPath(path).kind;
    const limit = limitForPath(path, maxTextBytes, maxDocsBytes);
    if (git && (git.type !== 'blob' || git.mode !== '100644' || !Number.isSafeInteger(git.size) ||
        git.size < 0 || git.size > limit || (kind !== 'docs' && !git.textValid) ||
        !git.md5 || !git.sha256 || (kind === 'docs' && !['text', 'binary'].includes(git.contentKind)))) {
      blocked.push({ path, reason: 'git-not-regular-verifiable-content-or-too-large', gitMode: git.mode,
        gitSize: git.size });
      if (openedForPath.length) openedEligible.push({ path, operation: 'blocked', opened: openedForPath });
      continue;
    }
    if (p4 && !safeP4(p4, kind, limit)) {
      blocked.push({ path, reason: 'p4-not-verifiable-content-or-too-large', p4Type: p4.type,
        p4DigestMd5: p4.md5 ?? null });
      if (openedForPath.length) openedEligible.push({ path, operation: 'blocked', opened: openedForPath });
      continue;
    }
    const operation = !p4 ? 'add' : !git ? 'delete' : git.md5 === p4.md5 ? 'keep' : 'edit';
    if (operation === 'delete' && ['project-descriptor', 'plugin-descriptor', 'project-config', 'plugin-config'].includes(kind)) {
      blocked.push({ path, reason: 'descriptor-or-config-delete-requires-explicit-review', kind });
      if (openedForPath.length) openedEligible.push({ path, operation: 'blocked', opened: openedForPath });
      continue;
    }
    if (openedForPath.length) openedEligible.push({ path, operation, opened: openedForPath });
    if (operation === 'keep') { unchangedCount++; continue; }
    const entry = { operation, path, kind,
      git: git ? { blobOid: git.oid, sha256: git.sha256, md5: git.md5, size: git.size,
        textEncoding: git.textEncoding ?? 'unknown', contentKind: git.contentKind ?? 'text' } : null,
      p4: p4 ? { depotFile: p4.depotFile, rev: p4.rev, md5: p4.md5, size: p4.size,
        type: p4.type } : null,
      opened: openedForPath,
      reviewFlags: kind === 'docs' && operation === 'edit' ?
        [git.contentKind === 'binary' || /^(?:binary)(?:\+|$)/i.test(p4.type) ?
          'binary-conflict-hold' : 'three-way-text-merge-required'] :
        kind === 'docs' && operation === 'delete' ? ['docs-strict-delete-review-required'] :
        operation === 'delete' ? ['native-or-editor-reference-audit-required'] :
        ['project-config', 'plugin-config', 'project-descriptor', 'plugin-descriptor'].includes(kind) ?
          ['asset-and-distribution-contract-review-required'] : [] };
    operations.push(entry);
  }
  const counts = { add: 0, edit: 0, delete: 0, keep: unchangedCount, blocked: blocked.length };
  for (const entry of operations) counts[entry.operation]++;
  const openedByChange = {};
  for (const item of openedEligible) {
    for (const record of item.opened) {
      const change = record.change ?? 'unknown';
      openedByChange[change] ??= { add: 0, edit: 0, delete: 0, keep: 0, blocked: 0, openedOnly: 0 };
      openedByChange[change][item.operation]++;
    }
  }
  for (const item of openedOnly) {
    for (const record of item.opened) {
      const change = record.change ?? 'unknown';
      openedByChange[change] ??= { add: 0, edit: 0, delete: 0, keep: 0, blocked: 0, openedOnly: 0 };
      openedByChange[change].openedOnly++;
    }
  }
  const plan = { schemaVersion: 1, dryRunOnly: true, deployable: false,
    source: { gitSha, p4Cl, depot, policySha256 }, counts, operations, blocked,
    openedEligible, openedOnly, openedByChange, excludedGitCounts, excludedP4Counts,
    warnings: [
      'No filesystem, P4, Git branch, or PCB changes are performed by this planner.',
      'GitHub HEAD, P4 opened ownership, P4 resolve state, import pin, compile, Blueprint load, and PCB gates require independent review.',
      'Only positive allowlist paths are scanned from P4; Docs are included while Content assets/Engine/Build are not enumerated.',
      'Docs shared edits are not automatically resolved: text needs a reviewed three-way base; binary conflicts must HOLD.',
      'A delete operation is a proposal only. Never apply it to an opened path or without a reviewed candidate and acceptance build.',
      'P4 opened add files absent from both submitted snapshots are reported as openedOnly, never as depot deletes.',
      'A keep against submitted P4@165 does not mean pending CL166/default workspace edits match Git; inspect workingDigestMatchesGit.',
    ] };
  plan.planSha256 = digest(Buffer.from(JSON.stringify(plan), 'utf8'));
  return plan;
}

function main() {
  const args = process.argv.slice(2);
  const usage = 'Usage: node strict-plan.mjs --git-repo <clean/hydrated repo> [--out <basename.json>]';
  if (args.includes('--help')) { process.stdout.write(`${usage}\n`); return; }
  const values = {};
  for (let i = 0; i < args.length; i += 2) {
    if (!args[i]?.startsWith('--') || !args[i + 1]) throw new Error(usage);
    const key = args[i].slice(2);
    if (!['git-repo', 'out'].includes(key) || Object.hasOwn(values, key)) throw new Error(usage);
    values[key] = args[i + 1];
  }
  if (!values['git-repo'] || !isAbsolute(values['git-repo'])) throw new Error('An absolute --git-repo is required');
  if (values.out && (basename(values.out) !== values.out || !/^[A-Za-z0-9_.-]+\.json$/.test(values.out))) {
    throw new Error('--out must be a simple JSON basename, written only inside this isolated planner folder');
  }
  const policyBytes = readFileSync(POLICY_PATH);
  const policy = JSON.parse(UTF8.decode(policyBytes));
  if (policy.schemaVersion !== 1 || policy.gitSha !== '4b359b3e931df3d4d7863e7033487aefbbb30399' ||
      policy.p4Cl !== 165 || policy.depot !== '//Dev01/main' || policy.maxTextBytes !== 2 * 1024 * 1024 ||
      policy.maxDocsBytes !== 16 * 1024 * 1024) {
    throw new Error('Unexpected policy: this tool is pinned to the reviewed Git/P4 snapshot');
  }
  const gitRepo = resolve(values['git-repo']);
  const gitEntries = readGit(gitRepo, policy.gitSha, policy.maxTextBytes, policy.maxDocsBytes);
  const p4Entries = readP4(policy.depot, policy.p4Cl);
  const opened = inspectOpenedWorkingTree(readOpened(policy.depot), gitEntries,
    policy.depot, policy.maxTextBytes, policy.maxDocsBytes);
  const changes = p4Read(['changes', '-m1', `${policy.depot}/...`]);
  const liveHead = Number(changes[0]?.change);
  const plan = buildPlan({ gitEntries, p4Entries, opened, gitSha: policy.gitSha,
    p4Cl: policy.p4Cl, depot: policy.depot, policySha256: digest(policyBytes),
    maxTextBytes: policy.maxTextBytes, maxDocsBytes: policy.maxDocsBytes });
  plan.liveP4HeadAtInspection = liveHead;
  plan.liveHeadMatchesPinned = liveHead === policy.p4Cl;
  if (!plan.liveHeadMatchesPinned) plan.warnings.push('Live P4 head differs from pinned CL165; plan is historical only.');
  delete plan.planSha256;
  plan.planSha256 = digest(Buffer.from(JSON.stringify(plan), 'utf8'));
  if (values.out) {
    const outputPath = join(HERE, values.out);
    writeFileSync(outputPath, `${JSON.stringify(plan, null, 2)}\n`, { flag: 'wx' });
    process.stdout.write(`${outputPath}\n`);
  }
  process.stdout.write(`${JSON.stringify({ gitSha: policy.gitSha, p4Cl: policy.p4Cl,
    liveP4Head: liveHead, counts: plan.counts, planSha256: plan.planSha256,
    openedOperations: plan.operations.filter((item) => item.opened.length).length,
    openedEligible: plan.openedEligible.length, openedOnly: plan.openedOnly.length,
    openedByChange: plan.openedByChange,
    blocked: plan.blocked.length, output: values.out ?? null })}\n`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) { process.stderr.write(`${error.stack ?? error}\n`); process.exitCode = 1; }
}

