# StoryEngineSubsystem 故事引擎架构

> 最后更新：2026-05-28
> 源文件：`Source/DevKit/Public/Story/StoryEngineSubsystem.h`

---

## 职责

`UStoryEngineSubsystem` 是 GameInstance 级 Subsystem，是项目故事/剧情的规则执行核心。

- 管理多个 `StoryRuleSetDA`（规则集资产），支持运行时热加载
- 接收故事事件，匹配并执行规则（条件求值 → 动作执行）
- 维护三类作用域的故事标志位（Flag）
- 管理任务目标状态（QuestTask）
- 防止事件处理重入（队列机制）

导演系统（DirectorSubsystem）是故事引擎的**主要外部调用者**，而不是被管理的对象。通用剧情逻辑通过 StoryEngine 的 RuleSet 配置，无需单独建导演类。

---

## 事件处理流程

```
BroadcastStoryEvent / BroadcastStoryEventWithPayload
    │
    ▼
PendingEvents 队列（防重入：处理中的事件不立即执行，入队等待）
    │
    ▼
ProcessStoryEvent(Context)
    │
    ├── CollectRulesForEvent(EventTag) → 从所有 RuleSet 收集匹配规则
    │
    ├── foreach Rule:
    │       EvaluateRule(Rule, Context)  → 条件组求值（AND/OR）
    │       ShouldSkipForFirePolicy()   → 检查触发策略（OncePerRun 等）
    │       DispatchActions(Rule, Context)
    │
    └── MarkRuleFired(Rule, Context)   → 写入 FiredRuleIds
```

### 防重入机制

```cpp
bool bIsProcessingEvents = false;
TArray<FStoryEventContext> PendingEvents;
```

调用 `BroadcastStoryEvent` 时：

- 若 `bIsProcessingEvents == false`：直接处理
- 若 `bIsProcessingEvents == true`：入队 PendingEvents，当前处理完成后消费队列

---

## 标志位（Flag）三种作用域

`EStoryFlagScope` 枚举定义：

| 作用域 | 常量 | 存活周期 | 持久化 |
| --- | --- | --- | --- |
| `Save` | `EStoryFlagScope::Save` | 永久（写入存档槽位） | 写入 YogSaveGame.StoryFlags |
| `Run` | `EStoryFlagScope::Run` | 本局（死亡/结束时 `ResetRunState()` 清除） | 不持久化（Checkpoint 里没有这部分） |
| `Session` | `EStoryFlagScope::Session` | 本次启动（退出时 `ResetSessionState()` 清除） | 不持久化 |

> **注意**：`Map` 不是标志位作用域。`OncePerMap` 是规则触发策略（`EStoryRuleFirePolicy`），控制规则是否每关只触发一次，与 StoryFlag 是两套独立机制。

使用建议：

- 教程进度、剧情关键节点 → `Save`（跨局永久保留）
- 本局内的临时进度 → `Run`（死亡后清除）
- 本次启动的 UI 状态或一次性提示 → `Session`

---

## 对外接口分组

### RuleSet 管理

| 接口 | 说明 |
| --- | --- |
| `SetRuleSets(TArray<UStoryRuleSetDA*>)` | 替换全部规则集 |
| `AddRuleSet(UStoryRuleSetDA*)` | 追加一个规则集 |

### 事件广播

| 接口 | 说明 |
| --- | --- |
| `BroadcastStoryEvent(EventTag, PC)` | 简单广播，无额外 payload |
| `BroadcastStoryEventWithContext(Context)` | 完整 Context 广播 |
| `BroadcastStoryEventWithPayload(EventTag, ContextTag, AreaTag, ItemTag, Actor, PC)` | 带 payload 广播（导演常用） |

### 动作执行（绕过规则匹配，直接执行）

| 接口 | 说明 |
| --- | --- |
| `ExecuteStoryAction(FStoryAction, Context)` | 执行单个 Action |
| `ExecuteStoryActions(TArray<FStoryAction>, Context)` | 批量执行 |

可执行的 Action 类型（`EStoryActionType`）：

| 类型 | 效果 |
| --- | --- |
| `SetFlag / ClearFlag` | 写入/清除标志位 |
| `ShowInfoHint` | 底部提示条（WeakHint/Dialogue/TutorialAreaHint） |
| `ShowTutorialPopup` | 教程弹窗（支持内联页面或 EventId 注册表） |
| `UnlockFeature` | 调用 MetaProgression.UnlockFeature |
| `SetQuestTask` | 设置任务目标 |
| `PlayLevelFlow` | 运行 LevelFlowAsset |
| `TriggerStoryEvent` | 广播另一个故事事件 |

### 标志位

| 接口 | 说明 |
| --- | --- |
| `SetStoryFlag(FlagTag, Scope, bool)` | 设置或清除标志位 |
| `HasStoryFlag(FlagTag, Scope)` | 查询标志位 |

### 任务目标（QuestTask）

| 接口 | 说明 |
| --- | --- |
| `SetQuestTask(TaskId, DisplayText, SourceTag, FlagTag)` | 创建/更新任务目标 |
| `CompleteQuestTask(TaskId)` | 标记任务完成 |
| `SetQuestTaskState(TaskId, EStoryQuestTaskState)` | 直接设置状态 |
| `GetQuestTask(TaskId, OutTask)` | 读取单个任务 |
| `GetAllQuestTasks()` | 读取全部任务（UI 遍历用） |
| `GetQuestTasksByState(State)` | 按状态过滤任务 |

### 状态重置

| 接口 | 调用时机 |
| --- | --- |
| `ResetRunState()` | 新局开始时（清除 Run 标志位和已触发规则） |
| `ResetSessionState()` | 退出/返回主菜单时（清除 Session 标志位） |

---

## 事件广播

| 事件 | 触发时机 |
| --- | --- |
| `OnStoryEventReceived(Context)` | 每次事件进入处理队列时 |
| `OnStoryRuleExecuted(RuleId, Context)` | 规则匹配并执行后 |
| `OnQuestTaskChanged(TaskData)` | 任务状态变更时 |

---

## 导演可用接口

导演系统直接调用的接口：

| 接口 | 使用场景 |
| --- | --- |
| `ExecuteStoryAction(Action, Context)` | 触发弹窗、设置标志位、任务目标等 |
| `BroadcastStoryEventWithPayload(...)` | 推进叙事规则（月光获取、终结技获取等） |
| `SetStoryFlag(Tag, Scope, bool)` | 直接写入进度标志，绕过规则匹配 |

**与导演的关系**：导演是调用方，StoryEngine 是执行引擎。导演知道"发生了什么"，StoryEngine 知道"该触发哪些规则"。

---

## 与其他系统的关系

```
StoryEngineSubsystem
    │
    ├── 被调用 → FirstRunTutorialDirectorSubsystem
    │
    ├── 读写 → YogSaveSubsystem (GetCurrentSave / CommitSave，持久化 Run 标志)
    │
    ├── 调用 → UTutorialManager (ShowTutorialPopup Action)
    │
    └── 调用 → YogMetaProgressionSubsystem (UnlockFeature Action)
```
