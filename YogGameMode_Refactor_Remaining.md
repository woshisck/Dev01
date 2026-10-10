# YogGameMode 重构 — 未完成任务 (Handoff)

**最后更新**: 2026-10-09
**当前状态**: 编译通过 (`Result: Succeeded`)，自动化测试**未验证**
**原始计划**: `C:\Users\sunchuankai\.claude\plans\delightful-napping-kettle.md`

---

## 一、已完成

| 阶段 | 内容 | 状态 |
|---|---|---|
| Phase 0 | 基线构建验证 | 完成 |
| Phase 1 | 移除 first-run tutorial 功能 | 完成 |
| Phase 1b | 移除 512 tutorial 系统 + 保留存档槽概念 | 完成 |
| Phase 2 | 删除死代码 | 完成 |

**成果**: 删除 43 个文件，修改 38 个文件。`YogGameMode.cpp` 4770 → **4172 行**，`YogGameMode.h` 751 → **674 行**。`Source/` 下已无任何 `UTutorialManager` / `ETutorialState` / `UDialogContentDA` / `FTutorialPage` / `FirstRunTutorialDirector` / `ATutorialMobSpawner` 引用。

### 本次做的两个关键判断（非机械删除，需知悉）

1. **五个枚举保留了 `Deprecated_*` 占位值，没有直接删除**
   `EYogUIScreenId` 在 `YogUIRegistry.h` 有明确注释：该枚举以 **raw uint8 索引**序列化进 `DA_YogUIRegistry.uasset`。从中间删除会**静默移位**其后所有 screen id。同理处理了 `EStoryEncounterActionKind`、`EStoryConditionType`、`EStoryActionType`、`EStoryEventActionType`。
   → 这些占位值**不要**随手清理，除非先确认对应 Content 资产已重新保存。

2. **`bInitialRoomPortalsRequireWeapon` 被保留并重新归类**
   原本挂在 `Campaign|FirstRun` 分类下，但它驱动的 `ShouldDelayInitialRoomPortalsUntilWeapon()` 是通用主城行为（没拿武器前不开传送门），与教程无关。已移到 `Campaign` 分类保留。

### 行为变更（需要产品确认）

- **主菜单 Start 按钮**：`YogGameInstanceBase.cpp` `StartNormalRunFromFrontend()` 原本强制选中保留槽，现改为 `SelectSlot(0)`。
  **风险**：若玩家 slot 0 已有存档，点 Start 会直接选中该槽。原计划建议走选档界面，当前实现是直接选 0，**可能覆盖玩家进度**，建议后续加冲突检测。
- **`bScriptedDefeatGameOver`** 仍被 `GI->ShowGameOverScreen(...)` 读取，但已无任何代码写 `true`，现在恒为 false。
- **`FStoryEventEntry::ActionType`** 默认值从 `TutorialPopup` 改为 `BroadcastOnly`（仅影响新建条目）。

---

## 二、待验证（优先做）

### 1. 自动化测试未跑通
`UnrealEditor-Cmd.exe` 退出码 3，日志只有 SDK 校验输出，编辑器未真正启动。测试结果**未知**。

```bash
G:/GitHub/Dev02/UnrealEngine-5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe \
  "D:/Fab/Meta/VTSH_LASE_Dev/VTSH_ShaderLib_Test/VTSH_ShaderDev_Test/DevKit.uproject" \
  -ExecCmds="Automation RunTests DevKit;Quit" \
  -unattended -nopause -nosplash -testexit="Automation Test Queue Empty" -log
```
建议改为在编辑器内用 Session Frontend 手动跑一次 `DevKit.*`。

### 2. PIE 冒烟测试
- 新开存档能正常进游戏（Start 按钮 → slot 0）
- 读取**旧存档**不报错（`TutorialState` / `FirstRunTutorialStage` 字段已删除，UE tagged-property 会静默丢弃，理论安全）
- 主城 → 战斗房 → 清场 → 掉落 → 选符文 → 传送门 → 下一关 全流程
- UI 各界面能正常打开（验证 `EYogUIScreenId` 占位值没有移位）

