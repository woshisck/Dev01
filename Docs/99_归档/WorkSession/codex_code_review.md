OpenAI Codex v0.125.0 (research preview)
--------
workdir: D:\Self\GItGame\Dev01
model: gpt-5.5
provider: openai
approval: never
sandbox: workspace-write [workdir, /tmp, $TMPDIR, C:\Users\gongzhengang\.codex\memories]
reasoning effort: xhigh
reasoning summaries: none
session id: 019e0be4-3003-73a1-975c-cac0f2112030
--------
user
current changes
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'C:\\Users\\gongzhengang\\.codex\\plugins\\cache\\openai-curated\\superpowers\\63976030\\skills\\using-superpowers\\SKILL.md'" in D:\Self\GItGame\Dev01
 succeeded in 634ms:
---
name: using-superpowers
description: Use when starting any conversation - establishes how to find and use skills, requiring Skill tool invocation before ANY response including clarifying questions
---

<SUBAGENT-STOP>
If you were dispatched as a subagent to execute a specific task, skip this skill.
</SUBAGENT-STOP>

<EXTREMELY-IMPORTANT>
If you think there is even a 1% chance a skill might apply to what you are doing, you ABSOLUTELY MUST invoke the skill.

IF A SKILL APPLIES TO YOUR TASK, YOU DO NOT HAVE A CHOICE. YOU MUST USE IT.

This is not negotiable. This is not optional. You cannot rationalize your way out of this.
</EXTREMELY-IMPORTANT>

## Instruction Priority

Superpowers skills override default system prompt behavior, but **user instructions always take precedence**:

1. **User's explicit instructions** (CLAUDE.md, GEMINI.md, AGENTS.md, direct requests) 鈥?highest priority
2. **Superpowers skills** 鈥?override default system behavior where they conflict
3. **Default system prompt** 鈥?lowest priority

If CLAUDE.md, GEMINI.md, or AGENTS.md says "don't use TDD" and a skill says "always use TDD," follow the user's instructions. The user is in control.

## How to Access Skills

**In Claude Code:** Use the `Skill` tool. When you invoke a skill, its content is loaded and presented to you鈥攆ollow it directly. Never use the Read tool on skill files.

**In Copilot CLI:** Use the `skill` tool. Skills are auto-discovered from installed plugins. The `skill` tool works the same as Claude Code's `Skill` tool.

**In Gemini CLI:** Skills activate via the `activate_skill` tool. Gemini loads skill metadata at session start and activates the full content on demand.

**In other environments:** Check your platform's documentation for how skills are loaded.

## Platform Adaptation

Skills use Claude Code tool names. Non-CC platforms: see `references/copilot-tools.md` (Copilot CLI), `references/codex-tools.md` (Codex) for tool equivalents. Gemini CLI users get the tool mapping loaded automatically via GEMINI.md.

# Using Skills

## The Rule

**Invoke relevant or requested skills BEFORE any response or action.** Even a 1% chance a skill might apply means that you should invoke the skill to check. If an invoked skill turns out to be wrong for the situation, you don't need to use it.

```dot
digraph skill_flow {
    "User message received" [shape=doublecircle];
    "About to EnterPlanMode?" [shape=doublecircle];
    "Already brainstormed?" [shape=diamond];
    "Invoke brainstorming skill" [shape=box];
    "Might any skill apply?" [shape=diamond];
    "Invoke Skill tool" [shape=box];
    "Announce: 'Using [skill] to [purpose]'" [shape=box];
    "Has checklist?" [shape=diamond];
    "Create TodoWrite todo per item" [shape=box];
    "Follow skill exactly" [shape=box];
    "Respond (including clarifications)" [shape=doublecircle];

    "About to EnterPlanMode?" -> "Already brainstormed?";
    "Already brainstormed?" -> "Invoke brainstorming skill" [label="no"];
    "Already brainstormed?" -> "Might any skill apply?" [label="yes"];
    "Invoke brainstorming skill" -> "Might any skill apply?";

    "User message received" -> "Might any skill apply?";
    "Might any skill apply?" -> "Invoke Skill tool" [label="yes, even 1%"];
    "Might any skill apply?" -> "Respond (including clarifications)" [label="definitely not"];
    "Invoke Skill tool" -> "Announce: 'Using [skill] to [purpose]'";
    "Announce: 'Using [skill] to [purpose]'" -> "Has checklist?";
    "Has checklist?" -> "Create TodoWrite todo per item" [label="yes"];
    "Has checklist?" -> "Follow skill exactly" [label="no"];
    "Create TodoWrite todo per item" -> "Follow skill exactly";
}
```

## Red Flags

These thoughts mean STOP鈥攜ou're rationalizing:

| Thought | Reality |
|---------|---------|
| "This is just a simple question" | Questions are tasks. Check for skills. |
| "I need more context first" | Skill check comes BEFORE clarifying questions. |
| "Let me explore the codebase first" | Skills tell you HOW to explore. Check first. |
| "I can check git/files quickly" | Files lack conversation context. Check for skills. |
| "Let me gather information first" | Skills tell you HOW to gather information. |
| "This doesn't need a formal skill" | If a skill exists, use it. |
| "I remember this skill" | Skills evolve. Read current version. |
| "This doesn't count as a task" | Action = task. Check for skills. |
| "The skill is overkill" | Simple things become complex. Use it. |
| "I'll just do this one thing first" | Check BEFORE doing anything. |
| "This feels productive" | Undisciplined action wastes time. Skills prevent this. |
| "I know what that means" | Knowing the concept 鈮?using the skill. Invoke it. |

## Skill Priority

When multiple skills could apply, use this order:

1. **Process skills first** (brainstorming, debugging) - these determine HOW to approach the task
2. **Implementation skills second** (frontend-design, mcp-builder) - these guide execution

"Let's build X" 鈫?brainstorming first, then implementation skills.
"Fix this bug" 鈫?debugging first, then domain-specific skills.

## Skill Types

**Rigid** (TDD, debugging): Follow exactly. Don't adapt away discipline.

**Flexible** (patterns): Adapt principles to context.

The skill itself tells you which.

## User Instructions

Instructions say WHAT, not HOW. "Add X" or "Fix Y" doesn't mean skip workflows.


exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git status --short; git rev-parse --show-toplevel; git diff --stat; git diff --cached --stat' in D:\Self\GItGame\Dev01
 succeeded in 741ms:
 M Config/DefaultGameplayTags.ini
 M Config/Tags/BuffTag.ini
 M Config/Tags/PlayerGameplayTag.ini
 M Content/YogRuneEditor/Flows/FA_Rune_Moonlight.uasset
 M Content/YogRuneEditor/Flows/FA_Rune_Moonlight_Forward_Burn.uasset
 M Content/YogRuneEditor/Flows/FA_Rune_Moonlight_Forward_Poison.uasset
 M Content/YogRuneEditor/Flows/FA_Rune_Moonlight_Reversed_Burn.uasset
 M Content/YogRuneEditor/Flows/FA_Rune_Moonlight_Reversed_Poison.uasset
 M Content/YogRuneEditor/Runes/DA_Rune_Moonlight.uasset
 M Docs/Conventions/AnimNotify.md
 M Docs/FeatureLog.md
 D Docs/GeneratedReports/CommandletReports/512EnemyRoomRuneSetupReport.md
 D Docs/GeneratedReports/CommandletReports/512RuneCardBatchReport.md
 D Docs/GeneratedReports/CommandletReports/512TutorialSetupReport.md
 D Docs/GeneratedReports/CommandletReports/BackpackDeckUIStyleSetupReport.md
 D Docs/GeneratedReports/CommandletReports/CombatMontageSyncReport.md
 D Docs/GeneratedReports/CommandletReports/CurrentRoomBuffWidgetSetupReport.md
 D Docs/GeneratedReports/CommandletReports/EnemyAITemplateGeneratorReport.md
 D Docs/GeneratedReports/CommandletReports/GamepadInputSetupReport.md
 D Docs/GeneratedReports/CommandletReports/MainUISetupReport.md
 D Docs/GeneratedReports/CommandletReports/PortalPreviewWidgetSetupReport.md
 D Docs/GeneratedReports/CommandletReports/PrayRoomSacrificeEventSetupReport.md
 D Docs/GeneratedReports/CommandletReports/README.md
 D Docs/GeneratedReports/CommandletReports/ShopRoomSetupReport.md
 D Docs/GeneratedReports/CommandletReports/TutorialPopupButtonHintSetupReport.md
 M Docs/INDEX.md
 D Docs/PM/CurrentProgress_20260421.md
 M Docs/PM/TASKS.md
 D Docs/Prototypes/RuneEditorPreview/index.html
 D Docs/Research/PlayerResearch_Design.md
 D Docs/Research/PlayerResearch_QA_20260415.md
 D Docs/Research/PlayerResearch_Questionnaire_v0.1_20260415.md
 D Docs/Research/PlayerResearch_Questionnaire_v0.2_20260429.md
 D Docs/Research/PlayerResearch_Report_v0.1_20260415.md
 M Docs/Systems/Combat/AttackDamage_ConfigGuide.md
 M Docs/Systems/Rune/YogRuneEditor_Technical.md
 D Docs/TBD_ComboMake.md
 M Docs/Tags/GameplayTag_MasterGuide.md
 D Docs/WorkSession/PhaseB_Workflow.md
 M Docs/WorkSession/codex_code_review.md
 M Docs/WorkSession/codex_plan_review.md
 M Docs/WorkSession/current_plan.md
 D Docs/WorkSession/data_authoring_plan.md
 D Docs/superpowers/plans/2026-05-08-rune-editor-formal.md
 D Docs/superpowers/plans/2026-05-08-rune-value-table-tabs.md
 M Docs/项目需求文档/512版本项目计划/设计/符文/终结技卡牌_德式双手剑.md
 M Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
 M Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp
 M Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp
 M Source/DevKit/Private/Character/YogPlayerControllerBase.cpp
 M Source/DevKit/Private/Component/ComboRuntimeComponent.cpp
 M Source/DevKit/Private/Projectile/MusketBullet.cpp
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_AddTag.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyExecution.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_AreaDamage.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcDamage.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_CheckDistance.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareFloat.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareInt.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_Delay.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_DestroyNiagara.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoDamage.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoOnce.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetAttribute.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneInfo.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneTuningValue.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantGA.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantSacrificePassive.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantTag.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_HasTag.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_IfStatement.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathFloat.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathInt.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnDamageDealt.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHealthChanged.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPeriodic.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagAdded.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagRemoved.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayMontage.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayNiagara.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_RemoveTag.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SendGameplayEvent.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_TrackMovement.h
 M Source/DevKit/Public/BuffFlow/Nodes/BFNode_WaitGameplayEvent.h
 M Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h
 M Source/DevKit/Public/Projectile/MusketBullet.h
 M Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
 M Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h
?? Content/YogRuneEditor/Asset/
?? Docs/Conventions/NodeCreation_RedLines.md
?? Docs/PM/CurrentProgress_20260508.md
?? Docs/Systems/Rune/FA_Moonlight_NodeSequences.md
?? Docs/Systems/Rune/RuneEditor_UserGuide.md
?? Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp
?? Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp
?? Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp
?? Source/DevKit/Private/Animation/ANS_FinisherTimeDilation.cpp
?? Source/DevKit/Private/Animation/AN_TriggerFinisherAbility.cpp
?? Source/DevKit/Private/BuffFlow/Nodes/BFNode_PureData.cpp
?? Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h
?? Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h
?? Source/DevKit/Public/AbilitySystem/Abilities/GA_Player_FinisherAttack.h
?? Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h
?? Source/DevKit/Public/Animation/AN_TriggerFinisherAbility.h
?? Source/DevKit/Public/BuffFlow/Nodes/BFNode_PureData.h
D:/Self/GItGame/Dev01
 Config/DefaultGameplayTags.ini                     |   5 +
 Config/Tags/BuffTag.ini                            |   4 +
 Config/Tags/PlayerGameplayTag.ini                  |  11 +
 .../YogRuneEditor/Flows/FA_Rune_Moonlight.uasset   | Bin 9494 -> 15910 bytes
 .../Flows/FA_Rune_Moonlight_Forward_Burn.uasset    | Bin 9562 -> 43213 bytes
 .../Flows/FA_Rune_Moonlight_Forward_Poison.uasset  | Bin 9574 -> 54954 bytes
 .../Flows/FA_Rune_Moonlight_Reversed_Burn.uasset   | Bin 9568 -> 43289 bytes
 .../Flows/FA_Rune_Moonlight_Reversed_Poison.uasset | Bin 9580 -> 55054 bytes
 .../YogRuneEditor/Runes/DA_Rune_Moonlight.uasset   | Bin 15647 -> 20837 bytes
 Docs/FeatureLog.md                                 |   2 +-
 .../512EnemyRoomRuneSetupReport.md                 |  43 --
 .../CommandletReports/512RuneCardBatchReport.md    | 319 ------------
 .../CommandletReports/512TutorialSetupReport.md    |  47 --
 .../BackpackDeckUIStyleSetupReport.md              |   9 -
 .../CommandletReports/CombatMontageSyncReport.md   |  42 --
 .../CurrentRoomBuffWidgetSetupReport.md            |   6 -
 .../EnemyAITemplateGeneratorReport.md              |  18 -
 .../CommandletReports/GamepadInputSetupReport.md   |  17 -
 .../CommandletReports/MainUISetupReport.md         |  10 -
 .../PortalPreviewWidgetSetupReport.md              |   6 -
 .../PrayRoomSacrificeEventSetupReport.md           |  11 -
 Docs/GeneratedReports/CommandletReports/README.md  |   8 -
 .../CommandletReports/ShopRoomSetupReport.md       |  13 -
 .../TutorialPopupButtonHintSetupReport.md          |   5 -
 Docs/INDEX.md                                      |   2 +-
 Docs/PM/CurrentProgress_20260421.md                | 141 ------
 Docs/Prototypes/RuneEditorPreview/index.html       | 416 ----------------
 Docs/Research/PlayerResearch_Design.md             | 152 ------
 Docs/Research/PlayerResearch_QA_20260415.md        | 550 ---------------------
 .../PlayerResearch_Questionnaire_v0.1_20260415.md  | 334 -------------
 .../PlayerResearch_Questionnaire_v0.2_20260429.md  | 159 ------
 .../PlayerResearch_Report_v0.1_20260415.md         | 248 ----------
 Docs/TBD_ComboMake.md                              |  57 ---
 Docs/WorkSession/PhaseB_Workflow.md                | 324 ------------
 Docs/WorkSession/codex_code_review.md              | 141 +++++-
 Docs/WorkSession/codex_plan_review.md              |  54 +-
 Docs/WorkSession/current_plan.md                   | 525 +++++++++++++-------
 Docs/WorkSession/data_authoring_plan.md            | 257 ----------
 .../plans/2026-05-08-rune-editor-formal.md         | 163 ------
 .../plans/2026-05-08-rune-value-table-tabs.md      |  50 --
 .../Nodes/BFNode_SpawnRangedProjectiles.cpp        |  69 ++-
 .../DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp |  60 ++-
 .../DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp   |   3 +-
 .../Private/Character/YogPlayerControllerBase.cpp  |  16 +
 .../Private/Component/ComboRuntimeComponent.cpp    |   8 +
 Source/DevKit/Private/Projectile/MusketBullet.cpp  | 109 ++++
 .../DevKit/Public/BuffFlow/Nodes/BFNode_AddTag.h   |   9 +-
 .../BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h | 126 ++---
 .../Public/BuffFlow/Nodes/BFNode_ApplyEffect.h     |  68 +--
 .../Public/BuffFlow/Nodes/BFNode_ApplyExecution.h  |  41 +-
 .../BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h |   9 +-
 .../Public/BuffFlow/Nodes/BFNode_AreaDamage.h      |  32 +-
 .../Public/BuffFlow/Nodes/BFNode_CalcDamage.h      |  42 +-
 .../Nodes/BFNode_CalcRuneGroundPathTransform.h     |  39 +-
 .../Public/BuffFlow/Nodes/BFNode_CheckDistance.h   |   6 +-
 .../Public/BuffFlow/Nodes/BFNode_CompareFloat.h    |  11 +-
 .../Public/BuffFlow/Nodes/BFNode_CompareInt.h      |  18 +-
 Source/DevKit/Public/BuffFlow/Nodes/BFNode_Delay.h |  15 +-
 .../Public/BuffFlow/Nodes/BFNode_DestroyNiagara.h  |   4 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_DoDamage.h |  22 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_DoOnce.h   |   4 +-
 .../Public/BuffFlow/Nodes/BFNode_GetAttribute.h    |  10 +-
 .../Public/BuffFlow/Nodes/BFNode_GetRuneInfo.h     |  37 +-
 .../BuffFlow/Nodes/BFNode_GetRuneTuningValue.h     |  12 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_GrantGA.h  |  25 +-
 .../BuffFlow/Nodes/BFNode_GrantSacrificePassive.h  |   3 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_GrantTag.h |  22 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_HasTag.h   |   6 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h  |  27 +-
 .../Public/BuffFlow/Nodes/BFNode_IfStatement.h     |   3 +-
 .../Public/BuffFlow/Nodes/BFNode_MathFloat.h       |  13 +-
 .../DevKit/Public/BuffFlow/Nodes/BFNode_MathInt.h  |  13 +-
 .../Public/BuffFlow/Nodes/BFNode_OnDamageDealt.h   |   9 +-
 .../Public/BuffFlow/Nodes/BFNode_OnHealthChanged.h |   5 +-
 .../Public/BuffFlow/Nodes/BFNode_OnPeriodic.h      |   8 +-
 .../Public/BuffFlow/Nodes/BFNode_OnTagAdded.h      |   8 +-
 .../Public/BuffFlow/Nodes/BFNode_OnTagRemoved.h    |  10 +-
 .../Public/BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h |  68 ++-
 .../Public/BuffFlow/Nodes/BFNode_PlayMontage.h     |  12 +-
 .../Public/BuffFlow/Nodes/BFNode_PlayNiagara.h     |  34 +-
 .../BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h     |   9 +-
 .../Public/BuffFlow/Nodes/BFNode_RemoveTag.h       |   9 +-
 .../Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h  |  18 +-
 .../BuffFlow/Nodes/BFNode_SendGameplayEvent.h      |  30 +-
 .../BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h   |  12 +-
 .../Nodes/BFNode_SpawnGameplayCueAtLocation.h      |  26 +-
 .../Nodes/BFNode_SpawnGameplayCueOnActor.h         |  12 +-
 .../BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h | 101 +++-
 .../BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h   |   9 +-
 .../Nodes/BFNode_SpawnRuneGroundPathEffect.h       |  89 +++-
 .../Nodes/BFNode_SpawnRuneProjectileProfile.h      |   3 +-
 .../Nodes/BFNode_SpawnSlashWaveProjectile.h        | 220 ++++++---
 .../Public/BuffFlow/Nodes/BFNode_TrackMovement.h   |  12 +-
 .../BuffFlow/Nodes/BFNode_WaitGameplayEvent.h      |  17 +-
 Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h |  38 +-
 Source/DevKit/Public/Projectile/MusketBullet.h     |  40 ++
 .../Private/RuneEditor/SRuneEditorWidget.cpp       | 128 ++++-
 .../Private/RuneEditor/SRuneEditorWidget.h         |   8 +-
 98 files changed, 1688 insertions(+), 4278 deletions(-)
warning: unable to access 'C:\Users\gongzhengang/.config/git/ignore': Permission denied
warning: unable to access 'C:\Users\gongzhengang/.config/git/ignore': Permission denied
warning: in the working copy of 'Config/DefaultGameplayTags.ini', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/Conventions/AnimNotify.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/FeatureLog.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/INDEX.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/PM/TASKS.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/Systems/Combat/AttackDamage_ConfigGuide.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/Systems/Rune/YogRuneEditor_Technical.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/Tags/GameplayTag_MasterGuide.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/WorkSession/codex_code_review.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/WorkSession/codex_plan_review.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/WorkSession/current_plan.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Docs/项目需求文档/512版本项目计划/设计/符文/终结技卡牌_德式双手剑.md', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Private/Projectile/MusketBullet.cpp', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_AddTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyExecution.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_AreaDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CheckDistance.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareFloat.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareInt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_Delay.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DestroyNiagara.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoOnce.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetAttribute.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneInfo.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneTuningValue.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantGA.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantSacrificePassive.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_HasTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_IfStatement.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathFloat.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathInt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnDamageDealt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHealthChanged.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPeriodic.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagAdded.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagRemoved.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayMontage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayNiagara.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_RemoveTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SendGameplayEvent.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_TrackMovement.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_WaitGameplayEvent.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/Projectile/MusketBullet.h', LF will be replaced by CRLF the next time Git touches it
warning: unable to access 'C:\Users\gongzhengang/.config/git/ignore': Permission denied

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git diff -- Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp Source/DevKit/Private/Character/YogPlayerControllerBase.cpp Source/DevKit/Private/Component/ComboRuntimeComponent.cpp Source/DevKit/Private/Projectile/MusketBullet.cpp Source/DevKit/Public/Projectile/MusketBullet.h Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h' in D:\Self\GItGame\Dev01
 succeeded in 431ms:
diff --git a/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp b/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
index 4d5467b9..c9c20522 100644
--- a/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
+++ b/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
@@ -21,6 +21,7 @@ UBFNode_SpawnRangedProjectiles::UBFNode_SpawnRangedProjectiles(const FObjectInit
 	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
 	YawOffsets = { -8.f, 8.f };
 	Damage = FFlowDataPinInputProperty_Float(0.f);
+	HitGameplayEventMagnitude = FFlowDataPinInputProperty_Float(0.f);
 	RequiredWeaponTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
 }
 
@@ -59,6 +60,7 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 
 	const FCombatCardEffectContext& CardContext = BFC->GetLastCombatCardEffectContext();
 	const FCombatDeckActionContext& ActionContext = CardContext.ActionContext;
+	const TArray<float> ResolvedYawOffsets = BuildResolvedYawOffsets(CardContext.ComboBonusStacks);
 
 	float ResolvedDamage = ActionContext.AttackDamage;
 	if (!bUseCombatCardAttackDamage || ResolvedDamage <= 0.f)
@@ -72,7 +74,7 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 		}
 	}
 
-	TSubclassOf<AMusketBullet> ResolvedBulletClass = ActionContext.RangedProjectileClass
+	TSubclassOf<AMusketBullet> ResolvedBulletClass = bPreferCombatCardProjectileClass && ActionContext.RangedProjectileClass
 		? ActionContext.RangedProjectileClass
 		: BulletClass;
 	if (!ResolvedBulletClass)
@@ -80,7 +82,7 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 		ResolvedBulletClass = AMusketBullet::StaticClass();
 	}
 
-	TSubclassOf<UGameplayEffect> ResolvedDamageEffect = ActionContext.RangedDamageEffectClass
+	TSubclassOf<UGameplayEffect> ResolvedDamageEffect = bPreferCombatCardDamageEffectClass && ActionContext.RangedDamageEffectClass
 		? ActionContext.RangedDamageEffectClass
 		: DamageEffectClass;
 	if (!ResolvedDamageEffect)
@@ -97,7 +99,18 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 
 	int32 SpawnedCount = 0;
-	for (const float YawOffset : YawOffsets)
+	float ResolvedHitEventMagnitude = HitGameplayEventMagnitude.Value;
+	if (HitGameplayEventTag.IsValid() && !bUseDamageAsHitGameplayEventMagnitude)
+	{
+		const FFlowDataPinResult_Float EventMagnitudeResult = TryResolveDataPinAsFloat(
+			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, HitGameplayEventMagnitude));
+		if (EventMagnitudeResult.Result == EFlowDataPinResolveResult::Success)
+		{
+			ResolvedHitEventMagnitude = EventMagnitudeResult.Value;
+		}
+	}
+
+	for (const float YawOffset : ResolvedYawOffsets)
 	{
 		const FRotator SpawnRotation(0.f, BaseYaw + YawOffset, 0.f);
 		AMusketBullet* Bullet = SourceCharacter->GetWorld()->SpawnActor<AMusketBullet>(
@@ -118,6 +131,11 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 			ActionContext.bFromDashSave,
 			SharedGuid,
 			ResolvedDamage);
+		Bullet->SetHitGameplayEvent(
+			HitGameplayEventTag,
+			bSendHitGameplayEventToSourceASC,
+			bUseDamageAsHitGameplayEventMagnitude,
+			ResolvedHitEventMagnitude);
 		++SpawnedCount;
 	}
 
@@ -133,6 +151,51 @@ void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
 	TriggerOutput(SpawnedCount > 0 ? TEXT("Out") : TEXT("Failed"), true);
 }
 
+TArray<float> UBFNode_SpawnRangedProjectiles::BuildResolvedYawOffsets(int32 ComboBonusStacks) const
+{
+	int32 BonusProjectileCount = 0;
+	if (bAddComboStacksToProjectileCount)
+	{
+		BonusProjectileCount = FMath::Min(
+			FMath::Max(0, MaxBonusProjectiles),
+			FMath::Max(0, ComboBonusStacks) * FMath::Max(0, ProjectilesPerComboStack));
+	}
+
+	TArray<float> Result;
+	if (!bUseProjectileCountPattern && BonusProjectileCount <= 0)
+	{
+		Result = YawOffsets;
+		if (Result.Num() == 0)
+		{
+			Result.Add(0.f);
+		}
+		return Result;
+	}
+
+	const int32 BaseProjectileCount = bUseProjectileCountPattern
+		? FMath::Max(1, ProjectileCount)
+		: FMath::Max(1, YawOffsets.Num());
+	const int32 FinalProjectileCount = FMath::Max(1, BaseProjectileCount + BonusProjectileCount);
+	Result.Reserve(FinalProjectileCount);
+
+	if (FinalProjectileCount == 1 || ProjectileConeAngleDegrees <= KINDA_SMALL_NUMBER)
+	{
+		for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
+		{
+			Result.Add(0.f);
+		}
+		return Result;
+	}
+
+	const float Step = ProjectileConeAngleDegrees / static_cast<float>(FinalProjectileCount - 1);
+	const float StartYaw = -ProjectileConeAngleDegrees * 0.5f;
+	for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
+	{
+		Result.Add(StartYaw + Step * static_cast<float>(Index));
+	}
+	return Result;
+}
+
 FVector UBFNode_SpawnRangedProjectiles::ResolveMuzzleLocation(ACharacter* SourceCharacter) const
 {
 	if (!SourceCharacter)
diff --git a/Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp b/Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp
index 0ffbb578..a879517d 100644
--- a/Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp
+++ b/Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp
@@ -1,4 +1,7 @@
 #include "BuffFlow/Nodes/YogFlowNodes.h"
+#include "Character/PlayerCharacterBase.h"
+#include "Component/ComboRuntimeComponent.h"
+#include "BuffFlow/BuffFlowComponent.h"
 
 namespace
 {
@@ -225,14 +228,6 @@ UYogFlowNode_SpawnRangedProjectiles::UYogFlowNode_SpawnRangedProjectiles(const F
 #endif
 }
 
-UYogFlowNode_SpawnSlashWave::UYogFlowNode_SpawnSlashWave(const FObjectInitializer& ObjectInitializer)
-	: Super(ObjectInitializer)
-{
-#if WITH_EDITOR
-	Category = SpawnCategory;
-#endif
-}
-
 UYogFlowNode_ConditionAttributeCompare::UYogFlowNode_ConditionAttributeCompare(const FObjectInitializer& ObjectInitializer)
 	: Super(ObjectInitializer)
 {
@@ -328,3 +323,52 @@ UYogFlowNode_LifecycleFinishBuff::UYogFlowNode_LifecycleFinishBuff(const FObject
 	Category = LifecycleCategory;
 #endif
 }
+
+// ---------------------------------------------------------------------------
+// Pure 数据节点实现
+// ---------------------------------------------------------------------------
+
+UBFNode_Pure_TuningValue::UBFNode_Pure_TuningValue(const FObjectInitializer& OI) : Super(OI)
+{
+#if WITH_EDITOR
+	Category = TEXT("Pure");
+#endif
+	OutputPins = { FFlowPin(FName("Value"), EFlowPinType::Float) };
+}
+
+FFlowDataPinResult_Float UBFNode_Pure_TuningValue::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
+{
+	if (PinName == FName("Value"))
+	{
+		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
+		{
+			return FFlowDataPinResult_Float(BFC->GetRuneTuningValueForFlow(GetFlowAsset(), TuningKey, DefaultValue));
+		}
+		return FFlowDataPinResult_Float(DefaultValue);
+	}
+	return FFlowDataPinResult_Float();
+}
+
+UBFNode_Pure_ComboIndex::UBFNode_Pure_ComboIndex(const FObjectInitializer& OI) : Super(OI)
+{
+#if WITH_EDITOR
+	Category = TEXT("Pure");
+#endif
+	OutputPins = { FFlowPin(FName("ComboIndex"), EFlowPinType::Int) };
+}
+
+FFlowDataPinResult_Int UBFNode_Pure_ComboIndex::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
+{
+	if (PinName == FName("ComboIndex"))
+	{
+		if (APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetBuffOwner()))
+		{
+			if (PC->ComboRuntimeComponent)
+			{
+				return FFlowDataPinResult_Int(PC->ComboRuntimeComponent->GetComboIndex());
+			}
+		}
+		return FFlowDataPinResult_Int(1);
+	}
+	return FFlowDataPinResult_Int();
+}
diff --git a/Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp b/Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp
index 86d97c9a..20ee49c9 100644
--- a/Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp
+++ b/Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp
@@ -34,7 +34,6 @@ UYogRuneFlowAsset::UYogRuneFlowAsset(const FObjectInitializer& ObjectInitializer
 	AllowedNodeClasses.Add(UYogFlowNode_SpawnAreaProfile::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_SpawnGroundPath::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_SpawnRangedProjectiles::StaticClass());
-	AllowedNodeClasses.Add(UYogFlowNode_SpawnSlashWave::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_ConditionAttributeCompare::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_ConditionHasTag::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_ConditionProbability::StaticClass());
@@ -47,6 +46,8 @@ UYogRuneFlowAsset::UYogRuneFlowAsset(const FObjectInitializer& ObjectInitializer
 	AllowedNodeClasses.Add(UYogFlowNode_PresentationFlipbook::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_LifecycleDelay::StaticClass());
 	AllowedNodeClasses.Add(UYogFlowNode_LifecycleFinishBuff::StaticClass());
+	AllowedNodeClasses.Add(UBFNode_Pure_TuningValue::StaticClass());
+	AllowedNodeClasses.Add(UBFNode_Pure_ComboIndex::StaticClass());
 
 	AllowedInSubgraphNodeClasses.Reset();
 	DeniedNodeClasses.Reset();
diff --git a/Source/DevKit/Private/Character/YogPlayerControllerBase.cpp b/Source/DevKit/Private/Character/YogPlayerControllerBase.cpp
index 96c0c5dd..6f45b229 100644
--- a/Source/DevKit/Private/Character/YogPlayerControllerBase.cpp
+++ b/Source/DevKit/Private/Character/YogPlayerControllerBase.cpp
@@ -420,6 +420,22 @@ void AYogPlayerControllerBase::HeavyAtack(const FInputActionValue& Value)
 			Buffer->RecordHeavyAttack();
 		}
 
+		// 终结技执行期间：将重攻击路由到确认事件，不驱动 ComboRuntime
+		static const FGameplayTag TAG_FinisherExecuting =
+			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherExecuting"));
+		if (UAbilitySystemComponent* ASC = player->GetASC())
+		{
+			if (ASC->HasMatchingGameplayTag(TAG_FinisherExecuting))
+			{
+				FGameplayEventData EventData;
+				EventData.Instigator = player;
+				ASC->HandleGameplayEvent(
+					FGameplayTag::RequestGameplayTag(TEXT("Action.Finisher.Confirm")),
+					&EventData);
+				return;
+			}
+		}
+
 		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasComboSource())
 		{
 			player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Heavy, player);
diff --git a/Source/DevKit/Private/Component/ComboRuntimeComponent.cpp b/Source/DevKit/Private/Component/ComboRuntimeComponent.cpp
index 0c9b3019..4805862d 100644
--- a/Source/DevKit/Private/Component/ComboRuntimeComponent.cpp
+++ b/Source/DevKit/Private/Component/ComboRuntimeComponent.cpp
@@ -517,6 +517,14 @@ bool UComboRuntimeComponent::TryActivateCombo(ECombatGraphInputAction InputActio
 	{
 		PlayerOwner->CombatDeckComponent->RestorePendingLinkContextFromDash();
 	}
+
+	static const FGameplayTag ChainActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Combo.ChainActive"), false);
+	if (ChainActiveTag.IsValid() && ASC->GetTagCount(ChainActiveTag) <= 0)
+	{
+		ASC->AddLooseGameplayTag(ChainActiveTag);
+		TrackRuntimeCombatLooseTag(ChainActiveTag);
+	}
+
 	return true;
 }
 
diff --git a/Source/DevKit/Private/Projectile/MusketBullet.cpp b/Source/DevKit/Private/Projectile/MusketBullet.cpp
index 1a210e61..76a8dbf4 100644
--- a/Source/DevKit/Private/Projectile/MusketBullet.cpp
+++ b/Source/DevKit/Private/Projectile/MusketBullet.cpp
@@ -5,11 +5,14 @@
 #include "Components/SphereComponent.h"
 #include "GameFramework/ProjectileMovementComponent.h"
 #include "GameFramework/Character.h"
+#include "Abilities/GameplayAbilityTypes.h"
 #include "AbilitySystemComponent.h"
 #include "AbilitySystemBlueprintLibrary.h"
 #include "Character/PlayerCharacterBase.h"
 #include "Component/CombatDeckComponent.h"
 #include "GameplayEffect.h"
+#include "NiagaraComponent.h"
+#include "NiagaraFunctionLibrary.h"
 #include "TimerManager.h"
 
 namespace
@@ -76,6 +79,39 @@ void AMusketBullet::SetCombatDeckContextWithGuid(
     CombatDeckAttackDamage = InAttackDamage;
 }
 
+void AMusketBullet::SetHitGameplayEvent(
+    FGameplayTag InEventTag,
+    bool bInSendToSourceASC,
+    bool bInUseDamageAsMagnitude,
+    float InEventMagnitude)
+{
+    HitGameplayEventTag = InEventTag;
+    bSendHitGameplayEventToSourceASC = bInSendToSourceASC;
+    bUseDamageAsHitGameplayEventMagnitude = bInUseDamageAsMagnitude;
+    HitGameplayEventMagnitude = InEventMagnitude;
+}
+
+void AMusketBullet::SetProjectileNiagara(
+    UNiagaraSystem* InProjectileVisualNiagaraSystem,
+    FVector InProjectileVisualNiagaraScale,
+    UNiagaraSystem* InHitNiagaraSystem,
+    FVector InHitNiagaraScale,
+    UNiagaraSystem* InExpireNiagaraSystem,
+    FVector InExpireNiagaraScale)
+{
+    ProjectileVisualNiagaraSystem = InProjectileVisualNiagaraSystem;
+    ProjectileVisualNiagaraScale = InProjectileVisualNiagaraScale;
+    HitNiagaraSystem = InHitNiagaraSystem;
+    HitNiagaraScale = InHitNiagaraScale;
+    ExpireNiagaraSystem = InExpireNiagaraSystem;
+    ExpireNiagaraScale = InExpireNiagaraScale;
+
+    if (HasActorBegunPlay())
+    {
+        SpawnProjectileVisualNiagara();
+    }
+}
+
 void AMusketBullet::BeginPlay()
 {
     Super::BeginPlay();
@@ -86,6 +122,7 @@ void AMusketBullet::BeginPlay()
         LifetimeTimerHandle, this, &AMusketBullet::Expire, Lifetime, false);
 
     ScheduleInitialOverlapCheck();
+    SpawnProjectileVisualNiagara();
 }
 
 void AMusketBullet::OnOverlapBegin(
@@ -205,10 +242,81 @@ void AMusketBullet::ApplyDamageTo(AActor* Target, const FVector& HitLocation)
         SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
     }
 
+    SendHitGameplayEvent(Target, SourceASC);
     ResolveCombatDeckOnHit();
+    SpawnBurstNiagara(HitNiagaraSystem, HitLocation, HitNiagaraScale);
     BP_OnHitEnemy(Target, HitLocation);
 }
 
