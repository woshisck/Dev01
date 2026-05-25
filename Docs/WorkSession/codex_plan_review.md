## Codex 审查结果

### 发现的问题
- `UStoryEncounterRuntimeSubsystem` 实际继承的是 `UGameInstanceSubsystem`，不是方案中写的 `WorldSubsystem`；基于 WorldSubsystem 的论证不准确。
- `PlayLevelFlow` 的修复方案可能导致 Flow 执行两次：`RunFlowViaProxy()` 后如果不 `return`，现有逻辑仍会继续调用 `StoryEngine->ExecuteStoryAction()`，再走 `AYogGameMode::RunStoryLevelFlow()`。
- 当前非 Trigger Actor 场景并非完全“静默失败”：现有代码会回退到 `StoryEngine -> GameMode` 执行 Flow；真正缺失的是以触发源 Actor 作为 Flow 上下文。
- `AStoryFlowProxy` 头文件示例中使用了 `UFlowAsset*`，但只前置声明了 `UFlowComponent` / `ULevelFlowAsset`，会缺少 `class UFlowAsset;`。
- `UFlowComponent` 当前未看到 `OnRootFlowFinished` 之类的结束委托；方案中的 `OnFlowFinished()` 绑定大概率不能直接编译。
- “延迟 1 帧 Destroy 兜底”不成立：带 `Delay`、等待 UI、等待选择、播放序列等异步 Flow 会被过早销毁。
- 只在 `TriggerEncounterNode()` 添加 `NodeEventFlow` 不够；当前 `TriggerEncounterNode()`、`TriggerEncounterGraphNode()`、`TriggerEncounterPoint()` 都各自独立执行 `Actions`，需要三处统一处理或抽出公共执行函数。
- 仅在 `AStoryFlowProxy` 上暴露 `GetContextSourceActor()` 不等于 Flow 图中已有 `[Get Story Context Actor]` 节点；还需要明确设计可被 LevelFlow 编辑器使用的节点或上下文访问机制。
- `RunFlowViaProxy()` 没有处理 `bStopExistingStoryFlow` 语义；用于 `PlayLevelFlow` 回退时会把原本“停旧 Flow 再启新 Flow”的行为改成并发 Spawn 多个 Proxy。
- `UStoryEncounterGraphNode` 的 fallback 节点目前没有 `Actions` / `NodeEventFlow` 编辑字段；如果未绑定 `UStoryEncounterPointDA` 的图节点也要支持该能力，方案漏了这部分。

### 潜在风险
- 新增 `TObjectPtr<ULevelFlowAsset>` 是硬引用，剧情 DA / Map 被加载时会连带加载 Flow Asset；如果 Flow 资产体量变大，可能增加加载成本。
- Proxy 作为 Flow Root Owner 会改变部分现有 LevelFlow 节点行为，例如 `LENode_ShowInfoPopup` 当前只识别 `ALevelEventTrigger` / `AStoryEncounterTrigger` 来绑定离开区域关闭逻辑。
- `ContextSourceActor` 是弱引用，死亡、复位或销毁后 Flow 延迟读取可能拿到空指针。
- FlowComponent 默认 `RootFlowMode = Authority`，NodeEventFlow 若承担纯 UI 表现，需要确认是否应在客户端、本地玩家或服务器执行。
- 临时 Proxy 上的 `UFlowComponent` 会注册到 FlowSubsystem；若存档发生在 Flow 运行中，需确认不会把临时剧情 Flow 错误写入存档。
- “现有资产兼容性零风险”表述过满；序列化层面通常兼容，但仍需要 UE 编译、资产加载、重存和编辑器详情面板验证。

### 改进建议
- 抽出公共方法，例如 `ExecuteEncounterNode(EncounterId, Node, SourceActor)`，统一处理 Context、条件、Actions、NodeEventFlow，避免三条入口重复漏改。
- 将 `RunFlowViaProxy()` 改为返回 `bool`，并在 `PlayLevelFlow` 回退执行成功后立刻 `return`，防止继续走 GameMode 回退。
- 区分两类执行：`NodeEventFlow` 默认可并发；`PlayLevelFlow` 回退应明确是否保留 `bStopExistingStoryFlow` 的单实例语义。
- 不要用 1 帧销毁兜底；应实现可靠完成信号，例如自定义 `UStoryFlowComponent` 监听 Root Flow Custom Output，或定时轮询 `GetRootInstances()` 直到该 Proxy 的 Root Flow 结束。
- 增加可编辑器使用的上下文节点，例如 `ULENode_GetStoryContextActor` / `ULENode_GetStoryContextLocation`，或定义 `IStoryFlowContextProvider` 接口让节点从 Root Owner 读取上下文。
- 在 `RunFlowViaProxy()` 中补充 `GetWorld()` 判空、Spawn 失败日志、必要的 `SpawnCollisionHandlingOverride`，并考虑设置 Proxy 的 Transient/Hidden/NoCollision 属性。
- 补自动化测试：`UStoryEncounterPointDA::ToEncounterNode()` 复制 `NodeEventFlow`；三条触发入口都会执行一次 NodeEventFlow；非 Trigger Actor 的 `PlayLevelFlow` 不会双跑。

### 需要向用户确认的问题
- `NodeEventFlow` 应在 `Actions[]` 之前、之后，还是与某个动作顺序混排执行？
- `PlayLevelFlow` 在非 Trigger Actor 场景下，应使用 Proxy 获取 SourceActor 上下文，还是继续使用 GameMode 的全局单 Flow 机制？
- `NodeEventFlow` 是否允许多个实例并发运行，还是同一节点/同一 Flow 需要去重或覆盖旧实例？
- Flow 中除 `SourceActor` 外是否还需要 `EncounterId`、`NodeId`、`PlayerController`、房间数据、事件 Tag、死亡位置快照等上下文？
- 未绑定 `UStoryEncounterPointDA` 的 `UStoryEncounterGraphNode` fallback 节点是否也要能配置 `NodeEventFlow`？
- 木头人死亡后 Actor 是否会很快复位或销毁？如果会，是否应在触发瞬间缓存 Transform，而不是让 Flow 延迟读取 Actor？

### 需要手动创建的引擎资产
- `FA_DummyDeath_DropHeavyCard`：`ULevelFlowAsset`，用于木头人死亡掉落重击卡流程。
- `BP_Pickup_HeavyCard` 或等价拾取物蓝图：若项目中尚不存在，需要创建并在 Flow 中引用。
- 对应剧情点 DA / EncounterMap 节点配置：为目标死亡节点手动设置 `NodeEventFlow` 引用。