### 3. Content 清理（我只动了 C++，Content 未碰）

**孤立资产，需在编辑器内删除：**
```
Content/Code/Core/System/B_TutorialMobSpawner.uasset          (0 处关卡放置，确认安全)
Content/UI/Playtest_UI/Tutorial/WBP_TutorialPopup.uasset
Content/Docs/UI/Tutorial/                                     (整个目录，10 个 DA_Tutorial_* + DA_TutorialRegistry)
Content/Docs/Map/LevelEvent/LFA_BackpackInfoGuid.uasset
Content/Docs/Map/LevelEvent/LFA_RuneInfoGuid.uasset
Content/Docs/Map/LevelEvent/LFA_WeaponInfoGuid.uasset
Content/Story/Flows/Tutorial/Generated/FirstRun/              (5 个 FA_EG_FirstRun_*)
Content/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/
Content/Story/EncounterPoints/Tutorial/FirstRun/
Content/Story/Rules/SR_FirstRun.uasset
```

**仍有入边引用，需手动解绑：**
- `Content/UI/HB_PlayerMain.uasset` — `TutorialPopupClass` 指向 WBP_TutorialPopup
- `Content/UI/BP_YogHUD.uasset` — 引用 DA_TutorialRegistry
- `Content/UI/DA_YogUIRegistry.uasset` — TutorialPopup 行

**Commandlet 已删，但它们写进 Content 的改动不会自动回滚：**
- `FirstRunTutorialRoomPoolSetupCommandlet` 曾改写 **5 个共享 RoomDataAsset** 的 `PortalDestinations`（Hub→01a→01b→WaterDungeon→PrayerRoom 路线）。这些改动仍在资产里，若不想要需手动改回。
- `FirstRunLoadingScreenSetupCommandlet` 曾把教程贴图绑进**共享的** `UI_LoadingScreen` 控件。

**不要删**：`SNode_ActivateTutorialSpawner` 及其 `FA_ActivateTutorialDummySpawner.uasset` — 名字带 Tutorial 但是通用 spawner 节点。

---

## 三、未开始：原计划 Phase 3-5（真正的重构）

> ⚠️ **行号随 Phase 2 再次位移**（文件现为 4172 行）。下面是**Phase 2 完成后重新定位的行号**。

### Phase 2 — 删除死代码 ✅ 已完成（2026-10-09）

实际删除内容（`YogGameMode.cpp` 4375 → **4172**，`YogGameMode.h` 736 → **674**）：

| 目标 | 处理 |
|---|---|
| 旧刷怪计时器簇 | 删除 `StartSpawnTimer` / `SpawnMob` / `TriggerImmediateSpawn` / `SomeEventThatTriggersImmediateSpawn` 四个函数体，及头文件 `FSpawnConfig` 结构、`SpawnConfig` / `SpawnTimerHandle` / `Current_CallCount` / `OnMapClean` 成员、`FCleanAllMobInMap` / `FSpawnMobStart` / `FSpawnMobFinish` / `FOnMapClean` 四个未用 delegate 声明 |
| `ConfirmArrangementAndTransition` 不可达分支 | 删除 `FName NextLevelName;` 及其后整段。函数保留（`LootSelectionWidget.cpp` 仍在调），现在只做 phase 切换 + 锁背包 |
| 临时 finisher 计数块 | 删除 `EnterArrangementPhase` 内的计数 `if` 块与下游 `bRefreshTemporaryFinisherLockView` 的 `RefreshDeckView()` 调用；同时删掉已无引用的 `bCountCombatClearsForTemporaryFinisherUnlock`，并移除 `Combat/FinisherDeprecation.h` include |
| `RemainKillCount` 兜底分支 | 删除 `UpdateFinishLevel` 末尾的兜底分支及 `RemainKillCount` 成员。`OnFinishLevel` / `FinishLevelEvent` 仍由 `CheckLevelComplete` 广播，`YogSaveSubsystem` 的订阅不受影响 |