+void AMusketBullet::SendHitGameplayEvent(AActor* Target, UAbilitySystemComponent* SourceASC)
+{
+    if (!HitGameplayEventTag.IsValid() || !Target || !SourceASC || !SourceCharacter)
+    {
+        return;
+    }
+
+    UAbilitySystemComponent* EventASC = bSendHitGameplayEventToSourceASC
+        ? SourceASC
+        : UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
+    if (!EventASC)
+    {
+        return;
+    }
+
+    FGameplayEventData Payload;
+    Payload.EventTag = HitGameplayEventTag;
+    Payload.Instigator = SourceCharacter;
+    Payload.Target = Target;
+    Payload.EventMagnitude = bUseDamageAsHitGameplayEventMagnitude
+        ? DamageMagnitude
+        : HitGameplayEventMagnitude;
+    Payload.ContextHandle = SourceASC->MakeEffectContext();
+    Payload.ContextHandle.AddInstigator(SourceCharacter, SourceCharacter);
+    Payload.OptionalObject = this;
+
+    EventASC->HandleGameplayEvent(HitGameplayEventTag, &Payload);
+}
+
+void AMusketBullet::SpawnProjectileVisualNiagara()
+{
+    if (!ProjectileVisualNiagaraSystem || ProjectileVisualNiagaraComponent)
+    {
+        return;
+    }
+
+    ProjectileVisualNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
+        ProjectileVisualNiagaraSystem,
+        GetRootComponent(),
+        NAME_None,
+        FVector::ZeroVector,
+        FRotator::ZeroRotator,
+        EAttachLocation::KeepRelativeOffset,
+        true,
+        true);
+
+    if (ProjectileVisualNiagaraComponent)
+    {
+        ProjectileVisualNiagaraComponent->SetRelativeScale3D(ProjectileVisualNiagaraScale);
+    }
+}
+
+void AMusketBullet::SpawnBurstNiagara(UNiagaraSystem* NiagaraSystem, const FVector& WorldLocation, const FVector& Scale) const
+{
+    if (!NiagaraSystem || !GetWorld())
+    {
+        return;
+    }
+
+    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
+        GetWorld(),
+        NiagaraSystem,
+        WorldLocation,
+        GetActorRotation(),
+        Scale,
+        true,
+        true);
+}
+
 void AMusketBullet::ResolveCombatDeckOnHit()
 {
     if (bCombatDeckResolved)
@@ -238,6 +346,7 @@ void AMusketBullet::ResolveCombatDeckOnHit()
 
 void AMusketBullet::Expire()
 {
+    SpawnBurstNiagara(ExpireNiagaraSystem, GetActorLocation(), ExpireNiagaraScale);
     BP_OnMiss();
     Destroy();
 }
diff --git a/Source/DevKit/Public/Projectile/MusketBullet.h b/Source/DevKit/Public/Projectile/MusketBullet.h
index 7e256ceb..9909fd11 100644
--- a/Source/DevKit/Public/Projectile/MusketBullet.h
+++ b/Source/DevKit/Public/Projectile/MusketBullet.h
@@ -11,6 +11,9 @@
 class USphereComponent;
 class UProjectileMovementComponent;
 class UGameplayEffect;
+class UAbilitySystemComponent;
+class UNiagaraComponent;
+class UNiagaraSystem;
 class ACharacter;
 
 /**
@@ -55,6 +58,20 @@ public:
         const FGuid& InAttackInstanceGuid,
         float InAttackDamage);
 
+    void SetHitGameplayEvent(
+        FGameplayTag InEventTag,
+        bool bInSendToSourceASC,
+        bool bInUseDamageAsMagnitude,
+        float InEventMagnitude);
+
+    void SetProjectileNiagara(
+        UNiagaraSystem* InProjectileVisualNiagaraSystem,
+        FVector InProjectileVisualNiagaraScale,
+        UNiagaraSystem* InHitNiagaraSystem,
+        FVector InHitNiagaraScale,
+        UNiagaraSystem* InExpireNiagaraSystem,
+        FVector InExpireNiagaraScale);
+
     /** 飞行速度（cm/s），蓄力完成子弹可配置更高速度 */
     UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
     float Speed = 2800.f;
@@ -96,6 +113,26 @@ private:
     ECardRequiredAction CombatDeckActionType = ECardRequiredAction::Any;
     FGuid CombatDeckAttackInstanceGuid;
     float CombatDeckAttackDamage = 0.f;
+    FGameplayTag HitGameplayEventTag;
+    bool bSendHitGameplayEventToSourceASC = true;
+    bool bUseDamageAsHitGameplayEventMagnitude = true;
+    float HitGameplayEventMagnitude = 0.f;
+
+    UPROPERTY()
+    TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem;
+
+    UPROPERTY()
+    TObjectPtr<UNiagaraSystem> HitNiagaraSystem;
+
+    UPROPERTY()
+    TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem;
+
+    UPROPERTY()
+    TObjectPtr<UNiagaraComponent> ProjectileVisualNiagaraComponent;
+
+    FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
+    FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);
+    FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);
 
     FTimerHandle LifetimeTimerHandle;
 
@@ -105,6 +142,9 @@ private:
         bool bFromSweep, const FHitResult& SweepHitResult);
 
     void ApplyDamageTo(AActor* Target, const FVector& HitLocation);
+    void SendHitGameplayEvent(AActor* Target, UAbilitySystemComponent* SourceASC);
+    void SpawnProjectileVisualNiagara();
+    void SpawnBurstNiagara(UNiagaraSystem* NiagaraSystem, const FVector& WorldLocation, const FVector& Scale) const;
     void ScheduleInitialOverlapCheck();
     void HandleInitialOverlaps();
     void ResolveCombatDeckOnHit();
diff --git a/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp b/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
index b3cacdee..13e2110a 100644
--- a/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
+++ b/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
@@ -1095,6 +1095,7 @@ TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryPanel()
 	AddFilter(LOCTEXT("NodeLibraryFilterCondition", "条件节点"), ENodeLibraryFilter::Condition);
 	AddFilter(LOCTEXT("NodeLibraryFilterPresentation", "表现节点"), ENodeLibraryFilter::Presentation);
 	AddFilter(LOCTEXT("NodeLibraryFilterLifecycle", "生命周期"), ENodeLibraryFilter::Lifecycle);
+	AddFilter(LOCTEXT("NodeLibraryFilterPure", "数据节点"), ENodeLibraryFilter::Pure);
 
 	TSharedRef<SWrapBox> NodeWrap = SNew(SWrapBox).UseAllottedSize(true);
 	auto AddNode = [this, &NodeWrap](ENodeLibraryFilter Filter, UClass* NodeClass, const FText& DisplayName, const FText& Description)
@@ -1119,7 +1120,6 @@ TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryPanel()
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectCost::StaticClass(), LOCTEXT("NodeEffectCostName", "消耗"), LOCTEXT("NodeEffectCostDescription", "扣除法力、能量、弹药等资源。"));
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAttributeModify::StaticClass(), LOCTEXT("NodeEffectAttributeModifyName", "属性修改"), LOCTEXT("NodeEffectAttributeModifyDescription", "修改属性数值，适合增益、减益、护盾等持续效果。"));
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectApplyState::StaticClass(), LOCTEXT("NodeEffectApplyStateName", "施加状态"), LOCTEXT("NodeEffectApplyStateDescription", "施加燃烧、中毒、流血、撕裂、诅咒等 GameplayEffect。"));
-	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectApplyProfile::StaticClass(), LOCTEXT("NodeEffectApplyProfileName", "效果配置"), LOCTEXT("NodeEffectApplyProfileDescription", "使用可复用 Rune Effect Profile 执行通用效果。"));
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectApplyInRadius::StaticClass(), LOCTEXT("NodeEffectApplyInRadiusName", "范围施加GE"), LOCTEXT("NodeEffectApplyInRadiusDescription", "向半径内目标施加 GameplayEffect。"));
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAreaDamage::StaticClass(), LOCTEXT("NodeEffectAreaDamageName", "范围伤害"), LOCTEXT("NodeEffectAreaDamageDescription", "对范围目标造成伤害，适合燃烧地面、爆炸等。"));
 	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAddTag::StaticClass(), LOCTEXT("NodeEffectAddTagName", "添加Tag"), LOCTEXT("NodeEffectAddTagDescription", "给目标或拥有者添加状态 Tag。"));
@@ -1130,11 +1130,9 @@ TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryPanel()
 	AddNode(ENodeLibraryFilter::Task, UYogFlowNode_TaskEndSkill::StaticClass(), LOCTEXT("NodeTaskEndSkillName", "结束技能"), LOCTEXT("NodeTaskEndSkillDescription", "结束当前技能流程。"));
 	AddNode(ENodeLibraryFilter::Task, UYogFlowNode_TaskPlayAnimation::StaticClass(), LOCTEXT("NodeTaskPlayAnimationName", "动画"), LOCTEXT("NodeTaskPlayAnimationDescription", "播放技能动作或蒙太奇。"));
 
-	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnProjectileProfile::StaticClass(), LOCTEXT("NodeSpawnProjectileProfileName", "生成投射物配置"), LOCTEXT("NodeSpawnProjectileProfileDescription", "生成配置化投射物，适合月光、穿透、弹幕。"));
 	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnAreaProfile::StaticClass(), LOCTEXT("NodeSpawnAreaProfileName", "生成区域配置"), LOCTEXT("NodeSpawnAreaProfileDescription", "生成可配置区域，适合燃烧圈、毒区、领域。"));
 	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnGroundPath::StaticClass(), LOCTEXT("NodeSpawnGroundPathName", "生成地面路径"), LOCTEXT("NodeSpawnGroundPathDescription", "生成路径类地面效果，适合燃烧轨迹或月光路径。"));
-	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnRangedProjectiles::StaticClass(), LOCTEXT("NodeSpawnRangedProjectilesName", "生成远程弹幕"), LOCTEXT("NodeSpawnRangedProjectilesDescription", "生成多枚远程投射物。"));
-	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnSlashWave::StaticClass(), LOCTEXT("NodeSpawnSlashWaveName", "生成斩击波"), LOCTEXT("NodeSpawnSlashWaveDescription", "生成近战斩波，适合终结技或月光剑气。"));
+	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnRangedProjectiles::StaticClass(), LOCTEXT("NodeSpawnRangedProjectilesName", "生成远程弹幕"), LOCTEXT("NodeSpawnRangedProjectilesDescription", "生成多枚远程投射物，支持连招增加数量和命中事件。"));
 
 	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionAttributeCompare::StaticClass(), LOCTEXT("NodeConditionAttributeCompareName", "属性比较"), LOCTEXT("NodeConditionAttributeCompareDescription", "比较属性数值，并按结果分支。"));
 	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionHasTag::StaticClass(), LOCTEXT("NodeConditionHasTagName", "拥有Tag"), LOCTEXT("NodeConditionHasTagDescription", "判断目标或拥有者是否有燃烧、中毒、月光等 Tag。"));
@@ -1145,11 +1143,12 @@ TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryPanel()
 	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationPlayVFX::StaticClass(), LOCTEXT("NodePresentationPlayVFXName", "Niagara特效"), LOCTEXT("NodePresentationPlayVFXDescription", "播放 Niagara 表现。"));
 	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationCueOnActor::StaticClass(), LOCTEXT("NodePresentationCueOnActorName", "Cue到角色"), LOCTEXT("NodePresentationCueOnActorDescription", "在角色身上触发 GameplayCue。"));
 	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationCueAtLocation::StaticClass(), LOCTEXT("NodePresentationCueAtLocationName", "Cue到位置"), LOCTEXT("NodePresentationCueAtLocationDescription", "在世界位置触发 GameplayCue。"));
-	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationVFXProfile::StaticClass(), LOCTEXT("NodePresentationVFXProfileName", "VFX配置"), LOCTEXT("NodePresentationVFXProfileDescription", "使用可复用 VFX Profile 触发表现。"));
 	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationFlipbook::StaticClass(), LOCTEXT("NodePresentationFlipbookName", "序列帧特效"), LOCTEXT("NodePresentationFlipbookDescription", "播放 Flipbook 类表现。"));
 
 	AddNode(ENodeLibraryFilter::Lifecycle, UYogFlowNode_LifecycleDelay::StaticClass(), LOCTEXT("NodeLifecycleDelayName", "延迟"), LOCTEXT("NodeLifecycleDelayDescription", "延迟后继续流程。"));
 	AddNode(ENodeLibraryFilter::Lifecycle, UYogFlowNode_LifecycleFinishBuff::StaticClass(), LOCTEXT("NodeLifecycleFinishBuffName", "结束符文"), LOCTEXT("NodeLifecycleFinishBuffDescription", "主动结束当前符文 Buff 生命周期。"));
+	AddNode(ENodeLibraryFilter::Pure, UBFNode_Pure_TuningValue::StaticClass(), LOCTEXT("NodePureTuningName", "读取数值"), LOCTEXT("NodePureTuningDesc", "输出数值表中某个 Key 的值，无执行引脚，拖线连接到效果节点的参数槽。"));
+	AddNode(ENodeLibraryFilter::Pure, UBFNode_Pure_ComboIndex::StaticClass(), LOCTEXT("NodePureComboName", "连击段数"), LOCTEXT("NodePureComboDesc", "输出当前连击段数，无执行引脚，可连接到伤害倍率等数值槽。"));
 
 	return SNew(SVerticalBox)
 		+ SVerticalBox::Slot()
@@ -1611,7 +1610,7 @@ TSharedRef<SWidget> SRuneEditorWidget::BuildComboRecipePanel()
 				.FillWidth(0.22f)
 				[
 					SNew(STextBlock)
-					.Text(LOCTEXT("ColNeighborTag", "邻接卡 ID 标签"))
+					.Text(LOCTEXT("ColNeighborTag", "邻接条件标签"))
 					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
 				]
 				+ SHorizontalBox::Slot()
@@ -1726,6 +1725,25 @@ TSharedRef<ITableRow> SRuneEditorWidget::BuildComboRecipeRow(FComboRecipeRowPtr
 				]
 				+ SHorizontalBox::Slot()
 				.AutoWidth()
+				.VAlign(VAlign_Center)
+				.Padding(2.f, 0.f, 0.f, 0.f)
+				[
+					SNew(SButton)
+					.Text(FText::FromString(TEXT("→")))
+					.ToolTipText(LOCTEXT("OpenLinkFlowTip", "在流程图面板中打开此连携 FA"))
+					.IsEnabled_Lambda([Row, bForward]()
+					{
+						const int32 Idx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
+						return Idx > 0;
+					})
+					.OnClicked_Lambda([this, Row, bForward]()
+					{
+						const int32 Idx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
+						return OnOpenComboLinkFlowClicked(Idx);
+					})
+				]
+				+ SHorizontalBox::Slot()
+				.AutoWidth()
 				.Padding(4.f, 0.f, 0.f, 0.f)
 				[
 					SNew(SBox)
@@ -1775,13 +1793,52 @@ TSharedRef<ITableRow> SRuneEditorWidget::BuildComboRecipeRow(FComboRecipeRowPtr
 			.VAlign(VAlign_Top)
 			.Padding(0.f, 0.f, 4.f, 0.f)
 			[
-				SNew(SEditableTextBox)
-				.HintText(LOCTEXT("NeighborTagHint", "Card.ID.XXX"))
-				.Text_Lambda([Row]() { return FText::FromString(Row->NeighborTagString); })
-				.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type)
-				{
-					Row->NeighborTagString = NewText.ToString();
-				})
+				SNew(SVerticalBox)
+				+ SVerticalBox::Slot()
+				.AutoHeight()
+				.Padding(0.f, 0.f, 0.f, 3.f)
+				[
+					SNew(SHorizontalBox)
+					+ SHorizontalBox::Slot()
+					.AutoWidth()
+					.VAlign(VAlign_Center)
+					[
+						SNew(SCheckBox)
+						.IsChecked_Lambda([Row]()
+						{
+							return Row->bUseEffectTag ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
+						})
+						.OnCheckStateChanged_Lambda([Row](ECheckBoxState NewState)
+						{
+							Row->bUseEffectTag = NewState == ECheckBoxState::Checked;
+						})
+					]
+					+ SHorizontalBox::Slot()
+					.AutoWidth()
+					.VAlign(VAlign_Center)
+					.Padding(4.f, 0.f, 0.f, 0.f)
+					[
+						SNew(STextBlock)
+						.Text_Lambda([Row]()
+						{
+							return Row->bUseEffectTag
+								? LOCTEXT("NeighborEffectTagMode", "效果Tag")
+								: LOCTEXT("NeighborIdTagMode", "ID Tag");
+						})
+						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
+					]
+				]
+				+ SVerticalBox::Slot()
+				.AutoHeight()
+				[
+					SNew(SEditableTextBox)
+					.HintText(LOCTEXT("NeighborTagHint", "Card.Effect.XXX / Card.ID.XXX"))
+					.Text_Lambda([Row]() { return FText::FromString(Row->NeighborTagString); })
+					.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type)
+					{
+						Row->NeighborTagString = NewText.ToString();
+					})
+				]
 			]
 			+ SHorizontalBox::Slot()
 			.FillWidth(0.37f)
@@ -1861,17 +1918,18 @@ void SRuneEditorWidget::RefreshComboRecipeRows()
 		return;
 	}
 
-	auto FindOrAddRow = [this](const FString& TagStr) -> FComboRecipeRowPtr
+	auto FindOrAddRow = [this](const FString& TagStr, bool bUseEffectTag) -> FComboRecipeRowPtr
 	{
 		for (const FComboRecipeRowPtr& Row : ComboRecipeRows)
 		{
-			if (Row->NeighborTagString == TagStr)
+			if (Row->NeighborTagString == TagStr && Row->bUseEffectTag == bUseEffectTag)
 			{
 				return Row;
 			}
 		}
 		FComboRecipeRowPtr NewRow = MakeShared<FComboRecipeEditorRow>();
 		NewRow->NeighborTagString = TagStr;
+		NewRow->bUseEffectTag = bUseEffectTag;
 		ComboRecipeRows.Add(NewRow);
 		return NewRow;
 	};
@@ -1896,12 +1954,19 @@ void SRuneEditorWidget::RefreshComboRecipeRows()
 	for (const FCombatCardLinkRecipe& Recipe : Rune->RuneInfo.CombatCard.LinkRecipes)
 	{
 		FString TagStr;
-		if (!Recipe.Condition.RequiredNeighborIdTags.IsEmpty())
+		bool bUseEffectTag = true;
+		if (!Recipe.Condition.RequiredNeighborEffectTags.IsEmpty())
+		{
+			TagStr = Recipe.Condition.RequiredNeighborEffectTags.First().ToString();
+			bUseEffectTag = true;
+		}
+		else if (!Recipe.Condition.RequiredNeighborIdTags.IsEmpty())
 		{
 			TagStr = Recipe.Condition.RequiredNeighborIdTags.First().ToString();
+			bUseEffectTag = false;
 		}
 
-		FComboRecipeRowPtr Row = FindOrAddRow(TagStr);
+		FComboRecipeRowPtr Row = FindOrAddRow(TagStr, bUseEffectTag);
 
 		if (Recipe.Direction == ECombatCardLinkOrientation::Forward)
 		{
@@ -1959,7 +2024,14 @@ FReply SRuneEditorWidget::OnSaveComboRecipesClicked()
 				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Row->NeighborTagString), false);
 				if (Tag.IsValid())
 				{
-					Recipe.Condition.RequiredNeighborIdTags.AddTag(Tag);
+					if (Row->bUseEffectTag)
+					{
+						Recipe.Condition.RequiredNeighborEffectTags.AddTag(Tag);
+					}
+					else
+					{
+						Recipe.Condition.RequiredNeighborIdTags.AddTag(Tag);
+					}
 				}
 			}
 
@@ -2953,14 +3025,15 @@ void SRuneEditorWidget::BindGraphEditorCommands()
 		FCanExecuteAction::CreateSP(this, &SRuneEditorWidget::CanDeleteSelectedGraphNodes));
 }
 
-void SRuneEditorWidget::RebuildGraphEditor()
+void SRuneEditorWidget::RebuildGraphEditor(UFlowAsset* OverrideFlow)
 {
 	if (!GraphEditorContainer.IsValid())
 	{
 		return;
 	}
 
-	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
+	UFlowAsset* FlowAsset = OverrideFlow ? OverrideFlow : GetSelectedFlowAsset();
+	DisplayedFlowAsset = FlowAsset;
 	UEdGraph* GraphToEdit = FlowAsset ? FlowAsset->GetGraph() : nullptr;
 	if (!GraphToEdit)
 	{
@@ -3420,6 +3493,21 @@ FReply SRuneEditorWidget::OnOpenFlowClicked() const
 	return FReply::Handled();
 }
 
+FReply SRuneEditorWidget::OnOpenComboLinkFlowClicked(int32 FlowIdx)
+{
+	if (FlowIdx <= 0 || !FlowAssetDataList.IsValidIndex(FlowIdx - 1))
+	{
+		return FReply::Handled();
+	}
+	UObject* Asset = FlowAssetDataList[FlowIdx - 1].GetAsset();
+	if (UFlowAsset* FA = Cast<UFlowAsset>(Asset))
+	{
+		OnCenterTabSelected(ECenterPanelTab::FlowGraph);
+		RebuildGraphEditor(FA);
+	}
+	return FReply::Handled();
+}
+
 FReply SRuneEditorWidget::SelectFlowNode(UFlowNode* FlowNode)
 {
 	SelectedFlowNode = FlowNode;
diff --git a/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h b/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h
index 389d6dfb..fdd827d3 100644
--- a/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h
+++ b/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h
@@ -54,6 +54,7 @@ struct FRuneEditorTuningRow
 struct FComboRecipeEditorRow
 {
 	FString NeighborTagString;
+	bool bUseEffectTag = true;
 
 	bool bHasForward = false;
 	int32 ForwardFlowIdx = 0;
@@ -110,7 +111,8 @@ private:
 		Spawn,
 		Condition,
 		Presentation,
-		Lifecycle
+		Lifecycle,
+		Pure
 	};
 
 	enum class EResourceFilter : uint8
@@ -155,7 +157,7 @@ private:
 	void RefreshFlowAssetOptions();
 	TSharedRef<SWidget> BuildTuningCategoryFilterBar();
 	void BindGraphEditorCommands();
-	void RebuildGraphEditor();
+	void RebuildGraphEditor(UFlowAsset* OverrideFlow = nullptr);
 	void OnSearchTextChanged(const FText& NewText);
 	void OnRuneSelectionChanged(FRuneRowPtr Row, ESelectInfo::Type SelectInfo);
 	void OnGraphSelectionChanged(const TSet<UObject*>& Nodes);
@@ -188,6 +190,7 @@ private:
 	FReply OnRunRuneClicked();
 	FReply OnOpenRuneClicked() const;
 	FReply OnOpenFlowClicked() const;
+	FReply OnOpenComboLinkFlowClicked(int32 FlowIdx);
 	FReply SelectFlowNode(UFlowNode* FlowNode);
 	void DeleteSelectedGraphNodes();
 	bool CanDeleteSelectedGraphNodes() const;
@@ -255,6 +258,7 @@ private:
 	TWeakObjectPtr<UObject> CopiedResource;
 	TWeakObjectPtr<URuneDataAsset> SelectedRune;
 	TWeakObjectPtr<UFlowNode> SelectedFlowNode;
+	TWeakObjectPtr<UFlowAsset> DisplayedFlowAsset;
 	FString SearchText;
 	FText StatusText;
 	FText RunFeedbackText;
warning: in the working copy of 'Source/DevKit/Private/Projectile/MusketBullet.cpp', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/Projectile/MusketBullet.h', LF will be replaced by CRLF the next time Git touches it

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git diff -- Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h Source/DevKit/Public/BuffFlow/Nodes/BFNode_PureData.h Source/DevKit/Private/BuffFlow/Nodes/BFNode_PureData.cpp' in D:\Self\GItGame\Dev01
 succeeded in 498ms:
diff --git a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h
index ffafad4f..53a3deba 100644
--- a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h
+++ b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h
@@ -11,47 +11,124 @@ class AMusketBullet;
 class UGameplayEffect;
 
 /**
- * Spawns extra ranged projectiles for combat-card effects such as Split.
- * The projectiles inherit the current combat-card attack GUID so they do not consume extra cards.
+ * 向目标方向发射一组远程弹丸。
+ * 弹丸继承当前战斗卡攻击实例 GUID，不额外消耗卡牌资源。
+ *
+ * 表现层分离原则：
+ *   - 弹丸飞行特效（轨迹/粒子）由弹丸 BP 类自身持有，不在本节点配置。
+ *   - 命中特效与命中回调请在流程图中通过「等待事件 → 特效表现」节点链实现。
  */
 UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Ranged Projectiles", Category = "BuffFlow|Projectile"))
 class DEVKIT_API UBFNode_SpawnRangedProjectiles : public UBFNode_Base
 {
 	GENERATED_UCLASS_BODY()
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// ---- 投射物基础 ----
+
+	// 发射来源 — 投射物从哪个角色的枪口位置发射（通常选 BuffOwner 即玩家自身）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "发射来源"))
 	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// 弹丸类（兜底）— 未启用「优先卡牌投射物类」时，或卡牌 DA 未配置投射物时使用此类
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "弹丸类（兜底）"))
 	TSubclassOf<AMusketBullet> BulletClass;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// 伤害效果类（兜底）— 未启用「优先卡牌伤害效果类」时，或卡牌 DA 未配置 GE 时使用此类
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "伤害效果类（兜底）"))
 	TSubclassOf<UGameplayEffect> DamageEffectClass;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// 优先卡牌投射物类 — 勾选后使用战斗卡 DA 上配置的弹丸类（推荐保持勾选）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "优先卡牌投射物类"))
+	bool bPreferCombatCardProjectileClass = true;
+
+	// 优先卡牌伤害效果类 — 勾选后使用战斗卡 DA 上配置的命中伤害 GE（推荐保持勾选）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "优先卡牌伤害效果类"))
+	bool bPreferCombatCardDamageEffectClass = true;
+
+	// 枪口插槽名 — 发射位置优先取角色骨骼/武器模型上同名插槽的世界坐标
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "枪口插槽名"))
 	FName MuzzleSocketName = TEXT("Muzzle");
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// 手动偏航角列表 — 每枚投射物的水平偏转角（度）；仅在不启用「数量模式」时生效
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "手动偏航角列表"))
 	TArray<float> YawOffsets;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// ---- 弹幕模式 ----
+
+	// 启用数量模式 — 勾选后改用「发射数量 + 散布锥角」均匀分布，取代手动偏航角列表
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (DisplayName = "启用数量模式"))
+	bool bUseProjectileCountPattern = false;
+
+	// 发射数量 — 启用数量模式时，单次发射的基础弹丸枚数（最小 1）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (ClampMin = "1", DisplayName = "发射数量"))
+	int32 ProjectileCount = 1;
+
+	// 散布锥角（度）— 多枚弹丸的水平展开角；0 = 全部正前方，90 = 90 度扇形
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|弹幕模式", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "散布锥角（度）"))
+	float ProjectileConeAngleDegrees = 0.f;
+
+	// ---- 伤害 ----
+
+	// 使用卡牌攻击力 — 勾选后伤害值来自战斗卡攻击力属性；否则使用下方「固定伤害值」
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "使用卡牌攻击力"))
 	bool bUseCombatCardAttackDamage = true;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "!bUseCombatCardAttackDamage"))
+	// 固定伤害值 — 不使用卡牌攻击力时生效；可连接 Pure 数据节点（读取数值）的输出引脚
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "!bUseCombatCardAttackDamage", DisplayName = "固定伤害值"))
 	FFlowDataPinInputProperty_Float Damage;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// ---- 连击追加 ----
+
+	// 连击追加发射数 — 勾选后：当前连击层数 × 每层追加数 = 额外多发弹丸枚数
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (DisplayName = "连击追加发射数"))
+	bool bAddComboStacksToProjectileCount = false;
+
+	// 每层连击追加数 — 每增加一层连击，额外多发几枚弹丸
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides, DisplayName = "每层连击追加数"))
+	int32 ProjectilesPerComboStack = 1;
+
+	// 最大追加上限 — 连击追加的弹丸总数上限（0 = 无上限）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|连击追加", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides, DisplayName = "最大追加上限"))
+	int32 MaxBonusProjectiles = 0;
+
+	// ---- 攻击实例 ----
+
+	// 共享攻击 GUID — 勾选后与主攻击共享同一 GUID，额外弹丸不额外消耗卡牌次数
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "共享攻击 GUID"))
 	bool bShareAttackInstanceGuid = true;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile")
+	// ---- 武器检查 ----
+
+	// 需要远程武器 Tag — 勾选后角色必须拥有指定 Tag 才可发射，否则触发 Failed 引脚
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (DisplayName = "需要远程武器 Tag"))
 	bool bRequireRangedWeaponTag = true;
 
-	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "bRequireRangedWeaponTag"))
+	// 所需武器 Tag — 勾选「需要远程武器 Tag」后，具体检查的 Tag（默认 Weapon.Type.Ranged）
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile", meta = (EditCondition = "bRequireRangedWeaponTag", DisplayName = "所需武器 Tag"))
 	FGameplayTag RequiredWeaponTag;
 
+	// ---- 命中事件（用于流程图回调） ----
+
+	// 命中事件 Tag — 弹丸命中时向 ASC 发送的 Gameplay 事件 Tag，供同 FA 内「等待事件」节点接收
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (DisplayName = "命中事件 Tag"))
+	FGameplayTag HitGameplayEventTag;
+
+	// 事件发给攻击来源 — 勾选后事件发给发射者（玩家）ASC，等待事件节点设置 Target = BuffOwner 即可接收
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid()", EditConditionHides, DisplayName = "事件发给攻击来源"))
+	bool bSendHitGameplayEventToSourceASC = true;
+
+	// 用伤害量作为事件强度 — Magnitude = 命中实际伤害值，供下游节点通过事件读取伤害数值
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid()", EditConditionHides, DisplayName = "用伤害量作为事件强度"))
+	bool bUseDamageAsHitGameplayEventMagnitude = true;
+
+	// 命中事件强度 — 不用伤害量时，手动指定的 Magnitude 值；可连接 Pure 数据节点输出
+	UPROPERTY(EditAnywhere, Category = "Ranged Projectile|命中事件", meta = (EditCondition = "HitGameplayEventTag.IsValid() && !bUseDamageAsHitGameplayEventMagnitude", EditConditionHides, DisplayName = "命中事件强度"))
+	FFlowDataPinInputProperty_Float HitGameplayEventMagnitude;
+
 protected:
 	virtual void ExecuteInput(const FName& PinName) override;
 
 private:
+	TArray<float> BuildResolvedYawOffsets(int32 ComboBonusStacks) const;
 	FVector ResolveMuzzleLocation(ACharacter* SourceCharacter) const;
 };
diff --git a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h
index be45bdbd..18d4b717 100644
--- a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h
+++ b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h
@@ -15,85 +15,120 @@ class DEVKIT_API UBFNode_SpawnRuneGroundPathEffect : public UBFNode_Base
 	GENERATED_UCLASS_BODY()
 
 public:
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
+	// 施加的效果类 — 路径区域内命中目标时施加的 GameplayEffect
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "施加的效果类"))
 	TSubclassOf<UGameplayEffect> Effect;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
+	// 命中目标策略 — 只命中敌人/只命中友方/全部
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "命中目标策略"))
 	ERuneGroundPathTargetPolicy TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
+	// 区域形状 — 矩形/扇形等碰撞区域形状
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "区域形状"))
 	ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Fallback facing mode when Spawn Location Override / Spawn Rotation Override are not linked. Prefer using Calc Rune Ground Path Transform for authored flows."))
+	// 朝向模式 — 无覆盖引脚时的默认朝向计算方式（推荐用 CalcRuneGroundPathTransform 节点替代）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "朝向模式",
+		ToolTip = "Fallback facing mode when Spawn Location Override / Spawn Rotation Override are not linked. Prefer using Calc Rune Ground Path Transform for authored flows."))
 	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Optional data pin. If linked, this exact world location is used instead of the internal position calculation."))
+	// 生成位置覆盖 — 可选数据引脚；连线后直接使用此世界坐标，不再自动计算
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "生成位置覆盖",
+		ToolTip = "Optional data pin. If linked, this exact world location is used instead of the internal position calculation."))
 	FFlowDataPinInputProperty_Vector SpawnLocationOverride;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (ToolTip = "Optional data pin. If linked, this exact world rotation is used instead of the internal facing calculation."))
+	// 生成朝向覆盖 — 可选数据引脚；连线后直接使用此世界旋转，不再自动计算
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "生成朝向覆盖",
+		ToolTip = "Optional data pin. If linked, this exact world rotation is used instead of the internal facing calculation."))
 	FFlowDataPinInputProperty_Rotator SpawnRotationOverride;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
+	// 以路径长度为中心 — 勾选后生成位置为路径区域的中心点
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "以路径长度为中心"))
 	bool bCenterOnPathLength = true;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position")
+	// 偏航角偏移 — 在计算朝向后额外旋转的角度（度）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Position", meta = (DisplayName = "偏航角偏移"))
 	float RotationYawOffset = 0.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
+	// 持续时间（秒）— 路径效果的存活时长
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01", DisplayName = "持续时间（秒）"))
 	float Duration = 3.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01"))
+	// 触发间隔（秒）— 路径区域每隔多少秒检测并施加效果一次
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "0.01", DisplayName = "触发间隔（秒）"))
 	float TickInterval = 1.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
+	// 路径长度（cm）— 区域沿前方的延伸长度
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径长度（cm）"))
 	float Length = 520.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
+	// 路径宽度（cm）— 区域的横向宽度
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径宽度（cm）"))
 	float Width = 220.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0"))
+	// 路径高度（cm）— 区域的垂直高度
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (ClampMin = "1.0", DisplayName = "路径高度（cm）"))
 	float Height = 120.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1.0", ToolTip = "Decal projection depth in cm. Keep this shallow so the path decal stays on the floor instead of projecting up onto characters."))
+	// 贴花投影深度（cm）— 地面贴花的投影厚度，保持较浅可防止贴花投影到角色身上
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1.0", DisplayName = "贴花投影深度（cm）",
+		ToolTip = "Decal projection depth in cm. Keep this shallow so the path decal stays on the floor instead of projecting up onto characters."))
 	float DecalProjectionDepth = 18.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ToolTip = "Rotates only the decal texture/mask on the floor. This does not rotate the damage/collision area. Try 0/90/180/270 if the decal visual direction does not match the yellow debug area."))
+	// 贴花平面旋转（度）— 只旋转贴花纹理方向，不影响碰撞区域；试试 0/90/180/270
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "贴花平面旋转（度）",
+		ToolTip = "Rotates only the decal texture/mask on the floor. This does not rotate the damage/collision area. Try 0/90/180/270 if the decal visual direction does not match the yellow debug area."))
 	float DecalPlaneRotationDegrees = 0.0f;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
+	// 生成位置偏移 — 相对于来源角色的局部偏移（X=前方，Y=右方，Z=上方）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "生成位置偏移"))
 	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path")
+	// 效果来源 — 路径中心计算的参考角色
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path", meta = (DisplayName = "效果来源"))
 	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
+	// 贴花材质 — 显示在地面上的区域指示贴花
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "贴花材质"))
 	TObjectPtr<UMaterialInterface> DecalMaterial;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
+	// Niagara 粒子系统 — 路径区域的持续粒子表现
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "Niagara 粒子系统"))
 	TObjectPtr<UNiagaraSystem> NiagaraSystem;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual")
+	// Niagara 缩放 — 粒子系统的缩放比例
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (DisplayName = "Niagara 缩放"))
 	FVector NiagaraScale = FVector(0.5f, 0.5f, 0.35f);
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1", ClampMax = "12", ToolTip = "Number of Niagara instances distributed along the path. Fire paths use multiple small instances to read as a ground strip."))
+	// Niagara 实例数量 — 沿路径分布的粒子实例数（火焰路径通常需要多个小实例）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Visual", meta = (ClampMin = "1", ClampMax = "12", DisplayName = "Niagara 实例数量",
+		ToolTip = "Number of Niagara instances distributed along the path. Fire paths use multiple small instances to read as a ground strip."))
 	int32 NiagaraInstanceCount = 1;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Tag 1 (SetByCaller)", ToolTip = "GameplayTag used by the GameplayEffect execution. Burn paths should use Data.Damage.Burn."))
+	// 伤害Tag1（SetByCaller）— GE执行时使用的Tag（如 Data.Damage.Burn）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害Tag1（SetByCaller）",
+		ToolTip = "GameplayTag used by the GameplayEffect execution. Burn paths should use Data.Damage.Burn."))
 	FGameplayTag SetByCallerTag1;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "Damage Value 1 / Tick", ToolTip = "Designer-facing damage value passed to SetByCallerTag1. For UGE_RuneBurn this is the burn damage per periodic tick."))
+	// 每Tick伤害数值1 — 传入 SetByCallerTag1 对应槽的实际伤害值（每次触发）
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "每Tick伤害数值1",
+		ToolTip = "Designer-facing damage value passed to SetByCallerTag1. For UGE_RuneBurn this is the burn damage per periodic tick."))
 	FFlowDataPinInputProperty_Float SetByCallerValue1 = FFlowDataPinInputProperty_Float(0.0f);
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
+	// 伤害Tag2
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害Tag2"))
 	FGameplayTag SetByCallerTag2;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
+	// 伤害数值2
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "伤害数值2"))
 	FFlowDataPinInputProperty_Float SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.0f);
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (ClampMin = "1"))
+	// 施加次数 — 每次触发时对目标施加GE的次数
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (ClampMin = "1", DisplayName = "施加次数"))
 	int32 ApplicationCount = 1;
 
-	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect")
+	// 每目标只施加一次 — 勾选后同一目标在路径存活期间只会被施加一次GE
+	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ground Path|Effect", meta = (DisplayName = "每目标只施加一次"))
 	bool bApplyOncePerTarget = false;
 
 	virtual void ExecuteInput(const FName& PinName) override;
diff --git a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h
index 3b6160de..e4847b88 100644
--- a/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h
+++ b/Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h
@@ -12,201 +12,279 @@ class UGameplayEffect;
 class UNiagaraSystem;
 
 /**
- * Spawns a configurable slash-wave projectile. Used by combat cards such as Moonlight.
+ * 生成一个可配置的斩波投射物。
+ * 表现层分离原则：飞行/命中/消亡的 Niagara 特效可直接在本节点配置（因斩波是独立投射物 Actor，
+ * 与远程弹幕不同，无法通过"等待命中→表现节点"模式处理），但命中回调事件仍走 EventTag 通知流程图。
  */
 UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Slash Wave Projectile", Category = "BuffFlow|Projectile"))
 class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
 {
 	GENERATED_UCLASS_BODY()
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// ---- 基础 ----
+
+	// 投射物类 — 斩波 Actor 类（BP 子类或默认 C++ 类）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "投射物类"))
 	TSubclassOf<ASlashWaveProjectile> ProjectileClass;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 伤害效果类 — 命中时施加的 GameplayEffect
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害效果类"))
 	TSubclassOf<UGameplayEffect> DamageEffect;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 发射来源 — 斩波从哪个角色位置生成
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "发射来源"))
 	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 伤害值 — 斩波命中时的基础伤害
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害值"))
 	float Damage = 10.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 伤害日志类型 — 用于调试日志标识此次伤害来源
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "伤害日志类型"))
 	FName DamageLogType = TEXT("Rune_SlashWave");
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1.0"))
+	// 飞行速度（cm/s）— 斩波的移动速度
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1.0", DisplayName = "飞行速度（cm/s）"))
 	float Speed = 1400.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0"))
