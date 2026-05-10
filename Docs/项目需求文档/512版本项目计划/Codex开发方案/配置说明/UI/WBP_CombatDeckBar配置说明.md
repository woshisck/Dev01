# WBP_CombatDeckBar 配置说明

## 作用

`WBP_CombatDeckBar` 是战斗卡组主显示条。

它绑定玩家身上的 `CombatDeckComponent`，显示当前剩余卡牌、下一张牌、洗牌进度、卡牌消耗提示和奖励加入提示。

## 父类

创建 WBP 时父类选择：

```text
CombatDeckBarWidget
```

## 推荐层级

```text
Root: CanvasPanel
  SizeBox 或 Border
    VerticalBox
      CardRow: HorizontalBox
        CardSlot_0: WBP_CombatDeckCardSlot
        CardSlot_1: WBP_CombatDeckCardSlot
        CardSlot_2: WBP_CombatDeckCardSlot
        CardSlot_3: WBP_CombatDeckCardSlot
        CardSlot_4: WBP_CombatDeckCardSlot
        CardSlot_5: WBP_CombatDeckCardSlot
        CardSlot_6: WBP_CombatDeckCardSlot
        CardSlot_7: WBP_CombatDeckCardSlot
      ShufflePanel: Border 或 Overlay
        ShuffleProgressBar: ProgressBar
        ShuffleText: Yog Common Rich Text Block
      StatusText: Yog Common Rich Text Block
      ConsumedToastText: Yog Common Rich Text Block
      RewardToastText: Yog Common Rich Text Block
```

## 组件用途和数据来源

| 控件名 | 类型 | 用途 | 数据来源 |
| --- | --- | --- | --- |
| `CardSlot_0` 到 `CardSlot_7` | `WBP_CombatDeckCardSlot` | 显示最多 8 张剩余卡牌；`CardSlot_0` 是下一张牌 | `CombatDeckComponent -> GetRemainingDeckSnapshot()` |
| `ShufflePanel` | `Border` 或 `Overlay` | 洗牌时显示，非洗牌时自动隐藏 | `CombatDeckComponent -> DeckState` |
| `ShuffleProgressBar` | `ProgressBar` | 显示洗牌进度 | `CombatDeckComponent -> OnShuffleProgress` |
| `ShuffleText` | `Yog Common Rich Text Block` | 显示洗牌百分比 | `OnShuffleProgress` 的归一化进度 |
| `StatusText` | `Yog Common Rich Text Block` | 显示卡组状态 | `CombatDeckComponent -> DeckState` 和剩余卡牌数量 |
| `ConsumedToastText` | `Yog Common Rich Text Block` | 显示刚消耗的卡牌 | `CombatDeckComponent -> OnCardConsumed`，卡名来自 `CombatCard.DisplayName` |
| `RewardToastText` | `Yog Common Rich Text Block` | 显示刚加入卡组的奖励卡牌 | `CombatDeckComponent -> OnRewardAddedToDeck`，卡名来自 `CombatCard.DisplayName` |

## 推荐配置

| 控件 | 关键属性 |
| --- | --- |
| `Root` | `CanvasPanel -> Anchors=Bottom Center; Alignment=(0.5,1.0); Position=(0,0); Size=(900,120)` |
| `SizeBox 或 Border` | `Width Override=900; Height Override=112; Brush Color=(0.012,0.014,0.018,0.78)` |
| `VerticalBox` | `Slot -> Fill/Fill; Padding=10` |
| `CardRow` | `HorizontalBox; Slot -> Auto; Padding=(0,0,0,6)` |
| `CardSlot_0` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_1` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_2` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_3` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_4` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_5` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_6` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `CardSlot_7` | `Size=(72,86); Padding=(2,0,2,0); Is Variable=true` |
| `ShufflePanel` | `Visibility=Collapsed; Slot -> Fill/Auto; Padding=(0,4,0,0); Brush Color=(0.05,0.05,0.07,0.82)` |
| `ShuffleProgressBar` | `Percent=0; Fill Color=(0.95,0.72,0.22,1); Size Height=8` |
| `ShuffleText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=12; OverrideColor=(1,0.86,0.45,1); Justification=Center` |
| `StatusText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=12; OverrideColor=(0.78,0.82,0.90,1); Justification=Center` |
| `ConsumedToastText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=14; OverrideColor=(1,1,1,1); Justification=Center; Visibility=Collapsed` |
| `RewardToastText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=14; OverrideColor=(0.55,1.00,0.70,1); Justification=Center; Visibility=Collapsed` |

## Class Defaults 参数

| 字段 | 参数 |
| --- | --- |
| `ToastVisibleDuration` | `0.8` |
| `ToastFadeDuration` | `0.25` |

## C++ 自动动画

`ConsumedToastText` 和 `RewardToastText` 的显示、保持、淡出由 C++ 自动处理，不需要在 WBP Graph 中写动画。

| 触发 | C++ 行为 |
| --- | --- |
| 卡牌被消耗 | 写入 `Consumed: 卡名`，显示 `ConsumedToastText`，保持 `ToastVisibleDuration` 秒后淡出 |
| 奖励加入卡组 | 写入 `Added: 卡名`，显示 `RewardToastText`，保持 `ToastVisibleDuration` 秒后淡出 |
| 淡出结束 | 自动把对应文本设为 `Collapsed` |

## 必须检查

- `CardSlot_0` 到 `CardSlot_7` 按需要放置，并勾选 `Is Variable`。
- `ShufflePanel / ShuffleProgressBar / ShuffleText / StatusText / ConsumedToastText / RewardToastText` 都勾选 `Is Variable`。
- 所有文字控件推荐使用 `Yog Common Rich Text Block`。
- 不需要手写绑定逻辑和提示动画，HUD 会自动调用 `BindToCombatDeck`，C++ 会自动处理提示淡出。
