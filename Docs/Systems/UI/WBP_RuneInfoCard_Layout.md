# WBP_RuneInfoCard 布局规格 v3

> 符文信息卡 Widget — 5 层 Overlay 结构（CardBG → ContentBlocker → CardContent → SelectionBorder）+ 卡外 GenericEffectList 副窗。
> 父类：`URuneInfoCardWidget`（[Source/DevKit/Public/UI/RuneInfoCardWidget.h](../../../Source/DevKit/Public/UI/RuneInfoCardWidget.h)）

---

## 类信息

| 项目 | 内容 |
|------|------|
| WBP 名 | `WBP_RuneInfoCard` |
| 路径 | `Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset` |
| 父类（C++） | `URuneInfoCardWidget`（继承 `UUserWidget`） |
| 使用方 | `WBP_LootSelection`（动态创建，外包 SizeBox+UButton）/ `WBP_BackpackScreen`（BindWidgetOptional） |
| 实例化 | C++ `CreateWidget<URuneInfoCardWidget>(this, RuneCardClass)` → `ShowRune(RuneInfo)` |
| 推荐尺寸 | LootSelection N≤4 时 320×420，N≥5 时 240×360（C++ SizeBox 强制 Override） |

---

## 控件层级（树形）

```
HorizontalBox（Root，HCenter VCenter）
├── Overlay（CardOverlay，HFill VFill，宽高由 LootSelection 的 SizeBox 控制）
│   ├── Image       (CardBG          [BindWidgetOptional]，HFill VFill，Z=0 底层背景)
│   ├── Border      (ContentBlocker  [BindWidgetOptional]，HFill VFill，Z=1 半透明黑遮罩)
│   ├── VerticalBox (CardContent，Padding=14，Z=2 主内容)
│   │   ├── HorizontalBox (Header，PadB=6)
│   │   │   ├── TextBlock (CardName    [BindWidgetOptional]，Bold 18，#FFFFFF，FillRule=Fill)
│   │   │   └── TextBlock (CardUpgrade [BindWidgetOptional]，Bold 14，#FFD966，Auto)
│   │   ├── SizeBox (IconArea，Height=110，PadB=8)
│   │   │   └── Image (CardIcon [BindWidgetOptional]，Center，Brush 由 C++ 动态设)
│   │   ├── CommonRichTextBlock (CardDesc   [BindWidgetOptional]，AutoWrap，#ECECEC，13)
│   │   ├── CommonRichTextBlock (CardEffect [BindWidgetOptional]，Bold，#FFD966，13，PadT=6)
│   │   ├── Spacer (Fill，让 Footer 沉底)
│   │   └── HorizontalBox (Footer，PadT=6)
│   │       ├── CanvasPanel (ShapeGrid    [BindWidgetOptional]，64×64，Auto)
│   │       ├── Spacer (Fill)
│   │       ├── Image     (GoldCostIcon  [BindWidgetOptional]，Auto，金币贴图)
│   │       └── TextBlock (GoldCostText  [BindWidgetOptional]，Bold 14，#FFD966，Auto)
│   └── Border (SelectionBorder [BindWidgetOptional]，HFill VFill，Z=10 选中描边)
└── GenericEffectList (WBP_GenericEffectList [BindWidgetOptional]，PadL=12，Auto)
```

> **Z-order 关键**：UOverlay 的 Z 顺序由**子控件添加顺序**决定 — 第一个添加的在最底层，最后添加的在最顶层。WBP Designer 中**层级树自上而下 = Z 从底到顶**。务必按上面顺序拖入。

---

## 各控件详细属性

### HorizontalBox（Root）

#### Slot（Canvas Slot 或父容器决定）
- 由父容器决定（LootSelection 用 SizeBox 强制宽高；背包页面用 BindWidgetOptional 嵌入）

子节点 Slot：
- CardOverlay：HFill VFill，FillRule=Fill
- GenericEffectList：HCenter VTop，FillRule=Auto，PadL=12

### Overlay（CardOverlay）

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Fill |
| Fill | 1.0 |
| HAlign / VAlign | Fill / Fill |

#### 子节点（注意添加顺序 = Z 顺序）
按 CardBG → ContentBlocker → CardContent → SelectionBorder 依次添加。