**行为变更**：未配置 `CampaignData` 的关卡里，原先 `MonsterKillCount >= RemainKillCount`（0 >= 0）会在**第一次击杀**就触发 `EnterArrangementPhase`。该路径已删除，这类关卡现在不会自动进入整理阶段 —— 这是修复，不是回归。

**删除前的验证**：`Source/` 全量 grep 零调用者；`Content/` 二进制扫描仅 `B_GameMode.uasset` 命中 `SpawnConfig`（CDO tagged property，无 BP 图调用，加载时静默丢弃）。`StartSpawnTimer`（BlueprintCallable）与 `OnMapClean`（BlueprintAssignable）在 Content 内零命中。

保留 `CompletedCombatBattleCount` 字段本身（`Cheater.cpp` / `FRunState` / 存档在用）。

### Phase 3 — 去重（约 200 行）

1. **RunState 捕获逻辑复制了 2 份** → 抽成一个 helper
   - `:759` (`EnterArrangementPhase`)
   - `:3419` (`TransitionToLevel` — **这份是超集**，额外存了 heat-carry tag 和 `SavedCharacterClass`)
   → 新建 `GameModes/YogRunStateCapture.h/.cpp`，`FRunState CaptureRunStateFromPlayer(APlayerCharacterBase*, int32 CompletedCombatBattleCount)`，取超集行为。**这是整个文件里单笔收益最高的改动。**
   （原第三份在 `ConfirmArrangementAndTransition` 死分支内，已随 Phase 2 删除）

2. **传送门封闭循环复制了 4 份** → 已有 file-static `SealPortalsExcept` (`:107`)，但 `StartLevelSpawning` 里四个分支各自手写了一遍 `GetAllActorsOfClass(APortal)` 循环：`:1620` / `:1662` / `:1716` / `:1826`。泛化成 `SealPortalsNotInDestinations(UWorld*, const TArray<FPortalDestConfig>&)` 复用。

3. **难度档位三元表达式重复** — `:1769-1771` 和 `:1887-1888` 复制了已存在的 `AYogGameMode::ResolveTier` (`:3753`) 的逻辑。改为直接调用。
   （顺带：`ResolveTier` 是唯一没有测试覆盖的 pure static，建议补一个）

### Phase 4 — 引入 `FActiveRoomContext`，拆分 `StartLevelSpawning`

**这是整个重构的架构关键点，Phase 5 依赖它。**

问题：`ActiveRoomData` 被每个功能簇读取；`ActiveGoldMin` / `ActiveGoldMax` / `ActiveBuffCount` 三个成员存在的唯一目的，是把返回值从 `StartLevelSpawning` 偷渡到 `EnterArrangementPhase`（跨越整个战斗时长）。不先解决这个，抽组件只是把共享可变状态换个地方放。

新建 `Source/DevKit/Public/GameModes/ActiveRoomContext.h`：
```cpp
USTRUCT()
struct FActiveRoomContext
{
    GENERATED_BODY()

    UPROPERTY() TObjectPtr<URoomDataAsset> RoomData = nullptr;
    int32 FloorIndex = 1;
    int32 GoldMin = 10;
    int32 GoldMax = 20;
    int32 BuffCount = 1;
    TArray<FBuffEntry> RoomBuffs;
    FGameplayTag GlobalStageTag;
    FGameplayTagContainer StoryEventTags;

    bool bHasRewardOptionsOverride = false;
    TArray<FLootOption> RewardOptionsOverride;
    bool bHasForcedPortalOverride = false;
    int32 ForcedPortalOverrideIndex = 0;
    bool bSuppressRoomClearRewardPickup = false;
};
```
吸收：`ActiveRoomData`、`ActiveRoomBuffs`、`ActiveGoldMin/Max`、`ActiveBuffCount`、`ActiveGlobalStageTag`、`ActiveStoryEventTags`，以及四个 story-override 成员。GameMode 持有一份，暴露 `const FActiveRoomContext& GetRoomContext() const`，只有 `UCampaignFlowComponent` 可写。

