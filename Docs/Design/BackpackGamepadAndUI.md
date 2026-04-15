# 背包 & 三选一：手柄适配 + UI 升级 设计文档

> 版本：2026-04-15  
> 涉及文件：`LootSelectionWidget`、`BackpackScreenWidget`、`RuneTooltipWidget`

---

## 一、三选一手柄导航

### 交互流程

```
UI 显示 → 手柄 D-Pad ← / → 切换高亮 → A 键确认选择
```

### 键位映射

| 键位 | 动作 |
|------|------|
| D-Pad Left / ← | 高亮向左移动，调用 `OnNavigateSelection(-1)` |
| D-Pad Right / → | 高亮向右移动，调用 `OnNavigateSelection(1)` |
| A / Enter | 确认当前高亮项，调用 `SelectRuneLoot(CurrentHighlightIndex)` |

### C++ 实现细节

- `LootSelectionWidget.h` 新增 `NativeOnKeyDown` + `CurrentHighlightIndex`（private，范围 0–2）
- 高亮移动时 `CurrentHighlightIndex` 自动 Clamp 到 `[0, 2]`
- `NativeConstruct` 中已有 `bIsFocusable = true`，Widget 可直接接收手柄输入

### 蓝图任务

`WBP_LootSelection` 实现 `OnNavigateSelection(Delta)` 蓝图事件 → 根据 `CurrentHighlightIndex` 高亮对应卡片（边框亮起 / 缩放动画）

---

## 二、背包手柄两步式抓取

### 交互流程

```
D-Pad 移动光标 → A（有符文格）抓取
  ├→ D-Pad 移动光标 → A（空格）放置
  ├→ D-Pad 移动光标 → A（有符文格）互换
  └→ B 取消，光标归位
```

### 键位映射

| 键位 | 状态 | 动作 |
|------|------|------|
| D-Pad Up/Down/Left/Right | 任意 | 移动手柄光标（`MoveGamepadCursor`） |
| A（`Gamepad_FaceButton_Bottom`） | 未持有 | 抓取光标格符文（若有）|
| A | 持有中 | 放置到当前格（空格=移动，有符文=互换）|
| B（`Gamepad_FaceButton_Right`） | 持有中 | 取消抓取 |
| Y（`Gamepad_FaceButton_Top`） | 任意 | 移除当前光标格的符文 |

### 属性（BlueprintReadOnly，供蓝图绘制光标）

| 属性 | 类型 | 含义 |
|------|------|------|
| `GamepadCursorCell` | `FIntPoint` | 手柄光标所在格（Col, Row） |
| `bGrabbingRune` | `bool` | 是否正在持有符文 |
| `GrabbedFromCell` | `FIntPoint` | 抓取源格（持有时有效） |

### C++ 实现细节

- `BackpackScreenWidget.NativeConstruct` 加 `bIsFocusable = true`
- `MoveGamepadCursor(DCol, DRow)` — 更新 `GamepadCursorCell`，同步 `SelectedCell`，触发 `OnSelectionChanged()`，并更新 Tooltip
- `GamepadConfirm()` — 两步逻辑：  
  - 第一步（未持有）：检测格子是否有符文 → 标记 `bGrabbingRune = true`  
  - 第二步（持有中）：目标空 → `MoveRune`，目标有符文 → `SwapRunes`（RemoveA + RemoveB + PlaceA@PivotB + PlaceB@PivotA）
- `GamepadCancel()` — 清除持有状态，调用 `OnSelectionChanged()`

### 蓝图任务

`WBP_BackpackScreen` 在 Tick 或通过 `GamepadCursorCell` 绑定，绘制手柄光标高亮框（建议：蓝色边框，持有时变橙色/动画）。

---

## 三、符文悬浮 Tooltip

### 触发规则

| 触发方式 | 条件 | 行为 |
|----------|------|------|
| 鼠标移动 | 进入有符文的格子 | `ShowRuneInfo(rune)` + 定位到鼠标右下方 |
| 鼠标移动 | 离开格子 | `HideTooltip()` |
| 手柄光标移动 | 光标移到有符文格 | `ShowRuneInfo(rune)` |
| 手柄光标移动 | 光标移到空格 | `HideTooltip()` |

### C++ 基类：`URuneTooltipWidget`

