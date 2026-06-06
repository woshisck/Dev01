> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# Stage One Run Loop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first playable closed loop: new save -> tutorial first run -> run summary -> hub town -> meta upgrade terminal -> second run.

**Architecture:** Use the already implemented C++ systems as the runtime spine: `UYogMetaProgressionSubsystem` loads the two configured DataTables from `DefaultGame.ini`, `AYogHUD` listens for run summary events, `AHubFacilityActor` opens the upgrade tree, and `AYogGameMode` dispatches campaign story tags into `UStoryEventManager`. Stage one is mostly content and Blueprint/DataAsset configuration, with small editor-script updates only to make repeated setup deterministic.

**Tech Stack:** Unreal Engine 5.4, C++ runtime/editor modules, Blueprint assets, DataTable assets, GameplayTags, DataAssets, Unreal Python editor scripts, existing commandlets.

---

## Scope

In scope:
- Create or refresh these five assets at their final paths:
  - `/Game/UI/MetaProgression/WBP_MetaNodeCard`
  - `/Game/UI/MetaProgression/WBP_MetaUpgradeTree`
  - `/Game/UI/MetaProgression/WBP_RunSummary`
  - `/Game/World/Hub/BP_HubFacilityActor`
  - `/Game/World/Hub/BP_HubUpgradeTerminal`
- Create or refresh these two DataTables at the paths already configured in `Config/DefaultGame.ini`:
  - `/Game/MetaProgression/DT_MetaCurrencyRules`
  - `/Game/MetaProgression/DT_MetaUpgradeNodes`
- Configure the active `AYogHUD` Blueprint so `RunSummaryWidgetClass = WBP_RunSummary`.
- Create `/Game/World/Hub/L_HubTown` with one upgrade terminal.
- Create `/Game/Docs/Map/DA_Campaign_Tutorial` for the first-run fixed tutorial stage sequence.
- Create `/Game/Data/Story/DA_StoryEventRegistry_Tutorial` for tutorial event dispatch.
- Verify the loop and commit only files changed by this stage.

Out of scope:
- Active skill runtime, skill loadout UI, skill input actions, and `ActiveSkillGrant` meta node type.
- Additional enemies, boss rooms, production meta-tree content, and final hub art.

## File And Asset Map

- Modify `Docs/WorkSession/create_meta_progression_editor_assets.py`: make the seed DataTable rows match the approved config and set asset defaults that are safe to automate.
- Use `Docs/WorkSession/create_hub_blueprints.py`: create the five BP/WBP assets with the correct parents.
- Use `Source/DevKitEditor/UI/MetaProgressionWidgetTreeSetupCommandlet.cpp`: build required widget trees and named controls.
- Modify `Config/DefaultGameplayTags.ini`: add tutorial and stage tags used by `DA_Campaign_Tutorial` and `DA_StoryEventRegistry_Tutorial`.
- Create/modify `Content/MetaProgression/DT_MetaCurrencyRules.uasset`: test currency rules.
- Create/modify `Content/MetaProgression/DT_MetaUpgradeNodes.uasset`: two test upgrade nodes.
- Create/modify `Content/UI/MetaProgression/WBP_MetaNodeCard.uasset`: upgrade node card widget.
- Create/modify `Content/UI/MetaProgression/WBP_MetaUpgradeTree.uasset`: upgrade tree widget.
- Create/modify `Content/UI/MetaProgression/WBP_RunSummary.uasset`: run summary widget.
- Create/modify `Content/World/Hub/BP_HubFacilityActor.uasset`: shared hub facility Blueprint parent.
- Create/modify `Content/World/Hub/BP_HubUpgradeTerminal.uasset`: terminal Blueprint with `WidgetClass = WBP_MetaUpgradeTree`.
- Create `Content/World/Hub/L_HubTown.umap`: minimal hub map.
- Create `Content/Docs/Map/DA_Campaign_Tutorial.uasset`: tutorial campaign.
- Create `Content/Data/Story/DA_StoryEventRegistry_Tutorial.uasset`: tutorial story event registry.
- Modify the existing `AYogHUD` Blueprint asset, using `/Game/UI/BP_YogHUD` as the standard location when no HUD Blueprint exists yet.

