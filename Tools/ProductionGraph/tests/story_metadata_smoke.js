const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');

const storyMetadata = require('../story_metadata');

const root = fs.mkdtempSync(path.join(os.tmpdir(), 'story-metadata-'));

function write(filePath, text = '') {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, text, 'utf8');
}

write(path.join(root, 'Content', 'Art', 'Map', 'Map_Data', 'L1_InitialRoom', 'InitialRoom.umap'));
write(path.join(root, 'Content', 'Docs', 'Map', 'DA_Campaign_Tutorial.uasset'));
write(path.join(root, 'Content', 'Docs', 'Map', 'DA_Room_TutorialHub.uasset'));
write(path.join(root, 'Content', 'Docs', 'BuffDocs', 'V2-RuneCard', 'DA_Rune512_Attack.uasset'));
write(path.join(root, 'Content', 'Docs', 'UI', 'Tutorial', 'DA_Tutorial_WeaponPickup.uasset'));
write(path.join(root, 'Content', 'Story', 'Encounters', 'Tutorial', 'EG_FirstRun_Tutorial.uasset'));
write(path.join(root, 'Config', 'Tags', 'StoryTag.ini'), 'GameplayTagList=(Tag="Story.Flag.FirstRunTutorial.Active",DevComment="")\n');
write(path.join(root, 'Docs', 'StorySource', 'tutorial', 'first_run.story.json'), JSON.stringify({
  storyId: 'first_run',
  arc: 'tutorial',
  chapter: 'hub',
  timeline: 'present',
  points: [
    {
      condition: { progressKey: 'hub_started' },
      actions: [{ progressKey: 'weapon_seen' }]
    }
  ],
  states: ['Story.Tutorial.HubStarted']
}));
write(path.join(root, 'Docs', 'StoryPipeline', 'Metadata', 'ue_project_assets.json'), JSON.stringify({
  generatedAt: '2026-05-23T00:00:00Z',
  roomData: [
    {
      name: 'DA_HubRoom_InitialRoom',
      path: '/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom',
      roomName: 'InitialRoom',
      displayName: 'Initial Room',
      tags: ['Room.Layer.L1'],
      isHubRoom: true
    }
  ],
  campaigns: [
    {
      name: 'DA_Campaign_MainRun',
      path: '/Game/Docs/Map/DA_Campaign_MainRun',
      layerTag: 'Room.Layer.L1',
      roomPool: ['/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom']
    }
  ]
}));

const result = storyMetadata.syncMetadata(root);
assert.ok(fs.existsSync(path.join(root, result.metadataFile)), 'metadata json should be written');
assert.equal(result.metadata.summary.maps, 1, 'map should be scanned');
assert.equal(result.metadata.summary.campaigns, 2, 'fast and UE campaigns should be scanned');
assert.equal(result.metadata.summary.rooms, 1, 'rooms should use UE RoomData metadata when available');
assert.equal(result.metadata.summary.runes, 1, 'rune should be scanned');
assert.equal(result.metadata.summary.tutorials, 1, 'tutorial asset should be scanned');
assert.ok(result.metadata.maps[0].path.endsWith('/InitialRoom'), 'map should be converted to /Game path');
assert.ok(result.metadata.tags.includes('Story.Flag.FirstRunTutorial.Active'), 'gameplay tags should be scanned');
assert.ok(result.metadata.storyTerms.arcs.includes('tutorial'), 'story arc terms should be scanned');
assert.ok(result.metadata.storyTerms.progressKeys.includes('weapon_seen'), 'progress keys should be scanned');
assert.ok(result.metadata.rooms.some((room) => room.roomName === 'InitialRoom'), 'UE room name should be merged');
assert.ok(result.metadata.ueMetadataFile, 'UE metadata file should be referenced when present');

console.log('Story metadata smoke passed');
