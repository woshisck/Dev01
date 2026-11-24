// A no-source-write adoption is allowed only after independently collected live parity and release evidence.
import { createHash } from 'node:crypto';
const assert = (ok, reason) => { if (!ok) throw new Error(reason); };
export const digest = (bytes) => createHash('sha256').update(bytes).digest('hex');
const SHA40 = /^[0-9a-f]{40}$/;
const SHA64 = /^[0-9a-f]{64}$/;

export function treeDigest(entries) {
  const rows = [...entries].map(([path, value]) => ({path, digest:value.digest.toLowerCase(), size:value.size}))
    .sort((a,b)=>a.path<b.path?-1:a.path>b.path?1:0);
  assert(rows.every((row)=>/^[0-9a-f]{32}$/.test(row.digest) && Number.isSafeInteger(row.size) && row.size>=0), 'AdoptionTreeEntryInvalid');
  return digest(Buffer.from(JSON.stringify(rows)));
}

export function buildAdoptionProposal(observation) {
  const o=observation;
  const state=JSON.parse(o.checkpointBytes.toString('utf8'));
  const build=JSON.parse(o.buildReceiptBytes.toString('utf8'));
  assert(state.schema===3 && state.status==='active' && SHA40.test(state.gitCommit??'') &&
    SHA64.test(state.policySha256??''), 'AdoptionCheckpointInvalid');
  assert(digest(o.checkpointBytes)===o.expectedCheckpointSha256 &&
    state.policySha256===o.expectedPriorPolicySha256, 'AdoptionCheckpointChanged');
  assert(o.repository===state.repository && o.stream===state.stream && o.archive===state.archive,
    'AdoptionSourceIdentityMismatch');
  assert(SHA40.test(o.targetGitCommit??'') && o.targetGitCommit===o.advertisedGitCommit &&
    o.gitHead===o.targetGitCommit && o.gitBranch==='main' && o.gitDirty===false && o.gitDescendant===true,
  'AdoptionGitIdentityMismatch');
  assert(o.activeExists===false && o.clientClean===true && o.engineImportCL===state.engineCL &&
    o.engineBuildId===state.buildId, 'AdoptionClientOrEngineChanged');
  assert(o.plan.status==='NO_OP' && o.plan.actions.length===0 && o.plan.holds.length===0 &&
    o.gitViolations.length===0 && o.p4Violations.length===0 && o.gitEntries.size>0 &&
    o.gitEntries.size===o.p4Entries.size && treeDigest(o.gitEntries)===treeDigest(o.p4Entries),
  'AdoptionSourceTreesNotEqual');
  assert(Number.isSafeInteger(o.p4Head) && o.p4Head>=state.p4Change &&
    o.archiveMeta.rev>=state.archiveRevision && o.archiveMeta.change>o.p4Head &&
    o.archiveDescriptionMatches===true && o.priorArchiveDescriptionMatches===true,
  'AdoptionArchiveIdentityMismatch');
  assert(o.verifications.length===new Set([state.archiveRevision,o.archiveMeta.rev]).size &&
    [...new Set([state.archiveRevision,o.archiveMeta.rev])].every((rev)=>o.verifications.some((v)=>
      v.revision===rev && v.exitCode===0 && v.stdout==='' && v.stderr==='')),
  'AdoptionArchiveVerifyFailed');
  assert(build.status==='complete' && build.project_cl===o.p4Head && build.code_cl===o.p4Head &&
    build.engine_cl===state.engineCL && build.engine_build_id===state.buildId,
  'AdoptionBuildReceiptMismatch');
  assert(Number.isSafeInteger(o.archiveMeta.size) && o.archiveMeta.size>0 &&
    /^[0-9a-f]{32}$/.test(o.archiveMeta.digest??''), 'AdoptionArchiveDigestMissing');
  const pins={oldCheckpointSha256:digest(o.checkpointBytes),priorPolicySha256:state.policySha256,
    nextPolicySha256:digest(o.policyBytes),targetGitCommit:o.targetGitCommit,p4Head:o.p4Head,
    codeCL:o.p4Head,engineCL:state.engineCL,buildId:state.buildId,archiveRevision:o.archiveMeta.rev,
    archiveChange:o.archiveMeta.change,archiveDigest:o.archiveMeta.digest,archiveSize:o.archiveMeta.size,
    sourceTreeSha256:treeDigest(o.gitEntries),sourceFileCount:o.gitEntries.size,
    buildReceiptSha256:digest(o.buildReceiptBytes),implementationSha256:o.implementationSha256};
  return {schema:1,purpose:'Dev01VerifiedBaselineAdoption',pins,
    checks:{sourceParity:true,noActiveTransaction:true,cleanIsolatedClient:true,engineIdentity:true,
      priorAndCurrentArchiveVerified:true,completedBuildReceipt:true},
    sourceWrites:false,p4Submissions:false};
}

export function validateAdoptionReview(reviewBytes, expectedSha256, freshProposal) {
  assert(SHA64.test(expectedSha256??'') && digest(reviewBytes)===expectedSha256, 'AdoptionReviewHashMismatch');
  const review=JSON.parse(reviewBytes.toString('utf8'));
  assert(review.schema===1 && review.purpose==='Dev01VerifiedBaselineAdoption' &&
    JSON.stringify(review.pins)===JSON.stringify(freshProposal.pins) && review.sourceWrites===false &&
    review.p4Submissions===false, 'AdoptionLiveEvidenceChanged');
  return review;
}