---

### Task 1: Preflight The Stage-One Scope

**Files:**
- Read: `Config/DefaultGame.ini`
- Read: `Config/DefaultGameplayTags.ini`
- Read: `Source/DevKit/Public/MetaProgression/MetaTypes.h`
- Read: `Source/DevKit/Public/UI/MetaNodeCardWidgetBase.h`
- Read: `Source/DevKit/Public/UI/YogMetaUpgradeTreeWidgetBase.h`
- Read: `Source/DevKit/Public/UI/YogRunSummaryWidgetBase.h`

- [ ] **Step 1: Check worktree status**

Run:

```powershell
git status --short
```

Expected known unrelated modified files can remain unstaged:

```text
 M Docs/01_策划需求与版本方案/429反馈迭代.md
 M Docs/01_策划需求与版本方案/核心方案/01_开发方案/符文/符文连携系统后续开发方案.md
 M Docs/03_策划配置与制作手册/核心配置说明/道具/油瓶雷石烟雾配置说明.md
```

- [ ] **Step 2: Confirm meta table config is already wired**

Run:

```powershell
rg -n "MetaUpgradeNodeTable|MetaCurrencyRuleTable" Config/DefaultGame.ini
```

Expected:

```text
Config/DefaultGame.ini:44:MetaUpgradeNodeTable=/Game/MetaProgression/DT_MetaUpgradeNodes.DT_MetaUpgradeNodes
Config/DefaultGame.ini:45:MetaCurrencyRuleTable=/Game/MetaProgression/DT_MetaCurrencyRules.DT_MetaCurrencyRules
```

- [ ] **Step 3: Confirm test currency tag exists**

Run:

```powershell
rg -n 'Currency\.Meta\.Common\.A' Config/DefaultGameplayTags.ini
```

Expected:

```text
Config/DefaultGameplayTags.ini:179:+GameplayTagList=(Tag="Currency.Meta.Common.A",DevComment="局外货币：普通资源A")
```

- [ ] **Step 4: Commit checkpoint is not created**

Do not commit during preflight. This step only confirms the stage-one work starts from the approved design commit and current local changes are understood.

---

### Task 2: Add Gameplay Tags For Tutorial Campaign Data

**Files:**
- Modify: `Config/DefaultGameplayTags.ini`

- [ ] **Step 1: Add the exact tag block**

Append these entries below the existing meta currency tags, keeping the same `+GameplayTagList` format:

```ini
+GameplayTagList=(Tag="Level.Stage.WeaponPickup",DevComment="教程阶段：拾取武器")
+GameplayTagList=(Tag="Level.Stage.FirstCombat",DevComment="教程阶段：第一场战斗")
+GameplayTagList=(Tag="Level.Stage.ShuffleRoom",DevComment="教程阶段：洗牌提示")
+GameplayTagList=(Tag="Level.Stage.Reward",DevComment="教程阶段：奖励选择")
+GameplayTagList=(Tag="Level.Stage.Backpack",DevComment="教程阶段：背包整理")
+GameplayTagList=(Tag="Level.Stage.LinkCard",DevComment="教程阶段：连携卡")
+GameplayTagList=(Tag="Level.Stage.Finisher",DevComment="教程阶段：终结技")
+GameplayTagList=(Tag="Level.Stage.RouteChoice",DevComment="教程阶段：路线选择")
+GameplayTagList=(Tag="Level.Stage.RunEnd",DevComment="教程阶段：跑局结算")
+GameplayTagList=(Tag="Tutorial.WeaponPickup",DevComment="教程事件：武器拾取弹窗")
+GameplayTagList=(Tag="Tutorial.CardConsume",DevComment="教程事件：卡牌消耗")
+GameplayTagList=(Tag="Tutorial.Shuffle",DevComment="教程事件：洗牌提示")
+GameplayTagList=(Tag="Tutorial.RewardToDeck",DevComment="教程事件：奖励进入卡组")
+GameplayTagList=(Tag="Tutorial.BackpackArrange",DevComment="教程事件：背包整理")
+GameplayTagList=(Tag="Tutorial.LinkCard",DevComment="教程事件：连携卡")
+GameplayTagList=(Tag="Tutorial.Finisher",DevComment="教程事件：终结技")
+GameplayTagList=(Tag="Tutorial.RouteRewardChoice",DevComment="教程事件：路线奖励选择")
+GameplayTagList=(Tag="Tutorial.RunSummary",DevComment="教程事件：跑局结算")
```

