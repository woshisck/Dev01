# 背包系统 UI 制作手册（零基础版）

> 适用范围：WBP_BackpackScreen / WBP_RuneInfoCard 蓝图制作  
> 适用人群：完全不熟悉 UE5 蓝图和 UMG 的人  
> 配套文档：[背包系统配置指南](../FeatureConfig/BackpackSystem_Guide.md)  
> 最后更新：2026-04-17

---

## 读前须知

**格子和颜色全部由 C++ 自动生成，你不需要写任何蓝图节点。**

只需要在 Designer 里放好 7 个指定名称的控件，C++ 就会自动找到它们并填充内容。

**本文约定：**

- `Content Browser`：编辑器左下角的资源浏览器
- `Details`：选中物体后右边出现的属性面板
- `Hierarchy`：Widget 蓝图左边的层级面板
- `Designer`：Widget 蓝图里的布局编辑区域
- `Event Graph`：Widget 蓝图里的逻辑编辑区域（本次**不需要动**）

---

## 第一步：制作 WBP_RuneInfoCard（符文信息卡）

> 先做这个，因为第二步的背包 Widget 里需要用到它。

### 1.1 创建 Widget

1. 在 Content Browser 右键 → `User Interface → Widget Blueprint`
2. 弹出"Pick Parent Class"，搜索 **`RuneInfoCardWidget`** → Select
3. 命名 **`WBP_RuneInfoCard`**，保存到 `Content/UI/Playtest_UI/`

### 1.2 Designer 层级结构

在 Hierarchy 面板按以下顺序搭建（缩进表示父子关系）：

```text
Overlay（根节点，默认就有）
├── Image               ← 命名: CardBG
└── Vertical Box        ← 命名: CardContent
    ├── Image           ← 命名: CardIcon
    ├── Text Block      ← 命名: CardName
    ├── Text Block      ← 命名: CardDesc
    └── Text Block      ← 命名: CardUpgrade
```

> ⚠️ 控件名必须和上面完全一致，包括大小写。名字错了 C++ 找不到。

### 1.3 设置各控件参数

#### Overlay（根节点）

- Details → Behavior → `Is Variable`：**打勾**

#### CardBG（Image，背景色）

- 选中 CardBG → 在 Hierarchy 面板右边看到 **Slot** 区域（不是 Details！）
- Slot → Horizontal Alignment：**Fill**
- Slot → Vertical Alignment：**Fill**
- Details → Appearance → Brush → Draw As：**Image** 或 **Box**（颜色随意，改成深色即可）
- Details → Color And Opacity：改成深色（例如 R=0.1, G=0.1, B=0.15, A=1）

#### CardContent（Vertical Box，内容容器）

- Slot → Horizontal Alignment：**Fill**
- Slot → Vertical Alignment：**Fill**
- Slot → Padding：**12**（四边都填 12）

#### CardIcon（Image，符文图标）

- Size To Content：**打勾**（自动匹配图片大小）
- 或者手动设置 Size X/Y = 80 × 80

#### CardName（Text Block，符文名称）

- Details → Content → Text：`符文名称`（占位文字）
- Details → Appearance → Font → Size：**16**，可以选 Bold

#### CardDesc（Text Block，符文描述）

- Details → Content → Auto Wrap Text：**打勾**（描述文字自动换行）
- Font Size：**12**

#### CardUpgrade（Text Block，升级等级）

- Font Size：**11**
- Details → Behavior → Visibility：**Collapsed**（默认隐藏，C++ 自动控制）

### 1.4 设置 Widget 大小

点击 Hierarchy 面板里最顶层的节点（Overlay 根节点）：

- 方案 A：Details → Layout → **Size to Content**：打勾（自动适应内容大小）
- 方案 B：在外面套一个 SizeBox，固定宽度（例如 240px）

---

## 第二步：制作 WBP_BackpackScreen（背包主界面）

### 2.1 创建 Widget

