// An explicit, reviewed recovery for an already converged external release.
// Does not reconcile, sync, submit, build, publish, or manufacture an active transaction.
import { spawnSync } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { closeSync, existsSync, fsyncSync, mkdirSync, openSync, readFileSync, renameSync, unlinkSync, writeFileSync, writeSync } from 'node:fs';
import { dirname, isAbsolute, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { RECOVERY_CONTEXT as C, readCheckpointForRecovery, assertClient, assertEngine,
  readGit, readP4, readOpened, readChangedPaths, planTerminal, assertDescendant,
  p4Head, archiveMeta, archiveDescriptionMatches, gitNetworkArgs } from './auto-sync.mjs';
import { parseP4Tags } from '../v2-readonly/read-live.mjs';
import { buildAdoptionProposal, digest, validateAdoptionReview } from './baseline-adoption-core.mjs';

const assert=(ok,reason)=>{if(!ok)throw new Error(reason);};
const HERE=dirname(fileURLToPath(import.meta.url));
function command(binary,args,cwd=C.ROOT){
  const r=spawnSync(binary,args,{cwd,encoding:null,timeout:120000,maxBuffer:256*1024*1024,windowsHide:true,
    env:{...process.env,GIT_TERMINAL_PROMPT:'0',GCM_INTERACTIVE:'never'}});
  assert(!r.error && r.status===0,`${binary} adoption read command failed: ${r.error?.message??r.stderr?.toString('utf8').slice(0,300)}`);
  return r;
}
const git=(args)=>command('git',[...gitNetworkArgs(C.POLICY),'-C',C.GIT,...args],C.GIT).stdout.toString('utf8').trim();
const p4=(args)=>command('p4',['-u',C.USER,'-c',C.CLIENT,...args]);
function initializeEnvironment(){
  process.env.P4PORT='ssl:localhost:1666';process.env.P4USER=C.USER;process.env.P4CLIENT=C.CLIENT;
  process.env.P4TICKETS='C:\\BuildAgent\\Dev01\\secrets\\Dev01BuildAgent.ticket';
  process.env.P4TRUST='C:\\BuildAgent\\Dev01\\secrets\\p4trust.txt';delete process.env.P4CHARSET;
}
function implementationDigest(){
  const files=['auto-sync.mjs','recovery-evidence.mjs','baseline-adoption-core.mjs','baseline-adoption.mjs',
    '../v2-readonly/planner.mjs','../v2-readonly/read-live.mjs','../strict-mirror-planner/strict-plan.mjs'];
  return Object.fromEntries(files.map((path)=>[path,digest(readFileSync(join(HERE,path)))]));
}
function capture(targetGitCommit,oldCheckpointSha,oldPolicySha){
  const state=readCheckpointForRecovery(oldCheckpointSha,oldPolicySha);
  assert(!existsSync(join(C.STATE_ROOT,'active.json')),'AdoptionActiveTransactionExists');
  assertClient();assertEngine(state);
  const head=p4Head();
  const advertised=git(['ls-remote','origin','refs/heads/main']);
  const remote=/^([0-9a-f]{40})\s+refs\/heads\/main$/.exec(advertised)?.[1];
  assert(git(['remote','get-url','origin'])===state.repository,'AdoptionGitOriginMismatch');
  assertDescendant(state.gitCommit,targetGitCommit);
  const g=readGit(C.GIT,targetGitCommit,C.POLICY),p=readP4(C.ROOT,C.DEPOT);
  const plan=planTerminal({gitEntries:g.entries,p4Entries:p.entries,
    changedPaths:readChangedPaths(C.GIT,state.gitCommit,targetGitCommit),openedPaths:readOpened(C.ROOT,C.DEPOT)});
  const meta=archiveMeta();
  const item=parseP4Tags(p4(['-ztag','fstat','-Ol','-T','depotFile,digest,fileSize',C.ARCHIVE]).stdout)
    .find((row)=>row.depotFile===C.ARCHIVE);
  assert(item,'AdoptionArchiveMetadataMissing');
  const verifications=[];
  for(const revision of new Set([state.archiveRevision,meta.rev])){
    const verified=p4(['verify','-q',`${C.ARCHIVE}#${revision}`]);
    verifications.push({revision,exitCode:verified.status,stdout:verified.stdout.toString('utf8'),stderr:verified.stderr.toString('utf8')});
  }
  const next=archiveMeta();
  assert(p4Head()===head && next.rev===meta.rev && next.change===meta.change &&
    git(['ls-remote','origin','refs/heads/main'])===advertised,'AdoptionSourceMovedDuringCapture');
  return buildAdoptionProposal({checkpointBytes:readFileSync(join(C.STATE_ROOT,'checkpoint.json')),
    expectedCheckpointSha256:oldCheckpointSha,expectedPriorPolicySha256:oldPolicySha,
    repository:state.repository,stream:C.DEPOT,archive:C.ARCHIVE,targetGitCommit,advertisedGitCommit:remote,
    gitHead:git(['rev-parse','HEAD']),gitBranch:git(['branch','--show-current']),gitDirty:!!git(['status','--porcelain=v1']),
    gitDescendant:true,activeExists:false,clientClean:true,engineImportCL:state.engineCL,
    engineBuildId:JSON.parse(readFileSync('C:\\Project\\Dev01-P4\\Engine\\Binaries\\Win64\\UnrealEditor.modules','utf8')).BuildId,
    plan,gitEntries:g.entries,p4Entries:p.entries,gitViolations:g.violations,p4Violations:p.violations,p4Head:head,
    archiveMeta:{...meta,digest:item.digest?.toLowerCase(),size:Number(item.fileSize)},
    archiveDescriptionMatches:archiveDescriptionMatches(meta.change,head),
    priorArchiveDescriptionMatches:archiveDescriptionMatches(state.archiveChange,state.codeCL),verifications,
    buildReceiptBytes:readFileSync('C:\\BuildAgent\\Dev01\\state\\completed_build.json'),
    policyBytes:readFileSync(C.POLICY_PATH),implementationSha256:implementationDigest()});
}
function atomicJson(path,data){
  const temp=`${path}.tmp-${randomUUID()}`;const fd=openSync(temp,'wx',0o600);
  try{writeSync(fd,Buffer.from(`${JSON.stringify(data,null,2)}\n`));fsyncSync(fd);}finally{closeSync(fd);}
  renameSync(temp,path);
}
function parse(args){
  assert(args[0]==='--plan'||args[0]==='--apply','Use --plan or --apply');
  const out={mode:args[0]};
  for(let i=1;i<args.length;i+=2){assert(/^--[a-z-]+$/.test(args[i]??'')&&args[i+1]&&!Object.hasOwn(out,args[i].slice(2)),'InvalidAdoptionArguments');out[args[i].slice(2)]=args[i+1];}
  const keys=Object.keys(out).filter(k=>k!=='mode').sort();
  const required=out.mode==='--plan'?['checkpoint-sha','git','old-policy-sha','out']:['expect-plan-sha','review'];
  assert(JSON.stringify(keys)===JSON.stringify(required.sort()),'InvalidAdoptionArguments');return out;
}
export function main(args=process.argv.slice(2)){
  const a=parse(args);initializeEnvironment();
  const lockPath=join(C.STATE_ROOT,'worker.lock');let lock;
  try{lock=openSync(lockPath,'wx',0o600);}catch{throw new Error('WorkerAlreadyRunningOrStaleLock');}
  try{
    writeSync(lock,Buffer.from(`${process.pid} verified-baseline-adoption ${new Date().toISOString()}\n`));fsyncSync(lock);
    if(a.mode==='--plan'){
      assert(isAbsolute(a.out)&&!existsSync(a.out),'AdoptionReviewOutputMustBeNewAbsolutePath');
      const proposal=capture(a.git,a['checkpoint-sha'],a['old-policy-sha']);
      const old=JSON.parse(readFileSync(join(C.STATE_ROOT,'checkpoint.json'),'utf8'));
      proposal.proposedCheckpoint={...old,policySha256:proposal.pins.nextPolicySha256,gitCommit:proposal.pins.targetGitCommit,
        p4Change:proposal.pins.p4Head,codeCL:proposal.pins.codeCL,archiveRevision:proposal.pins.archiveRevision,
        archiveChange:proposal.pins.archiveChange,lastDisposition:'verified-external-baseline-adoption',updatedAt:new Date().toISOString()};
      mkdirSync(dirname(a.out),{recursive:true});writeFileSync(a.out,`${JSON.stringify(proposal,null,2)}\n`,{flag:'wx'});
      process.stdout.write(`${JSON.stringify({Status:'AdoptionReviewable',Review:a.out,ReviewSha256:digest(readFileSync(a.out)),SourceWrites:false})}\n`);return;
    }
    assert(isAbsolute(a.review),'AdoptionReviewPathMustBeAbsolute');
    const reviewBytes=readFileSync(a.review);assert(digest(reviewBytes)===a['expect-plan-sha'],'AdoptionReviewHashMismatch');
    const review=JSON.parse(reviewBytes.toString('utf8'));
    const proposal=capture(review.pins.targetGitCommit,review.pins.oldCheckpointSha256,review.pins.priorPolicySha256);
    validateAdoptionReview(reviewBytes,a['expect-plan-sha'],proposal);
    assert(review.proposedCheckpoint?.gitCommit===proposal.pins.targetGitCommit &&
      review.proposedCheckpoint.policySha256===proposal.pins.nextPolicySha256 &&
      review.proposedCheckpoint.p4Change===proposal.pins.p4Head && review.proposedCheckpoint.codeCL===proposal.pins.codeCL &&
      review.proposedCheckpoint.archiveRevision===proposal.pins.archiveRevision &&
      review.proposedCheckpoint.archiveChange===proposal.pins.archiveChange &&
      review.proposedCheckpoint.engineCL===proposal.pins.engineCL && review.proposedCheckpoint.buildId===proposal.pins.buildId,
    'AdoptionProposedCheckpointMismatch');
    const checkpointPath=join(C.STATE_ROOT,'checkpoint.json');const oldBytes=readFileSync(checkpointPath);
    assert(digest(oldBytes)===proposal.pins.oldCheckpointSha256,'AdoptionCheckpointChangedBeforeCommit');
    const expectedCheckpoint={...JSON.parse(oldBytes.toString('utf8')),policySha256:proposal.pins.nextPolicySha256,
      gitCommit:proposal.pins.targetGitCommit,p4Change:proposal.pins.p4Head,codeCL:proposal.pins.codeCL,
      archiveRevision:proposal.pins.archiveRevision,archiveChange:proposal.pins.archiveChange,
      lastDisposition:'verified-external-baseline-adoption',updatedAt:review.proposedCheckpoint.updatedAt};
    assert(Number.isFinite(Date.parse(expectedCheckpoint.updatedAt)) &&
      JSON.stringify(expectedCheckpoint)===JSON.stringify(review.proposedCheckpoint),'AdoptionCheckpointFieldsChanged');
    const receiptDir=join(C.STATE_ROOT,'receipts','baseline-adoptions');mkdirSync(receiptDir,{recursive:true});
    const backup=join(receiptDir,`${proposal.pins.oldCheckpointSha256}.checkpoint.json`);
    if(existsSync(backup))assert(digest(readFileSync(backup))===digest(oldBytes),'AdoptionBackupConflict');
    else writeFileSync(backup,oldBytes,{flag:'wx'});
    const receiptPath=join(receiptDir,`${a['expect-plan-sha']}.json`);assert(!existsSync(receiptPath),'AdoptionReceiptAlreadyExists');
    const receipt={schema:1,purpose:review.purpose,state:'prepared',reviewSha256:a['expect-plan-sha'],pins:proposal.pins,
      oldCheckpointBackup:backup,proposedCheckpoint:review.proposedCheckpoint,createdAt:new Date().toISOString()};
    writeFileSync(receiptPath,`${JSON.stringify(receipt,null,2)}\n`,{flag:'wx'});
    const currentArchive=archiveMeta();
    assert(digest(readFileSync(checkpointPath))===proposal.pins.oldCheckpointSha256 && p4Head()===proposal.pins.p4Head &&
      currentArchive.rev===proposal.pins.archiveRevision && currentArchive.change===proposal.pins.archiveChange &&
      !existsSync(join(C.STATE_ROOT,'active.json')),'AdoptionEvidenceChangedBeforeCommit');
    atomicJson(checkpointPath,review.proposedCheckpoint);receipt.state='complete';receipt.completedAt=new Date().toISOString();atomicJson(receiptPath,receipt);
    process.stdout.write(`${JSON.stringify({Status:'BaselineAdopted',GitCommit:proposal.pins.targetGitCommit,P4Change:proposal.pins.p4Head,Receipt:receiptPath,SourceWrites:false})}\n`);
  }finally{closeSync(lock);unlinkSync(lockPath);}
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  try{main();}catch(error){process.stderr.write(`${error.stack??error}\n`);process.exitCode=2;}
}
