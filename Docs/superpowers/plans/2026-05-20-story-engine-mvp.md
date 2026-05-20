# Story Engine MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first-layer Story Engine MVP as an experience director for memory tutorial, first-run guidance, and hub onboarding.

**Architecture:** Add a `UStoryEngineSubsystem` that evaluates `UStoryRuleSetDA` data assets using event context, flag scopes, conditions, actions, fire policy, and priority. Keep the existing `UStoryEventManager` as the legacy event entry point and forward its events into the new engine instead of creating a competing event bus.

**Tech Stack:** Unreal Engine 5.4 C++, GameInstanceSubsystem, DataAsset, GameplayTags, YogSaveSubsystem, TutorialManager, LevelFlow, MetaProgression, UE automation tests.

---

### Task 1: Add Rule Data Tests

**Files:**
- Create: `Source/DevKit/Private/Tests/StoryEngineTests.cpp`

- [ ] **Step 1: Write failing automation tests**

Add tests that create a `UStoryRuleSetDA`, insert rules for valid existing tags, and assert that disabled rules are ignored and matching rules are sorted by priority.

- [ ] **Step 2: Run build to verify red**

Run:
`& "D:\UE\UE_5.4\Engine\Build\BatchFiles\Build.bat" DevKitEditor Win64 Development -Project="D:\Self\GItGame\Dev01\DevKit.uproject" -WaitMutex`

Expected: compile fails because `StoryRuleSetDA.h` and rule types do not exist yet.

### Task 2: Implement Data Types

**Files:**
- Create: `Source/DevKit/Public/Story/StoryRuleTypes.h`
- Create: `Source/DevKit/Public/Story/StoryRuleSetDA.h`
- Create: `Source/DevKit/Private/Story/StoryRuleSetDA.cpp`

- [ ] **Step 1: Add enums and structs**

Define flag scope, condition type, action type, fire policy, condition match policy, event context, condition, action, rule, and quest task state structs.

- [ ] **Step 2: Add rule set asset**

Add `UStoryRuleSetDA` with `Rules`, `bEnabled`, and `GetRulesForEventSorted`.

- [ ] **Step 3: Run build**

Expected: Task 1 tests compile and pass for rule filtering/sorting.

### Task 3: Add Save Data

**Files:**
- Modify: `Source/DevKit/Public/SaveGame/YogSaveGame.h`
- Modify: `Source/DevKit/Private/SaveGame/YogSaveSubsystem.cpp`

- [ ] **Step 1: Add persistent story fields**

Add `StoryFlags`, `StoryFiredRuleIds`, and `StoryQuestTasks` to `UYogSaveGame`.

- [ ] **Step 2: Clear story onboarding state on new game**

Reset story flags, fired rules, and quest tasks inside `ResetSlotForNewGame`.

### Task 4: Implement Story Engine Subsystem

**Files:**
- Create: `Source/DevKit/Public/Story/StoryEngineSettings.h`
- Create: `Source/DevKit/Private/Story/StoryEngineSettings.cpp`
- Create: `Source/DevKit/Public/Story/StoryEngineSubsystem.h`
- Create: `Source/DevKit/Private/Story/StoryEngineSubsystem.cpp`
- Modify: `Source/DevKit/DevKit.Build.cs`

- [ ] **Step 1: Add settings**

Add `UStoryEngineSettings` with `TArray<TSoftObjectPtr<UStoryRuleSetDA>> RuleSets`.

- [ ] **Step 2: Add subsystem rule loading and event broadcast**

Load configured rule sets on initialize, allow runtime `SetRuleSets`, and expose `BroadcastStoryEvent` / `BroadcastStoryEventWithContext`.

- [ ] **Step 3: Add flag and fire policy storage**

Implement Save, Run, and Session flag scopes plus Always, OncePerSave, OncePerRun, and OncePerMap fire policies.

- [ ] **Step 4: Add condition evaluation**

Support HasFlag, FeatureUnlocked, TutorialStateEquals, RunCountAtLeast, EventTagEquals, ContextTagMatches, and inversion.

- [ ] **Step 5: Add action dispatch**

Support SetFlag, ClearFlag, PlayLevelFlow, ShowTutorialPopup, ShowInfoHint, SetQuestTask, CompleteQuestTask, UnlockFeature, AddMetaCurrency, and TriggerStoryEvent.

### Task 5: Integrate Existing Story Event Manager

**Files:**
- Modify: `Source/DevKit/Private/Story/StoryEventManager.cpp`
- Modify: `Source/DevKit/Public/Story/StoryEventTypes.h`

- [ ] **Step 1: Convert legacy context to MVP context**

Forward each event handled by `UStoryEventManager::ProcessCampaignStage` into `UStoryEngineSubsystem`.

- [ ] **Step 2: Reset run state together**

Call `StoryEngineSubsystem->ResetRunState()` from `UStoryEventManager::ResetRunEvents`.

### Task 6: Verify

**Files:**
- No file changes.

- [ ] **Step 1: Build DevKitEditor**

Run the same `Build.bat` command.

- [ ] **Step 2: Run relevant automation tests if editor commandlet is available**

Run Story Engine tests through `UnrealEditor-Cmd.exe` automation if the local editor can launch in command-line mode.

- [ ] **Step 3: Inspect git diff**

Confirm only Story engine, save data, build config, tests, and this plan changed.

### Task 7: Seed Initial Story Content

**Files:**
- Create: `Config/Tags/StoryTag.ini`
- Modify: `Config/DefaultGame.ini`
- Create: `Source/DevKitEditor/Story/StoryRuleSetSetupCommandlet.h`
- Create: `Source/DevKitEditor/Story/StoryRuleSetSetupCommandlet.cpp`
- Create: `Content/Story/Rules/SR_MemoryTutorial.uasset`
- Create: `Content/Story/Rules/SR_FirstRun.uasset`
- Create: `Content/Story/Rules/SR_HubOnboarding.uasset`

- [ ] **Step 1: Add Story/Tutorial gameplay tags**

Add tags for memory tutorial events, first-run events, hub onboarding events, story flags, quest ids, sources, and legacy tutorial stage tags already referenced by existing tutorial campaign assets.

- [ ] **Step 2: Add configured default rule sets**

Add `UStoryEngineSettings` entries for `/Game/Story/Rules/SR_MemoryTutorial`, `/Game/Story/Rules/SR_FirstRun`, and `/Game/Story/Rules/SR_HubOnboarding`.

- [ ] **Step 3: Add a repeatable setup commandlet**

Create `UStoryRuleSetSetupCommandlet` so the three seed assets can be regenerated from code with deterministic rules.

- [ ] **Step 4: Generate assets**

Run:
`& "D:\UE\UE_5.4\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Self\GItGame\Dev01\DevKit.uproject" -run=StoryRuleSetSetup -Apply -unattended -nop4 -nosplash -NoSound -NullRHI -log`

Expected: three packages are written under `Content/Story/Rules/`. Existing unrelated missing asset errors may still make the commandlet process return non-zero, so verify asset load through automation tests.
