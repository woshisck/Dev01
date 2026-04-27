# Phase B 编辑器侧制作流程

> 配套 Phase A C++ 改动（已完成 + 编译通过）。本文档是 WBP / DA / BP 资产层的逐步操作指南。

---

## 概览

| #   | 任务                                                            | 资产                                                      | 预估时间     |
| --- | ------------------------------------------------------------- | ------------------------------------------------------- | -------- |
| B1  | 重做 WBP_RuneInfoCard（5 层 Overlay 结构）                           | `Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset`  | 30~60 分钟 |
| B2  | WBP_LootSelection 的 CardContainer 改 WrapBox                   | `Content/UI/Playtest_UI/WBP_LootSelection.uasset`       | 5 分钟     |
| B3  | 新建 BP_KeywordDecorator                                        | `Content/Docs/UI/Tutorial/BP_KeywordDecorator.uasset`   | 2 分钟     |
| B4  | CardDesc/CardEffect 加 BP_KeywordDecorator 到 Decorator Classes | （在 B1 完成时一并做）                                           | 1 分钟     |
| B5  | 选 2~3 个示范 DA 加 `<key>...</key>` 标签                            | `Content/Docs/BuffDocs/Playtest_GA/**/DA_Rune_*.uasset` | 5 分钟     |

**总耗时：** 约 50~75 分钟

---

## 前置准备

1. **打开编辑器** — 双击 `DevKit.uproject`
2. **确认本次 C++ 改动已加载** — 编辑器启动后查 Output Log，搜索 `URuneInfoCardWidget` / `UYogKeywordRichTextDecorator` 关键字应该能找到它们（说明类已注册）
3. **关闭所有打开的 WBP** — 避免在 Designer 里切类型时缓存不一致

---

## B1：重做 WBP_RuneInfoCard

详细布局规格见 [WBP_RuneInfoCard_Layout.md](../Systems/UI/WBP_RuneInfoCard_Layout.md)。这里是逐步操作。

### B1.1 清空旧内容

1. Content Browser 打开 `Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset`
2. 切到 **Graph** 视图 → **Event Graph**
3. **删除所有 BP 节点**（如有 OnConstruction、OnInitialized 等节点全删）
4. 切回 **Designer**
5. 在层级树（左下）选根控件 → **Delete** 所有子节点（保留根 Canvas）

> 如果删除时报错"Variable XXX is in use"，说明 Event Graph 还有引用，回到 Graph 删干净。

### B1.2 搭建根布局

1. 层级树根 = `[CanvasPanel]`（保持默认）
2. 拖一个 **HorizontalBox** 到根 Canvas，命名 `Root`
3. Root 的 Slot：
   - Anchors = (0, 0) ~ (1, 1)（撑满）
   - Position X/Y = 0/0，Size = 撑开
   - HAlign = Center, VAlign = Center

### B1.3 搭建 Overlay 层（5 层 Z-order）

> ⚠️ **关键**：UOverlay 的 Z 顺序 = 子节点添加顺序。**层级树自上而下 = Z 从底到顶**。务必按下面顺序依次添加。

1. Root 内拖入 **Overlay**，命名 `CardOverlay`
   - Slot: Size Rule = Fill, Fill = 1.0, HAlign/VAlign = Fill

2. **第 1 层（最底）**：CardOverlay 内拖入 **Image**，命名 `CardBG`
   - Slot: HAlign/VAlign = Fill
   - Brush: 留空（C++ 自动赋值）
   - Variable ✅ 勾选（让 C++ BindWidget 找得到）

3. **第 2 层**：CardOverlay 内拖入 **Border**，命名 `ContentBlocker`
   - Slot: HAlign/VAlign = Fill, Padding = 0
   - Brush: Tint = (0, 0, 0, 0.35), Draw As = Box
   - Visibility = HitTestInvisible
   - Variable ✅

4. **第 3 层**：CardOverlay 内拖入 **VerticalBox**，命名 `CardContent`
   - Slot: HAlign/VAlign = Fill, Padding = 14
   - Variable ❌（不需要 BindWidget）
   - 子节点见 B1.4

5. **第 4 层（最顶）**：CardOverlay 内拖入 **Border**，命名 `SelectionBorder`
   - Slot: HAlign/VAlign = Fill, Padding = -2
   - Brush: Tint = (1, 0.85, 0.4, 1) #FFD966, Draw As = Box, Margin = (0.45, 0.45, 0.45, 0.45)
   - Visibility = Hidden（C++ NativeConstruct 也会设一次）
   - Variable ✅

