## Codex 审查结果

### 发现的问题
- `current_plan.md:40` 计划复用 `LENode_*`，但现有 `ULENode_Base` 通过 `AllowedAssetClasses = { ULevelFlowAsset::StaticClass() }` 限定只能用于 `ULevelFlowAsset`。迁移到 `UStoryFlowAsset` 后，`LENode_Delay`、`LENode_ShowTutorial`、`LENode_SpawnRewardPickup`、`LENode_ActivateTutorialSpawner` 等默认不可用。
- `current_plan.md:165` 对节点过滤的判断与现状相反：`LENode_*` 不会自然出现在 Story FA；反而 `UBFNode_Base` 只排除了 `ULevelFlowAsset`，所以 BuffFlow 节点会出现在 `UStoryFlowAsset` 中。
- `USNode_ShowTutorialPopup` 方案自相矛盾：`current_plan.md:79-83` 写阻塞等待关闭，`current_plan.md:172` 又确认非阻塞立即 Out。实现路径和 Pin 语义必须二选一。
- `USNode_EnablePortal` 仅调用 `APortal::EnablePortal()` 不足以让传送门可进入。当前 `APortal::Open()` 才会设置 `bIsOpen`、`SelectedLevel`、`SelectedRoom` 等状态；`EnablePortal_Implementation()` 主要只是应用表现。
- `USNode_RecordProgress` 文档里的 tag 规则写成 `Story.Progress.{EncounterId}.{ProgressKey}`，但现有 `MakeProgressTagName()` 实际生成 `Story.Encounter.Progress.{EncounterId}.{ProgressKey}`。如果资产或说明按前者配置，会导致进度条件不匹配。
- `RunFlowViaProxy()` 当前代码里已经是 `UFlowAsset*`，计划中“从 `ULevelFlowAsset*` 改为 `UFlowAsset*`”属于过期步骤；但测试里仍有 `NewObject<UFlowAsset>` 赋给 `NodeEventFlow`，改成 `UStoryFlowAsset` 后需要同步更新。
- `USNode_GiveCard` 的 Phase B 只写入战斗卡组，已确认决策又要求同时写入背包。两处不一致，且需要定义卡组添加失败但背包添加成功时的处理策略。

### 潜在风险
- 旧 `FA_DummyDeath_DropHeavyCard` 是 `ULevelFlowAsset`，不能简单在同一路径“覆盖”为 `UStoryFlowAsset`；通常需要删除旧资产、换路径，或写明确的资产迁移逻辑。
- 绕过 `StoryEncounterRuntimeSubsystem::ExecuteEncounterAction()` 后，现有输入设备文案变体解析（键鼠/手柄）不会自动生效，ShowHint / Quest 类节点可能丢失当前 DA 行为。
- “覆盖第一章教程所有操作”的节点清单不足：现有动作还包括 `UnlockFeature`、`SetQuestObjective`、`SetActorEnabled`、`SetRoomRewardOverride`、`SetPortalOverride`、`SpawnRewardPickup` 等。
- 只新增 `UStoryFlowAsset` 运行时类，未新增 `StoryFlowAssetFactory` 时，内容浏览器创建入口和图初始化体验可能不如现有 `Level Event Flow`。
- `MakeProgressTag()` 使用未注册 tag 时会返回 invalid；节点需要日志或 Failed 分支，否则设计侧很难定位配置错误。

### 改进建议
- 明确 Story FA 节点过滤策略：`USNode_Base` 设置 `AllowedAssetClasses = { UStoryFlowAsset::StaticClass() }`；同时更新 `UBFNode_Base` 排除 `UStoryFlowAsset`。
- 若要复用 `LENode_*`，把 `ULENode_Base` 的允许列表扩展到 `UStoryFlowAsset`；若不想混用，则补齐对应 `USNode_*`。
- 将 `USNode_ShowTutorialPopup` 固定为非阻塞或阻塞，并同步修改字段、Pin 名称、测试和文档。
- 把 `USNode_EnablePortal` 改成调用 GameMode/Portal 的完整开启流程，或重命名为仅表现用途的节点，避免误以为可进入。
- 增加迁移影响范围：更新自动化测试、DummyDeath commandlet、资产创建工厂、编辑器筛选/校验逻辑。
- 为 Story 节点补充最小自动化测试：节点过滤、RecordProgress tag、GiveCard 成败分支、Portal 开启语义、TutorialPopup Out 时机。

### 需要向用户确认的问题
- `USNode_ShowTutorialPopup` 最终要非阻塞立即 Out，还是等待玩家关闭弹窗后 Out？
- Story FA 中是否允许复用 `LENode_*`，还是所有剧情节点都必须是 `USNode_*`？
- `EnablePortal` 的目标是只打开表现，还是生成可交互、可进门的完整传送门？
- `GiveCard` 是否要求卡组和背包同时成功才算成功？其中一步失败时是否回滚？
- 本阶段是否要完全迁移 `Actions[]`，还是仅把 `NodeEventFlow` 类型改为 `UStoryFlowAsset` 并保留旧 DA Actions？

### 需要手动创建的引擎资产
- `UStoryFlowAsset` 类型的 Story FA 资产：按第一章/教程每个需要迁移的 `NodeEventFlow` 创建或迁移。
- `FA_DummyDeath_DropHeavyCard`：若 commandlet 不能安全删除并重建旧 `ULevelFlowAsset`，需手动删除旧资产后重建为 `UStoryFlowAsset`。
- 对应 `UStoryEncounterPointDA` 的 `NodeEventFlow` 引用：需要指向新的 Story FA 资产。
- 无新增蓝图、DT、材质需求。