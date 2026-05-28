# 剧情系统 — Story

> 包含 StoryEngine 规则引擎、导演系统（Director）、教程流水线。

## 架构说明

| 文档 | 内容 |
| --- | --- |
| [StoryEngine_Architecture.md](StoryEngine_Architecture.md) | StoryEngine 完整架构：事件处理流、标志位作用域、接口分组 |
| [DirectorInterfaces.md](DirectorInterfaces.md) | **导演接口汇总**（双向：导演→系统 + 系统→导演钩子）+ 缺口分析 |
| [StoryEngine_Design.md](StoryEngine_Design.md) | 设计说明 |
| [StoryFA_NodeReference.md](StoryFA_NodeReference.md) | 故事 FA 节点参考 |
| [StoryEngine_MVP_Status.md](StoryEngine_MVP_Status.md) | MVP 当前状态 |

## 使用指南

| 文档 | 内容 |
| --- | --- |
| [../../StoryPipeline/FirstRunTutorial_StatusAndTodo.md](../../StoryPipeline/FirstRunTutorial_StatusAndTodo.md) | 教程流水线进度与待办 |
| [../../StorySource/FirstRunTutorial_Story.md](../../StorySource/FirstRunTutorial_Story.md) | 教程文案 |

## 接口参考（速查）

```cpp
// 触发剧情规则
StoryEngine->BroadcastStoryEventWithPayload(EventTag, ...);

// 直接执行动作（绕过规则）
StoryEngine->ExecuteStoryAction(FStoryAction, Context);

// 写入永久进度标志
StoryEngine->SetStoryFlag(FlagTag, EStoryFlagScope::Save, true);
```

标志位作用域：`Save`（永久）/ `Run`（本局）/ `Session`（本次启动）  
`OncePerMap` 是规则触发策略，不是标志位作用域。

## 关联系统

- [../Progression/](../Progression/) — SaveSubsystem + MetaProgression（导演调用的目标系统）
- [../Rune/FA_UniversalArchitecture.md](../Rune/FA_UniversalArchitecture.md) — FA 节点体系
