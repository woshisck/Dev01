# 背包系统 UI 制作手册（零基础版）

> 适用范围：WBP_RuneSlot / WBP_BackpackGrid / WBP_PendingGrid / WBP_BackpackScreen 蓝图制作  
> 适用人群：完全不熟悉 UE5 蓝图和 UMG 的人  
> 配套文档：[背包系统配置指南](BackpackSystem_Guide.md)  
> 最后更新：2026-04-18

---

## 读前须知

**格子和颜色全部由 C++ 自动生成，你不需要写任何蓝图节点。**

只需要在各 WBP 的 Designer 里放好指定名称的控件，C++ 就会自动找到并填充。

**控件命名区分大小写，名字错了 C++ 找不到！**

**本文约定：**

- `Content Browser`：编辑器左下角资源浏览器
- `Details`：选中控件后右侧属性面板
- `Hierarchy`：Widget 蓝图左侧层级面板
- `Designer`：Widget 蓝图布局编辑区域
- `Palette`：Widget 蓝图左侧可拖入的控件列表

---

## 整体 Widget 架构

```text
WBP_BackpackScreen      ← 主界面（BackpackScreenWidget）
├── WBP_BackpackGrid    ← 主格子（BackpackGridWidget），变量名: BackpackGridWidget
│   ├── SizeBox         ← 变量名: GridSizeBox（固定格子像素尺寸）
│   │   └── UniformGridPanel ← 变量名: BackpackGrid ★（C++ 在此生成格子）
│
├── WBP_PendingGrid     ← 待放置区（PendingGridWidget），变量名: PendingGridWidget
│   └── SizeBox         ← 变量名: PendingGridSizeBox（固定格子像素尺寸）
│       └── UniformGridPanel ← 变量名: PendingRuneGrid ★（C++ 在此生成格子）
│
├── WBP_RuneInfoCard    ← 符文信息卡，变量名: RuneInfoCard
├── Image               ← 浮空拖拽图标，变量名: GrabbedRuneIcon
└── Button              ← 出售按钮，变量名: SellButton
```

> ★ = 必须放且命名精确，其余带"可选"标注的不放也不会报错。

---

## 第一步：制作 WBP_RuneSlot（单格子）

> 这是主格子里每一格的 Widget，由 BackpackGridWidget 在运行时自动创建，不需要手动放入。

### 1.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`RuneSlotWidget`** → Select
3. 命名 **`WBP_RuneSlot`**，保存到 `Content/UI/Playtest_UI/BackpackGrid/`

### 1.2 Designer 层级结构

```text
Overlay（根节点）
├── Image    ← 变量名: CellBG         （格子背景色，C++ 控制颜色）
├── Image    ← 变量名: ActiveZoneOverlay（激活区特效层，C++ 控制显隐）
└── Image    ← 变量名: CellIcon        （符文图标，C++ 控制）
```

三个控件均为 **BindWidgetOptional**（名字不对只是无效，不会崩）。

### 1.3 各控件 Slot 设置

所有三个 Image 的 Overlay Slot：

| 属性 | 值 |
|------|-----|
| Horizontal Alignment | **Fill** |
| Vertical Alignment | **Fill** |

ActiveZoneOverlay 额外：

- Details → Behavior → Visibility：**Collapsed**（C++ 在激活区格子时自动显示）

---

## 第二步：制作 WBP_RuneInfoCard（符文信息卡）

### 2.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`RuneInfoCardWidget`** → Select
3. 命名 **`WBP_RuneInfoCard`**，保存到 `Content/UI/Playtest_UI/`

### 2.2 Designer 层级结构

```text
Overlay（根节点）
├── Image           ← 变量名: CardBG
└── Vertical Box    ← 变量名: CardContent
    ├── Image       ← 变量名: CardIcon
    ├── Text Block  ← 变量名: CardName
    ├── Text Block  ← 变量名: CardDesc
    └── Text Block  ← 变量名: CardUpgrade
```

