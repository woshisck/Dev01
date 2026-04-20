# 编辑器配置 TODO — 液态玻璃框 + 热度升阶炫彩

> 生成日期：2026-04-19  
> 对应功能：[UI-015] GlassFrameWidget、[VFX-004] 热度升阶玻璃边缘光  
> 全部为编辑器操作，不需要改 C++ 代码

---

## 任务总览

| # | 任务 | 预计时间 | 依赖 |
|---|------|----------|------|
| A | 创建 M_GlassFrame 材质 | 15 min | — |
| B | 创建 WBP_GlassFrame 蓝图 | 10 min | A |
| C | 更新玩家升阶 Overlay 材质（Custom 节点） | 10 min | — |
| D | 更新武器升阶 Overlay 材质（Custom 节点） | 10 min | — |
| E | 在背包界面使用 WBP_GlassFrame（可选） | 10 min | B |

---

## 任务 A：创建 M_GlassFrame 材质

### A-1 新建材质文件

1. Content Browser → 建议路径 `Content/UI/Materials/`
2. 右键 → **Material** → 命名 `M_GlassFrame`
3. 双击打开材质编辑器

### A-2 设置材质属性

点击材质输出节点（画布中心那个大节点），在 **Details** 面板设置：

| 属性 | 值 |
|------|----|
| **Material Domain** | `User Interface` |
| **Blend Mode** | `Translucent` |
| **Shading Model** | `Unlit` |

### A-3 创建 7 个 Scalar Parameter 节点

在画布空白处按住 **S + 左键** 创建 ScalarParameter（或右键 → ScalarParameter），共创建 7 个：

| 参数名（精确） | 默认值 | 用途 |
|----------------|--------|------|
| `CornerRadius`  | 0.06   | 圆角半径（越大越圆） |
| `BorderWidth`   | 0.025  | 边框宽度 |
| `FresnelPower`  | 2.5    | 菲涅尔集中度（大→光越集中在角落） |
| `IridIntensity` | 0.18   | 炫彩强度 |
| `IridSpeed`     | 0.04   | 炫彩流速 |

> ⚠️ 参数名大小写必须和上表完全一致，C++ 用字符串 Key 来写入，写错不报错但参数无效。

### A-4 创建其余标准节点

- **TexCoord[0]**：右键 → Texture Coordinate（默认 index=0）
- **Time**：右键 → Time

### A-5 添加 Custom 节点

右键 → **Custom**，点选它，在 **Details** 面板配置：

**Output Type**：`CMOT Float 4`

**Include File Paths（关键！和玩家/武器材质一样的做法）**：

Details 面板找到 **Include File Paths** 数组 → 点 `+` → 填入：

```text
/Project/GlassFrameUI.ush
```

**Inputs（按顺序添加，名称精确）**：

| Name | Type |
|------|------|
| UV | Float2 |
| Time | Float1 |
| CornerRadius | Float1 |
| BorderWidth | Float1 |
| FresnelPower | Float1 |
| IridIntensity | Float1 |
| IridSpeed | Float1 |

点 Inputs 数组右边的 **+** 按钮添加，每条填名字和类型。

**Code 字段（只需一行函数调用）**：

```hlsl
return GlassFrameMain(UV, Time, CornerRadius, BorderWidth, FresnelPower, IridIntensity, IridSpeed);
```

> ⚠️ 所有 HLSL 逻辑都在 `Shaders/GlassFrameUI.ush` 里，该文件通过 `FDevKitModule::StartupModule()` 注册的 `/Project` 虚拟路径访问，跨电脑有效。UI 材质无顶点法线，ush 内用 UV 离中心距离模拟菲涅尔。

### A-6 连接节点

按以下连线：

```
TexCoord[0]      → Custom.UV
Time             → Custom.Time
CornerRadius     → Custom.CornerRadius
BorderWidth      → Custom.BorderWidth
FresnelPower     → Custom.FresnelPower
IridIntensity    → Custom.IridIntensity
IridSpeed        → Custom.IridSpeed

Custom (输出Float4)
  → Break Float 4（右键 → Break Float 4 Value）
       → RGB (Float3) → 材质输出节点 Emissive Color
       → A  (Float1) → 材质输出节点 Opacity
```

### A-7 保存并编译

Ctrl+S → 等待编译完成（左下角进度条消失）。

---

## 任务 B：创建 WBP_GlassFrame 蓝图

### B-1 新建 Widget Blueprint

