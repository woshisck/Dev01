# 卡组 HUD WBP 配置说明

## 作用
显示 1D 战斗卡组、下一张卡、洗牌进度、卡牌消耗提示和奖励加入提示。

本功能逻辑已经写在 C++ 中，WBP 只需要在 Designer 中摆控件并按指定名称命名，不需要写蓝图 Graph。

文字控件支持两类：

- 普通 `TextBlock`
- `Yog Common Rich Text Block`

如果需要关键词高亮、效果解释、流血/月光等说明文本，优先使用 `Yog Common Rich Text Block`。

## 需要创建的 WBP
### 1. `WBP_CombatDeckCardSlot`
父类选择：

`CombatDeckCardSlotWidget`

推荐层级：

```text
Root: SizeBox 或 Border
  CardFrame: Border
    Overlay 或 VerticalBox
      CardIcon: Image
      CardNameText: TextBlock
      ActionText: TextBlock
      TypeText: TextBlock
      StateText: TextBlock
```

必须勾选 `Is Variable` 的控件：

- `CardFrame`
- `CardIcon`，可选，不放也可以
- `CardNameText`
- `ActionText`
- `TypeText`
- `StateText`

### 2. `WBP_CombatDeckBar`
父类选择：

`CombatDeckBarWidget`

推荐层级：

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
        ShuffleText: TextBlock
      StatusText: TextBlock
      ConsumedToastText: TextBlock
      RewardToastText: TextBlock
```

必须勾选 `Is Variable` 的控件：

- `CardSlot_0` 到 `CardSlot_7`
- `ShufflePanel`
- `ShuffleProgressBar`
- `ShuffleText`
- `StatusText`
- `ConsumedToastText`
- `RewardToastText`

以上文字控件可以是普通 `TextBlock`，也可以替换为 `Yog Common Rich Text Block`。控件名字必须保持一致。

## Yog Common Rich Text Block 推荐配置

用于卡牌说明、浮窗、效果解释时建议配置：

| 字段 | 推荐值 |
| --- | --- |
| `FontStyleClass` | 项目通用中文字体样式，例如 `BP_InfoPopupTextStyle` |
| `OverrideFontSize` | 14 或 16 |
| `OverrideColor` | 浅灰或白色，Alpha 必须大于 0 |
| `Decorator Classes` | 需要关键词高亮时添加 `BP_KeywordDecorator` |

文本示例：

```text
造成 5 点 <key>流血</key>，持续 3 秒。
月光：前一张为 <key>攻击牌</key> 时触发 <key>月光斩</key>。
```

### 3. 放入 `WBP_HUDRoot`
打开 `WBP_HUDRoot`，把 `WBP_CombatDeckBar` 放进主 Canvas。

控件名称必须改为：

`CombatDeckBar`

并勾选 `Is Variable`。

## 推荐摆放
- 锚点：屏幕下方居中。
- 位置：玩家血条或武器 UI 上方。
- 卡槽尺寸：建议先用 `96 x 132` 或 `110 x 150`。
- `CardNameText` 建议开启自动换行，避免长卡名溢出。

## 测试方式
1. 进入战斗房间，确认卡组条出现。
2. 命中敌人，确认第一张卡消失，下一张卡高亮。
3. 打空卡组，确认 `ShufflePanel` 显示进度。
4. 选择三选一奖励，确认 `RewardToastText` 出现加入提示。
