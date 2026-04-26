# 03 关卡循环 — 已完成功能盘点

> 范围：主循环 / 波次 / 传送门 / 存档 / 三选一奖励 / Buff 池 / LevelFlow 编排 / 关卡过场。
> UI 表现层（三选一卡 / 关卡揭幕 WBP 配置）→ [06_UI_HUD.md](06_UI_HUD.md)；相机过场动画 → [05_FeedbackLayer.md](05_FeedbackLayer.md)。

---

## 主循环骨架

### [LOOP-001] 主循环：波次刷怪 + 触发条件 + 兜底池
- **设计需求**：Roguelite 房间内"打波次→清场→选符文→进门"的核心循环；触发条件可配（全清 / 50%死亡 / 20%死亡 / 计时）；ActiveRoomData 缺失时用兜底池防卡关。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/GameModes/YogGameMode.h` + `Private/GameModes/YogGameMode.cpp`（`StartLevelSpawning` / `CheckWaveTrigger` / `EnterArrangementPhase` / `CheckLevelComplete` / `FallbackLootPool`）
  - `Public/Mob/MobSpawner.h` + `Public/Mob/MobSpawnConfig.h`
- **设计文档**：[LevelSystem_ConfigGuide](../Systems/Level/LevelSystem_ConfigGuide.md) · [LevelSystem_ProgrammerDoc](../Systems/Level/LevelSystem_ProgrammerDoc.md)
- **验收方式**：
  1. 进关应自动开始第一波刷怪；清场后 `CurrentPhase` 应切到 Arrangement
  2. ActiveRoomData 为空时 `GenerateIndependentLootOptions` 应从 FallbackLootPool 抽

### [BUFF-POOL] 关卡 Buff 奖励池
- **设计需求**：不同房间从不同池抽 Buff，按 FloorConfig 难度档位选择高低池。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Data/BuffDataAsset.h`（`BuffPool` 字段）
  - `Public/Data/RoomDataAsset.h`（`URoomDataAsset` — 关联到 BuffDataAsset）
  - `Public/GameModes/YogGameMode.h` + `.cpp`（`ResolveTier(Room, TotalScore, LowMax, HighMin)` 静态方法）
- **设计文档**：[BuffPool_ConfigGuide](../Systems/Level/BuffPool_ConfigGuide.md)
- **验收方式**：低难度档房间应抽到 LowPool 内的 Buff，高难度应抽到 HighPool

### [FIX-027] 背包战斗锁定改为 ELevelPhase 驱动
- **设计需求**：进入关卡到第一波刷怪前不能开背包 — 旧实现用 `HasAliveEnemies()`，进关到刷怪前窗口可操作，是 bug。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/UI/BackpackScreenWidget.cpp`（`IsInCombatPhase()` 改用 `GM->CurrentPhase == ELevelPhase::Combat`）
  - `Private/GameModes/YogGameMode.cpp`（`StartLevelSpawning` 对 `bIsHubRoom` 直接写 Arrangement）
- **验收方式**：进关后立刻按 Tab 应不能开背包；清场后应能开；进主城应能开

---

## 传送门系统

### [LEVEL-001 / MAP-001] 传送门切关基础（Portal + DA_Campaign）
- **设计需求**：多分支选关；门到门的目标可配；门有 Closed / NeverOpen / Destination 三种美术。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/Map/Portal.h` + `Private/Map/Portal.cpp`（`EnablePortal` / `DisablePortal` / `NeverOpen` / `Open(LevelName, RoomDA)` / `DestinationArtMap`）
  - `Public/Data/CampaignDataAsset.h`（`PortalDestinations[].PortalIndex`）
- **设计文档**：[Portal_ConfigGuide](../Systems/Level/Portal_ConfigGuide.md)
- **验收方式**：放置 Portal 设 Index=0，DA_Campaign 配对应 PortalDestinations[0]，关卡结算应自动开门并切美术

### [LEVEL-006] 传送门 v3：按 E 进入 + HUD 双层预览 + 渐黑过场
- **设计需求**：把"踩进就走"改为"按 E 主动选下一关"；屏幕外门画方位箭头；屏幕内门显示单例浮窗（房间名 / 类型徽章 / 已确定 Buff / 战利品摘要）；按 E 触发渐黑过场后切关。
- **状态**：✅ C++完成；⚙ WBP_PortalPreview / WBP_PortalDirection 待按规格搭建并赋到 BP_YogHUD
- **核心文件**：
  - `Public/Map/Portal.h` + `Private/Map/Portal.cpp`（`HandlePlayerEnterRange` / `TryEnter` / `FinishEntry` / `AbortEntry` / `PreRolledBuffs`）
  - `Public/UI/PortalPreviewWidget.h` + `Private/UI/PortalPreviewWidget.cpp`（新）
  - `Public/UI/PortalDirectionWidget.h` + `Private/UI/PortalDirectionWidget.cpp`（新）
  - `Public/UI/YogHUD.h` + `Private/UI/YogHUD.cpp`（`TickPortalPreview` / `BeginBlackoutFade` / `EndBlackoutFade` / `ShowPortalGuidance`）
  - `Public/Character/PlayerCharacterBase.h`（`PendingPortal`）
  - `Public/System/YogGameInstanceBase.h`（`PendingRoomBuffs` / `bPlayLevelIntroFadeIn`）
  - `Public/Data/RoomDataAsset.h`（`DisplayName : FText`）