### Image（CardBG）⭐ BindWidgetOptional

#### Slot（Overlay Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Fill / Fill |
| Padding | 0 |

#### 自身
| 属性 | 值 |
|---|---|
| Brush | 留空（C++ 自动赋值：有 CardBackground 用纹理，否则用 DefaultCardBGBrush） |
| Visibility | Visible（C++ 切 SelfHitTestInvisible） |

> **不要**在 WBP 里手动设 Brush — C++ `ShowRune` 会覆盖。

### Border（ContentBlocker）⭐ BindWidgetOptional

#### Slot（Overlay Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Fill / Fill |
| Padding | 0 |

#### 自身
| 属性              | 值                                  |
| --------------- | ---------------------------------- |
| Brush → Tint    | #000000 α=0.35（线性 (0, 0, 0, 0.35)） |
| Brush → Draw As | Box                                |
| Visibility      | HitTestInvisible（不挡卡片点击）           |
| Padding         | 0                                  |

> **作用**：盖在 CardBG 之上、CardContent 之下，把背景纹理压暗确保白字可读。alpha 0.25~0.45 自调。

### VerticalBox（CardContent）

#### Slot（Overlay Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Fill / Fill |
| Padding | 14 |

子节点 Slot 默认 Auto，按需调整 PadT/PadB。

### HorizontalBox（Header）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| Padding B | 6 |

#### 子控件
- CardName：FillRule=Fill，VAlign=Center
- CardUpgrade：FillRule=Auto，VAlign=Center

### TextBlock（CardName）⭐ BindWidgetOptional

| 属性 | 值 |
|---|---|
| Text | 留空（C++ ShowRune 设） |
| Font Size | 18 |
| Typeface | Bold |
| Color and Opacity | #FFFFFF |
| Justification | Left |

### TextBlock（CardUpgrade）⭐ BindWidgetOptional

| 属性 | 值 |
|---|---|
| Text | 留空（C++ 设 "Lv.II" / "Lv.III"，UpgradeLevel=0 时 Collapsed） |
| Font Size | 14 |
| Typeface | Bold |
| Color and Opacity | #FFD966（金黄）|
| Justification | Right |

### SizeBox（IconArea）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| Padding B | 8 |

#### 自身
| 属性 | 值 |
|---|---|
| Override Height | 110 |
| Override Width | （不设，跟随父宽） |

### Image（CardIcon）⭐ BindWidgetOptional

#### Slot（SizeBox Slot）
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Center / Center |

#### 自身
| 属性 | 值 |
|---|---|
| Brush | 留空（C++ 设 RuneIcon 纹理，无图标时 Collapsed） |

### CommonRichTextBlock（CardDesc）⭐ BindWidgetOptional

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |

#### 自身
| 属性                                      | 值                                 |
| --------------------------------------- | --------------------------------- |
| Text                                    | 留空（C++ 设 RuneDescription）         |
| Decorator Classes [0]                   | `BP_InputActionDecorator`         |
| Decorator Classes [1]                   | `BP_KeywordDecorator` ⭐ **新增**    |
| Yog Style Override → Font Style Class   | `BP_InfoPopupTextStyle`（中文字体）     |
| Yog Style Override → Override Font Size | `13`                              |
| Yog Style Override → Override Color     | `(R=0.93, G=0.93, B=0.93, A=1.0)` |
| Auto Wrap Text                          | true                              |
| Justification                           | Left                              |

### CommonRichTextBlock（CardEffect）⭐ BindWidgetOptional

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| Padding T | 6 |

#### 自身
| 属性                                      | 值                                                       |
| --------------------------------------- | ------------------------------------------------------- |
| Text                                    | 留空（C++ 设 GenericEffects 关键词列表，如 "击退 · 燃烧"；空时 Collapsed） |
| Decorator Classes [0]                   | `BP_InputActionDecorator`                               |
| Decorator Classes [1]                   | `BP_KeywordDecorator` ⭐ **新增**                          |
| Yog Style Override → Font Style Class   | `BP_InfoPopupTextStyle`                                 |
| Yog Style Override → Override Font Size | `13`                                                    |
| Yog Style Override → Override Color     | `(R=1.0, G=0.85, B=0.4, A=1.0)`（#FFD966 金黄）             |
| Yog Style Override → Override Typeface  | `Bold`                                                  |
| Justification                           | Left                                                    |

