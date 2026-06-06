> 状态：归档。仅用于历史追溯，不作为当前实现依据。

## Codex 审查结果

### 发现的问题
- `current_plan.md:52` 和 `current_plan.md:81` 写成 Run / Session / Map 标志位，但代码里的 `EStoryFlagScope` 实际是 `Save / Run / Session`，Map 只存在于规则触发策略 `OncePerMap`，不是 StoryFlag 作用域。
- `current_plan.md:47` 写 DataTable 在 GameInstance BP 中赋值，但当前实现从 `UMetaProgressionSettings` / `Config/DefaultGame.ini` 加载 `MetaUpgradeNodeTable` 与 `MetaCurrencyRuleTable`。
- `current_plan.md:73-83` 的“已实现导演接口”只列了导演向外调用的少量接口，遗漏了导演自身被外部系统调用的入口，如 `HandleArrangementPhase`、`HandleRewardRuneAdded`、`HandleSacrificeConfirmed`、`HandleScriptedDefeatDeath`、`HandleFirstBackpackOpened` 等。
- `current_plan.md:79-83` 对 StoryEngine / Save / GameMode 的副作用写得过粗。例如教程死亡流程还会调用 `ClearRunCheckpoint()` 与 `GI->ClearRunState()`，只写 `MarkFirstRunTutorialCompleted()` 会低估状态清理影响。
- `current_plan.md:89` 提议 `GameMode::OverrideNextSpawnWave()`，但本轮原则写的是“只记录现有事实，不设计未来”。该项应明确标为“候选接口/未实现”，否则和设计原则冲突。
- `current_plan.md:90` 写“无对话系统”不够准确：项目已有 `GameDialogWidget`、`DialogContentDA`、教程弹窗/提示与 StoryEncounter 的 `Dialogue` action 映射；缺的是“实时对话气泡/完整对话流程”，不是完全没有对话相关系统。
- `current_plan.md:109-116` 写“新建 6 个文件”，但当前工作区中这些目标路径已经存在。若继续执行，应改为审校/更新现有文档，避免覆盖未跟踪或未提交内容。

### 潜在风险
- 接口汇总如果只扫 `Public/*.h`，会遗漏 `Private` 中实际调用链、StoryFlow/SNode 节点、UI 触发点和蓝图资产里的调用。
- 新建 `Docs/04_开发实现与系统文档/系统/*` 与既有 `Docs/04_开发实现与系统文档/系统/Story/*` 可能形成双份 StoryEngine 文档，后续容易漂移。
- `StartForcedSurvivalEncounter()` 当前依赖 `ActiveRoomData` 或现有 `WavePlans` 选择敌人，并不等同于“指定特定遭遇战”；文档若不说明限制，后续接口设计会被误判。
- “Portal 上加 `SetLocked(bool)` 即可”低估了传送门状态语义：需要明确碰撞、视觉、HUD 预览、`TryEnter` 防重入、`DisablePortal/NeverOpen` 的关系。
- `DocConventions.md` 只靠人工约束，缺少检查手段，文档仍可能很快过时。

### 改进建议
- 将 `DirectorInterfaces.md` 拆成三类：导演主动调用的系统接口、外部系统调用导演的入口、StoryFlow/SNode 可用动作节点。
- 每个接口补充来源文件、是否 `BlueprintCallable/Pure`、参数、返回值、主要副作用、持久化行为、调用时机限制。
- 修正 StoryFlag 作用域为 `Save / Run / Session`，另起一节说明 `OncePerMap` 是规则触发策略，不是标志位作用域。
- 对“当前缺口分析”统一标注状态：`已实现`、`已有替代方案`、`未实现候选接口`、`需系统设计`。
- 在 `SystemDependencyMap.md` 中同时画调用方向和持久化方向，至少覆盖 Director、StoryEngine、SaveSubsystem、MetaProgression、GameMode、GameInstance、Portal/UI。
- 为文档规范增加最低校验项：更新接口文档时必须引用源码路径；新增 `UFUNCTION(BlueprintCallable)` 或 StoryFlow 节点时同步更新接口清单。

### 需要向用户确认的问题
- “导演接口汇总”是否只记录 `FirstRunTutorialDirectorSubsystem` 当前用到的接口，还是要记录所有未来通用剧情导演可调用的公共接口？
- 现有 `Docs/04_开发实现与系统文档/系统/*` 和 `Docs/99_归档/旧方案/旧根目录入口/Architecture/*` 是否就是本计划的执行产物？如果是，本轮应审校这些文件，而不是按“新建文件”执行。
- 对话缺口是否限定为“实时气泡/NPC 对话流程”，并继续保留现有 TutorialPopup、InfoHint、DialogContentDA 作为可用能力？
- 刷怪接口是否要进入本轮文档的“未来缺口”即可，还是需要另开后续代码方案来设计确定性刷怪 API？

### 需要手动创建的引擎资产
- 无

