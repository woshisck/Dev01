# Feature Log

记录每次完成的功能，供快速定位和回溯。

---

## 2026-05-25 — 剧情节点绑定 Flow Asset（NodeEventFlow）+ 死亡监听器 + 拾取物 FA 节点

**状态：** 已验证通过

**核心变更：**

### 1. NodeEventFlow — 节点绑定 FA

- `StoryEncounterTypes.h` line 205：`FStoryEncounterNode` 加 `NodeEventFlow`（`UFlowAsset*`）字段
- `StoryEncounterPointDataAsset.h` line 46：`UStoryEncounterPointDA` 加同名字段
- `StoryEncounterPointDataAsset.cpp`：`ToEncounterNode()` 同步该字段
- `StoryEncounterRuntimeSubsystem.cpp` line 689：`ExecuteEncounterNodeCore()` 提取三入口公共逻辑——先跑 `Actions[]`，再通过 Proxy 启动 `NodeEventFlow`
- `PlayLevelFlow` 非 Trigger Actor 回退路径修复：改用 Proxy，加 `return` 防止双跑（line 827）

### 2. AStoryFlowProxy — FA 运行代理

- **新建** `StoryFlowProxy.h/.cpp`：轻量代理 Actor，持有 `UFlowComponent`，Flow 运行完毕后定时轮询（0.25s）自动 Destroy
- 上下文快照：`ContextSourceActor`（弱引用）、`ContextTransform`（触发时抓取的快照）、`ContextPlayerController`
- `bStopExisting` 支持：spawn 前先销毁同 FlowAsset 的已有 Proxy（用于 `PlayLevelFlow` 的 `bStopExistingStoryFlow`）

### 3. 新 FA 节点

- **新建** `LENode_GetStoryContext.h/.cpp`：FA 内读取 Proxy 上下文（SourceActor Object Pin、ContextTransform Pin、PlayerController Object Pin），分类 `LevelEvent|Story`
- **新建** `LENode_SpawnRewardPickup.h/.cpp`：FA 内直接生成奖励拾取物；自动从 Proxy 读取 `ContextTransform`，无需手动接线位置，分类 `LevelEvent|Reward`

### 4. AStoryEncounterDeathListener — 死亡触发器

- **新建** `StoryEncounterDeathListener.h/.cpp`：关卡 Actor，不需要 Overlap Trigger，通过 `TargetActorName` 或 `TargetActorTag` 在 `BeginPlay` 自动绑定场景内匹配角色的 `OnCharacterDiedNative` 委托
- 支持三种绑定目标：`EncounterPoint`、`EncounterMap + NodeId`、`EncounterGraph + NodeId`
- `bTriggerOnce` 控制只触发一次还是每次死亡都触发

### 5. TrainingDummyCharacter

- `FinishDying()` 加 `OnCharacterDied.Broadcast(this)`：确保木头人死亡事件能被故事子系统接收，触发掉落拾取物逻辑

### 6. DummyDeathFlowSetupCommandlet（编辑器辅助）

- **新建** `DummyDeathFlowSetupCommandlet.h/.cpp`：自动创建或更新 `FA_DummyDeath_DropHeavyCard`，配置 `LENode_SpawnRewardPickup` 节点（重击卡），并绑定到 `EP_FirstRun_TrainingDummyCombo` 的 `NodeEventFlow` 字段

### 7. 自动化测试

- 补充 StoryEncounter 自动化测试：覆盖 Point 转换、Proxy 上下文、NodeEventFlow 启动、非 Trigger 来源的 PlayLevelFlow Proxy 路径

**典型用法：**

```text
剧情图 Death 节点（木头人）
  Actions[] = [ WeakHint: "击败木头人！" ]
  NodeEventFlow = FA_DummyDeath_DropHeavyCard
        ↓ Actions 执行完后
  StoryFlowProxy 运行 FA，携带 ContextTransform（木头人死亡时位置）
  FA 内: [Start] → [Spawn Reward Pickup] → [Finish]
```

**关联方案：** `Docs/WorkSession/current_plan.md`

---
