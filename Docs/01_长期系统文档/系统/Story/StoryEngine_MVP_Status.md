# Story Engine MVP 状态与缺口整理

> 更新时间：2026-05-21
> 范围：教程、弱引导、轻量任务、首局/主城故事事件。完整 NPC 对话与长期任务线属于阶段二。

## 当前已补齐

### 1. 玩法事件接线

以下玩法节点已开始向 `UStoryEngineSubsystem` 广播事件：

| 玩法节点 | StoryEvent |
|---|---|
| 玩家获得第一枚进入战斗牌组的符文奖励 | `Story.Event.FirstRun.FirstRuneObtained` |
| 任意符文奖励获得 | `Story.Event.Item.Obtained`，并附带 `Story.Item.Rune` |
| 玩家首次/任意打开背包界面 | `Story.Event.FirstRun.FirstBackpackOpened` |
| 主城房间进入 | `Story.Event.Hub.FirstEntered` 与 `Story.Event.Area.Entered` |
| 玩家死亡 | `Story.Event.Player.Died` |
| 玩家复活 | `Story.Event.Player.Revived` |
| 局外成长节点购买 | `Story.Event.Hub.MetaFirstUpgradePurchased` |

这些事件本身可以重复广播，是否只生效一次由 `StoryRule.FirePolicy` 控制。

### 2. 任务状态 API

`UStoryEngineSubsystem` 现在支持：

- `SetQuestTask`
- `CompleteQuestTask`
- `SetQuestTaskState`
- `GetQuestTask`
- `GetAllQuestTasks`
- `GetQuestTasksByState`

这让遗圣目录 UI 可以直接查询 Active / Completed / Failed / Overwritten 任务，并监听 `OnQuestTaskChanged` 做刷新。

### 3. 教程弹窗开关

`UTutorialManager.bTutorialPopupsEnabled` 已在 `DefaultGame.ini` 中启用。
后续如果某些教程希望保持弱引导，可以在 StoryRule 中优先使用 `ShowInfoHint` 或 `PlayLevelFlow`，而不是 `ShowTutorialPopup`。

## 仍未完成

### 1. 记忆碎片教程内容

引擎已支持 `MemoryTutorial.Started / PlayerFailed / Completed` 等事件，但具体教程房间、剧情杀、锚点回退、Boss 骑士团成员演出，还需要内容资产与 LevelFlow 配置。

### 2. 遗圣目录 UI

数据层已可用，但还需要 UI：

- 当前任务列表
- 已完成任务列表
- 被划掉或改写的任务表现
- 任务来源区分：遗圣目录 / 黑夜少女 / 系统

### 3. 黑夜少女与长期剧情

当前 StoryEngine 只适合做“触发器”和“状态机底座”。
黑夜少女对话、NPC 状态变化、魂系分支任务线，应进入阶段二的 Dialogue & Questline：

- Generic Graph：任务线状态结构
- InkCPP：台词与分支文本
- Dialogue UI：对话显示、选项、角色表现
- StoryFlag：作为两个系统共享状态

## 推荐下一步

1. 配置一条完整的记忆碎片教程 LevelFlow：开始、失败回锚点、完成、剧情杀。
2. 做一个最小遗圣目录 UI，先只读 `GetQuestTasksByState(Active)`。
3. 为黑夜少女做 3 个状态：未见面、主城首次显化、第一局后解释符文/香炉。
