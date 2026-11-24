import assert from 'node:assert/strict';
import test from 'node:test';
import { readFileSync } from 'node:fs';
import { classifyV2, classifyImpact, planTerminal } from './planner.mjs';
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
