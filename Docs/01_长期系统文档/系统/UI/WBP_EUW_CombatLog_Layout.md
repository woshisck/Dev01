# WBP 布局规格 — EUW_CombatLog

> UI 优化参考文档。技术架构见 [EUW_CombatLog_Technical.md](EUW_CombatLog_Technical.md)

---

## 类信息

| 项目 | 内容 |
|------|------|
| 资产名 | `EUW_CombatLog` |
| 资产类型 | Editor Utility Widget Blueprint |
| 父类（C++） | `UCombatLogWidget` |
| 打开方式 | 菜单 → Window → Editor Utility Widgets → EUW_CombatLog（或右键资产 → Run） |
| 推荐存放位置 | `Content/UI/DebugUI/EUW_CombatLog` |

---

## BindWidget 汇总表

这三个控件名称必须与 C++ 变量名完全一致，否则运行时无法绑定。

| C++ 变量名 | 控件类型 | 绑定类型 | 说明 |
|-----------|---------|---------|------|
| `FilterButtonBox` | WrapBox | BindWidgetOptional | 过滤按钮容器；未绑定时 C++ 自动创建，但位置不可控 |
| `LogScrollBox` | ScrollBox | BindWidgetOptional | 日志行滚动区域 |
| `SummaryText` | TextBlock | BindWidgetOptional | 摘要统计文字块 |

**重要**：`BindWidgetOptional` 未绑定也不报错，但无法显示对应功能。建议三个都手动放置。

---

## 推荐控件层级

```
CanvasPanel（根）
├── WrapBox（FilterButtonBox [BindWidgetOptional]，过滤按钮行）
├── ScrollBox（LogScrollBox [BindWidgetOptional]，日志主区域）
├── TextBlock（SummaryText [BindWidgetOptional]，摘要统计）
└── Button（BtnReset，重置按钮）
    └── TextBlock（"重置"）
```

---

## 各控件详细属性

### FilterButtonBox（过滤按钮行）

| 属性 | 推荐值 |
|------|--------|
| 控件类型 | WrapBox |
| **变量名（必须）** | `FilterButtonBox` |
| Canvas Anchors | Min(0,0) Max(1,0) — 顶部横展 |
| Canvas Position | (0, 0) |
| Canvas Size | (0, 40) — 高 40px，宽随锚点自适应 |
| Orientation | Horizontal |
| Inner Slot Padding | (4, 0) — 按钮间距 |
| 背景 | 无（透明），或加 Border/Image 作背景 |

> **C++ 自动生成的按钮**（无法在 Designer 看到）：C++ 会在 `BuildFilterButtons()` 里为 9 个过滤器各创建一个 `UButton + UTextBlock`，颜色见下方颜色速查表。如果想自定义样式，可以不依赖自动生成，直接在 Blueprint 里手动放置按钮并在 EventGraph 调用 `SetFilter(ECombatLogFilter)`。

---

### LogScrollBox（日志滚动区域）

| 属性 | 推荐值 |
|------|--------|
| 控件类型 | ScrollBox |
| **变量名（必须）** | `LogScrollBox` |
| Canvas Anchors | Min(0,0) Max(1,1) — 填满剩余空间（需配合 Top Offset 留出按钮和摘要高度） |
| Canvas Position | (0, 44) |
| Canvas Size Offset | (0, -44-120) — 上留过滤行，下留摘要区 |
| Orientation | Vertical |
| Scroll Bar Visibility | Auto |
| Always Show Scrollbar | false |

---

### SummaryText（摘要统计）