> **CardEffect 颜色由 WBP 配置**（C++ 不调 SetColorAndOpacity）。这里建议金黄+加粗以区分关键词列表。

### Spacer

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Fill |
| Fill | 1.0 |

> 作用：把 Footer 推到底部。

### HorizontalBox（Footer）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| Padding T | 6 |

#### 子控件
- ShapeGrid：FillRule=Auto，VAlign=Bottom
- Spacer：FillRule=Fill
- GoldCostIcon：FillRule=Auto，VAlign=Center，PadR=4
- GoldCostText：FillRule=Auto，VAlign=Center

### CanvasPanel（ShapeGrid）⭐ BindWidgetOptional

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |

#### 自身
| 属性 | 值 |
|---|---|
| Size | 64 × 64（建议在 Designer 显式设固定大小，避免 0 宽）|

> 子节点由 C++ `BuildShapeGrid` 动态生成 — Designer 不要手动加任何子节点。

### Image（GoldCostIcon）⭐ BindWidgetOptional

| 属性 | 值 |
|---|---|
| Brush → Resource | 金币贴图（金币图标资产路径，由用户自配）|
| Brush → Image Size | 18 × 18 |
| Visibility | Collapsed（C++ 在 GoldCost > 0 时切 SelfHitTestInvisible） |

### TextBlock（GoldCostText）⭐ BindWidgetOptional

| 属性 | 值 |
|---|---|
| Text | 留空（C++ 设 "{N} G"，GoldCost=0 时 Collapsed） |
| Font Size | 14 |
| Typeface | Bold |
| Color and Opacity | #FFD966 |
| Justification | Left |

### Border（SelectionBorder）⭐ BindWidgetOptional

#### Slot（Overlay Slot）— **必须最后添加，确保 Z 在最顶**
| 属性 | 值 |
|---|---|
| HAlign / VAlign | Fill / Fill |
| Padding | -2（向外扩 2px，让描边明显） |

#### 自身
| 属性 | 值 |
|---|---|
| Brush → Tint | #FFD966 α=1.0（金黄）|
| Brush → Draw As | Box（启用 9-slice 描边） |
| Brush → Margin | (0.45, 0.45, 0.45, 0.45)（保留中心透明）|
| Brush → Image | 留空（用纯色 9-slice）或自配描边贴图 |
| Visibility | Hidden（C++ 默认隐藏，SetSelected(true) 切 HitTestInvisible） |

> **C++ 不主动 SetVisibility(Hidden) 在 NativeConstruct**——要看 NativeConstruct 实现，目前会强制 Hidden 一次。

### GenericEffectList（WBP_GenericEffectList）⭐ BindWidgetOptional

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign / VAlign | Left / Top |
| Padding L | 12 |