- **设计文档**：[Portal_ConfigGuide](../Systems/Level/Portal_ConfigGuide.md) · [WBP_PortalPreview_Layout](../Systems/Level/WBP_PortalPreview_Layout.md) · [WBP_PortalDirection_Layout](../Systems/Level/WBP_PortalDirection_Layout.md)
- **验收方式**：
  1. 关卡结算后非主城关，屏幕外门应画方位箭头；进入门 Box 应弹浮窗 + "按 E 进入"
  2. 按 E 应渐黑 → 切关 → 下一关淡入
  3. 主城（HubRoom）应不显示任何浮窗 / 箭头

---

## 存档与跨关状态

### [SAVE-001] 跨关持久化 YogSaveSubsystem（FRunState）
- **设计需求**：HP / 金币 / 背包符文 / 热度 / 献祭恩赐都要切关保留；中断后能从"当前房间起点"继续。
- **状态**：✅ C++完成（含 [FIX-024] 热度恢复 / [FIX-026] SacrificeGrace 恢复）
- **核心文件**：
  - `Public/System/YogGameInstanceBase.h`（`FRunState`：HP / Gold / 背包 / `CurrentHeat` / `ActiveSacrificeGrace` / `PendingRoomData` / `PendingRoomBuffs` / `PendingNextFloor`）
  - `Public/SaveGame/YogSaveSubsystem.h` + `Private/SaveGame/YogSaveSubsystem.cpp`
  - `Public/SaveGame/YogSaveGame.h`（`TutorialState`）
  - `Private/Character/PlayerCharacterBase.cpp`（`RestoreRunStateFromGI` 跨阶热度恢复）
  - `Private/GameModes/YogGameMode.cpp`（`ConfirmArrangementAndTransition` / Portal `TransitionToLevel` 两处保存）
- **设计文档**：[CrossLevelState_Technical](../Systems/Level/CrossLevelState_Technical.md)
- **验收方式**：
  1. 拾取符文 / 升 Phase / 接受献祭 → 退出游戏 → 重启 → 状态应一致
  2. 携带余烬切关 → 下一关 BeginPlay 应仍是 Phase 1（FIX-024 修复点）

---

## 关卡内事件编排（LevelFlow）

### [LFLOW-001 / FEAT-012] LevelFlow 时间轴节点（独立于 BuffFlow）
- **设计需求**：关卡内事件序列 / 引导要能可视化编排；LevelFlow 节点与 BuffFlow 节点在编辑器内互不可见，避免误用。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/LevelFlow/LevelFlowAsset.h`（`ULevelFlowAsset`）
  - `Public/LevelFlow/LevelEventTrigger.h` + `Private/LevelFlow/LevelEventTrigger.cpp`（`ALevelEventTrigger` Box 触发器）
  - `Public/LevelFlow/Nodes/LENode_Base.h`（`AllowedAssetClasses = {ULevelFlowAsset}`）
  - `Public/LevelFlow/Nodes/LENode_TimeDilation.h` / `LENode_Delay.h`（FTSTicker 真实时间）/ `LENode_ShowTutorial.h` / `LENode_ShowInfoPopup.h`
- **设计文档**：[EditorSetup_LevelFlow_Tutorial_WeaponSpawner](../TODO/EditorSetup_LevelFlow_Tutorial_WeaponSpawner.md)
- **验收方式**：右键创建 Data Asset → Level Event Flow，Flow 编辑器内右键应只显示 LevelEvent 分类节点

### [FEAT-030] LENode_ShowTutorial bPauseGame 选项
- **设计需求**：信息浮窗有时不需要暂停游戏（如纯提示性"获得新符文"）。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/LevelFlow/Nodes/LENode_ShowTutorial.h` + `.cpp`
  - `Public/UI/GameDialogWidget.h` + `.cpp`（`ShowPopup(Pages, bPauseGame=true)` 向后兼容）
  - `Public/Tutorial/TutorialManager.h` + `.cpp`（`ShowByEventID(EventID, PC, bPauseGame=true)`）
- **验收方式**：LevelFlow DA 中 Show Tutorial Popup 节点取消勾选 Pause Game，弹窗期间游戏应继续运行

### [FEAT-031] LENode_WaitForLootSelected
- **设计需求**：LevelFlow 编排里要能等玩家选完三选一再继续后续节点。
- **状态**：✅ C++完成
- **核心文件**：
  - `Public/LevelFlow/Nodes/LENode_WaitForLootSelected.h` + `.cpp`
  - `Public/GameModes/YogGameMode.h`（`OnLootSelected` 委托，在 `SelectLoot()` 末尾广播）
- **验收方式**：连法 `Start → WaitForLootSelected → ShowTutorial → End`，玩家选符文后 Tutorial 才弹出

