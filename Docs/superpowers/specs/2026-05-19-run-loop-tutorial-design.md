# 第一轮游戏循环与教程解锁设计

## 背景

当前项目已经完成或基本确认了战斗、符文、关卡拾取、奖励入组、卡组整理等核心局内玩法方向。局外成长、主城设施、结算页、玩家主动技能和战斗道具也已经有技术底座或早期实现。

后续制作目标不是一次性扩大所有系统，而是先完成一个稳定的首轮游戏循环：

```text
新存档 -> 第一局教程 Run -> 结算 -> 主城 -> 升级/准备 -> 第二局
```

确认后的制作优先级：

1. 先完成可玩闭环。
2. 再扩展局内构筑深度，包括玩家主动技能、战斗道具、进阶奖励池。
3. 教程系统贯穿两阶段，用来控制功能逐步开放。

## 核心设计决策

第一局教程 Run 由故事事件引擎编排，而不是由 `UTutorialManager` 直接决定关卡流程。

`UTutorialManager` 只负责教程状态、弹窗内容、是否正在展示、是否已完成和存档推进。它不应该决定第一关出现什么房间、什么时候刷怪、什么时候给奖励。

第一局呈现应由以下数据和系统共同控制：

| 系统 | 职责 |
| --- | --- |
| `UCampaignDataAsset` | 定义第一局固定阶段顺序和每阶段的 `GlobalStageTag` / `StoryEventTags` |
| `URoomDataAsset` | 定义具体房间内容，包括敌人、奖励、资源、是否开放商店/祭坛 |
| `UStoryEventRegistryDA` | 把阶段事件映射到 `TutorialPopup`、`LevelFlow` 或 `BroadcastOnly` |
| `ULevelFlowAsset` | 编排多步骤表现，例如延迟、教程弹窗、InfoPopup、等待奖励选择、等待整理完成 |
| `UTutorialManager` | 展示教程内容，推进教程状态，避免重复弹窗 |
| `UYogMetaProgressionSubsystem` | 保存功能解锁、货币、升级节点和教程后开放的局外内容 |

## 推荐第一局流程

第一局应是一条可控教学链。每个阶段只引入一个主要新概念。

| 阶段 | `GlobalStageTag` | `StoryEventTags` | 教学目标 |
| --- | --- | --- | --- |
| 武器拾取 | `Level.Stage.WeaponPickup` | `Tutorial.WeaponPickup` | 学会拾取武器，理解武器带起始卡组 |
| 第一战 | `Level.Stage.FirstCombat` | `Tutorial.CardConsume` | 学会攻击、卡牌按顺序消耗、基础战斗反馈 |
| 奖励 | `Level.Stage.Reward` | `Tutorial.RewardToDeck` | 学会三选一奖励进入卡组 |
| 背包整理 | `Level.Stage.Backpack` | `Tutorial.BackpackArrange` | 学会整理卡组、理解放置和顺序 |
| 连携教学 | `Level.Stage.LinkCard` | `Tutorial.LinkCard` | 学会连携卡的触发原因和收益 |
| 终结技教学 | `Level.Stage.Finisher` | `Tutorial.Finisher` | 学会终结技条件、触发和确认 |
| 路线选择 | `Level.Stage.RouteChoice` | `Tutorial.RouteRewardChoice` | 学会通过传送门或奖励方向影响下一关 |
| 跑局结束 | `Level.Stage.RunEnd` | `Tutorial.RunSummary` | 学会结算、局外资源、回主城 |

这些阶段不要求全部是独立地图。它们可以是同一 Campaign 的不同 FloorTable 条目，也可以是少量房间通过 LevelFlow 控制表现节奏。

## 教程表现规则

教程分为两类。

中心教程弹窗用于安全时机：

- 拾取武器后。
- 奖励选择前或奖励入组后。
- 背包整理开始时。
- 结算页展示时。
- 首次进入主城设施时。

HUD 或 InfoPopup 短提示用于战斗中：

- 卡牌消耗。
- 洗牌开始或完成。
- 连携触发。
- 终结技可用。
- 道具或主动技能首次解锁后的轻提示。

战斗中避免频繁弹出需要确认的中心弹窗。需要暂停和多页说明的内容，应放在战斗前、战斗后或整理阶段。

## 功能解锁节奏

所有功能都应通过同一套功能解锁状态控制，避免教程没教过但系统已经开放。

建议使用 `MetaProgression.UnlockedFeatures` 保存功能状态，并由 `UYogMetaProgressionSubsystem::IsFeatureUnlocked` 查询。

推荐解锁顺序：