- [ ] **Step 2: Verify every new tag appears once**

Run:

```powershell
rg -n "Level\.Stage\.WeaponPickup|Tutorial\.WeaponPickup|Tutorial\.RunSummary" Config/DefaultGameplayTags.ini
```

Expected:

```text
The command prints one matching line for Level.Stage.WeaponPickup, one matching line for Tutorial.WeaponPickup, and one matching line for Tutorial.RunSummary.
```

- [ ] **Step 3: Commit the tag config**

Run:

```powershell
git add Config/DefaultGameplayTags.ini
git commit -m "Add tutorial campaign gameplay tags"
```

Expected: one commit containing only `Config/DefaultGameplayTags.ini`.

---

### Task 3: Update The Meta Progression Seed Script

**Files:**
- Modify: `Docs/WorkSession/create_meta_progression_editor_assets.py`

- [ ] **Step 1: Replace the CSV seed data**

Replace the existing `currency_csv` and `node_csv` constants with:

```python
currency_csv = """---,CurrencyTag,DisplayName,ShortName,Icon,MaxCapacity
Common_A,Currency.Meta.Common.A,星骸碎片,碎片,,0
"""

node_csv = """---,DisplayName,Side,MaxLevel,MysticLevelRequired,Prerequisites,CostsPerLevel,EffectType,StatAttribute,StatValuePerLevel,FeatureTag,StarterRuneToGrant,EditorPositionX,EditorPositionY
Node.Flesh.HP.Basic,生命强化Ⅰ,Flesh,3,0,,"((CurrencyTag=Currency.Meta.Common.A,Amount=10))",StatBoost,,0.0,,,0,0
Node.Flesh.Attack.Basic,攻击强化Ⅰ,Flesh,3,0,,"((CurrencyTag=Currency.Meta.Common.A,Amount=15))",StatBoost,,0.0,,,320,0
"""
```

- [ ] **Step 2: Enable automated Blueprint default assignment**

Replace the two disabled `if False and ...` blocks near the end of the script with:

```python
if tree_asset:
    tree_cdo = get_cdo(WBP_TREE)
    card_class = load_bp_class(WBP_CARD)
    tree_cdo.set_editor_property("node_card_widget_class", card_class)
    tree_asset.mark_package_dirty()
    unreal.KismetEditorUtilities.compile_blueprint(tree_asset)
    unreal.EditorAssetLibrary.save_asset(WBP_TREE, False)

if terminal_asset:
    terminal_cdo = get_cdo(BP_TERMINAL)
    terminal_cdo.set_editor_property("facility_display_name", unreal.Text("升级终端"))
    terminal_asset.mark_package_dirty()
    unreal.KismetEditorUtilities.compile_blueprint(terminal_asset)
    unreal.EditorAssetLibrary.save_asset(BP_TERMINAL, False)
```

The terminal `WidgetClass` is assigned by `Docs/WorkSession/create_hub_blueprints.py`.

- [ ] **Step 3: Verify the script contains approved row names**

Run:

```powershell
rg -n "Common_A|Node\.Flesh\.HP\.Basic|Node\.Flesh\.Attack\.Basic|星骸碎片|升级终端" Docs/WorkSession/create_meta_progression_editor_assets.py
```

Expected: all five strings appear.

- [ ] **Step 4: Commit the script update**

Run:

```powershell
git add Docs/WorkSession/create_meta_progression_editor_assets.py
git commit -m "Seed stage one meta progression assets"
```

Expected: one commit containing only the seed script.

---

### Task 4: Create The Five BP/WBP Assets And DataTables