1. Content Browser → 建议路径 `Content/UI/Widgets/Glass/`
2. 右键 → **User Interface → Widget Blueprint**
3. 弹出对话框选父类：搜索 `GlassFrameWidget`，选中 → Create
4. 命名 `WBP_GlassFrame`

### B-2 搭建 Designer 层级

打开 WBP_GlassFrame，切到 **Designer** 标签。

在 **Hierarchy** 面板搭建以下层级（根节点默认是 CanvasPanel，我们改成 Overlay）：

```
[Root]  Overlay                   ← 删掉默认 CanvasPanel，拖入 Overlay 作根节点
  ├── BackgroundBlur              命名精确: GlassBG
  ├── Image                       命名精确: GlassBorderImage
  └── NamedSlot                   命名: Content（或任意名）
```

**如何添加控件**：在 Palette 搜索对应名称，拖到 Hierarchy 里。

### B-3 设置每个控件属性

#### BackgroundBlur（名称 GlassBG）

| 属性 | 值 |
|------|----|
| Slot → Size | **Fill**（横向纵向都 Fill，撑满父级） |
| Blur Strength | 14 |
| Apply Alpha to Blur | ✓ 勾选 |
| Color and Opacity → A | 0.55 |

#### Image（名称 GlassBorderImage）

| 属性 | 值 |
|------|----|
| Slot → Size | **Fill** |
| Brush → Image | 留空（C++ 运行时会把 DynMat 设置进来） |
| Color and Opacity | White (1,1,1,1) |

#### NamedSlot（名称 Content）

| 属性 | 值 |
|------|----|
| Slot → Size | **Fill** |

### B-4 填入材质

在 **Details** 面板（选中 WBP_GlassFrame 根节点或空白处）找到分类 **玻璃框|材质**：

| 属性 | 值 |
|------|----|
| Glass Border Material | `M_GlassFrame` |

### B-5 保存并验证

Ctrl+S。在 Designer 视图中可以看到预览（编辑器里 BackgroundBlur 会显示为灰色，运行时才有模糊效果）。

---

## 任务 C：更新玩家升阶 Overlay 材质

找到你现有的玩家热度升阶 Overlay 材质（`BP_PlayerCharacterBase` → Heat|Visual → `PhaseUpPlayerOverlayMaterial` 指向的那个材质）。

打开材质，找到 **Custom 节点**。

### C-1 更新 Include File Paths

点选 Custom 节点，在 **Details** 面板找到 **Include File Paths**：

- 点 `+` 添加一条
- 填入：`/Project/PlayerGlowOverlay.ush`

> 这个路径由 C++ 注册，不是文件系统路径。如果编译报"file not found"，说明模块没编译成功，重启编辑器重新编译即可。

### C-2 更新 Inputs（新增 4 个）

在 Details → Inputs 数组，添加以下 4 个（已有的保留，不要删）：

| Name | Type | 说明 |
|------|------|------|
| `Time` | Float1 | 连接 Time 节点 |
| `BandWidth` | Float1 | 连接 ScalarParam "BandWidth" 默认 0.15 |
| `IridIntensity` | Float1 | 连接 ScalarParam "IridIntensity" 默认 0.28 |
| `IridSpeed` | Float1 | 连接 ScalarParam "IridSpeed" 默认 0.06 |

同时检查已有的 Inputs，确保以下 7 个都存在（顺序要和 Code 里的函数参数顺序对齐）：

| 顺序 | Name | 连接 |
|------|------|------|
| 1 | UV | TexCoord[0] |
| 2 | N | VertexNormalWS |
| 3 | V | CameraVectorWS |
| 4 | Time | **新增** Time 节点 |
| 5 | EmissiveColor | Vector Parameter "EmissiveColor" |
| 6 | SweepProgress | Scalar Parameter "SweepProgress" |
| 7 | GlowAlpha | Scalar Parameter "GlowAlpha" |
| 8 | FresnelPower | Scalar Parameter "FresnelPower"（默认 2.5） |
| 9 | BandWidth | **新增** Scalar Parameter "BandWidth"（默认 0.15） |
| 10 | SwipeCount | Scalar Parameter "SwipeCount" |
| 11 | IridIntensity | **新增** Scalar Parameter "IridIntensity"（默认 0.28） |
| 12 | IridSpeed | **新增** Scalar Parameter "IridSpeed"（默认 0.06） |

### C-3 替换 Code 字段

把 Code 字段内容**全部替换**为：

```hlsl
return PlayerGlowMain(UV, N, V, Time, EmissiveColor, SweepProgress, GlowAlpha, FresnelPower, BandWidth, SwipeCount, IridIntensity, IridSpeed);
```

