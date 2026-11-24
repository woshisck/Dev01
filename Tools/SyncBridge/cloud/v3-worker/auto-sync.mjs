// Dev01 GitHub-main -> P4 worker. This file is deployed only after a pinned
// baseline, isolated P4 client, and live no-op rehearsal have been audited.
import { spawnSync } from 'node:child_process';
import { createHash, randomUUID } from 'node:crypto';
import {
  closeSync, existsSync, fsyncSync, mkdirSync, openSync, readFileSync,
  renameSync, unlinkSync, writeFileSync, writeSync,
} from 'node:fs';
import { dirname, isAbsolute, join, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';
import { readChangedPaths, readGit, readOpened, readP4, parseGitTree, parseP4Tags } from '../v2-readonly/read-live.mjs';
import { classifyImpact, classifyV2, planTerminal, UHT_PROJECT } from '../v2-readonly/planner.mjs';
import { textEncoding } from '../strict-mirror-planner/strict-plan.mjs';
import { recoveryActionsDigest, validateRecoveryReview, validateApprovedOpenedOverlaps, gitNetworkArgs } from './recovery-evidence.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const POLICY = JSON.parse(readFileSync(join(HERE, '../v2-readonly/policy.v2.json')));
const REMOTE = 'https://github.com/woshisck/Dev01.git';
const DEPOT = '//Dev01/main';
const ARCHIVE = '//Dev01Binaries/UGS/++Dev01+main-Editor.zip';
const USER = 'Dev01BuildAgent';
const CLIENT = 'mirror_v3_10_0_0_10_Dev01_main';
const ROOT = 'C:\\Project\\Dev01-MirrorV3-20261024';
const GIT = 'C:\\Project\\Dev01-GitHub';
const STATE_ROOT = 'C:\\BuildAgent\\Dev01\\state\\git-to-p4-v3';
const SHA = /^[0-9a-f]{40}$/;
const UUID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
const UTF8 = new TextDecoder('utf-8', { fatal: true });
const GBK = new TextDecoder('gbk', { fatal: true });
const sha256 = (b) => createHash('sha256').update(b).digest('hex');
const md5 = (b) => createHash('md5').update(b).digest('hex');
const fail = (reason) => { throw new Error(reason); };
const assert = (condition, reason) => { if (!condition) fail(reason); };
const decode = (bytes) => { try { return UTF8.decode(bytes); } catch { return GBK.decode(bytes); } };

function command(binary, argv, { cwd = ROOT, input, allow = [] } = {}) {
  const result = spawnSync(binary, argv, { cwd, input, encoding: null,
    maxBuffer: 256 * 1024 * 1024, timeout: 120000, windowsHide: true,
    env: { ...process.env, GIT_TERMINAL_PROMPT: '0', GCM_INTERACTIVE: 'never' } });
  if (result.error) fail(`${binary} ${argv.at(0)} failed: ${result.error.message}`);
  if (result.status !== 0 && !allow.includes(result.status)) {
    fail(`${binary} ${argv.at(0)} exit ${result.status}: ${decode(result.stdout).slice(0, 300)} ${decode(result.stderr).slice(0, 300)}`);
  }
  return result;
}
const git = (args, options = {}) => command('git', [...gitNetworkArgs(POLICY), '-C', GIT, ...args], { cwd: GIT, ...options });
const p4 = (args, options = {}) => command('p4', ['-u', USER, '-c', CLIENT, ...args], options);
const gitText = (args, options = {}) => git(args, options).stdout.toString('utf8').trim();
const p4Text = (args, options = {}) => decode(p4(args, options).stdout).trim();

function jsonAtomic(path, data) {
  const temp = `${path}.tmp-${randomUUID()}`;
  const fd = openSync(temp, 'wx', 0o600);
  try { writeSync(fd, Buffer.from(`${JSON.stringify(data, null, 2)}\n`, 'utf8')); fsyncSync(fd); }
  finally { closeSync(fd); }
  renameSync(temp, path);
}

function checkpointIdentity() {
  const state = JSON.parse(readFileSync(join(STATE_ROOT, 'checkpoint.json'), 'utf8'));
  assert(state.schema === 3 && state.status === 'active' && SHA.test(state.gitCommit) &&
    Number.isSafeInteger(state.p4Change) && Number.isSafeInteger(state.codeCL) &&
    Number.isSafeInteger(state.engineCL) && UUID.test(state.buildId) &&
    state.repository === REMOTE && state.stream === DEPOT && state.archive === ARCHIVE,
  'InvalidCheckpoint');
  return state;
}

function readRecoveryReview(state, evidence, extra = {}) {
  assert(evidence?.schema === 1 && /^[0-9a-f]{64}$/.test(evidence.reviewSha256 ?? ''), 'RecoveryEvidenceInvalid');
  const reviewPath = join(STATE_ROOT, 'receipts', 'recovery-reviews', `${evidence.reviewSha256}.json`);
  return validateRecoveryReview({ state,
    checkpointBytes: readFileSync(join(STATE_ROOT, 'checkpoint.json')),
    policyBytes: readFileSync(join(HERE, '../v2-readonly/policy.v2.json')),
    reviewBytes: readFileSync(reviewPath), evidence, ...extra });
}

export function readCheckpointForRecovery(expectedCheckpointSha256, expectedOldPolicySha256) {
  assert(/^[0-9a-f]{64}$/.test(expectedCheckpointSha256) && /^[0-9a-f]{64}$/.test(expectedOldPolicySha256), 'RecoveryCheckpointPinInvalid');
  const bytes = readFileSync(join(STATE_ROOT, 'checkpoint.json'));
  assert(sha256(bytes) === expectedCheckpointSha256, 'RecoveryCheckpointChanged');
  const state = checkpointIdentity();
  assert(state.policySha256 === expectedOldPolicySha256, 'RecoveryPriorPolicyMismatch');
  return state;
}

function loadState() {
  const state = checkpointIdentity();
  if (state.policySha256 !== sha256(readFileSync(join(HERE, '../v2-readonly/policy.v2.json')))) {
    const activePath = join(STATE_ROOT, 'active.json');
    assert(existsSync(activePath), 'MirrorPolicyChanged');
    const journal = JSON.parse(readFileSync(activePath, 'utf8'));
    readRecoveryReview(state, journal.recoveryEvidence, { journal });
  }
  return state;
}


function p4Head() {
  const line = p4Text(['changes', '-m', '1', `${DEPOT}/...`]);
  const match = /^Change (\d+)\s/.exec(line);
  assert(match, 'UnreadableP4Head');
  return Number(match[1]);
}

export function readOpenedForRecovery(actions, ownChange = null) {
  const paths = new Set(actions.map((item) => item.path));
  const opened = parseP4Tags(p4(['-ztag', 'opened', '-a', `${DEPOT}/...`], { allow: [1] }).stdout)
    .filter((row) => row.depotFile?.startsWith(`${DEPOT}/`))
    .filter((row) => paths.has(row.depotFile.slice(DEPOT.length + 1)))
    .filter((row) => !(ownChange != null && row.user === USER && row.client === CLIENT && row.change === String(ownChange)));
  if (!opened.length) return [];
  const depots = [...new Set(opened.map((row) => row.depotFile))];
  const metadata = new Map(parseP4Tags(p4(['-ztag', 'fstat', '-Ol', '-T', 'depotFile,headRev,headType,digest,fileSize', ...depots]).stdout)
    .filter((row) => row.depotFile).map((row) => [row.depotFile, row]));
  return opened.map((row) => {
    const meta = metadata.get(row.depotFile);
    assert(meta && meta.digest && meta.fileSize && meta.headRev, 'RecoveryOpenedMetadataMissing');
    return { path: row.depotFile.slice(DEPOT.length + 1), user: row.user, client: row.client,
      change: row.change, action: row.action, type: row.type, rev: row.rev ?? null, haveRev: row.haveRev ?? null,
      serverDigest: meta.digest.toLowerCase(), serverSize: Number(meta.fileSize),
      serverRev: Number(meta.headRev), serverType: meta.headType };
  });
}

function noResult(result, allowed) {
  const value = `${decode(result.stdout)}\n${decode(result.stderr)}`.trim();
  return [0, 1].includes(result.status) && (!value || allowed.test(value));
}

function assertClient() {
  assert(existsSync(ROOT), 'MissingIsolatedClientRoot');
  const info = p4Text(['info']);
  assert(info.includes(`Client name: ${CLIENT}`) && info.includes(`Client root: ${ROOT}`) &&
    info.includes('Client stream: //Dev01/main') && info.includes(`User name: ${USER}`),
  'P4ClientIdentityMismatch');
  const spec = p4Text(['client', '-o', CLIENT]);
  assert(/^LineEnd:\s+unix\s*$/m.test(spec) &&
    new RegExp(`^Host:\\s+${process.env.COMPUTERNAME}\\s*$`, 'im').test(spec),
  'P4ClientContractMismatch');
  assert(noResult(p4(['opened'], { allow: [1] }), /^File\(s\) not opened on this client\.$/),
  'MirrorClientHasOpenedFiles');
  assert(noResult(p4(['changes', '-s', 'pending', '-c', CLIENT], { allow: [1] }), /^(?:No pending changelists\.)?$/),
  'MirrorClientHasPendingChange');
  assert(noResult(p4(['resolve', '-n'], { allow: [1] }), /^No file\(s\) to resolve\.$/),
  'MirrorClientNeedsResolve');
  const reconcile = p4(['reconcile', '-n', '-I', '-e', '-a', '-d', `${DEPOT}/...`]);
  assert(noResult(reconcile, /^\/\/Dev01\/main\/\.\.\. - no file\(s\) to reconcile\.$/),
    'MirrorClientLocalFilesDrifted');
}

function assertEngine(state) {
  const stream = p4Text(['stream', '-o', DEPOT]);
  assert(stream.includes(`import Engine/... //Dev01Engine/main/...@${state.engineCL}`), 'EngineImportChanged');
  const modules = JSON.parse(readFileSync('C:\\Project\\Dev01-P4\\Engine\\Binaries\\Win64\\UnrealEditor.modules', 'utf8'));
  assert(modules.BuildId === state.buildId, 'EngineBuildIdChanged');
}

function advertisedMain() {
  const line = gitText(['ls-remote', 'origin', 'refs/heads/main']);
  const match = /^([0-9a-f]{40})\s+refs\/heads\/main$/.exec(line);
  assert(match, 'UnreadableGitHubMain');
  return match[1];
}

function fetchExact() {
  assert(gitText(['remote', 'get-url', 'origin']) === REMOTE, 'UnexpectedGitOrigin');
  assert(!gitText(['status', '--porcelain=v1']), 'GitWorkspaceDirty');
  assert(gitText(['branch', '--show-current']) === 'main', 'GitBranchChanged');
  const remote = advertisedMain();
  if (gitText(['rev-parse', 'refs/remotes/origin/main']) !== remote) {
    git(['fetch', '--no-tags', 'origin', 'main']);
  }
  assert(gitText(['rev-parse', 'refs/remotes/origin/main']) === remote &&
    advertisedMain() === remote, 'GitRemoteMovedDuringFetch');
  return remote;
}

function assertDescendant(base, target) {
  const ancestor = git(['merge-base', '--is-ancestor', base, target], { allow: [1] });
  assert(ancestor.status === 0, 'GitHistoryRewritten');
}

function assertMainContains(target) {
  const remote = advertisedMain();
  if (remote === target) return;
  git(['fetch', '--no-tags', 'origin', 'main']);
  assert(gitText(['rev-parse', 'refs/remotes/origin/main']) === remote &&
    advertisedMain() === remote, 'GitRemoteMovedDuringTransaction');
  assertDescendant(target, remote);
}

function typeFor(item, bytes) {
  if (item.action === 'edit' && item.p4?.type) return item.p4.type;
  if (item.path === UHT_PROJECT) return 'binary';
  if (classifyV2(item.path).kind === 'docs' && !textEncoding(bytes, true)) return 'binary';
  return 'text';
}

function safeLocal(path) {
  assert(classifyV2(path).eligible && !/[@#%*]/.test(path), `UnsafePath:${path}`);
  const target = resolve(ROOT, ...path.split('/'));
  const relative = target.slice(ROOT.length);
  assert(relative.startsWith(sep) && !relative.slice(1).startsWith(`..${sep}`), `EscapedClientRoot:${path}`);
  return target;
}

export function readRawGitBlob(gitCommand, sha, path, expected) {
  // NUL-delimited names stay literal even with core.quotePath=true or CJK paths.
  const tree = parseGitTree(gitCommand(['ls-tree', '-rz', sha, '--', path]).stdout);
  const entry = tree.get(path);
  assert(tree.size === 1 && entry?.mode === '100644' && entry.type === 'blob' &&
    SHA.test(entry.oid) && (expected.oid === undefined || entry.oid === expected.oid), `GitBlobUnavailable:${path}`);
  const bytes = gitCommand(['cat-file', 'blob', entry.oid]).stdout;
  assert(bytes.length === expected.size && sha256(bytes) === expected.sha256 &&
    md5(bytes) === expected.digest, `GitBlobDrift:${path}`);
  return bytes;
}

function rawBlob(sha, path, expected) {
  return readRawGitBlob(git, sha, path, expected);
}

function archiveMeta() {
  const lines = p4Text(['fstat', '-T', 'depotFile,headRev,headChange', ARCHIVE]);
  const rev = Number(/^\.\.\. headRev (\d+)$/m.exec(lines)?.[1]);
  const change = Number(/^\.\.\. headChange (\d+)$/m.exec(lines)?.[1]);
  assert(Number.isSafeInteger(rev) && rev > 0 && Number.isSafeInteger(change), 'ArchiveMetadataUnreadable');
  return { rev, change };
}

function archiveDescriptionMatches(change, codeCL) {
  const text = p4Text(['-ztag', 'describe', '-s', String(change)]);
  return /^\.\.\. status submitted$/m.test(text) &&
    new RegExp(`^\\.\\.\\. desc \\[CL ${String(codeCL).padStart(8, '0')}\\] Cloud-built DevKitEditor`, 'm').test(text) &&
    new RegExp(`^\\.\\.\\. depotFile0 ${ARCHIVE.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}$`, 'm').test(text) &&
    !/^\.\.\. depotFile1 /m.test(text) && /^\.\.\. type0 binary(?:\+S\d+)?$/m.test(text);
}

function priorPcbMatches(state) {
  const meta = archiveMeta();
  if (meta.rev !== state.archiveRevision || meta.change !== state.archiveChange) return false;
  return archiveDescriptionMatches(meta.change, state.codeCL);
}

function report(status, reason, extra = {}) {
  const payload = { Timestamp: new Date().toISOString(), Mode: 'AutomaticGitHubToP4', Status: status,
    Reason: reason, WritesToGitOrP4: extra.P4Submitted === true, ...extra };
  jsonAtomic(join(STATE_ROOT, 'status.json'), payload);
  process.stdout.write(`${JSON.stringify(payload)}\n`);
  if (status === 'Blocked') process.exitCode = 2;
}

function checkPlan(state, target) {
  const oldTree = readGit(GIT, state.gitCommit, POLICY);
  const p4Tree = readP4(ROOT, DEPOT);
  assert(!oldTree.violations.length && !p4Tree.violations.length, 'BaselineTreeInvalid');
  const baseline = planTerminal({ gitEntries: oldTree.entries, p4Entries: p4Tree.entries });
  assert(baseline.status === 'NO_OP', 'P4SharedTreeDivergedSinceCheckpoint');
  const newTree = readGit(GIT, target, POLICY);
  assert(!newTree.violations.length, 'GitTreeViolatesPolicy');
  const changedPaths = readChangedPaths(GIT, state.gitCommit, target);
  const openedPaths = readOpened(ROOT, DEPOT);
  const plan = planTerminal({ gitEntries: newTree.entries, p4Entries: p4Tree.entries,
    changedPaths, openedPaths });
  assert(plan.status !== 'HOLD', `PlanHold:${JSON.stringify(plan.holds.slice(0, 8))}`);
  assert(plan.actions.length <= 10000, 'TooManyActionsForOneAutomaticChange');
  assert(plan.impact !== 'ambiguous-build-input-hold', 'AmbiguousUGSCodeImpact');
  return { plan, changedPaths, openedCount: openedPaths.length };
}

function runDryOrIdle(state, target, head, apply) {
  const { plan, changedPaths, openedCount } = checkPlan(state, target);
  if (!plan.actions.length && target !== state.gitCommit && apply) {
    assert(priorPcbMatches(state), 'PriorPCBNotVerified');
    const next = { ...state, gitCommit: target, p4Change: head,
      updatedAt: new Date().toISOString(), lastDisposition: 'asset-or-generated-no-op' };
    jsonAtomic(join(STATE_ROOT, 'checkpoint.json'), next);
    report('VerifiedNoSharedChanges', 'SkippedNonMirroredChanges',
      { GitCommit: target, P4Change: head, Skipped: plan.skipped.length });
    return;
  }
  if (!plan.actions.length) {
    report(target === state.gitCommit ? 'VerifiedNoSharedChanges' : 'DryRunReviewable',
      target === state.gitCommit ? 'NoChanges' : 'SkippedNonMirroredChangesPreview',
      { GitCommit: target, P4Change: head, OpenedElsewhere: openedCount,
        Skipped: plan.skipped.length });
    return;
  }
  if (!apply) {
    report('DryRunReviewable', 'ChangesReadyForStaging', { GitCommit: target,
      P4Change: head, Impact: plan.impact, Actions: plan.actions.map(({ action, path }) => ({ action, path })),
      ChangedPathCount: changedPaths.length, Skipped: plan.skipped.length });
    return;
  }
  stageSubmit(state, target, head, plan);
}

function stageSubmit(state, target, head, plan, recoveryEvidence = null) {
  const journalPath = join(STATE_ROOT, 'active.json');
  assert(!existsSync(journalPath), 'PriorTransactionRequiresReview');
  assert(p4Head() === head, 'P4HeadMovedBeforeStage');
  assertMainContains(target);
  if (recoveryEvidence) {
    const review = readRecoveryReview(state, recoveryEvidence, { actions: plan.actions });
    assert(plan.status !== 'HOLD' && plan.impact === 'binary-build-required' &&
      target === review.pins.targetGitCommit && head === review.pins.baseP4Head, 'RecoveryPlanIdentityMismatch');
    assertClient();
    assertEngine(state);
    validateApprovedOpenedOverlaps(review.approvedOpenedOverlaps ?? [], readOpenedForRecovery(plan.actions));
    const meta = archiveMeta();
    assert(meta.rev === review.pins.archiveRevision && meta.change === review.pins.archiveChange &&
      archiveDescriptionMatches(meta.change, review.pins.codeCL), 'RecoveryExternalPCBChanged');
    assert(archiveDescriptionMatches(state.archiveChange, state.codeCL), 'RecoveryPriorPCBIdentityMismatch');
    for (const revision of new Set([state.archiveRevision, meta.rev])) {
      const verified = p4(['verify', '-q', `${ARCHIVE}#${revision}`]);
      assert(!verified.stdout.length && !verified.stderr.length, 'RecoveryPCBVerifyNotQuiet');
    }
    const build = JSON.parse(readFileSync('C:\\BuildAgent\\Dev01\\state\\completed_build.json', 'utf8'));
    assert(build.status === 'complete' && build.project_cl === head && build.code_cl === head &&
      build.engine_cl === state.engineCL && build.engine_build_id === state.buildId, 'RecoveryExternalBuildReceiptMismatch');
    assert(p4Head() === head && archiveMeta().rev === meta.rev && archiveMeta().change === meta.change,
      'RecoveryReleaseMovedDuringVerify');
    const priorCheckpoint = readFileSync(join(STATE_ROOT, 'checkpoint.json'));
    assert(sha256(priorCheckpoint) === review.pins.oldCheckpointSha256, 'RecoveryCheckpointChangedBeforeStage');
    const backupDirectory = join(STATE_ROOT, 'receipts', 'recovery-checkpoints');
    mkdirSync(backupDirectory, { recursive: true });
    const checkpointBackup = join(backupDirectory, `${review.pins.oldCheckpointSha256}.json`);
    if (existsSync(checkpointBackup)) assert(sha256(readFileSync(checkpointBackup)) === review.pins.oldCheckpointSha256,
      'RecoveryCheckpointBackupConflict');
    else {
      const backupFd = openSync(checkpointBackup, 'wx', 0o600);
      try { writeSync(backupFd, priorCheckpoint); fsyncSync(backupFd); } finally { closeSync(backupFd); }
    }
    recoveryEvidence = { ...recoveryEvidence, checkpointBackup };
  }
  const actions = plan.actions.map((item) => ({ action: item.action, path: item.path,
    depot: `${DEPOT}/${item.path}`,
    bytes: item.action === 'delete' ? null : rawBlob(target, item.path, item.git),
    type: item.action === 'delete' ? item.p4.type :
      item.action === 'edit' ? item.p4.type : null,
    priorDigest: item.p4?.digest ?? null, priorSize: item.p4?.size ?? null,
    priorRev: item.p4?.rev ?? null, priorType: item.p4?.type ?? null }));
  for (const item of actions) if (item.bytes && !item.type) item.type = typeFor(item, item.bytes);
  const journal = { schema: 1, state: 'stage-started', gitCommit: target,
    priorGitCommit: state.gitCommit, baseP4Head: head, priorCodeCL: state.codeCL,
    engineCL: state.engineCL, buildId: state.buildId, impact: plan.impact,
    actions: actions.map(({ action, path, type, bytes, priorDigest, priorSize, priorRev, priorType }) => ({ action, path, type,
      sha256: bytes && sha256(bytes), digest: bytes && md5(bytes), size: bytes?.length ?? null,
      priorDigest, priorSize, priorRev, priorType })),
    ...(recoveryEvidence ? { recoveryEvidence } : {}),
    createdAt: new Date().toISOString() };
  if (recoveryEvidence) readRecoveryReview(state, recoveryEvidence, { journal });
  jsonAtomic(journalPath, journal);
  const description = `GitHub main ${target} strict mirror from P4 CL${head}; ${plan.impact}`;
  const spec = `Change: new\nClient: ${CLIENT}\nUser: ${USER}\nStatus: new\nDescription:\n\t${description}\n`;
  const created = p4Text(['change', '-i'], { input: Buffer.from(spec, 'ascii') });
  const cl = Number(/Change (\d+) created\./.exec(created)?.[1]);
  assert(Number.isSafeInteger(cl) && cl > head, 'P4ChangeCreationUncertain');
  journal.change = cl;
  journal.state = 'change-created';
  jsonAtomic(journalPath, journal);
  for (const item of actions) {
    const local = safeLocal(item.path);
    if (item.action !== 'add') p4(['sync', '-q', `${item.depot}@${head}`]);
    if (item.action === 'edit') p4(['edit', '-c', String(cl), item.depot]);
    if (item.action === 'delete') p4(['delete', '-c', String(cl), item.depot]);
    if (item.bytes) {
      mkdirSync(dirname(local), { recursive: true });
      if (item.action === 'add') assert(!existsSync(local), `UnexpectedLocalAddPath:${item.path}`);
      writeFileSync(local, item.bytes);
      assert(sha256(readFileSync(local)) === sha256(item.bytes), `LocalStageByteMismatch:${item.path}`);
      if (item.action === 'add') p4(['add', '-t', item.type, '-c', String(cl), item.depot]);
    }
  }
  journal.state = 'opened';
  jsonAtomic(journalPath, journal);
  verifyOwnOpened(cl, actions);
  assert(p4Head() === head, 'P4HeadMovedBeforeShelf');
  assertMainContains(target);
  p4(['shelve', '-c', String(cl)]);
  journal.state = 'shelved';
  jsonAtomic(journalPath, journal);
  verifyShelf(cl, actions);
  journal.actions = actions.map(({ action, path, type, bytes, priorDigest, priorSize, priorRev, priorType }) => ({ action, path, type,
    sha256: bytes && sha256(bytes), digest: bytes && md5(bytes), size: bytes?.length ?? null,
    priorDigest, priorSize, priorRev, priorType }));
  journal.state = 'shelf-verified';
  jsonAtomic(journalPath, journal);
  assert(p4Head() === head, 'P4HeadMovedBeforeSubmit');
  assertMainContains(target);
  verifyOwnOpened(cl, actions);
  const opened = readOpened(ROOT, DEPOT);
  const ownPaths = new Set(actions.map((x) => x.path));
  const overlappingElsewhere = opened.filter((x) => ownPaths.has(x));
  if (recoveryEvidence) {
    const review = readRecoveryReview(state, recoveryEvidence, { journal });
    validateApprovedOpenedOverlaps(review.approvedOpenedOverlaps ?? [], readOpenedForRecovery(plan.actions, cl));
  } else assert(overlappingElsewhere.length === actions.length, 'OpenedOwnershipChangedBeforeSubmit');
  journal.state = 'submit-started';
  jsonAtomic(journalPath, journal);
  p4(['shelve', '-d', '-c', String(cl)]);
  p4(['submit', '-c', String(cl)]);
  verifySubmitted(cl, target, actions);
  journal.state = plan.impact === 'binary-build-required' ? 'submitted-await-pcb' : 'submitted-metadata';
  journal.submittedCL = cl;
  jsonAtomic(journalPath, journal);
  if (journal.state === 'submitted-metadata') {
    assert(priorPcbMatches(state), 'PriorPCBNotVerified');
    finishTransaction(state, journal, state.codeCL, state.archiveRevision, state.archiveChange);
    report('VerifiedNoSharedChanges', 'SubmittedMetadataReusingPCB', { GitCommit: target,
      P4Change: cl, P4Submitted: true });
  } else report('SubmittedAwaitingPCB', 'ProductionPCBTaskWillBuild',
    { GitCommit: target, P4Change: cl, CodeCL: cl, P4Submitted: true });
}

function verifyOwnOpened(change, actions) {
  const rows = p4Text(['-ztag', 'opened', '-c', String(change)]).split(/\r?\n\s*\r?\n/)
    .filter(Boolean).map((record) => Object.fromEntries(
      [...record.matchAll(/^\.\.\. ([A-Za-z0-9]+) ?(.*)$/gm)].map((match) => [match[1], match[2]])));
  assert(rows.length === actions.length, 'OpenedCountMismatch');
  const expected = new Map(actions.map((x) => [x.depot, x]));
  for (const row of rows) {
    const item = expected.get(row.depotFile);
    assert(item && row.action === item.action && row.change === String(change) &&
      row.client === CLIENT && row.user === USER && row.type === item.type,
    `OpenedActionMismatch:${row.depotFile ?? 'unknown'}`);
    expected.delete(row.depotFile);
  }
  assert(expected.size === 0, 'OpenedPathMissing');
}

function verifyRows(change, actions, shelf, allowDocsRetry = false) {
  const output = p4Text(['-ztag', 'describe', '-s', ...(shelf ? ['-S'] : []), String(change)]);
  const rows = [...output.matchAll(/^\.\.\. depotFile(\d+) (.+)$/gm)].map((match) => ({
    path: match[2], index: match[1],
  }));
  assert(rows.length === actions.length, 'P4ActionCountMismatch');
  const expected = new Map(actions.map((x) => [x.depot, x]));
  const docsToRetype = [];
  for (const row of rows) {
    const item = expected.get(row.path);
    assert(item, `UnexpectedP4Path:${row.path}`);
    const action = new RegExp(`^\\.\\.\\. action${row.index} (.+)$`, 'm').exec(output)?.[1];
    const type = new RegExp(`^\\.\\.\\. type${row.index} (.+)$`, 'm').exec(output)?.[1];
    assert(action === item.action && type === item.type, `P4ActionOrTypeMismatch:${row.path}`);
    if (item.bytes) {
      const result = p4(['print', '-q', `${item.depot}@=${change}`]);
      assert(result.stderr.length === 0, `P4PrintWarning:${row.path}`);
      const printed = result.stdout;
      if (sha256(printed) !== sha256(item.bytes)) {
        if (allowDocsRetry && classifyV2(item.path).kind === 'docs' && item.type === 'text')
          docsToRetype.push(item);
        else fail(`P4ByteMismatch:${row.path}`);
      }
    }
    expected.delete(row.path);
  }
  assert(expected.size === 0, 'MissingP4Actions');
  return docsToRetype;
}

function verifyShelf(change, actions) {
  const docs = verifyRows(change, actions, true, true);
  if (docs.length) {
    for (const item of docs) {
      const local = safeLocal(item.path);
      assert(sha256(readFileSync(local)) === sha256(item.bytes), `DocsLocalByteDrift:${item.path}`);
      p4(['reopen', '-t', 'binary', '-c', String(change), item.depot]);
      item.type = 'binary';
    }
    p4(['shelve', '-f', '-c', String(change)]);
    verifyRows(change, actions, true);
  }
}
function verifySubmitted(change, sha, actions) {
  const description = p4Text(['describe', '-s', String(change)]);
  assert(description.includes(`GitHub main ${sha} strict mirror`), 'SubmittedMarkerMismatch');
  verifyRows(change, actions, false);
}

function finishTransaction(state, journal, codeCL, archiveRevision, archiveChange) {
  if (journal.recoveryEvidence) readRecoveryReview(state, journal.recoveryEvidence, { journal });
  const next = { ...state,
    policySha256: journal.recoveryEvidence ? sha256(readFileSync(join(HERE, '../v2-readonly/policy.v2.json'))) : state.policySha256,
    gitCommit: journal.gitCommit, p4Change: journal.submittedCL,
    codeCL, archiveRevision, archiveChange, updatedAt: new Date().toISOString(),
    lastDisposition: journal.impact };
  jsonAtomic(join(STATE_ROOT, 'checkpoint.json'), next);
  journal.state = 'complete';
  jsonAtomic(join(STATE_ROOT, 'active.json'), journal);
  renameSync(join(STATE_ROOT, 'active.json'), join(STATE_ROOT, 'receipts', `${journal.gitCommit}.json`));
}

function resumeSubmitted(state, journal) {
  assert(journal.gitCommit && SHA.test(journal.gitCommit) &&
    journal.engineCL === state.engineCL && journal.buildId === state.buildId,
  'ActiveTransactionIdentityMismatch');
  if (state.gitCommit === journal.gitCommit && state.p4Change === journal.submittedCL &&
    state.lastDisposition === journal.impact &&
    ['submitted-await-pcb', 'submitted-metadata', 'complete'].includes(journal.state)) {
    assert(!existsSync(join(STATE_ROOT, 'receipts', `${journal.gitCommit}.json`)),
      'DuplicateCompletedReceipt');
    renameSync(join(STATE_ROOT, 'active.json'), join(STATE_ROOT, 'receipts', `${journal.gitCommit}.json`));
    report('VerifiedNoSharedChanges', 'RecoveredCompletedReceipt', { GitCommit: state.gitCommit });
    return;
  }
  assert(journal.priorGitCommit === state.gitCommit, 'CheckpointChangedDuringTransaction');
  assert(['submit-started', 'submitted-await-pcb', 'submitted-metadata'].includes(journal.state),
    'IncompleteTransactionNeedsManualRecovery');
  const submittedCL = journal.state === 'submit-started' ? journal.change : journal.submittedCL;
  assert(Number.isSafeInteger(submittedCL) && submittedCL > journal.baseP4Head,
    'SubmittedChangeIdentityMissing');
  assert(p4Head() === submittedCL, 'P4HeadChangedBeforePCBReceipt');
  const actions = journal.actions.map((item) => ({ action: item.action, path: item.path,
    depot: `${DEPOT}/${item.path}`, type: item.type,
    bytes: item.action === 'delete' ? null : rawBlob(journal.gitCommit, item.path, item) }));
  verifySubmitted(submittedCL, journal.gitCommit, actions);
  if (journal.state === 'submit-started') {
    journal.submittedCL = submittedCL;
    journal.state = journal.impact === 'binary-build-required' ? 'submitted-await-pcb' : 'submitted-metadata';
    jsonAtomic(join(STATE_ROOT, 'active.json'), journal);
  }
  if (journal.state === 'submitted-metadata') {
    assert(priorPcbMatches(state), 'PriorPCBNotVerified');
    finishTransaction(state, journal, state.codeCL, state.archiveRevision, state.archiveChange);
    report('VerifiedNoSharedChanges', 'RecoveredMetadataSubmission', { GitCommit: journal.gitCommit,
      P4Change: submittedCL, P4Submitted: true });
    return;
  }
  const meta = archiveMeta();
  if (!archiveDescriptionMatches(meta.change, submittedCL)) {
    const buildStatusPath = 'C:\\BuildAgent\\Dev01\\state\\auto_ugs_build_status.json';
    if (existsSync(buildStatusPath)) {
      const buildStatus = JSON.parse(readFileSync(buildStatusPath, 'utf8'));
      if (buildStatus.result === 'failed' && buildStatus.project_cl === submittedCL &&
        buildStatus.code_cl === submittedCL) {
        report('Blocked', 'ProductionPCBFailed', { GitCommit: journal.gitCommit,
          P4Change: submittedCL, CodeCL: submittedCL });
        return;
      }
    }
    if (Date.now() - Date.parse(journal.createdAt) > 2 * 60 * 60 * 1000) {
      report('Blocked', 'ProductionPCBTimeout', { GitCommit: journal.gitCommit,
        P4Change: submittedCL, CodeCL: submittedCL });
      return;
    }
    report('SubmittedAwaitingPCB', 'WaitingForProductionPCBTask', { GitCommit: journal.gitCommit,
      P4Change: submittedCL, CodeCL: submittedCL });
    return;
  }
  assert(meta.rev > state.archiveRevision, 'PCBRevisionDidNotAdvance');
  const verified = p4(['verify', '-q', ARCHIVE]);
  assert(verified.stdout.length === 0 && verified.stderr.length === 0, 'PCBVerifyNotQuiet');
  const build = JSON.parse(readFileSync('C:\\BuildAgent\\Dev01\\state\\completed_build.json', 'utf8'));
  assert(build.status === 'complete' && build.project_cl === submittedCL &&
    build.code_cl === submittedCL && build.engine_cl === state.engineCL &&
    build.engine_build_id === state.buildId, 'BuildReceiptMismatch');
  finishTransaction(state, journal, submittedCL, meta.rev, meta.change);
  report('VerifiedNoSharedChanges', 'SubmittedAndPCBVerified', { GitCommit: journal.gitCommit,
    P4Change: submittedCL, CodeCL: submittedCL, ArchiveRevision: meta.rev,
    P4Submitted: true });
}

export const RECOVERY_CONTEXT = Object.freeze({ HERE, ROOT, GIT, STATE_ROOT, DEPOT, ARCHIVE, USER, CLIENT, POLICY,
  POLICY_PATH: join(HERE, '../v2-readonly/policy.v2.json') });
export { readGit, readP4, readOpened, readChangedPaths, planTerminal, classifyV2, classifyImpact,
  assertClient, assertEngine, p4Head, archiveMeta, archiveDescriptionMatches, priorPcbMatches,
  fetchExact, assertDescendant, stageSubmit, recoveryActionsDigest, gitNetworkArgs, loadState };

export function main(argv = process.argv.slice(2)) {
  assert(argv.length === 1 && ['--dry-run', '--apply'].includes(argv[0]), 'Use --dry-run or --apply');
  const apply = argv[0] === '--apply';
  assert(isAbsolute(ROOT) && isAbsolute(STATE_ROOT), 'UnsafeConfiguredRoot');
  mkdirSync(STATE_ROOT, { recursive: true });
  mkdirSync(join(STATE_ROOT, 'receipts'), { recursive: true });
  const lockPath = join(STATE_ROOT, 'worker.lock');
  let lock;
  try { lock = openSync(lockPath, 'wx', 0o600); }
  catch { fail('WorkerAlreadyRunningOrStaleLock'); }
  try {
    writeSync(lock, Buffer.from(`${process.pid} ${new Date().toISOString()}\n`)); fsyncSync(lock);
    process.env.P4PORT = 'ssl:localhost:1666';
    process.env.P4USER = USER;
    process.env.P4CLIENT = CLIENT;
    process.env.P4TICKETS = 'C:\\BuildAgent\\Dev01\\secrets\\Dev01BuildAgent.ticket';
    process.env.P4TRUST = 'C:\\BuildAgent\\Dev01\\secrets\\p4trust.txt';
    delete process.env.P4CHARSET;
    const state = loadState();
    assertClient();
    assertEngine(state);
    if (existsSync(join(STATE_ROOT, 'active.json'))) {
      if (!apply) {
        report('DryRunReviewable', 'ActiveTransactionRequiresApplyOrReview',
          { GitCommit: state.gitCommit, P4Change: state.p4Change });
        return;
      }
      resumeSubmitted(state, JSON.parse(readFileSync(join(STATE_ROOT, 'active.json'), 'utf8')));
      return;
    }
    assert(priorPcbMatches(state), 'PriorPCBNotVerified');
    const target = fetchExact();
    assertDescendant(state.gitCommit, target);
    const head = p4Head();
    runDryOrIdle(state, target, head, apply);
  } finally {
    closeSync(lock);
    // Exact owned lock only. A crash leaves it for manual examination.
    unlinkSync(lockPath);
  }
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) {
    try {
      const detail = String(error?.message ?? error).slice(0, 4096);
      jsonAtomic(join(STATE_ROOT, 'last-error.json'),
        { Timestamp: new Date().toISOString(), Detail: detail });
      const reason = detail.startsWith('git ') ? 'GitCommandFailed' :
        detail.startsWith('p4 ') ? 'P4CommandFailed' :
        detail.startsWith('PlanHold:') ? 'PlanHold' :
        /^[A-Za-z][A-Za-z0-9]{2,80}$/.test(detail) ? detail : 'AutoWorkerFailedSeeProtectedDetail';
      report('Blocked', reason, { DetailPath: join(STATE_ROOT, 'last-error.json') });
    }
    catch { process.stderr.write(`${error.stack ?? error}\n`); process.exitCode = 2; }
  }
}