### 2.3 各控件设置

| 控件 | 关键属性 |
|------|---------|
| CardBG | Slot → Fill/Fill；Color：深色（R=0.1, G=0.1, B=0.15） |
| CardContent | Slot → Fill/Fill；Padding：12（四边） |
| CardIcon | Size 80×80 或勾选 Size To Content |
| CardName | Font Size 16，Bold |
| CardDesc | Font Size 12；勾选 Auto Wrap Text |
| CardUpgrade | Font Size 11；Visibility → **Collapsed** |

---

## 第三步：制作 WBP_BackpackGrid（主格子 Widget）

### 3.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`BackpackGridWidget`** → Select
3. 命名 **`WBP_BackpackGrid`**，保存到 `Content/UI/Playtest_UI/BackpackGrid/`

### 3.2 Designer 层级结构

```text
Canvas Panel（根节点，或 Overlay 亦可）
├── SizeBox                     ← 变量名: GridSizeBox ★
│   └── UniformGridPanel        ← 变量名: BackpackGrid ★
│
```

### 3.3 GridSizeBox 设置

SizeBox 不需要手动设置宽高，C++ 在 BuildGrid 时会自动写入精确像素尺寸（保证每格 1:1 正方形）。

- Slot → Horizontal Alignment：**Left**（不要 Fill，否则 SizeBox 会被拉伸）
- Slot → Vertical Alignment：**Top**

### 3.4 BackpackGrid (UniformGridPanel) 设置

- `Is Variable`：**打勾**
- 变量名：**`BackpackGrid`**（大小写精确）
- 面板内**不要放任何子控件**，C++ 自动生成

Slot（位于 SizeBox 内）：

| 属性 | 值 |
|------|-----|
| Horizontal Alignment | **Fill** |
| Vertical Alignment | **Fill** |

### 3.5 填写 Details 配置

打开 WBP_BackpackGrid → 右上角 Details（非控件 Details）：

| 属性 | 填写内容 |
|------|---------|
| Slot → Run Slot Class | **WBP_RuneSlot**（第一步创建的）|
| Style DA | **DA_BackpackStyle**（第六步创建）|

---

## 第四步：制作 WBP_PendingGrid（待放置区 Widget）

### 4.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`PendingGridWidget`** → Select
3. 命名 **`WBP_PendingGrid`**，保存到 `Content/UI/Playtest_UI/BackpackGrid/`

### 4.2 Designer 层级结构

```text
Canvas Panel（根节点）
└── SizeBox                     ← 变量名: PendingGridSizeBox ★
    └── UniformGridPanel        ← 变量名: PendingRuneGrid ★
```

### 4.3 各控件设置

**SizeBox (PendingGridSizeBox)**：

- `Is Variable`：**打勾**
- Slot → Horizontal Alignment：**Left**
- 宽高不用手动填，BuildSlots 时 C++ 自动设置

**UniformGridPanel (PendingRuneGrid)**：

- `Is Variable`：**打勾**
- 变量名：**`PendingRuneGrid`**
- 面板内**不要放任何子控件**

Slot（位于 SizeBox 内）：

| 属性 | 值 |
|------|-----|
| Horizontal Alignment | **Fill** |
| Vertical Alignment | **Fill** |

### 4.4 填写 Details 配置

| 属性 | 填写内容 |
|------|---------|
| Config → Pending Grid Cols | **2**（列数） |
| Config → Pending Grid Rows | **4**（行数） |
| Style DA | **DA_BackpackStyle** |

---

## 第五步：制作 WBP_BackpackScreen（主界面）

### 5.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`BackpackScreenWidget`** → Select
3. 命名 **`WBP_BackpackScreen`**，保存到 `Content/UI/Playtest_UI/`

### 5.2 画布尺寸

点击 Designer 画布空白区域 → Details → **Screen Size**：Width 1920 / Height 1080

