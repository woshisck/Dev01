# WBP_PortalPreview 布局规格

> 用于"传送门下一关预览浮窗"（HUD 单例）。
> 父类：`UPortalPreviewWidget`（`Source/DevKit/Public/UI/PortalPreviewWidget.h`）

---

## 类信息

| 项目      | 内容                                                                   |
| ------- | -------------------------------------------------------------------- |
| WBP 名   | `WBP_PortalPreview`                                                  |
| 父类（C++） | `UPortalPreviewWidget`                                               |
| 加入方式    | `AYogHUD::ShowPortalGuidance` 内 `CreateWidget + AddToViewport(Z=15)` |
| 配置入口    | `BP_YogHUD` → Details → `Portal Preview Class = WBP_PortalPreview`   |
| 触发      | 关卡结算后由 HUD 在每帧 TickPortalPreview 中按"距玩家最近的开启 Portal"自动显示/切换          |

---

## 控件层级（树形）

```
CanvasPanel（根，Full Screen）
└── SizeBox（Root，左上锚 0/0，Auto尺寸）
    └── Border（BG，#1A1A2E α=0.92，Pad=14/12/14/12，圆角不需要）
        └── VerticalBox（VStack，HFill）
            ├── HorizontalBox（HeaderBox，HFill，PadB=8）
            │   ├── Border（RoomTypeBadge [BindWidgetOptional]，纯色背景，Pad=8/2/8/2，自动大小）
            │   │   └── TextBlock（RoomTypeText [BindWidgetOptional]，字号11 Bold，#FFFFFF）
            │   └── TextBlock（RoomNameText [BindWidgetOptional]，HFill，VCenter，PadL=8，字号15 Bold，#ECECEC）
            ├── TextBlock（"敌人将携带印记"，字号12，#888888，PadB=4）
            ├── VerticalBox（BuffListBox [BindWidgetOptional]，HFill，C++ 动态填充每行；浮窗高度随内容自然撑开）
            ├── TextBlock（LootSummaryText [BindWidgetOptional]，HFill，字号12，#D0D0D0，PadT=8 PadB=4）
            └── HorizontalBox（InteractHintRoot [BindWidgetOptional]，HFill，VCenter，PadT=8，初始 Collapsed）
                ├── Border（"E"键徽章，Pad=6/2/6/2，#3A3A4A）
                │   └── TextBlock（"E"，字号13 Bold，#FFFFFF）
                └── TextBlock（"进入"，字号13，#ECECEC，VCenter，PadL=6）
```

---

## 各控件详细属性

### CanvasPanel（根）

| 属性 | 值 |
|---|---|
| Anchors | Min(0,0) Max(1,1)（全屏） |
| Position / Size | 0/0 / 撑开 |

### SizeBox（Root）

#### Slot（Canvas Slot）
| 属性 | 值 |
|---|---|
| Anchors | Min(0,0) Max(0,0)（左上锚） |
| Position X/Y | 0 / 0（运行时由 HUD::TickPortalPreview 调 SetPositionInViewport） |
| Size X/Y | 320 / 200 |
| Alignment X/Y | 0.5 / 0.5（以中心点对齐 — 让运行时设定的屏幕坐标作为浮窗中心） |
| bAutoSize | false |

#### 自身属性
| 属性 | 值 |
|---|---|
| Min Desired Width / Height | 280 / 160 |
| Max Desired Width | 360 |

### Border（BG）

| 属性 | 值 |
|---|---|
| Brush → Tint | #1A1A2E A=0.92（线性 (0.10,0.10,0.18,0.92)） |
| Brush → Draw As | Box |
| Padding | 14/12/14/12 |
| HAlign / VAlign | Fill / Fill |

### VerticalBox（VStack）

子控件 Slot 全部 HFill；其他默认。

### HorizontalBox（HeaderBox）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Fill |
| Padding B | 8 |

### Border（RoomTypeBadge）

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| VAlign | Center |

#### 自身属性
| 属性 | 值 |
|---|---|
| Brush → Tint | (0.6, 0.6, 0.6, 0.92)（运行时由 C++ `SetBrushColor` 按 Room.Type.* 覆盖） |
| Brush → Draw As | Box |
| Padding | 8/2/8/2 |

### TextBlock（RoomTypeText）

| 属性 | 值 |
|---|---|
| Text | "普通"（运行时由 C++ 覆盖） |
| Font Size | 11 |
| Typeface | Bold |
| Color | #FFFFFF |
| Justification | Center |

### TextBlock（RoomNameText）

#### Slot（HBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Fill |
| Fill Coefficient | 1.0 |
| VAlign | Center |
| Padding L | 8 |

#### 自身属性
| 属性 | 值 |
|---|---|
| Text | "下一关"（运行时由 C++ 覆盖） |
| Font Size | 15 |
| Typeface | Bold |
| Color | #ECECEC |

### TextBlock（"敌人将携带印记" 静态标题）

| 属性 | 值 |
|---|---|
| Slot Size Rule | Auto |
| Padding B | 4 |
| Text | "敌人将携带印记" |
| Font Size | 12 |
| Color | #888888 |

### VerticalBox（BuffListBox）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Fill |

