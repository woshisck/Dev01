// Explicit, pinned recovery of an externally advanced Dev01 release.
// Normal automatic sync keeps its original guards; this command creates a real
// reviewed transaction and uses the normal shelf/submit/build/PCB completion path.
import { createHash, randomUUID } from 'node:crypto';
import { closeSync, existsSync, fsyncSync, mkdirSync, openSync, readFileSync, unlinkSync, writeFileSync, writeSync } from 'node:fs';
import { dirname, isAbsolute, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  RECOVERY_CONTEXT as C, archiveDescriptionMatches, archiveMeta, assertClient,
  assertDescendant, assertEngine, fetchExact, p4Head, planTerminal, readChangedPaths,
  readCheckpointForRecovery, readGit, readOpened, readOpenedForRecovery, readP4,
  recoveryActionsDigest, stageSubmit,
} from './auto-sync.mjs';
import { validateApprovedOpenedOverlaps, validateRecoveryReview } from './recovery-evidence.mjs';

const hash = (bytes) => createHash('sha256').update(bytes).digest('hex');
const requireThat = (ok, why) => { if (!ok) throw new Error(why); };
const sha40 = /^[0-9a-f]{40}$/;
const sha64 = /^[0-9a-f]{64}$/;

export function parseArgs(argv) {
  if (argv.length === 1 && argv[0] === '--help') return { help: true };
  const out = {};
  const keys = new Set(['mode', 'target', 'checkpoint-sha', 'old-policy-sha', 'out', 'review', 'review-sha']);
  requireThat(argv.length % 2 === 0, 'ArgumentsMustBeExplicitPairs');
  for (let i = 0; i < argv.length; i += 2) {
    const key = argv[i]?.slice(2);
    requireThat(argv[i]?.startsWith('--') && keys.has(key) && argv[i + 1] && !Object.hasOwn(out, key), 'UnexpectedOrDuplicateArgument');
    out[key] = argv[i + 1];
  }
  requireThat(['plan', 'apply'].includes(out.mode), 'UseModePlanOrApply');
  const expected = out.mode === 'plan'
    ? ['mode', 'target', 'checkpoint-sha', 'old-policy-sha', 'out']
    : ['mode', 'review', 'review-sha'];
  requireThat(Object.keys(out).length === expected.length && expected.every((key) => Object.hasOwn(out, key)), 'ModeArgumentMismatch');
  if (out.mode === 'plan') {
    requireThat(sha40.test(out.target) && sha64.test(out['checkpoint-sha']) && sha64.test(out['old-policy-sha']), 'InvalidPlanPins');
    requireThat(isAbsolute(out.out), 'PlanOutputMustBeAbsolute');
  } else requireThat(isAbsolute(out.review) && sha64.test(out['review-sha']), 'InvalidReviewPins');
  return out;
}

export function assertReviewablePlan(plan) {
  requireThat(Array.isArray(plan.actions) && plan.actions.length > 0 && plan.actions.length <= 10000, 'RecoveryActionCountInvalid');
  requireThat(plan.impact === 'binary-build-required', 'RecoveryRequiresNormalBuildAndPCB');
  const unexpected = plan.holds.filter((item) => item.reason !== 'opened-overlap');
  requireThat(unexpected.length === 0, `RecoveryPlanHold:${JSON.stringify(unexpected.slice(0, 12))}`);
  return plan;
}

function configureP4() {
  process.env.P4PORT = 'ssl:localhost:1666';
  process.env.P4USER = C.USER;
  process.env.P4CLIENT = C.CLIENT;
  process.env.P4TICKETS = 'C:\\BuildAgent\\Dev01\\secrets\\Dev01BuildAgent.ticket';
  process.env.P4TRUST = 'C:\\BuildAgent\\Dev01\\secrets\\p4trust.txt';
  delete process.env.P4CHARSET;
}

function locked(fn) {
  requireThat(isAbsolute(C.STATE_ROOT), 'UnsafeStateRoot');
  const lockPath = join(C.STATE_ROOT, 'worker.lock');
  const owner = `${process.pid} reviewed-recovery ${randomUUID()}\n`;
  let fd;
  try { fd = openSync(lockPath, 'wx', 0o600); }
  catch { throw new Error('WorkerAlreadyRunningOrStaleLock'); }
  try {
    writeSync(fd, Buffer.from(owner)); fsyncSync(fd);
    configureP4();
    requireThat(!existsSync(join(C.STATE_ROOT, 'active.json')), 'PriorTransactionRequiresReview');
    return fn();
  } finally {
    closeSync(fd);
    requireThat(readFileSync(lockPath, 'utf8') === owner, 'RecoveryLockOwnershipChanged');
    unlinkSync(lockPath);
  }
}

function livePlan(state, target) {
  assertClient(); assertEngine(state);
  requireThat(fetchExact() === target, 'RecoveryTargetIsNotAdvertisedMain');
  assertDescendant(state.gitCommit, target);
  const head = p4Head();
  const git = readGit(C.GIT, target, C.POLICY);
  const p4 = readP4(C.ROOT, C.DEPOT);
  requireThat(!git.violations.length && !p4.violations.length, 'RecoveryTreeInvalid');
  const changedPaths = readChangedPaths(C.GIT, state.gitCommit, target);
  const openedPaths = readOpened(C.ROOT, C.DEPOT);
  const plan = assertReviewablePlan(planTerminal({ gitEntries: git.entries, p4Entries: p4.entries, changedPaths, openedPaths }));
  const overlaps = readOpenedForRecovery(plan.actions);
  const archive = archiveMeta();
  const build = JSON.parse(readFileSync('C:\\BuildAgent\\Dev01\\state\\completed_build.json', 'utf8'));
  requireThat(build.status === 'complete' && build.project_cl === head && build.code_cl === head &&
    build.engine_cl === state.engineCL && build.engine_build_id === state.buildId, 'RecoveryExternalBuildReceiptMismatch');
  requireThat(archiveDescriptionMatches(archive.change, build.code_cl), 'RecoveryExternalPCBIdentityMismatch');
  requireThat(p4Head() === head, 'P4HeadMovedDuringRecoveryPlan');
  return { head, plan, overlaps, archive, build, gitEntries: git.entries.size, p4Entries: p4.entries.size,
    changedPathCount: changedPaths.length, openedCount: openedPaths.length };
}

