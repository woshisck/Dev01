# WBP_RuneInfoCard 配置说明

## 作用

背包中选中符文时显示符文信息。现在可额外显示战斗卡牌分类、CardId、效果 Tag、连携方向和配方数量。

## 父类

`RuneInfoCardWidget`

## 控件配置

| 控件名                 | 类型                                             | 推荐参数                            | 用途                                         |
| ------------------- | ---------------------------------------------- | ------------------------------- | ------------------------------------------ |
| `CardBG`            | `Image`                                        | Slot Fill/Fill                  | 显示 `RuneConfig.CardBackground`，为空时使用默认深色背景 |
| `CardIcon`          | `Image`                                        | Size `80x80`                    | 显示 `RuneConfig.RuneIcon`                   |
| `CardName`          | `TextBlock`                                    | Font Size 16, Bold              | 显示 `RuneConfig.RuneName`                   |
| `CardDesc`          | `Yog/Common Rich Text` 或 `CommonRichTextBlock` | Auto Wrap Text 开启               | 显示 `RuneConfig.RuneDescription`            |
| `CardEffect`        | `Yog/Common Rich Text` 或 `CommonRichTextBlock` | Font Size 12                    | 显示 `GenericEffects` 关键词                    |
| `CardCombatInfo`    | `Yog/Common Rich Text` 或 `CommonRichTextBlock` | Font Size 11, Auto Wrap Text 开启 | 显示 CombatCard 信息                           |
| `ShapeGrid`         | `CanvasPanel`                                  | Size `120x120`                  | C++ 动态生成符文形状点阵                             |
| `GenericEffectList` | `GenericEffectListWidget`                      | 默认 Collapsed                    | 显示流血等关键词说明浮窗                               |
| `SelectionBorder`   | `Border`                                       | 默认 Hidden                       | 选中态高亮                                      |
| `GoldCostIcon`      | `Image`                                        | Size `16x16`                    | GoldCost > 0 时显示                           |
| `GoldCostText`      | `TextBlock`                                    | Font Size 11                    | 显示金币消耗                                     |

## CardCombatInfo 显示内容

C++ 自动写入：

```text
分类：普通卡牌 / 连携卡牌 / 终结技卡牌
CardId：Card.ID.Moonlight
效果Tag：Card.Effect.Moonlight
当前方向：正向 / 反向
连携配方：2 条
```

如果不放 `CardCombatInfo` 控件，不会报错，只是不显示战斗卡牌详情。

## 验收

1. 打开背包。
2. 选中一张战斗卡牌符文。
3. 右侧信息卡能看到分类、CardId、效果 Tag。
4. 选中月光时能看到当前方向和配方数量。