**Files:**
- Create/modify: `Content/UI/MetaProgression/WBP_MetaNodeCard.uasset`
- Create/modify: `Content/UI/MetaProgression/WBP_MetaUpgradeTree.uasset`
- Create/modify: `Content/UI/MetaProgression/WBP_RunSummary.uasset`
- Create/modify: `Content/World/Hub/BP_HubFacilityActor.uasset`
- Create/modify: `Content/World/Hub/BP_HubUpgradeTerminal.uasset`
- Create/modify: `Content/MetaProgression/DT_MetaCurrencyRules.uasset`
- Create/modify: `Content/MetaProgression/DT_MetaUpgradeNodes.uasset`

- [ ] **Step 1: Run the asset creation script**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -ExecutePythonScript=Docs/WorkSession/create_hub_blueprints.py -unattended -nop4 -nosplash
```

Expected output includes:

```text
WBP_MetaUpgradeTree
WBP_MetaNodeCard
WBP_RunSummary
BP_HubFacilityActor
BP_HubUpgradeTerminal
Done.
```

- [ ] **Step 2: Run the DataTable seed script**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -ExecutePythonScript=Docs/WorkSession/create_meta_progression_editor_assets.py -unattended -nop4 -nosplash
```

Expected output writes `Saved/MetaProgressionEditorAssets.txt` and saves:

```text
/Game/MetaProgression/DT_MetaUpgradeNodes
/Game/MetaProgression/DT_MetaCurrencyRules
```

- [ ] **Step 3: Build required widget trees**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=MetaProgressionWidgetTreeSetup -Apply -ForceLayout -unattended -nop4 -nosplash -nullrhi
```

Expected: commandlet exits with code `0`.

- [ ] **Step 4: Verify WBP_MetaNodeCard controls**

Open `/Game/UI/MetaProgression/WBP_MetaNodeCard` and confirm these Designer widgets exist with exact names:

```text
TxtNodeName
TxtLevelProgress
TxtCost
ProgressLevel
BtnPurchase
```

- [ ] **Step 5: Verify WBP_MetaUpgradeTree controls and defaults**

Open `/Game/UI/MetaProgression/WBP_MetaUpgradeTree` and confirm:

```text
TxtCurrencyAmount
BtnFleshSide
BtnMysticSide
BtnClose
NodeList
NodeCardWidgetClass = /Game/UI/MetaProgression/WBP_MetaNodeCard.WBP_MetaNodeCard_C
```

The runtime has a native path that calls `NodeList->ClearChildren`, creates `NodeCardWidgetClass`, calls `InitCard`, binds `OnPurchaseRequested`, and adds the card to `NodeList`. Keep the Blueprint events `BP_AddNodeCard` and `BP_ClearNodeCards` empty for stage one unless a later visual-only pass needs a graph override.

- [ ] **Step 6: Verify WBP_RunSummary controls and defaults**

Open `/Game/UI/MetaProgression/WBP_RunSummary` and confirm:

```text
TxtFloorReached
TxtEnemiesKilled
BtnReturnToHub
HubLevelName = L_HubTown
```

- [ ] **Step 7: Verify BP_HubUpgradeTerminal defaults**

Open `/Game/World/Hub/BP_HubUpgradeTerminal` and confirm:

```text
Parent Class = BP_HubFacilityActor
WidgetClass = /Game/UI/MetaProgression/WBP_MetaUpgradeTree.WBP_MetaUpgradeTree_C
FacilityDisplayName = 升级终端
```

- [ ] **Step 8: Commit generated meta assets**

Run:

```powershell
git add Content/UI/MetaProgression Content/World/Hub Content/MetaProgression
git commit -m "Create stage one meta progression assets"
```

Expected: one commit containing the five BP/WBP assets and two DataTables.

---

### Task 5: Configure Run Summary On AYogHUD

**Files:**
- Create/modify: `Content/UI/BP_YogHUD.uasset`
- Modify: `Content/Code/Core/GameMode/B_GameMode.uasset`

- [ ] **Step 1: Locate an existing AYogHUD Blueprint**

Run:

```powershell
Get-ChildItem -Path Content -Recurse -Filter '*.uasset' | Where-Object { $_.Name -match 'BP_YogHUD|B_HUD_Intro' } | Select-Object -ExpandProperty FullName
```

Expected accepted result is one of:

```text
D:\Self\GItGame\Dev01\Content\UI\BP_YogHUD.uasset
D:\Self\GItGame\Dev01\Content\UI\B_HUD_Intro.uasset
D:\Self\GItGame\Dev01\Content\UI\Playtest_UI\HUD\BP_YogHUD.uasset
```

- [ ] **Step 2: Create the standard HUD Blueprint when the search returns no result**

In the editor:

```text
Path: /Game/UI/
Asset Name: BP_YogHUD
Parent Class: YogHUD
```

Open `/Game/Code/Core/GameMode/B_GameMode` and set:

```text
HUD Class = /Game/UI/BP_YogHUD.BP_YogHUD_C
```

- [ ] **Step 3: Set RunSummaryWidgetClass**

Open the selected `AYogHUD` Blueprint and set:

```text
RunSummaryWidgetClass = /Game/UI/MetaProgression/WBP_RunSummary.WBP_RunSummary_C
```

- [ ] **Step 4: Compile and save**

Compile and save the selected `AYogHUD` Blueprint and `/Game/Code/Core/GameMode/B_GameMode`.

- [ ] **Step 5: Commit HUD wiring**

Run:

```powershell
git add Content/UI/BP_YogHUD.uasset Content/UI/B_HUD_Intro.uasset Content/UI/Playtest_UI/HUD/BP_YogHUD.uasset Content/Code/Core/GameMode/B_GameMode.uasset
git commit -m "Wire run summary widget into HUD"
```

Expected: Git stages the existing HUD asset that was modified, or the newly created `/Game/UI/BP_YogHUD.uasset` plus `B_GameMode.uasset`. If `git add` reports a missing path for non-existing candidate assets, rerun `git add` with only the paths that exist.

---

### Task 6: Build The Minimal Hub Town Map

**Files:**
- Create: `Content/World/Hub/L_HubTown.umap`

- [ ] **Step 1: Create the level**

In the editor:

```text
File -> New Level -> Basic
Save As: /Game/World/Hub/L_HubTown
```

- [ ] **Step 2: Configure World Settings**

Set:

```text
GameMode Override = /Game/Code/Core/GameMode/B_GameMode.B_GameMode_C
```

- [ ] **Step 3: Place the player start**

Add a `PlayerStart` actor:

```text
Name: PlayerStart_Hub
Location: X=0, Y=0, Z=120
Rotation: X=0, Y=0, Z=0
```

- [ ] **Step 4: Place the upgrade terminal**

Add `/Game/World/Hub/BP_HubUpgradeTerminal`:

```text
Name: BP_HubUpgradeTerminal_Upgrade
Location: X=250, Y=0, Z=0
Rotation: X=0, Y=0, Z=180
```

- [ ] **Step 5: Save and test map load**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject /Game/World/Hub/L_HubTown -game -unattended -nop4 -nosplash -nullrhi
```

