import assert from 'node:assert/strict';
import test from 'node:test';
import { mkdtempSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { pathToFileURL } from 'node:url';
import { spawnSync } from 'node:child_process';
import { classifyV2, classifyImpact, planTerminal } from './planner.mjs';
import { parseGitTree, readGit } from './read-live.mjs';
import { readRawGitBlob } from '../v3-worker/auto-sync.mjs';
const policy=JSON.parse(readFileSync(new URL('./policy.v2.json',import.meta.url)));
test('all 38 reviewed sources are eligible but vendor assets and unreviewed resources remain excluded',()=>{
  assert.equal(policy.reviewedExactTextPaths.length,38);
  for(const path of policy.reviewedExactTextPaths)assert.equal(classifyV2(path).eligible,true,path);
  for(const path of ['Plugins/YogVision/Resources/T_VisionStrokes.png','Plugins/YogVision/Resources/Unknown.py','Plugins/YogVision/Shaders/Private/Unknown.usf','Content/FluidNinja/Texture.uasset'])assert.equal(classifyV2(path).eligible,false,path);
});
test('later USF edits require a binary build and HLSL edits require explicit impact review',()=>{
  const usf=policy.reviewedExactTextPaths.find(p=>p.endsWith('.usf'));const hlsl=policy.reviewedExactTextPaths.find(p=>p.endsWith('.hlsl'));
  assert.equal(classifyImpact([{path:usf}]),'binary-build-required');
  assert.equal(classifyImpact([{path:hlsl}]),'ambiguous-build-input-hold');
});
test('the exact root maintenance documents are skipped but arbitrary root documents are held',()=>{
  const paths=['.gitignore','.gitattributes','ChargeAbility_Plan.md','DebugCommands.md','StunCameraShake_Plan.md'];
  const good=planTerminal({gitEntries:{},p4Entries:{},changedPaths:paths});assert.equal(good.status,'NO_OP');
  assert.equal(planTerminal({gitEntries:{},p4Entries:{},changedPaths:['Unreviewed.md']}).status,'HOLD');
});
test('byte-identical newly eligible sources do not fabricate a code submission',()=>{
  const path=policy.reviewedExactTextPaths.find(p=>p.endsWith('.usf'));const item={digest:'a'.repeat(32),size:100,type:'text'};
  const plan=planTerminal({gitEntries:{[path]:item},p4Entries:{[path]:item},changedPaths:[path]});assert.equal(plan.status,'NO_OP');assert.equal(plan.actions.length,0);
});
test('default automatic planner still holds a changed path opened by another client',()=>{
  const path='Source/DevKit/Foo.cpp';const plan=planTerminal({gitEntries:{[path]:{digest:'a'.repeat(32),size:1}},p4Entries:{[path]:{digest:'b'.repeat(32),size:1,type:'text'}},openedPaths:[path]});assert.equal(plan.status,'HOLD');assert.ok(plan.holds.some(h=>h.reason==='opened-overlap'));
});
test('only the explicit Git-only maintenance source list is skipped',()=>{
  assert.equal(policy.exactGitOnlyPaths.length,24);
  const plan=planTerminal({gitEntries:{},p4Entries:{},changedPaths:policy.exactGitOnlyPaths});assert.equal(plan.status,'NO_OP');
  for(const path of ['Tools/SyncBridge/config.json','Tools/SyncBridge/sync_state.json','Tools/SyncBridge/sync_bridge.log','Tools/SyncBridge/cloud/unreviewed-new.mjs'])assert.equal(planTerminal({gitEntries:{},p4Entries:{},changedPaths:[path]}).status,'HOLD',path);
});
test('Git tree parser accepts name-only enumeration and preserves legacy size records',()=>{
  const oid='a'.repeat(40);
  assert.deepEqual(parseGitTree(Buffer.from(`100644 blob ${oid}\tSource/DevKit/Foo.cpp\0`)).get('Source/DevKit/Foo.cpp'),{mode:'100644',type:'blob',oid});
  assert.equal(parseGitTree(Buffer.from(`100644 blob ${oid} 12\tSource/DevKit/Foo.cpp\0`)).get('Source/DevKit/Foo.cpp').size,12);
  assert.throws(()=>parseGitTree(Buffer.from(`100644 blob ${oid} BAD\tSource/DevKit/Foo.cpp\0`)),/Invalid\/duplicate/);
});
test('partial clone reads eligible source without fetching an unrelated missing asset',()=>{
  const root=mkdtempSync(join(tmpdir(),'dev01-partial-source-'));
  const origin=join(root,'origin'),clone=join(root,'clone');
  const run=(args,cwd=root)=>{
    const result=spawnSync('git',args,{cwd,encoding:'utf8',windowsHide:true,env:{...process.env,GIT_TERMINAL_PROMPT:'0'}});
    assert.equal(result.status,0,`${args.join(' ')}: ${result.stderr}`);
    return result.stdout.trim();
  };
  try {
    run(['init','-b','main',origin]);
    run(['config','uploadpack.allowFilter','true'],origin);
    mkdirSync(join(origin,'Source','DevKit'),{recursive:true});
    mkdirSync(join(origin,'Content'),{recursive:true});
    writeFileSync(join(origin,'Source','DevKit','Foo.cpp'),'int x = 1;\n');
    writeFileSync(join(origin,'Content','Huge.uasset'),Buffer.alloc(2*1024*1024,0x61));
    run(['-c','core.autocrlf=false','add','.'],origin);
    run(['-c','user.name=Sync Test','-c','user.email=sync@example.invalid','commit','-m','partial clone fixture'],origin);
    const target=run(['rev-parse','HEAD'],origin),asset=run(['rev-parse','HEAD:Content/Huge.uasset'],origin);
    run(['clone','--filter=blob:none','--no-checkout',pathToFileURL(origin).href,clone]);
    const objects=()=>new Set(run(['cat-file','--batch-all-objects','--batch-check=%(objectname)'],clone).split(/\r?\n/));
    assert.equal(objects().has(asset),false,'fixture asset must initially be a missing promised blob');
    const result=readGit(clone,target,policy);
    assert.equal(result.violations.length,0);
    assert.equal(result.entries.size,1);
    assert.equal(result.entries.get('Source/DevKit/Foo.cpp').size,11);
    assert.equal(objects().has(asset),false,'excluded blob must stay missing after source inspection');
    const bounded=readGit(clone,target,{...policy,maxTextBytes:1});
    assert.equal(bounded.entries.size,0);
    assert.equal(bounded.violations[0].reason,'git-object-or-size-invalid');
    assert.equal(objects().has(asset),false);
    assert.throws(()=>readGit(clone,target,{...policy,gitHttpProxy:'http://external.invalid:7890'}),/InvalidGitHttpProxyPolicy/);
  } finally { rmSync(root,{recursive:true,force:true}); }
});
test('raw Git blob reads a CJK document with quotePath enabled and retains all identity guards',()=>{
  const root=mkdtempSync(join(tmpdir(),'dev01-cjk-blob-'));
  const git=(args)=>{
    const result=spawnSync('git',['-C',root,...args],{encoding:null,windowsHide:true,env:{...process.env,GIT_TERMINAL_PROMPT:'0'}});
    assert.equal(result.status,0,result.stderr?.toString('utf8'));
    return result;
  };
  try {
    git(['init','-b','main']);
    git(['config','core.quotePath','true']);
    mkdirSync(join(root,'Docs'),{recursive:true});
    const path='Docs/同步修复 文档.md',bytes=Buffer.from('# 中文同步说明\n','utf8');
    writeFileSync(join(root,...path.split('/')),bytes);
    git(['-c','core.autocrlf=false','add','.']);
    git(['-c','user.name=Sync Test','-c','user.email=sync@example.invalid','commit','-m','CJK fixture']);
    const sha=git(['rev-parse','HEAD']).stdout.toString('ascii').trim();
    assert.match(git(['ls-tree','-r',sha,'--',path]).stdout.toString('utf8'),/\t"/,'fixture must exercise the quoted path that previously failed');
    const expected=readGit(root,sha,policy).entries.get(path);
    assert.ok(expected);
    assert.deepEqual(readRawGitBlob(git,sha,path,expected),bytes);
    const {oid,...journalExpected}=expected;
    assert.deepEqual(readRawGitBlob(git,sha,path,journalExpected),bytes,'older journals without an OID still use all byte hash guards');
    assert.throws(()=>readRawGitBlob(git,sha,path,{...expected,oid:'f'.repeat(40)}),/GitBlobUnavailable/);
    assert.throws(()=>readRawGitBlob(git,sha,path,{...expected,sha256:'f'.repeat(64)}),/GitBlobDrift/);
    assert.throws(()=>readRawGitBlob(git,sha,'Docs',expected),/GitBlobUnavailable/);
  } finally { rmSync(root,{recursive:true,force:true}); }
});
