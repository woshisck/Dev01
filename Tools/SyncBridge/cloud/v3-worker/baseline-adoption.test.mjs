import assert from 'node:assert/strict';
import test from 'node:test';
import {buildAdoptionProposal,digest,validateAdoptionReview} from './baseline-adoption-core.mjs';

function fixture(){
  const state={schema:3,status:'active',repository:'https://github.com/woshisck/Dev01.git',stream:'//Dev01/main',archive:'//Dev01Binaries/UGS/++Dev01+main-Editor.zip',gitCommit:'1'.repeat(40),policySha256:'2'.repeat(64),p4Change:173,codeCL:170,engineCL:160,buildId:'521bf6ff-a998-43fe-962c-978d9a1e7d5e',archiveRevision:36,archiveChange:171};
  const checkpointBytes=Buffer.from(JSON.stringify(state));
  const entries=new Map([['Source/Test.cpp',{digest:'a'.repeat(32),size:10}]]);
  return {checkpointBytes,expectedCheckpointSha256:digest(checkpointBytes),expectedPriorPolicySha256:state.policySha256,
    repository:state.repository,stream:state.stream,archive:state.archive,targetGitCommit:'3'.repeat(40),advertisedGitCommit:'3'.repeat(40),
    gitHead:'3'.repeat(40),gitBranch:'main',gitDirty:false,gitDescendant:true,activeExists:false,clientClean:true,
    engineImportCL:160,engineBuildId:state.buildId,plan:{status:'NO_OP',actions:[],holds:[]},gitViolations:[],p4Violations:[],
    gitEntries:entries,p4Entries:structuredClone(entries),p4Head:179,archiveMeta:{rev:37,change:182,size:481516166,digest:'b'.repeat(32)},
    archiveDescriptionMatches:true,priorArchiveDescriptionMatches:true,verifications:[36,37].map(revision=>({revision,exitCode:0,stdout:'',stderr:''})),
    buildReceiptBytes:Buffer.from(JSON.stringify({status:'complete',project_cl:179,code_cl:179,engine_cl:160,engine_build_id:state.buildId})),
    policyBytes:Buffer.from('{"exactPaths":[]}'),implementationSha256:{worker:'c'.repeat(64)}};
}
test('byte parity and separately verified external release produce a no-source-write proposal',()=>{const plan=buildAdoptionProposal(fixture());assert.equal(plan.pins.p4Head,179);assert.equal(plan.sourceWrites,false);assert.equal(plan.p4Submissions,false);});
test('no-op adoption refuses source differences even if a caller labels its plan NO_OP',()=>{const f=fixture();f.p4Entries.get('Source/Test.cpp').digest='d'.repeat(32);assert.throws(()=>buildAdoptionProposal(f),/AdoptionSourceTreesNotEqual/);});
test('source actions cannot be hidden in an adoption',()=>{const f=fixture();f.plan.actions=[{action:'delete',path:'Source/Test.cpp'}];assert.throws(()=>buildAdoptionProposal(f),/AdoptionSourceTreesNotEqual/);});
test('prior and current archive verification must both be fresh successful quiet commands',()=>{const f=fixture();f.verifications.pop();assert.throws(()=>buildAdoptionProposal(f),/AdoptionArchiveVerifyFailed/);});
test('the build receipt must identify the current submitted code head and engine',()=>{const f=fixture();f.buildReceiptBytes=Buffer.from('{"status":"complete","project_cl":170,"code_cl":170}');assert.throws(()=>buildAdoptionProposal(f),/AdoptionBuildReceiptMismatch/);});
test('a dirty or different Git checkout cannot be adopted',()=>{const f=fixture();f.gitDirty=true;assert.throws(()=>buildAdoptionProposal(f),/AdoptionGitIdentityMismatch/);});
test('an active transaction cannot be superseded by adoption',()=>{const f=fixture();f.activeExists=true;assert.throws(()=>buildAdoptionProposal(f),/AdoptionClientOrEngineChanged/);});
test('apply re-capture rejects a changed archive, tree, policy or code implementation pin',()=>{const f=fixture();const plan=buildAdoptionProposal(f),bytes=Buffer.from(JSON.stringify(plan));assert.doesNotThrow(()=>validateAdoptionReview(bytes,digest(bytes),buildAdoptionProposal(f)));const other=structuredClone(plan);other.pins.implementationSha256.worker='d'.repeat(64);assert.throws(()=>validateAdoptionReview(bytes,digest(bytes),other),/AdoptionLiveEvidenceChanged/);assert.throws(()=>validateAdoptionReview(bytes,'0'.repeat(64),plan),/AdoptionReviewHashMismatch/);});
