# WBP_CombatDeckCardSlot 配置说明

## 作用

`WBP_CombatDeckCardSlot` 显示一张战斗卡牌。

它读取 `FCombatCardInstance`，显示卡牌图标、名称、动作类型、卡牌类型，以及这张牌是否是下一张即将打出的牌。

## 父类

创建 WBP 时父类选择：

```text
CombatDeckCardSlotWidget
```

## 推荐层级

```text
Root: SizeBox 或 Border
  CardFrame: Border
    Overlay 或 VerticalBox
      CardIcon: Image
      CardNameText: Yog Common Rich Text Block
      ActionText: Yog Common Rich Text Block
      TypeText: Yog Common Rich Text Block
      StateText: Yog Common Rich Text Block
```

## 组件用途和数据来源

| 控件名 | 类型 | 用途 | 数据来源 |
| --- | --- | --- | --- |
| `CardFrame` | `Border` | 卡牌外框；下一张牌会变成高亮色 | C++ 根据 `bIsNextCard` 设置颜色 |
| `CardIcon` | `Image` | 显示符文图标；没有图标时自动隐藏 | `RuneDataAsset -> RuneInfo -> RuneConfig -> RuneIcon` |
| `CardNameText` | `Yog Common Rich Text Block` | 显示卡牌名称 | 优先读 `RuneDataAsset -> RuneInfo -> CombatCard -> DisplayName`，为空时使用 DA 资源名 |
| `ActionText` | `Yog Common Rich Text Block` | 显示动作要求 | `RuneDataAsset -> RuneInfo -> CombatCard -> RequiredAction` |
| `TypeText` | `Yog Common Rich Text Block` | 显示卡牌类型 | `RuneDataAsset -> RuneInfo -> CombatCard -> CardType` |
| `StateText` | `Yog Common Rich Text Block` | 显示下一张牌状态 | 当前槽位是下一张牌时显示 `NEXT` |

## 推荐配置

| 控件 | 关键属性 |
| --- | --- |
| `Root` | `SizeBox -> Width Override=96; Height Override=132` |
| `CardFrame` | `Slot -> Fill/Fill; Padding=0; Brush Color=深灰 (R=0.08, G=0.08, B=0.10, A=0.88)` |
| `WBP_CombatDeckCardSlot` Class Defaults | `NormalCardFrameColor=(0.12,0.12,0.16,0.85); NextCardFrameColor=(0.95,0.72,0.22,0.95); EmptyCardFrameColor=(0.03,0.03,0.04,0.35)` |
| `Overlay 或 VerticalBox` | `Slot -> Fill/Fill; Padding=8` |
| `CardIcon` | `SizeBox -> Width Override=48; Height Override=48; Horizontal Alignment=Center; Visibility=Visible` |
| `CardNameText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=14; OverrideColor=(1,1,1,1); Auto Wrap Text=true; Justification=Center` |
| `ActionText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=11; OverrideColor=(0.78,0.82,0.90,1); Auto Wrap Text=false; Justification=Center` |
| `TypeText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=11; OverrideColor=(0.70,0.78,1.00,1); Auto Wrap Text=false; Justification=Center` |
| `StateText` | `FontStyleClass=BP_InfoPopupTextStyle; OverrideFontSize=12; OverrideColor=(1.00,0.78,0.24,1); Auto Wrap Text=false; Justification=Center` |

## 颜色字段位置

`NormalCardFrameColor / NextCardFrameColor / EmptyCardFrameColor` 不是 Button 控件字段，也不是 `CardFrame` 这个 Border 子控件的字段。

配置位置：

```text
打开 WBP_CombatDeckCardSlot -> Class Defaults -> Combat Deck|Style
```

`CardFrame` 自己只需要配置 Border 的 `Brush Color`。运行时 C++ 会根据普通卡、下一张卡、空槽状态，把上面的颜色写到 `CardFrame`。

## 必须检查

- `CardFrame` 勾选 `Is Variable`。
- `CardIcon` 如果放了，也勾选 `Is Variable`。
- `CardNameText / ActionText / TypeText / StateText` 必须勾选 `Is Variable`。
- 控件名字必须完全一致。