| 属性 | 推荐值 |
|------|--------|
| 控件类型 | TextBlock |
| **变量名（必须）** | `SummaryText` |
| Canvas Anchors | Min(0,1) Max(1,1) — 底部横展 |
| Canvas Position | (0, -120) |
| Canvas Size | (0, 116) |
| Justification | Left |
| Font Size | 10 |
| Font Color | (0.8, 0.8, 0.8, 1.0) — 灰白 |
| Auto Wrap Text | true |
| 背景建议 | 加 Border，颜色 (#1A1A1A, Alpha=0.6) 作底部统计区背景 |

> 摘要文本格式（C++ 自动生成）：
> ```
> -- 卡牌统计 --
> 消耗: 9次 | 命中: 12次 | 匹配: 7次 | 连携: 5次 | 终结技: 2次 | 洗牌: 3次
> -- 伤害统计 --
> 普通: 350(18次) | 暴击: 280(6次) | 符文: 120(4次) | 流血: 80(12次)
> 卡牌: 520(12次) | 终结技: 320(2次) | 连携: 90(5次)
> 总计: 1760
> ```

---

### BtnReset（重置按钮，可选）

| 属性 | 推荐值 |
|------|--------|
| 控件类型 | Button |
| Canvas Anchors | Min(1,0) Max(1,0) — 右上角 |
| Canvas Position | (-60, 4) |
| Canvas Size | (56, 32) |
| OnClicked | → Graph → Call `ResetLog()` |
| 标签文字 | "重置" |
| 字号 | 10 |

---

## 颜色速查表

### 过滤按钮标签颜色（C++ 自动生成）

| 按钮 | 颜色 (R,G,B) |
|------|-------------|
| 全部 | (1.0, 1.0, 1.0) 白 |
| 普通 | (0.7, 0.7, 0.7) 灰 |
| 暴击 | (1.0, 0.9, 0.1) 黄 |
| 符文 | (0.8, 0.4, 1.0) 紫 |
| 流血 | (1.0, 0.2, 0.2) 红 |
| 卡牌 | (0.4, 0.7, 1.0) 淡蓝 |
| 终结技 | (1.0, 0.85, 0.0) 金 |
| 连携 | (1.0, 0.55, 0.1) 橙 |
| 洗牌 | (0.5, 0.7, 1.0) 灰蓝 |

### 日志行颜色（按优先级从高到低）

| DamageType | 颜色 (R,G,B) |
|-----------|-------------|
| Card_Finisher | (1.0, 0.85, 0.0) 金 |
| Card_Link | (1.0, 0.55, 0.1) 橙 |
| Card_Matched | (0.2, 0.9, 0.9) 青 |
| Card_Hit | (0.4, 0.7, 1.0) 淡蓝 |
| Card_Consume | (0.4, 0.7, 1.0) 淡蓝 |
| Card_Shuffle | (0.5, 0.7, 1.0) 灰蓝 |
| Attack_Crit（暴击） | (1.0, 0.9, 0.1) 黄 |
| Rune_*（符文） | (0.8, 0.3, 1.0) 紫 |
| Bleed（流血） | (1.0, 0.2, 0.1) 红 |
| Attack（普通） | (1.0, 1.0, 1.0) 白 |

---

## UI 优化建议

### 过滤按钮选中态
C++ 自动生成的按钮无选中高亮。UI 优化时推荐：
- 方案 A：在 Blueprint 手动放 9 个按钮，EventGraph 调用 `SetFilter` + 更新按钮颜色（通过 `GetActiveFilter()` 比较）
- 方案 B：不用 C++ 自动生成，删除 `FilterButtonBox` 的 BindWidget 绑定并手动管理

### 布局参考比例（800×600 编辑器停靠面板）

```
┌─────────────────────────────────┐  ← 全部/普通/暴击/符文/流血/卡牌/终结技/连携/洗牌  (H=40)
│  [全部] [普通] [暴击] ... [重置] │
├─────────────────────────────────┤
│                                 │
│  [01:23] 玩家 -> 小鬼  轻击1    │  ← LogScrollBox（自动填充中间区域）
│    25×1.20×1.00 ★CRIT = 30.0  │
│  [01:24] [CARD] 怒锋入铠 消耗  │
│    -> 小鬼 [MATCH] = 45.0      │
│  ...                            │
├─────────────────────────────────┤
│ -- 卡牌统计 --                  │  ← SummaryText（固定底部 H=116）
│ 消耗: 9次 | 命中: 12次 | ...   │
│ -- 伤害统计 --                  │
│ 普通: 350(18次) | 暴击: 280... │
│ 总计: 1760                      │
└─────────────────────────────────┘
```

### 日志行间距
日志行是动态创建的 `UTextBlock`，如需行间距可在 `AddLogRow` 里改用 `UBorder` 包裹并设置 Padding，或改用 `UListView` 替代 ScrollBox 获得更好的性能（大量条目时 ScrollBox 子控件过多会卡）。

---

## Class Defaults 可调参数

打开 EUW_CombatLog Blueprint → Details 面板 → Class Defaults：

| 参数 | 说明 | 默认值 |
|------|------|--------|
| Log Font Size | 日志行字体大小 | 11 |
| Filter Font Size | 过滤按钮标签字体大小 | 10 |
