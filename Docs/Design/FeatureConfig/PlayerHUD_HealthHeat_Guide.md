# 玩家 HUD：血条边框 + 热度条升阶抖动 制作指南

> 适用版本：UE 5.4 | 纯 WBP/Blueprint，无需 C++ 改动

---

## 一、血条边框

### 思路

用 **Overlay** 叠两层：底层是 `ProgressBar`（血条本体），顶层是一张 `Image`（边框贴图）。Image 完全覆盖 ProgressBar，颜色/尺寸独立控制。

### 步骤

1. 打开血条所在 WBP（如 `WBP_PlayerMain`）
2. 找到现有 `ProgressBar`（血条），在 Hierarchy 里选中它
3. 右键 → **Wrap With → Overlay**
4. 在 Overlay 内，**ProgressBar 上方**再添加一个 `Image`（它会自动叠在 ProgressBar 之上）
5. 选中该 `Image`：
   - **Brush → Image**：指定边框贴图（建议 9-Slice PNG，四角固定，中心透明）
   - **Brush → Draw As**：`Border`（仅绘制九宫格边缘）或 `Box`（完整九宫格含中心）
   - **Brush → Margin**：填写九宫格边距（与贴图切割比例一致，如 0.1）
   - **Color and Opacity**：调整边框颜色/透明度
6. 选中 Image 的 **Slot**：
   - Horizontal / Vertical Alignment 均设为 `Fill`，让边框撑满 Overlay
   - Padding 全部 `0`

> **如果没有专用边框贴图**：Draw As 改为 `Box`，Image 留空，Tint Color 设为半透明白色，可做简单实色框。

### 层级结构参考

```
Overlay
├── ProgressBar          ← 血条本体（Alignment: Fill/Fill）
└── Image (Border)       ← 边框图（Alignment: Fill/Fill, Padding 0）
```

---

## 二、热度条升阶抖动（缩放弹跳）

### 思路

在 WBP 里制作一段 UMG Animation（`Anim_HeatPhaseUp`），对热度条容器做 Scale 关键帧弹跳。当热度阶段变化时从蓝图触发播放。

### 步骤

#### 2-1 制作动画

1. 打开热度条所在 WBP
2. 在底部 **Animations** 面板点 `+` → 命名 `Anim_HeatPhaseUp`
3. 点 **+ Track** → 选择热度条的**父容器 Widget**（建议套一层 `SizeBox` 或直接选 `ProgressBar`）
4. 展开该 Track → **Add Transform Track**
5. 确保时间轴在 `0.00`，在 **Scale** 行点 `+` 添加关键帧（值 `1.0, 1.0`）
6. 按下表添加其余关键帧：

| 时间（s） | Scale X | Scale Y | 说明 |
|-----------|---------|---------|------|
| 0.00 | 1.00 | 1.00 | 初始 |
| 0.06 | 1.18 | 1.18 | 快速放大 |
| 0.14 | 0.90 | 0.90 | 回弹缩小 |
| 0.22 | 1.10 | 1.10 | 二次弹跳 |
| 0.32 | 0.97 | 0.97 | 轻微回弹 |
| 0.40 | 1.00 | 1.00 | 归位 |

> 插值类型选 **Cubic（Auto）** 让过渡更顺滑。可在 Details 面板调整每个关键帧的 Tangent。

#### 2-2 触发动画

热度变化事件已通过 `OnHeatPhaseChanged` 委托广播，在 **WBP 蓝图事件图** 里接收并播放：

**方法 A：在 WBP 里绑定 C++ 委托（推荐）**

```
Event NativeConstruct / Event Construct
  → Get Owning Player Pawn
  → Cast to PlayerCharacterBase
  → Bind Event to OnHeatPhaseChanged → 自定义 Event: OnHeatChanged(NewPhase)

Event OnHeatChanged(NewPhase)
  → 判断 NewPhase > 0（排除初始化调用）
  → Play Animation → Anim_HeatPhaseUp
     Num Loops to Play: 1
     Play Mode: Forward
```

**方法 B：直接在 HUD/PlayerMain WBP 里调用**

如果热度条 WBP 是 `WBP_PlayerMain` 的子 Widget，在 PlayerMain 里收到事件后调用热度条 Widget 的 `Play Animation` 节点。

#### 2-3 确保 Render Transform 中心点正确

抖动时默认以控件**左上角**为缩放原点，会导致位移感。需要设置缩放原点为控件中心：

- 选中热度条容器，Details → **Render Transform → Pivot** 改为 `0.5, 0.5`

---

## 三、可调参数汇总

| 参数 | 位置 | 说明 |
|------|------|------|
| 边框贴图 Margin | WBP Image → Brush → Margin | 九宫格边距，需与贴图切割比例匹配 |
| 边框颜色 | WBP Image → Color and Opacity | 可做渐变色或跟随热度阶段变色 |
| 抖动 Scale 峰值 | Anim 关键帧 0.06s | 默认 1.18，越大越夸张 |
| 抖动时长 | 最后一帧时间 | 默认 0.40s，可压缩到 0.25s 更干脆 |
| 缩放原点 | Render Transform → Pivot | 必须 `0.5, 0.5` 否则有位移感 |