---

## 三选一奖励

### [COMBAT-003a] 三选一奖励基础（GenerateIndependentLootOptions + RewardPickup）
> （ID 拆分说明：原 `COMBAT-003` 同时表示三选一与武器系统，此处取三选一部分；武器部分见 [04_Weapons.md](04_Weapons.md) 的 [COMBAT-003b]）
- **设计需求**：击杀掉 RewardPickup → 拾取后弹三选一卡 → 选完自动进背包；多个拾取物互不干扰。
- **状态**：✅ C++完成（含 [FIX-022] 多拾取物独立 / [FIX-023] CommonUI 隔离）
- **核心文件**：
  - `Public/Map/RewardPickup.h` + `Private/Map/RewardPickup.cpp`
  - `Public/GameModes/YogGameMode.h` + `.cpp`（`GenerateIndependentLootOptions()` / `OnLootSelected` 委托）
  - `Public/UI/LootSelectionWidget.h` + `Private/UI/LootSelectionWidget.cpp`（CommonUI 完全隔离：`GetDesiredInputConfig` 返回空 / `NativeOnDeactivated` 不调 Super / `NativeOnFocusLost` 不调 Super）
- **设计文档**：[LootSelection_Technical](../Systems/UI/LootSelection_Technical.md)
- **验收方式**：
  1. 击杀 boss 应掉 RewardPickup；走过去拾取应弹三选一卡
  2. 多个 RewardPickup 同时存在，每个都应独立触发自己的三选一
  3. 选完应自动 OpenBackpack（UI-014）

### [FEAT-025 副] Loot 8 方向最佳落点算法
- **设计需求**：掉落物落到玩家可见 + 无碰撞的位置 — 别掉墙后 / 楼上 / 屏幕外。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/GameModes/YogGameMode.cpp`（`FindLootSpawnLocation`：8 方向候选 → 无碰撞 + 相机可见 + 屏幕内 5% 边缘余量 → 全失败回退玩家位）
- **验收方式**：在贴墙 / 角落处击杀，掉落物应出现在玩家可见的合理位置

---

## 关卡过场效果

### [MAP-002 / FEAT-023] 关卡结束揭幕特效（时间膨胀 + 渐黑 + 圆形揭幕）
- **设计需求**：关卡完成有仪式感的过场 — 时间膨胀 → 渐黑 → 围绕 Loot 圆心扩散揭幕。
- **状态**：✅ C++完成；⚙ WBP_LevelEndReveal + DA_LevelEndEffect 待配（LEVEL-END-CONFIG）
- **核心文件**：
  - `Public/UI/LevelEndRevealWidget.h` + `Private/UI/LevelEndRevealWidget.cpp`
  - `Public/Data/LevelEndEffectDA.h`（`SlowMoScale` / `SlowMoDuration` / `BlackoutFadeDuration` / `RevealDuration` / `RevealEdgeSharpness` / `RevealMaterial`）
  - `Public/UI/YogHUD.h` + `Private/UI/YogHUD.cpp`（`TriggerLevelEndEffect(LootWorldPos)`）
- **验收方式**：关卡 boss 掉 Loot 时应触发：变慢 → 渐黑 → Loot 位置圆形揭幕扩散

### [MAP-003 / FEAT-024] 开场镜头标记（LevelIntroCameraMarker）
- **设计需求**：关卡加载后切到标记处视角 → 停留几秒 → 平滑回玩家，制造开场镜头感。
- **状态**：✅ C++完成；⚙ 关卡放置 Actor 并从 LevelFlow 调 `TriggerIntro()` 待做
- **核心文件**：
  - `Public/Map/LevelIntroCameraMarker.h` + `Private/Map/LevelIntroCameraMarker.cpp`
- **验收方式**：关卡放 Actor → BeginPlay 调 TriggerIntro → 应切到标记视角停留 2s 再 1.5s 平滑回玩家

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| LEVEL-006 WBP（A）| 创建 `WBP_PortalPreview`（父类 `UPortalPreviewWidget`）按 Layout 规格搭建 | [WBP_PortalPreview_Layout](../Systems/Level/WBP_PortalPreview_Layout.md) |
| LEVEL-006 WBP（B）| 创建 `WBP_PortalDirection`（父类 `UPortalDirectionWidget`，根 RootCanvas + ArrowTexture 字段）| [WBP_PortalDirection_Layout](../Systems/Level/WBP_PortalDirection_Layout.md) |
| LEVEL-006 HUD | `BP_YogHUD` Details 填 `PortalPreviewClass` / `PortalDirectionClass` | 同上 |
| LEVEL-END-CONFIG | 创建 `WBP_LevelEndReveal`（全屏 Image 命名 RevealImage）+ `DA_LevelEndEffect` + 配置 BP_YogHUD | [TASKS](../PM/TASKS.md) |
| LEVEL-INTRO-CONFIG | 关卡中放 `ALevelIntroCameraMarker` + 从 LevelFlow 或 BeginPlay 调 `TriggerIntro()` | 同上 |
