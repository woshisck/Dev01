## Codex 审查结果

### 发现的问题
- 【严重】Continue 与 Start 走同一逻辑：`YogGameInstanceBase.cpp:285-286` 都绑定到 `StartNewRunFromFrontend`，而该函数会 `ClearRunState()`（`340-350`），最终清除磁盘 `RunCheckpoint`（`747-760`）。这与方案中 Continue 应恢复 `RunCheckpoint.bIsValid`、中途退出依赖最近存档点的设计冲突。
- 【严重】`ReturnToMainMenu()` 也调用 `ClearRunState()`（`562-565`），会清除中途退出存档点；方案矩阵明确“退出游戏（中途）”不清除 `RunCheckpoint`。
- 【严重】存档点恢复链路未接通：`RestoreRunStateFromCheckpoint()` 仅定义（`YogSaveSubsystem.cpp:221-253`），未被 `SelectSlot()`、`LoadSaveGame()` 或前端 Continue 调用；`LoadSaveGame()` 目前只 `LoadPlayer()`（`370-373`）。
- 【严重】清关后存档点可能写入旧状态：`EnterArrangementPhase()` 在奖励前调用 `TriggerCheckpoint()`（`YogGameMode.cpp:422-425`），但 `TriggerCheckpoint()` 只序列化 `GI->PendingRunState`（`YogSaveSubsystem.cpp:185-219`）。当前战斗结束时的 HP/Gold/Phase/背包状态没有在此之前同步到 `PendingRunState`。
- 【严重】异步保存没有排队机制：`DoAsyncSave()` 在 `bAsyncSavePending` 为 true 时直接 return（`YogSaveSubsystem.cpp:294-297`），与方案“下一次 QuickSave 排队等前一次完成”不一致，可能丢掉最后一次修改。
- 【偏差】背包快速存档触发点不符合方案：方案要求 `BackpackGridComponent::OnGridChanged` + 0.5s 防抖；当前只在 `BackpackScreenWidget::NativeOnDeactivated()` 关闭界面时调用 `QuickSave()`（`1027-1046`），且 `YogSaveSubsystem.h` 没有 `QuickSaveDebounceTimer`。
- 【偏差】关卡入口存档点放在 `BeginPlay()`（`YogGameMode.cpp:367-375`），不是方案指定的 `StartLevelSpawning` 刷怪前；并且 `CurrentFloor` 在 `StartLevelSpawning()` 中才从 `PendingNextFloor` 更新（`1091-1099`），可能写入错误楼层。
- 【偏差】`FMetaCurrencyRow` 缺少方案要求的 `FSlateBrush Icon` 字段；当前只有 `CurrencyTag/DisplayName/ShortName/MaxCapacity`（`MetaTypes.h:97-109`）。
- 【遗漏】`YogMetaProgressionSubsystem` 未实现方案要求的 `AddMysticPoints()` 与 `GetAvailableMysticPoints()`；当前只有 `GetMysticSideLevel()`（`YogMetaProgressionSubsystem.h:80-82`）。
- 【偏差】`FRunCheckpointData` 仍直接保存 `FPlacedRune/FRuneInstance`（`YogSaveGame.h:358-362`），而这些内部仍可能含 `TObjectPtr` 资产引用；方案要求 UObject 指针拍平成软引用。

### 潜在风险
- `RestoreRunStateFromCheckpoint()` 使用 `LoadSynchronous()`（`YogSaveSubsystem.cpp:245-252`），没有方案要求的异步加载、`PendingLoadHandles`、加载失败降级与提示。
- `CommitSave()` 调用 `WriteSaveGame()`（`YogMetaProgressionSubsystem.cpp:79-84`），而 `WriteSaveGame()` 会顺带 `SavePlayer/SaveMap`（`YogSaveSubsystem.cpp:337-342`）；局外成长变更可能意外捕获局内旧数据。
- `ResetSlotForNewGame()` 只清 `MetaProgression/RunCheckpoint/Tutorial/ShownPopupKeys`（`YogSaveSubsystem.cpp:93-104`），旧的 `PlayerStateData`、`WeaponInstanceItems`、`MapStateData` 等仍保留，未来 Continue/LoadPlayer 接通后可能污染新局。
- `SelectSlot()` 写入 `LastActiveSlot`，但 `Initialize()` 只 `LoadSettings()`，没有按 `LastActiveSlot` 自动选择槽位；启动后默认空 `CurrentSaveGame` 可能不是上次槽位。
- `CanPurchaseNode()` 的前置检查只要求前置等级 >= 1（`YogMetaProgressionSubsystem.cpp:162-165`），如果设计期望“前置满级”，这里会提前解锁。
- 货币接口没有校验 `CurrencyTag` 是否存在于 `DT_MetaCurrencyRules`，也没有防御负成本数据；错误 DataTable 可能导致非法货币写入或购买时反向加钱。

### 改进建议
- 拆分 `StartNewRunFromFrontend` 与 Continue：新游戏走 `ResetSlotForNewGame()`，Continue 走 `SelectSlot()` + `RestoreRunStateFromCheckpoint()` + 加载主地图。
- 将“中途返回主菜单/退出”与“死亡/结局/新游戏”分成不同清理路径，避免普通退出调用 `ClearRunCheckpoint()`。
- 在清关后触发 `TriggerCheckpoint()` 前，先从当前 Player/Backpack/CombatDeck 构建最新 `FRunState`，再写入 `RunCheckpoint`。
- 把关卡入口 checkpoint 移到 `StartLevelSpawning()` 更新 `CurrentFloor` 之后、刷怪之前。
- 为 `DoAsyncSave()` 增加 dirty/queued 标记：保存中收到新请求时记录待保存，完成回调后再保存一次最新对象。
- 按方案把背包存档接到 `BackpackGridComponent::OnGridChanged`，使用 0.5s 防抖；关闭界面可作为兜底保存。
- 为 `FRunCheckpointData` 中的符文/献祭相关数据设计真正的可序列化软引用快照，避免直接保存含 `TObjectPtr` 的运行时结构。
- 补齐 `AddMysticPoints()`、`GetAvailableMysticPoints()`，明确神秘点、已花费点、`MysticSideLevel` 三者关系。

### 需要向用户确认的问题
- 背包快速存档是否允许从“每次格子变动防抖保存”改为“关闭背包时保存”？如果不允许，当前实现需要调整。
- 神秘侧前置节点的条件是“前置至少 1 级”还是“前置满级”？当前代码按至少 1 级处理。
- 本次审查范围未包含 `DefaultGameplayTags.ini` 和两个 DataTable 资产；是否需要继续核对 9 种局外货币、`MysticPoint` 与初始节点数据是否完整？