### B1.4 搭建 CardContent 内容

CardContent (VerticalBox) 内按顺序拖入：

1. **HorizontalBox** `Header` （PadB=6）
   - 内拖入 **TextBlock** `CardName`：FillRule=Fill, VAlign=Center, Font Size 18 Bold #FFFFFF, ✅ Variable
   - 内拖入 **TextBlock** `CardUpgrade`：FillRule=Auto, VAlign=Center, Font Size 14 Bold #FFD966, Justification=Right, ✅ Variable

2. **SizeBox** `IconArea` （Size Rule=Auto, PadB=8, Override Height=110）
   - 内拖入 **Image** `CardIcon`：HAlign/VAlign=Center, Brush 留空, ✅ Variable

3. **CommonRichTextBlock** `CardDesc` （Size Rule=Auto）
   - Decorator Classes [+]：（B4 步骤里加 2 个 decorator）
   - Yog Style Override → Font Style Class = `BP_InfoPopupTextStyle`
   - Yog Style Override → Override Font Size = 13
   - Yog Style Override → Override Color = (0.93, 0.93, 0.93, 1)
   - Auto Wrap Text = true
   - ✅ Variable

4. **CommonRichTextBlock** `CardEffect` （Size Rule=Auto, PadT=6）
   - Decorator Classes [+]：（B4 步骤里加）
   - Yog Style Override → Font Style Class = `BP_InfoPopupTextStyle`
   - Yog Style Override → Override Font Size = 13
   - Yog Style Override → Override Color = (1, 0.85, 0.4, 1)
   - Yog Style Override → Override Typeface = `Bold`
   - ✅ Variable

5. **Spacer** （Size Rule=Fill, Fill=1.0） — 把 Footer 推到底

6. **HorizontalBox** `Footer` （Size Rule=Auto, PadT=6）
   - 内拖入 **CanvasPanel** `ShapeGrid`：FillRule=Auto, Size=64×64（**必须显式设大小**），✅ Variable
   - 内拖入 **Spacer**：FillRule=Fill
   - 内拖入 **Image** `GoldCostIcon`：FillRule=Auto, VAlign=Center, PadR=4, Brush.Resource=金币贴图（自配）, Image Size=18×18, Visibility=Collapsed, ✅ Variable
   - 内拖入 **TextBlock** `GoldCostText`：FillRule=Auto, VAlign=Center, Font Size 14 Bold #FFD966, ✅ Variable

### B1.5 搭建 GenericEffectList 副窗（卡外）

1. Root (HorizontalBox) 内拖入一个 **WBP_GenericEffectList 实例**（用户控件），命名 `GenericEffectList`
   - Slot: Size Rule=Auto, HAlign=Left, VAlign=Top, PadL=12
   - ✅ Variable

> 如果 WBP_GenericEffectList 没有现成实例，先在 Content Browser 找到 `Content/UI/Playtest_UI/Runes/WBP_GenericEffectList.uasset` 拖进来即可。

### B1.6 编译保存

1. Designer 上方 **Compile** 按钮 → 看输出无 Error
2. **Save**
3. 关闭再重开 WBP_RuneInfoCard，确认所有 BindWidget 字段都正常显示（左侧 Variables 面板能看到 CardBG / ContentBlocker / CardName / ... / SelectionBorder / GenericEffectList 等）

---

## B2：WBP_LootSelection 的 CardContainer 改 WrapBox

详细规格见 [WBP_LootSelection_Layout.md](../Systems/UI/WBP_LootSelection_Layout.md)。

### B2.1 删除旧 CardContainer

1. 打开 `Content/UI/Playtest_UI/WBP_LootSelection.uasset`
2. **Graph → Event Graph** 检查所有引用 `CardContainer` 的节点（应该没有，C++ 全管），有则删
3. 切到 **Designer**
4. 层级树找到 `CardContainer`（之前是 HorizontalBox） → **删除**

### B2.2 新建 WrapBox 替换

1. 在原位置（CenterColumn VerticalBox 内，TitleText 之下、BottomBar 之上）拖入 **WrapBox**
2. 命名 = `CardContainer`（一字不差）
3. ✅ **Is Variable** 必须勾上
4. **Slot（VBox Slot）**:
   - Size Rule = Auto
   - HAlign = Center, VAlign = Center
   - Padding B = 24
