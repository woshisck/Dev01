# 故事引擎设计方案

> 版本：v1.1 | 2026-05-20
> 范围：两层架构——第一层 MVP（体验导演层）、第二层对话任务线（后续扩展）

---

## 架构总览

```text
┌──────────────────────────────────────────────────────┐
│  第一层：Story Engine MVP（当前目标）                  │
│  体验导演层：记忆碎片教程 / 首局引导 / 主城成长闭环    │
│                                                      │
│  StoryFlag → StoryCondition → StoryEvent             │
│                                    ↓ StoryAction     │
│  接入：LevelFlow / TutorialManager / Save / Meta     │
└──────────────────────────────────────────────────────┘
                        ↓ 后续扩展
┌──────────────────────────────────────────────────────┐
│  第二层：Dialogue & Questline（阶段二）               │
│  NPC 对话 / 魂系任务线 / 遗圣目录                    │
│                                                      │
│  Generic Graph + InkCPP + 对话 UI + NPC 状态序列     │
└──────────────────────────────────────────────────────┘
```

---

## 第一层：Story Engine MVP

### 1.1 设计目标

不是对话系统，而是**体验导演系统**：在对的时机，以对的方式，触发对的内容。

触发来源不限于 NPC 交互，包括：

- 进入区域
- 首次获得道具
- 战斗失败回锚点
- 剧情杀
- 第一局开始/结束
- 主城首次进入

### 1.2 四个核心概念

#### StoryFlag（故事标记）

持久化的布尔状态，存入存档。

```text
IsFirstRun                    // 是否第一局
MemoryTutorial.Completed      // 记忆碎片教程是否完成
FirstRune.Obtained            // 是否获得过第一个符文
FirstBackpack.Opened          // 是否打开过背包
Hub.FirstEntered              // 是否首次进入主城
Meta.FirstUpgradePurchased    // 是否购买过第一个主城升级
NightGirl.FirstMet            // 是否见过黑夜少女
```

#### StoryCondition（触发条件）

评估当前状态，决定一个事件或动作是否应该执行。

```text
HasFlag(name)              // 标记已设置
NotHasFlag(name)           // 标记未设置
IsFirstRun                 // 第一局中
IsInMemoryTutorial         // 在记忆碎片教程中
TutorialState == X         // 教程处于某个阶段
FeatureUnlocked(name)      // 功能已解锁
NotFeatureUnlocked(name)   // 功能未解锁
RunCount >= N              // 已跑过N局
```

条件支持组合：

```text
AND(NotHasFlag(FirstRune.Obtained), IsFirstRun)
OR(HasFlag(X), HasFlag(Y))
NOT(HasFlag(Z))
```

#### StoryEvent（故事事件）

游戏里发生的关键时机，广播给 StoryEngine 监听。

```text
// 记忆碎片教程
MemoryTutorial.Started
MemoryTutorial.AnchorReached
MemoryTutorial.PlayerFailed
MemoryTutorial.ScriptedDefeat
MemoryTutorial.Completed

// 第一局流程
FirstRun.Started
FirstRune.Obtained
FirstBackpack.Opened
FirstRun.Ended

// 主城
Hub.FirstEntered
Meta.FirstUpgradePurchased

// 通用
Player.Died
Player.EnteredArea(AreaTag)
Item.Obtained(ItemTag)
```

#### StoryAction（执行动作）

事件触发后执行的具体行为，结构化枚举，不用 FString。

```text
SetFlag(name, value)           // 设置/清除故事标记
PlayLevelFlow(FlowAsset)       // 触发 LevelFlow 序列（演出、剧情杀）
ShowTutorialPopup(TutorialId)  // 调用 TutorialManager 显示教学弹窗
ShowInfoHint(Text, Duration)   // 显示非阻塞提示字幕
SetQuestTask(TaskId, Text)     // 设置遗圣目录当前任务描述
CompleteQuestTask(TaskId)      // 标记任务完成
UnlockFeature(FeatureName)     // 解锁主城功能/UI面板
AddMetaCurrency(Amount)        // 给予主城货币奖励
TriggerStoryEvent(EventName)   // 级联触发另一个事件
```

---

### 1.3 StoryRule：事件 → 条件 → 动作

MVP 的核心数据单元是 **StoryRule**，一条规则 = 一次体验触发：

```text
StoryRule {
    TriggerEvent  : StoryEvent         // 监听什么事件
    Conditions[]  : StoryCondition     // 满足什么条件
    Actions[]     : StoryAction        // 执行什么动作
    bOnce         : bool               // 是否只触发一次
}
```

示例规则：

