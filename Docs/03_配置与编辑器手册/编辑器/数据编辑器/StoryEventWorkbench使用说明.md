# Story Event Workbench 使用说明

## 入口

菜单路径：

`Tools -> DevKit Tools -> Balance Editors -> Story Event Workbench`

用途：

`Story Event Workbench` 是故事事件引擎的集中编辑器。它用来管理 `StoryEventRegistryDA`，并检查 `CampaignDataAsset.FloorTable.StoryEventTags` 是否已经配置到故事事件注册表中。

## 界面结构

### 左侧：故事事件注册表

左侧扫描并显示项目中的 `StoryEventRegistryDA`。

表格列：

| 列 | 说明 |
| --- | --- |
| `Registry` | 注册表资产名 |
| `E` | Entries 总数 |
| `T` | `TutorialPopup` 事件数量 |
| `F` | `LevelFlow` 事件数量 |
| `B` | `BroadcastOnly` 事件数量 |
| `Open` | 打开资产编辑器 |

选中一个注册表后，中间区域会显示它的 Details 面板。

### 中间：注册表 Details

中间用于编辑 `StoryEventRegistryDA.Entries`。

每一条 Entry 对应一个故事事件：

| 字段 | 说明 |
| --- | --- |
| `EventTag` | Campaign 阶段中要监听的事件标签 |
| `ActionType` | 事件行为类型 |
| `TutorialEventID` | 教程弹窗 ID |
| `LevelFlow` | 要启动的 Level Event Flow |
| `bOnlyWhenTutorialIncomplete` | 教程完成后跳过 |
| `bFireOncePerRun` | 本局只触发一次 |

### 右侧：Campaign 事件对照

右侧扫描并显示 `CampaignDataAsset`。

上半区选择 Campaign，下半区列出该 Campaign 的所有 `StoryEventTags`。

表格列：

| 列 | 说明 |
| --- | --- |
| `Floor` | 第几关，从 1 开始显示 |
| `Stage` | `GlobalStageTag` |
| `Event` | `StoryEventTags` 中的单个事件标签 |
| `Config` | 该事件是否已经存在于当前选中的 `StoryEventRegistryDA` |

如果 `Config` 显示 `Missing`，说明 Campaign 里使用了这个故事事件标签，但当前注册表没有配置对应行为。

## 推荐使用流程

1. 打开 `Story Event Workbench`。
2. 左侧选择主运行用的 `DA_StoryEventRegistry_MainRun`。
3. 右侧选择当前使用的 `DA_Campaign_*`。
4. 查看右下角事件表。
5. 对所有 `Missing` 的事件，在中间 `Entries` 里新增配置。
6. 保存注册表和 Campaign。
7. PIE 进入对应阶段，检查教程弹窗或 LevelFlow 是否触发。

## 常用配置

### 教程弹窗

| 字段 | 示例 |
| --- | --- |
| `EventTag` | `Tutorial.CardConsume` |
| `ActionType` | `TutorialPopup` |
| `TutorialEventID` | `tutorial_card_consume` |
| `bPauseGame` | `true` |
| `bOnlyWhenTutorialIncomplete` | `true` |
| `bFireOncePerRun` | `true` |

### 剧情 LevelFlow

| 字段 | 示例 |
| --- | --- |
| `EventTag` | `Story.FirstCombat.Intro` |
| `ActionType` | `LevelFlow` |
| `LevelFlow` | `LFA_Tutorial_CardConsume` |
| `bStopExistingStoryFlow` | `true` |
| `bFireOncePerRun` | `true` |

### 蓝图自定义事件

| 字段 | 示例 |
| --- | --- |
| `EventTag` | `Story.RouteChoice.Custom` |
| `ActionType` | `BroadcastOnly` |

蓝图监听 `UStoryEventManager.OnStoryEventDispatched` 后自行处理。

## 和 Level Data Workbench 的分工

| 工具 | 负责内容 |
| --- | --- |
| `Level Data Workbench` | 编辑房间、Campaign、FloorTable、StoryEventTags |
| `Story Event Workbench` | 编辑 StoryEventRegistry，并检查 Campaign 事件是否已注册 |

一句话：

先在 `Level Data Workbench` 里决定“哪个阶段发生什么事件”，再在 `Story Event Workbench` 里决定“这个事件具体做什么”。