Expected: editor command starts the map without asset load errors. Close it after the map reaches BeginPlay.

- [ ] **Step 6: Commit hub map**

Run:

```powershell
git add Content/World/Hub/L_HubTown.umap
git commit -m "Add minimal hub town map"
```

Expected: one commit containing only `L_HubTown.umap`.

---

### Task 7: Configure Tutorial Campaign DataAsset

**Files:**
- Create: `Content/Docs/Map/DA_Campaign_Tutorial.uasset`
- Modify: one or more tutorial room assets under `Content/Docs/Map/`

- [ ] **Step 1: Create DA_Campaign_Tutorial**

In the editor:

```text
Path: /Game/Docs/Map/
Asset Name: DA_Campaign_Tutorial
Class: CampaignDataAsset
```

- [ ] **Step 2: Assign campaign room references**

Set:

```text
DefaultStartingRoom = /Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom
RoomPool =
  /Game/Docs/Map/DA_Room_Prison_Normal
  /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01a
  /Game/Docs/Map/DA_L1_Room/DA_Room_CL_corridor_01b
  /Game/Docs/Map/DA_L1_Room/DA_Room_CL_WaterDungeon
LayerTag = empty
```

- [ ] **Step 3: Fill FloorTable with the fixed tutorial sequence**

Create these eight rows in order:

