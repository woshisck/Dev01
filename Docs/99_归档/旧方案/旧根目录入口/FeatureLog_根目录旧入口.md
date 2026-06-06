> 状态：归档。仅用于历史追溯，不作为当前实现依据。

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

## 2026-05-26 — Story Director FA 体系（UStoryFlowAsset + USNode_*）

**状态：** 代码已写完，待编译验证

**核心变更：**

### 1. UStoryFlowAsset — 独立剧情 FA 类型

- **新建** `Story/Flow/StoryFlowAsset.h/.cpp`：纯类型标识类，继承 `UFlowAsset`，DisplayName "Story Director Flow"
- `UStoryEncounterPointDA.NodeEventFlow` 字段类型从 `UFlowAsset*` 升级为 `UStoryFlowAsset*`
- `FStoryEncounterNode.NodeEventFlow` 同步升级

### 2. USNode_Base — SNode 基类

- **新建** `Story/Flow/Nodes/SNode_Base.h/.cpp`：设置 `AllowedAssetClasses = { UStoryFlowAsset }`，严格限定 SNode_* 只在 Story FA 编辑器内可见
- 三个访问器：`GetStoryEngine()`、`GetPlayerController()`、`GetStoryProxy()`

### 3. 七个 USNode 节点（全新）

| 节点 | 文件 |
| --- | --- |
| `USNode_ShowHint` | 显示弱引导提示 |
| `USNode_ShowTutorialPopup` | 显示教程弹窗 |
| `USNode_RecordProgress` | 持久化剧情进度 Tag |
| `USNode_GiveCard` | 给予卡牌（Out/DeckFailed/InventoryFailed 三引脚） |
| `USNode_EnablePortal` | 激活传送门（可选 Open） |
| `USNode_SpawnRewardPickup` | 生成奖励拾取物（Story FA 版） |
| `USNode_ActivateTutorialSpawner` | 激活教程刷怪点（Story FA 版） |

### 4. FA 隔离修复

- `BFNode_Base.cpp`：`DeniedAssetClasses` 新增 `UStoryFlowAsset`，防止 Buff 节点污染 Story FA 编辑器

### 5. Commandlet 升级

- `DummyDeathFlowSetupCommandlet.cpp`：全面替换为 `UStoryFlowAsset` + `USNode_SpawnRewardPickup`，新增旧类型资产冲突检测
- `FirstRunTutorialSpawnerSetupCommandlet.cpp`（新建）：创建 `FA_ActivateTutorialDummySpawner`（Story FA）+ 配置 `USNode_ActivateTutorialSpawner`，创建 `EP_FirstRun_WeaponPickupActivateDummy` 故事点，配置 `B_TutorialMobSpawner` 蓝图

### 6. 自动化测试修复

- `StoryEncounterGraphTests.cpp`、`StoryEncounterRuntimeTests.cpp`：`NewObject<UFlowAsset>` 改为 `NewObject<UStoryFlowAsset>`，适配类型收窄

**关联文档：** [StoryFA_NodeReference.md](../../../01_长期系统文档/系统/Story/StoryFA_NodeReference.md)

---


## 2026-05-26 — SNode 补全（Tier 1+2）：SetActorEnabled / UnlockFeature / SetRoomRewardOverride / SetPortalOverride / SetQuestObjective / TutorialAreaHint

**状态：** 代码已写完，待编译验证

**核心变更：**

### 1. 一档节点（教程必需）

- **SNode_SetActorEnabled**：按 Actor Name/Tag 遍历场景，SetHidden/Collision/Tick。用于教程期间隐藏正常流程起始武器、显示教程演示武器
- **SNode_UnlockFeature**：`YogMetaProgressionSubsystem::UnlockFeature(FeatureTag)`，解锁冲刺/背包等功能
- **SNode_SetRoomRewardOverride**：`AYogGameMode::SetRoomRewardOptionsOverride / Clear`，覆盖教程房间固定奖励池
- **SNode_SetPortalOverride**：`AYogGameMode::SetForcedPortalOverride / Clear`，强制指定传送门目的地

### 2. 二档节点（玩法补充）

- **SNode_SetQuestObjective**：`StoryEngineSubsystem::SetQuestTask()`，新增/更新任务目标条目（写存档 + 广播 OnQuestTaskChanged）
- **SNode_TutorialAreaHint**：ShowInfoHint 包装，Duration 默认 0（常驻），适合区域进入后持续展示的操作说明
- **SNode_ShowHint 扩展**：新增可选 `HintTitle` 字段（空 = 无标题 WeakHint 样式，有值 = 带标题 InfoHint）

### 3. ActionKind 覆盖情况

本次补全后，DA `Actions[]` 的 13 个 ActionKind 中，已有 SNode 对应 10 个。剩余 3 个暂缓：

- `PlayLevelFlow`：DA Actions[] 保留，Story FA 内不嵌套 Level FA
- `Dialogue`：阶段二对话系统
- `TeleportToNode`：暂无用例

**关联文档：** [StoryFA_NodeReference.md](../../../01_长期系统文档/系统/Story/StoryFA_NodeReference.md)

---

