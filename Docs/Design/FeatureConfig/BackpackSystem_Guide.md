# 背包系统蓝图制作指南

> 适用范围：WBP_BackpackScreen / WBP_RuneInfoCard 蓝图制作、样式 DA 配置  
> 适用人群：策划 + 美术  
> 配套文档：[背包系统技术文档](../Systems/BackpackSystem_Technical.md)  
> 最后更新：2026-04-17

---

## 架构概述

**所有格子由 C++ 自动生成**，蓝图 Designer 里只需放 7 个命名控件，不需要手动建 Button 数组、不需要 Event Graph 节点。

| 控件名（大小写完全一致） | 类型 | 说明 |
| --- | --- | --- |
| `BackpackGrid` | UniformGridPanel | 主背包网格，C++ 填充 |
| `PendingRuneGrid` | UniformGridPanel | 左侧待放置区，C++ 填充 |
| `RuneInfoCard` | WBP_RuneInfoCard 实例 | 右侧符文信息卡 |
| `GrabbedRuneIcon` | Image | 拖拽/手柄抓取时的浮空图标 |
| `SellButton` | Button | 出售选中格子的符文 |
| `HintText` | RichTextBlock | 操作提示文字（可选） |
| `RuneTooltip` | WBP_RuneTooltip 实例 | 鼠标悬浮 Tooltip（可选） |

---

## 一、WBP_BackpackScreen 蓝图制作

### 1.1 创建 Widget

1. Content Browser → `+ Add → User Interface → Widget Blueprint`
2. 父类搜索 **`BackpackScreenWidget`** → Select
3. 文件命名 **`WBP_BackpackScreen`**，放到 `Content/UI/Playtest_UI/`

### 1.2 Designer 层级结构

```
Canvas Panel（全屏，Anchors: 全拉伸）
├── Border [半透明背景，Brush Color #1A1A1A Alpha=0.9，全屏]
│   └── Horizontal Box [两栏布局，Size Auto，Canvas 居中锚点]
│       │
│       ├── [左列] Vertical Box (Size Auto)
│       │   ├── Text "待放置"
│       │   └── UniformGridPanel  ← 变量名: PendingRuneGrid  ★ 必须留空，C++填充
│       │
│       ├── [中列] Vertical Box (Size Auto)
│       │   ├── Text "背包"
│       │   └── UniformGridPanel  ← 变量名: BackpackGrid  ★ 必须留空，C++填充
│       │
│       └── [右列] WBP_RuneInfoCard 实例  ← 变量名: RuneInfoCard
│           (Visibility: Collapsed，C++ 自动控制显隐)
│
├── Image  ← 变量名: GrabbedRuneIcon  (Visibility: Collapsed, HitTestInvisible)
│   [放在 Canvas Panel 根层，位置随意，C++ Tick 每帧移动]
│
└── Button  ← 变量名: SellButton
    └── Text "出售"
```

> ⚠️ `BackpackGrid` 和 `PendingRuneGrid` 内部**不能放任何子控件**，C++ 会清空并重建。
>
> ⚠️ `GrabbedRuneIcon` 必须在 Canvas Panel 根层（不在 Border 里），这样它可以自由定位到任意屏幕坐标。
>
> ⚠️ `RuneInfoCard` 要放在层级最下方（最后一个子控件），确保渲染在最上层。

### 1.3 各控件关键参数

#### UniformGridPanel（BackpackGrid / PendingRuneGrid）

- Slot Padding：不需要设置，C++ 通过格子间距自动控制
- 保持默认，不要在 Designer 里改 Min Desired Slot Width/Height

#### GrabbedRuneIcon

- Size to Content：关（不勾选）
- Image Size：64×64（与格子同尺寸，C++ 用 StyleDA.CellSize 驱动）
- Brush → Image：留空（C++ 运行时填入）

#### SellButton

- 不需要在 Event Graph 里绑定 OnClicked，C++ 自动绑定

#### RuneInfoCard（WBP_RuneInfoCard 实例）

- Overlay Slot → Horizontal/Vertical Alignment：Fill（两个都要填 Fill）

### 1.4 Event Graph 不需要任何节点

所有逻辑（格子颜色、拖拽、选中、出售）均由 C++ 处理，蓝图 Event Graph 保持空白即可。

---

## 二、WBP_RuneInfoCard 蓝图制作

### 2.1 创建 Widget

1. 父类搜索 **`RuneInfoCardWidget`**
2. 文件命名 **`WBP_RuneInfoCard`**，放到 `Content/UI/Playtest_UI/`

### 2.2 Designer 层级结构

```
Overlay（根节点）
├── Image  ← 变量名: CardBG  [背景色，Overlay Slot: Fill + Fill]
└── Vertical Box  ← 变量名: CardContent  [Overlay Slot: Fill + Fill, Padding 12]
    ├── Image      ← 变量名: CardIcon    [图标，Size Auto]
    ├── Text       ← 变量名: CardName    [符文名称，字号 16，加粗]
    ├── Text       ← 变量名: CardDesc    [描述，字号 12，Auto Wrap Text 勾上]
    └── Text       ← 变量名: CardUpgrade [升级等级，字号 11，默认 Collapsed]
```