5. **自身属性**:
   - Orientation = Horizontal
   - Inner Slot Padding = (0, 0)
   - Horizontal Alignment = Center
   - Wrap Size = 9999（默认大值；C++ 动态切）
   - Use Explicit Wrap Size = false（C++ 动态切）

### B2.3 编译保存

1. Compile → 看是否报 BindWidget 类型校验通过
2. Save

> 如果 Compile 报 `BindWidget CardContainer of type UWrapBox not found`，检查 (a) 命名是否完全一致 (b) 是否勾了 Is Variable (c) 类型是否真的是 WrapBox 不是 HorizontalBox。

---

## B3：新建 BP_KeywordDecorator

### B3.1 创建蓝图

1. Content Browser 进入 `Content/Docs/UI/Tutorial/`
2. 右键 → **Blueprint Class**
3. 搜索父类 = `YogKeywordRichTextDecorator`（C++ 类，本次新建）
4. 命名 = `BP_KeywordDecorator`
5. 双击打开

### B3.2 配置 Class Defaults

在 Class Defaults 面板（Details）找到 **Keyword** 分组，按需调：

| 字段 | 默认值 | 说明 |
|---|---|---|
| Keyword Color | (1, 0.85, 0.4, 1) #FFD966 | 关键词颜色（金黄）|
| b Bold | true | 加粗（要求字体资产含 Bold typeface） |
| Font Size Override | 0 | 0 = 沿用控件字号；想放大可填如 16 |

### B3.3 保存

Save → 关闭。

---

## B4：CardDesc / CardEffect 加 Decorator Classes

> 这一步如果在 B1.4 已经做过就跳过。

1. 打开 `WBP_RuneInfoCard`
2. 选 `CardDesc` 控件 → Details → 找到 **Decorator Classes** 数组
3. **Decorator Classes [0]** = `BP_InputActionDecorator`（已有）
4. **Decorator Classes [1]** = `BP_KeywordDecorator`（B3 新建的）
5. 同样操作 `CardEffect`

### 验证

预览面板（或 PIE）看 RichText 应能识别两类标签：
- `<input action="LightAttack"/>` → 显示按键图标
- `<key>击退</key>` → 金黄加粗文字

---

## B5：DA 编辑规范 — `<key>` 标签

### B5.1 编辑规范

DA 的 `RuneConfig.RuneDescription` 字段里，凡是引用了 GenericEffects 的关键词都用 `<key>...</key>` 包起来：

**示例**

| DA                     | 旧 RuneDescription | 新 RuneDescription                               |
| ---------------------- | ----------------- | ----------------------------------------------- |
| DA_Rune_DashAttack     | "冲刺攻击造成额外伤害并附加击退" | "冲刺攻击造成额外伤害并附加 `<key>击退</key>`"                 |
| DA_Rune_BurnMark       | "命中时附加燃烧印记"       | "命中时附加 `<key>燃烧</key>` 印记"                      |
| DA_Rune_FrostShard     | "造成 5 点冰冻伤害并击退目标" | "造成 5 点 `<key>冰冻</key>` 伤害并 `<key>击退</key>` 目标" |
| DA_Rune_ChainLightning | "闪电链最多跳 3 次"      | "闪电链最多跳 3 次（链路中无关键词时不加标签）"                      |

### B5.2 操作步骤

1. Content Browser 进入 `Content/Docs/BuffDocs/Playtest_GA/` 找几个 DA_Rune_*
2. 双击打开
3. RuneConfig → RuneDescription → 改成上面格式
4. 同时确认 RuneConfig.GenericEffects 数组里有对应的 DA_RGE_*（如 DA_RGE_Knockback / DA_RGE_Burn）
5. Save

> **没有 GenericEffects 引用就不用加 `<key>` 标签**。`<key>` 是为关键词提供视觉强调，不是必须的；标签内容必须能在 GenericEffects 里找到对应的 DA，否则副窗不会展开解释。

### B5.3 兼容性

- 不加标签的旧 DA 仍正常显示（RichText 识别不到的标签按普通文字渲染）
- `<key>` 拼写错误（如 `<keyword>`）会原样显示标签，提示拼错

---

## 验证流程

完成 B1~B5 后，做以下 PIE 验证：

### V1：单卡 + 关键词高亮