```text
Index 0
TotalDifficultyScore = 0
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.WeaponPickup
StoryEventTags = Tutorial.WeaponPickup

Index 1
TotalDifficultyScore = 10
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.FirstCombat
StoryEventTags = Tutorial.CardConsume

Index 2
TotalDifficultyScore = 10
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.ShuffleRoom
StoryEventTags = Tutorial.Shuffle

Index 3
TotalDifficultyScore = 10
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.Reward
StoryEventTags = Tutorial.RewardToDeck

Index 4
TotalDifficultyScore = 0
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.Backpack
StoryEventTags = Tutorial.BackpackArrange

Index 5
TotalDifficultyScore = 15
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.LinkCard
StoryEventTags = Tutorial.LinkCard

Index 6
TotalDifficultyScore = 20
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.Finisher
StoryEventTags = Tutorial.Finisher

Index 7
TotalDifficultyScore = 0
bForceElite = false
EliteChance = 0.0
ShopChance = 0.0
EventChance = 0.0
GlobalStageTag = Level.Stage.RouteChoice
StoryEventTags = Tutorial.RouteRewardChoice
```

- [ ] **Step 4: Configure first-loop meta currency reward**

Open `/Game/Docs/Map/DA_Room_Prison_Normal` and set:

```text
MetaCurrencyRewards =
  CurrencyTag = Currency.Meta.Common.A
  Amount = 25
```

This guarantees the first completed combat room can fund at least one test upgrade: `生命强化Ⅰ` costs 10 and `攻击强化Ⅰ` costs 15.

- [ ] **Step 5: Configure the active GameMode for tutorial campaign testing**

Open `/Game/Code/Core/GameMode/B_GameMode` and set:

```text
CampaignData = /Game/Docs/Map/DA_Campaign_Tutorial
bDispatchStoryEventsFromCampaign = true
```

Set `StoryEventRegistry` after Task 8 creates the registry.

- [ ] **Step 6: Commit tutorial campaign data**

Run:

```powershell
git add Content/Docs/Map/DA_Campaign_Tutorial.uasset Content/Docs/Map/DA_Room_Prison_Normal.uasset Content/Code/Core/GameMode/B_GameMode.uasset
git commit -m "Configure tutorial campaign sequence"
```

Expected: one commit containing the tutorial campaign, the room reward update, and the GameMode campaign reference.

---

### Task 8: Configure Tutorial Story Event Registry

**Files:**
- Create: `Content/Data/Story/DA_StoryEventRegistry_Tutorial.uasset`
- Modify: `Content/Code/Core/GameMode/B_GameMode.uasset`
- Verify: `Content/Docs/UI/Tutorial/DA_TutorialRegistry.uasset`

- [ ] **Step 1: Refresh tutorial popup content**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -run=Tutorial512Setup -Apply -unattended -nop4 -nosplash
```

Expected: commandlet exits with code `0` and refreshes `/Game/Docs/UI/Tutorial/DA_TutorialRegistry`.

- [ ] **Step 2: Create DA_StoryEventRegistry_Tutorial**

In the editor:

```text
Path: /Game/Data/Story/
Asset Name: DA_StoryEventRegistry_Tutorial
Class: StoryEventRegistryDA
```

- [ ] **Step 3: Add registry entries**

Create these entries in `Entries`:

```text
Entry 0
EventTag = Tutorial.WeaponPickup
ActionType = TutorialPopup
TutorialEventID = tutorial_weapon_pickup
bPauseGame = true
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一局拾取武器后展示。

Entry 1
EventTag = Tutorial.CardConsume
ActionType = BroadcastOnly
TutorialEventID = None
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一战阶段广播；卡牌消耗由战斗 HUD 表达。

Entry 2
EventTag = Tutorial.Shuffle
ActionType = TutorialPopup
TutorialEventID = tutorial_shuffle_hint
bPauseGame = false
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一次洗牌或补牌节奏提示。

Entry 3
EventTag = Tutorial.RewardToDeck
ActionType = TutorialPopup
TutorialEventID = tutorial_first_rune
bPauseGame = true
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一次选择符文奖励后展示。

