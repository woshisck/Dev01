// Read-only pinned policy and terminal-state planner. No network, P4, or filesystem writes.
import { readFileSync } from 'node:fs';
import { classifyPath } from '../strict-mirror-planner/strict-plan.mjs';

export const GENESIS_GIT = '4b359b3e931df3d4d7863e7033487aefbbb30399';
export const GENESIS_P4 = 167;
export const UHT_PROJECT = 'Plugins/UnLua/Source/UnLuaDefaultParamCollectorUbtPlugin/UnLuaDefaultParamCollectorUbtPlugin.ubtplugin.csproj';

const ASSET_EXTENSIONS = new Set(['.uasset', '.umap', '.uexp', '.ubulk', '.uptnl', '.fbx', '.blend', '.png', '.jpg', '.jpeg', '.tga', '.wav', '.mp3']);
const GENERATED_SEGMENTS = new Set(['obj', 'bin', 'binaries', 'intermediate', 'saved', 'deriveddatacache', '.vs', '.idea', 'node_modules', 'dist', 'out', '__pycache__', '.pytest_cache', '.mypy_cache', '.cache']);
const PROTECTED_ROOTS = new Set(['engine', 'build', 'buildscripts', 'sourceart']);
// Checked against the actual UGS PerforceUtils.CodeExtensions in X:/Dev-BuildEngine.
const UGS_CODE_SUFFIXES = ['.c', '.cc', '.cpp', '.inl', '.m', '.mm', '.rc', '.cs', '.csproj', '.h', '.hpp', '.usf', '.ush', '.uproject', '.uplugin', '.sln', '.native.verse'];

function extension(path) {
  const leaf = path.slice(path.lastIndexOf('/') + 1);
  const dot = leaf.lastIndexOf('.');
  return dot < 0 ? '' : leaf.slice(dot).toLowerCase();
}