然后把 `StartLevelSpawning` (`:1374` 起，约 450 行) 拆成：
`ResetPerRoomState()` → `ResolveRoomForFloor()` → `DispatchCampaignStage()` → 四选一的 `EnterHubRoom()` / `EnterShopRoom()` / `EnterEventRoom()` / `EnterCombatRoom()`
（这四个分支本来就互斥、各自以 early `return` 结尾，拆起来很自然）

### Phase 5 — 抽 7 个组件（按风险升序）

全部放 `Source/DevKit/*/GameModes/Components/`，继承 `UActorComponent`，跟随项目既有惯例（`Component/` 下已有 28 个组件）。GameMode 本身是 `AModularGameMode`，之后可以用 GameFeature 注入。

| 顺序 | 组件 | 约行数 | 当前锚点 / 备注 |
|---|---|---|---|
| 1 | `UEnemyRegistryComponent` | 130 | `:3913` 起 + `AliveEnemies` + `OnBossRegisteredNative`。**零共享状态，12 个外部调用者（相机/AI/HUD/spawner），纯机械重指向。先做这个验证模式。** |
| 2 | `UGameOverComponent` | 300 | `:1114` 起 + 3 个 revive static + 死亡调参 UPROPERTY。把 `PlayerDeathReviveTests.cpp` 一起挪过去 |
| 3 | `URoomFixtureComponent` | 200 | `:926` 起（商店 + 祭坛生成）。只读 `RoomContext.RoomData` |
| 4 | `UWaveSpawnComponent` | 1100 | 波次算法 + 约 320 行 file-static enemy-rune helper + 波次状态/计时器。**注意**：`HandleLifecycleEnemySpawned/Failed` 必须保留为 public BuffFlow 回调 —— `BFNode_SpawnEnemyFromContext.cpp` 通过 `GetAuthGameMode` 调它，GameMode 要转发给组件 |
| 5 | `ULootRewardComponent` | 400 | 战利品生成 + `SelectLoot` + `FindLootSpawnLocation` + reward pickup 生成 + `CurrentLootOptions` / `FallbackLootPool`。`LootSelectionWidget.cpp` 直接读 `CurrentLootOptions`，需重指向 |
| 6 | `UPortalFlowComponent` | 400 | `:3645` `ActivateHubPortals` / `:3761` `ActivatePortals` / `:3380` `TransitionToLevel`。两个 Activate 函数高度相似，合并出一个 `TryOpenPortal()`。`Portal.cpp:518` 是 `TransitionToLevel` 唯一调用者 |
| 7 | `UCampaignFlowComponent` | 350 | 持有 `FActiveRoomContext`。`ResolveActiveCampaignData` / `RollRoomTypeForFloor` / `SelectRoomByTag` / `SelectRoomBuffs` / `ResolveTier` / `CurrentFloor`。**风险最高，最后做**，等其它簇都改成走 `GetRoomContext()` 之后 |

**留在 `AYogGameMode` 上**：`BeginPlay`/`StartPlay`/`PostLogin`/`RestartPlayer`、`CurrentPhase` + `OnPhaseChanged`、`EnterArrangementPhase`、`ConfirmArrangementAndTransition`、`TriggerLifecycleEvent`/`RunStoryLevelFlow` + `LifecycleFlowComponent`、HUD 绑定、story-override setter（`StoryEncounterRuntimeSubsystem` 和 2 个 StoryFlow 节点在用）、6 个有测试覆盖的 static。

**目标**：`YogGameMode.cpp` 收敛到约 900 行。

---

## 四、"什么进脚本、什么留 C++"（原始问题的回答）