### 5.3 层级结构

```text
Canvas Panel（全屏）
├── Border                          ← 半透明背景
│   └── Horizontal Box              ← 三栏布局
│       ├── WBP_PendingGrid 实例    ← 变量名: PendingGridWidget ★
│       ├── WBP_BackpackGrid 实例   ← 变量名: BackpackGridWidget ★
│       └── WBP_RuneInfoCard 实例   ← 变量名: RuneInfoCard ★
│
├── Image                           ← 变量名: GrabbedRuneIcon ★
└── Button                          ← 变量名: SellButton ★
```

### 5.4 搭建步骤

#### 步骤 A：Canvas Panel（全屏）

- Anchors：全屏拉伸（四角那种）
- Offset Left/Top/Right/Bottom：全部 **0**

#### 步骤 B：Border（半透明背景）

- Anchors：全屏拉伸，Offset 全 0
- Brush → Tint：R=0.06, G=0.06, B=0.10, A=0.92

#### 步骤 C：Horizontal Box（三栏）

在 Border 里放 Horizontal Box：

- Anchors：居中（中间那个），Alignment X=0.5, Y=0.5
- 尺寸根据实际格子数量调整

#### 步骤 D：WBP_PendingGrid（左列）

在 Horizontal Box 里拖入 **WBP_PendingGrid** 实例：

- `Is Variable`：**打勾**
- 变量名：**`PendingGridWidget`**
- Slot → Size：**Auto**（重要：不要 Fill）
- Slot → Vertical Alignment：**Center**

#### 步骤 E：WBP_BackpackGrid（中列）

在 Horizontal Box 里拖入 **WBP_BackpackGrid** 实例：

- `Is Variable`：**打勾**
- 变量名：**`BackpackGridWidget`**
- Slot → Size：**Auto**（重要：不要 Fill）
- Slot → Vertical Alignment：**Center**

#### 步骤 F：WBP_RuneInfoCard（右列）

在 Horizontal Box 里拖入 **WBP_RuneInfoCard** 实例：

- `Is Variable`：**打勾**
- 变量名：**`RuneInfoCard`**
- Slot → Vertical Alignment：**Center**
- Details → Behavior → Visibility：**Collapsed**（C++ 自动控制）

> ⚠️ RuneInfoCard 必须是 Horizontal Box 的**最后一个子控件**（层级最下方）。

#### 步骤 G：GrabbedRuneIcon（浮空图标）

**回到 Canvas Panel 根层**（不在 Border 里），放一个 **Image**：

- `Is Variable`：**打勾**
- 变量名：**`GrabbedRuneIcon`**
- Visibility：**Collapsed**
- Is Hit Test Invisible：**打勾**
- Size：64×64

#### 步骤 H：SellButton（出售按钮）

在 Canvas Panel 根层放一个 **Button**：

- `Is Variable`：**打勾**
- 变量名：**`SellButton`**
- 内部放 Text Block，内容 = `出售`
- 拖到右下角合适位置

---

## 第六步：配置 DA_BackpackStyle

### 6.1 创建

Content Browser → `+ Add → Miscellaneous → Data Asset` → 选 **`BackpackStyleDataAsset`**

命名 **`DA_BackpackStyle`**，保存到 `Content/UI/Playtest_UI/`

### 6.2 填入各 Widget

| Widget | 属性位置 | 填写 |
|--------|---------|------|
| WBP_BackpackGrid | Details → Style DA | DA_BackpackStyle |
| WBP_PendingGrid | Details → Style DA | DA_BackpackStyle |

（WBP_BackpackScreen 本身无 StyleDA 字段，通过子 Widget 分别引用）

### 6.3 颜色配置

双击打开 `DA_BackpackStyle`，可修改所有颜色：

**格子颜色：**

