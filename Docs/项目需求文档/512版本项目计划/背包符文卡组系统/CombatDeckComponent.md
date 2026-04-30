# CombatDeckComponent

## 目标

`CombatDeckComponent` 是 512 版本的前台卡组运行时组件，负责把后台背包中的符文或卡牌，转换为战斗中可消耗的 1D 攻击卡组。

玩家看到的是一条 1D 卡牌序列，而不是旧 2D 背包格子。

## 组件职责

| 职责 | 说明 |
| --- | --- |
| 维护卡组列表 | 保存玩家当前可用于战斗的 DeckList |
| 维护前台序列 | ActiveSequence 表示当前 UI 展示和可消耗的卡牌 |
| 处理攻击消耗 | 每次攻击事件尝试消耗当前卡牌 |
| 判断动作匹配 | RequiredAction 与 ActionType 匹配时触发奖励 |
| 处理连携 | 根据 LinkMode 读取上一张或传递给下一张 |
| 处理终结技 | 根据 FinisherRequirement 判断额外效果 |
| 处理洗牌冷却 | 卡牌打空后进入 EmptyShuffling |
| 派发 UI 事件 | 通知 UI 卡牌消耗、洗牌开始、洗牌完成等状态 |

## 运行时状态

| 字段 | 说明 |
| --- | --- |
| `DeckList` | 玩家当前卡组固定顺序列表 |
| `ActiveSequence` | 当前正在 UI 上展示并可消耗的卡牌 |
| `CurrentIndex` | 当前待消耗卡牌下标 |
| `DeckState` | Ready / EmptyShuffling |
| `ShuffleCooldown` | 当前剩余洗牌时间 |
| `LastResolvedCard` | 上一张被消耗的卡牌，用于 Link 读取 |
| `PendingLinkContext` | 传递给下一张卡的连携上下文 |

## 核心接口

```cpp
FCombatCardResolveResult ResolveAttackCard(
    ECardRequiredAction ActionType,
    bool bIsComboFinisher,
    bool bFromDashSave
);
```

## 处理流程

1. 如果 `DeckState` 是 `EmptyShuffling`，返回无卡结果，不阻止玩家攻击。
2. 如果 `ActiveSequence` 没有可用卡牌，返回无卡结果，不阻止玩家攻击。
3. 读取 `CurrentIndex` 对应卡牌。
4. 判断 `ActionType` 是否满足卡牌 `RequiredAction`。
5. 卡牌无论是否匹配都会被消耗。
6. 不匹配时只触发 `BaseFlow`。
7. 匹配时触发 `BaseFlow`、`MatchedFlow`，并检查 Link 和 Finisher。
8. 推进 `CurrentIndex`。
9. 如果已经打空，进入洗牌冷却。

## 验收标准

- 卡组不控制玩家能否攻击。
- 卡牌打空后玩家仍可攻击，但不会触发卡牌。
- 每次洗牌后按 DeckList 固定顺序重新填充 ActiveSequence。
- UI 通过事件更新，不直接读取组件内部数组。