> ⚠️ 参数顺序必须与函数定义一致，不能乱。函数定义在 `/Project/PlayerGlowOverlay.ush`。

### C-4 添加新 Scalar Parameters（如果尚未添加）

在材质图表空白处创建：

| 参数名 | 默认值 |
|--------|--------|
| `BandWidth` | 0.15 |
| `IridIntensity` | 0.28 |
| `IridSpeed` | 0.06 |

并连线到 Custom 节点对应 Input。

### C-5 保存编译

Ctrl+S，等待编译完成。

---

## 任务 D：更新武器升阶 Overlay 材质

找到武器热度升阶材质（在某个 `DA_WPN_*` → `HeatOverlayMaterial` 里配置的材质）。

步骤与任务 C 完全一致，区别只有两处：

**D-1 Include File Paths** 填：
```
/Project/WeaponGlowOverlay.ush
```

**D-3 Code 字段**替换为：
```hlsl
return WeaponGlowMain(UV, N, V, Time, EmissiveColor, SweepProgress, GlowAlpha, FresnelPower, BandWidth, SwipeCount, IridIntensity, IridSpeed);
```

**D-4 IridIntensity 默认值** 用 `0.18`（武器比玩家弱）

其余 Inputs / ScalarParams / 连线与任务 C 相同。

---

## 任务 E：在背包界面使用 WBP_GlassFrame（可选）

如果你希望背包的容器用玻璃质感，可以在 `WBP_BackpackScreen` 里：

1. 找到背包格子的外层容器
2. 将外层替换为 `WBP_GlassFrame`（作为父级容器）
3. 把背包格子拖入 `WBP_GlassFrame` 的 `Content` NamedSlot
4. 调整参数：`BlurStrength=14 / CornerRadius=0.06 / IridIntensity=0.18`

---

## 验证清单

完成全部任务后，进 PIE 测试：

| 测试项 | 预期效果 |
|--------|----------|
| 打开背包 | 背包界面背景有毛玻璃模糊（能看到后方场景，略模糊） |
| 背包边框 | 圆角方形边框，角落有淡淡炫彩缓慢流动 |
| 玩家升至热度阶段 1/2/3 | 角色边缘出现对应颜色的菲涅尔光，极边缘处有彩虹光 |
| 武器升阶 | 武器边缘也有同类效果，炫彩比玩家略弱 |
| 移动时炫彩 | 炫彩根据法线方向分布，不随时间快速闪烁（IridSpeed≈0.04–0.06） |

---

## 各场景推荐参数速查

| 场景 | BlurStrength | CornerRadius | BorderWidth | FresnelPower | IridIntensity | IridSpeed |
|------|---|---|---|---|---|---|
| 背包主界面 | 14 | 0.06 | 0.025 | 2.5 | 0.18 | 0.04 |
| 武器图案框（WBP_WeaponGlassIcon） | 10 | 0.08 | 0.025 | 2.5 | 0.12 | 0.04 |
| HUD 背包缩略框 | 6  | 0.10 | 0.020 | 3.0 | 0.08 | 0.03 |
| 三选一符文奖励框 | 16 | 0.05 | 0.030 | 2.0 | 0.22 | 0.05 |
| 玩家升阶炫彩强度（材质参数） | — | — | — | — | 0.28 | 0.06 |
| 武器升阶炫彩强度（材质参数） | — | — | — | — | 0.18 | 0.06 |

---

## 常见错误排查

| 现象 | 原因 | 解决 |
|------|------|------|
| 边框完全不显示 | GlassBorderMaterial 未填 | WBP Details → 玻璃框\|材质 → 填 M_GlassFrame |
| 背景没有模糊 | 控件名不是 `GlassBG` | 在 Hierarchy 精确改名 |
| Custom 节点报 "file not found" | 引擎未编译 DevKit 模块 | 关编辑器 → 重新编译 → 重开 |
| Custom 节点报 "undeclared identifier PlayerGlowMain" | Include File Paths 未填 | Details → Include File Paths → 填路径 |
| Custom 节点报 "too many arguments" | Code 里函数参数数量与 Inputs 不匹配 | 检查 Inputs 是否有对应 11 个（按顺序） |
| 炫彩闪烁太快 | IridSpeed 过大 | 改为 0.02–0.06 |
| 参数修改运行时无效 | 忘记调 ApplyGlassStyle | 蓝图里改完属性后调 ApplyGlassStyle 节点 |