1. Content Browser 右键 → `User Interface → Widget Blueprint`
2. 搜索父类 **`BackpackScreenWidget`** → Select
3. 命名 **`WBP_BackpackScreen`**，保存到 `Content/UI/Playtest_UI/`

### 2.2 画布尺寸

点击 Designer 画布空白区域（不选中任何控件）→ Details → **Screen Size**：

- Width：**1920**，Height：**1080**

### 2.3 层级结构总览

```text
[Canvas Panel]（全屏）
├── [Border] 半透明背景                    ← 全屏铺满
│   └── [Horizontal Box] 两栏布局
│       ├── [Vertical Box] 左列
│       │   ├── [Text] "待放置"
│       │   └── [UniformGridPanel]          ← 变量名: PendingRuneGrid ★
│       │
│       ├── [Vertical Box] 中列
│       │   ├── [Text] "背包"
│       │   └── [UniformGridPanel]          ← 变量名: BackpackGrid ★
│       │
│       └── WBP_RuneInfoCard 实例           ← 变量名: RuneInfoCard ★
│
├── [Image] 浮空拖拽图标                   ← 变量名: GrabbedRuneIcon ★
└── [Button] 出售按钮                      ← 变量名: SellButton ★
```

> ★ = C++ 需要绑定的控件，名字必须精确匹配

### 2.4 搭建步骤

#### 步骤 A：放 Canvas Panel

从 Palette 搜索 **Canvas Panel** 拖到画布。

选中它 → Details：

- Anchors：点锚点图标，选**全屏拉伸**（四个角的那种）
- Offset Left/Top/Right/Bottom：全部 **0**

#### 步骤 B：放半透明背景 Border

在 Canvas Panel 里放一个 **Border**：

- Anchors：全屏拉伸，Offset 全 0
- Details → Brush → Tint：R=0.06, G=0.06, B=0.10, A=**0.92**（深蓝黑半透明）

#### 步骤 C：放两栏 Horizontal Box

在 Border 里放 **Horizontal Box**：

- Anchors：居中（中间那个），Alignment X=0.5, Y=0.5
- Size X：约 **800**，Size Y：约 **600**

> 后续可以根据实际格子数量和格子尺寸来调整。

#### 步骤 D：左列（待放置区）

在 Horizontal Box 里放 **Vertical Box**，命名 `LeftColumn`：

- Slot → Size：**Auto**（不要用 Fill，否则格子会被拉伸）

在 LeftColumn 里放：

1. **Text Block**，Content = `待放置`，Font Size 14，居中
2. **Uniform Grid Panel** → `Is Variable` 打勾，变量名改为 **`PendingRuneGrid`**

> ⚠️ `PendingRuneGrid` 里面**不要放任何控件**，C++ 会自动生成格子。

#### 步骤 E：中列（主背包网格）

在 Horizontal Box 里再放一个 **Vertical Box**，命名 `CenterColumn`：

- Slot → Size：**Auto**（同上，必须是 Auto，否则格子会被拉伸变形）

在 CenterColumn 里放：

1. **Text Block**，Content = `背包`，Font Size 14，居中
2. **Uniform Grid Panel** → `Is Variable` 打勾，变量名改为 **`BackpackGrid`**

> ⚠️ `BackpackGrid` 里面**不要放任何控件**，C++ 会自动生成格子。

#### 步骤 F：右列（符文信息卡）

在 Horizontal Box 里放第三个子控件：直接**拖入 `WBP_RuneInfoCard`**（从 Content Browser 拖）。

- `Is Variable` 打勾，变量名改为 **`RuneInfoCard`**
- Slot → Vertical Alignment：**Center**
- Details → Behavior → Visibility：**Collapsed**（C++ 自动控制显示）

> ⚠️ `RuneInfoCard` 必须是 Horizontal Box 的**最后一个子控件**（层级最下方），确保渲染在最上层。

#### 步骤 G：浮空图标 Image（拖拽跟手用）

**回到 Canvas Panel 根层**（不要放在 Border 里），放一个 **Image**：