```text
// 规则：首次获得符文 → 显示教程
Rule_FirstRuneObtained {
    TriggerEvent : Item.Obtained(Rune)
    Conditions   : [NotHasFlag(FirstRune.Obtained), IsFirstRun]
    Actions      : [
        SetFlag(FirstRune.Obtained, true),
        ShowTutorialPopup(Tutorial_HowToEquipRune),
        SetQuestTask(Quest_Main, "打开背包查看你的符文")
    ]
    bOnce        : true
}

// 规则：首次进入主城 → 触发黑夜少女出现演出
Rule_HubFirstEnter {
    TriggerEvent : Hub.FirstEntered
    Conditions   : [NotHasFlag(Hub.FirstEntered)]
    Actions      : [
        SetFlag(Hub.FirstEntered, true),
        PlayLevelFlow(LF_NightGirl_FirstAppear),
        SetQuestTask(Quest_Main, "和黑夜少女交谈")
    ]
    bOnce        : true
}

// 规则：记忆碎片教程失败 → 提示重试
Rule_MemoryTutorialFailed {
    TriggerEvent : MemoryTutorial.PlayerFailed
    Conditions   : [IsInMemoryTutorial]
    Actions      : [
        ShowInfoHint("你已回到锚点，再次尝试", 3.0)
    ]
    bOnce        : false
}
```

---

### 1.4 与现有系统的接入

```text
StoryEngine（新建 GameInstance Subsystem）
    │
    ├── 读写 → YogSaveSubsystem        （StoryFlag 持久化）
    ├── 触发 → TutorialManager         （ShowTutorialPopup）
    ├── 触发 → LevelFlow               （PlayLevelFlow 演出序列）
    ├── 触发 → MetaProgressionSystem   （UnlockFeature / AddMetaCurrency）
    └── 写入 → QuestTaskWidget         （SetQuestTask / CompleteQuestTask）
```

StoryEngine 本身**不做具体事情**，只做路由：收事件 → 评估条件 → 分发动作给已有系统。

---

### 1.5 StoryRule 的存储方式

所有规则存在 DataAsset 里，运行时加载，不写死在代码中：

```text
Content/Story/Rules/
    ├── SR_MemoryTutorial.uasset     // 记忆碎片教程相关规则
    ├── SR_FirstRun.uasset           // 第一局引导规则
    └── SR_HubOnboarding.uasset     // 主城首次进入规则
```

每个 DA 是一个 `UStoryRuleSet`，包含 `TArray<FStoryRule>`。

---

### 1.6 实现顺序（MVP）

```text
Step 1：数据结构（C++ Struct）
  ├── FStoryCondition（枚举 + 参数）
  ├── FStoryAction（枚举 + 参数）
  └── FStoryRule（Event + Conditions[] + Actions[] + bOnce）

Step 2：UStoryRuleSetDA（DataAsset）
  └── TArray<FStoryRule>，在编辑器里填规则

Step 3：UStoryEngineSubsystem（GameInstance Subsystem）
  ├── LoadRuleSets()         // 启动时加载所有 RuleSet DA
  ├── BroadcastEvent(Name)   // 外部调用入口
  ├── EvaluateConditions()   // 检查条件
  └── DispatchActions()      // 执行动作，调用各子系统

Step 4：StoryFlag 接入存档
  └── YogSaveGame 加 TMap<FName, bool> StoryFlags

Step 5：接入各触发点
  ├── 首局开始 → BroadcastEvent(FirstRun.Started)
  ├── 首次进入主城 → BroadcastEvent(Hub.FirstEntered)
  └── 获得符文 → BroadcastEvent(Item.Obtained.Rune)
```

---

## 第二层：Dialogue & Questline（阶段二）

> 当 MVP 跑通后再实施，无前置阻塞。

### 2.1 定位

服务于长期 NPC 对话和魂系任务线，内容量大、分支多时使用。

### 2.2 工具选型

| 工具 | 职责 |
|---|---|
| **Generic Graph** | NPC 状态序列图（任务线结构） |
| **InkCPP**（Fab） | 对话台词文本（Inky 桌面编辑器撰写） |

### 2.3 与第一层的关系

第二层的对话节点激活，可以通过第一层的 `StoryAction: PlayLevelFlow` 或 `TriggerStoryEvent` 触发，两层共享 StoryFlag 状态。

### 2.4 实现前提

- InkCPP 在 UE5.4 兼容性验证完毕
- 对话 UI（WBP_DialogueWidget）设计确认
- 第一条 NPC 任务线内容完成（黑夜少女 3 个状态）

---

## 附：待定问题

### MVP 阶段

- [ ] QuestTask（遗圣目录）是简单 FText，还是需要多步骤任务结构？
- [ ] StoryFlag 是否需要分"局内重置"和"跨局持久"两种？
- [ ] HintActor（环境提示字幕）是否在 MVP 阶段实现，还是延后？

### 阶段二

- [ ] InkCPP UE5.4 兼容性验证
- [ ] 对话 UI 是否需要立绘/角色头像？
- [ ] 遗圣目录任务文本是写在 Ink 里，还是直接用 StoryAction.SetQuestTask？
- [ ] NPC 物理位置变化（状态切换后换位置）走 Flow 还是直接 BP？