### 留在 C++ — 不变式重、确定性要求高、或在热路径上
- 波次预算算法（`GenerateWavePlans` / `BuildWavePlan`）：预算、按类型上限、整关上限的多约束分配
- 敌人注册表与空间查询：相机和 AI 每帧调用
- RunState 序列化：正确性关键，且要兼容存档
- 阶段状态机（`CurrentPhase`）、传送门洗牌/选择、战利品 RNG 与去重、难度档位阈值

### 应该进数据（DataAsset）— 目前硬编码在 C++ 里
- Hub story tag `Story.Area.Hub` / `Story.Event.Hub.FirstEntered` 目前是字符串字面量，应该放到 `UCampaignDataAsset` 上，跟已有的 `GlobalStageTag` / `StoryEventTags` 放一起
- Hub terminal class 路径 `/Game/Code/Core/Hub/BP_HubActiveSkillTerminal` 应该放到 `URoomDataAsset`，像已有的 `ShopActorClass` / 祭坛 class 字段一样
- （原本 5 条 first-run `/Game/...` 路径已随教程移除一起消失）

### 应该进脚本 —— **接口已经建好了，但一直没接线**

`LifecycleEventFlows`（`YogGameMode.h`）是一个 `TMap<EGameLifecycleEvent, ULevelFlowAsset*>`，接着一个真实的 `UFlowComponent`，`TriggerLifecycleEvent()` 和一次性去重都已实现。`LENode_*` 节点库支持阻塞等待（延时、弹窗关闭、选完战利品）。

**但它在所有 Content 资产里是空的 —— 0 条配置。** 头文件注释指向一个叫 `BP_GameMode_Default` 的资产，而那个资产早就改名成 `B_GameMode` 了。

也就是说：**处理一次性叙事节拍的脚本逃生口早就建好了，只是从来没接上。**

建议：不要再发明新的脚本系统。重构完成后：
- 把每个房间的叙事节拍走 `LifecycleEventFlows`
- 每只敌人的生成演出用 `UEnemyData` 上已经能用的 `USpawnLifecycleFlowAsset` 钩子

这两条都是现成可用的路径。属于后续工作，不在本次重构范围内。

---

## 五、验证命令

```bash
# 构建（必须用 G: 自定义引擎，官方版会在 UHT 阶段因 EStylizedCharacterLightMode 失败）
G:/GitHub/Dev02/UnrealEngine-5.8/Engine/Build/BatchFiles/Build.bat \
  DevKitEditor Win64 Development \
  -project="D:/Fab/Meta/VTSH_LASE_Dev/VTSH_ShaderLib_Test/VTSH_ShaderDev_Test/DevKit.uproject" -WaitMutex
```

**每个 Phase 做完都要构建一次，不要攒着一起编。**

分阶段验收：
- **Phase 2** ✅：构建已通过。唯一行为变化是无 `CampaignData` 关卡不再因首杀误触整理阶段（见第三节）
- **Phase 3**：PIE 走一次关卡切换，确认 HP / 金币 / 符文 / 卡组能跨传送门带过去（RunState 捕获是风险点）
- **Phase 4**：主城 / 商店 / 献祭事件 / 战斗 四种房型各 PIE 一次，确认传送门各自正常开启
- **Phase 5**：每抽完一个组件，PIE 完整战斗房：刷怪 → 清场 → 掉落 → 选取 → 传送门 → 下一关。相机战斗偏移必须仍能跟随敌人（验证 `UEnemyRegistryComponent`）

---

## 六、已知遗留问题

1. `StartNormalRunFromFrontend()` 直接选 slot 0，可能覆盖已有存档 —— 建议加冲突检测或改走选档界面
2. `bScriptedDefeatGameOver` 恒为 false，无写入方
3. 五个枚举里的 `Deprecated_*` 占位值在 Content 迁移完成前不能删
4. 自动化测试尚未成功运行过一次
5. Content 侧孤立资产未清理（见第二节）