+	// 最大飞行距离（cm）— 超过此距离后斩波自动消亡
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0", DisplayName = "最大飞行距离（cm）"))
 	float MaxDistance = 800.f;
 
-	/** <= 0 means unlimited targets. Each actor can receive DamageApplicationsPerTarget hits. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 最大命中目标数 — <=0 表示无限制；每个目标可被命中 DamageApplicationsPerTarget 次
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "最大命中目标数"))
 	int32 MaxHitCount = 2;
 
-	/** Number of damage applications per target after the target first overlaps this wave. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1", ClampMax = "20"))
+	// 每目标命中次数 — 同一目标首次接触后允许被施加伤害的总次数
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1", ClampMax = "20", DisplayName = "每目标命中次数"))
 	int32 DamageApplicationsPerTarget = 1;
 
-	/** Delay between repeated damage applications on the same target. <= 0 applies repeats immediately. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0"))
+	// 命中间隔（秒）— 对同一目标重复施加伤害的最小时间间隔；<=0 表示立即重复
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0", DisplayName = "命中间隔（秒）"))
 	float DamageApplicationInterval = 0.25f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// 碰撞盒半尺寸 — 斩波碰撞检测的半尺寸（XYZ 各半）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "碰撞盒半尺寸"))
 	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);
 
-	/** When true, larger CollisionBoxExtent also scales the projectile actor/VFX relative to the projectile default extent. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// ---- 外观 ----
+
+	// 随碰撞盒缩放外观 — 勾选后碰撞盒变大时，投射物的视觉比例同步缩放
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "随碰撞盒缩放外观"))
 	bool bScaleVisualWithCollisionExtent = true;
 
-	/** Extra visual scale multiplier after collision-based scale is applied. Does not change the final configured collision extent. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 外观额外缩放 — 在碰撞缩放基础上叠加的额外视觉缩放（不影响碰撞区域）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "外观额外缩放"))
 	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);
 
-	/** Optional Niagara used as the projectile's main blade visual. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 飞行粒子系统 — 斩波飞行中的主体视觉 Niagara 效果
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "飞行粒子系统"))
 	TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem = nullptr;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 飞行粒子缩放
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "飞行粒子缩放"))
 	FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
 
-	/** Hide mesh/old BP visual components so only ProjectileVisualNiagaraSystem is visible. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 隐藏默认外观 — 勾选后隐藏投射物蓝图上的默认网格/组件，只显示飞行粒子系统
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "隐藏默认外观"))
 	bool bHideDefaultProjectileVisuals = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 命中粒子系统 — 斩波命中目标时播放的 Niagara 效果
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "命中粒子系统"))
 	TObjectPtr<UNiagaraSystem> HitNiagaraSystem = nullptr;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 命中粒子缩放
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "命中粒子缩放"))
 	FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 消亡粒子系统 — 斩波飞行结束/超出距离时播放的 Niagara 效果
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "消亡粒子系统"))
 	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem = nullptr;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
+	// 消亡粒子缩放
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual", meta = (DisplayName = "消亡粒子缩放"))
 	FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events")
+	// ---- 事件 ----
+
+	// 命中事件Tag — 斩波命中时向源 ASC 发送的 Gameplay 事件 Tag
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events", meta = (DisplayName = "命中事件Tag"))
 	FGameplayTag HitGameplayEventTag;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events")
+	// 消亡事件Tag — 斩波消亡时向源 ASC 发送的 Gameplay 事件 Tag
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events", meta = (DisplayName = "消亡事件Tag"))
 	FGameplayTag ExpireGameplayEventTag;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "1"))
+	// ---- 弹幕模式 ----
+
+	// 发射数量 — 单次发射的基础斩波枚数
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "1", DisplayName = "发射数量"))
 	int32 ProjectileCount = 1;
 
-	/** Adds projectiles from the combat-card combo stacks when this Flow was started by CombatDeck. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (DisplayName = "Add Combo Stacks To Projectile Count"))
+	// 连击追加发射数 — 勾选后：连击层数 × 每层追加数 = 额外多发枚数
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (DisplayName = "连击追加发射数"))
 	bool bAddComboStacksToProjectileCount = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
+	// 每层连击追加数 — 每增加一层连击额外多发几枚
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", DisplayName = "每层连击追加数",
+		EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
 	int32 ProjectilesPerComboStack = 1;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
+	// 最大追加上限 — 连击追加的斩波总数上限（0=无上限）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", DisplayName = "最大追加上限",
+		EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
 	int32 MaxBonusProjectiles = 0;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", ClampMax = "180.0"))
+	// 散布锥角（度）— 多枚斩波的水平展开角；0=全部正前方
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "散布锥角（度）"))
 	float ProjectileConeAngleDegrees = 0.f;
 
-	/** When true, extra projectiles are fired one after another along the same path instead of fanning out. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (DisplayName = "Spawn Projectiles Sequentially"))
+	// 顺序发射 — 勾选后多枚斩波沿同一路径依次生成而非扇形展开
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (DisplayName = "顺序发射"))
 	bool bSpawnProjectilesSequentially = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", EditCondition = "bSpawnProjectilesSequentially", EditConditionHides))
+	// 顺序发射间隔（秒）— 顺序模式下每枚斩波的生成时间间隔
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", DisplayName = "顺序发射间隔（秒）",
+		EditCondition = "bSpawnProjectilesSequentially", EditConditionHides))
 	float SequentialProjectileSpawnInterval = 0.12f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Collision")
+	// ---- 碰撞 ----
+
+	// 碰到静态物体销毁 — 勾选后斩波碰到 WorldStatic 时立即销毁（否则穿透）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Collision", meta = (DisplayName = "碰到静态物体销毁"))
 	bool bDestroyOnWorldStaticHit = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// ---- 伤害 ----
+
+	// 强制纯伤害 — 勾选后跳过护甲计算，直接对生命值扣血
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "强制纯伤害"))
 	bool bForcePureDamage = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
+	// 护甲伤害加成倍率 — 目标护甲值 × 此倍率 叠加到伤害（0=不启用）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲伤害加成倍率"))
 	float BonusArmorDamageMultiplier = 0.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// 叠加来源护甲到伤害 — 勾选后将发射者的护甲值转化为额外伤害
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "叠加来源护甲到伤害"))
 	bool bAddSourceArmorToDamage = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
+	// 护甲转伤害倍率 — 来源护甲 × 此倍率 = 额外伤害
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲转伤害倍率"))
 	float SourceArmorToDamageMultiplier = 1.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// 生成时消耗来源护甲 — 勾选后发射时消耗发射者一部分护甲值
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "生成时消耗来源护甲"))
 	bool bConsumeSourceArmorOnSpawn = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
+	// 护甲消耗倍率 — 消耗的护甲量 = 伤害 × 此倍率
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0", DisplayName = "护甲消耗倍率"))
 	float SourceArmorConsumeMultiplier = 1.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// 附加命中效果 — 命中时额外施加的 GameplayEffect（如附加灼烧）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加命中效果"))
 	TSubclassOf<UGameplayEffect> AdditionalHitEffect;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// 附加效果Tag — 附加命中效果使用的 SetByCaller Tag
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加效果Tag"))
 	FGameplayTag AdditionalHitSetByCallerTag;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
+	// 附加效果数值 — 传入附加命中效果的 SetByCaller 数值
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (DisplayName = "附加效果数值"))
 	float AdditionalHitSetByCallerValue = 0.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split")
+	// ---- 分裂 ----
+
+	// 首次命中分裂 — 勾选后斩波首次命中目标时分裂为多枚子弹
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "首次命中分裂"))
 	bool bSplitOnFirstHit = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0"))
+	// 最大分裂代数 — 子弹可继续分裂的最大层数（1=只有主弹分裂，子弹不再分裂）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0", DisplayName = "最大分裂代数"))
 	int32 MaxSplitGenerations = 1;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "1"))
+	// 分裂数量 — 每次分裂生成的子弹枚数
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "1", DisplayName = "分裂数量"))
 	int32 SplitProjectileCount = 3;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
+	// 分裂锥角（度）— 分裂子弹的水平展开角
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "分裂锥角（度）"))
 	float SplitConeAngleDegrees = 45.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (EditCondition = "bSplitOnFirstHit", EditConditionHides))
+	// 随机分裂方向 — 勾选后分裂子弹方向在锥角内随机散布
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "随机分裂方向",
+		EditCondition = "bSplitOnFirstHit", EditConditionHides))
 	bool bRandomizeSplitDirections = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
+	// 偏航随机范围（度）— 随机模式下水平方向的最大抖动角度
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", DisplayName = "偏航随机范围（度）",
+		EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
 	float SplitRandomYawJitterDegrees = 0.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "45.0", EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
+	// 俯仰随机范围（度）— 随机模式下垂直方向的最大抖动角度
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "45.0", DisplayName = "俯仰随机范围（度）",
+		EditCondition = "bSplitOnFirstHit && bRandomizeSplitDirections", EditConditionHides))
 	float SplitRandomPitchDegrees = 0.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
+	// 分裂伤害倍率 — 子弹伤害 = 主弹伤害 × 此倍率
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", DisplayName = "分裂伤害倍率"))
 	float SplitDamageMultiplier = 0.5f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.01"))
+	// 分裂速度倍率 — 子弹速度 = 主弹速度 × 此倍率
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.01", DisplayName = "分裂速度倍率"))
 	float SplitSpeedMultiplier = 2.f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
+	// 分裂最大距离倍率 — 子弹最大飞行距离 = 主弹距离 × 此倍率
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", DisplayName = "分裂最大距离倍率"))
 	float SplitMaxDistanceMultiplier = 0.6f;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split")
+	// 分裂碰撞盒倍率 — 子弹碰撞盒 = 主弹碰撞盒 × 此向量
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (DisplayName = "分裂碰撞盒倍率"))
 	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);
 
-	/** Enables one or more enemy-hit bounces only for split child projectiles. The parent projectile still only splits. */
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (EditCondition = "bSplitOnFirstHit", EditConditionHides))
+	// 分裂子弹命中弹跳 — 仅对分裂产生的子弹启用敌人命中后弹跳（主弹不受影响）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (DisplayName = "分裂子弹命中弹跳",
+		EditCondition = "bSplitOnFirstHit", EditConditionHides))
 	bool bBounceSplitChildrenOnEnemyHit = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (ClampMin = "0", EditCondition = "bSplitOnFirstHit && bBounceSplitChildrenOnEnemyHit", EditConditionHides))
+	// 子弹最大弹跳次数 — 分裂子弹可弹跳的最大次数（0=不限）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split|Bounce", meta = (ClampMin = "0", DisplayName = "子弹最大弹跳次数",
+		EditCondition = "bSplitOnFirstHit && bBounceSplitChildrenOnEnemyHit", EditConditionHides))
 	int32 SplitChildMaxEnemyBounces = 0;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave")
+	// ---- 生成位置 ----
+
+	// 生成位置偏移 — 相对于来源角色的局部偏移（X=前方，Z=上方）
+	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (DisplayName = "生成位置偏移"))
 	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// ---- 发射特效 ----
+
+	// 发射粒子系统 — 斩波生成瞬间在来源位置播放的 Niagara 效果
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子系统"))
 	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem = nullptr;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// 发射粒子效果名 — 供 DestroyNiagara 节点按名称精确销毁
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子效果名"))
 	FName LaunchNiagaraEffectName = NAME_None;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// 发射粒子附着到来源 — 勾选后发射粒子跟随来源角色移动；否则固定在世界坐标
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子附着到来源"))
 	bool bAttachLaunchNiagaraToSource = false;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// 发射粒子位置偏移 — 相对于生成点的额外偏移
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子位置偏移"))
 	FVector LaunchNiagaraOffset = FVector::ZeroVector;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// 发射粒子旋转偏移 — 发射粒子的额外旋转
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子旋转偏移"))
 	FRotator LaunchNiagaraRotationOffset = FRotator::ZeroRotator;
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// 发射粒子缩放
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "发射粒子缩放"))
 	FVector LaunchNiagaraScale = FVector(1.f, 1.f, 1.f);
 
-	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
+	// FA结束时销毁发射粒子 — 勾选后 FA 停止时立即销毁发射粒子实例
+	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX", meta = (DisplayName = "FA结束时销毁发射粒子"))
 	bool bDestroyLaunchNiagaraWithFlow = false;
 
 protected:
diff --git a/Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h b/Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h
index 5c1a7242..dcaf0d4c 100644
--- a/Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h
+++ b/Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h
@@ -34,9 +34,10 @@
 #include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
 #include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
 #include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
-#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
+#include "BuffFlow/Nodes/BFNode_PureData.h"
 #include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
 #include "Nodes/Graph/FlowNode_Finish.h"
+#include "Types/FlowDataPinResults.h"
 #include "YogFlowNodes.generated.h"
 
 UCLASS(NotBlueprintable, meta = (DisplayName = "流程控制", Category = "技能"))
@@ -267,15 +268,6 @@ public:
 	UYogFlowNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer);
 };
 
-UCLASS(NotBlueprintable, meta = (DisplayName = "生成斩击波", Category = "任务节点|生成"))
-class DEVKIT_API UYogFlowNode_SpawnSlashWave : public UBFNode_SpawnSlashWaveProjectile
-{
-	GENERATED_BODY()
-
-public:
-	UYogFlowNode_SpawnSlashWave(const FObjectInitializer& ObjectInitializer);
-};
-
 UCLASS(NotBlueprintable, meta = (DisplayName = "属性比较", Category = "条件节点"))
 class DEVKIT_API UYogFlowNode_ConditionAttributeCompare : public UBFNode_CompareFloat
 {
@@ -383,3 +375,29 @@ class DEVKIT_API UYogFlowNode_LifecycleFinishBuff : public UBFNode_FinishBuff
 public:
 	UYogFlowNode_LifecycleFinishBuff(const FObjectInitializer& ObjectInitializer);
 };
+
+// ---------------------------------------------------------------------------
+// Pure 数据节点 — 无执行引脚，仅供输出数值
+// ---------------------------------------------------------------------------
+
+UCLASS(NotBlueprintable, meta = (DisplayName = "读取数值（Pure）", Category = "Pure"))
+class DEVKIT_API UBFNode_Pure_TuningValue : public UBFNode_PureData
+{
+	GENERATED_UCLASS_BODY()
+
+	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "数值表中的 Key 名称"))
+	FName TuningKey;
+
+	UPROPERTY(EditAnywhere, Category = "Pure", meta = (ToolTip = "Key 不存在时的回退值"))
+	float DefaultValue = 0.f;
+
+	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
+};
+
+UCLASS(NotBlueprintable, meta = (DisplayName = "连击段数（Pure）", Category = "Pure"))
+class DEVKIT_API UBFNode_Pure_ComboIndex : public UBFNode_PureData
+{
+	GENERATED_UCLASS_BODY()
+
+	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
+};
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h', LF will be replaced by CRLF the next time Git touches it

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PureData.h'; Write-Output '---CPP---'; Get-Content -Raw 'Source/DevKit/Private/BuffFlow/Nodes/BFNode_PureData.cpp'" in D:\Self\GItGame\Dev01
 succeeded in 744ms:
#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PureData.generated.h"

/**
 * Pure data node base 鈥?no execution pins.
 * Subclasses supply data values via IFlowDataPinValueSupplierInterface.
 * Use OutputPins with non-Exec PinType to declare data output pins.
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API UBFNode_PureData : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteInput(const FName& PinName) override {}
};

---CPP---
#include "BuffFlow/Nodes/BFNode_PureData.h"

UBFNode_PureData::UBFNode_PureData(const FObjectInitializer& OI) : Super(OI)
{
	InputPins.Empty();
	OutputPins.Empty();
}


exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'rg "EFlowPinType::" Source/DevKit -n | Select-Object -First 100' in D:\Self\GItGame\Dev01
2026-05-09T08:40:01.290729Z ERROR codex_core::tools::router: error=Exit code: 1
Wall time: 1.1 seconds
Output:
rg : 无法将“rg”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。请检查名称的拼写，如果包括路径，请确保路径正确，然后再试一次。
所在位置 行:2 字符: 1
+ rg "EFlowPinType::" Source/DevKit -n | Select-Object -First 100
+ ~~
    + CategoryInfo          : ObjectNotFound: (rg:String) [], CommandNotFoundException
    + FullyQualifiedErrorId : CommandNotFoundException
 

 exited 1 in 1131ms:
rg : 无法将“rg”项识别为 cmdlet、函数、脚本文件或可运行程序的名称。请检查名称的拼写，如果包括路径，请确保路径正确，然后再试一次。
所在位置 行:2 字符: 1
+ rg "EFlowPinType::" Source/DevKit -n | Select-Object -First 100
+ ~~
    + CategoryInfo          : ObjectNotFound: (rg:String) [], CommandNotFoundException
    + FullyQualifiedErrorId : CommandNotFoundException
 

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit -Include *.h,*.cpp | Select-String -Pattern 'EFlowPinType::' | Select-Object -First 120 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 707ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\YogFlowNodes.cpp:336:OutputPins = { FFlowPin(FName("Value"), EFlowPinType::Float) };
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\YogFlowNodes.cpp:357:OutputPins = { FFlowPin(FName("ComboIndex"), EFlowPinType::Int) };

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Plugins,Source -Include *.h,*.cpp -ErrorAction SilentlyContinue | Select-String -Pattern 'enum.*EFlowPinType|EFlowPinType' | Select-Object -First 200 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 1043ms:
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:18:FLOW_API UEnum* Z_Construct_UEnum_Flow_EFlowPinType();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:99:{ "EditCondition", "PinType == EFlowPinType::Class" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:111:{ "EditCondition", "PinType == EFlowPinType::Object" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:123:{ "EditCondition", "PinType == EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:135:{ "EditCondition", "PinType == EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPin.gen.cpp:167:const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FFlowPin_Statics::NewProp_PinType = { "PinType", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FFlowPin, PinType), Z_Construct_UEnum_Flow_EFlowPinType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PinType_MetaData), NewProp_PinType_MetaData) }; // 2635112643
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:14:FLOW_API UEnum* Z_Construct_UEnum_Flow_EFlowPinType();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:18:// Begin Enum EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:19:static FEnumRegistrationInfo Z_Registration_Info_UEnum_EFlowPinType;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:20:static UEnum* EFlowPinType_StaticEnum()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:22:if (!Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:24:Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_Flow_EFlowPinType, (UObject*)Z_Construct_UPackage__Script_Flow(), TEXT("EFlowPinType"));
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:26:return Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:28:template<> FLOW_API UEnum* StaticEnum<EFlowPinType>()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:30:return EFlowPinType_StaticEnum();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:32:struct Z_Construct_UEnum_Flow_EFlowPinType_Statics
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:38:{ "Bool.Name", "EFlowPinType::Bool" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:41:{ "Class.Name", "EFlowPinType::Class" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:44:{ "Enum.Name", "EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:47:{ "Exec.Name", "EFlowPinType::Exec" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:50:{ "Float.Name", "EFlowPinType::Float" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:53:{ "GameplayTag.Name", "EFlowPinType::GameplayTag" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:56:{ "GameplayTagContainer.Name", "EFlowPinType::GameplayTagContainer" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:59:{ "InstancedStruct.Name", "EFlowPinType::InstancedStruct" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:62:{ "Int.Name", "EFlowPinType::Int" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:65:{ "Invalid.Name", "EFlowPinType::Invalid" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:67:{ "Max.Name", "EFlowPinType::Max" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:69:{ "Min.Name", "EFlowPinType::Min" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:72:{ "Name.Name", "EFlowPinType::Name" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:75:{ "Object.Name", "EFlowPinType::Object" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:78:{ "Rotator.Name", "EFlowPinType::Rotator" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:81:{ "String.Name", "EFlowPinType::String" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:84:{ "Text.Name", "EFlowPinType::Text" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:87:{ "Transform.Name", "EFlowPinType::Transform" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:90:{ "Vector.Name", "EFlowPinType::Vector" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:95:{ "EFlowPinType::Exec", (int64)EFlowPinType::Exec },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:96:{ "EFlowPinType::Bool", (int64)EFlowPinType::Bool },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:97:{ "EFlowPinType::Int", (int64)EFlowPinType::Int },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:98:{ "EFlowPinType::Float", (int64)EFlowPinType::Float },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:99:{ "EFlowPinType::Name", (int64)EFlowPinType::Name },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:100:{ "EFlowPinType::String", (int64)EFlowPinType::String },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:101:{ "EFlowPinType::Text", (int64)EFlowPinType::Text },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:102:{ "EFlowPinType::Enum", (int64)EFlowPinType::Enum },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:103:{ "EFlowPinType::Vector", (int64)EFlowPinType::Vector },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:104:{ "EFlowPinType::Rotator", (int64)EFlowPinType::Rotator },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:105:{ "EFlowPinType::Transform", (int64)EFlowPinType::Transform },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:106:{ "EFlowPinType::GameplayTag", (int64)EFlowPinType::GameplayTag },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:107:{ "EFlowPinType::GameplayTagContainer", (int64)EFlowPinType::GameplayTagContainer },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:108:{ "EFlowPinType::InstancedStruct", (int64)EFlowPinType::InstancedStruct },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:109:{ "EFlowPinType::Object", (int64)EFlowPinType::Object },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:110:{ "EFlowPinType::Class", (int64)EFlowPinType::Class },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:111:{ "EFlowPinType::Max", (int64)EFlowPinType::Max },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:112:{ "EFlowPinType::Invalid", (int64)EFlowPinType::Invalid },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:113:{ "EFlowPinType::Min", (int64)EFlowPinType::Min },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:117:const UECodeGen_Private::FEnumParams Z_Construct_UEnum_Flow_EFlowPinType_Statics::EnumParams = {
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:120:"EFlowPinType",
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:121:"EFlowPinType",
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:122:Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enumerators,
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:124:UE_ARRAY_COUNT(Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enumerators),
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:127:METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enum_MetaDataParams)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:129:UEnum* Z_Construct_UEnum_Flow_EFlowPinType()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:131:if (!Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:133:UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton, Z_Construct_UEnum_Flow_EFlowPinType_Statics::EnumParams);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:135:return Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:137:// End Enum EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.gen.cpp:234:{ EFlowPinType_StaticEnum, TEXT("EFlowPinType"), &Z_Registration_Info_UEnum_EFlowPinType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2635112643U) },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:22:#define FOREACH_ENUM_EFLOWPINTYPE(op) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:23:op(EFlowPinType::Exec) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:24:op(EFlowPinType::Bool) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:25:op(EFlowPinType::Int) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:26:op(EFlowPinType::Float) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:27:op(EFlowPinType::Name) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:28:op(EFlowPinType::String) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:29:op(EFlowPinType::Text) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:30:op(EFlowPinType::Enum) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:31:op(EFlowPinType::Vector) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:32:op(EFlowPinType::Rotator) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:33:op(EFlowPinType::Transform) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:34:op(EFlowPinType::GameplayTag) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:35:op(EFlowPinType::GameplayTagContainer) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:36:op(EFlowPinType::InstancedStruct) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:37:op(EFlowPinType::Object) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:38:op(EFlowPinType::Class) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:39:op(EFlowPinType::Max) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:40:op(EFlowPinType::Invalid) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:41:op(EFlowPinType::Min)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:43:enum class EFlowPinType : uint8;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:44:template<> struct TIsUEnumClass<EFlowPinType> { enum { Value = true }; };
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealEditor\Inc\Flow\UHT\FlowPinEnums.generated.h:45:template<> FLOW_API UEnum* StaticEnum<EFlowPinType>();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:18:FLOW_API UEnum* Z_Construct_UEnum_Flow_EFlowPinType();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:99:{ "EditCondition", "PinType == EFlowPinType::Class" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:111:{ "EditCondition", "PinType == EFlowPinType::Object" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:123:{ "EditCondition", "PinType == EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:135:{ "EditCondition", "PinType == EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPin.gen.cpp:167:const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FFlowPin_Statics::NewProp_PinType = { "PinType", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FFlowPin, PinType), Z_Construct_UEnum_Flow_EFlowPinType, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PinType_MetaData), NewProp_PinType_MetaData) }; // 2635112643
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:14:FLOW_API UEnum* Z_Construct_UEnum_Flow_EFlowPinType();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:18:// Begin Enum EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:19:static FEnumRegistrationInfo Z_Registration_Info_UEnum_EFlowPinType;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:20:static UEnum* EFlowPinType_StaticEnum()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:22:if (!Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:24:Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_Flow_EFlowPinType, (UObject*)Z_Construct_UPackage__Script_Flow(), TEXT("EFlowPinType"));
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:26:return Z_Registration_Info_UEnum_EFlowPinType.OuterSingleton;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:28:template<> FLOW_API UEnum* StaticEnum<EFlowPinType>()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:30:return EFlowPinType_StaticEnum();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:32:struct Z_Construct_UEnum_Flow_EFlowPinType_Statics
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:38:{ "Bool.Name", "EFlowPinType::Bool" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:41:{ "Class.Name", "EFlowPinType::Class" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:44:{ "Enum.Name", "EFlowPinType::Enum" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:47:{ "Exec.Name", "EFlowPinType::Exec" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:50:{ "Float.Name", "EFlowPinType::Float" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:53:{ "GameplayTag.Name", "EFlowPinType::GameplayTag" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:56:{ "GameplayTagContainer.Name", "EFlowPinType::GameplayTagContainer" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:59:{ "InstancedStruct.Name", "EFlowPinType::InstancedStruct" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:62:{ "Int.Name", "EFlowPinType::Int" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:65:{ "Invalid.Name", "EFlowPinType::Invalid" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:67:{ "Max.Name", "EFlowPinType::Max" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:69:{ "Min.Name", "EFlowPinType::Min" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:72:{ "Name.Name", "EFlowPinType::Name" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:75:{ "Object.Name", "EFlowPinType::Object" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:78:{ "Rotator.Name", "EFlowPinType::Rotator" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:81:{ "String.Name", "EFlowPinType::String" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:84:{ "Text.Name", "EFlowPinType::Text" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:87:{ "Transform.Name", "EFlowPinType::Transform" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:90:{ "Vector.Name", "EFlowPinType::Vector" },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:95:{ "EFlowPinType::Exec", (int64)EFlowPinType::Exec },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:96:{ "EFlowPinType::Bool", (int64)EFlowPinType::Bool },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:97:{ "EFlowPinType::Int", (int64)EFlowPinType::Int },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:98:{ "EFlowPinType::Float", (int64)EFlowPinType::Float },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:99:{ "EFlowPinType::Name", (int64)EFlowPinType::Name },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:100:{ "EFlowPinType::String", (int64)EFlowPinType::String },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:101:{ "EFlowPinType::Text", (int64)EFlowPinType::Text },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:102:{ "EFlowPinType::Enum", (int64)EFlowPinType::Enum },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:103:{ "EFlowPinType::Vector", (int64)EFlowPinType::Vector },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:104:{ "EFlowPinType::Rotator", (int64)EFlowPinType::Rotator },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:105:{ "EFlowPinType::Transform", (int64)EFlowPinType::Transform },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:106:{ "EFlowPinType::GameplayTag", (int64)EFlowPinType::GameplayTag },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:107:{ "EFlowPinType::GameplayTagContainer", (int64)EFlowPinType::GameplayTagContainer },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:108:{ "EFlowPinType::InstancedStruct", (int64)EFlowPinType::InstancedStruct },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:109:{ "EFlowPinType::Object", (int64)EFlowPinType::Object },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:110:{ "EFlowPinType::Class", (int64)EFlowPinType::Class },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:111:{ "EFlowPinType::Max", (int64)EFlowPinType::Max },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:112:{ "EFlowPinType::Invalid", (int64)EFlowPinType::Invalid },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:113:{ "EFlowPinType::Min", (int64)EFlowPinType::Min },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:117:const UECodeGen_Private::FEnumParams Z_Construct_UEnum_Flow_EFlowPinType_Statics::EnumParams = {
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:120:"EFlowPinType",
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:121:"EFlowPinType",
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:122:Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enumerators,
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:124:UE_ARRAY_COUNT(Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enumerators),
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:127:METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enum_MetaDataParams), Z_Construct_UEnum_Flow_EFlowPinType_Statics::Enum_MetaDataParams)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:129:UEnum* Z_Construct_UEnum_Flow_EFlowPinType()
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:131:if (!Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:133:UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton, Z_Construct_UEnum_Flow_EFlowPinType_Statics::EnumParams);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:135:return Z_Registration_Info_UEnum_EFlowPinType.InnerSingleton;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:137:// End Enum EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.gen.cpp:234:{ EFlowPinType_StaticEnum, TEXT("EFlowPinType"), &Z_Registration_Info_UEnum_EFlowPinType, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2635112643U) },
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:22:#define FOREACH_ENUM_EFLOWPINTYPE(op) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:23:op(EFlowPinType::Exec) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:24:op(EFlowPinType::Bool) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:25:op(EFlowPinType::Int) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:26:op(EFlowPinType::Float) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:27:op(EFlowPinType::Name) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:28:op(EFlowPinType::String) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:29:op(EFlowPinType::Text) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:30:op(EFlowPinType::Enum) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:31:op(EFlowPinType::Vector) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:32:op(EFlowPinType::Rotator) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:33:op(EFlowPinType::Transform) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:34:op(EFlowPinType::GameplayTag) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:35:op(EFlowPinType::GameplayTagContainer) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:36:op(EFlowPinType::InstancedStruct) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:37:op(EFlowPinType::Object) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:38:op(EFlowPinType::Class) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:39:op(EFlowPinType::Max) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:40:op(EFlowPinType::Invalid) \
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:41:op(EFlowPinType::Min)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:43:enum class EFlowPinType : uint8;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:44:template<> struct TIsUEnumClass<EFlowPinType> { enum { Value = true }; };
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Intermediate\Build\Win64\UnrealGame\Inc\Flow\UHT\FlowPinEnums.generated.h:45:template<> FLOW_API UEnum* StaticEnum<EFlowPinType>();
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_CustomInput.cpp:92:if (FlowDataPinProperty.GetFlowPinType() == EFlowPinType::Enum)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_CustomInput.cpp:98:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_CustomInput.cpp:140:// Must implement TrySupplyDataPinAs... for every EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_CustomInput.cpp:141:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_DefineProperties.cpp:81:if (FlowDataPinProperty.GetFlowPinType() == EFlowPinType::Enum)
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_DefineProperties.cpp:87:// We may need to manually call any PostEdit linked property updates here for future EFlowPinType values
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_DefineProperties.cpp:88:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_Start.cpp:47:// Must implement TrySupplyDataPinAs... for every EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\Graph\FlowNode_Start.cpp:48:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNode.cpp:694:// Must implement TrySupplyDataPinAs... for every EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNode.cpp:695:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:915:EFlowDataPinResolveResult UFlowNodeBase::TryResolveDataPinPrerequisites(const FName& PinName, const UFlowNode*& FlowNode, const FFlowPin*& FlowPin, EFlowPinType PinType) const
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:940:// Must implement TryResolveDataPinAs...() for every EFlowPinType
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:941:FLOW_ASSERT_ENUM_MAX(EFlowPinType, 16);
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:943:template <typename TFlowDataPinResultType, EFlowPinType PinType>
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:966:TResolveDataPinWorkingData<FFlowDataPinResult_Bool, EFlowPinType::Bool> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:987:TResolveDataPinWorkingData<FFlowDataPinResult_Int, EFlowPinType::Int> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:1008:TResolveDataPinWorkingData<FFlowDataPinResult_Float, EFlowPinType::Float> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:1029:TResolveDataPinWorkingData<FFlowDataPinResult_Name, EFlowPinType::Name> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:1050:TResolveDataPinWorkingData<FFlowDataPinResult_String, EFlowPinType::String> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:1071:TResolveDataPinWorkingData<FFlowDataPinResult_Text, EFlowPinType::Text> WorkData;
D:\Self\GItGame\Dev01\Plugins\FlowGraph-2.1-5.4\Source\Flow\Private\Nodes\FlowNodeBase.cpp:1092:TResolveDataPinWorkingData<FFlowDataPinResult_Enum, EFlowPinType::Enum> WorkData;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h'; Write-Output '---'; Get-Content -Raw 'Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp'; Write-Output '---GA_FinisherCharge.h---'; Get-Content -Raw 'Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h'; Write-Output '---GA_FinisherCharge.cpp---'; Get-Content -Raw 'Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp'; Write-Output '---GA_Player_FinisherAttack.h---'; Get-Content -Raw 'Source/DevKit/Public/AbilitySystem/Abilities/GA_Player_FinisherAttack.h'; Write-Output '---GA_Player_FinisherAttack.cpp---'; Get-Content -Raw 'Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp'" in D:\Self\GItGame\Dev01
 succeeded in 748ms:
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_ApplyFinisherMark.generated.h"

/**
 * 杞婚噺 GA锛氬悜鐩爣鏂藉姞 FinisherMark GE锛堟ā寮忓悓 GA_Knockback锛? * - 棰勬巿浜堬細鎵€鏈夎鑹诧紙DA_Base_AbilitySet锛? * - 婵€娲绘柟寮忥細Action.ApplyFinisherMark GameplayEvent
 * - 閰嶇疆椤癸細FinisherMarkGEClass 鈫?GE_FinisherMark锛圔lueprint濉啓锛? *
 * 鍚岀悊锛孉ction.ClearFinisherMark 浜嬩欢鐢?GA_ApplyClearFinisherMark 鎴栫洿鎺ュ湪姝A涓? * 閫氳繃 RemoveActiveEffectsWithGrantedTags 瀹炵幇銆? * 鏈珿A浠呰礋璐ｆ柦鍔犲嵃璁帮紝娓呴櫎鐢?GA_FinisherCharge::EndAbility 缁熶竴澶勭悊銆? */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_ApplyFinisherMark : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_ApplyFinisherMark(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    /** 鍗拌 GE锛圔lueprint濉?GE_FinisherMark锛孌uration=12s锛岃祴浜?Buff.Status.FinisherMark锛?/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TSubclassOf<UGameplayEffect> FinisherMarkGEClass;
};

---
#include "AbilitySystem/Abilities/GA_ApplyFinisherMark.h"

#include "AbilitySystemComponent.h"

static const FGameplayTag TAG_Action_ApplyFinisherMark =
    FGameplayTag::RequestGameplayTag(TEXT("Action.ApplyFinisherMark"));

static const FGameplayTag TAG_Buff_Status_FinisherMark =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherMark"));

UGA_ApplyFinisherMark::UGA_ApplyFinisherMark(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = TAG_Action_ApplyFinisherMark;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_ApplyFinisherMark::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!FinisherMarkGEClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_ApplyFinisherMark] FinisherMarkGEClass 鏈厤缃?));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 鑻ュ凡鏈夊嵃璁帮紝涓嶉噸澶嶆柦鍔狅紙骞傜瓑淇濇姢锛?    if (!ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherMark))
    {
        FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(FinisherMarkGEClass, GetAbilityLevel());
        if (SpecHandle.IsValid())
        {
            ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

---GA_FinisherCharge.h---
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_FinisherCharge.generated.h"

class UAbilityTask_WaitGameplayEvent;
class AYogCharacterBase;

/**
 * 缁堢粨鎶€鍏呰兘绐楀彛绠＄悊 GA
 * - 棰勬巿浜堬細鐜╁锛圖A_Base_AbilitySet锛? * - 婵€娲绘柟寮忥細Action.FinisherCharge.Activate GameplayEvent
 * - 鑱岃矗锛?娆＄獥鍙ｈ鏁般€佹瘡娆″懡涓彂閫佸嚮閫€+鍗拌銆佺獥鍙ｇ粨鏉熸竻闄ゅ嵃璁? *
 * Blueprint 蹇呴』閰嶇疆锛? *   AbilityTags             = PlayerState.AbilityCast.FinisherCharge
 *   ActivationOwnedTags     = PlayerState.AbilityCast.FinisherCharge   鈫?闃绘柇鑷韩閲嶅婵€娲? *   ActivationBlockedTags   = PlayerState.AbilityCast.FinisherCharge
 *   FinisherChargeGEClass   = GE_FinisherCharge
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_FinisherCharge : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_FinisherCharge(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    /** 绐楀彛鍐呮渶澶氬鐞嗙殑鍑婚€€+鍗拌娆℃暟锛堝搴擥E鏈€澶у眰鏁帮級*/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    int32 MaxCharges = 5;

    /** 鐢ㄤ簬鏌ユ壘鍜岀Щ闄ゅ厖鑳紾E鐨勭被锛圔lueprint濉?GE_FinisherCharge锛?/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TSubclassOf<UGameplayEffect> FinisherChargeGEClass;

private:
    UFUNCTION()
    void OnAttackHit(FGameplayTag EventTag, FGameplayEventData EventData);

    void OnFinisherChargeTagChanged(const FGameplayTag Tag, int32 NewCount);

    // 寤惰繜涓€甯х粨鏉燂紝纭繚 AN_TriggerFinisherAbility 鑳藉湪鍚屽抚妫€娴嬪埌 FinisherWindowOpen
    void EndAbilityDeferred();

    void ClearAllMarks();

    int32 RemainingCharges = 0;

    FActiveGameplayEffectHandle ChargeGEHandle;

    FDelegateHandle TagChangedHandle;

    FTimerHandle DeferredEndHandle;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;
};

---GA_FinisherCharge.cpp---
#include "AbilitySystem/Abilities/GA_FinisherCharge.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayTagsManager.h"
#include "Kismet/GameplayStatics.h"

static const FGameplayTag TAG_Action_FinisherCharge_Activate =
    FGameplayTag::RequestGameplayTag(TEXT("Action.FinisherCharge.Activate"));

static const FGameplayTag TAG_Action_FinisherCharge_ChargeConsumed =
    FGameplayTag::RequestGameplayTag(TEXT("Action.FinisherCharge.ChargeConsumed"));

static const FGameplayTag TAG_Buff_Status_FinisherCharge =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherCharge"));

static const FGameplayTag TAG_Buff_Status_FinisherMark =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherMark"));

// WindowOpen Tag锛氱嫭绔嬩簬GE灞傛暟锛屼緵 AN_TriggerFinisherAbility 妫€娴嬬獥鍙ｆ槸鍚︽湁鏁?static const FGameplayTag TAG_Buff_Status_FinisherWindowOpen =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherWindowOpen"));

static const FGameplayTag TAG_Ability_Event_Attack_Hit =
    FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));

static const FGameplayTag TAG_Buff_Status_FinisherExecuting =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherExecuting"));

UGA_FinisherCharge::UGA_FinisherCharge(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = TAG_Action_FinisherCharge_Activate;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_FinisherCharge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
    if (!ASC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 鎵惧埌 GE_FinisherCharge 鐨?Handle锛岀敤浜庡悗缁€愬眰绉婚櫎
    if (FinisherChargeGEClass)
    {
        FGameplayEffectQuery Query;
        Query.EffectDefinition = FinisherChargeGEClass;
        TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
        if (Handles.Num() > 0)
        {
            ChargeGEHandle = Handles[0];
            const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ChargeGEHandle);
            if (ActiveGE)
            {
                RemainingCharges = ActiveGE->Spec.GetStackCount();
            }
        }
    }

    if (RemainingCharges <= 0)
    {
        RemainingCharges = MaxCharges;
    }

    // 鎺堜簣绐楀彛杩借釜Tag锛氱嫭绔嬩簬GE灞傛暟锛屾渶鍚庝竴灞傚懡涓秷鑰楀悗GE娑堝け浣嗙獥鍙ｄ粛鏈夋晥
    // AN_TriggerFinisherAbility 妫€娴嬫Tag鍐冲畾鏄惁瑙﹀彂缁堢粨鎶€
    ASC->AddLooseGameplayTag(TAG_Buff_Status_FinisherWindowOpen);

    // 鐩戝惉 Buff.Status.FinisherCharge Tag 娑堝け锛圙E 8绉掑埌鏈熸垨灞傛暟褰掗浂鏃惰Е鍙戯級
    TagChangedHandle = ASC->RegisterGameplayTagEvent(
        TAG_Buff_Status_FinisherCharge,
        EGameplayTagEventType::NewOrRemoved
    ).AddUObject(this, &UGA_FinisherCharge::OnFinisherChargeTagChanged);

    // 寰幆鐩戝惉鐜╁鏀诲嚮鍛戒腑浜嬩欢
    WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        TAG_Ability_Event_Attack_Hit,
        nullptr,
        false,  // OnlyTriggerOnce = false 鈫?閲嶅瑙﹀彂
        true
    );
    WaitHitTask->EventReceived.AddDynamic(this, &UGA_FinisherCharge::OnAttackHit);
    WaitHitTask->ReadyForActivation();
}

void UGA_FinisherCharge::OnAttackHit(FGameplayTag EventTag, FGameplayEventData EventData)
{
    if (RemainingCharges <= 0)
    {
        return;
    }

    AActor* HitTarget = EventData.Target.Get();
    if (!HitTarget)
    {
        return;
    }

    UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponent(CurrentActorInfo);
    if (!PlayerASC)
    {
        return;
    }

    // 鍏堟淳鍙?ChargeConsumed 浜嬩欢锛團A_Finisher_ChargeHit 鐩戝惉锛夛紝鍐嶆墸灞傛暟
    // 淇濊瘉 FA 鍦?GE 鏈€鍚庝竴灞傝绉婚櫎鍓嶅凡鏀跺埌浜嬩欢
    FGameplayEventData ChargeConsumedPayload;
    ChargeConsumedPayload.Instigator = GetAvatarActorFromActorInfo();
    ChargeConsumedPayload.Target     = HitTarget;
    PlayerASC->HandleGameplayEvent(TAG_Action_FinisherCharge_ChargeConsumed, &ChargeConsumedPayload);

    // 娑堣€?娆″厖鑳斤紝鐒跺悗绉婚櫎GE灞傛暟
    RemainingCharges--;

    if (ChargeGEHandle.IsValid())
    {
        // GE 灞傛暟褰掗浂鏃惰嚜鍔ㄧЩ闄?鈫?Tag 娑堝け 鈫?OnFinisherChargeTagChanged 鈫?EndAbility
        PlayerASC->RemoveActiveGameplayEffect(ChargeGEHandle, 1);
    }
    else if (RemainingCharges <= 0)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_FinisherCharge::OnFinisherChargeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount <= 0)
    {
        // 鍏堝仠姝㈠懡涓洃鍚紝闃叉缁х画娑堣€楀厖鑳?        if (WaitHitTask)
        {
            WaitHitTask->EndTask();
            WaitHitTask = nullptr;
        }

        // 寤惰繜涓€甯х粨鏉燂紝淇濊瘉 AN_TriggerFinisherAbility 鍦ㄥ悓甯ц兘妫€娴嬪埌 FinisherWindowOpen
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimerForNextTick(
                FTimerDelegate::CreateUObject(this, &UGA_FinisherCharge::EndAbilityDeferred));
        }
    }
}

void UGA_FinisherCharge::EndAbilityDeferred()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_FinisherCharge::ClearAllMarks()
{
    if (!GetWorld())
    {
        return;
    }

    // 缁堢粨鎶€姝ｅ湪鎵ц鏃惰烦杩囨竻鐞嗭細FA_Finisher_Detonate 浼氶€愮洰鏍囩Щ闄ゅ嵃璁?    UAbilitySystemComponent* ASC = GetAbilitySystemComponent(CurrentActorInfo);
    if (ASC && ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherExecuting))
    {
        return;
    }

    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AYogCharacterBase::StaticClass(), AllCharacters);

    for (AActor* Actor : AllCharacters)
    {
        AYogCharacterBase* Char = Cast<AYogCharacterBase>(Actor);
        if (!Char || !Char->GetASC())
        {
            continue;
        }
        if (Char->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_FinisherMark))
        {
            // 鐩存帴绉婚櫎GE锛堜笉璧癎ameplayEvent锛岄伩鍏嶉渶瑕侀澶栭鎺堜簣鎺ユ敹GA锛?            Char->GetASC()->RemoveActiveEffectsWithGrantedTags(
                FGameplayTagContainer(TAG_Buff_Status_FinisherMark));
        }
    }
}

void UGA_FinisherCharge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo))
    {
        // 娉ㄩ攢Tag鐩戝惉
        if (TagChangedHandle.IsValid())
        {
            ASC->RegisterGameplayTagEvent(
                TAG_Buff_Status_FinisherCharge,
                EGameplayTagEventType::NewOrRemoved
            ).Remove(TagChangedHandle);
            TagChangedHandle.Reset();
        }

        // 绉婚櫎绐楀彛杩借釜Tag锛堝厑璁?AN_TriggerFinisherAbility 涓嶅啀瑙﹀彂锛?        ASC->RemoveLooseGameplayTag(TAG_Buff_Status_FinisherWindowOpen);

        // 娓呯悊娈嬬暀 GE锛堟甯歌€楀敖璺緞宸茬敱GE鑷繁绉婚櫎锛岃繖閲屽鐞嗚秴鏃?鍙栨秷璺緞锛?        if (ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherCharge))
        {
            ASC->RemoveActiveEffectsWithGrantedTags(
                FGameplayTagContainer(TAG_Buff_Status_FinisherCharge));
        }
    }

    // 缁撴潫鍛戒腑绛夊緟浠诲姟
    if (WaitHitTask)
    {
        WaitHitTask->EndTask();
        WaitHitTask = nullptr;
    }

    // 娓呴櫎鎵€鏈夋湭寮曠垎鐨勫嵃璁帮紙绐楀彛缁撴潫鎴栬鍙栨秷鏃跺厹搴曪級
    ClearAllMarks();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

---GA_Player_FinisherAttack.h---
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Player_FinisherAttack.generated.h"

class UYogAbilityTask_PlayMontageAndWaitForEvent;
class AYogCharacterBase;

/**
 * 缁堢粨鎶€涓?GA锛堢姸鎬佹満鑱岃矗锛屼笉鐩存帴鏂藉姞浼ゅ锛? * - 婵€娲绘柟寮忥細Action.Player.FinisherAttack GameplayEvent锛堢敱 AN_TriggerFinisherAbility 瑙﹀彂锛? * - 鑱岃矗锛氳挋澶鎾斁銆佸瓙寮规椂闂存帶鍒躲€佺‘璁よ緭鍏ユ娴嬨€侀亶鍘嗗嵃璁扮洰鏍囧苟娲惧彂寮曠垎浜嬩欢
 * - 浼ゅ/鍑婚€€/鍓茶鐢?FA_Finisher_Detonate 閫氳繃 WaitGameplayEvent(DetonateTarget) 澶勭悊
 *
 * Blueprint 蹇呴』閰嶇疆锛? *   AbilityTags             = PlayerState.AbilityCast.Finisher
 *   ActivationOwnedTags     = Buff.Status.FinisherExecuting
 *   ActivationRequiredTags  = Buff.Status.FinisherWindowOpen
 *   ActivationBlockedTags   = Buff.Status.Dead, Buff.Status.FinisherExecuting
 *   CancelAbilitiesWithTag  = PlayerState.AbilityCast.LightAtk | HeavyAtk | Dash锛堟帓闄?FinisherCharge锛? *   FinisherMontage         = AM_Player_FinisherAttack
 *
 * 钂欏お濂?Notify 閰嶇疆锛? *   [ANS_FinisherTimeDilation]  鈫?瑕嗙洊瀛愬脊鏃堕棿鍖洪棿
 *   [AN_MeleeDamage]            鈫?鏀诲嚮鍒ゅ畾甯э紝EventTag = Ability.Event.Finisher.HitFrame
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Player_FinisherAttack : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Player_FinisherAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    /** 缁堢粨鎶€钂欏お濂?*/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TObjectPtr<UAnimMontage> FinisherMontage;

    /**
     * 鐜╁纭鏃剁殑浼ゅ鍊嶇巼锛屽啓鍏?DetonateTarget 浜嬩欢鐨?EventMagnitude
     * 锛堟湭纭 = 1.0锛岀‘璁?= 姝ゅ€硷級
     * 浼ゅ鍩虹鍊煎湪鍗＄墝 DA 鏁板€艰〃涓厤缃紙Key: DetonationDamage锛?     */
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    float ConfirmedDamageMultiplier = 2.0f;

private:
    UFUNCTION()
    void OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

    void DetonateMarks(bool bConfirmed);

    void RestoreTimeDilation();

    bool bPlayerConfirmed      = false;
    bool bTimeDilationRestored = false;
    bool bDetonated            = false;

    UPROPERTY()
    TObjectPtr<UYogAbilityTask_PlayMontageAndWaitForEvent> MontageTask;
};