> 内容由 C++ `SetPreviewInfo` 动态构造。每行 = `HorizontalBox[ Image 28×28(VAlign Top) | VerticalBox{ Name(13) / Description(11, AutoWrap) / GenericEffects×N(11, AutoWrap, "• 名：描"格式) } ]`，行间距 6px，子行间距 2px。
> 颜色 / 字号 / 间距均在 [PortalPreviewWidget.cpp](../../../Source/DevKit/Private/UI/PortalPreviewWidget.cpp) 顶部 namespace 常量集中维护，需调整改 .cpp 即可。
> 浮窗高度随符文条数和描述长度自然撑开（外层 SizeBox 不限制 MaxDesiredHeight）。BuffPool 为空时该容器无子控件，自动塌陷高度。

### TextBlock（LootSummaryText）

| 属性 | 值 |
|---|---|
| Slot Size Rule | Auto |
| Padding T | 8 |
| Padding B | 4 |
| Text | "战利品：符文 ×3"（运行时由 C++ 覆盖） |
| Font Size | 12 |
| Color | #D0D0D0 |

### HorizontalBox（InteractHintRoot）

#### Slot（VBox Slot）
| 属性 | 值 |
|---|---|
| Size Rule | Auto |
| HAlign | Fill |
| VAlign | Center |
| Padding T | 8 |

#### 自身属性
| 属性 | 值 |
|---|---|
| **Visibility（默认）** | **Collapsed**（C++ `SetInteractHintVisible(true)` 时改为 HitTestInvisible） |

### Border（"E"键徽章）

| 属性 | 值 |
|---|---|
| Slot Size Rule | Auto |
| Brush → Tint | #3A3A4A α=1.0 |
| Brush → Draw As | Box |
| Padding | 6/2/6/2 |

> 内含 TextBlock "E"，字号 13 Bold，颜色 #FFFFFF，Justification Center。

### TextBlock（"进入"）

| 属性 | 值 |
|---|---|
| Slot Size Rule | Auto |
| VAlign | Center |
| Padding L | 6 |
| Text | "进入" |
| Font Size | 13 |
| Color | #ECECEC |

---

## BindWidget 汇总

### 必须（无）

本 Widget 全部使用 BindWidgetOptional — 任意控件缺失都不崩，对应区段自动跳过显示。

### 可选（BindWidgetOptional）

| C++ 变量名 | 控件类型 | WBP 控件名 | 缺失行为 |
|---|---|---|---|
| `RoomNameText` | TextBlock | `RoomNameText` | 不显示房间名 |
| `RoomTypeBadge` | Border | `RoomTypeBadge` | 类型背景色不变 |
| `RoomTypeText` | TextBlock | `RoomTypeText` | 不显示类型文字 |
| `BuffListBox` | VerticalBox | `BuffListBox` | 不显示 Buff 列表 |
| `LootSummaryText` | TextBlock | `LootSummaryText` | 不显示战利品摘要 |
| `InteractHintRoot` | Widget（HBox/Border 均可） | `InteractHintRoot` | 不显示"按 E 进入"提示 |

---

## 颜色速查

| 元素 | Hex | Linear RGBA |
|---|---|---|
| 弹窗背景 | #1A1A2E α=0.92 | (0.10, 0.10, 0.18, 0.92) |
| 房间名文字 | #ECECEC | (0.93, 0.93, 0.93, 1.0) |
| 静态标题灰 | #888888 | (0.53, 0.53, 0.53, 1.0) |
| Loot 摘要 | #D0D0D0 | (0.82, 0.82, 0.82, 1.0) |
| Hint "E" 徽章背景 | #3A3A4A | (0.23, 0.23, 0.29, 1.0) |
| Hint 文字 | #ECECEC | (0.93, 0.93, 0.93, 1.0) |
| **类型徽章颜色（C++ 静态映射，覆盖 RoomTypeBadge.BrushColor）** | | |
| Room.Type.Normal | #999999 | (0.60, 0.60, 0.60, 0.92) |
| Room.Type.Elite | #D85A4A | (0.85, 0.35, 0.29, 0.92) |
| Room.Type.Shop | #D9B047 | (0.85, 0.69, 0.28, 0.92) |
| Room.Type.Event | #7A5BC9 | (0.48, 0.36, 0.79, 0.92) |

---

## BP 视觉强化钩（可选实现）

WBP 可在事件图表中实现以下事件追加视觉效果（不实现也能跑）：

| 事件 | 触发时机 |
|---|---|
| `On Preview Shown` | 浮窗刚切换到新 Target Portal 时 |
| `On Preview Hidden` | （C++ 当前未主动调；可由 BP 自行扩展） |
| `On Interact Hint Shown` | "按 E 进入"提示首次显示 |

---

## 注意事项

- **不要在 WBP 的 Visibility 里设默认值** — C++ 通过 `AddToViewport` 后立刻 `SetVisibility(Collapsed)`，由 TickPortalPreview 决定何时显示
- `InteractHintRoot` 默认 Collapsed，是因为浮窗刚显示时玩家可能还没进 Box；进 Box 后 C++ 调 `SetInteractHintVisible(true)` 切到 HitTestInvisible
- 浮窗位置由 HUD 每帧 `SetPositionInViewport(屏幕坐标, false)` 设置，不要在 WBP 中固定 Position