function createOnly(path, bytes) {
  mkdirSync(dirname(path), { recursive: true });
  const fd = openSync(path, 'wx', 0o600);
  try { writeSync(fd, bytes); fsyncSync(fd); } finally { closeSync(fd); }
}

export function counts(actions) {
  return Object.fromEntries(['add', 'edit', 'delete'].map((action) => [action, actions.filter((item) => item.action === action).length]));
}

export function main(argv = process.argv.slice(2)) {
  const args = parseArgs(argv);
  if (args.help) {
    process.stdout.write('Plan: --mode plan --target SHA --checkpoint-sha SHA256 --old-policy-sha SHA256 --out NEW_ABSOLUTE_FILE\nApply: --mode apply --review ABSOLUTE_FILE --review-sha SHA256\nPlan never approves deletes or overlapping work automatically.\n');
    return;
  }
  return locked(() => {
    if (args.mode === 'plan') {
      const state = readCheckpointForRecovery(args['checkpoint-sha'], args['old-policy-sha']);
      const live = livePlan(state, args.target);
      const review = {
        schema: 1, purpose: 'Dev01ReviewedReconciliation', createdAt: new Date().toISOString(),
        pins: { oldCheckpointSha256: args['checkpoint-sha'], priorPolicySha256: args['old-policy-sha'],
          nextPolicySha256: hash(readFileSync(C.POLICY_PATH)), targetGitCommit: args.target,
          baseP4Head: live.head, engineCL: state.engineCL, buildId: state.buildId,
          archiveRevision: live.archive.rev, archiveChange: live.archive.change, codeCL: live.build.code_cl,
          actionsSha256: recoveryActionsDigest(live.plan.actions) },
        approvedDeletes: [], approvedOpenedOverlaps: [],
        proposedDeletes: live.plan.actions.filter((item) => item.action === 'delete').map((item) => item.path),
        proposedOpenedOverlaps: live.overlaps, actions: live.plan.actions,
        summary: { ...counts(live.plan.actions), impact: live.plan.impact,
          gitManaged: live.gitEntries, p4Managed: live.p4Entries, changedPathCount: live.changedPathCount,
          openedCount: live.openedCount, protectedSkippedCount: live.plan.skipped.length },
      };
      const bytes = Buffer.from(JSON.stringify(review, null, 2) + '\n');
      createOnly(args.out, bytes);
      process.stdout.write(JSON.stringify({ Status: 'RecoveryPlanRequiresReview', Review: args.out,
        ReviewSha256: hash(bytes), ...review.summary, ProposedOpenedOverlaps: live.overlaps.length }) + '\n');
      return;
    }
    const bytes = readFileSync(args.review);
    requireThat(hash(bytes) === args['review-sha'], 'RecoveryReviewHashMismatch');
    const review = JSON.parse(bytes.toString('utf8'));
    const state = readCheckpointForRecovery(review.pins?.oldCheckpointSha256, review.pins?.priorPolicySha256);
    const live = livePlan(state, review.pins.targetGitCommit);
    requireThat(live.head === review.pins.baseP4Head && live.archive.rev === review.pins.archiveRevision &&
      live.archive.change === review.pins.archiveChange && live.build.code_cl === review.pins.codeCL, 'RecoveryReleasePinChanged');
    validateApprovedOpenedOverlaps(review.approvedOpenedOverlaps, live.overlaps);
    const evidence = { schema: 1, reviewSha256: args['review-sha'] };
    validateRecoveryReview({ state, checkpointBytes: readFileSync(join(C.STATE_ROOT, 'checkpoint.json')),
      policyBytes: readFileSync(C.POLICY_PATH), reviewBytes: bytes, evidence, actions: live.plan.actions });
    const approvedPaths = new Set(review.approvedOpenedOverlaps.map((row) => row.path));
    requireThat(live.plan.holds.every((item) => item.reason === 'opened-overlap' && approvedPaths.has(item.path)), 'RecoveryUnapprovedHold');
    const plan = { ...live.plan, status: 'PLAN', holds: [] };
    const directory = join(C.STATE_ROOT, 'receipts', 'recovery-reviews');
    const storedReview = join(directory, `${evidence.reviewSha256}.json`);
    if (existsSync(storedReview)) requireThat(readFileSync(storedReview).equals(bytes), 'StoredRecoveryReviewChanged');
    else createOnly(storedReview, bytes);
    const checkpointBackup = join(directory, `${evidence.reviewSha256}.checkpoint-before.json`);
    const checkpointBytes = readFileSync(join(C.STATE_ROOT, 'checkpoint.json'));
    if (existsSync(checkpointBackup)) requireThat(readFileSync(checkpointBackup).equals(checkpointBytes), 'CheckpointBackupChanged');
    else createOnly(checkpointBackup, checkpointBytes);
    stageSubmit(state, review.pins.targetGitCommit, live.head, plan, evidence);
  });
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) {
    process.stderr.write(`Reviewed recovery stopped: ${error.message}\n`);
    process.exitCode = 2;
  }
}