Entry 4
EventTag = Tutorial.BackpackArrange
ActionType = TutorialPopup
TutorialEventID = tutorial_backpack
bPauseGame = true
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一次进入整理环节展示。

Entry 5
EventTag = Tutorial.LinkCard
ActionType = TutorialPopup
TutorialEventID = tutorial_card_link
bPauseGame = true
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 第一次出现连携卡教学展示。

Entry 6
EventTag = Tutorial.Finisher
ActionType = BroadcastOnly
TutorialEventID = None
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 终结技阶段先广播，后续用专门 LevelFlow 强化表现。

Entry 7
EventTag = Tutorial.RouteRewardChoice
ActionType = BroadcastOnly
TutorialEventID = None
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 路线选择阶段先广播，避免第一版弹窗过密。

Entry 8
EventTag = Tutorial.RunSummary
ActionType = BroadcastOnly
TutorialEventID = None
bOnlyWhenTutorialIncomplete = true
bFireOncePerRun = true
DesignerNote = 结算阶段由 WBP_RunSummary 负责表达。
```

- [ ] **Step 4: Wire registry into GameMode**

Open `/Game/Code/Core/GameMode/B_GameMode` and set:

```text
StoryEventRegistry = /Game/Data/Story/DA_StoryEventRegistry_Tutorial
bDispatchStoryEventsFromCampaign = true
```

- [ ] **Step 5: Commit registry wiring**

Run:

```powershell
git add Content/Data/Story/DA_StoryEventRegistry_Tutorial.uasset Content/Code/Core/GameMode/B_GameMode.uasset Content/Docs/UI/Tutorial
git commit -m "Configure tutorial story event registry"
```

Expected: one commit containing the registry, the GameMode reference, and tutorial popup assets refreshed by the commandlet.

---

### Task 9: Verify The Closed Loop

**Files:**
- Verify generated/modified assets from Tasks 2-8.

- [ ] **Step 1: Run the meta settings automation test**

Run:

```powershell
UnrealEditor-Cmd.exe DevKit.uproject -ExecCmds="Automation RunTests DevKit.MetaProgression.SettingsReadConfiguredTables; Quit" -unattended -nop4 -nosplash -nullrhi
```

Expected: test `DevKit.MetaProgression.SettingsReadConfiguredTables` passes.

- [ ] **Step 2: Run a new-save tutorial smoke test in PIE**

In editor:

```text
1. Start from a new save.
2. Start DA_Campaign_Tutorial through /Game/Code/Core/GameMode/B_GameMode.
3. Confirm Tutorial.WeaponPickup opens tutorial_weapon_pickup.
4. Clear at least one combat room.
5. Confirm Currency.Meta.Common.A increases by 25 after room clear.
6. End the run through death or clear flow.
7. Confirm WBP_RunSummary appears.
8. Click BtnReturnToHub.
9. Confirm /Game/World/Hub/L_HubTown loads.
10. Interact with BP_HubUpgradeTerminal_Upgrade.
11. Confirm WBP_MetaUpgradeTree opens and shows Node.Flesh.HP.Basic and Node.Flesh.Attack.Basic.
12. Purchase one node.
13. Start a second run and confirm the same save retains the purchased node level.
```

- [ ] **Step 3: Verify no unrelated files are staged**

Run:

```powershell
git status --short
```

Expected: all stage-one files are clean after commits. The unrelated docs listed in Task 1 can remain modified and unstaged.

- [ ] **Step 4: Final integration commit is not created**

No squash or cleanup commit is needed because each task has its own commit. Keep unrelated local docs unstaged.

---

## Phase Two Boundary

After this plan is complete, start a separate active skill plan with these accepted constraints:

- Default active skill slots: 1, meta progression unlocks up to 3.
- Input model: `SkillSelect` cycles current skill, `SkillUse` activates selected skill.
- Skill data: `UActiveSkillDataAsset` defines GA class, icon, cooldown type, and carry rules.
- Runtime component: `UPlayerActiveSkillComponent` lives on `PlayerCharacterBase`.
- UI: `WBP_LoadoutSelect` in hub and 1-3 skill slots in HUD.
- Meta extension: add `ActiveSkillGrant` node type after the closed loop is playable.