const REVIEWED_EXACT_TEXT_PATHS = new Set(JSON.parse(readFileSync(new URL('./policy.v2.json', import.meta.url), 'utf8')).reviewedExactTextPaths ?? []);
const EXACT_GIT_ONLY_PATHS = new Set(JSON.parse(readFileSync(new URL('./policy.v2.json', import.meta.url), 'utf8')).exactGitOnlyPaths ?? []);
const REVIEWABLE_GIT_ONLY_ROOT_FILES = new Set(['.gitignore', '.gitattributes', 'ChargeAbility_Plan.md', 'DebugCommands.md', 'StunCameraShake_Plan.md']);
for (const path of EXACT_GIT_ONLY_PATHS) {
  if (REVIEWABLE_GIT_ONLY_ROOT_FILES.has(path)) continue;
  const segments = path.toLowerCase().split('/');
  if (!path.startsWith('Tools/SyncBridge/') || /[@#%*\\]/.test(path) ||
      segments.some((part) => !part || part === '.' || part === '..' || ['dist', 'build', '__pycache__', '.git', 'node_modules', 'state', 'logs', 'cache', 'secrets'].includes(part)) ||
      /(?:^|\/)(?:config\.json|sync_state\.json|sync_bridge\.log)$/i.test(path) ||
      !/\.(?:mjs|js|py|ps1|md|json|txt|cmd|bat)$/i.test(path)) throw new Error('InvalidExactGitOnlyPath');
}
for (const path of REVIEWED_EXACT_TEXT_PATHS) {
  if (!/^Plugins\/YogVision\/(?:Shaders\/Private\/[^/]+\.usf|Resources\/[^/]+\.(?:hlsl|py|json|txt))$/.test(path) || /[@#%*\\]/.test(path)) {
    throw new Error('InvalidReviewedExactTextPath');
  }
}

export function classifyV2(path) {
  if (REVIEWED_EXACT_TEXT_PATHS.has(path)) {
    return { eligible: true, kind: path.endsWith('.usf') ? 'plugin-shader-source' : 'plugin-material-authoring-source' };
  }
  // Exact exception, not all .csproj files. The old classifier remains immutable evidence.
  if (path === UHT_PROJECT) return { eligible: true, kind: 'plugin-build-input' };
  return classifyPath(path);
}

export function classifyExcludedChange(path) {
  if (EXACT_GIT_ONLY_PATHS.has(path)) return 'git-only-skip';
  const parts = path.toLowerCase().split('/');
  if (PROTECTED_ROOTS.has(parts[0]) || ['.zip', '.dll', '.exe', '.pdb', '.pak'].includes(extension(path))) return 'protected-release-input';
  if (parts.some((part) => GENERATED_SEGMENTS.has(part))) return 'generated-skip';
  if ((parts[0] === 'content' || parts[0] === 'plugins') && ASSET_EXTENSIONS.has(extension(path))) return 'asset-skip';
  return 'unreviewed-path';
}

function normalizeEntries(entries) {
  return entries instanceof Map ? entries : new Map(Object.entries(entries ?? {}));
}

function equalEntry(a, b) {
  if (!a || !b) return false;
  return String(a.digest).toLowerCase() === String(b.digest).toLowerCase() && Number(a.size) === Number(b.size);
}

function acceptableP4Type(path, kind, type) {
  if (path === UHT_PROJECT) return type === 'binary';
  if (kind === 'docs') return /^(?:text|unicode|utf8|utf16|binary)$/i.test(type ?? '');
  return /^(?:text|unicode|utf8|utf16)(?:\+[A-Za-z0-9]+)?$/i.test(type ?? '');
}

export function planTerminal({ gitEntries, p4Entries, changedPaths = [], openedPaths = [] }) {
  const git = normalizeEntries(gitEntries);
  const p4 = normalizeEntries(p4Entries);
  const opened = new Set(openedPaths);
  const actions = [];
  const skipped = [];
  const holds = [];
  const paths = [...new Set([...git.keys(), ...p4.keys()])].sort();
  const windowsPaths = new Map();
  for (const path of paths) {
    if (!classifyV2(path).eligible) continue;
    const key = path.toLowerCase();
    const prior = windowsPaths.get(key);
    if (prior && prior !== path) holds.push({ path, otherPath: prior, reason: 'windows-case-collision' });
    else windowsPaths.set(key, path);
  }
  for (const path of paths) {
    const policy = classifyV2(path);
    if (!policy.eligible) {
      if (git.has(path)) skipped.push({ path, reason: classifyExcludedChange(path) });
      continue;
    }
    const g = git.get(path);
    const p = p4.get(path);
    if (!g && !p) continue;
    if (p && !acceptableP4Type(path, policy.kind, p.type)) {
      holds.push({ path, reason: 'p4-filetype-unverified-or-disallowed', type: p.type ?? null });
    }
    const action = !p ? 'add' : !g ? 'delete' : equalEntry(g, p) ? null : 'edit';
    if (action) actions.push({ action, path, kind: policy.kind, git: g ?? null, p4: p ?? null });
    if (action && opened.has(path)) holds.push({ path, reason: 'opened-overlap' });
  }
  for (const path of changedPaths) {
    if (classifyV2(path).eligible) continue;
    const reason = classifyExcludedChange(path);
    if (reason === 'asset-skip' || reason === 'generated-skip' || reason === 'git-only-skip') {
      if (!skipped.some((x) => x.path === path)) skipped.push({ path, reason });
    } else holds.push({ path, reason });
  }
  holds.sort((a, b) => a.path.localeCompare(b.path));
  skipped.sort((a, b) => a.path.localeCompare(b.path));
  return { status: holds.length ? 'HOLD' : actions.length ? 'PLAN' : 'NO_OP', actions, skipped, holds,
    impact: classifyImpact(actions), openedUnrelated: [...opened].filter((x) => !actions.some((a) => a.path === x)).sort() };
}

export function classifyImpact(actions) {
  if (!actions.length) return 'no-change';
  let binaryBuildRequired = false;
  for (const { path } of actions) {
    // Material Custom-node sources need an explicit asset/build-impact review on later edits.
    if (REVIEWED_EXACT_TEXT_PATHS.has(path) && path.endsWith('.hlsl')) return 'ambiguous-build-input-hold';
    // UGS classifies depot paths by suffix unless an independently verified
    // CodeFilter says otherwise. Do not silently reuse a PCB for Docs/*.cpp,
    // or auto-build one under the blanket "Docs-only" policy.
    const ugsCodeSuffix = UGS_CODE_SUFFIXES.some((suffix) => path.toLowerCase().endsWith(suffix));
    if (classifyV2(path).kind === 'docs' &&
        ugsCodeSuffix) return 'ambiguous-build-input-hold';
    if (/^(?:Source|Plugins\/[^/]+\/Source)\//i.test(path) && !ugsCodeSuffix &&
        classifyV2(path).kind !== 'plugin-build-input') return 'ambiguous-build-input-hold';
    if (ugsCodeSuffix || classifyV2(path).kind === 'plugin-build-input') binaryBuildRequired = true;
  }
  return binaryBuildRequired ? 'binary-build-required' : 'metadata-only';
}

export function detectCommitState({ genesis, remote, cached, descendant }) {
  if (!/^[0-9a-f]{40}$/i.test(remote ?? '')) return { status: 'HOLD', reason: 'invalid-remote-sha' };
  if (remote === genesis) return { status: 'NO_NEW_COMMIT' };
  if (!cached) return { status: 'NEEDS_FETCH' };
  if (!descendant) return { status: 'HOLD', reason: 'force-push-or-divergence' };
  return { status: 'NEW_COMMIT' };
}

export function assessProvenance({ detectionStatus, actionCount, p4Head, genesisP4 = GENESIS_P4, reviewedParityP4 = 169 }) {
  if (p4Head !== genesisP4 && p4Head !== reviewedParityP4) return { status: 'HOLD', reason: 'unreviewed-or-rejected-p4-head' };
  if (detectionStatus === 'NO_NEW_COMMIT' && actionCount > 0) return { status: 'HOLD', reason: 'p4-divergence-without-git-update' };
  return { status: 'PASS', reason: p4Head === genesisP4 ? 'reviewed-genesis' : 'reviewed-manual-byte-parity-not-worker' };
}