1. Backpack 里悬浮一个含 GenericEffects 的符文 → ✅ RuneInfoCard 显示完整：
   - 5 层 Z-order 正确（无白框遮挡文字）
   - CardDesc 里 `<key>击退</key>` 显示金黄加粗
   - CardEffect 显示 "击退 · 燃烧" 关键词列表
   - GoldCost 正常 / 0 时 Collapsed
   - ShapeGrid 64×64 点阵正常

### V2：3 选 1（N≤4 大卡）

1. PIE 触发 LootSelection（N=3 LootOptions）→ ✅ 3 张大卡（320×420）单行居中
2. DPad 移动焦点 → ✅ 选中卡 SelectionBorder 金黄描边 + 1.06 倍缩放
3. 选中卡 GenericEffectList 卡侧展开

### V3：6 选 1（N≥5 小卡 2×3）

1. 临时改 GameMode 测试代码 / 用 console 命令让 N=6
2. ✅ 6 张小卡（240×360）= 3+3 双行
3. 第二行水平居中（不是左对齐）

### V4：N=5（3+2 排列）

1. N=5 → ✅ 第一行 3 张，第二行 2 张
2. 第二行 2 张水平居中

### V5：段切换 + UX

1. 卡片段 → DPad Down 切到按钮段 → ✅ 选中卡的 GenericEffectList 自动收起
2. DPad Up 回卡片段 → ✅ 当前焦点卡的 GenericEffectList 重新展开

### V6：边界

- 配 7 个 LootOptions → ✅ 截断 6 张 + Output Log 警告
- 配 6 个但其中 3 个无效 → ✅ 显示 3 张大卡（按 ValidCount 算）
- DA 不加 `<key>` 标签 → ✅ 正常显示纯文本，无视觉异常

---

## 常见问题

**Q：编辑器启动时 WBP 报 BindWidget CardContainer not found**
A：B2 没做或 WrapBox 命名错。打开 WBP_LootSelection 检查 CardContainer 是否是 WrapBox 类型 + 名字一字不差。

**Q：关键词没加粗，只变了颜色**
A：项目中文字体没 Bold typeface。两种解决：(a) BP_KeywordDecorator 把 b Bold 关掉，只用颜色区分；(b) 换字体资产（在 Project Settings → Engine → General Settings 找字体路径）。

**Q：5 张卡第二行左对齐**
A：检查 (a) WBP CardContainer 自身 Horizontal Alignment 是不是 Center；(b) C++ RebuildCards 已强制设 SetHorizontalAlignment(HAlign_Center)，如果 BP 硬覆盖了就看 BP 端是否动了 SetHorizontalAlignment。

**Q：白框还是盖在文字上**
A：CardOverlay 子节点添加顺序错了。Designer 层级树**自上而下** = Z 从底到顶。确保顺序是：
```
CardOverlay
├── CardBG (最上 = 最底层)
├── ContentBlocker
├── CardContent
└── SelectionBorder (最下 = 最顶层)
```
顺序错就在层级树里**拖动节点**重新排列。

**Q：选中态边框出现但卡片没缩放**
A：检查 RuneInfoCard 自身 Render Transform Pivot 是否被 BP 覆盖了（C++ NativeConstruct 设 0.5/0.5）。如果 BP 子类的 PreConstruct 改了，删除该改动。

**Q：CardEffect 一直显示 RuneDescription 而不是 GenericEffects 关键词**
A：确认 C++ 已重新编译加载（启动 Output Log 看是否有新版 ShowRune 日志）。如果 C++ 没重编，关闭编辑器 → 跑编译 → 重开。

---

## 关联文档

- [WBP_RuneInfoCard 完整布局规格](../Systems/UI/WBP_RuneInfoCard_Layout.md)
- [WBP_LootSelection 完整布局规格](../Systems/UI/WBP_LootSelection_Layout.md)
- [当前任务方案 v3](current_plan.md)
- [Codex 方案审查](codex_plan_review.md)
- [Codex 代码审查（最终通过版）](codex_code_review.md)
- C++ 接口：[RuneInfoCardWidget.h](../../Source/DevKit/Public/UI/RuneInfoCardWidget.h) / [LootSelectionWidget.h](../../Source/DevKit/Public/UI/LootSelectionWidget.h) / [YogKeywordRichTextDecorator.h](../../Source/DevKit/Public/UI/YogKeywordRichTextDecorator.h)