| 参数 | 默认 | 说明 |
|------|------|------|
| Empty Color | 中灰 | 非激活区空格 |
| Occupied Inactive Color | 深紫灰 | 非激活区有符文 |
| Occupied Active Color | 亮蓝 | 激活区有符文 |
| Selected Color | 金黄 | 选中格 |
| Hover Color | 绿 | 拖拽悬浮目标格 |
| Grabbed Source Color | 亮黄 | 被抓起的源格 |

**格子尺寸：**

| 参数 | 默认 | 说明 |
|------|------|------|
| Cell Size | 64 px | 格子边长（SizeBox 自动计算总尺寸） |
| Cell Padding | 2 px | 格子间距 |
| Cell Corner Radius | 3 px | 格子圆角 |
| Icon Padding | 6 px | 符文图标内边距 |

---

## 第七步：配置背包网格大小

打开角色蓝图 **B_PlayerCharacterBase**：

1. Components 面板 → `BackpackGridComponent`
2. Details → Config → **Grid Width**（列数，默认 5）
3. Details → Config → **Grid Height**（行数，默认 5）

激活区形状在武器 DA（`WeaponDefinition`）里配置：**Backpack Config → Activation Zone Config → Zone Shapes**。  
每个 Shape 是一组格子坐标（X=列，Y=行，从 0 开始），最多 3 阶。

---

## 第八步：验证

按 Play，然后：

1. 走近场景 `BP_RewardPickup` 按 `E` 选一个符文
2. 按 **Tab** 打开背包

| 检查 | 正常现象 |
|------|---------|
| 主格子出现 | 灰色/彩色格子填满背包区，格子 1:1 正方形 |
| 激活区颜色 | 激活区按当前武器配置叠加显示，颜色清晰可读 |
| 单击抓取 | 单击有符文的格子 → 符文立即进入抓取（GrabbedRuneIcon 浮空图标出现，该符文黄框高亮，右侧信息卡弹出） |
| 移动放置 | 抓取中点击空格 → 符文移入目标格，抓取结束 |
| 换格自动拾起 | 抓取中点击已有符文的格子（swap）→ 两格互换后，被替换出来的符文自动抓取（黄框立即转移） |
| 长按退回 | 抓取中持续按住鼠标左键 3s → 符文自动送回待放置区，抓取结束 |
| 悬停绿框 | 无抓取时鼠标移到有符文格子上 → 该符文显示绿色包围框；移开后消失 |
| 战斗锁定 | 关卡战斗阶段尝试移动符文 → 格子红闪+横向抖动，弹出"战斗阶段无法移动符文"提示 |
| 拖拽放置 | 按住符文快速拖动（超过拖拽阈值）→ 浮空图标跟鼠标，松手放置（备用路径） |
| 待放置区 | 左侧出现 2×4 灰色槽位，选符文后图标填入 |
| 出售 | 选中格子点"出售"按钮，符文消失 |

---

## 遇到问题

| 问题 | 解决方法 |
|------|---------|
| 格子没出现 | 检查 `BackpackGrid` / `PendingRuneGrid` 变量名（大小写）；UniformGridPanel 里不要手放子控件 |
| 格子不是正方形 | 检查 SizeBox（`GridSizeBox` / `PendingGridSizeBox`）是否放且命名正确；子控件 Slot 是否为 Fill/Fill |
| 子 Widget 绑定失败 | BackpackGridWidget / PendingGridWidget 的变量名是否精确（大小写）；父类选择是否正确 |
| 激活区颜色没变 | DA_BackpackStyle 是否填入 WBP_BackpackGrid 的 Style DA 字段；武器 DA 的 Zone Shapes 是否填写 |
| RuneInfoCard 没出现 | 变量名 `RuneInfoCard`；确认它是 WBP_RuneInfoCard 实例；确认它是 HorizontalBox 最后一个子控件 |
| GrabbedRuneIcon 不跟鼠标 | 确认 Image 在 Canvas Panel 根层（不在 Border 里）；变量名精确为 `GrabbedRuneIcon` |
