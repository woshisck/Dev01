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
| 处理普通释放 | 普通卡攻击出牌即触发 BaseFlow |
| 处理连携 | 少数 Link 卡根据前后卡、连招是否继续和条件配置触发正向/反向/断链结果 |
| 处理终结技 | 本阶段暂不扩展，后续单独整理 |
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
| `GlobalComboMultiplierConfig` | 普通卡连击倍率的全局配置 |

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
4. 卡牌被消耗，普通卡不再检查 Light / Heavy 是否匹配。
5. 普通卡执行 `BaseFlow`，并根据战斗侧提供的连击段数读取全局倍率。
6. Link 卡读取前一张或等待后一张，按配置触发正向连携、反向赋能或断链释放。
7. 如果存在等待中的反向连携，下一张卡打出时判断是否接受赋能；成功时额外执行单独配置的反向连携 Flow。
8. 推进 `CurrentIndex`。
9. 如果已经打空，进入洗牌冷却。

## 验收标准

- 卡组不控制玩家能否攻击。
- 卡牌打空后玩家仍可攻击，但不会触发卡牌。
- 每次洗牌后按 DeckList 固定顺序重新填充 ActiveSequence。
- UI 通过事件更新，不直接读取组件内部数组。