### 2.3 关键设置

- 根节点 Overlay 的 **Size to Content**：勾上（或者用 SizeBox 固定宽高）
- `CardBG`：Overlay Slot → Horizontal Alignment = Fill，Vertical Alignment = Fill
- `CardContent`：Overlay Slot → Horizontal/Vertical Alignment = Fill，Padding = 12

---

## 三、配置视觉样式（DA_BackpackStyle）

### 3.1 创建 DataAsset

1. Content Browser → `+ Add → Miscellaneous → Data Asset`
2. 选择类型 **`BackpackStyleDataAsset`**
3. 命名 **`DA_BackpackStyle`**，放到 `Content/UI/Playtest_UI/`

### 3.2 填入 WBP_BackpackScreen

打开 `WBP_BackpackScreen` → Details 面板 → **Backpack | Style → Style DA** → 填入 `DA_BackpackStyle`

### 3.3 可配置参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| Empty Color | 灰 (0.40, 0.40, 0.42) | 未激活区空格 |
| Empty Active Color | 深蓝 (0.15, 0.35, 0.75) | 激活区空格 |
| Occupied Active Color | 亮蓝 (0.10, 0.55, 1.00) | 激活区有符文 |
| Occupied Inactive Color | 橙 (0.55, 0.35, 0.05) | 非激活区有符文 |
| Selected Color | 金黄 (1.00, 0.82, 0.10) | 选中高亮 |
| Hover Color | 绿 (0.10, 0.80, 0.20) | 拖拽悬浮目标格 |
| Grabbed Source Color | 暗橙 (0.25, 0.15, 0.03) | 被拖起的源格 |
| Pending Has Rune Color | 深紫 (0.12, 0.08, 0.22) | 左侧有符文槽 |
| Pending Empty Color | 灰 (0.40, 0.40, 0.42) | 左侧空槽 |
| Cell Size | 64 px | 格子边长 |
| Cell Padding | 2 px | 格子间距 |
| Cell Corner Radius | 3 px | 圆角半径 |
| Icon Padding | 6 px | 图标内边距 |

---

## 四、配置背包网格大小

在 **角色蓝图**（B_PlayerCharacterBase）的 Components 面板选中 `BackpackGridComponent`：

- Details → **Config → Grid Width**：列数（默认 5）
- Details → **Config → Grid Height**：行数（默认 5）

---

## 五、符文池配置

在 **BP_YogGameModeBase** → Class Defaults → **Level Flow → Fallback Loot Pool**：

添加符文资产（`Content/Docs/BuffDocs/Playtest_GA/` 下），至少 3 个。

---

## 六、拖拽操作说明（技术）

| 操作 | 行为 |
|---|---|
| 按住左键拖拽格子中符文 | 启动拖拽，`GrabbedRuneIcon` 立即出现在鼠标位置跟随移动 |
| 拖到目标格子松手 | 放置（若有符文则交换） |
| 拖到空白区松手 / Esc | 取消，符文回原位 |
| 从左侧待放置区拖到主格子 | 将 PendingRune 放置到背包 |
| 手柄 D-Pad 导航 + A 抓取 + A 放置 | 等同鼠标操作，`GrabbedRuneIcon` 跟随手柄光标 |

> 浮空图标由 C++ Tick 每帧驱动（`GrabbedRuneIcon`），不依赖 UE 内置 `DefaultDragVisual`，可直接出现在鼠标下方，无飞入动画。

---

## 七、展示前验证清单

| 检查项 | 验证方法 |
|---|---|
| 格子出现且有颜色 | 开背包后看到灰/蓝色格子 |
| 符文图标显示 | 三选一选符文后，格子里有图标 |
| 拖拽跟手 | 按住符文拖动，浮空图标紧随鼠标 |
| 悬浮高亮 | 拖到目标格时格子变绿 |
| RuneInfoCard 弹出 | 点选格子后右侧出现符文详情卡 |
| 出售按钮生效 | 选中格子后点 SellButton，符文消失 |
| 手柄导航 | D-Pad 移动光标，A 抓取/放置 |

---

## 八、常见问题

**Q：格子没有出现？**  
A：`BackpackGrid` 控件名拼写是否完全匹配（大小写一致），UniformGridPanel 里是否为空（没有手动放 Button）。

**Q：GrabbedRuneIcon 不跟随鼠标？**  
A：确认 `GrabbedRuneIcon` 在 Canvas Panel 根层，不在任何 Border/Overlay 内。Image 变量名精确为 `GrabbedRuneIcon`。

**Q：RuneInfoCard 选中格子后没有出现？**  
A：1) 确认控件名精确为 `RuneInfoCard`；2) 确认它是 `WBP_RuneInfoCard` 实例（父类为 RuneInfoCardWidget）；3) 确认它是 Canvas Panel 层级的**最后一个子控件**（渲染最上层）。

**Q：格子被拉伸（不是正方形）？**  
A：包含 `BackpackGrid` 的 VerticalBox / HorizontalBox Slot → **Size = Auto**（不要用 Fill）。

**Q：待放置区颜色不对？**  
A：在 `DA_BackpackStyle` 里调 `Pending Has Rune Color` / `Pending Empty Color`。