---GA_Player_FinisherAttack.cpp---
#include "AbilitySystem/Abilities/GA_Player_FinisherAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

static const FGameplayTag TAG_Action_Player_FinisherAttack =
    FGameplayTag::RequestGameplayTag(TEXT("Action.Player.FinisherAttack"));

static const FGameplayTag TAG_Action_Finisher_Confirm =
    FGameplayTag::RequestGameplayTag(TEXT("Action.Finisher.Confirm"));

static const FGameplayTag TAG_Ability_Event_Finisher_HitFrame =
    FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Finisher.HitFrame"));

static const FGameplayTag TAG_Buff_Status_FinisherMark =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherMark"));

static const FGameplayTag TAG_Buff_Status_Dead =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"));

// 寮曠垎浜嬩欢娲惧彂鑷崇帺瀹禔SC锛孎A_Finisher_Detonate 閫氳繃 WaitGameplayEvent 鐩戝惉
static const FGameplayTag TAG_Action_FinisherAttack_DetonateTarget =
    FGameplayTag::RequestGameplayTag(TEXT("Action.FinisherAttack.DetonateTarget"));

UGA_Player_FinisherAttack::UGA_Player_FinisherAttack(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = TAG_Action_Player_FinisherAttack;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_Player_FinisherAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    bPlayerConfirmed      = false;
    bTimeDilationRestored = false;

    if (!FinisherMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Player_FinisherAttack] FinisherMontage 鏈厤缃?));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FGameplayTagContainer EventTags;
    EventTags.AddTag(TAG_Action_Finisher_Confirm);
    EventTags.AddTag(TAG_Ability_Event_Finisher_HitFrame);

    MontageTask = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
        this,
        NAME_None,
        FinisherMontage,
        EventTags,
        1.f,
        NAME_None,
        true
    );

    MontageTask->EventReceived.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageEvent);
    MontageTask->OnCompleted.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageInterrupted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageInterrupted);
    MontageTask->ReadyForActivation();
}

void UGA_Player_FinisherAttack::OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData)
{
    if (EventTag == TAG_Action_Finisher_Confirm)
    {
        if (!bPlayerConfirmed)
        {
            bPlayerConfirmed = true;
            RestoreTimeDilation();
        }
    }
    else if (EventTag == TAG_Ability_Event_Finisher_HitFrame)
    {
        DetonateMarks(bPlayerConfirmed);
    }
}

void UGA_Player_FinisherAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Player_FinisherAttack::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Player_FinisherAttack::DetonateMarks(bool bConfirmed)
{
    if (bDetonated)
    {
        return;
    }
    bDetonated = true;

    if (!GetWorld())
    {
        return;
    }

    UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponent(CurrentActorInfo);
    if (!PlayerASC)
    {
        return;
    }

    AActor* PlayerActor = GetAvatarActorFromActorInfo();

    // EventMagnitude 浣滀负纭鍊嶇巼浼犵粰 FA锛?.0 = 鏈‘璁わ紝ConfirmedDamageMultiplier = 宸茬‘璁?    const float ConfirmMultiplier = bConfirmed ? ConfirmedDamageMultiplier : 1.0f;

    TArray<AActor*> AllCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AYogCharacterBase::StaticClass(), AllCharacters);

    for (AActor* Actor : AllCharacters)
    {
        AYogCharacterBase* Target = Cast<AYogCharacterBase>(Actor);
        if (!Target || !Target->GetASC())
        {
            continue;
        }
        // 璺宠繃姝讳骸鐩爣
        if (Target->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_Dead))
        {
            continue;
        }
        if (!Target->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_FinisherMark))
        {
            continue;
        }

        // 鍚戠帺瀹禔SC娲惧彂寮曠垎浜嬩欢锛孎A_Finisher_Detonate 璐熻矗锛?        //   GetRuneTuningValue("DetonationDamage") 脳 EventMagnitude 鈫?ApplyEffect
        //   SendSlash 鈫?(鏉′欢)SendKnockback 鈫?RemoveMark
        FGameplayEventData DetEvent;
        DetEvent.Instigator     = PlayerActor;
        DetEvent.Target         = Target;
        DetEvent.EventMagnitude = ConfirmMultiplier;
        PlayerASC->HandleGameplayEvent(TAG_Action_FinisherAttack_DetonateTarget, &DetEvent);
    }
}

void UGA_Player_FinisherAttack::RestoreTimeDilation()
{
    if (bTimeDilationRestored)
    {
        return;
    }
    bTimeDilationRestored = true;

    if (AWorldSettings* WS = GetWorld() ? GetWorld()->GetWorldSettings() : nullptr)
    {
        WS->SetTimeDilation(1.0f);
    }

    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        Avatar->CustomTimeDilation = 1.0f;
    }
}