#### 自身
- 这是一个 WBP 实例（父类 `UGenericEffectListWidget`）
- C++ `SetGenericEffectsExpanded(true)` 时自动展开，否则 Collapsed
- 详见 [WBP_GenericEffectList 文档](#)（待补 / 直接看 WBP）

---

## BindWidget 汇总

### 都是 BindWidgetOptional（缺失不报错，但功能不全）

| C++ 变量名 | 控件类型 | WBP 控件名 | 说明 |
|---|---|---|---|
| `CardBG` | UImage | `CardBG` | C++ ShowRune 自动赋 Brush |
| `ContentBlocker` | UBorder | `ContentBlocker` | 暗化遮罩，纯视觉（C++ 不操作）|
| `CardName` | UTextBlock | `CardName` | C++ 设 RuneName |
| `CardIcon` | UImage | `CardIcon` | C++ 设 RuneIcon 或 Collapsed |
| `CardDesc` | UCommonRichTextBlock | `CardDesc` | C++ 设 RuneDescription |
| `CardEffect` | UCommonRichTextBlock | `CardEffect` | C++ 设 GenericEffects 关键词列表 |
| `CardUpgrade` | UTextBlock | `CardUpgrade` | C++ 设 "Lv.X" 或 Collapsed |
| `ShapeGrid` | UCanvasPanel | `ShapeGrid` | C++ BuildShapeGrid 动态填充 |
| `GoldCostIcon` | UImage | `GoldCostIcon` | C++ 控制显隐 |
| `GoldCostText` | UTextBlock | `GoldCostText` | C++ 设 "{N} G" 或 Collapsed |
| `SelectionBorder` | UBorder | `SelectionBorder` | C++ SetSelected(true/false) 切显隐 |
| `GenericEffectList` | UGenericEffectListWidget | `GenericEffectList` | C++ SetGenericEffectsExpanded 控制 |

> 命名一字不差，区分大小写。

---

## Class Defaults 配置

| 字段 | 默认值 | 说明 |
|---|---|---|
| `DefaultCardBGBrush.Tint` | #1A1A22 α=0.9 | C++ 构造函数已设；CardBackground 留空时用此色 |
| `DefaultCardBGBrush.DrawAs` | Box | C++ 构造函数已设 |
| `SelectedRenderScale` | 1.06 | 选中时整卡放大比例 |
| `ScaleInterpSpeed` | 12.0 | 选中插值速度（FInterpTo Speed）|

---

## 颜色速查

| 元素 | Hex | Linear RGBA |
|---|---|---|
| 默认卡背 DefaultCardBG | #1A1A22 α=0.9 | (0.102, 0.102, 0.133, 0.9) |
| 内容遮罩 ContentBlocker | #000000 α=0.35 | (0, 0, 0, 0.35) |
| 主标题 CardName | #FFFFFF | (1, 1, 1, 1) |
| 描述文字 CardDesc | #ECECEC | (0.93, 0.93, 0.93, 1) |
| 关键词高亮 CardEffect / KeywordColor / SelectionBorder / GoldCostText / CardUpgrade | #FFD966 | (1, 0.85, 0.4, 1) |
| ShapeGrid 已占格 | #3399FF | (0.20, 0.60, 1.00, 1) |
| ShapeGrid 空格 | #2E2E38 α=0.6 | (0.18, 0.18, 0.22, 0.6) |

---

## 注意事项

### Overlay Z-order = 子节点添加顺序

WBP Designer 的层级树**自上而下** = Z **从底到顶**。务必按 CardBG → ContentBlocker → CardContent → SelectionBorder 顺序添加，否则会出现"白框盖住文字"等遮挡问题。

### CommonRichTextBlock vs RichTextBlock

CardDesc / CardEffect 必须用 **CommonRichTextBlock**（非引擎自带 RichTextBlock）。CommonRichTextBlock 提供 `Yog Style Override` 字段统一管理字体/字号/颜色，与项目其他 RichText 控件一致。

### Decorator Classes 数组

CardDesc / CardEffect 都需要 2 个 decorator：
- [0] `BP_InputActionDecorator` — `<input action="X"/>` 按键图标
- [1] `BP_KeywordDecorator` — `<key>击退</key>` 关键词高亮

顺序无关紧要（不同 tag name 不冲突），但建议保持一致。

### CardBG 不要在 WBP 里设 Brush

C++ `ShowRune` 会强制覆盖 CardBG 的 Brush（要么用 RuneConfig.CardBackground 纹理，要么用 DefaultCardBGBrush）。在 WBP 里设 Brush 没有意义，反而会让人困惑。

### SelectionBorder Padding=-2 让描边外扩

把 SelectionBorder 的 Slot Padding 设为 -2（四边外扩 2px），描边会画在卡片边缘外部，避免遮住卡内内容。也可以保持 0，描边贴在卡片边缘。

### CardEffect 颜色不要由 C++ 控制

C++ `ShowRune` 不调 `CardEffect->SetColorAndOpacity` — 所有视觉样式由 WBP 端统一配置（Yog Style Override）。这样美术调色不需要改 C++。

### GenericEffectList 是 WBP 实例不是 C++ 类

`GenericEffectList` 字段类型是 `UGenericEffectListWidget`，但在 WBP Designer 里要拖入 `WBP_GenericEffectList`（这是个 WBP 资产，父类是 `UGenericEffectListWidget`）。命名 = `GenericEffectList`。

---

## 验证流程

### 1. 编译
1. 重写 WBP 完成后保存 → 编译 BP（确保无报错）
2. 关闭重开编辑器（避免 BindWidget 缓存问题）

### 2. 单卡显示（背包页面）
1. 进入 Backpack
2. 拖一个符文上去 → 悬浮 → ✅ RuneInfoCard 显示：
   - 背景是 RuneConfig.CardBackground 纹理（或默认暗灰）
   - 顶部 CardName + CardUpgrade（如 Lv.III）
   - 中间 CardIcon
   - 下方 CardDesc 描述文本
   - 描述下方 CardEffect 显示通用效果关键词（如 "击退 · 燃烧"）
   - 底部左侧 ShapeGrid 64×64 点阵
   - 底部右侧 GoldCostIcon + GoldCostText（"50 G"）

### 3. 选中态高亮（LootSelection）
1. 进 PIE → 触发 LootSelection
2. DPad 移动焦点到不同卡 → ✅ 选中卡：
   - SelectionBorder 显示金黄描边
   - 整卡缩放至 1.06
   - 卡侧 GenericEffectList 副窗展开

### 4. 关键词高亮（DA 编辑）
1. 找一个有 GenericEffects 的 DA（如 DA_Rune_DashAttack）
2. 编辑 RuneDescription：`"冲刺攻击造成 10 点伤害，并附加 <key>击退</key>"`
3. 触发 LootSelection 显示该卡 → ✅ "击退" 字样以金黄加粗显示

### 5. 边界
- GoldCost = 0 → ✅ GoldCostIcon + GoldCostText 都 Collapsed
- GenericEffects = [] → ✅ CardEffect Collapsed
- UpgradeLevel = 0 → ✅ CardUpgrade Collapsed
- CardBackground 留空 → ✅ CardBG 显示 DefaultCardBGBrush 暗灰

---

## 排错速查

| 现象 | 原因 | 解决 |
|---|---|---|
| 卡片白框盖住文字 | Overlay 子节点顺序错（CardBG/ContentBlocker 在 CardContent 之后）| 按 BG→Blocker→Content→Border 顺序重新排列 |
| 卡片背景全黑透明 | 没设 ContentBlocker 或 alpha 太高 | ContentBlocker.Tint α=0.35 左右 |
| `<key>击退</key>` 原样显示标签 | 没加 BP_KeywordDecorator | CardDesc/CardEffect 的 Decorator Classes [+] BP_KeywordDecorator |
| 关键词颜色没变 | KeywordDecorator 没派生项目字体样式 | 检查 BP_KeywordDecorator Class Defaults: KeywordColor、bBold |
| 关键词没加粗 | 项目中文字体没 Bold typeface | 换字体资产或在 KeywordColor 上做差异（颜色变化即可识别）|
| 选中态边框没出现 | SelectionBorder 没绑定 / 没设 Brush | 命名 SelectionBorder + Brush.Tint 设金黄 + DrawAs=Box |
| 卡片选中后没缩放 | RenderTransformPivot 没设 0.5/0.5 | C++ NativeConstruct 已自动设；如果是手动覆盖了，删掉手动设置 |
| GoldCost 显示 0 G | 没判断 > 0 / WBP 没绑 GoldCostText | C++ 已自动 Collapse；检查 BindWidget 命名 |
| ShapeGrid 没显示 | CanvasPanel 没固定大小 | Designer 给 ShapeGrid 设 Size 64×64 |

---

## 关联文档

- [RuneInfoCard C++ 接口](../../../Source/DevKit/Public/UI/RuneInfoCardWidget.h)
- [WBP_LootSelection 布局](WBP_LootSelection_Layout.md) — 多卡容器
- [WBP_GenericEffectList](../../../Content/UI/Playtest_UI/Runes/WBP_GenericEffectList.uasset) — 卡侧副窗
- [BP_InputActionDecorator](../../../Content/Docs/UI/Tutorial/BP_InputActionDecorator.uasset) — 已有按键图标
- [BP_KeywordDecorator](../../../Content/Docs/UI/Tutorial/BP_KeywordDecorator.uasset) — **本次新建**关键词高亮
- [当前任务方案 v3](../../WorkSession/current_plan.md)
- [Phase B 制作流程](../../WorkSession/PhaseB_Workflow.md) — 编辑器侧的逐步操作
- [WBP 布局规格标准](WBP_LayoutSpec_Standard.md)