| 控件绑定（BindWidgetOptional） | 显示内容 |
|-------------------------------|----------|
| `TooltipName` (TextBlock) | 符文名称 |
| `TooltipDesc` (TextBlock) | 符文描述 |
| `TooltipType` (TextBlock) | 增益 / 减益 / 无 |
| `TooltipIcon` (Image) | 符文图标 |

| 函数 | 描述 |
|------|------|
| `ShowRuneInfo(FRuneInstance)` | 填充文本/图标，设置 `Visible` |
| `HideTooltip()` | 设置 `Collapsed` |
| `OnTooltipShown(Rune)` | BlueprintImplementableEvent，供动画/额外逻辑 |
| `OnTooltipHidden()` | BlueprintImplementableEvent |

### 定位

`BackpackScreenWidget.UpdateTooltipForCell` 通过 `SetRenderTranslation(LocalPos + FVector2D(16, -8))` 将 Tooltip 定位到光标右上方。

### 蓝图设置步骤

1. 新建 `WBP_RuneTooltip`，父类选 `RuneTooltipWidget`
2. 添加背景 Border + 上述 4 个控件，命名完全一致
3. 在 `WBP_BackpackScreen` 的根 Overlay 顶层加入 `WBP_RuneTooltip`，命名为 `RuneTooltip`，初始 Visibility = Collapsed
4. 无需蓝图节点，C++ 自动调用

---

## 四、拖拽 Swap（互换逻辑）

### 触发条件

拖拽符文 A 到已被符文 B 占用的格子时（`MoveRune` 返回 false）。

### 互换算法

```
1. 记录 PivotA = 拖拽源符文的 Pivot（RuneOp->SrcPivot）
2. 找到目标格的符文 B 及其 PivotB
3. RemoveRune(A), RemoveRune(B)
4. TryPlaceRune(A, PivotB)   // A 去到 B 原来的位置
5. TryPlaceRune(B, PivotA)   // B 回到 A 原来的位置
6. 若任一放置失败（形状冲突）→ 回滚，提示"形状冲突"
```

### 状态消息

| 结果 | 提示文字 |
|------|----------|
| 互换成功 | "已互换：符文A ↔ 符文B" |
| 互换失败（形状冲突） | "无法互换：形状冲突" |

---

## 五、拖拽浮空视觉

在 `NativeOnDragDetected` 创建拖拽图标后，对图标应用：

```cpp
FWidgetTransform DragTransform;
DragTransform.Scale       = FVector2D(1.08f, 1.08f);  // 放大 8%
DragTransform.Translation = FVector2D(0.f, -8.f);     // 上移 8px
DragVisual->SetRenderTransform(DragTransform);
```

效果：拖拽时符文图标略微放大并上浮，产生被拿起的视觉感。

---

## 六、文件清单

| 文件 | 变更类型 | 内容摘要 |
|------|----------|----------|
| `Public/UI/LootSelectionWidget.h` | 修改 | +`NativeOnKeyDown`, +`CurrentHighlightIndex` |
| `Private/UI/LootSelectionWidget.cpp` | 修改 | 实现 `NativeOnKeyDown`（D-Pad + A） |
| `Public/UI/BackpackScreenWidget.h` | 修改 | +`GamepadCursorCell/bGrabbingRune/GrabbedFromCell`, +`RuneTooltip`, +`NativeOnKeyDown/NativeOnMouseMove`, +辅助方法声明 |
| `Private/UI/BackpackScreenWidget.cpp` | 修改 | `bIsFocusable=true`, 实现手柄逻辑/Swap/Tooltip/拖拽浮空 |
| `Public/UI/RuneTooltipWidget.h` | 新建 | Tooltip 基类声明 |
| `Private/UI/RuneTooltipWidget.cpp` | 新建 | Tooltip 基类实现 |

---

## 七、蓝图任务汇总

| 蓝图资产 | 任务 |
|----------|------|
| `WBP_LootSelection` | 实现 `OnNavigateSelection(Delta)` → 高亮对应卡片 |
| `WBP_BackpackScreen` | 根据 `GamepadCursorCell` + `bGrabbingRune` 绘制手柄光标；在 Overlay 顶层添加 `WBP_RuneTooltip`（命名 `RuneTooltip`） |
| `WBP_RuneTooltip`（新建） | 父类选 `RuneTooltipWidget`；放置 `TooltipName/TooltipDesc/TooltipType/TooltipIcon`；加背景边框 |
