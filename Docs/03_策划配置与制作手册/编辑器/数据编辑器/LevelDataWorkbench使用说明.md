# Level Data Workbench 使用说明

## 入口

菜单路径：

`Tools -> DevKit Tools -> Balance Editors -> Level Data Workbench`

用途：

`Level Data Workbench` 是关卡数据工作台，不是路线编辑器。它集中展示和编辑现有的 `RoomDataAsset` 与 `CampaignDataAsset`，用于快速检查关卡细则、全局流程表、教程/故事事件标签。

## 界面结构

### 左侧：关卡列表

左侧显示项目中扫描到的 `RoomDataAsset`。

常用检查项：

| 字段 | 用途 |
| --- | --- |
| `RoomName` | 进入关卡时使用的关卡名或地图名 |
| `RoomTags` | 标记房间类型、层级、用途，例如普通房、商店房、事件房 |
| `EnemyPool` | 本房间可刷出的敌人池 |
| `LootPool` | 本房间可给出的奖励池 |
| `PortalDestinations` | 本房间通往下一关的传送门配置 |

左侧用于选中一个房间，方便在中间区域编辑它的细则。

### 中间：房间细则

中间显示当前选中的 `RoomDataAsset` Details 面板。

这里编辑的是“单个房间内部有什么”，例如：

- 敌人池。
- 奖励池。
- 房间类型标签。
- 是否主城/枢纽房。
- 传送门出口。
- 商店、事件、祭坛等房间专属配置。

配置原则：

- 普通战斗体验放在 `EnemyPool / LootPool / RoomTags`。
- 下一关出口放在 `PortalDestinations`。
- 教程和故事流程不要写死在房间细则里，应该通过右侧全局流程表的 `GlobalStageTag / StoryEventTags` 控制。

### 右侧：全局流程

右侧显示项目中扫描到的 `CampaignDataAsset`。

`CampaignDataAsset` 决定“一局游戏按什么阶段推进”，核心字段是：

| 字段 | 用途 |
| --- | --- |
| `FloorTable` | 全局关卡阶段表，按顺序填写每一关的宏观配置 |
| `RoomPool` | 当前 Campaign 可用的全局房间池 |
| `DefaultStartingRoom` | 第一关默认房间 |
| `LayerTag` | 本 Campaign 只选指定层级的房间 |

`FloorTable` 中每个阶段额外提供两个接口字段：

| 字段 | 用途 |
| --- | --- |
| `GlobalStageTag` | 当前阶段的全局标识，例如主城、第一战、奖励选择 |
| `StoryEventTags` | 当前阶段要抛给故事/教程系统的事件标签 |

## 传送门配置

当前版本不强制使用 Generic Graph。

一关通往另一关仍由 `RoomDataAsset.PortalDestinations` 控制：

| 字段 | 用途 |
| --- | --- |
| `PortalIndex` | 场景中传送门 Actor 的编号，和 `APortal.Index` 对应 |
| `RoomTypeTag` | 这扇门希望通往的房间类型 |
| `RoomPool` | 这扇门专属的候选房间池 |
| `PreviewTitle / PreviewDescription` | 传送门 UI 预览文案 |

后续如果需要更强的可视化，可以在中间区域增加 Generic Graph 视图，把每个 Root 节点表示为一个 `PortalIndex`。但这不是当前教程开发的阻塞项。

## 推荐工作流

1. 在左侧选择或检查目标 `RoomDataAsset`。
2. 在中间确认该房间的敌人、奖励、传送门是否完整。
3. 在右侧选择本局使用的 `CampaignDataAsset`。
4. 在 `FloorTable` 中按顺序填写每个阶段。
5. 对需要教程/故事触发的阶段填写：
   - `GlobalStageTag`
   - `StoryEventTags`
6. 保存资产，关闭编辑器后重新打开，确认数据没有丢失。

## 第一局教程配置示例

| 阶段 | `GlobalStageTag` | `StoryEventTags` |
| --- | --- | --- |
| 主城准备 | `Level.Stage.Hub` | `Tutorial.StartingLoadout` |
| 第一战 | `Level.Stage.FirstCombat` | `Tutorial.CardConsume` |
| 洗牌教学 | `Level.Stage.ShuffleRoom` | `Tutorial.Shuffle` |
| 奖励教学 | `Level.Stage.Reward` | `Tutorial.RewardToDeck` |
| 连携教学 | `Level.Stage.LinkCard` | `Tutorial.LinkCard` |
| 路线选择 | `Level.Stage.RouteChoice` | `Tutorial.RouteRewardChoice` |

这些标签不会直接决定房间内容。房间内容仍然由 `RoomDataAsset` 控制，标签只负责通知故事/教程系统“现在发生到了哪个全局阶段”。