void UGA_Player_FinisherAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 浠讳綍閫€鍑鸿矾寰勯兘纭繚鏃堕棿鎭㈠
    RestoreTimeDilation();

    if (MontageTask)
    {
        MontageTask->EndTask();
        MontageTask = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$files=@('"'Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp','Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp','Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp','Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_Player_FinisherAttack.h'); foreach("'$f in $files){Write-Output "--- $f"; $i=0; Get-Content $f | ForEach-Object { $i++; if($_ -match '"'UE_LOG|static const FGameplayTag TAG_Buff_Status_FinisherWindowOpen|if \\("'!ASC->HasMatching|ClearAllMarks|comment|/'"\\*|\\*/|//.*if|RemoveActiveEffects|EndAbility\\(Handle') { \"{0,4}: {1}\" -f "'$i,$_ } }}' in D:\Self\GItGame\Dev01
 succeeded in 655ms:
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp
  32:         UE_LOG(LogTemp, Warning, TEXT("[GA_ApplyFinisherMark] FinisherMarkGEClass 鏈厤缃?));
  33:         EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
  40:         EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
  44:     // 鑻ュ凡鏈夊嵃璁帮紝涓嶉噸澶嶆柦鍔狅紙骞傜瓑淇濇姢锛?    if (!ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherMark))
  53:     EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp
  22: // WindowOpen Tag锛氱嫭绔嬩簬GE灞傛暟锛屼緵 AN_TriggerFinisherAbility 妫€娴嬬獥鍙ｆ槸鍚︽湁鏁?static const FGameplayTag TAG_Buff_Status_FinisherWindowOpen =
  53:         EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
 145:         // 鍏堝仠姝㈠懡涓洃鍚紝闃叉缁х画娑堣€楀厖鑳?        if (WaitHitTask)
 165: void UGA_FinisherCharge::ClearAllMarks()
 190:             // 鐩存帴绉婚櫎GE锛堜笉璧癎ameplayEvent锛岄伩鍏嶉渶瑕侀澶栭鎺堜簣鎺ユ敹GA锛?            Char->GetASC()->RemoveActiveEffectsWithGrantedTags(
 217:         // 娓呯悊娈嬬暀 GE锛堟甯歌€楀敖璺緞宸茬敱GE鑷繁绉婚櫎锛岃繖閲屽鐞嗚秴鏃?鍙栨秷璺緞锛?        if (ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherCharge))
 219:             ASC->RemoveActiveEffectsWithGrantedTags(
 232:     ClearAllMarks();
 234:     Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp
  53:         UE_LOG(LogTemp, Error, TEXT("[GA_Player_FinisherAttack] FinisherMontage 鏈厤缃?));
  54:         EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
 194:     Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h
   7: /**
  10:  * 鍚岀悊锛孉ction.ClearFinisherMark 浜嬩欢鐢?GA_ApplyClearFinisherMark 鎴栫洿鎺ュ湪姝A涓? * 閫氳繃 RemoveActiveEffectsWithGrantedTags 瀹炵幇銆? * 鏈珿A浠呰礋璐ｆ柦鍔犲嵃璁帮紝娓呴櫎鐢?GA_FinisherCharge::EndAbility 缁熶竴澶勭悊銆? */
  25:     /** 鍗拌 GE锛圔lueprint濉?GE_FinisherMark锛孌uration=12s锛岃祴浜?Buff.Status.FinisherMark锛?/
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h
  10: /**
  17:  */
  39:     /** 绐楀彛鍐呮渶澶氬鐞嗙殑鍑婚€€+鍗拌娆℃暟锛堝搴擥E鏈€澶у眰鏁帮級*/
  43:     /** 鐢ㄤ簬鏌ユ壘鍜岀Щ闄ゅ厖鑳紾E鐨勭被锛圔lueprint濉?GE_FinisherCharge锛?/
  56:     void ClearAllMarks();
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_Player_FinisherAttack.h
  10: /**
  22:  */
  44:     /** 缁堢粨鎶€钂欏お濂?*/
  48:     /**
  51:      * 浼ゅ鍩虹鍊煎湪鍗＄墝 DA 鏁板€艰〃涓厤缃紙Key: DetonationDamage锛?     */

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "@'
from pathlib import Path
for path in ['Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp','Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp','Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp','Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h']:
    print('---',path)
    data=Path(path).read_bytes()
    # Print problematic lines repr as bytes and decoded latin-1? line split by \\n
    for i,line in enumerate(data.splitlines(),1):
        if i in [22,32,44,53,145,190,217,25,43]:
            print(i, repr(line[:300]))
'@ | python -" in D:\Self\GItGame\Dev01
 succeeded in 726ms:
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp
22 b'void UGA_ApplyFinisherMark::ActivateAbility('
25 b'    const FGameplayAbilityActivationInfo ActivationInfo,'
32 b'        UE_LOG(LogTemp, Warning, TEXT("[GA_ApplyFinisherMark] FinisherMarkGEClass \xe6\x9c\xaa\xe9\x85\x8d\xe7\xbd\xae"));'
43 b''
44 b'    // \xe8\x8b\xa5\xe5\xb7\xb2\xe6\x9c\x89\xe5\x8d\xb0\xe8\xae\xb0\xef\xbc\x8c\xe4\xb8\x8d\xe9\x87\x8d\xe5\xa4\x8d\xe6\x96\xbd\xe5\x8a\xa0\xef\xbc\x88\xe5\xb9\x82\xe7\xad\x89\xe4\xbf\x9d\xe6\x8a\xa4\xef\xbc\x89'
53 b''
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp
22 b'// WindowOpen Tag\xef\xbc\x9a\xe7\x8b\xac\xe7\xab\x8b\xe4\xba\x8eGE\xe5\xb1\x82\xe6\x95\xb0\xef\xbc\x8c\xe4\xbe\x9b AN_TriggerFinisherAbility \xe6\xa3\x80\xe6\xb5\x8b\xe7\xaa\x97\xe5\x8f\xa3\xe6\x98\xaf\xe5\x90\xa6\xe6\x9c\x89\xe6\x95\x88'
25 b''
32 b'UGA_FinisherCharge::UGA_FinisherCharge(const FObjectInitializer& ObjectInitializer)'
43 b'void UGA_FinisherCharge::ActivateAbility('
44 b'    const FGameplayAbilitySpecHandle Handle,'
53 b'    {'
145 b'    {'
190 b'        }'
217 b'        }'
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp
22 b'static const FGameplayTag TAG_Buff_Status_Dead ='
25 b'// \xe5\xbc\x95\xe7\x88\x86\xe4\xba\x8b\xe4\xbb\xb6\xe6\xb4\xbe\xe5\x8f\x91\xe8\x87\xb3\xe7\x8e\xa9\xe5\xae\xb6ASC\xef\xbc\x8cFA_Finisher_Detonate \xe9\x80\x9a\xe8\xbf\x87 WaitGameplayEvent \xe7\x9b\x91\xe5\x90\xac'
32 b'    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;'
43 b'    const FGameplayAbilityActivationInfo ActivationInfo,'
44 b'    const FGameplayEventData* TriggerEventData)'
53 b'        UE_LOG(LogTemp, Error, TEXT("[GA_Player_FinisherAttack] FinisherMontage \xe6\x9c\xaa\xe9\x85\x8d\xe7\xbd\xae"));'
145 b'        if (!Target->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_FinisherMark))'
190 b'    if (MontageTask)'
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_ApplyFinisherMark.h
22 b'public:'
25 b'    virtual void ActivateAbility('
32 b'    UPROPERTY(EditDefaultsOnly, Category = "Finisher")'
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_FinisherCharge.h
22 b'UCLASS(BlueprintType, Blueprintable)'
25 b'    GENERATED_BODY()'
32 b'        const FGameplayAbilityActorInfo* ActorInfo,'
43 b'    /** \xe7\xaa\x97\xe5\x8f\xa3\xe5\x86\x85\xe6\x9c\x80\xe5\xa4\x9a\xe5\xa4\x84\xe7\x90\x86\xe7\x9a\x84\xe5\x87\xbb\xe9\x80\x80+\xe5\x8d\xb0\xe8\xae\xb0\xe6\xac\xa1\xe6\x95\xb0\xef\xbc\x88\xe5\xaf\xb9\xe5\xba\x94GE\xe6\x9c\x80\xe5\xa4\xa7\xe5\xb1\x82\xe6\x95\xb0\xef\xbc\x89*/'
44 b'    UPROPERTY(EditDefaultsOnly, Category = "Finisher")'
53 b'    void OnAttackHit(FGameplayTag EventTag, FGameplayEventData EventData);'

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h'; Write-Output '---'; Get-Content -Raw 'Source/DevKit/Private/Animation/ANS_FinisherTimeDilation.cpp'; Write-Output '---AN---'; Get-Content -Raw 'Source/DevKit/Public/Animation/AN_TriggerFinisherAbility.h'; Write-Output '---'; Get-Content -Raw 'Source/DevKit/Private/Animation/AN_TriggerFinisherAbility.cpp'" in D:\Self\GItGame\Dev01
 succeeded in 595ms:
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_FinisherTimeDilation.generated.h"

/**
 * 鍖洪棿 AnimNotifyState锛氬瓙寮规椂闂存晥鏋溿€? *
 * Begin锛氬叏灞€鏃堕棿鑶ㄨ儉 脳 SlowDilation锛堥粯璁?.15锛夛紝鐜╁鎶垫秷淇濇寔姝ｅ父閫熷害锛屾樉绀鸿緭鍏ユ彁绀篣I銆? * End锛? 鑻ユ椂闂磋啫鑳€鏈GA鎻愬墠鎭㈠锛屽垯鍦ㄦ澶勬仮澶嶏紝闅愯棌UI銆? *
 * 鏀剧疆浣嶇疆锛氱粓缁撴妧钂欏お濂囦腑"杈撳叆绐楀彛"鐨勮捣姝㈠尯闂淬€? */
UCLASS(meta = (DisplayName = "ANS Finisher Time Dilation"))
class DEVKIT_API UANS_FinisherTimeDilation : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    /** 鍏ㄥ眬鎱㈠姩浣滃€嶇巼锛?.15 = 鍏ㄥ眬鍑忛€熷埌1/6.6锛岀帺瀹剁敤CustomTimeDilation鎶垫秷锛?/
    UPROPERTY(EditAnywhere, Category = "TimeDilation")
    float SlowDilation = 0.15f;

    /** 鏄剧ず/闅愯棌鎻愮ずUI鐢ㄧ殑GameplayEvent Tag锛堝箍鎾粰PlayerController锛?/
    UPROPERTY(EditAnywhere, Category = "UI")
    FGameplayTag PromptShowEventTag;

    UPROPERTY(EditAnywhere, Category = "UI")
    FGameplayTag PromptHideEventTag;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};

---
#include "Animation/ANS_FinisherTimeDilation.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/WorldSettings.h"

void UANS_FinisherTimeDilation::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character)
    {
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return;
    }

    const float SafeDilation = FMath::Clamp(SlowDilation, 0.001f, 1.0f);

    // 鍏ㄥ眬鍑忛€?    if (AWorldSettings* WS = World->GetWorldSettings())
    {
        WS->SetTimeDilation(SafeDilation);
    }

    // 鐜╁鑷韩鐢ㄥ€掓暟鎶垫秷锛岀淮鎸佹甯搁€熷害锛?瀛愬脊鏃堕棿"鏁堟灉锛?    Character->CustomTimeDilation = 1.0f / SafeDilation;

    // 閫氱煡 UI 鏄剧ず杈撳叆鎻愮ず
    if (PromptShowEventTag.IsValid() && Character->GetASC())
    {
        FGameplayEventData UIPayload;
        UIPayload.Instigator = Character;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, PromptShowEventTag, UIPayload);
    }
}

void UANS_FinisherTimeDilation::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character)
    {
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return;
    }

    // 浠呭湪鏈 GA 鎻愬墠鎭㈠鏃舵墠鎭㈠锛堢帺瀹剁‘璁ゅ悗 GA 浼氭彁鍓嶆仮澶?GlobalTimeDilation = 1.0锛?    if (AWorldSettings* WS = World->GetWorldSettings())
    {
        if (!FMath::IsNearlyEqual(WS->GetEffectiveTimeDilation(), 1.0f, 0.01f))
        {
            WS->SetTimeDilation(1.0f);
            Character->CustomTimeDilation = 1.0f;
        }
        else
        {
            // GA宸叉仮澶嶅叏灞€鏃堕棿锛屼粎纭繚鐜╁CustomTimeDilation澶嶄綅
            Character->CustomTimeDilation = 1.0f;
        }
    }

    // 閫氱煡 UI 闅愯棌杈撳叆鎻愮ず
    if (PromptHideEventTag.IsValid() && Character->GetASC())
    {
        FGameplayEventData UIPayload;
        UIPayload.Instigator = Character;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, PromptHideEventTag, UIPayload);
    }
}

FString UANS_FinisherTimeDilation::GetNotifyName_Implementation() const
{
    return FString::Printf(TEXT("Finisher Time Dilation x%.2f"), SlowDilation);
}

---AN---
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_TriggerFinisherAbility.generated.h"

/**
 * 鍗曞抚 AnimNotify锛氭娴嬬帺瀹舵槸鍚﹀浜庣粓缁撴妧鍏呰兘绐楀彛锛岃嫢鏄垯鑷姩瑙﹀彂缁堢粨鎶€ GA銆? *
 * 鏀剧疆浣嶇疆锛欻3 钂欏お濂囦激瀹冲垽瀹氬抚锛圓N_MeleeDamage锛変箣鍚庣殑鍥炴敹甯с€? * 鐢ㄦ硶锛氬皢鏈?Notify 鎷栧叆 H3 钂欏お濂?Notifies 杞ㄩ亾锛屾棤闇€棰濆閰嶇疆銆? */
UCLASS(meta = (DisplayName = "AN Trigger Finisher Ability"))
class DEVKIT_API UAN_TriggerFinisherAbility : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};

---
#include "Animation/AN_TriggerFinisherAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"

// 妫€娴?WindowOpen Tag锛堣€岄潪 FinisherCharge锛夛紝纭繚鏈€鍚庝竴灞傚懡涓秷鑰桮E鍚庝粛鑳借Е鍙戠粓缁撴妧
static const FGameplayTag TAG_Buff_Status_FinisherWindowOpen =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherWindowOpen"));

static const FGameplayTag TAG_Action_Player_FinisherAttack =
    FGameplayTag::RequestGameplayTag(TEXT("Action.Player.FinisherAttack"));

void UAN_TriggerFinisherAbility::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character || !Character->GetASC())
    {
        return;
    }

    // 妫€娴?WindowOpen Tag锛圙A_FinisherCharge EndAbility 鏃剁Щ闄わ紝姣擥E Tag鏇存櫄娑堝け锛?    if (!Character->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_FinisherWindowOpen))
    {
        return;
    }

    FGameplayEventData Payload;
    Payload.Instigator = Character;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, TAG_Action_Player_FinisherAttack, Payload);
}

FString UAN_TriggerFinisherAbility::GetNotifyName_Implementation() const
{
    return TEXT("Trigger Finisher Ability");
}


exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit -Include *.cpp,*.h | Select-String -Pattern 'static const FGameplayTag .*RequestGameplayTag' | Select-Object -First 80 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 787ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Fear.cpp:106:static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Freeze.cpp:78:static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:52:static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:62:static const FGameplayTag ArmoredTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:73:static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:564:static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:565:static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:566:static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:567:static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:752:static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:753:static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:754:static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:755:static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:1010:static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayerDash.cpp:275:static const FGameplayTag DashEventTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Dash"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Rend.cpp:96:static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Wound.cpp:71:static const FGameplayTag DataDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:174:static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:190:static const FGameplayTag KillTag  = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Kill"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:191:static const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Death"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:310:static const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:326:static const FGameplayTag KillTag  = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Kill"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Attribute\DamageAttributeSet.cpp:327:static const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Death"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Execution\GEExec_PoisonDamage.cpp:91:static const FGameplayTag ArmoredTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\DamageExecution.cpp:160:static const FGameplayTag NextHitCritTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.NextHitCrit"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\DamageExecution.cpp:198:static const FGameplayTag CritHitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.CritHit"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:27:static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:28:static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:29:static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:38:static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:49:static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:50:static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:51:static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:311:static const FGameplayTag MovementCategory = FGameplayTag::RequestGameplayTag(TEXT("Block.Movement"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:312:static const FGameplayTag AICategory       = FGameplayTag::RequestGameplayTag(TEXT("Block.AI"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:581:static const FGameplayTag MeleeTag  = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"),  false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:582:static const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:595:static const FGameplayTag MeleeTag  = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"),  false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:596:static const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Actors\CombatItemAreaActor.cpp:277:static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Actors\CombatItemAreaActor.cpp:278:static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Actors\CombatItemAreaActor.cpp:293:static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_DoDamage.cpp:87:static const FGameplayTag DataDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:747:static const FGameplayTag Phase1 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.1"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:748:static const FGameplayTag Phase2 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.2"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:749:static const FGameplayTag Phase3 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.3"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:521:static const FGameplayTag ChainActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Combo.ChainActive"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:481:static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:616:static const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:617:static const FGameplayTag HeavyAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:618:static const FGameplayTag DashAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.DashAtk"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:780:static const FGameplayTag TagRetainTwo = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.TwoPhase"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:781:static const FGameplayTag TagRetainOne = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.OnePhase"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2228:static const FGameplayTag TagRetainTwo = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.TwoPhase"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2229:static const FGameplayTag TagRetainOne = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.OnePhase"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2297:static const FGameplayTag EliteTag  = FGameplayTag::RequestGameplayTag(FName("Room.Type.Elite"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2298:static const FGameplayTag ShopTag   = FGameplayTag::RequestGameplayTag(FName("Room.Type.Shop"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2299:static const FGameplayTag EventTag  = FGameplayTag::RequestGameplayTag(FName("Room.Type.Event"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2300:static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2314:static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\GameModes\YogGameMode.cpp:2476:static const FGameplayTag NormalTag = FGameplayTag::RequestGameplayTag(FName("Room.Type.Normal"));
D:\Self\GItGame\Dev01\Source\DevKit\Private\Map\Portal.cpp:95:static const FGameplayTag RoomTypeRoot = FGameplayTag::RequestGameplayTag(

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git diff -- Config/DefaultGameplayTags.ini Config/Tags/BuffTag.ini Config/Tags/PlayerGameplayTag.ini' in D:\Self\GItGame\Dev01
 succeeded in 505ms:
diff --git a/Config/DefaultGameplayTags.ini b/Config/DefaultGameplayTags.ini
index 55f3c934..fd2db848 100644
--- a/Config/DefaultGameplayTags.ini
+++ b/Config/DefaultGameplayTags.ini
@@ -169,3 +169,8 @@ NetIndexFirstBitSegment=16
 +GameplayTagList=(Tag="Rune.ID.Burn",DevComment="Yog generated burn rune id")
 +GameplayTagList=(Tag="Rune.ID.Poison",DevComment="Yog generated poison rune id")
 +GameplayTagList=(Tag="Rune.ID.Moonlight",DevComment="Yog generated moonlight rune id")
+
++GameplayTagList=(Tag="State.Combo.ChainActive",DevComment="连击链激活中（冲刺保留），供连携配方 RequiredComboTags 读取")
++GameplayTagList=(Tag="Event.Moonlight.Hit",DevComment="月光主流程：投射物命中目标时发送给角色 ASC")
++GameplayTagList=(Tag="Event.Moonlight.FwdBurn.Hit",DevComment="月焰 LinkFlow：命中事件")
++GameplayTagList=(Tag="Event.Moonlight.FwdPoison.Hit",DevComment="蚀月 LinkFlow：命中事件，触发附加中毒")
diff --git a/Config/Tags/BuffTag.ini b/Config/Tags/BuffTag.ini
index 1c91914f..b5ea9267 100644
--- a/Config/Tags/BuffTag.ini
+++ b/Config/Tags/BuffTag.ini
@@ -61,6 +61,10 @@ GameplayTagList=(Tag="Buff.Status.HitReact",DevComment="【Status】受击硬直
 GameplayTagList=(Tag="Buff.Status.HitStop",DevComment="HitStop 相关状态父节点，子节点由攻击方 FA 授予，OnDamageDealt 节点消费后移除")
 GameplayTagList=(Tag="Buff.Status.HitStop.Freeze",DevComment="【HitStop】命中时暂停攻击方蒙太奇（冻结帧）；由攻击类 FA 或暴击 FA 写入攻击方 ASC，BFNode_HitStop 消费后立即移除")
 GameplayTagList=(Tag="Buff.Status.HitStop.Slow",DevComment="【HitStop】命中时减慢攻击方蒙太奇播放速率（延缓帧），结束后自动追帧；由攻击类 FA 或暴击 FA 写入攻击方 ASC，BFNode_HitStop 消费后立即移除")
+GameplayTagList=(Tag="Buff.Status.FinisherCharge",DevComment="【Status】终结技充能窗口，由 GE_FinisherCharge GrantedTags 授予（5层/8s），GA_FinisherCharge 激活期间维持，层数耗尽或到期自动消失")
+GameplayTagList=(Tag="Buff.Status.FinisherExecuting",DevComment="【Status】正在执行终结技动作，GA_Player_FinisherAttack ActivationOwnedTags 授予；PlayerController 检测此 Tag 时将重攻击路由到 Action.Finisher.Confirm 而非 ComboRuntime")
+GameplayTagList=(Tag="Buff.Status.FinisherMark",DevComment="【Status】终结技印记，由 GE_FinisherMark GrantedTags 授予（12s）；GA_Player_FinisherAttack 攻击落下时扫描此 Tag 引爆；GA_FinisherCharge 窗口结束时自动清除")
+GameplayTagList=(Tag="Buff.Status.FinisherWindowOpen",DevComment="【Status】终结技窗口追踪Tag；GA_FinisherCharge ActivateAbility 时通过 AddLooseGameplayTag 授予，EndAbility 时移除；独立于 GE_FinisherCharge 层数，供 AN_TriggerFinisherAbility 检测（避免最后一层命中后Tag已消失导致终结技无法触发）")
 GameplayTagList=(Tag="Buff.Status.Knockback",DevComment="【Status】击退触发信号，由 FA AddTag 节点写入，GA_Knockback 消费后立即移除")
 GameplayTagList=(Tag="Buff.Status.NextHitCrit",DevComment="【Status】下一次伤害强制暴击信号；弱点窥破（Rune 1009）检测到末击后由 FA Add Tag 授予，C++ 伤害管线在计算暴击前检测此 Tag 并消费（RemoveLooseGameplayTag），实现真实暴击并触发 On Crit Hit 事件；测试版 FA 暂不使用此 Tag")
 GameplayTagList=(Tag="Buff.Status.Poisoned",DevComment="【Status】中毒状态；毒牙（Rune 1011）暴击命中后授予目标，测试版通过 Grant Tag Timed 管理，正式版由 GE_Poison 的 GrantedTags 在 GE 存活期间自动维护，GE 到期时自动移除")
diff --git a/Config/Tags/PlayerGameplayTag.ini b/Config/Tags/PlayerGameplayTag.ini
index 1b43ba5f..ea204a7a 100644
--- a/Config/Tags/PlayerGameplayTag.ini
+++ b/Config/Tags/PlayerGameplayTag.ini
@@ -7,6 +7,12 @@ GameplayTagList=(Tag="Action.Heat.CanPhaseUp",DevComment="允许热度升阶的
 GameplayTagList=(Tag="Action.HitReact",DevComment="【Action】触发受击硬直 GA 的事件信号，由 FA 判断是否需要受击后发送至目标")
 GameplayTagList=(Tag="Action.HitReact.Back",DevComment="【Action】受击动画方向 key：攻击者在目标背面时使用（PassiveMap lookup）")
 GameplayTagList=(Tag="Action.HitReact.Front",DevComment="【Action】受击动画方向 key：攻击者在目标正面时使用（PassiveMap lookup）")
+GameplayTagList=(Tag="Action.ApplyFinisherMark",DevComment="【Action】给目标施加终结技印记事件，由 GA_FinisherCharge 在每次命中时发送至被命中目标，GA_ApplyFinisherMark（预授予所有角色）监听此事件")
+GameplayTagList=(Tag="Action.ClearFinisherMark",DevComment="【Action】清除目标印记事件，GA_FinisherCharge::EndAbility 在窗口结束时发送至所有有印记的目标")
+GameplayTagList=(Tag="Action.Finisher.Confirm",DevComment="【Action】玩家在终结技子弹时间内按下重攻击的确认事件；由 YogPlayerControllerBase::HeavyAtack 检测 Buff.Status.FinisherExecuting 后发送至玩家 ASC")
+GameplayTagList=(Tag="Action.FinisherCharge.Activate",DevComment="【Action】激活/通知 GA_FinisherCharge 的事件；由 FA_FinisherCard_BaseEffect 的 BFNode_SendGameplayEvent 发送至玩家 ASC")
+GameplayTagList=(Tag="Action.FinisherCharge.ChargeConsumed",DevComment="【Action】GA_FinisherCharge 每次攻击命中消耗充能时派发至玩家 ASC 的中间事件；EventData.Target = 被击敌人；FA_Finisher_ChargeHit 通过 WaitGameplayEvent 监听，负责对 LastDamageTarget 执行击退和施加印记")
+GameplayTagList=(Tag="Action.FinisherAttack.DetonateTarget",DevComment="【Action】GA_Player_FinisherAttack 对每个印记目标派发至玩家 ASC 的引爆中间事件；EventData.Target = 被引爆目标，EventMagnitude = 确认倍率（1.0=未确认/2.0=已确认）；FA_Finisher_Detonate 通过 WaitGameplayEvent 监听，负责读取数值表、ApplyEffect、割裂和条件击退")
 GameplayTagList=(Tag="Action.Knockback",DevComment="【Action】触发击退 GA 的事件信号，由 FA 发送至目标，GA AbilityTriggers 监听此 Event")
 GameplayTagList=(Tag="Action.Rune",DevComment="【Action】符文系统发出的内部事件父节点，由 FA BuffFlow 节点发送，C++ GA 监听")
 GameplayTagList=(Tag="Action.Rune.KnockbackApplied",DevComment="【Action】击退成功广播信号；FA_Rune_Knockback 发送至 BuffOwner ASC，供 FA_Rune_KnockbackStagger 通过 Wait Gameplay Event 监听后施加减速")
@@ -14,6 +20,9 @@ GameplayTagList=(Tag="Action.Rune.SlashWaveHit",DevComment="【Action】刀光
 GameplayTagList=(Tag="Action.Rune.MoonlightBurnHit",DevComment="512 Moonlight burn projectile hit event. Slash-wave projectile sends this to the player ASC; LinkFA listens before playing compact burn hit VFX on the enemy surface.")
 GameplayTagList=(Tag="Action.Rune.MoonlightPoisonHit",DevComment="512 Moonlight poison projectile hit event. Slash-wave projectile sends this to the player ASC; LinkFA listens before playing poison VFX and applying poison propagation.")
 GameplayTagList=(Tag="Action.Rune.MoonlightPoisonExpired",DevComment="512 Moonlight poison projectile expired event. Reserved for expire VFX or cleanup.")
+GameplayTagList=(Tag="Ability.Event.Finisher.HitFrame",DevComment="【Event】终结技蒙太奇攻击判定帧事件；由 AN_MeleeDamage（EventTag 配置为此）发送至玩家 ASC；GA_Player_FinisherAttack 的 PlayMontageAndWaitForEvent 监听后执行引爆逻辑")
+GameplayTagList=(Tag="Action.Player.FinisherAttack",DevComment="【Action】触发终结技主 GA（GA_Player_FinisherAttack）的事件；由 AN_TriggerFinisherAbility 在 H3 蒙太奇回收帧发送至玩家 ASC")
+GameplayTagList=(Tag="Action.Slash",DevComment="【Action】触发割裂效果（移动时掉血）的事件信号，由 GA_Player_FinisherAttack 在引爆印记时发送至目标")
 GameplayTagList=(Tag="Action.UpperCut",DevComment="")
 GameplayTagList=(Tag="Block.AI",DevComment="【Block】阻断分类 Key：AI 行为树逻辑，配置在 StateConflictDA.BlockCategoryMap")
 GameplayTagList=(Tag="Block.Movement",DevComment="【Block】阻断分类 Key：角色移动，配置在 StateConflictDA.BlockCategoryMap")
@@ -35,6 +44,8 @@ GameplayTagList=(Tag="PlayerState.AbilityCast.LightAtk.Combo1",DevComment="")
 GameplayTagList=(Tag="PlayerState.AbilityCast.LightAtk.Combo2",DevComment="")
 GameplayTagList=(Tag="PlayerState.AbilityCast.LightAtk.Combo3",DevComment="")
 GameplayTagList=(Tag="PlayerState.AbilityCast.LightAtk.Combo4",DevComment="")
+GameplayTagList=(Tag="PlayerState.AbilityCast.Finisher",DevComment="终结技主 GA（GA_Player_FinisherAttack）AbilityTag，配置在 CancelAbilitiesWithTag 可取消普攻连招")
+GameplayTagList=(Tag="PlayerState.AbilityCast.FinisherCharge",DevComment="终结技充能窗口管理 GA（GA_FinisherCharge）AbilityTag；ActivationOwnedTags 授予时阻断自身重复激活")
 GameplayTagList=(Tag="PlayerState.AbilityCast.Skill",DevComment="玩家主动技能父节点")
 GameplayTagList=(Tag="PlayerState.AbilityCast.Skill.Skill1",DevComment="玩家施法动作1")
 GameplayTagList=(Tag="PlayerState.AbilityCast.Skill.Skill2",DevComment="玩家施法动作2")
warning: in the working copy of 'Config/DefaultGameplayTags.ini', LF will be replaced by CRLF the next time Git touches it

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source,Content,Config,Docs -Include *.cpp,*.h,*.ini,*.md,*.uasset -ErrorAction SilentlyContinue | Select-String -Pattern 'UYogFlowNode_SpawnSlashWave|YogFlowNode_SpawnSlashWave|SpawnSlashWave' -ErrorAction SilentlyContinue | Select-Object -First 200 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
2026-05-09T08:43:02.209223Z ERROR codex_core::tools::router: error=Exit code: 124
Wall time: 120.4 seconds
Output:
command timed out after 120357 milliseconds
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:97:SpawnSlashWave();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:101:void UGA_SlashWaveCounter::SpawnSlashWave()
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:1:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:14:UBFNode_SpawnSlashWaveProjectile::UBFNode_SpawnSlashWaveProjectile(const FObjectInitializer& ObjectInitializer)
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:24:void UBFNode_SpawnSlashWaveProjectile::ExecuteInput(const FName& PinName)
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:208:TEXT("[SpawnSlashWaveProjectile] Class=%s Count=%d ComboBonus=%d Sequential=%d SequentialInterval=%.2f Cone=%.1f Damage=%.1f Speed=%.1f Distance=%.1f HitCount=%d DamageApps=%d DamageInterval=%.2f CollisionExtent=%s VisualWithCollision=%d VisualMultiplier=%s ProjectileVisual=%s HideDefault=%d"),
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:230:void UBFNode_SpawnSlashWaveProjectile::Cleanup()
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:250:float UBFNode_SpawnSlashWaveProjectile::ResolveDamage(ACharacter* SourceCharacter) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:266:void UBFNode_SpawnSlashWaveProjectile::ConsumeSourceArmor(ACharacter* SourceCharacter) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:284:void UBFNode_SpawnSlashWaveProjectile::SpawnLaunchNiagara(
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\CombatDeckComponent.cpp:5:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\CombatDeckComponent.cpp:32:const UBFNode_SpawnSlashWaveProjectile* ProjectileNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:11:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:633:if (Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:27:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1411:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1414:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1444:UBFNode_SpawnSlashWaveProjectile* FlowSlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1447:FlowSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1604:bSplitFlowHasSlashWave |= Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value) != nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1631:UBFNode_SpawnSlashWaveProjectile* ReversedSplitSlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1634:ReversedSplitSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1810:if (const UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value))
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1950:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1960:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:2051:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:2063:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:116:void SpawnSlashWave();
D:\Self\GItGame\Dev01\Source\DevKit\Public\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.h:7:#include "BFNode_SpawnSlashWaveProjectile.generated.h"
D:\Self\GItGame\Dev01\Source\DevKit\Public\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.h:20:class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:24:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:1272:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:1970:const UBFNode_SpawnSlashWaveProjectile* SlashNode,
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2128:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2130:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2215:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2217:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2567:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2569:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2763:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2765:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2872:TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara `%s` -> persistent UGE_RuneBurn DOT; cleared Flipbook nodes=%d."),
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2911:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2913:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:3090:TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara hit -> Apply GE_Poison x3 -> Play Niagara spread -> ApplyGEInRadius radius=300 max=3; cleared Flipbook nodes=%d."),
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Base.uasset:6:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Attack.uasset:6:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Burn.uasset:9:AssetGuid���AutoInputDataPins����AutoOutputDataPins�E]�!bAddComboStacksToProjectileCountr��bAllowDeletion���BFNode_ApplyEffect]�Z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Pierce.uasset:9:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���bDestroyOnWorldStaticHit��z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��bForcePureDamageM��BonusArmorDamageMultiplier�@M
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Poison.uasset:9:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�!bAddComboStacksToProjectileCountr��bAllowDeletion���bAttachToTarget�Ag�bExcludeLocationSourceActor_ۓ�BFNode_ApplyEffect]�ZBFNode_ApplyGEInRadiusSUBFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_ReduceDamage.uasset:7:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Shield.uasset:8:bAllowDeletion���bConsumeSourceArmorOnSpawnZ�t�"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Split.uasset:9:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Attack.uasset:7:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Burn.uasset:7:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�bAllowDeletion���bApplyOncePerTargetFm�BFNode_ApplyEffect]�Z#BFNode_CalcRuneGroundPathTransform�V��BFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��BFNode_SpawnRuneAreaProfile�K�E!BFNode_SpawnRuneGroundPathEffect��G BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��CbLoop�|��body�+�Qbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Pierce.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BonusArmorDamageMultiplier�@M
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Poison.uasset:10:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�bAllowDeletion���bAttachToTarget�Ag�bExcludeLocationSourceActor_ۓ�BFNode_ApplyEffect]�ZBFNode_ApplyGEInRadiusSU#BFNode_CalcRuneGroundPathTransform�V��BFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��BFNode_SpawnRuneAreaProfile�K�E!BFNode_SpawnRuneGroundPathEffect��G BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_ReduceDamage.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Shield.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Split.uasset:10:AssetGuid���bAllowDeletion���bBounceSplitChildrenOnEnemyHit�Y#�bDestroyOnWorldStaticHit��z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Pierce_Base.uasset:12:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Backward.uasset:8:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Base.uasset:6:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Forward.uasset:7:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��

 exited 124 in 120357ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:97:SpawnSlashWave();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:101:void UGA_SlashWaveCounter::SpawnSlashWave()
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:1:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:14:UBFNode_SpawnSlashWaveProjectile::UBFNode_SpawnSlashWaveProjectile(const FObjectInitializer& ObjectInitializer)
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:24:void UBFNode_SpawnSlashWaveProjectile::ExecuteInput(const FName& PinName)
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:208:TEXT("[SpawnSlashWaveProjectile] Class=%s Count=%d ComboBonus=%d Sequential=%d SequentialInterval=%.2f Cone=%.1f Damage=%.1f Speed=%.1f Distance=%.1f HitCount=%d DamageApps=%d DamageInterval=%.2f CollisionExtent=%s VisualWithCollision=%d VisualMultiplier=%s ProjectileVisual=%s HideDefault=%d"),
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:230:void UBFNode_SpawnSlashWaveProjectile::Cleanup()
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:250:float UBFNode_SpawnSlashWaveProjectile::ResolveDamage(ACharacter* SourceCharacter) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:266:void UBFNode_SpawnSlashWaveProjectile::ConsumeSourceArmor(ACharacter* SourceCharacter) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:284:void UBFNode_SpawnSlashWaveProjectile::SpawnLaunchNiagara(
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\CombatDeckComponent.cpp:5:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\CombatDeckComponent.cpp:32:const UBFNode_SpawnSlashWaveProjectile* ProjectileNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:11:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\SacrificeRuneComponent.cpp:633:if (Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:27:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1411:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1414:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1444:UBFNode_SpawnSlashWaveProjectile* FlowSlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1447:FlowSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1604:bSplitFlowHasSlashWave |= Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value) != nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1631:UBFNode_SpawnSlashWaveProjectile* ReversedSplitSlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1634:ReversedSplitSlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1810:if (const UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value))
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1950:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:1960:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:2051:UBFNode_SpawnSlashWaveProjectile* SlashNode = nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Tests\CombatDeckComponentTests.cpp:2063:SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:116:void SpawnSlashWave();
D:\Self\GItGame\Dev01\Source\DevKit\Public\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.h:7:#include "BFNode_SpawnSlashWaveProjectile.generated.h"
D:\Self\GItGame\Dev01\Source\DevKit\Public\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.h:20:class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:24:#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:1272:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:1970:const UBFNode_SpawnSlashWaveProjectile* SlashNode,
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2128:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2130:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2215:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2217:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2567:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2569:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2763:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2765:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2872:TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara `%s` -> persistent UGE_RuneBurn DOT; cleared Flipbook nodes=%d."),
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2911:UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:2913:[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
D:\Self\GItGame\Dev01\Source\DevKitEditor\Rune\RuneCardBatchGeneratorCommandlet.cpp:3090:TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara hit -> Apply GE_Poison x3 -> Play Niagara spread -> ApplyGEInRadius radius=300 max=3; cleared Flipbook nodes=%d."),
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Base.uasset:6:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Attack.uasset:6:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Burn.uasset:9:AssetGuid���AutoInputDataPins����AutoOutputDataPins�E]�!bAddComboStacksToProjectileCountr��bAllowDeletion���BFNode_ApplyEffect]�Z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Pierce.uasset:9:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���bDestroyOnWorldStaticHit��z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��bForcePureDamageM��BonusArmorDamageMultiplier�@M
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Poison.uasset:9:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�!bAddComboStacksToProjectileCountr��bAllowDeletion���bAttachToTarget�Ag�bExcludeLocationSourceActor_ۓ�BFNode_ApplyEffect]�ZBFNode_ApplyGEInRadiusSUBFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_ReduceDamage.uasset:7:AssetGuid���!bAddComboStacksToProjectileCountr��bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Shield.uasset:8:bAllowDeletion���bConsumeSourceArmorOnSpawnZ�t�"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Forward_Split.uasset:9:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Attack.uasset:7:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Burn.uasset:7:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�bAllowDeletion���bApplyOncePerTargetFm�BFNode_ApplyEffect]�Z#BFNode_CalcRuneGroundPathTransform�V��BFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��BFNode_SpawnRuneAreaProfile�K�E!BFNode_SpawnRuneGroundPathEffect��G BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��CbLoop�|��body�+�Qbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Pierce.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��BonusArmorDamageMultiplier�@M
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Poison.uasset:10:AttachTargetT�>�AutoInputDataPins����AutoOutputDataPins�E]�bAllowDeletion���bAttachToTarget�Ag�bExcludeLocationSourceActor_ۓ�BFNode_ApplyEffect]�ZBFNode_ApplyGEInRadiusSU#BFNode_CalcRuneGroundPathTransform�V��BFNode_PlayFlipbookVFX9-��BFNode_PlayNiagara|��BFNode_SpawnRuneAreaProfile�K�E!BFNode_SpawnRuneGroundPathEffect��G BFNode_SpawnSlashWaveProjectile�I��BFNode_WaitGameplayEvent,{�bGEAppliedg��Cbool.d�
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_ReduceDamage.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Shield.uasset:6:AssetGuid���bAllowDeletion���"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Moonlight_Reversed_Split.uasset:10:AssetGuid���bAllowDeletion���bBounceSplitChildrenOnEnemyHit�Y#�bDestroyOnWorldStaticHit��z"BFNode_SpawnRuneProjectileProfile��c� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\512Generated\Flow\FA_Rune512_Pierce_Base.uasset:12:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Backward.uasset:8:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Base.uasset:6:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��
D:\Self\GItGame\Dev01\Content\Docs\BuffDocs\V2-RuneCard\GenericRune\FA_Moonlight_Forward.uasset:7:AssetGuid���bAllowDeletion��� BFNode_SpawnSlashWaveProjectile�I��

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git diff --name-only -- Source | Sort-Object' in D:\Self\GItGame\Dev01
 succeeded in 665ms:
Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
Source/DevKit/Private/BuffFlow/Nodes/YogFlowNodes.cpp
Source/DevKit/Private/BuffFlow/YogRuneFlowAsset.cpp
Source/DevKit/Private/Character/YogPlayerControllerBase.cpp
Source/DevKit/Private/Component/ComboRuntimeComponent.cpp
Source/DevKit/Private/Projectile/MusketBullet.cpp
Source/DevKit/Public/BuffFlow/Nodes/BFNode_AddTag.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyExecution.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_AreaDamage.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcDamage.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_CheckDistance.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareFloat.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareInt.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_Delay.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_DestroyNiagara.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoDamage.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoOnce.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetAttribute.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneInfo.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneTuningValue.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantGA.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantSacrificePassive.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantTag.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_HasTag.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_IfStatement.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathFloat.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathInt.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnDamageDealt.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHealthChanged.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPeriodic.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagAdded.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagRemoved.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayMontage.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayNiagara.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_RemoveTag.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SendGameplayEvent.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_TrackMovement.h
Source/DevKit/Public/BuffFlow/Nodes/BFNode_WaitGameplayEvent.h
Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h
Source/DevKit/Public/Projectile/MusketBullet.h
Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h
warning: in the working copy of 'Source/DevKit/Private/Projectile/MusketBullet.cpp', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_AddTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyEffect.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyExecution.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_AreaDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CheckDistance.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareFloat.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_CompareInt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_Delay.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DestroyNiagara.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoDamage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_DoOnce.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetAttribute.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneInfo.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GetRuneTuningValue.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantGA.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantSacrificePassive.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_GrantTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_HasTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_IfStatement.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathFloat.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_MathInt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnDamageDealt.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnHealthChanged.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnPeriodic.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagAdded.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_OnTagRemoved.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayMontage.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayNiagara.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_RemoveTag.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SendGameplayEvent.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnActorAtLocation.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueAtLocation.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnGameplayCueOnActor.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_TrackMovement.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_WaitGameplayEvent.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/BuffFlow/Nodes/YogFlowNodes.h', LF will be replaced by CRLF the next time Git touches it
warning: in the working copy of 'Source/DevKit/Public/Projectile/MusketBullet.h', LF will be replaced by CRLF the next time Git touches it

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.cpp' | ForEach-Object { "'$i++; if($i -ge 1 -and $i -le 120){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 715ms:
   1: #include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
   2: 
   3: #include "AbilitySystem/Attribute/BaseAttributeSet.h"
   4: #include "AbilitySystemBlueprintLibrary.h"
   5: #include "AbilitySystemComponent.h"
   6: #include "BuffFlow/BuffFlowComponent.h"
   7: #include "Components/SkeletalMeshComponent.h"
   8: #include "GameFramework/Character.h"
   9: #include "NiagaraComponent.h"
  10: #include "NiagaraFunctionLibrary.h"
  11: #include "Projectile/SlashWaveProjectile.h"
  12: #include "TimerManager.h"
  13: 
  14: UBFNode_SpawnSlashWaveProjectile::UBFNode_SpawnSlashWaveProjectile(const FObjectInitializer& ObjectInitializer)
  15: 	: Super(ObjectInitializer)
  16: {
  17: #if WITH_EDITOR
  18: 	Category = TEXT("BuffFlow|Projectile");
  19: #endif
  20: 	InputPins = { FFlowPin(TEXT("In")) };
  21: 	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
  22: }
  23: 
  24: void UBFNode_SpawnSlashWaveProjectile::ExecuteInput(const FName& PinName)
  25: {
  26: 	if (!ProjectileClass)
  27: 	{
  28: 		TriggerOutput(TEXT("Failed"), true);
  29: 		return;
  30: 	}
  31: 
  32: 	AActor* SourceActor = ResolveTarget(SourceSelector);
  33: 	ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
  34: 	if (!SourceCharacter || !SourceCharacter->GetWorld())
  35: 	{
  36: 		TriggerOutput(TEXT("Failed"), true);
  37: 		return;
  38: 	}
  39: 
  40: 	const float ResolvedDamage = ResolveDamage(SourceCharacter);
  41: 	ConsumeSourceArmor(SourceCharacter);
  42: 
  43: 	FTransform SourceTransformOverride;
  44: 	const bool bUseSourceTransformOverride = GetBuffFlowComponent()
  45: 		&& GetBuffFlowComponent()->GetActiveSourceTransformOverride(SourceTransformOverride);
  46: 	FVector Forward = bUseSourceTransformOverride
  47: 		? SourceTransformOverride.GetRotation().GetForwardVector()
  48: 		: SourceCharacter->GetActorForwardVector();
  49: 	Forward.Z = 0.0f;
  50: 	Forward = Forward.GetSafeNormal();
  51: 	if (Forward.IsNearlyZero())
  52: 	{
  53: 		Forward = SourceCharacter->GetActorForwardVector();
  54: 	}
  55: 	FVector Right = bUseSourceTransformOverride
  56: 		? SourceTransformOverride.GetRotation().GetRightVector()
  57: 		: SourceCharacter->GetActorRightVector();
  58: 	Right.Z = 0.0f;
  59: 	Right = Right.GetSafeNormal();
  60: 	if (Right.IsNearlyZero())
  61: 	{
  62: 		Right = SourceCharacter->GetActorRightVector();
  63: 	}
  64: 	const FVector SourceLocation = bUseSourceTransformOverride
  65: 		? SourceTransformOverride.GetLocation()
  66: 		: SourceCharacter->GetActorLocation();
  67: 	const FVector Up = FVector::UpVector;
  68: 	const FVector SpawnLocation = SourceLocation
  69: 		+ Forward * SpawnOffset.X
  70: 		+ Right * SpawnOffset.Y
  71: 		+ Up * SpawnOffset.Z;
  72: 	const FRotator BaseSpawnRotation = Forward.Rotation();
  73: 
  74: 	SpawnLaunchNiagara(SourceCharacter, SpawnLocation, BaseSpawnRotation);
  75: 
  76: 	FActorSpawnParameters SpawnParams;
  77: 	SpawnParams.Owner = SourceCharacter;
  78: 	SpawnParams.Instigator = SourceCharacter;
  79: 	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  80: 
  81: 	FSlashWaveProjectileRuntimeConfig Config;
  82: 	Config.Damage = ResolvedDamage;
  83: 	Config.DamageEffect = DamageEffect;
  84: 	Config.Speed = Speed;
  85: 	Config.MaxDistance = MaxDistance;
  86: 	Config.MaxHitCount = MaxHitCount;
  87: 	Config.DamageApplicationsPerTarget = DamageApplicationsPerTarget;
  88: 	Config.DamageApplicationInterval = DamageApplicationInterval;
  89: 	Config.CollisionBoxExtent = CollisionBoxExtent;
  90: 	Config.bScaleVisualWithCollisionExtent = bScaleVisualWithCollisionExtent;
  91: 	Config.VisualScaleMultiplier = VisualScaleMultiplier;
  92: 	Config.ProjectileVisualNiagaraSystem = ProjectileVisualNiagaraSystem;
  93: 	Config.ProjectileVisualNiagaraScale = ProjectileVisualNiagaraScale;
  94: 	Config.bHideDefaultProjectileVisuals = bHideDefaultProjectileVisuals;
  95: 	Config.HitNiagaraSystem = HitNiagaraSystem;
  96: 	Config.HitNiagaraScale = HitNiagaraScale;
  97: 	Config.ExpireNiagaraSystem = ExpireNiagaraSystem;
  98: 	Config.ExpireNiagaraScale = ExpireNiagaraScale;
  99: 	Config.HitGameplayEventTag = HitGameplayEventTag;
 100: 	Config.ExpireGameplayEventTag = ExpireGameplayEventTag;
 101: 	Config.DamageLogType = DamageLogType;
 102: 	Config.bDestroyOnWorldStaticHit = bDestroyOnWorldStaticHit;
 103: 	Config.bForcePureDamage = bForcePureDamage;
 104: 	Config.BonusArmorDamageMultiplier = BonusArmorDamageMultiplier;
 105: 	Config.AdditionalHitEffect = AdditionalHitEffect;
 106: 	Config.AdditionalHitSetByCallerTag = AdditionalHitSetByCallerTag;
 107: 	Config.AdditionalHitSetByCallerValue = AdditionalHitSetByCallerValue;
 108: 	Config.bSplitOnFirstHit = bSplitOnFirstHit;
 109: 	Config.MaxSplitGenerations = MaxSplitGenerations;
 110: 	Config.SplitProjectileCount = SplitProjectileCount;
 111: 	Config.SplitConeAngleDegrees = SplitConeAngleDegrees;
 112: 	Config.bRandomizeSplitDirections = bRandomizeSplitDirections;
 113: 	Config.SplitRandomYawJitterDegrees = SplitRandomYawJitterDegrees;
 114: 	Config.SplitRandomPitchDegrees = SplitRandomPitchDegrees;
 115: 	Config.SplitDamageMultiplier = SplitDamageMultiplier;
 116: 	Config.SplitSpeedMultiplier = SplitSpeedMultiplier;
 117: 	Config.SplitMaxDistanceMultiplier = SplitMaxDistanceMultiplier;
 118: 	Config.SplitCollisionBoxExtentMultiplier = SplitCollisionBoxExtentMultiplier;
 119: 	Config.bBounceOnEnemyHit = bBounceSplitChildrenOnEnemyHit;
 120: 	Config.MaxEnemyBounces = SplitChildMaxEnemyBounces;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.cpp' | ForEach-Object { "'$i++; if($i -ge 120 -and $i -le 190){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 604ms:
 120: 	Config.MaxEnemyBounces = SplitChildMaxEnemyBounces;
 121: 
 122: 	int32 ComboBonusProjectiles = 0;
 123: 	if (bAddComboStacksToProjectileCount)
 124: 	{
 125: 		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
 126: 		{
 127: 			if (BFC->HasCombatCardEffectContext())
 128: 			{
 129: 				const FCombatCardEffectContext& CombatCardContext = BFC->GetLastCombatCardEffectContext();
 130: 				ComboBonusProjectiles = CombatCardContext.ComboBonusStacks * FMath::Max(0, ProjectilesPerComboStack);
 131: 				ComboBonusProjectiles = FMath::Min(FMath::Max(0, MaxBonusProjectiles), ComboBonusProjectiles);
 132: 			}
 133: 		}
 134: 	}
 135: 
 136: 	const int32 SpawnCount = FMath::Max(1, ProjectileCount + ComboBonusProjectiles);
 137: 	const bool bUseSequentialSpawn = bSpawnProjectilesSequentially && SpawnCount > 1;
 138: 	const float ClampedCone = bUseSequentialSpawn ? 0.f : FMath::Clamp(ProjectileConeAngleDegrees, 0.f, 180.f);
 139: 	const float Step = SpawnCount > 1 ? ClampedCone / static_cast<float>(SpawnCount - 1) : 0.f;
 140: 	const float StartYaw = SpawnCount > 1 ? -ClampedCone * 0.5f : 0.f;
 141: 	const float SequentialInterval = FMath::Max(0.f, SequentialProjectileSpawnInterval);
 142: 
 143: 	TWeakObjectPtr<ACharacter> WeakSourceCharacter(SourceCharacter);
 144: 	const TSubclassOf<ASlashWaveProjectile> SpawnProjectileClass = ProjectileClass;
 145: 	const FSlashWaveProjectileRuntimeConfig SpawnConfig = Config;
 146: 	const FActorSpawnParameters BaseSpawnParams = SpawnParams;
 147: 	auto SpawnProjectileAtRotation = [WeakSourceCharacter, SpawnProjectileClass, SpawnConfig, BaseSpawnParams, SpawnLocation](const FRotator& SpawnRotation) -> bool
 148: 	{
 149: 		ACharacter* Source = WeakSourceCharacter.Get();
 150: 		if (!Source || !Source->GetWorld() || !SpawnProjectileClass)
 151: 		{
 152: 			return false;
 153: 		}
 154: 
 155: 		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
 156: 		ASlashWaveProjectile* Projectile = Source->GetWorld()->SpawnActorDeferred<ASlashWaveProjectile>(
 157: 			SpawnProjectileClass,
 158: 			SpawnTransform,
 159: 			BaseSpawnParams.Owner,
 160: 			BaseSpawnParams.Instigator,
 161: 			BaseSpawnParams.SpawnCollisionHandlingOverride);
 162: 
 163: 		if (Projectile)
 164: 		{
 165: 			Projectile->SetSourceCharacterForSpawn(Source);
 166: 			Projectile->FinishSpawning(SpawnTransform);
 167: 			Projectile->InitProjectileWithConfig(Source, SpawnConfig);
 168: 			return true;
 169: 		}
 170: 		return false;
 171: 	};
 172: 
 173: 	int32 SpawnedOrScheduledCount = 0;
 174: 	for (int32 Index = 0; Index < SpawnCount; ++Index)
 175: 	{
 176: 		const float YawOffset = StartYaw + Step * Index;
 177: 		const FVector Direction = Forward.RotateAngleAxis(YawOffset, FVector::UpVector);
 178: 		const FRotator SpawnRotation = bUseSequentialSpawn ? BaseSpawnRotation : Direction.Rotation();
 179: 
 180: 		if (bUseSequentialSpawn && Index > 0 && SequentialInterval > KINDA_SMALL_NUMBER)
 181: 		{
 182: 			FTimerHandle TimerHandle;
 183: 			SourceCharacter->GetWorld()->GetTimerManager().SetTimer(
 184: 				TimerHandle,
 185: 				FTimerDelegate::CreateLambda([SpawnProjectileAtRotation, SpawnRotation]()
 186: 				{
 187: 					SpawnProjectileAtRotation(SpawnRotation);
 188: 				}),
 189: 				SequentialInterval * static_cast<float>(Index),
 190: 				false);

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp' | ForEach-Object { "'$i++; if($i -le 220){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 561ms:
   1: #include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
   2: 
   3: #include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
   4: #include "AbilitySystemBlueprintLibrary.h"
   5: #include "AbilitySystemComponent.h"
   6: #include "BuffFlow/BuffFlowComponent.h"
   7: #include "Character/PlayerCharacterBase.h"
   8: #include "Components/SkeletalMeshComponent.h"
   9: #include "GameFramework/Character.h"
  10: #include "Item/Weapon/WeaponInstance.h"
  11: #include "Projectile/MusketBullet.h"
  12: #include "Types/FlowDataPinResults.h"
  13: 
  14: UBFNode_SpawnRangedProjectiles::UBFNode_SpawnRangedProjectiles(const FObjectInitializer& ObjectInitializer)
  15: 	: Super(ObjectInitializer)
  16: {
  17: #if WITH_EDITOR
  18: 	Category = TEXT("BuffFlow|Projectile");
  19: #endif
  20: 	InputPins = { FFlowPin(TEXT("In")) };
  21: 	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
  22: 	YawOffsets = { -8.f, 8.f };
  23: 	Damage = FFlowDataPinInputProperty_Float(0.f);
  24: 	HitGameplayEventMagnitude = FFlowDataPinInputProperty_Float(0.f);
  25: 	RequiredWeaponTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
  26: }
  27: 
  28: void UBFNode_SpawnRangedProjectiles::ExecuteInput(const FName& PinName)
  29: {
  30: 	ACharacter* SourceCharacter = Cast<ACharacter>(ResolveTarget(SourceSelector));
  31: 	if (!SourceCharacter || !SourceCharacter->GetWorld())
  32: 	{
  33: 		TriggerOutput(TEXT("Failed"), true);
  34: 		return;
  35: 	}
  36: 
  37: 	if (bRequireRangedWeaponTag)
  38: 	{
  39: 		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
  40: 		const FGameplayTag RangedTag = RequiredWeaponTag.IsValid()
  41: 			? RequiredWeaponTag
  42: 			: FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
  43: 		if (!ASC || !RangedTag.IsValid() || !ASC->HasMatchingGameplayTag(RangedTag))
  44: 		{
  45: 			UE_LOG(LogTemp, Verbose,
  46: 				TEXT("[SpawnRangedProjectiles] Skip: Source=%s missing required weapon tag %s."),
  47: 				*GetNameSafe(SourceCharacter),
  48: 				*RangedTag.ToString());
  49: 			TriggerOutput(TEXT("Failed"), true);
  50: 			return;
  51: 		}
  52: 	}
  53: 
  54: 	UBuffFlowComponent* BFC = GetBuffFlowComponent();
  55: 	if (!BFC || !BFC->HasCombatCardEffectContext())
  56: 	{
  57: 		TriggerOutput(TEXT("Failed"), true);
  58: 		return;
  59: 	}
  60: 
  61: 	const FCombatCardEffectContext& CardContext = BFC->GetLastCombatCardEffectContext();
  62: 	const FCombatDeckActionContext& ActionContext = CardContext.ActionContext;
  63: 	const TArray<float> ResolvedYawOffsets = BuildResolvedYawOffsets(CardContext.ComboBonusStacks);
  64: 
  65: 	float ResolvedDamage = ActionContext.AttackDamage;
  66: 	if (!bUseCombatCardAttackDamage || ResolvedDamage <= 0.f)
  67: 	{
  68: 		ResolvedDamage = Damage.Value;
  69: 		const FFlowDataPinResult_Float DamageResult = TryResolveDataPinAsFloat(
  70: 			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, Damage));
  71: 		if (DamageResult.Result == EFlowDataPinResolveResult::Success)
  72: 		{
  73: 			ResolvedDamage = DamageResult.Value;
  74: 		}
  75: 	}
  76: 
  77: 	TSubclassOf<AMusketBullet> ResolvedBulletClass = bPreferCombatCardProjectileClass && ActionContext.RangedProjectileClass
  78: 		? ActionContext.RangedProjectileClass
  79: 		: BulletClass;
  80: 	if (!ResolvedBulletClass)
  81: 	{
  82: 		ResolvedBulletClass = AMusketBullet::StaticClass();
  83: 	}
  84: 
  85: 	TSubclassOf<UGameplayEffect> ResolvedDamageEffect = bPreferCombatCardDamageEffectClass && ActionContext.RangedDamageEffectClass
  86: 		? ActionContext.RangedDamageEffectClass
  87: 		: DamageEffectClass;
  88: 	if (!ResolvedDamageEffect)
  89: 	{
  90: 		ResolvedDamageEffect = UGE_MusketBullet_Damage::StaticClass();
  91: 	}
  92: 
  93: 	const FVector SpawnLocation = ResolveMuzzleLocation(SourceCharacter);
  94: 	const float BaseYaw = SourceCharacter->GetActorRotation().Yaw + ActionContext.RangedBaseYawOffsetDeg;
  95: 	const FGuid SharedGuid = bShareAttackInstanceGuid ? ActionContext.AttackInstanceGuid : FGuid();
  96: 
  97: 	FActorSpawnParameters Params;
  98: 	Params.Instigator = Cast<APawn>(SourceCharacter);
  99: 	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 100: 
 101: 	int32 SpawnedCount = 0;
 102: 	float ResolvedHitEventMagnitude = HitGameplayEventMagnitude.Value;
 103: 	if (HitGameplayEventTag.IsValid() && !bUseDamageAsHitGameplayEventMagnitude)
 104: 	{
 105: 		const FFlowDataPinResult_Float EventMagnitudeResult = TryResolveDataPinAsFloat(
 106: 			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRangedProjectiles, HitGameplayEventMagnitude));
 107: 		if (EventMagnitudeResult.Result == EFlowDataPinResolveResult::Success)
 108: 		{
 109: 			ResolvedHitEventMagnitude = EventMagnitudeResult.Value;
 110: 		}
 111: 	}
 112: 
 113: 	for (const float YawOffset : ResolvedYawOffsets)
 114: 	{
 115: 		const FRotator SpawnRotation(0.f, BaseYaw + YawOffset, 0.f);
 116: 		AMusketBullet* Bullet = SourceCharacter->GetWorld()->SpawnActor<AMusketBullet>(
 117: 			ResolvedBulletClass,
 118: 			SpawnLocation,
 119: 			SpawnRotation,
 120: 			Params);
 121: 
 122: 		if (!Bullet)
 123: 		{
 124: 			continue;
 125: 		}
 126: 
 127: 		Bullet->InitBullet(SourceCharacter, ResolvedDamage, ResolvedDamageEffect);
 128: 		Bullet->SetCombatDeckContextWithGuid(
 129: 			ActionContext.ActionType,
 130: 			ActionContext.bIsComboFinisher,
 131: 			ActionContext.bFromDashSave,
 132: 			SharedGuid,
 133: 			ResolvedDamage);
 134: 		Bullet->SetHitGameplayEvent(
 135: 			HitGameplayEventTag,
 136: 			bSendHitGameplayEventToSourceASC,
 137: 			bUseDamageAsHitGameplayEventMagnitude,
 138: 			ResolvedHitEventMagnitude);
 139: 		++SpawnedCount;
 140: 	}
 141: 
 142: 	UE_LOG(LogTemp, Warning,
 143: 		TEXT("[SpawnRangedProjectiles] Source=%s Count=%d Damage=%.1f BaseYaw=%.1f Guid=%s Bullet=%s"),
 144: 		*GetNameSafe(SourceCharacter),
 145: 		SpawnedCount,
 146: 		ResolvedDamage,
 147: 		BaseYaw,
 148: 		*SharedGuid.ToString(),
 149: 		*GetNameSafe(ResolvedBulletClass.Get()));
 150: 
 151: 	TriggerOutput(SpawnedCount > 0 ? TEXT("Out") : TEXT("Failed"), true);
 152: }
 153: 
 154: TArray<float> UBFNode_SpawnRangedProjectiles::BuildResolvedYawOffsets(int32 ComboBonusStacks) const
 155: {
 156: 	int32 BonusProjectileCount = 0;
 157: 	if (bAddComboStacksToProjectileCount)
 158: 	{
 159: 		BonusProjectileCount = FMath::Min(
 160: 			FMath::Max(0, MaxBonusProjectiles),
 161: 			FMath::Max(0, ComboBonusStacks) * FMath::Max(0, ProjectilesPerComboStack));
 162: 	}
 163: 
 164: 	TArray<float> Result;
 165: 	if (!bUseProjectileCountPattern && BonusProjectileCount <= 0)
 166: 	{
 167: 		Result = YawOffsets;
 168: 		if (Result.Num() == 0)
 169: 		{
 170: 			Result.Add(0.f);
 171: 		}
 172: 		return Result;
 173: 	}
 174: 
 175: 	const int32 BaseProjectileCount = bUseProjectileCountPattern
 176: 		? FMath::Max(1, ProjectileCount)
 177: 		: FMath::Max(1, YawOffsets.Num());
 178: 	const int32 FinalProjectileCount = FMath::Max(1, BaseProjectileCount + BonusProjectileCount);
 179: 	Result.Reserve(FinalProjectileCount);
 180: 
 181: 	if (FinalProjectileCount == 1 || ProjectileConeAngleDegrees <= KINDA_SMALL_NUMBER)
 182: 	{
 183: 		for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
 184: 		{
 185: 			Result.Add(0.f);
 186: 		}
 187: 		return Result;
 188: 	}
 189: 
 190: 	const float Step = ProjectileConeAngleDegrees / static_cast<float>(FinalProjectileCount - 1);
 191: 	const float StartYaw = -ProjectileConeAngleDegrees * 0.5f;
 192: 	for (int32 Index = 0; Index < FinalProjectileCount; ++Index)
 193: 	{
 194: 		Result.Add(StartYaw + Step * static_cast<float>(Index));
 195: 	}
 196: 	return Result;
 197: }
 198: 
 199: FVector UBFNode_SpawnRangedProjectiles::ResolveMuzzleLocation(ACharacter* SourceCharacter) const
 200: {
 201: 	if (!SourceCharacter)
 202: 	{
 203: 		return FVector::ZeroVector;
 204: 	}
 205: 
 206: 	if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
 207: 	{
 208: 		if (Mesh->DoesSocketExist(MuzzleSocketName))
 209: 		{
 210: 			return Mesh->GetSocketLocation(MuzzleSocketName);
 211: 		}
 212: 	}
 213: 
 214: 	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(SourceCharacter))
 215: 	{
 216: 		if (Player->EquippedWeaponInstance)
 217: 		{
 218: 			TArray<USkeletalMeshComponent*> Meshes;
 219: 			Player->EquippedWeaponInstance->GetComponents<USkeletalMeshComponent>(Meshes);
 220: 			for (USkeletalMeshComponent* WeaponMesh : Meshes)

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp' | ForEach-Object { "'$i++; if($i -ge 214 -and $i -le 270){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 648ms:
 214: 	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(SourceCharacter))
 215: 	{
 216: 		if (Player->EquippedWeaponInstance)
 217: 		{
 218: 			TArray<USkeletalMeshComponent*> Meshes;
 219: 			Player->EquippedWeaponInstance->GetComponents<USkeletalMeshComponent>(Meshes);
 220: 			for (USkeletalMeshComponent* WeaponMesh : Meshes)
 221: 			{
 222: 				if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
 223: 				{
 224: 					return WeaponMesh->GetSocketLocation(MuzzleSocketName);
 225: 				}
 226: 			}
 227: 		}
 228: 	}
 229: 
 230: 	return SourceCharacter->GetActorLocation()
 231: 		+ SourceCharacter->GetActorForwardVector() * 80.f
 232: 		+ FVector(0.f, 0.f, 60.f);
 233: }

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/Projectile/MusketBullet.cpp' | ForEach-Object { "'$i++; if($i -ge 1 -and $i -le 380){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 681ms:
   1: // Fill out your copyright notice in the Description page of Project Settings.
   2: 
   3: #include "Projectile/MusketBullet.h"
   4: 
   5: #include "Components/SphereComponent.h"
   6: #include "GameFramework/ProjectileMovementComponent.h"
   7: #include "GameFramework/Character.h"
   8: #include "Abilities/GameplayAbilityTypes.h"
   9: #include "AbilitySystemComponent.h"
  10: #include "AbilitySystemBlueprintLibrary.h"
  11: #include "Character/PlayerCharacterBase.h"
  12: #include "Component/CombatDeckComponent.h"
  13: #include "GameplayEffect.h"
  14: #include "NiagaraComponent.h"
  15: #include "NiagaraFunctionLibrary.h"
  16: #include "TimerManager.h"
  17: 
  18: namespace
  19: {
  20:     FGameplayTag MusketActDamageTag()
  21:     {
  22:         return FGameplayTag::RequestGameplayTag(FName(TEXT("Attribute.ActDamage")));
  23:     }
  24: }
  25: 
  26: AMusketBullet::AMusketBullet()
  27: {
  28:     PrimaryActorTick.bCanEverTick = false;
  29: 
  30:     CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
  31:     CollisionSphere->InitSphereRadius(8.f);
  32:     CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
  33:     CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
  34:     CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
  35:     CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
  36:     SetRootComponent(CollisionSphere);
  37: 
  38:     ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
  39:     ProjectileMovement->InitialSpeed           = Speed;
  40:     ProjectileMovement->MaxSpeed               = Speed;
  41:     ProjectileMovement->ProjectileGravityScale = 0.f;
  42:     ProjectileMovement->bShouldBounce          = false;
  43:     ProjectileMovement->bRotationFollowsVelocity = true;
  44: }
  45: 
  46: void AMusketBullet::InitBullet(ACharacter* InSource, float InDamage,
  47:                                TSubclassOf<UGameplayEffect> InDamageEffect)
  48: {
  49:     SourceCharacter   = InSource;
  50:     DamageMagnitude   = InDamage;
  51:     DamageEffectClass = InDamageEffect;
  52: 
  53:     if (ProjectileMovement)
  54:     {
  55:         ProjectileMovement->InitialSpeed = Speed;
  56:         ProjectileMovement->MaxSpeed     = Speed;
  57:         ProjectileMovement->Velocity     = GetActorForwardVector() * Speed;
  58:     }
  59: 
  60:     ScheduleInitialOverlapCheck();
  61: }
  62: 
  63: void AMusketBullet::SetCombatDeckContext(ECardRequiredAction InActionType, bool bInComboFinisher, bool bInFromDashSave)
  64: {
  65:     SetCombatDeckContextWithGuid(InActionType, bInComboFinisher, bInFromDashSave, FGuid(), 0.f);
  66: }
  67: 
  68: void AMusketBullet::SetCombatDeckContextWithGuid(
  69:     ECardRequiredAction InActionType,
  70:     bool bInComboFinisher,
  71:     bool bInFromDashSave,
  72:     const FGuid& InAttackInstanceGuid,
  73:     float InAttackDamage)
  74: {
  75:     CombatDeckActionType = InActionType;
  76:     bCombatDeckComboFinisher = bInComboFinisher;
  77:     bCombatDeckFromDashSave = bInFromDashSave;
  78:     CombatDeckAttackInstanceGuid = InAttackInstanceGuid;
  79:     CombatDeckAttackDamage = InAttackDamage;
  80: }
  81: 
  82: void AMusketBullet::SetHitGameplayEvent(
  83:     FGameplayTag InEventTag,
  84:     bool bInSendToSourceASC,
  85:     bool bInUseDamageAsMagnitude,
  86:     float InEventMagnitude)
  87: {
  88:     HitGameplayEventTag = InEventTag;
  89:     bSendHitGameplayEventToSourceASC = bInSendToSourceASC;
  90:     bUseDamageAsHitGameplayEventMagnitude = bInUseDamageAsMagnitude;
  91:     HitGameplayEventMagnitude = InEventMagnitude;
  92: }
  93: 
  94: void AMusketBullet::SetProjectileNiagara(
  95:     UNiagaraSystem* InProjectileVisualNiagaraSystem,
  96:     FVector InProjectileVisualNiagaraScale,
  97:     UNiagaraSystem* InHitNiagaraSystem,
  98:     FVector InHitNiagaraScale,
  99:     UNiagaraSystem* InExpireNiagaraSystem,
 100:     FVector InExpireNiagaraScale)
 101: {
 102:     ProjectileVisualNiagaraSystem = InProjectileVisualNiagaraSystem;
 103:     ProjectileVisualNiagaraScale = InProjectileVisualNiagaraScale;
 104:     HitNiagaraSystem = InHitNiagaraSystem;
 105:     HitNiagaraScale = InHitNiagaraScale;
 106:     ExpireNiagaraSystem = InExpireNiagaraSystem;
 107:     ExpireNiagaraScale = InExpireNiagaraScale;
 108: 
 109:     if (HasActorBegunPlay())
 110:     {
 111:         SpawnProjectileVisualNiagara();
 112:     }
 113: }
 114: 
 115: void AMusketBullet::BeginPlay()
 116: {
 117:     Super::BeginPlay();
 118: 
 119:     CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMusketBullet::OnOverlapBegin);
 120: 
 121:     GetWorld()->GetTimerManager().SetTimer(
 122:         LifetimeTimerHandle, this, &AMusketBullet::Expire, Lifetime, false);
 123: 
 124:     ScheduleInitialOverlapCheck();
 125:     SpawnProjectileVisualNiagara();
 126: }
 127: 
 128: void AMusketBullet::OnOverlapBegin(
 129:     UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
 130:     UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
 131:     bool /*bFromSweep*/, const FHitResult& SweepHitResult)
 132: {
 133:     if (bHasHit || !OtherActor || OtherActor == this || OtherActor == SourceCharacter)
 134:         return;
 135: 
 136:     if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
 137:         return;
 138: 
 139:     bHasHit = true;
 140:     GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
 141: 
 142:     const FVector HitLoc = SweepHitResult.ImpactPoint.IsNearlyZero()
 143:         ? OtherActor->GetActorLocation()
 144:         : FVector(SweepHitResult.ImpactPoint);
 145: 
 146:     ApplyDamageTo(OtherActor, HitLoc);
 147:     Destroy();
 148: }
 149: 
 150: void AMusketBullet::ScheduleInitialOverlapCheck()
 151: {
 152:     if (bInitialOverlapCheckScheduled || bHasHit || !SourceCharacter || !HasActorBegunPlay() || !GetWorld())
 153:     {
 154:         return;
 155:     }
 156: 
 157:     bInitialOverlapCheckScheduled = true;
 158:     HandleInitialOverlaps();
 159: 
 160:     if (!bHasHit && !IsActorBeingDestroyed() && GetWorld())
 161:     {
 162:         GetWorld()->GetTimerManager().SetTimerForNextTick(
 163:             FTimerDelegate::CreateUObject(this, &AMusketBullet::HandleInitialOverlaps));
 164:     }
 165: }
 166: 
 167: void AMusketBullet::HandleInitialOverlaps()
 168: {
 169:     if (bHasHit || !CollisionSphere || !SourceCharacter || IsActorBeingDestroyed())
 170:     {
 171:         return;
 172:     }
 173: 
 174:     CollisionSphere->UpdateOverlaps();
 175: 
 176:     TArray<AActor*> OverlappingActors;
 177:     CollisionSphere->GetOverlappingActors(OverlappingActors);
 178: 
 179:     AActor* BestTarget = nullptr;
 180:     float BestDistanceSq = TNumericLimits<float>::Max();
 181:     const FVector Origin = GetActorLocation();
 182: 
 183:     for (AActor* OverlappingActor : OverlappingActors)
 184:     {
 185:         if (!OverlappingActor || OverlappingActor == this || OverlappingActor == SourceCharacter)
 186:         {
 187:             continue;
 188:         }
 189: 
 190:         if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OverlappingActor))
 191:         {
 192:             continue;
 193:         }
 194: 
 195:         const float DistanceSq = FVector::DistSquared(Origin, OverlappingActor->GetActorLocation());
 196:         if (DistanceSq < BestDistanceSq)
 197:         {
 198:             BestDistanceSq = DistanceSq;
 199:             BestTarget = OverlappingActor;
 200:         }
 201:     }
 202: 
 203:     if (!BestTarget)
 204:     {
 205:         return;
 206:     }
 207: 
 208:     bHasHit = true;
 209:     if (GetWorld())
 210:     {
 211:         GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
 212:     }
 213: 
 214:     const FVector HitLoc = BestTarget->GetActorLocation();
 215:     UE_LOG(LogTemp, Warning, TEXT("[MusketBullet] InitialOverlapHit Target=%s"),
 216:         *GetNameSafe(BestTarget));
 217:     ApplyDamageTo(BestTarget, HitLoc);
 218:     Destroy();
 219: }
 220: 
 221: void AMusketBullet::ApplyDamageTo(AActor* Target, const FVector& HitLocation)
 222: {
 223:     if (!Target || !DamageEffectClass || !SourceCharacter) return;
 224: 
 225:     UAbilitySystemComponent* TargetASC =
 226:         UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
 227:     UAbilitySystemComponent* SourceASC =
 228:         UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
 229: 
 230:     if (!TargetASC || !SourceASC) return;
 231: 
 232:     FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
 233:     CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
 234:     CtxHandle.AddSourceObject(this);
 235: 
 236:     FGameplayEffectSpecHandle SpecHandle =
 237:         SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, CtxHandle);
 238: 
 239:     if (SpecHandle.IsValid())
 240:     {
 241:         SpecHandle.Data->SetSetByCallerMagnitude(MusketActDamageTag(), DamageMagnitude);
 242:         SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
 243:     }
 244: 
 245:     SendHitGameplayEvent(Target, SourceASC);
 246:     ResolveCombatDeckOnHit();
 247:     SpawnBurstNiagara(HitNiagaraSystem, HitLocation, HitNiagaraScale);
 248:     BP_OnHitEnemy(Target, HitLocation);
 249: }
 250: 
 251: void AMusketBullet::SendHitGameplayEvent(AActor* Target, UAbilitySystemComponent* SourceASC)
 252: {
 253:     if (!HitGameplayEventTag.IsValid() || !Target || !SourceASC || !SourceCharacter)
 254:     {
 255:         return;
 256:     }
 257: 
 258:     UAbilitySystemComponent* EventASC = bSendHitGameplayEventToSourceASC
 259:         ? SourceASC
 260:         : UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
 261:     if (!EventASC)
 262:     {
 263:         return;
 264:     }
 265: 
 266:     FGameplayEventData Payload;
 267:     Payload.EventTag = HitGameplayEventTag;
 268:     Payload.Instigator = SourceCharacter;
 269:     Payload.Target = Target;
 270:     Payload.EventMagnitude = bUseDamageAsHitGameplayEventMagnitude
 271:         ? DamageMagnitude
 272:         : HitGameplayEventMagnitude;
 273:     Payload.ContextHandle = SourceASC->MakeEffectContext();
 274:     Payload.ContextHandle.AddInstigator(SourceCharacter, SourceCharacter);
 275:     Payload.OptionalObject = this;
 276: 
 277:     EventASC->HandleGameplayEvent(HitGameplayEventTag, &Payload);
 278: }
 279: 
 280: void AMusketBullet::SpawnProjectileVisualNiagara()
 281: {
 282:     if (!ProjectileVisualNiagaraSystem || ProjectileVisualNiagaraComponent)
 283:     {
 284:         return;
 285:     }
 286: 
 287:     ProjectileVisualNiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
 288:         ProjectileVisualNiagaraSystem,
 289:         GetRootComponent(),
 290:         NAME_None,
 291:         FVector::ZeroVector,
 292:         FRotator::ZeroRotator,
 293:         EAttachLocation::KeepRelativeOffset,
 294:         true,
 295:         true);
 296: 
 297:     if (ProjectileVisualNiagaraComponent)
 298:     {
 299:         ProjectileVisualNiagaraComponent->SetRelativeScale3D(ProjectileVisualNiagaraScale);
 300:     }
 301: }
 302: 
 303: void AMusketBullet::SpawnBurstNiagara(UNiagaraSystem* NiagaraSystem, const FVector& WorldLocation, const FVector& Scale) const
 304: {
 305:     if (!NiagaraSystem || !GetWorld())
 306:     {
 307:         return;
 308:     }
 309: 
 310:     UNiagaraFunctionLibrary::SpawnSystemAtLocation(
 311:         GetWorld(),
 312:         NiagaraSystem,
 313:         WorldLocation,
 314:         GetActorRotation(),
 315:         Scale,
 316:         true,
 317:         true);
 318: }
 319: 
 320: void AMusketBullet::ResolveCombatDeckOnHit()
 321: {
 322:     if (bCombatDeckResolved)
 323:     {
 324:         return;
 325:     }
 326: 
 327:     APlayerCharacterBase* PlayerSource = Cast<APlayerCharacterBase>(SourceCharacter);
 328:     if (!PlayerSource || !PlayerSource->CombatDeckComponent)
 329:     {
 330:         return;
 331:     }
 332: 
 333:     FCombatDeckActionContext Context;
 334:     Context.ActionType = CombatDeckActionType;
 335:     Context.bIsComboFinisher = bCombatDeckComboFinisher;
 336:     Context.bFromDashSave = bCombatDeckFromDashSave;
 337:     Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
 338:     Context.AttackInstanceGuid = CombatDeckAttackInstanceGuid.IsValid()
 339:         ? CombatDeckAttackInstanceGuid
 340:         : FGuid::NewGuid();
 341:     Context.AttackDamage = CombatDeckAttackDamage > 0.f ? CombatDeckAttackDamage : DamageMagnitude;
 342: 
 343:     const FCombatCardResolveResult Result = PlayerSource->CombatDeckComponent->ResolveAttackCardWithContext(Context);
 344:     bCombatDeckResolved = Result.bHadCard;
 345: }
 346: 
 347: void AMusketBullet::Expire()
 348: {
 349:     SpawnBurstNiagara(ExpireNiagaraSystem, GetActorLocation(), ExpireNiagaraScale);
 350:     BP_OnMiss();
 351:     Destroy();
 352: }

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit/Private/AbilitySystem/Abilities -Include *.cpp | Select-String -Pattern 'GetAbilitySystemComponent\\(ActorInfo\\)' | Select-Object -First 40 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 632ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_ApplyFinisherMark.cpp:37:UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:51:UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:207:if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo))

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Source/DevKit/Public/AbilitySystem/Abilities/YogGameplayAbility.h' | Select-String -Pattern 'GetAbilitySystemComponent' -Context 2,2" in D:\Self\GItGame\Dev01
 succeeded in 616ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source Plugins -Include *.h,*.cpp -ErrorAction SilentlyContinue | Select-String -Pattern 'GetAbilitySystemComponent\\s*\\(' | Select-Object -First 80 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 809ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source -Include *.h,*.cpp | Select-String -SimpleMatch 'GetAbilitySystemComponent' | Select-Object -First 80 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 698ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:48:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:54:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:65:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:73:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:81:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:87:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:221:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:230:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:239:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_MusketBase.cpp:250:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_ApplyFinisherMark.cpp:37:UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Bleed.cpp:84:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Fear.cpp:99:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:51:UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:115:UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponent(CurrentActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:175:UAbilitySystemComponent* ASC = GetAbilitySystemComponent(CurrentActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:207:if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo))
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Freeze.cpp:57:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:63:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:73:if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:312:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:885:if (UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:889:UAbilitySystemComponent* CombatCardASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayerDash.cpp:273:if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Player_FinisherAttack.cpp:119:UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponent(CurrentActorInfo);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayMontage.cpp:183:if (UAbilitySystemComponent* ASCLocal = GetAbilitySystemComponentFromActorInfo())
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayMontage.cpp:243:UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Rend.cpp:89:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:68:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Wound.cpp:63:UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\DamageExecution.cpp:142:UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\DamageExecution.cpp:318://UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\HealthPercentExecution.cpp:71:UYogAbilitySystemComponent* TargetAbilitySystemComponent = Cast<UYogAbilitySystemComponent>(ExecutionParams.GetTargetAbilitySystemComponent());
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\HealthPercentExecution.cpp:75://UYogGameplayAbility* target_CurrentAbilityClass = TargetAbilitySystemComponent->GetCurrentAbilityClass();
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\HealthPercentExecution.cpp:85:AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\HealthPercentExecution.cpp:110:UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\ExecutionCalculation\HealthPercentExecution.cpp:122://UYogAbilitySystemComponent* TargetASC = Cast<UYogAbilitySystemComponent>(TargetAbilitySystemComponent);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\GameplayEffect\YogGameplayEffect.cpp:19:UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\YogAbilitySystemComponent.cpp:931:UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AI\BTDecorator_HasAbilityWithTag.cpp:21:const UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\AI\BTTask_ActivateAbilityByTag.cpp:31:UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\AI\BTTask_EnemyAttackByProfile.cpp:120:const UAbilitySystemComponent* TargetASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\AI\BTTask_EnemyAttackByProfile.cpp:192:UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AnimNotifyState_AddGameplayTag.cpp:9:return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AnimNotifyState_ComboWindow.cpp:22:return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AnimNotifyState_PostAtkWindow.cpp:68:UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AnimNotify_RemoveGameplayTag.cpp:15:? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\YogAnimInstance.cpp:29:if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Actors\BFAreaDamageZone.cpp:106:UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Actors\BFAreaDamageZone.cpp:108:UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Actors\RuneGroundPathEffectActor.cpp:279:UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_ApplyEffect.cpp:94:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_ApplyExecution.cpp:40:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_ApplyGEInRadius.cpp:152:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_ApplyGEInRadius.cpp:179:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_ApplyRuneEffectProfile.cpp:48:UAbilitySystemComponent* TargetASC = TargetActor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor) : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_CalcDamage.cpp:28:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_DoDamage.cpp:44:UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_GrantTag.cpp:48:UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnRangedProjectiles.cpp:39:UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnRuneAreaProfile.cpp:50:UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnRuneGroundPathEffect.cpp:38:UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnRuneProjectileProfile.cpp:276:if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter))
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnRuneProjectileProfile.cpp:292:UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:258:if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter))
D:\Self\GItGame\Dev01\Source\DevKit\Private\BuffFlow\Nodes\BFNode_SpawnSlashWaveProjectile.cpp:273:UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:146:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:190:if (UAbilitySystemComponent* HeatASC = GetAbilitySystemComponent())
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:297:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:340:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:364:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:393:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:413:if (BackpackGridComponent && GetAbilitySystemComponent())
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:415:BackpackGridComponent->InitWithASC(GetAbilitySystemComponent());
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:419:if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:424:if (SkillChargeComponent && GetAbilitySystemComponent() && PlayerAttributeSet)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:426:SkillChargeComponent->InitWithASC(GetAbilitySystemComponent());
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:481:if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\PlayerCharacterBase.cpp:717:UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\YogCharacterBase.cpp:104:return Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent());

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$paths=@('"'C:\\Program Files\\Epic Games','D:\\UE_5.4','D:\\Epic Games','D:\\Program Files\\Epic Games'); foreach("'$p in $paths){ if(Test-Path $p){ Write-Output "-- $p"; Get-ChildItem -Path $p -Recurse -Filter GameplayAbility.h -ErrorAction SilentlyContinue | Select-Object -First 5 | ForEach-Object { $_.FullName } }}' in D:\Self\GItGame\Dev01
 succeeded in 870ms:
-- C:\Program Files\Epic Games

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path 'C:\\Program Files\\Epic Games' -Filter GameplayAbility.h -ErrorAction SilentlyContinue | Select-Object -First 10 -ExpandProperty FullName" in D:\Self\GItGame\Dev01
 succeeded in 785ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit/Public/AbilitySystem/Abilities -Include *.h | Select-String -Pattern 'UAnimMontage' -Context 0,3 | Select-Object -First 80 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 777ms:
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:43:TObjectPtr<UAnimMontage> FireMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:35:TObjectPtr<UAnimMontage> FireMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_Reload_All.h:30:TObjectPtr<UAnimMontage> ReloadAllMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_Reload_Single.h:37:TObjectPtr<UAnimMontage> ReloadOneMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_SprintAttack.h:37:TObjectPtr<UAnimMontage> SprintAtkMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:144:static UAN_MeleeDamage* GetFirstDamageNotify(UAnimMontage* Montage);
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:146:void ScheduleNodeComboWindow(UAnimMontage* Montage, float PlayRate);
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_Player_FinisherAttack.h:51:TObjectPtr<UAnimMontage> FinisherMontage;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\YogGameplayAbility.h:24:class UAnimMontage;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit/Public/Animation -Include *.h | Select-String -Pattern 'FGameplayTag' -Context 0,0 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 752ms:
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\AnimNotifyState_AddGameplayTag.h:22:FGameplayTagContainer Tags;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\AnimNotifyState_PostAtkWindow.h:43:FGameplayTag TagToClearOnActionInput;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\AnimNotify_RemoveGameplayTag.h:22:FGameplayTag TagToRemove;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\ANS_FinisherTimeDilation.h:27:FGameplayTag PromptShowEventTag;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\ANS_FinisherTimeDilation.h:30:FGameplayTag PromptHideEventTag;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\AN_MeleeDamage.h:49:FGameplayTag EventTag;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\AN_MeleeDamage.h:102:TArray<FGameplayTag> OnHitEventTags;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Animation\YogAnimInstance.h:37:FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'foreach($f in @('"'Source/DevKit/Public/Animation/AnimNotifyState_PostAtkWindow.h','Source/DevKit/Public/Animation/AnimNotify_RemoveGameplayTag.h','Source/DevKit/Public/Animation/AN_MeleeDamage.h')){Write-Output \"--- "'$f"; Get-Content $f -TotalCount 20}' in D:\Self\GItGame\Dev01
 succeeded in 622ms:
--- Source/DevKit/Public/Animation/AnimNotifyState_PostAtkWindow.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_PostAtkWindow.generated.h"

/**
 * AnimNotifyState锛氭敾鍑诲悗鎽囧揩閫熸贩鍑虹獥鍙? *
 * 鏀惧湪钂欏お濂囩殑 Recovery 鍖烘锛堟垨鏁翠釜鍚庢憞娈碉級锛屾瘡 Tick 妫€娴嬩袱绫昏緭鍏ワ細
 *
 *   1. 鏀诲嚮杈撳叆锛圠ightAtk / HeavyAtk锛夛細
 *      娓呴櫎 TagToClearOnActionInput锛堥粯璁?CanCombo锛夛紝钂欏お濂囨湰韬敱
 *      鏂版縺娲荤殑 GA 閫氳繃 GAS EndAbility 娴佺▼鍋滄锛屾棤闇€姝ゅ寮哄埗鎵撴柇銆? *
 *   2. 绉诲姩杈撳叆锛? *      鐩存帴璋冪敤 Montage_Stop(MoveBlendOutTime) 娣峰嚭褰撳墠钂欏お濂囷紝
 *      瑙﹀彂 GA_PlayMontage::OnMontageBlendOut 鈫?EndAbility锛屾仮澶嶇Щ鍔ㄣ€? *
 * 鍙傛暟锛? *   MoveBlendOutTime  鈥?绉诲姩杈撳叆瑙﹀彂鏃剁殑 Blend 鏃堕棿锛堢锛夛紱0 = 绔嬪嵆鍋滄
 *   TagToClearOnActionInput 鈥?妫€娴嬪埌鏀诲嚮杈撳叆鏃惰褰掗浂鐨?Loose Tag
 */
UCLASS(meta = (DisplayName = "Post Attack Window"))
--- Source/DevKit/Public/Animation/AnimNotify_RemoveGameplayTag.h
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_RemoveGameplayTag.generated.h"

/**
 * AnimNotify锛堢偣瑙﹀彂锛夛細鍦ㄨ挋澶鎸囧畾甯у皢 ASC 涓婄殑 Gameplay Tag 璁℃暟寮哄埗褰掗浂銆? * 鍙奖鍝?Loose Tag锛圓ddLooseGameplayTag 娣诲姞鐨勶級锛屼笉褰卞搷 GE-granted tag銆? * 鍏稿瀷鐢ㄩ€旓細鍦ㄨ繛鎷涙渶鍚庝竴鎷涜挋澶鐨?CanCombo 绐楀彛缁撴潫鍚庢寕姝?Notify锛? *           纭繚 PlayerState.AbilityCast.CanCombo 琚共鍑€娓呴櫎銆? */
UCLASS(meta = (DisplayName = "Remove Gameplay Tag"))
class DEVKIT_API UAnimNotify_RemoveGameplayTag : public UAnimNotify
{
    GENERATED_BODY()

public:
    /** 瑕佺Щ闄ょ殑 Tag锛堝叏閮ㄨ鏁板綊闆讹級 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
    FGameplayTag TagToRemove;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
--- Source/DevKit/Public/Animation/AN_MeleeDamage.h
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "Data/AbilityData.h"
#include "AN_MeleeDamage.generated.h"

class URuneDataAsset;
class UMontageAttackDataAsset;

UENUM(BlueprintType)
enum class EHitStopMode : uint8
{
	None    UMETA(DisplayName = "None"),
	Freeze  UMETA(DisplayName = "Freeze"),
	Slow    UMETA(DisplayName = "Slow"),
};

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem Source/DevKit -Filter *.h; Get-Content 'Source/DevKit/DevKit.h' -ErrorAction SilentlyContinue; Get-Content 'Source/DevKit/DevKit.Build.cs' -ErrorAction SilentlyContinue" in D:\Self\GItGame\Dev01
 succeeded in 680ms:
// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class DevKit : ModuleRules
{
	public DevKit(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        string enginePath = Path.GetFullPath(Target.RelativeEnginePath);


        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;


        PublicIncludePaths.AddRange(
        new string[] {
            //"DevKit",
            //"DevKitEditor"
        }
        );

        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "CoreOnline",
                "CoreUObject", 
				"Engine", 
				"InputCore",
                "Niagara",
                "ModularGameplay",
                "ModularGameplayActors",
                "PhysicsCore",
                "GameplayTags",
                "GameplayTasks",
                "DataRegistry",
                "ReplicationGraph",
                "GameFeatures",
                "Hotfix",
                "PropertyPath",
                "CommonUI",
                "CommonInput",
                "NavigationSystem",
                "AIModule",
                "AnimGraphRuntime",
                "GameplayAbilities",
                "Flow",
                "GenericGraphRuntime",

            });

		PrivateDependencyModuleNames.AddRange(new string[] {
                "Core",
                "InputCore",
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "DeveloperSettings",
                "EnhancedInput",
                "NetCore",
                "RHI",
                "Projects",
                "Gauntlet",
                "UMG",
                "AIModule",
                "AnimGraphRuntime"
            });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit -Include *.cpp,*.h | Select-String -Pattern 'AbilityTask_WaitGameplayEvent|EventReceived.AddDynamic' | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 796ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.cpp:6:#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.cpp:82:UAbilityTask_WaitGameplayEvent* ReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.cpp:86:ReleaseTask->EventReceived.AddDynamic(this, &UGA_Musket_HeavyAttack::OnHeavyReleaseEvent);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.cpp:62:Task->EventReceived.AddDynamic(this,  &UGA_Musket_LightAttack::OnFireEvent);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:5:#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:91:WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_FinisherCharge.cpp:98:WaitHitTask->EventReceived.AddDynamic(this, &UGA_FinisherCharge::OnAttackHit);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:4:#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:40:UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_KnockbackDebuff.cpp:42:WaitTask->EventReceived.AddDynamic(this, &UGA_KnockbackDebuff::OnDamageTaken);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_MeleeAttack.cpp:665:Task->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnEventReceived);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Player_FinisherAttack.cpp:72:MontageTask->EventReceived.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageEvent);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayMontage.cpp:104:PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_PlayMontage.cpp:107://PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:3:#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:48:WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_SlashWaveCounter.cpp:56:WaitEventTask->EventReceived.AddDynamic(this, &UGA_SlashWaveCounter::OnAttackEventReceived);
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Wound.cpp:3:#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Wound.cpp:53:UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
D:\Self\GItGame\Dev01\Source\DevKit\Private\AbilitySystem\Abilities\GA_Wound.cpp:55:WaitTask->EventReceived.AddDynamic(this, &UGA_Wound::OnDamageTaken);
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_FinisherCharge.h:7:class UAbilityTask_WaitGameplayEvent;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_FinisherCharge.h:71:TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:8:class UAbilityTask_WaitGameplayEvent;
D:\Self\GItGame\Dev01\Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:110:TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'foreach($f in @('"'Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h','Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_KnockbackDebuff.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_SlashWaveCounter.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_Wound.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_MeleeAttack.h')){Write-Output \"--- "'$f"; Select-String -Path $f -Pattern '"'On.*Event|OnDamageTaken|OnAttackEventReceived|OnEventReceived' -Context 0,2}" in D:\Self\GItGame\Dev01
 succeeded in 752ms:
--- Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h

> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:93:        const FGameplayEventData* Trig
gerEventData) override;
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:94:
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:95:    virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:118:    UFUNCTION() void OnHeavyReleaseEv
ent(FGameplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:119:
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:120:    UFUNCTION() void OnFireMontageCom
plete(FGameplayTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:121:    UFUNCTION() void OnFireMontageBle
ndOut(FGameplayTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:122:    UFUNCTION() void OnFireMontageInt
errupted(FGameplayTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:123:    UFUNCTION() void OnFireMontageCan
celled(FGameplayTag EventTag, FGameplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:124:};
--- Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:59:        const FGameplayEventData* Trig
gerEventData) override;
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:60:
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:61:    virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:73:    UFUNCTION() void OnFireEvent(FGame
playTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:74:    UFUNCTION() void OnMontageComplete
(FGameplayTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:75:    UFUNCTION() void OnMontageInterrup
ted(FGameplayTag EventTag, FGameplayEventData EventData);
> Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:76:    UFUNCTION() void OnMontageCancelle
d(FGameplayTag EventTag, FGameplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_LightAttack.h:77:};
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_KnockbackDebuff.h
> Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:28:		const FGameplayEventData* TriggerEventData) ov
erride;
  Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:29:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:30:	virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:49:	void OnDamageTaken(FGameplayEventData Payload);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:50:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:51:	void OnKnockbackDebuffTagChanged(const FGamepla
yTag Tag, int32 NewCount);
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_SlashWaveCounter.h
> Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:17: *           → FA_Rune_SlashWave 的 OnDamageDeal
t 触发后由 BFNode_SendGameplayEvent 发送
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:18: *       挥刀模式（bHitRequired=false）：监听 Action.Att
ack.Swing
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:19: *           → AN_MeleeDamage::Notify 每次挥刀时发送
> Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:94:	    const FGameplayEventData* TriggerEventData
) override;
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:95:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:96:	virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:113:	void OnAttackEventReceived(FGameplayEventData
 Payload);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:114:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_SlashWaveCounter.h:115:	/** 生成刀光投射物（在角色前方 SpawnOffset 处，沿角色朝向发射）*/
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_Wound.h
> Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:34:		const FGameplayEventData* TriggerEventData) override;
  Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:35:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:36:	virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:58:	void OnDamageTaken(FGameplayEventData Payload);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:59:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_Wound.h:60:	void OnWoundedTagChanged(const FGameplayTag Tag, int32 Ne
wCount);
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_MeleeAttack.h
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:78:		const FGameplayEventData* TriggerEventData) overri
de;
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:79:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:80:	virtual void EndAbility(
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:151:	AActor* PreviewFirstCombatDeckHitTarget(const FGam
eplayEventData& EventData) const;
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:152:	void PrimeCombatDeckHitContext(const FGameplayEven
tData& EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:153:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:154:	void TryResolveCombatDeckOnHit();
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:161:	void OnMontageCompleted(FGameplayTag EventTag, FGa
meplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:162:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:163:	UFUNCTION()
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:164:	void OnMontageBlendOut(FGameplayTag EventTag, FGam
eplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:165:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:166:	UFUNCTION()
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:167:	void OnMontageInterrupted(FGameplayTag EventTag, F
GameplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:168:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:169:	UFUNCTION()
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:170:	void OnMontageCancelled(FGameplayTag EventTag, FGa
meplayEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:171:
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:172:	UFUNCTION()
> Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:173:	void OnEventReceived(FGameplayTag EventTag, FGamep
layEventData EventData);
  Source\DevKit\Public\AbilitySystem\Abilities\GA_MeleeAttack.h:174:};



exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.cpp' | ForEach-Object { "'$i++; if($i -ge 50 -and $i -le 90){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 778ms:
  50:         return;
  51:     }
  52: 
  53:     FGameplayTagContainer EventTags;
  54:     if (FireEventTag.IsValid())
  55:     {
  56:         EventTags.AddTag(FireEventTag);
  57:     }
  58: 
  59:     auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
  60:         this, NAME_None, FireMontage, EventTags, 1.f, NAME_None, true, 1.f);
  61: 
  62:     Task->EventReceived.AddDynamic(this,  &UGA_Musket_LightAttack::OnFireEvent);
  63:     Task->OnCompleted.AddDynamic(this,    &UGA_Musket_LightAttack::OnMontageComplete);
  64:     Task->OnBlendOut.AddDynamic(this,     &UGA_Musket_LightAttack::OnMontageComplete);
  65:     Task->OnInterrupted.AddDynamic(this,  &UGA_Musket_LightAttack::OnMontageInterrupted);
  66:     Task->OnCancelled.AddDynamic(this,    &UGA_Musket_LightAttack::OnMontageCancelled);
  67: 
  68:     Task->ReadyForActivation();
  69: }
  70: 
  71: void UGA_Musket_LightAttack::EndAbility(
  72:     const FGameplayAbilitySpecHandle Handle,
  73:     const FGameplayAbilityActorInfo* ActorInfo,
  74:     const FGameplayAbilityActivationInfo ActivationInfo,
  75:     bool bReplicateEndAbility,
  76:     bool bWasCancelled)
  77: {
  78:     UnlockMovement();
  79:     Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
  80: }
  81: 
  82: void UGA_Musket_LightAttack::DoFire()
  83: {
  84:     if (bFired) return;
  85:     bFired = true;
  86: 
  87:     const float TunedDamageMultiplier = TuningData ? TuningData->LightDamageMultiplier : DamageMultiplier;
  88:     const float TunedHalfAngleDeg = TuningData ? TuningData->LightHalfAngleDeg : HalfAngleDeg;
  89:     const float Damage = GetBaseAttack() * TunedDamageMultiplier;
  90:     const float Angle  = FMath::RandRange(-TunedHalfAngleDeg, TunedHalfAngleDeg);

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'foreach($f in @('"'Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h','Source/DevKit/Public/AbilitySystem/Abilities/GA_KnockbackDebuff.h')){Write-Output \"--- "'$f"; Select-String -Path $f -Pattern '"'OnHeavyReleaseEvent|OnDamageTaken' -Context 0,0} ; "'$i=0; Get-Content '"'Source/DevKit/Private/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.cpp' | ForEach-Object { "'$i++; if($i -ge 76 -and $i -le 92){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 751ms:
--- Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h

Source\DevKit\Public\AbilitySystem\Abilities\Musket\GA_Musket_HeavyAttack.h:118:    UFUNCTION() void OnHeavyReleaseEven
t(FGameplayEventData EventData);
--- Source/DevKit/Public/AbilitySystem/Abilities/GA_KnockbackDebuff.h
Source\DevKit\Public\AbilitySystem\Abilities\GA_KnockbackDebuff.h:49:	void OnDamageTaken(FGameplayEventData Payload);
  76:         this, TunedChargeTime, TunedStartHalfAngle, TunedEndHalfAngle, TunedStartRadius, TunedEndRadius);
  77:     ChargeTask->OnChargeTick.AddDynamic(this, &UGA_Musket_HeavyAttack::OnChargeTick);
  78:     ChargeTask->OnChargeFull.AddDynamic(this, &UGA_Musket_HeavyAttack::OnChargeFullNotify);
  79:     ChargeTask->ReadyForActivation();
  80: 
  81:     // 绛夊緟閲嶆敾鍑绘澗閿簨浠讹紙Controller Completed 缁戝畾閫氳繃 HandleGameplayEvent 鍙戦€侊級
  82:     UAbilityTask_WaitGameplayEvent* ReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
  83:         this,
  84:         FGameplayTag::RequestGameplayTag("GameplayEvent.Musket.HeavyRelease"),
  85:         nullptr, false, true);
  86:     ReleaseTask->EventReceived.AddDynamic(this, &UGA_Musket_HeavyAttack::OnHeavyReleaseEvent);
  87:     ReleaseTask->ReadyForActivation();
  88: }
  89: 
  90: void UGA_Musket_HeavyAttack::OnChargeTick(float HalfAngleDeg, float RadiusCm, bool bFull)
  91: {
  92:     CurrentHalfAngle = HalfAngleDeg;



exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/Character/YogPlayerControllerBase.cpp' | ForEach-Object { "'$i++; if($i -ge 380 -and $i -le 460){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 666ms:
 380: 			SetInputMode(InputMode);
 381: 		}
 382: 	}
 383: 	else
 384: 	{
 385: 		FInputModeGameOnly InputMode;
 386: 		SetInputMode(InputMode);
 387: 		SetShowMouseCursor(false);
 388: 	}
 389: }
 390: 
 391: void AYogPlayerControllerBase::LightAtack(const FInputActionValue& Value)
 392: {
 393: 	if (bBlockGameInput) return;
 394: 	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
 395: 	{
 396: 		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
 397: 		{
 398: 			Buffer->RecordLightAttack();
 399: 		}
 400: 
 401: 		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasComboSource())
 402: 		{
 403: 			player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Light, player);
 404: 			return;
 405: 		}
 406: 
 407: 		FGameplayTagContainer TagContainer;
 408: 		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.LightAtk")));
 409: 		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
 410: 	}
 411: 	//UE_LOG(LogTemp, Log, TEXT("LightAtack"));
 412: }
 413: void AYogPlayerControllerBase::HeavyAtack(const FInputActionValue& Value)
 414: {
 415: 	if (bBlockGameInput) return;
 416: 	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
 417: 	{
 418: 		if (UBufferComponent* Buffer = player->GetInputBufferComponent())
 419: 		{
 420: 			Buffer->RecordHeavyAttack();
 421: 		}
 422: 
 423: 		// 缁堢粨鎶€鎵ц鏈熼棿锛氬皢閲嶆敾鍑昏矾鐢卞埌纭浜嬩欢锛屼笉椹卞姩 ComboRuntime
 424: 		static const FGameplayTag TAG_FinisherExecuting =
 425: 			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherExecuting"));
 426: 		if (UAbilitySystemComponent* ASC = player->GetASC())
 427: 		{
 428: 			if (ASC->HasMatchingGameplayTag(TAG_FinisherExecuting))
 429: 			{
 430: 				FGameplayEventData EventData;
 431: 				EventData.Instigator = player;
 432: 				ASC->HandleGameplayEvent(
 433: 					FGameplayTag::RequestGameplayTag(TEXT("Action.Finisher.Confirm")),
 434: 					&EventData);
 435: 				return;
 436: 			}
 437: 		}
 438: 
 439: 		if (player->ComboRuntimeComponent && player->ComboRuntimeComponent->HasComboSource())
 440: 		{
 441: 			player->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Heavy, player);
 442: 			return;
 443: 		}
 444: 
 445: 		FGameplayTagContainer TagContainer;
 446: 		TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("PlayerState.AbilityCast.HeavyAtk")));
 447: 		player->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
 448: 	}
 449: 	//UE_LOG(LogTemp, Log, TEXT("HeavyAtack"));
 450: }
 451: 
 452: 
 453: void AYogPlayerControllerBase::HeavyAttackReleased(const FInputActionValue& Value)
 454: {
 455: 	if (bBlockGameInput) return;
 456: 	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
 457: 	{
 458: 		UAbilitySystemComponent* ASC = player->GetASC();
 459: 		if (!ASC) return;
 460: 		FGameplayEventData EventData;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit -Include *Buffer*.h,*Buffer*.cpp | Select-String -Pattern 'RecordHeavyAttack|Consume|Buffer' | Select-Object -First 120 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 650ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:4:#include "Character/InputBufferComponent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:8:UInputBufferComponent::UInputBufferComponent()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:19:void UInputBufferComponent::BeginPlay()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:29:void UInputBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:36:void UInputBufferComponent::UpdateActionBuffer(EPlayerActionInput actionInput)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:38://UpdateBuffer(ActionBuffer, actionInput, MAX_BUFFER_SIZE);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:41:if (ActionBuffer.Num() < MAX_BUFFER_SIZE)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:43:ActionBuffer.Insert(actionInput, 0);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:44:/*FVector2D cache_item = MovementBuffer[0];*/
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:48:ActionBuffer.Pop();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:49:ActionBuffer.Insert(actionInput, 0);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:53:void UInputBufferComponent::UpdateMoveBuffer(FVector2D move)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:55://UpdateBuffer(this->MovementBuffer, move, this->MAX_BUFFER_SIZE);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:58:if (MovementBuffer.Num() <= MAX_BUFFER_SIZE)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:60:MovementBuffer.Insert(move, 0);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:61:/*FVector2D cache_item = MovementBuffer[0];*/
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:65:MovementBuffer.Pop();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:66:MovementBuffer.Insert(move, 0);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:69://UE_LOG(LogTemp, Warning, TEXT("this->MovementBuffer: %d"), this->MovementBuffer.Num());
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:74://FVector2D UInputBufferComponent::GetLastFrameInput(FVector2D Movement)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:76://	return buffer.Pop();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:79:void UInputBufferComponent::ClearActionBuffer()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:81:ClearBuffer(ActionBuffer);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:84:void UInputBufferComponent::ClearMovementBuffer()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:86:ClearBuffer(MovementBuffer);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:89:void UInputBufferComponent::DebugPrintAction()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:93:for (const EPlayerActionInput& Element : ActionBuffer)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:103:void UInputBufferComponent::DebugPrintMovement()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:108:for (const FVector2D& Element : MovementBuffer)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:119:FVector2D UInputBufferComponent::GetLastMoveInput(FVector2D Movement)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:121:return MovementBuffer.Pop();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:122://return GetLastItem(MovementBuffer);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:125:EPlayerActionInput UInputBufferComponent::GetLastActionInput(FVector2D Movement)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:127:if (ActionBuffer.Num() > 0)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Character\InputBufferComponent.cpp:129:return ActionBuffer.Pop();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:4:#include "Component/BufferComponent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:8:UBufferComponent::UBufferComponent()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:17:void UBufferComponent::RecordLightAttack()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:22:void UBufferComponent::RecordHeavyAttack()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:27:void UBufferComponent::RecordDash()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:32:void UBufferComponent::RecordMove(const FVector2D& Direction)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:37:bool UBufferComponent::HasBufferedInput(EInputCommandType Type, float TimeWindow) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:51:bool UBufferComponent::HasBufferedInputSince(EInputCommandType Type, float SinceTime) const
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:64:bool UBufferComponent::ConsumeBufferedInput(EInputCommandType Type)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:79:bool UBufferComponent::ConsumeBufferedInputSince(EInputCommandType Type, float SinceTime)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:93:void UBufferComponent::ClearBuffer()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:100:void UBufferComponent::BeginPlay()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:108:UYogGameplayEffect* UBufferComponent::GetItemAt(int index)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:110:return BufferArray[index];
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:113:void UBufferComponent::MoveToNextItem()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:116:if (CurrentIndex == BufferArray.Num())
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:126:void UBufferComponent::PushCommand(const FInputCommand& Command)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:139:void UBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\BufferComponent.cpp:146:FString UBufferComponent::CommandToString(const FInputCommand& Command)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:4:#include "Component/HitBoxBufferComponent.h"
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:7:UHitBoxBufferComponent::UHitBoxBufferComponent()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:18:void UHitBoxBufferComponent::BeginPlay()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:28:void UHitBoxBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:35://void UHitBoxBufferComponent::Clear()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:37://	array_HitboxBuffer.Empty();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:40://void UHitBoxBufferComponent::Initialize(TArray<FHitBoxData> array_GA)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:43://	array_HitboxBuffer.Append(array_GA);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:46://void UHitBoxBufferComponent::UpdateTrigger(int index, bool trigger)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:48://	//if (index < array_HitboxBuffer.Num())
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\HitBoxBufferComponent.cpp:50://	//	this->array_HitboxBuffer[index].HasTriggered = trigger;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:8:#include "InputBufferComponent.generated.h"
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:24:class DEVKIT_API UInputBufferComponent : public UActorComponent
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:30:UInputBufferComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:40:UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:41:void UpdateMoveBuffer(FVector2D move);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:43:UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:44:void UpdateActionBuffer(EPlayerActionInput actionInput);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:46:UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:49:UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:56:TArray<EPlayerActionInput> ActionBuffer;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:58:UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:59:TArray<FVector2D> MovementBuffer;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:61:UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:65:UPROPERTY(BlueprintReadOnly, Category = "Input buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:66:int32 MAX_BUFFER_SIZE = 10;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:69:void ClearActionBuffer();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:73:void ClearMovementBuffer();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:82:void ClearBuffer(TArray<T> TargetArray)
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:102:void UpdateBuffer(TArray<T> TargetArray, T element, int32 MAX_BUFFER_SIZE)
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:104:if (TargetArray.Num() <= MAX_BUFFER_SIZE)
D:\Self\GItGame\Dev01\Source\DevKit\Public\Character\InputBufferComponent.h:107:/*FVector2D cache_item = MovementBuffer[0];*/
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:7:#include "BufferComponent.generated.h"
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:72:class DEVKIT_API UBufferComponent : public UActorComponent
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:78:UBufferComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:81:void RecordHeavyAttack();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:90:UFUNCTION(BlueprintCallable, Category = "InputBuffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:91:bool HasBufferedInput(EInputCommandType Type, float TimeWindow = 0.3f) const;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:100:UFUNCTION(BlueprintCallable, Category = "InputBuffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:101:bool HasBufferedInputSince(EInputCommandType Type, float SinceTime) const;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:108:UFUNCTION(BlueprintCallable, Category = "InputBuffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:109:bool ConsumeBufferedInput(EInputCommandType Type);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:111:UFUNCTION(BlueprintCallable, Category = "InputBuffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:112:bool ConsumeBufferedInputSince(EInputCommandType Type, float SinceTime);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:115:UFUNCTION(BlueprintCallable, Category = "InputBuffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:116:void ClearBuffer();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\BufferComponent.h:140:TArray<UYogGameplayEffect*> BufferArray;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:9:#include "HitBoxBufferComponent.generated.h"
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:14:class DEVKIT_API UHitBoxBufferComponent : public UActorComponent
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:20:UHitBoxBufferComponent();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:31://TArray<FHitBoxData> array_HitboxBuffer;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:33://UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:36://UFUNCTION(BlueprintCallable, Category = "Buffer")
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\HitBoxBufferComponent.h:39://UFUNCTION(BlueprintCallable, Category = "Buffer")

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Select-String -Path 'Source/DevKit/Private/Component/ComboRuntimeComponent.cpp','Source/DevKit/Public/Component/ComboRuntimeComponent.h' -Pattern 'TrackRuntimeCombatLooseTag|RuntimeCombatLooseTags|RemoveLoose|ChainActive|TryActivateCombo|Reset' -Context 0,2 | Select-Object -First 200 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 844ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:105:ResetCombo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:124:ResetCombo();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:210:bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:212:return TryActivateCombo(ToGraphInputAction(InputAction), PlayerOwner);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:215:bool UComboRuntimeComponent::TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:311:ComboTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:312:ClearRuntimeCombatLooseTags();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:353:ComboTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:427:TrackRuntimeCombatLooseTag(TemporaryTag);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:451:ASC->RemoveLooseGameplayTag(TemporaryTag);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:497:ComboTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:521:static const FGameplayTag ChainActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Combo.ChainActive"), false);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:522:if (ChainActiveTag.IsValid() && ASC->GetTagCount(ChainActiveTag) <= 0)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:524:ASC->AddLooseGameplayTag(ChainActiveTag);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:525:TrackRuntimeCombatLooseTag(ChainActiveTag);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:531:void UComboRuntimeComponent::ResetCombo()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:540:ComboTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:550:ClearRuntimeCombatLooseTags();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:624:ClearRuntimeCombatLooseTags();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:631:void UComboRuntimeComponent::ClearRuntimeCombatLooseTags()
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:637:RuntimeCombatLooseTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:641:for (const FGameplayTag& Tag : RuntimeCombatLooseTags)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:653:RuntimeCombatLooseTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:656:void UComboRuntimeComponent::TrackRuntimeCombatLooseTag(const FGameplayTag& Tag)
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:660:RuntimeCombatLooseTags.AddTag(Tag);
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:699:ComboTags.Reset();
D:\Self\GItGame\Dev01\Source\DevKit\Private\Component\ComboRuntimeComponent.cpp:707:ClearRuntimeCombatLooseTags();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:31:bool TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:33:bool TryActivateCombo(ECombatGraphInputAction InputAction, APlayerCharacterBase* PlayerOwner);
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:42:void ResetCombo();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:54:void ClearRuntimeCombatLooseTags();
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:112:FGameplayTagContainer RuntimeCombatLooseTags;
D:\Self\GItGame\Dev01\Source\DevKit\Public\Component\ComboRuntimeComponent.h:114:void TrackRuntimeCombatLooseTag(const FGameplayTag& Tag);

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKit/Private/Component/ComboRuntimeComponent.cpp' | ForEach-Object { "'$i++; if(($i -ge 280 -and $i -le 330) -or ($i -ge 620 -and $i -le 665)){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 731ms:
 280: 			{
 281: 				NextNode = ComboConfig->FindRootNode(CardInputAction);
 282: 			}
 283: 		}
 284: 	}
 285: 
 286: 	if (bBlockedRootFallbackDuringActiveNode)
 287: 	{
 288: 		UE_LOG(LogTemp, Verbose,
 289: 			TEXT("[ComboRuntime] Ignore input without child while active node is playing input=%s current=%s"),
 290: 			*GetGraphInputName(InputAction),
 291: 			*StartNodeId.ToString());
 292: 		return false;
 293: 	}
 294: 
 295: 	if (!NextNode || !NextNode->AbilityTag.IsValid())
 296: 	{
 297: 		UE_LOG(LogTemp, Warning,
 298: 			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s config=%s"),
 299: 			*GetGraphInputName(InputAction),
 300: 			*CurrentNodeId.ToString(),
 301: 			*GetNameSafe(ComboGraph),
 302: 			*GetNameSafe(ComboConfig));
 303: 		if (!CurrentNodeId.IsNone() && PlayerOwner->CombatDeckComponent)
 304: 		{
 305: 			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
 306: 		}
 307: 		bComboContinued = false;
 308: 		bExitedComboState = !CurrentNodeId.IsNone();
 309: 		CurrentNodeId = NAME_None;
 310: 		ComboIndex = 0;
 311: 		ComboTags.Reset();
 312: 		ClearRuntimeCombatLooseTags();
 313: 		return false;
 314: 	}
 315: 
 316: 	if (bFoundChildNode && !bUseDashSavedNode)
 317: 	{
 318: 		if (CanComboTag.IsValid() && ASC->GetTagCount(CanComboTag) <= 0)
 319: 		{
 320: 			UE_LOG(LogTemp, Verbose,
 321: 				TEXT("[ComboRuntime] Queue child activation until CanCombo opens input=%s current=%s next=%s"),
 322: 				*GetGraphInputName(InputAction),
 323: 				*CurrentNodeId.ToString(),
 324: 				*NextNode->NodeId.ToString());
 325: 			return false;
 326: 		}
 327: 	}
 328: 
 329: 	const FWeaponComboNodeConfig PreviousActiveNode = ActiveNode;
 330: 	const FGuid PreviousAttackGuid = ActiveAttackGuid;
 620: 	}
 621: 
 622: 	if (bClearCombatTags)
 623: 	{
 624: 		ClearRuntimeCombatLooseTags();
 625: 	}
 626: 
 627: 	ActiveDashNode = FWeaponComboNodeConfig();
 628: 	bActiveDashNodeValid = false;
 629: }
 630: 
 631: void UComboRuntimeComponent::ClearRuntimeCombatLooseTags()
 632: {
 633: 	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner());
 634: 	UAbilitySystemComponent* ASC = PlayerOwner ? PlayerOwner->GetAbilitySystemComponent() : nullptr;
 635: 	if (!ASC)
 636: 	{
 637: 		RuntimeCombatLooseTags.Reset();
 638: 		return;
 639: 	}
 640: 
 641: 	for (const FGameplayTag& Tag : RuntimeCombatLooseTags)
 642: 	{
 643: 		if (Tag.IsValid())
 644: 		{
 645: 			ASC->SetLooseGameplayTagCount(Tag, 0);
 646: 		}
 647: 	}
 648: 	ClearComboWindowAndProgressLooseTags(ASC);
 649: 	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC))
 650: 	{
 651: 		YogASC->ConsumeDashSave();
 652: 	}
 653: 	RuntimeCombatLooseTags.Reset();
 654: }
 655: 
 656: void UComboRuntimeComponent::TrackRuntimeCombatLooseTag(const FGameplayTag& Tag)
 657: {
 658: 	if (Tag.IsValid())
 659: 	{
 660: 		RuntimeCombatLooseTags.AddTag(Tag);
 661: 	}
 662: }
 663: 
 664: void UComboRuntimeComponent::ExpireSavedDashNode()
 665: {

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKitEditor/Private/RuneEditor -Include *.cpp,*.h | Select-String -Pattern 'DisplayedFlowAsset|GetSelectedFlowAsset|RebuildGraphEditor\\(' | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 897ms:
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:359:RebuildGraphEditor();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:428:return GetSelectedFlowAsset() != nullptr;
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:1271:return NodeClass && GetSelectedFlowAsset() != nullptr;
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:2924:UFlowAsset* FlowAsset = GetSelectedFlowAsset();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:2935:RebuildGraphEditor();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3028:void SRuneEditorWidget::RebuildGraphEditor(UFlowAsset* OverrideFlow)
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3035:UFlowAsset* FlowAsset = OverrideFlow ? OverrideFlow : GetSelectedFlowAsset();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3036:DisplayedFlowAsset = FlowAsset;
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3088:UFlowAsset* FlowAsset = GetSelectedFlowAsset();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3184:RebuildGraphEditor();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3254:UFlowNode* NewNode = FRuneEditorFlowAuthoring::AddNodeAfter(GetSelectedFlowAsset(), GetSelectedFlowNode(), NodeClass, Message);
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3492:OpenAsset(GetSelectedFlowAsset());
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3506:RebuildGraphEditor(FA);
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3567:const UFlowAsset* FlowAsset = GetSelectedFlowAsset();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3577:const UFlowAsset* FlowAsset = GetSelectedFlowAsset();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3675:UFlowAsset* SRuneEditorWidget::GetSelectedFlowAsset() const
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.cpp:3805:RebuildGraphEditor();
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.h:160:void RebuildGraphEditor(UFlowAsset* OverrideFlow = nullptr);
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.h:218:UFlowAsset* GetSelectedFlowAsset() const;
D:\Self\GItGame\Dev01\Source\DevKitEditor\Private\RuneEditor\SRuneEditorWidget.h:261:TWeakObjectPtr<UFlowAsset> DisplayedFlowAsset;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp' | ForEach-Object { "'$i++; if(($i -ge 1240 -and $i -le 1290) -or ($i -ge 2900 -and $i -le 3105) -or ($i -ge 3230 -and $i -le 3270) -or ($i -ge 3660 -and $i -le 3695)){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 648ms:
1240: 				]
1241: 				+ SVerticalBox::Slot()
1242: 				.AutoHeight()
1243: 				.Padding(0.f, 2.f, 0.f, 0.f)
1244: 				[
1245: 					SNew(STextBlock)
1246: 					.Text(ClassName)
1247: 					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
1248: 				]
1249: 				+ SVerticalBox::Slot()
1250: 				.AutoHeight()
1251: 				.Padding(0.f, 2.f, 0.f, 0.f)
1252: 				[
1253: 					SNew(STextBlock)
1254: 					.Text(Description)
1255: 					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
1256: 					.AutoWrapText(true)
1257: 				]
1258: 				+ SVerticalBox::Slot()
1259: 				.FillHeight(1.f)
1260: 				[
1261: 					SNew(SSpacer)
1262: 				]
1263: 				+ SVerticalBox::Slot()
1264: 				.AutoHeight()
1265: 				.HAlign(HAlign_Right)
1266: 				[
1267: 					SNew(SButton)
1268: 					.Text(LOCTEXT("AddNodeFromLibrary", "娣诲姞"))
1269: 					.IsEnabled_Lambda([this, NodeClass]()
1270: 					{
1271: 						return NodeClass && GetSelectedFlowAsset() != nullptr;
1272: 					})
1273: 					.OnClicked(this, &SRuneEditorWidget::OnAddNodeFromLibrary, NodeClass)
1274: 				]
1275: 			]
1276: 		];
1277: }
1278: 
1279: TSharedRef<SWidget> SRuneEditorWidget::BuildDetailsPanel()
1280: {
1281: 	const FText InitialName = GetSelectedRuneNameText();
1282: 	const FText InitialTag = GetSelectedRuneTagText();
1283: 	const FText InitialSummary = GetSelectedSummaryText();
1284: 
1285: 	FDetailsViewArgs NodeDetailsArgs;
1286: 	NodeDetailsArgs.bHideSelectionTip = true;
1287: 	NodeDetailsArgs.bShowOptions = false;
1288: 	NodeDetailsArgs.bShowScrollBar = true;
1289: 	NodeDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
1290: 	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
2900: 		RuneListView->RequestListRefresh();
2901: 		for (const FRuneRowPtr& Row : RuneRows)
2902: 		{
2903: 			if (Row.IsValid() && Row->Asset == SelectedRune)
2904: 			{
2905: 				RuneListView->SetSelection(Row);
2906: 				break;
2907: 			}
2908: 		}
2909: 	}
2910: 
2911: 	StatusText = NewStatus.IsEmpty()
2912: 		? FText::Format(LOCTEXT("RefreshStatus", "绗︽枃璧勬簮锛歿0}銆傝妭鐐硅彍鍗曞凡闄愬埗涓?Yog 涓撶敤鑺傜偣銆?),
2913: 			FText::AsNumber(RuneRows.Num()))
2914: 		: NewStatus;
2915: 
2916: 	RefreshFlowNodes();
2917: 	RefreshTuningRows();
2918: }
2919: 
2920: void SRuneEditorWidget::RefreshFlowNodes()
2921: {
2922: 	FlowNodeRows.Reset();
2923: 
2924: 	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
2925: 	for (const FRuneEditorFlowNodeSummary& Summary : FRuneEditorFlowAuthoring::BuildFlowNodeSummaries(FlowAsset))
2926: 	{
2927: 		FlowNodeRows.Add(MakeShared<FRuneEditorFlowNodeRow>(Summary));
2928: 	}
2929: 
2930: 	if (!SelectedFlowNode.IsValid() && FlowNodeRows.Num() > 0)
2931: 	{
2932: 		SelectedFlowNode = FlowNodeRows[0]->Summary.Node;
2933: 	}
2934: 
2935: 	RebuildGraphEditor();
2936: 	SyncNodeInspector();
2937: }
2938: 
2939: void SRuneEditorWidget::RefreshTuningRows()
2940: {
2941: 	TuningRows.Reset();
2942: 
2943: 	static const FName ComboOnlyFilter(TEXT("##ComboBonus"));
2944: 
2945: 	if (const URuneDataAsset* Rune = GetSelectedRune())
2946: 	{
2947: 		const TArray<FRuneTuningScalar>& Scalars = Rune->GetTuningScalars();
2948: 		for (int32 Index = 0; Index < Scalars.Num(); ++Index)
2949: 		{
2950: 			const FRuneTuningScalar& Scalar = Scalars[Index];
2951: 			if (!TuningCategoryFilter.IsNone())
2952: 			{
2953: 				if (TuningCategoryFilter == ComboOnlyFilter)
2954: 				{
2955: 					if (!Scalar.ComboBonus.IsEnabled()) continue;
2956: 				}
2957: 				else if (Scalar.Category != TuningCategoryFilter)
2958: 				{
2959: 					continue;
2960: 				}
2961: 			}
2962: 			TuningRows.Add(MakeShared<FRuneEditorTuningRow>(Index));
2963: 		}
2964: 	}
2965: 
2966: 	if (TuningListView.IsValid())
2967: 	{
2968: 		TuningListView->RequestListRefresh();
2969: 	}
2970: }
2971: 
2972: TSharedRef<SWidget> SRuneEditorWidget::BuildTuningCategoryFilterBar()
2973: {
2974: 	struct FFilterEntry { FText Label; FName Category; };
2975: 	static const FFilterEntry Entries[] = {
2976: 		{ LOCTEXT("TuningFilterAll",      "鍏ㄩ儴"),     NAME_None              },
2977: 		{ LOCTEXT("TuningFilterDamage",   "浼ゅ"),     FName("Damage")        },
2978: 		{ LOCTEXT("TuningFilterProjectile","椋炶鐗?),  FName("Projectile")    },
2979: 		{ LOCTEXT("TuningFilterStack",    "灞傛暟"),     FName("Stack")         },
2980: 		{ LOCTEXT("TuningFilterDuration", "鎸佺画鏃堕棿"), FName("Duration")      },
2981: 		{ LOCTEXT("TuningFilterCombo",    "杩炴嫑濂栧姳"), FName("##ComboBonus")  },
2982: 	};
2983: 
2984: 	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
2985: 	for (const FFilterEntry& E : Entries)
2986: 	{
2987: 		const FName Cat = E.Category;
2988: 		const FText Lbl = E.Label;
2989: 		Bar->AddSlot()
2990: 			.AutoWidth()
2991: 			.Padding(0.f, 0.f, 4.f, 0.f)
2992: 			[
2993: 				SNew(SButton)
2994: 				.Text_Lambda([this, Cat, Lbl]()
2995: 				{
2996: 					return TuningCategoryFilter == Cat
2997: 						? FText::Format(LOCTEXT("ActiveTuningFilter", "鈼?{0}"), Lbl)
2998: 						: Lbl;
2999: 				})
3000: 				.OnClicked(this, &SRuneEditorWidget::OnTuningCategoryFilterClicked, Cat)
3001: 			];
3002: 	}
3003: 	return Bar;
3004: }
3005: 
3006: FReply SRuneEditorWidget::OnTuningCategoryFilterClicked(FName Category)
3007: {
3008: 	TuningCategoryFilter = Category;
3009: 	RefreshTuningRows();
3010: 	return FReply::Handled();
3011: }
3012: 
3013: void SRuneEditorWidget::BindGraphEditorCommands()
3014: {
3015: 	if (GraphEditorCommands.IsValid())
3016: 	{
3017: 		return;
3018: 	}
3019: 
3020: 	FGenericCommands::Register();
3021: 	GraphEditorCommands = MakeShared<FUICommandList>();
3022: 	GraphEditorCommands->MapAction(
3023: 		FGenericCommands::Get().Delete,
3024: 		FExecuteAction::CreateSP(this, &SRuneEditorWidget::DeleteSelectedGraphNodes),
3025: 		FCanExecuteAction::CreateSP(this, &SRuneEditorWidget::CanDeleteSelectedGraphNodes));
3026: }
3027: 
3028: void SRuneEditorWidget::RebuildGraphEditor(UFlowAsset* OverrideFlow)
3029: {
3030: 	if (!GraphEditorContainer.IsValid())
3031: 	{
3032: 		return;
3033: 	}
3034: 
3035: 	UFlowAsset* FlowAsset = OverrideFlow ? OverrideFlow : GetSelectedFlowAsset();
3036: 	DisplayedFlowAsset = FlowAsset;
3037: 	UEdGraph* GraphToEdit = FlowAsset ? FlowAsset->GetGraph() : nullptr;
3038: 	if (!GraphToEdit)
3039: 	{
3040: 		RuneGraphEditor.Reset();
3041: 		GraphEditorContainer->SetContent(
3042: 			SNew(SBorder)
3043: 			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
3044: 			.Padding(12.f)
3045: 			[
3046: 				SNew(STextBlock)
3047: 				.Text(LOCTEXT("NoGraphToEdit", "娌℃湁鎵惧埌娴佺▼鍥俱€傝鍏堟柊寤烘垨閫夋嫨涓€涓?Yog 绗︽枃璧勬簮銆?))
3048: 				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
3049: 				.AutoWrapText(true)
3050: 			]);
3051: 		return;
3052: 	}
3053: 
3054: 	FGraphAppearanceInfo AppearanceInfo;
3055: 	AppearanceInfo.CornerText = LOCTEXT("RuneGraphCorner", "YOG 绗︽枃娴佺▼");
3056: 
3057: 	SGraphEditor::FGraphEditorEvents GraphEvents;
3058: 	GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &SRuneEditorWidget::OnGraphSelectionChanged);
3059: 
3060: 	BindGraphEditorCommands();
3061: 
3062: 	SAssignNew(RuneGraphEditor, SGraphEditor)
3063: 		.AdditionalCommands(GraphEditorCommands)
3064: 		.Appearance(AppearanceInfo)
3065: 		.GraphToEdit(GraphToEdit)
3066: 		.GraphEvents(GraphEvents)
3067: 		.AutoExpandActionMenu(true);
3068: 
3069: 	GraphEditorContainer->SetContent(RuneGraphEditor.ToSharedRef());
3070: 	RuneGraphEditor->ZoomToFit(false);
3071: 
3072: 	if (UFlowNode* FlowNode = GetSelectedFlowNode())
3073: 	{
3074: 		if (UEdGraphNode* GraphNode = FlowNode->GetGraphNode())
3075: 		{
3076: 			RuneGraphEditor->SetNodeSelection(GraphNode, true);
3077: 		}
3078: 	}
3079: }
3080: 
3081: void SRuneEditorWidget::DeleteSelectedGraphNodes()
3082: {
3083: 	if (!RuneGraphEditor.IsValid() || GEditor->PlayWorld != nullptr)
3084: 	{
3085: 		return;
3086: 	}
3087: 
3088: 	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
3089: 	UEdGraph* Graph = FlowAsset ? FlowAsset->GetGraph() : RuneGraphEditor->GetCurrentGraph();
3090: 	if (!Graph)
3091: 	{
3092: 		return;
3093: 	}
3094: 
3095: 	const FGraphPanelSelectionSet SelectedNodes = RuneGraphEditor->GetSelectedNodes();
3096: 	if (SelectedNodes.Num() == 0)
3097: 	{
3098: 		return;
3099: 	}
3100: 
3101: 	const FScopedTransaction Transaction(LOCTEXT("DeleteRuneFlowNodes", "鍒犻櫎绗︽枃娴佺▼鑺傜偣"));
3102: 	Graph->Modify();
3103: 	if (FlowAsset)
3104: 	{
3105: 		FlowAsset->Modify();
3230: 	if (LibraryTags.HasTagExact(CategoryTag))
3231: 	{
3232: 		LibraryTags.RemoveTag(CategoryTag);
3233: 	}
3234: 	else
3235: 	{
3236: 		LibraryTags.AddTag(CategoryTag);
3237: 	}
3238: 
3239: 	Rune->MarkPackageDirty();
3240: 	RefreshData(LOCTEXT("LibraryCategoryUpdated", "宸叉洿鏂扮鏂囪祫婧愬垎绫汇€?));
3241: 	return FReply::Handled();
3242: }
3243: 
3244: FReply SRuneEditorWidget::OnNodeLibraryFilterSelected(ENodeLibraryFilter Filter)
3245: {
3246: 	ActiveNodeLibraryFilter = Filter;
3247: 	StatusText = LOCTEXT("NodeLibraryFilterStatus", "宸叉洿鏂拌妭鐐瑰簱绛涢€夈€?);
3248: 	return FReply::Handled();
3249: }
3250: 
3251: FReply SRuneEditorWidget::OnAddNodeFromLibrary(UClass* NodeClass)
3252: {
3253: 	FText Message;
3254: 	UFlowNode* NewNode = FRuneEditorFlowAuthoring::AddNodeAfter(GetSelectedFlowAsset(), GetSelectedFlowNode(), NodeClass, Message);
3255: 	StatusText = Message;
3256: 
3257: 	if (NewNode)
3258: 	{
3259: 		SelectedFlowNode = NewNode;
3260: 		RefreshFlowNodes();
3261: 		SyncNodeInspector();
3262: 		if (RuneGraphEditor.IsValid())
3263: 		{
3264: 			if (UEdGraphNode* GraphNode = NewNode->GetGraphNode())
3265: 			{
3266: 				RuneGraphEditor->ClearSelectionSet();
3267: 				RuneGraphEditor->SetNodeSelection(GraphNode, true);
3268: 				RuneGraphEditor->JumpToNode(GraphNode, false);
3269: 			}
3270: 		}
3660: 
3661: UObject* SRuneEditorWidget::GetSelectedResource() const
3662: {
3663: 	if (SelectedResource.IsValid())
3664: 	{
3665: 		return SelectedResource.Get();
3666: 	}
3667: 	return SelectedRune.Get();
3668: }
3669: 
3670: URuneDataAsset* SRuneEditorWidget::GetSelectedRune() const
3671: {
3672: 	return SelectedRune.Get();
3673: }
3674: 
3675: UFlowAsset* SRuneEditorWidget::GetSelectedFlowAsset() const
3676: {
3677: 	if (const URuneDataAsset* Rune = GetSelectedRune())
3678: 	{
3679: 		return Rune->GetFlowAsset();
3680: 	}
3681: 	return Cast<UFlowAsset>(GetSelectedResource());
3682: }
3683: 
3684: UFlowNode* SRuneEditorWidget::GetSelectedFlowNode() const
3685: {
3686: 	return SelectedFlowNode.Get();
3687: }
3688: 
3689: URuneDataAsset* SRuneEditorWidget::FindRuneForFlow(UFlowAsset* FlowAsset) const
3690: {
3691: 	if (!FlowAsset)
3692: 	{
3693: 		return nullptr;
3694: 	}
3695: 

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command '$i=0; Get-Content '"'Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp' | ForEach-Object { "'$i++; if($i -ge 3080 -and $i -le 3195){"{0,4}: {1}" -f $i,$_} }' in D:\Self\GItGame\Dev01
 succeeded in 599ms:
3080: 
3081: void SRuneEditorWidget::DeleteSelectedGraphNodes()
3082: {
3083: 	if (!RuneGraphEditor.IsValid() || GEditor->PlayWorld != nullptr)
3084: 	{
3085: 		return;
3086: 	}
3087: 
3088: 	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
3089: 	UEdGraph* Graph = FlowAsset ? FlowAsset->GetGraph() : RuneGraphEditor->GetCurrentGraph();
3090: 	if (!Graph)
3091: 	{
3092: 		return;
3093: 	}
3094: 
3095: 	const FGraphPanelSelectionSet SelectedNodes = RuneGraphEditor->GetSelectedNodes();
3096: 	if (SelectedNodes.Num() == 0)
3097: 	{
3098: 		return;
3099: 	}
3100: 
3101: 	const FScopedTransaction Transaction(LOCTEXT("DeleteRuneFlowNodes", "鍒犻櫎绗︽枃娴佺▼鑺傜偣"));
3102: 	Graph->Modify();
3103: 	if (FlowAsset)
3104: 	{
3105: 		FlowAsset->Modify();
3106: 	}
3107: 
3108: 	bool bDeletedAnyNode = false;
3109: 	SelectedFlowNode.Reset();
3110: 	RuneGraphEditor->ClearSelectionSet();
3111: 
3112: 	for (UObject* SelectedObject : SelectedNodes)
3113: 	{
3114: 		UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
3115: 		if (!Node || !Node->CanUserDeleteNode())
3116: 		{
3117: 			continue;
3118: 		}
3119: 
3120: 		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node);
3121: 		UFlowNode* FlowNode = FlowGraphNode ? Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()) : nullptr;
3122: 
3123: 		if (const UEdGraphSchema* Schema = Graph->GetSchema())
3124: 		{
3125: 			Schema->BreakNodeLinks(*Node);
3126: 		}
3127: 		Node->DestroyNode();
3128: 
3129: 		if (FlowAsset && FlowNode)
3130: 		{
3131: 			FlowAsset->UnregisterNode(FlowNode->GetGuid());
3132: 		}
3133: 		bDeletedAnyNode = true;
3134: 	}
3135: 
3136: 	if (!bDeletedAnyNode)
3137: 	{
3138: 		return;
3139: 	}
3140: 
3141: 	if (FlowAsset)
3142: 	{
3143: 		FlowAsset->PostEditChange();
3144: 	}
3145: 	Graph->NotifyGraphChanged();
3146: 	RefreshFlowNodes();
3147: 	StatusText = LOCTEXT("DeleteGraphNodesStatus", "宸插垹闄ら€変腑鐨勭鏂囨祦绋嬭妭鐐广€?);
3148: }
3149: 
3150: bool SRuneEditorWidget::CanDeleteSelectedGraphNodes() const
3151: {
3152: 	if (!RuneGraphEditor.IsValid() || GEditor->PlayWorld != nullptr)
3153: 	{
3154: 		return false;
3155: 	}
3156: 
3157: 	const FGraphPanelSelectionSet SelectedNodes = RuneGraphEditor->GetSelectedNodes();
3158: 	for (UObject* SelectedObject : SelectedNodes)
3159: 	{
3160: 		const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
3161: 		if (Node && Node->CanUserDeleteNode())
3162: 		{
3163: 			return true;
3164: 		}
3165: 	}
3166: 
3167: 	return false;
3168: }
3169: 
3170: void SRuneEditorWidget::OnSearchTextChanged(const FText& NewText)
3171: {
3172: 	SearchText = NewText.ToString();
3173: 	RefreshData(LOCTEXT("SearchStatus", "Rune list filtered."));
3174: }
3175: 
3176: void SRuneEditorWidget::OnRuneSelectionChanged(FRuneRowPtr Row, ESelectInfo::Type SelectInfo)
3177: {
3178: 	SelectedRune = Row.IsValid() ? Row->Asset.Get() : nullptr;
3179: 	SelectedResource = SelectedRune.Get();
3180: 	SelectedFlowNode.Reset();
3181: 	RunFeedbackText = LOCTEXT("RunFeedbackSelectionChanged", "褰撳墠閫夋嫨灏氭湭杩愯绗︽枃銆?);
3182: 	RefreshFlowNodes();
3183: 	RefreshTuningRows();
3184: 	RebuildGraphEditor();
3185: 	SyncSelectedRuneEditorFields();
3186: 	StatusText = SelectedRune.IsValid()
3187: 		? FText::Format(LOCTEXT("SelectionStatus", "宸查€夋嫨 {0}銆?), GetSelectedRuneNameText())
3188: 		: LOCTEXT("NoSelectionStatus", "鏈€夋嫨绗︽枃銆?);
3189: }
3190: 
3191: FReply SRuneEditorWidget::OnBottomTabSelected(EBottomPanelTab Tab)
3192: {
3193: 	ActiveBottomTab = Tab;
3194: 	if (BottomPanelSwitcher.IsValid())
3195: 	{

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Plugins/FlowGraph-2.1-5.4/Source/Flow/Public/Nodes/FlowPin.h' | Select-String -Pattern 'struct FFlowPin|FFlowPin\\(' -Context 0,40" in D:\Self\GItGame\Dev01
 succeeded in 730ms:

> // Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Types/FlowPinEnums.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"

#include "FlowPin.generated.h"

class UEnum;
class UClass;
class UObject;
class IPropertyHandle;

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/Flow.FlowDataPinBlueprintLibrary.MakeStruct", HasNativeBreak = 
"/Script/Flow.FlowDataPinBlueprintLibrary.BreakStruct"))
struct FLOW_API FFlowPin
{
	GENERATED_BODY()

	// A logical name, used during execution of pin
	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FName PinName;

	// An optional Display Name, you can use it to override PinName without the need to update graph connections
	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FText PinFriendlyName;

	UPROPERTY(EditDefaultsOnly, Category = FlowPin)
	FString PinToolTip;

protected:
	// PinType (implies PinCategory)
	UPROPERTY(EditAnywhere, Category = FlowPin)
	EFlowPinType PinType = EFlowPinType::Exec;

	// Sub-category object
	// (used to identify the struct or class type for some PinCategories, see IsSubtypeSupportedPinCategory)
	UPROPERTY()
	TWeakObjectPtr<UObject> PinSubCategoryObject;

#if WITH_EDITORONLY_DATA
	// Filter for limiting the compatible classes for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for 
runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Class", EditConditionHid
es))
	TSubclassOf<UClass> SubCategoryClassFilter = UClass::StaticClass();

	// Filter for limiting the compatible object types for this data pin.
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for 
runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Object", EditConditionHi
des))
	TSubclassOf<UObject> SubCategoryObjectFilter = UObject::StaticClass();

	// Configuration option for setting the EnumClass to a Blueprint Enum 
	// (C++ enums must bind by name using SubCategoryEnumName, due to a limitation with UE's UEnum discovery).
	// This property is editor-only, but it is automatically copied into PinSubCategoryObject if the PinType matches (for 
runtime use).
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Enum", EditConditionHide
s))
	TObjectPtr<UEnum> SubCategoryEnumClass = nullptr;

	// name of enum defined in c++ code, will take priority over asset from EnumType property
	//  (this is a work-around because EnumClass cannot find C++ Enums, 
	//   so you need to type the name of the enum in here, manually)
	// See also: FFlowPin::PostEditChangedEnumName()
	UPROPERTY(EditAnywhere, Category = FlowPin, meta = (EditCondition = "PinType == EFlowPinType::Enum", EditConditionHide
s))
	FString SubCategoryEnumName;
#endif // WITH_EDITORONLY_DATA

public:

	// PinCategory aliases for (a subset of) those defined in UEdGraphSchema_K2
	FORCEINLINE static FName PC_Exec() { return FName(TEXT("exec")); }
	FORCEINLINE static FName PC_Boolean() { return FName(TEXT("bool")); }
	FORCEINLINE static FName PC_Byte() { return FName(TEXT("byte")); }
	FORCEINLINE static FName PC_Class() { return FName(TEXT("class")); }
	FORCEINLINE static FName PC_Int() { return FName(TEXT("int")); }
	FORCEINLINE static FName PC_Int64() { return FName(TEXT("int64")); }
	FORCEINLINE static FName PC_Float() { return FName(TEXT("float")); }
	FORCEINLINE static FName PC_Double() { return FName(TEXT("double")); }
	FORCEINLINE static FName PC_Name() { return FName(TEXT("name")); }
	FORCEINLINE static FName PC_Object() { return FName(TEXT("object")); }
	FORCEINLINE static FName PC_String() { return FName(TEXT("string")); }
	FORCEINLINE static FName PC_Text() { return FName(TEXT("text")); }
	FORCEINLINE static FName PC_Struct() { return FName(TEXT("struct")); }
	FORCEINLINE static FName PC_Enum() { return FName(TEXT("enum")); }

	FORCEINLINE static FName AnyPinName() { return FName(TEXT("AnyPinName")); }

	FFlowPin()
		: PinName(NAME_None)
	{
	}

	FFlowPin(const FName& InPinName)
		: PinName(InPinName)
	{
	}

	FFlowPin(const FString& InPinName)
		: PinName(*InPinName)
	{
	}

	FFlowPin(const FText& InPinName)
		: PinName(*InPinName.ToString())
	{
	}

	FFlowPin(const TCHAR* InPinName)
		: PinName(FName(InPinName))
	{
	}

	FFlowPin(const uint8& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const int32& InPinName)
		: PinName(FName(*FString::FromInt(InPinName)))
	{
	}

	FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	FFlowPin(const FStringView InPinName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FStringView InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, const FString& InPinTooltip)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
		, PinToolTip(InPinTooltip)
	{
	}

	FFlowPin(const FName& InPinName, const FText& InPinFriendlyName, EFlowPinType InFlowPinType, UObject* SubCategoryObjec
t = nullptr)
		: PinName(InPinName)
		, PinFriendlyName(InPinFriendlyName)
	{
		SetPinType(InFlowPinType, SubCategoryObject);
	}

	FFlowPin(const FName& InPinName, EFlowPinType InFlowPinType, UObject* SubCategoryObject = nullptr)
		: PinName(InPinName)
	{
		SetPinType(InFlowPinType, SubCategoryObject);
	}

	FORCEINLINE bool IsValid() const
	{
		return !PinName.IsNone();
	}

	FORCEINLINE bool operator==(const FFlowPin& Other) const
	{
		return PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FFlowPin& Other) const
	{
		return PinName != Other.PinName;
	}

	FORCEINLINE bool operator==(const FName& Other) const
	{
		return PinName == Other;
	}

	FORCEINLINE bool operator!=(const FName& Other) const
	{
		return PinName != Other;
	}

	friend uint32 GetTypeHash(const FFlowPin& FlowPin)
	{
		return GetTypeHash(FlowPin.PinName);
	}

public:

#if WITH_EDITOR
	// Must be called from PostEditChangeProperty() by an owning UObject <sigh>
	// whenever PinType, 
	void PostEditChangedPinTypeOrSubCategorySource();
	FText BuildHeaderText() const;

	static bool ValidateEnum(const UEnum& EnumType);
#endif // WITH_EDITOR

	void SetPinType(const EFlowPinType InFlowPinType, UObject* SubCategoryObject = nullptr);
	EFlowPinType GetPinType() const { return PinType; }
	static FName GetPinCategoryFromPinType(EFlowPinType FlowPinType);
	static const TArray<FName>& GetFlowPinTypeEnumValuesWithoutSpaces();

	const TWeakObjectPtr<UObject>& GetPinSubCategoryObject() const { return PinSubCategoryObject; }

	static bool ArePinArraysMatchingNamesAndTypes(const TArray<FFlowPin>& Left, const TArray<FFlowPin>& Right);
	static bool DoPinsMatchNamesAndTypes(const FFlowPin& LeftPin, const FFlowPin& RightPin)
	{
		return (LeftPin.PinName == RightPin.PinName && LeftPin.PinType == RightPin.PinType && LeftPin.PinSubCategoryObject ==
 RightPin.PinSubCategoryObject);
	}

	// FFlowPin instance signatures for "trait" functions
	FORCEINLINE bool IsExecPin() const { return PinType == EFlowPinType::Exec; }
	FORCEINLINE bool IsDataPin() const { return PinType != EFlowPinType::Exec; }
	// --

	// PinCategory "trait" functions:
	FORCEINLINE static bool IsExecPinCategory(const FName& PC) { return PC == PC_Exec(); }
	FORCEINLINE static bool IsDataPinCategory(const FName& PC) { return PC != PC_Exec(); }
	FORCEINLINE static bool IsBoolPinCategory(const FName& PC) { return PC == PC_Boolean(); }
	FORCEINLINE static bool IsIntPinCategory(const FName& PC) { return PC == PC_Byte() || PC == PC_Int() || PC == PC_Int64
(); }
	FORCEINLINE static bool IsFloatPinCategory(const FName& PC) { return PC == PC_Double() || PC == PC_Float(); }
	FORCEINLINE static bool IsEnumPinCategory(const FName& PC) { return PC == PC_Enum(); }
	FORCEINLINE static bool IsTextPinCategory(const FName& PC) { return PC == PC_Name() || PC == PC_String() || PC == PC_T
ext(); }
	FORCEINLINE static bool IsObjectPinCategory(const FName& PC) { return PC == PC_Object(); }
	FORCEINLINE static bool IsClassPinCategory(const FName& PC) { return PC == PC_Class(); }	
	FORCEINLINE static bool IsStructPinCategory(const FName& PC) { return PC == PC_Struct(); }
	// --

	// IsConvertable trait functions:
	FORCEINLINE static bool IsConvertableToBoolPinCategory(const FName& PC) { return IsBoolPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToIntPinCategory(const FName& PC) { return IsIntPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToFloatPinCategory(const FName& PC) { return IsFloatPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToEnumPinCategory(const FName& PC) { return IsEnumPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToTextPinCategory(const FName& PC) { return IsTextPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToObjectPinCategory(const FName& PC) { return IsObjectPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToClassPinCategory(const FName& PC) { return IsClassPinCategory(PC); }
	FORCEINLINE static bool IsConvertableToStructPinCategory(const FName& PC) { return IsStructPinCategory(PC); }
	// --

	// Metadata keys for properties that bind and auto-generate Data Pins:

	// SourceForOutputFlowPin
	//   May be used on a non-FFlowDataPinProperty within a UFlowNode to bind the
	//   output data pin to use the property as its source.
	//
	//   If a string value is given, it is interpreted as the Data Pin's name,
	//   otherwise, the property's DisplayName (or lacking that, its authored name)
	//   will be assumed to also be the Pin's name.
	FORCEINLINE static FName MetadataKey_SourceForOutputFlowPin() { return FName(TEXT("SourceForOutputFlowPin")); }

	// DefaultForInputFlowPin
	//   May be used on a non-FFlowDataPinProperty within a UFlowNode to bind the
	//   input data pin to use the property as its default value.
	//
	//   If the input pin IS NOT connected to another node, then the bound property
	//   value will be supplied as a default.
	// 
	//   If the input pin IS connected to another node, then the connected node's supplied
	//   value will be used instead of the default from the bound property.
	// 
	//   If a string value is given, it is interpreted as the Data Pin's name,
	//   otherwise, the property's DisplayName (or lacking that, its authored name)
	//   will be assumed to also be the Pin's name.
	FORCEINLINE static FName MetadataKey_DefaultForInputFlowPin() { return FName(TEXT("DefaultForInputFlowPin")); }

	// FlowPinType
	//   May be used on either a property (within a UFlowNode) or a USTRUCT declaration for
	//   a FFlowDataPinProperty subclass.
	//   
	//   If used on a property, then it indicates that a data pin of the given type should be auto-generated,
	//   and bound to the property.  May be used in conjunction with SourceForOutputFlowPin or DefaultForInputFlowPin
	//   (but not both) to determine how the property binding is to be applied (as input default or output supply source)
	//
	//   If used on a FFlowDataPinProperty struct declaration, then it defines the type of pin
	//   that should be auto-generated when the struct is used as a property in a UFlowNode.
	//
	//   The string value of the metadata should exactly match a value in EFlowPinType
	FORCEINLINE static FName MetadataKey_FlowPinType() { return FName(TEXT("FlowPinType")); }
	// --

protected:

	void TrySetStructSubCategoryObjectFromPinType();

private:

};

USTRUCT()
struct FLOW_API FFlowPinHandle
{
	GENERATED_BODY()

	// Update SFlowPinHandleBase code if this property name would be ever changed
	UPROPERTY()
	FName PinName;

	FFlowPinHandle()
		: PinName(NAME_None)
	{
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowInputPinHandle : public FFlowPinHandle
{
	GENERATED_BODY()

	FFlowInputPinHandle()
	{
	}
};

USTRUCT(BlueprintType)
struct FLOW_API FFlowOutputPinHandle : public FFlowPinHandle
{
	GENERATED_BODY()

	FFlowOutputPinHandle()
	{
	}
};

// Processing Flow Nodes creates map of connected pins
USTRUCT()
struct FLOW_API FConnectedPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FName PinName;

	FConnectedPin()
		: NodeGuid(FGuid())
		, PinName(NAME_None)
	{
	}

	FConnectedPin(const FGuid InNodeId, const FName& InPinName)
		: NodeGuid(InNodeId)
		, PinName(InPinName)
	{
	}

	FORCEINLINE bool operator==(const FConnectedPin& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinName == Other.PinName;
	}

	FORCEINLINE bool operator!=(const FConnectedPin& Other) const
	{
		return NodeGuid != Other.NodeGuid || PinName != Other.PinName;
	}

	friend uint32 GetTypeHash(const FConnectedPin& ConnectedPin)
	{
		return GetTypeHash(ConnectedPin.NodeGuid) + GetTypeHash(ConnectedPin.PinName);
	}
};

UENUM(BlueprintType)
enum class EFlowPinActivationType : uint8
{
	Default,
	Forced,
	PassThrough
};

// Every time pin is activated, we record it and display this data while user hovers mouse over pin
#if !UE_BUILD_SHIPPING
struct FLOW_API FPinRecord
{
	double Time;
	FString HumanReadableTime;
	EFlowPinActivationType ActivationType;

	static FString NoActivations();
	static FString PinActivations();
	static FString ForcedActivation();
	static FString PassThroughActivation();

	FPinRecord();
	FPinRecord(const double InTime, const EFlowPinActivationType InActivationType);

private:
	FORCEINLINE static FString DoubleDigit(const int32 Number);
};
#endif




exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content -Raw 'Source/DevKit/Public/BuffFlow/Nodes/BFNode_Base.h' | Select-String -Pattern 'TrySupply|IFlowDataPin|class DEVKIT_API UBFNode_Base' -Context 0,5" in D:\Self\GItGame\Dev01
 succeeded in 594ms:

> #pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_Base.generated.h"

class UBuffFlowComponent;
class UYogAbilitySystemComponent;
class AYogCharacterBase;

/**
 * 鎵€鏈?BuffFlow 鑺傜偣鐨勫熀绫? * 鎻愪緵璁块棶 BuffFlowComponent / ASC / BuffOwner 鐨勪究鎹锋柟娉? */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API UBFNode_Base : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	/** 鑾峰彇鎸傚湪瑙掕壊涓婄殑 BuffFlowComponent */
	UBuffFlowComponent* GetBuffFlowComponent() const;

	/** 鑾峰彇 Buff 鎷ユ湁鑰呯殑 ASC */
	UYogAbilitySystemComponent* GetOwnerASC() const;

	/** 鑾峰彇 Buff 鎷ユ湁鑰呰鑹?*/
	AYogCharacterBase* GetBuffOwner() const;

	/** 鏍规嵁 Target 閫夋嫨鍣ㄨВ鏋愮洰鏍?Actor */
	AActor* ResolveTarget(EBFTargetSelector Selector) const;

private:
	/** 缂撳瓨锛岄伩鍏嶆瘡娆¤皟鐢ㄩ兘鏌ユ壘 */
	mutable TWeakObjectPtr<UBuffFlowComponent> CachedComponent;
};




exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'git diff --check -- Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp Source/DevKit/Private/AbilitySystem/Abilities/GA_Player_FinisherAttack.cpp Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h Source/DevKit/Public/Animation/AN_TriggerFinisherAbility.h' in D:\Self\GItGame\Dev01
 succeeded in 475ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-ChildItem -Recurse -Path Source/DevKit/Private/Animation -Include *.cpp | Select-String -Pattern 'FGameplayEventData' -Context 0,0 | ForEach-Object { \""'$($_.Path):$($_.LineNumber):$($_.Line.Trim())" }' in D:\Self\GItGame\Dev01
 succeeded in 555ms:
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\ANS_FinisherTimeDilation.cpp:43:FGameplayEventData UIPayload;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\ANS_FinisherTimeDilation.cpp:89:FGameplayEventData UIPayload;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AN_MeleeDamage.cpp:69:FGameplayEventData EventData;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AN_MeleeDamage.cpp:81:FGameplayEventData SwingEventData;
D:\Self\GItGame\Dev01\Source\DevKit\Private\Animation\AN_TriggerFinisherAbility.cpp:35:FGameplayEventData Payload;

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content 'Source/DevKit/Private/Animation/AN_MeleeDamage.cpp' -TotalCount 15" in D:\Self\GItGame\Dev01
 succeeded in 824ms:
// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"

UAN_MeleeDamage::UAN_MeleeDamage()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack"));
}


exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Get-Content 'Source/DevKit/Public/AbilitySystem/Abilities/YogGameplayAbility.h' -TotalCount 80" in D:\Self\GItGame\Dev01
 succeeded in 745ms:
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "YogAbilityTypes.h"
#include "Engine/DataTable.h"
#include "Data/AbilityData.h"


#include "YogGameplayAbility.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;
struct FActionData;
struct FHitBoxData;


class AActor;
class AController;
class FText;
class UAnimMontage;
class UGameplayEffect;

struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityEndedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityStartSignature);


UCLASS()
class DEVKIT_API UYogGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UYogGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;


	/** Called when this ability is granted to the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();

	/** Called when this ability is removed from the ability system component. */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityRemoved")
	void K2_OnAbilityRemoved();



	UPROPERTY(BlueprintAssignable)
	FAbilityEndedSignature EventOn_AbilityEnded;

	UPROPERTY(BlueprintAssignable)
	FAbilityStartSignature EventOn_AbilityStart;


	/** Make gameplay effect container spec to be applied later, using the passed in container */
	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual FYogGameplayEffectContainerSpec MakeEffectContainerSpecFromContainer(const FYogGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual FYogGameplayEffectContainerSpec MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);


	/** Applies a gameplay effect container spec that was previously created */
	UFUNCTION(BlueprintCallable, Category = Ability)
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FYogGameplayEffectContainerSpec& ContainerSpec);

	/** Applies a gameplay effect container, by creating and then applying the spec */
	UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Select-String -Path 'Source/DevKit/Public/AbilitySystem/Abilities/YogGameplayAbility.h','Source/DevKit/Private/AbilitySystem/Abilities/YogGameplayAbility.cpp' -SimpleMatch 'GetAbilitySystemComponent' -Context 0,2" in D:\Self\GItGame\Dev01
 succeeded in 624ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command 'Get-PSDrive -PSProvider FileSystem | ForEach-Object { $_.Root } | ForEach-Object { if(Test-Path $_) { Get-ChildItem -Path $_ -Recurse -Filter GameplayAbility.h -ErrorAction SilentlyContinue | Select-Object -First 3 -ExpandProperty FullName } }' in D:\Self\GItGame\Dev01
 succeeded in 63693ms:
D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h
Test-Path : Access is denied
所在位置 行:2 字符: 87
+ ... ForEach-Object { $_.Root } | ForEach-Object { if(Test-Path $_) { Get- ...
+                                                      ~~~~~~~~~~~~
    + CategoryInfo          : PermissionDenied: (G:\:String) [Test-Path], UnauthorizedAccessException
    + FullyQualifiedErrorId : ItemExistsUnauthorizedAccessError,Microsoft.PowerShell.Commands.TestPathCommand
 

exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Select-String -Path 'D:\\UE\\UE_5.4\\Engine\\Plugins\\Runtime\\GameplayAbilities\\Source\\GameplayAbilities\\Public\\Abilities\\GameplayAbility.h' -Pattern 'GetAbilitySystemComponent' -Context 2,2" in D:\Self\GItGame\Dev01
 succeeded in 629ms:

  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:184
:	/** Returns the AbilitySystemComponent that is activating this ability */
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:185
:	UFUNCTION(BlueprintCallable, Category = Ability)
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:186
:	UAbilitySystemComponent* GetAbilitySystemComponentFromActorInfo() const;
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:187
:	UAbilitySystemComponent* GetAbilitySystemComponentFromActorInfo_Checked() const;
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:188
:	UAbilitySystemComponent* GetAbilitySystemComponentFromActorInfo_Ensured() const;
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:189
:
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\GameplayAbility.h:190
:	/** Gets the current actor info bound to this ability - can only be called on instanced abilities. */



exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Select-String -Path 'D:\\UE\\UE_5.4\\Engine\\Plugins\\Runtime\\GameplayAbilities\\Source\\GameplayAbilities\\Public\\Abilities\\Tasks\\AbilityTask_WaitGameplayEvent.h' -Pattern 'DECLARE_DYNAMIC|EventReceived|WaitGameplayEvent' -Context 0,2" in D:\Self\GItGame\Dev01
 succeeded in 626ms:

> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:9:#include "AbilityTask_WaitGameplayEvent.generated.h"
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:10:
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:11:class UAbilitySystemComponent;
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:13:DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitGameplayEventDelegate, FGameplayEventData, Payload
);
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:14:
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:15:UCLASS()
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:16:class GAMEPLAYABILITIES_API UAbilityTask_WaitGameplayEvent : public UAbilityTask
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:17:{
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:18:	GENERATED_UCLASS_BODY()
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:21:	FWaitGameplayEventDelegate	EventReceived;
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:22:
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:23:	/**
> D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:29:	static UAbilityTask_WaitGameplayEvent* WaitGameplayEvent(UGameplayAbility* OwningAbility, FGamepla
yTag EventTag, AActor* OptionalExternalTarget=nullptr, bool OnlyTriggerOnce=false, bool OnlyMatchExact = true);
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:30:
  D:\UE\UE_5.4\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\Abilities\Tasks\AbilityTask_Wai
tGameplayEvent.h:31:	void SetExternalTarget(AActor* Actor);



exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "Select-String -Path 'D:\\UE\\UE_5.4\\Engine\\Source\\Runtime\\Engine\\Classes\\Animation\\AnimNotifies\\AnimNotifyState.h' -Pattern 'GameplayTag' -SimpleMatch" in D:\Self\GItGame\Dev01
 succeeded in 666ms:
exec
"C:\\windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe" -Command "@'
from pathlib import Path
files=['Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h','Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp','Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp','Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp','Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp']
for f in files:
 print('---',f)
 for i,line in enumerate(Path(f).read_text(encoding='utf-8').splitlines(),1):
  if f.endswith('ANS_FinisherTimeDilation.h') and 20<=i<=31: print(f'{i}: {line}')
  if f.endswith('GA_FinisherCharge.cpp') and (90<=i<=100 or 110<=i<=120): print(f'{i}: {line}')
  if f.endswith('GA_ApplyFinisherMark.cpp') and 35<=i<=39: print(f'{i}: {line}')
  if f.endswith('BFNode_SpawnRangedProjectiles.cpp') and (126<=i<=139 or 154<=i<=162): print(f'{i}: {line}')
  if f.endswith('SRuneEditorWidget.cpp') and 3498<=i<=3507: print(f'{i}: {line}')
'@ | python -" in D:\Self\GItGame\Dev01
 succeeded in 833ms:
--- Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h
20: public:
21:     /** 全局慢动作倍率（0.15 = 全局减速到1/6.6，玩家用CustomTimeDilation抵消）*/
22:     UPROPERTY(EditAnywhere, Category = "TimeDilation")
23:     float SlowDilation = 0.15f;
24: 
25:     /** 显示/隐藏提示UI用的GameplayEvent Tag（广播给PlayerController）*/
26:     UPROPERTY(EditAnywhere, Category = "UI")
27:     FGameplayTag PromptShowEventTag;
28: 
29:     UPROPERTY(EditAnywhere, Category = "UI")
30:     FGameplayTag PromptHideEventTag;
31: 
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp
90:     // 循环监听玩家攻击命中事件
91:     WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
92:         this,
93:         TAG_Ability_Event_Attack_Hit,
94:         nullptr,
95:         false,  // OnlyTriggerOnce = false → 重复触发
96:         true
97:     );
98:     WaitHitTask->EventReceived.AddDynamic(this, &UGA_FinisherCharge::OnAttackHit);
99:     WaitHitTask->ReadyForActivation();
100: }
110:     if (!HitTarget)
111:     {
112:         return;
113:     }
114: 
115:     UAbilitySystemComponent* PlayerASC = GetAbilitySystemComponent(CurrentActorInfo);
116:     if (!PlayerASC)
117:     {
118:         return;
119:     }
120: 
--- Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp
35:     }
36: 
37:     UAbilitySystemComponent* ASC = GetAbilitySystemComponent(ActorInfo);
38:     if (!ASC)
39:     {
--- Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp
126: 
127: 		Bullet->InitBullet(SourceCharacter, ResolvedDamage, ResolvedDamageEffect);
128: 		Bullet->SetCombatDeckContextWithGuid(
129: 			ActionContext.ActionType,
130: 			ActionContext.bIsComboFinisher,
131: 			ActionContext.bFromDashSave,
132: 			SharedGuid,
133: 			ResolvedDamage);
134: 		Bullet->SetHitGameplayEvent(
135: 			HitGameplayEventTag,
136: 			bSendHitGameplayEventToSourceASC,
137: 			bUseDamageAsHitGameplayEventMagnitude,
138: 			ResolvedHitEventMagnitude);
139: 		++SpawnedCount;
154: TArray<float> UBFNode_SpawnRangedProjectiles::BuildResolvedYawOffsets(int32 ComboBonusStacks) const
155: {
156: 	int32 BonusProjectileCount = 0;
157: 	if (bAddComboStacksToProjectileCount)
158: 	{
159: 		BonusProjectileCount = FMath::Min(
160: 			FMath::Max(0, MaxBonusProjectiles),
161: 			FMath::Max(0, ComboBonusStacks) * FMath::Max(0, ProjectilesPerComboStack));
162: 	}
--- Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp
3498: 	if (FlowIdx <= 0 || !FlowAssetDataList.IsValidIndex(FlowIdx - 1))
3499: 	{
3500: 		return FReply::Handled();
3501: 	}
3502: 	UObject* Asset = FlowAssetDataList[FlowIdx - 1].GetAsset();
3503: 	if (UFlowAsset* FA = Cast<UFlowAsset>(Asset))
3504: 	{
3505: 		OnCenterTabSelected(ECenterPanelTab::FlowGraph);
3506: 		RebuildGraphEditor(FA);
3507: 	}

codex
The patch introduces compile-blocking issues in the new finisher abilities and notify header. It also has runtime/editor issues that can drop projectile hit events, edit the wrong flow asset, and disable combo bonus projectiles under the documented default cap.

Full review comments:

- [P1] Use the valid GAS ASC accessor — D:/Self/GItGame/Dev01/Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp:37-37
  When these new finisher ability files are compiled, this call does not resolve: `UGameplayAbility` exposes `GetAbilitySystemComponentFromActorInfo()` or `ActorInfo->AbilitySystemComponent.Get()`, not `GetAbilitySystemComponent(ActorInfo)`. The same pattern appears in `GA_FinisherCharge` and `GA_Player_FinisherAttack`, so adding these files will break the DevKit module build.

- [P1] Match the WaitGameplayEvent delegate signature — D:/Self/GItGame/Dev01/Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp:98-98
  For this attack-hit listener, `UAbilityTask_WaitGameplayEvent::EventReceived` is a one-parameter delegate (`FGameplayEventData`), while `OnAttackHit` is declared with `(FGameplayTag, FGameplayEventData)`. Binding this standard task will be rejected by UHT/C++, unlike the custom montage task delegate, so the finisher charge ability will not compile until the handler uses the one-argument payload form or a delegate that supplies the tag.

- [P1] Include GameplayTagContainer for notify tags — D:/Self/GItGame/Dev01/Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h:27-30
  This new header declares `FGameplayTag` UPROPERTYs but only includes `CoreMinimal.h` and `AnimNotifyState.h`; those headers do not define GameplayTags. Because its .cpp includes this header first and UHT parses it directly, the module will fail unless `GameplayTagContainer.h` is included, as in the other animation notify headers.

- [P2] Configure hit events before initial overlap checks — D:/Self/GItGame/Dev01/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp:127-134
  When a bullet is spawned already overlapping a target, `InitBullet()` immediately calls `ScheduleInitialOverlapCheck()` and can apply damage before the newly added `SetHitGameplayEvent()` runs. In that close-range initial-overlap path, `HitGameplayEventTag` is still invalid, so Link FAs waiting on the configured hit event never fire; set all hit-event state before any initial overlap check can execute.

- [P2] Edit the displayed link flow, not the selected rune flow — D:/Self/GItGame/Dev01/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp:3506-3506
  Clicking the new arrow opens a link FA by only rebuilding the graph with an override, but selection remains on the rune and add/delete/open code still calls `GetSelectedFlowAsset()`. In this scenario the graph shows the link flow while node-library additions and delete bookkeeping target the primary rune flow, which can edit or unregister nodes on the wrong asset; the displayed flow needs to become the active flow for editor commands.

- [P2] Treat zero bonus cap as unlimited — D:/Self/GItGame/Dev01/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp:159-161
  With `bAddComboStacksToProjectileCount` enabled and `MaxBonusProjectiles` left at its documented/default value of 0, this `Min(0, computedBonus)` forces the bonus count to 0, so combo stacks never add projectiles. The UI/comment says 0 means no upper limit, so only positive caps should clamp the computed bonus.
2026-05-09T08:51:09.110519Z ERROR codex_core::session: failed to record rollout items: thread 019e0be4-3032-7d02-9760-5b2723598502 not found
2026-05-09T08:51:09.289497Z ERROR codex_core::session: failed to record rollout items: thread 019e0be4-3003-73a1-975c-cac0f2112030 not found
The patch introduces compile-blocking issues in the new finisher abilities and notify header. It also has runtime/editor issues that can drop projectile hit events, edit the wrong flow asset, and disable combo bonus projectiles under the documented default cap.

Full review comments:

- [P1] Use the valid GAS ASC accessor — D:/Self/GItGame/Dev01/Source/DevKit/Private/AbilitySystem/Abilities/GA_ApplyFinisherMark.cpp:37-37
  When these new finisher ability files are compiled, this call does not resolve: `UGameplayAbility` exposes `GetAbilitySystemComponentFromActorInfo()` or `ActorInfo->AbilitySystemComponent.Get()`, not `GetAbilitySystemComponent(ActorInfo)`. The same pattern appears in `GA_FinisherCharge` and `GA_Player_FinisherAttack`, so adding these files will break the DevKit module build.

- [P1] Match the WaitGameplayEvent delegate signature — D:/Self/GItGame/Dev01/Source/DevKit/Private/AbilitySystem/Abilities/GA_FinisherCharge.cpp:98-98
  For this attack-hit listener, `UAbilityTask_WaitGameplayEvent::EventReceived` is a one-parameter delegate (`FGameplayEventData`), while `OnAttackHit` is declared with `(FGameplayTag, FGameplayEventData)`. Binding this standard task will be rejected by UHT/C++, unlike the custom montage task delegate, so the finisher charge ability will not compile until the handler uses the one-argument payload form or a delegate that supplies the tag.

- [P1] Include GameplayTagContainer for notify tags — D:/Self/GItGame/Dev01/Source/DevKit/Public/Animation/ANS_FinisherTimeDilation.h:27-30
  This new header declares `FGameplayTag` UPROPERTYs but only includes `CoreMinimal.h` and `AnimNotifyState.h`; those headers do not define GameplayTags. Because its .cpp includes this header first and UHT parses it directly, the module will fail unless `GameplayTagContainer.h` is included, as in the other animation notify headers.

- [P2] Configure hit events before initial overlap checks — D:/Self/GItGame/Dev01/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp:127-134
  When a bullet is spawned already overlapping a target, `InitBullet()` immediately calls `ScheduleInitialOverlapCheck()` and can apply damage before the newly added `SetHitGameplayEvent()` runs. In that close-range initial-overlap path, `HitGameplayEventTag` is still invalid, so Link FAs waiting on the configured hit event never fire; set all hit-event state before any initial overlap check can execute.

- [P2] Edit the displayed link flow, not the selected rune flow — D:/Self/GItGame/Dev01/Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp:3506-3506
  Clicking the new arrow opens a link FA by only rebuilding the graph with an override, but selection remains on the rune and add/delete/open code still calls `GetSelectedFlowAsset()`. In this scenario the graph shows the link flow while node-library additions and delete bookkeeping target the primary rune flow, which can edit or unregister nodes on the wrong asset; the displayed flow needs to become the active flow for editor commands.

- [P2] Treat zero bonus cap as unlimited — D:/Self/GItGame/Dev01/Source/DevKit/Private/BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.cpp:159-161
  With `bAddComboStacksToProjectileCount` enabled and `MaxBonusProjectiles` left at its documented/default value of 0, this `Min(0, computedBonus)` forces the bonus count to 0, so combo stacks never add projectiles. The UI/comment says 0 means no upper limit, so only positive caps should clamp the computed bonus.