| 时间点 | 解锁功能 |
| --- | --- |
| 新存档开始 | 只开放基础移动、攻击、武器拾取、第一局固定教程内容 |
| 武器教学完成 | 开放基础卡组 HUD 和卡牌消耗提示 |
| 奖励教学完成 | 开放奖励入组和奖励池基础过滤 |
| 背包教学完成 | 开放卡组整理入口 |
| 连携教学完成 | 开放基础连携卡池 |
| 终结技教学完成 | 开放终结技卡和终结技 UI |
| 第一局结算完成 | 开放主城、局外资源、升级终端、再次出发 |
| 第二局开始 | 开放半自由奖励池和少量路线选择 |
| 第二局后 | 逐步开放主动技能、战斗道具、商店、祭坛、进阶连携 |

功能 Tag 建议按用途分组，例如：

```text
Feature.Tutorial.WeaponDone
Feature.Tutorial.RewardDone
Feature.Tutorial.BackpackDone
Feature.Tutorial.LinkDone
Feature.Tutorial.FinisherDone
Feature.Hub.Enabled
Feature.Meta.UpgradeTerminal
Feature.Run.StartLoadoutSelect
Feature.Combat.ActiveSkill
Feature.Combat.CombatItem
Feature.Room.Shop
Feature.Room.Altar
```

具体命名应以项目 GameplayTag 规范为准。

## 奖励池与房间控制

第一局奖励池应强控，不使用完整随机池。

第一局只出现已教学或即将教学的内容：

- 武器房只给指定武器。
- 第一战只刷低复杂度敌人。
- 奖励房只给普通攻击符文或明确教学卡。
- 连携教学阶段固定给一张可触发连携的卡。
- 终结技阶段固定保证终结技可用。
- 商店、祭坛、复杂房间 Buff、完整道具池默认关闭。

第二局开始可以部分放开：

- 根据已完成教程阶段过滤奖励池。
- 根据局外解锁过滤起始符文、主动技能和战斗道具。
- 根据 `FeatureUnlock` 决定是否生成商店、祭坛、特殊事件房。

## 数据流

第一局阶段进入时：

```text
AYogGameMode::StartLevelSpawning
  -> 读取 CampaignDataAsset.FloorTable
  -> 缓存 GlobalStageTag / StoryEventTags
  -> 广播 OnCampaignStageEntered
  -> UStoryEventManager::ProcessCampaignStage
  -> StoryEventRegistryDA 查找事件
  -> 执行 TutorialPopup 或 LevelFlow
```

教程完成时：

```text
UTutorialManager
  -> 推进 ETutorialState
  -> 写入 UYogSaveGame.TutorialState
  -> 必要时调用 MetaProgressionSubsystem.UnlockFeature
```

跑局结束时：

```text
AYogGameMode
  -> BroadcastRunEnded
  -> UYogRunSummaryWidgetBase 展示结算
  -> 玩家返回主城
  -> Feature.Hub.Enabled / Feature.Meta.UpgradeTerminal 已开放
```

主城阶段：

```text
AHubFacilityActor
  -> 玩家交互
  -> 检查功能是否已解锁
  -> 推入对应 UI，例如 WBP_MetaUpgradeTree
```

## 错误与边界处理

- `StoryEventTags` 未注册时，只记录日志并跳过，不阻塞关卡。
- `TutorialRegistryDA` 找不到文案时，允许 fallback 文案，但应在编辑器报告中暴露。
- 教程已完成时，`bOnlyWhenTutorialIncomplete` 的故事事件必须跳过。
- `bFireOncePerRun` 用于防止同一局内重复触发同一教程事件。
- 如果玩家跳过教程或读取旧存档，应根据 `TutorialState` 和 `UnlockedFeatures` 迁移到合理状态。
- 战斗中不要打开会抢输入焦点的复杂 UI，除非 LevelFlow 明确暂停游戏。

## 测试与验收

第一阶段验收标准：

- 新存档稳定进入第一局教程 Run。
- 第一局阶段顺序可由 Campaign/StoryEvent/LevelFlow 控制。
- 每个教学点只触发一次，教程完成后不重复触发。
- 第一局必然出现奖励入组、背包整理、一次连携、一次终结技。
- 第一局结束后展示结算页，并能返回主城。
- 主城只开放已解锁设施，升级终端可读取局外货币和节点状态。
- 第二局开始时奖励池和房间内容会按已解锁功能过滤。

第二阶段验收标准：

- 主动技能和战斗道具不会在教学前出现在 UI、奖励池或输入提示里。
- 当功能解锁后，UI、输入、奖励池和教程提示同步开放。
- 商店、祭坛等复杂房间可通过 `FeatureUnlock` 逐步加入 Campaign。

## 后续实施顺序

1. 整理第一局 Campaign 阶段表和 StoryEventTags。
2. 配置 `StoryEventRegistryDA`，把第一局事件映射到 `TutorialPopup` 或 `LevelFlow`。
3. 补齐第一局教程文案和必要的 LevelFlow 资产。
4. 接入第一局完成后的结算页和主城返回。
5. 用 `UnlockedFeatures` 控制主城设施、奖励池、主动技能、战斗道具和特殊房间开放。
6. 在第二局后逐步开放主动技能、战斗道具、商店、祭坛和进阶连携。