- `Is Variable` 打勾，变量名改为 **`GrabbedRuneIcon`**
- Details → Behavior → Visibility：**Collapsed**
- Details → Behavior → `Is Hit Test Invisible`：**打勾**
- Size：**64 × 64**（和格子同尺寸）
- 位置随意，C++ 每帧会自动把它移到鼠标/手柄光标位置

#### 步骤 H：出售按钮

在 Canvas Panel 根层放一个 **Button**：

- `Is Variable` 打勾，变量名改为 **`SellButton`**
- 在 Button 里放一个 Text Block，内容 = `出售`
- 位置：拖到背包界面右下角合适的地方

---

## 第三步：配置视觉样式 DataAsset

格子颜色、尺寸等视觉参数通过 DataAsset 配置，不需要改代码。

### 3.1 创建 DA_BackpackStyle

Content Browser → `+ Add → Miscellaneous → Data Asset` → 选 **`BackpackStyleDataAsset`**

命名 **`DA_BackpackStyle`**，保存到 `Content/UI/Playtest_UI/`

### 3.2 填入 WBP_BackpackScreen

打开 `WBP_BackpackScreen` → Details 面板（右上角，不是控件 Details） → 搜索 **Style DA** → 填入 `DA_BackpackStyle`

> 如果看不到 Style DA，确认父类是 BackpackScreenWidget（不是 UserWidget）。

### 3.3 调整颜色（可选，有默认值）

双击打开 `DA_BackpackStyle`，可以修改：

| 参数 | 默认颜色 | 说明 |
| --- | --- | --- |
| Empty Color | 灰 | 未激活区空格 |
| Empty Active Color | 深蓝 | 激活区空格 |
| Occupied Active Color | 亮蓝 | 激活区有符文 |
| Occupied Inactive Color | 橙 | 非激活区有符文 |
| Selected Color | 金黄 | 鼠标点击选中 |
| Hover Color | 绿 | 拖拽悬浮目标格 |
| Cell Size | 64 px | 格子边长 |

---

## 第四步：配置背包网格大小

打开角色蓝图 **B_PlayerCharacterBase**：

1. 在 Components 面板找到 `BackpackGridComponent`
2. Details → **Config → Grid Width**：列数（默认 5）
3. Details → **Config → Grid Height**：行数（默认 5）

---

## 第五步：验证

在编辑器按 Play，然后：

1. 走近场景中的 `BP_RewardPickup` 按 `E`，三选一选一个符文
2. 按 **Tab** 打开背包
3. 验证以下内容：

| 检查 | 正常现象 |
| --- | --- |
| 格子出现 | 灰色/蓝色格子填满背包区 |
| 符文图标显示 | 刚选的符文出现在格子里 |
| 点格子 | 右边出现 RuneInfoCard 显示符文名称和描述 |
| 拖拽格子 | 按住符文拖动，浮空图标紧随鼠标，松手放置 |
| 悬浮高亮 | 拖拽时，悬浮的目标格变绿 |
| 出售 | 点选格子后点"出售"按钮，符文消失 |

---

## 遇到问题

| 问题 | 解决方法 |
| --- | --- |
| 格子没出现 | 检查 `BackpackGrid` 变量名拼写（大小写）；确认 UniformGridPanel 里没有手动放子控件 |
| 格子被拉伸不是正方形 | 包含 BackpackGrid 的 VerticalBox Slot → Size 改为 **Auto** |
| RuneInfoCard 没出现 | 检查变量名 `RuneInfoCard`；确认是 WBP_RuneInfoCard 实例（父类 RuneInfoCardWidget）；确认它是 HorizontalBox 最后一个子控件 |
| GrabbedRuneIcon 不跟鼠标 | 确认 Image 在 Canvas Panel 根层（不在 Border 里）；变量名精确为 `GrabbedRuneIcon` |
| Style DA 字段找不到 | 父类是否选了 BackpackScreenWidget（不是 UserWidget） |
| 左侧待放置区没格子 | 检查变量名 `PendingRuneGrid`；确认 UniformGridPanel 里没有手动子控件 |
