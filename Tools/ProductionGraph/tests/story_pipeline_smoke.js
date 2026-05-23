const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const storyPipeline = require('../story_pipeline');
const storyMetadata = require('../story_metadata');

const root = fs.mkdtempSync(path.join(os.tmpdir(), 'story-pipeline-'));
fs.mkdirSync(path.join(root, 'Content', 'Maps'), { recursive: true });
fs.writeFileSync(path.join(root, 'Content', 'Maps', 'Prison_Lower.umap'), '', 'utf8');
storyMetadata.syncMetadata(root);

const segment = storyPipeline.normalizeSegment({
  schemaVersion: 1,
  storyId: 'main_prison_awaken',
  arc: 'main',
  chapter: 'prison',
  timeline: 'present',
  encounterId: 'EM_Prison_Awakening',
  graphAsset: 'EG_Prison_Awakening',
  maps: ['Prison_Lower'],
  points: [
    {
      nodeId: 'wake_up',
      asset: 'EP_Prison_Awakening_WakeUp',
      displayName: '地下复活节点苏醒',
      kind: 'System',
      playerFacingEvent: '玩家在地下复活节点醒来。',
      firePolicy: 'Once',
      placementLevel: 'Prison_Lower',
      placementName: 'Trigger_WakeUp',
      condition: { kind: 'None' },
      actions: [
        {
          type: 'TutorialPopup',
          tutorialEventId: 'tutorial_awaken',
          title: '遗圣目录',
          body: '候圣体稳定。继续前进。',
          tutorialPages: [
            { title: '苏醒', body: '<input action="Move"/> 移动角色。', subText: '跟随光源。' },
            { title: '记录', body: '遗圣目录已经开始记录。' }
          ]
        },
        { type: 'RecordProgress', progressKey: 'wake_up_seen' }
      ],
      extra: { futureField: true }
    }
  ],
  states: ['Story.Main.Prison.AwakeningSeen'],
  keyItems: ['Item.Story.PrimordialBlood'],
  workItems: [{ type: 'LevelFlow', title: '配置苏醒镜头', status: 'todo' }],
  extra: { customNarrativeNote: 'keep me' }
});

const saved = storyPipeline.writeSegment(root, segment);
assert.ok(fs.existsSync(path.join(root, saved.sourceFile)), 'story source json should be written');
assert.ok(
  fs.existsSync(path.join(root, 'Docs', 'StorySource', 'main', 'main_prison_awaken.story.md')),
  'story markdown should be written'
);

const segments = storyPipeline.listSegments(root);
assert.equal(segments.length, 1, 'one segment should be listed');
assert.equal(segments[0].pointCount, 1, 'point count should be preserved');

const request = storyPipeline.createCodexRequest(root, {
  mode: '导出引擎manifest',
  sourceFiles: [saved.sourceFile]
});
assert.ok(fs.existsSync(path.join(root, request.requestFile)), 'codex request json should be written');
assert.ok(fs.existsSync(path.join(root, request.markdownFile)), 'codex request markdown should be written');

const exported = storyPipeline.exportPipeline(root, [saved.sourceFile]);
assert.ok(fs.existsSync(path.join(root, exported.manifestFile)), 'import manifest should be written');
assert.ok(fs.existsSync(path.join(root, exported.workloadFile)), 'workload should be written');
assert.ok(fs.existsSync(path.join(root, exported.validationFile)), 'validation report should be written');
assert.ok(exported.manifest.metadataFile, 'manifest should reference synced metadata');
assert.equal(exported.manifest.validation.errors.length, 0, 'valid segment should not produce errors');
assert.equal(exported.manifest.segments[0].extra.customNarrativeNote, 'keep me', 'extra fields should be preserved');
assert.equal(exported.manifest.segments[0].points[0].actions[0].tutorialPages.length, 2, 'tutorial popup pages should be preserved');

console.log('Story pipeline smoke passed');
