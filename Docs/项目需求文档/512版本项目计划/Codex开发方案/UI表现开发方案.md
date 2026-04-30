# Codex-UI表现组开发方案

## 组定位

本组负责 1D 卡组 HUD、洗牌装填表现、卡牌匹配反馈、连携触发原因、奖励加入提示。UI 表现组只监听卡组事件，不直接修改卡组状态。

## 负责范围

| 模块 | 说明 |
| --- | --- |
| 1D 卡组条 | 展示 ActiveSequence 和下一张卡 |
| 卡牌消耗表现 | 当前卡淡出、移走或点亮后消失 |
| 动作匹配反馈 | Light / Heavy 匹配时给明确反馈 |
| 连携提示 | 告诉玩家为什么触发连携 |
| 终结技提示 | 终结击触发卡牌时给更强反馈 |
| 洗牌装填 UI | 卡组打空后 UI 清空或显示进度 |
| 奖励加入提示 | 三选一后提示奖励已加入卡组 |

## 不负责范围

- 不判断卡牌是否匹配。
- 不推进 CurrentIndex。
- 不修改 DeckList。
- 不执行 BuffFlow。
- 不决定房间奖励生成。

## 监听事件

| 事件 | UI 响应 |
| --- | --- |
| `OnDeckLoaded` | 显示当前卡组序列 |
| `OnCardConsumed` | 移除或淡出当前卡 |
| `OnActionMatched` | 播放匹配反馈 |
| `OnLinkTriggered` | 显示连携原因 |
| `OnFinisherTriggered` | 播放终结技强化表现 |
| `OnShuffleStarted` | 清空卡组条，进入装填状态 |
| `OnShuffleProgress` | 更新装填进度 |
| `OnShuffleCompleted` | 按固定顺序回填卡组条 |
| `OnRewardAddedToDeck` | 显示奖励已加入卡组 |

## 文案原则

连携和热度强化不能只播放特效，还要告诉玩家原因。

示例：

- 月光读取上一张攻击牌：伤害提升。
- 火种已传递：下一张卡获得爆发。
- 终结击命中：卡牌效果强化。
- 热度加压：连携倍率提升。
- 装填完成：卡组已回填。

## 开发步骤

1. 确认当前 HUD 或 Widget 架构。
2. 新增 1D 卡组条基础布局。
3. 接入 `OnDeckLoaded` 和 `OnCardConsumed`。
4. 接入 `OnShuffleStarted`、`OnShuffleProgress`、`OnShuffleCompleted`。
5. 接入匹配、连携、终结技提示。
6. 接入奖励加入卡组提示。
7. 做一轮战斗截图检查，确认 UI 不挡住战斗主体。

## 验收标准

- 开局能看到当前卡组序列。
- 每次卡牌消耗后 UI 同步变化。
- 洗牌期间 UI 清空或显示装填进度。
- 洗牌结束后 UI 按固定顺序回填。
- 连携触发时能看到原因提示。
- 奖励选择后能看到“已加入卡组”的反馈。
