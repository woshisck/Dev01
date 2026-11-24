// Pure validation for a separately reviewed recovery transaction. No state or source-control writes.
import { createHash } from 'node:crypto';

const HEX64 = /^[0-9a-f]{64}$/;
const HEX40 = /^[0-9a-f]{40}$/;
const fail = (condition, reason) => { if (!condition) throw new Error(reason); };
export const sha256 = (bytes) => createHash('sha256').update(bytes).digest('hex');

export function gitNetworkArgs(policy) {
  if (policy.gitHttpProxy === undefined) return [];
  const value = policy.gitHttpProxy;
  const match = typeof value === 'string' && /^http:\/\/(127\.0\.0\.1|localhost):([0-9]{1,5})\/?$/.exec(value);
  fail(match && Number(match[2]) >= 1 && Number(match[2]) <= 65535, 'InvalidGitHttpProxyPolicy');
  return ['-c', `http.proxy=${value}`];
}

export function normalizedRecoveryActions(actions) {
  fail(Array.isArray(actions), 'RecoveryActionsMissing');
  const paths = new Set();
  return actions.map((item) => {
    fail(['add', 'edit', 'delete'].includes(item.action) && typeof item.path === 'string' &&
      item.path.length > 0 && !paths.has(item.path), 'RecoveryActionInvalid');
    paths.add(item.path);
    const plan = Object.hasOwn(item, 'git') || Object.hasOwn(item, 'p4');
    return {
      action: item.action, path: item.path,
      sha256: item.action === 'delete' ? null : plan ? item.git?.sha256 : item.sha256,
      digest: item.action === 'delete' ? null : plan ? item.git?.digest : item.digest,
      size: item.action === 'delete' ? null : plan ? item.git?.size : item.size,
      priorDigest: plan ? item.p4?.digest ?? null : item.priorDigest ?? null,
      priorSize: plan ? item.p4?.size ?? null : item.priorSize ?? null,
      priorRev: plan ? item.p4?.rev ?? null : item.priorRev ?? null,
      priorType: plan ? item.p4?.type ?? null : item.priorType ?? null,
    };
  }).sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
}

export function recoveryActionsDigest(actions) {
  return sha256(Buffer.from(JSON.stringify(normalizedRecoveryActions(actions)), 'utf8'));
}

export function normalizedOpenedRecord(row) {
  return { path: row.path, user: row.user, client: row.client,
    change: String(row.change), action: row.action, type: row.type,
    rev: row.rev == null ? null : String(row.rev), haveRev: row.haveRev == null ? null : String(row.haveRev),
    serverDigest: row.serverDigest, serverSize: row.serverSize,
    serverRev: row.serverRev, serverType: row.serverType };
}

export function validateApprovedOpenedOverlaps(approved, actual) {
  fail(Array.isArray(approved) && Array.isArray(actual), 'RecoveryOpenedApprovalMissing');
  for (const row of approved) {
    fail(typeof row.path === 'string' && row.user && row.client && row.action === 'edit' &&
      /^(?:text|unicode|utf8|utf16)(?:\+[A-Za-z0-9]+)?$/i.test(row.type ?? '') &&
      !/\+.*l/i.test(row.type) && row.serverType === row.type &&
      /^[0-9a-f]{32}$/.test(row.serverDigest ?? '') && Number.isSafeInteger(row.serverSize) &&
      Number.isSafeInteger(row.serverRev) && row.serverRev > 0 && /^(?:\d+|default)$/.test(String(row.change)),
    'RecoveryOpenedApprovalDisallowed');
  }
  const canonical = (rows) => rows.map(normalizedOpenedRecord)
    .map((row) => JSON.stringify(row)).sort();
  fail(new Set(canonical(approved)).size === approved.length, 'RecoveryOpenedApprovalDuplicate');
  fail(JSON.stringify(canonical(approved)) === JSON.stringify(canonical(actual)), 'RecoveryOpenedOwnershipChanged');
  return true;
}

export function validateRecoveryReview({ state, checkpointBytes, policyBytes, reviewBytes, evidence, journal, actions }) {
  fail(evidence?.schema === 1 && HEX64.test(evidence.reviewSha256 ?? ''), 'RecoveryEvidenceInvalid');
  fail(sha256(reviewBytes) === evidence.reviewSha256, 'RecoveryReviewHashMismatch');
  const review = JSON.parse(reviewBytes.toString('utf8'));
  const pins = review.pins;
  fail(review.schema === 1 && review.purpose === 'Dev01ReviewedReconciliation' && pins, 'RecoveryReviewSchemaInvalid');
  for (const key of ['oldCheckpointSha256', 'priorPolicySha256', 'nextPolicySha256', 'actionsSha256']) {
    fail(HEX64.test(pins[key] ?? ''), `RecoveryPinInvalid:${key}`);
  }
  fail(HEX40.test(pins.targetGitCommit ?? ''), 'RecoveryGitPinInvalid');
  fail(sha256(checkpointBytes) === pins.oldCheckpointSha256, 'RecoveryCheckpointChanged');
  fail(state.policySha256 === pins.priorPolicySha256, 'RecoveryPriorPolicyMismatch');
  fail(sha256(policyBytes) === pins.nextPolicySha256, 'RecoveryNextPolicyMismatch');
  fail(pins.engineCL === state.engineCL && pins.buildId === state.buildId, 'RecoveryEngineMismatch');
  for (const key of ['baseP4Head', 'archiveRevision', 'archiveChange', 'codeCL']) {
    fail(Number.isSafeInteger(pins[key]) && pins[key] > 0, `RecoveryPinInvalid:${key}`);
  }
  fail(pins.baseP4Head >= state.p4Change && pins.codeCL === pins.baseP4Head &&
    pins.archiveRevision >= state.archiveRevision && pins.archiveChange > pins.codeCL,
  'RecoveryExternalReleaseIdentityInvalid');
  fail(Array.isArray(review.approvedDeletes) && new Set(review.approvedDeletes).size === review.approvedDeletes.length,
    'RecoveryDeleteApprovalInvalid');
  if (journal) {
    fail(journal.schema === 1 && journal.priorGitCommit === state.gitCommit &&
      journal.gitCommit === pins.targetGitCommit && journal.baseP4Head === pins.baseP4Head &&
      journal.engineCL === pins.engineCL && journal.buildId === pins.buildId &&
      journal.impact === 'binary-build-required' && journal.recoveryEvidence?.reviewSha256 === evidence.reviewSha256,
    'RecoveryJournalIdentityMismatch');
    actions = journal.actions;
  }
  fail(actions && recoveryActionsDigest(actions) === pins.actionsSha256, 'RecoveryActionReviewMismatch');
  const actionRecords = new Map(normalizedRecoveryActions(actions).map((item) => [item.path, item]));
  const overlaps = review.approvedOpenedOverlaps ?? [];
  validateApprovedOpenedOverlaps(overlaps, overlaps);
  for (const row of overlaps) {
    const item = actionRecords.get(row.path);
    if (item?.action === 'delete') fail(review.approvedDeletes.includes(row.path), 'RecoveryOpenedDeleteRequiresReview');
    fail(item && ['edit', 'delete'].includes(item.action) && row.serverDigest === item.priorDigest &&
      row.serverSize === item.priorSize && row.serverRev === item.priorRev && row.serverType === item.priorType,
    'RecoveryOpenedPreimageMismatch');
  }
  const deletes = actions.filter((item) => item.action === 'delete').map((item) => item.path).sort();
  fail(JSON.stringify(deletes) === JSON.stringify([...review.approvedDeletes].sort()), 'RecoveryDeleteApprovalMismatch');
  return review;
}
