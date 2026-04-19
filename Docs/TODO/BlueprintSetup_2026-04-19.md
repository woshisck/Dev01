# 蓝图配置 TODO — 2026-04-19

> 所有 C++ 逻辑已完成，本文档只涉及**编辑器操作**：搭控件层级、填 Details、创建资产。  
> 不需要在 Event Graph 写任何蓝图节点。

---

## 概览：需要完成的任务

| # | 任务 | 类型 | 预计时间 |
|---|------|------|---------|
| 1 | 创建材质 M_GlassFrame | 新建材质 | 10 min |
| 2 | 修改 WBP_WeaponFloat（加 InfoContainer） | 改现有 WBP | 5 min |
| 3 | 创建 WBP_WeaponGlassIcon | 新建 WBP | 10 min |
| 4 | 创建 DA_WeaponGlassAnim | 新建 Data Asset | 3 min |
| 5 | 创建 WBP_EnemyArrow | 新建 WBP | 5 min |
| 6 | 配置 HUD 蓝图 Details | 填引用 | 3 min |
| 7 | 检查 BackpackScreen 是否重复创建 | 排查 | 3 min |
| 8 | 连接武器拾取调用 | 蓝图节点 | 5 min |

---

## 任务 1：创建材质 M_GlassFrame

**路径：** `Content/UI/Materials/M_GlassFrame`（路径可自定义，只要 HUD 能引用即可）

### 1.1 材质基础设置

Content Browser → 右键 → Material，命名 `M_GlassFrame`，打开材质编辑器：

| 设置项 | 值 |
|--------|-----|
| Material Domain | **User Interface** |
| Blend Mode | **Translucent** |
| Shading Model | Unlit |

### 1.2 创建 5 个 Scalar Parameter 节点

右键 → `ScalarParameter`，依次创建（名称必须**完全一致**，C++ 通过名称写入）：

| 参数名 | 默认值 |
|--------|--------|
| `CornerRadius` | 0.06 |
| `BorderWidth` | 0.025 |
| `FresnelPower` | 2.5 |
| `IridIntensity` | 0.18 |
| `IridSpeed` | 0.04 |

### 1.3 创建 Custom 节点（核心 HLSL）

右键 → `Custom`，设置：
- **Output Type：** CMOT Float 4
- **Description：** GlassFrame
- **Inputs（顺序必须一致）：**

| Input Name | Type | 连接到 |
|------------|------|--------|
| UV | Float2 | `TexCoord[0]` 节点 |
| Time | Float1 | `Time` 节点 |
| CornerRadius | Float1 | 对应 Scalar Parameter |
| BorderWidth | Float1 | 对应 Scalar Parameter |
| FresnelPower | Float1 | 对应 Scalar Parameter |
| IridIntensity | Float1 | 对应 Scalar Parameter |
| IridSpeed | Float1 | 对应 Scalar Parameter |

**Code 框内填入以下 HLSL（全选粘贴）：**

```hlsl
float2 uv = UV - 0.5;
float2 q = abs(uv) - (0.5 - CornerRadius);
float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - CornerRadius;

float innerMask  = 1.0 - smoothstep(-0.002, 0.0, dist);
float borderMask = 1.0 - smoothstep(0.0, BorderWidth, abs(dist));

float fresnelRaw = length(abs(uv) * 2.0);
fresnelRaw = saturate(fresnelRaw);
float fresnel = pow(fresnelRaw, FresnelPower);

float angle  = atan2(uv.y, uv.x);
float hueRaw = frac(angle / 6.28318 + Time * IridSpeed);

float3 irid;
float h6 = hueRaw * 6.0;
irid.r = saturate(abs(h6 - 3.0) - 1.0);
irid.g = saturate(2.0 - abs(h6 - 2.0));
irid.b = saturate(2.0 - abs(h6 - 4.0));

float iridMask   = borderMask * pow(fresnel, 3.0);
float3 iridColor = irid * iridMask * IridIntensity;

float3 borderBaseColor = float3(0.85, 0.90, 1.00);
float  borderAlpha     = borderMask * lerp(0.30, 0.72, fresnel);

float3 innerColor = float3(0.05, 0.05, 0.10);
float  innerAlpha = innerMask * 0.12;

float3 finalColor = (borderBaseColor + iridColor) * borderMask
                  + innerColor * innerMask * (1.0 - borderMask);
float  finalAlpha = saturate(borderAlpha + innerAlpha);

return float4(finalColor, finalAlpha);
```

### 1.4 连接输出引脚

Custom 节点输出 RGBA → 右键 → `Break Float 4 (R, G, B, A)`：
- **RGB** → 材质输出节点的 `Emissive Color`
- **A** → 材质输出节点的 `Opacity`

Save → Apply。

---

## 任务 2：修改 WBP_WeaponFloat（加 InfoContainer）

打开已有的 `WBP_WeaponFloat`，进入 Designer 面板：

### 2.1 当前层级（假设是这样）

```
[Root] Canvas / Overlay
├── WeaponThumbnail   ← Image，保持不动
├── WeaponNameText    ← TextBlock
├── WeaponDescText    ← TextBlock
├── WeaponSubDescText ← TextBlock
├── ZoneGrid1/2/3     ← CanvasPanel
├── Zone1Image/2/3    ← Image（可选）
└── RuneListBox       ← VerticalBox
```

### 2.2 目标层级

```
[Root] Canvas / Overlay
├── WeaponThumbnail                ← Image，保持在外面（飞行时需要它）
└── [新增] Overlay/Border 命名 "InfoContainer"
    ├── WeaponNameText
    ├── WeaponDescText
    ├── WeaponSubDescText
    ├── ZoneGrid1 / Zone1Image
    ├── ZoneGrid2 / Zone2Image
    ├── ZoneGrid3 / Zone3Image
    └── RuneListBox
```

### 2.3 操作步骤

1. 在 Hierarchy 中**选中**所有非 Thumbnail 控件（多选 Shift/Ctrl）
2. 右键 → **Wrap With → Overlay**（或 Border、Box，任意容器均可）
3. 在 Hierarchy 中将新容器**重命名为 `InfoContainer`**（名称必须精确）
4. 确认 `WeaponThumbnail` 还在 `InfoContainer` 外面（同级或其他位置）

> **为什么这样做：** C++ 在折叠动画开始时会找名为 `InfoContainer` 的控件统一 `Collapsed`，  
> 这样只需一个 `SetVisibility` 调用，而不是逐个隐藏文字和格子。

Compile → Save。

---

## 任务 3：创建 WBP_WeaponGlassIcon

**路径：** `Content/UI/Widgets/WBP_WeaponGlassIcon`（路径自定义）

### 3.1 创建步骤

Content Browser → 右键 → User Interface → Widget Blueprint  
弹出选择父类窗口 → 搜索 `WeaponGlassIconWidget` → 选择 → 命名 `WBP_WeaponGlassIcon`

### 3.2 Designer 层级

打开 Designer，删除默认的 Canvas Panel，改为：

```
[Root] Overlay（Size Fill 填满，Alignment 0.5/0.5）
├── BackgroundBlur       命名 "GlassBG"
├── Image                命名 "GlassBorderImage"
├── Image                命名 "WeaponThumbnailImg"
└── Image                命名 "HeatColorOverlay"
```

逐一说明：

| 控件 | 名称 | 关键设置 |
|------|------|---------|
| Overlay（根） | — | Size To Content 取消，Anchors 填满父级 |
| BackgroundBlur | `GlassBG` | Size Fill，Apply Alpha to Blur ✓，Brush Alpha 0.55 |
| Image（边框） | `GlassBorderImage` | Size Fill，Brush 留空（运行时 DynMat） |
| Image（缩略图） | `WeaponThumbnailImg` | Size Fill，初始可见性 Collapsed |
| Image（热度叠加） | `HeatColorOverlay` | Size Fill，Blend Mode Additive，初始颜色 (0,0,0,0) |

> 名称**区分大小写**，必须完全一致，否则 `BindWidgetOptional` 不会绑定。

### 3.3 Details 配置

选中根 Widget（Class Defaults / Widget Blueprint Details）：

| 属性 | 值 |
|------|----|
| **玻璃框\|材质 → GlassBorderMaterial** | `M_GlassFrame` |
| 玻璃框\|模糊 → BlurStrength | `6`（HUD 小图标推荐值） |
| 玻璃框\|边框 → CornerRadius | `0.10` |
| 玻璃框\|边框 → BorderWidth | `0.025` |
| 玻璃框\|菲涅尔 → FresnelPower | `2.5` |
| 玻璃框\|炫彩 → IridIntensity | `0.08`（HUD 小图标要弱） |
| 玻璃框\|炫彩 → IridSpeed | `0.04` |

Compile → Save。

---

## 任务 4：创建 DA_WeaponGlassAnim

Content Browser → 右键 → Miscellaneous → **Data Asset**  
弹出选择类窗口 → 搜索 `WeaponGlassAnimDA` → 选择 → 命名 `DA_WeaponGlassAnim`

打开资产，填写以下参数：

| 分类 | 参数 | 推荐值 | 说明 |
|------|------|--------|------|
| 时序 | AutoCollapseDelay | `2.5` | 武器浮窗显示后自动折叠的等待秒数 |
| 动画\|折叠 | CollapseDuration | `0.25` | 文字/符文区消失的过渡时间（秒） |
| 动画\|缩小 | ShrinkDuration | `0.35` | 浮窗缩小到图标尺寸的时间（秒） |
| 动画\|缩小 | GlassIconSize | `X=64, Y=64` | HUD 图标尺寸（px），与 WBP_WeaponGlassIcon 实际显示尺寸一致 |
| 动画\|飞行 | FlyDuration | `0.45` | 飞向 HUD 左下角的时间（秒） |
| 动画\|飞行 | HUDOffsetFromBottomLeft | `X=44, Y=120` | 图标中心距屏幕左边/底部的像素偏移 |
| 动画\|消失 | ExpandDuration | `0.20` | 开背包时图标放大消失的时间（秒） |
| 动画\|消失 | ExpandScale | `1.35` | 消失时的最大放大倍率 |
| 外观 | ThumbnailFlyOpacity | `0.45` | 飞行过程中缩略图的半透明度 |

Save。

---

## 任务 5：创建 WBP_EnemyArrow

**路径：** `Content/UI/Widgets/WBP_EnemyArrow`

### 5.1 创建步骤

Content Browser → 右键 → User Interface → Widget Blueprint  
父类选 `EnemyArrowWidget` → 命名 `WBP_EnemyArrow`

### 5.2 Designer 层级

删除默认根节点，添加：

```
[Root] Canvas Panel    命名 "RootCanvas"
```

仅这一个控件，无子控件。  
**Anchors：全屏（左上 0,0，右下 1,1），Offset 全部填 0。**  
Size to Content **不勾选**，让它完全填满屏幕。

> C++ 在 NativeConstruct 里会动态向 `RootCanvas` 添加 MaxArrows 个 Image 箭头，  
> 不需要在 Designer 里手动放箭头。

### 5.3 Details 配置

| 属性 | 值 | 说明 |
|------|----|------|
| **ArrowTexture** | （你的三角箭头贴图） | 必填。贴图约定：顶点朝上（-Y 方向） |
| AppearDelay | `1.5` | 屏幕内无敌人且无受伤超过多少秒后显示 |
| MaxArrows | `3` | 最多同时显示几个箭头 |
| ArrowSize | `32` | 箭头图标尺寸（px） |
| EdgeMargin | `60` | 距屏幕边缘留白（px） |
| ArrowColor | R=1, G=0.8, B=0.2, A=0.9 | 黄色，可自定义 |
| OnScreenShrink | `150` | 敌人离屏幕边缘多少 px 内算"在屏幕内"（值大=箭头出现更早） |
| ArrowAngleOffset | `90` | 贴图顶点朝上时填 90，朝右填 0，朝下填 -90 |

Compile → Save。

---

## 任务 6：配置 HUD 蓝图 Details

打开 **BP_YogHUD**（或你的 HUD 蓝图），点击 Class Defaults：

| 分类 | 属性 | 填入 |
|------|------|------|
| Backpack | BackpackScreenClass | `WBP_BackpackScreen` |
| WeaponGlass | WeaponFloatClass | `WBP_WeaponFloat` |
| WeaponGlass | WeaponGlassIconClass | `WBP_WeaponGlassIcon` |
| WeaponGlass | WeaponGlassAnimDA | `DA_WeaponGlassAnim` |
| EnemyArrow | EnemyArrowWidgetClass | `WBP_EnemyArrow` |
| Tutorial | TutorialPopupClass | `WBP_TutorialPopup`（已有） |
| PauseEffect | PauseFadeDuration | `0.25` |
| PauseEffect | PauseTargetSaturation | `0.10` |
| PauseEffect | PauseTargetGain | `0.40` |

Compile → Save。

---

## 任务 7：检查 BackpackScreen 是否重复创建

**YogHUD 已经在 BeginPlay 里负责创建 WBP_BackpackScreen 并 AddToViewport。**  
如果之前有其他地方也在创建它，会出现两个背包界面叠在一起。

### 排查方法

在 Content Browser 中右键 `WBP_BackpackScreen` → **Reference Viewer**（或 Find Referencers），  
找到所有蓝图引用它的地方。

### 需要删除的模式

如果在任何蓝图 Event Graph 里看到类似这样的节点：

```
Create Widget (Class = WBP_BackpackScreen) → Add to Viewport
```

**删掉这段逻辑**，保留 HUD 里的那份。

---

## 任务 8：连接武器拾取调用

武器拾取触发点（通常是武器 Actor 的 `OnPickedUp` 事件或 GameMode 里的逻辑）：

### 需要在蓝图里添加的节点

```
[OnWeaponPickedUp 事件]
  → Get Player Controller
  → Get HUD
  → Cast to YogHUD
  → TriggerWeaponPickup (Def = 武器定义引用)
```

**`Def` 参数** 是 `UWeaponDefinition*` 类型，从武器 Actor 的定义资产里传入。

---

## 验证清单

完成以上配置后，进入 PIE 按以下顺序验证：

- [ ] **武器浮窗显示**：拾取武器后浮窗正常弹出（缩略图 + 名称 + 激活区）
- [ ] **自动折叠**：2.5 秒后文字区消失，缩略图缩小飞向左下角
- [ ] **玻璃图标**：飞行结束后左下角出现玻璃图标（毛玻璃底 + 炫彩边框 + 缩略图）
- [ ] **开背包消失**：打开背包时玻璃图标放大渐隐消失
- [ ] **暂停效果**：打开背包/三选一时屏幕变暗去饱和，关闭后恢复
- [ ] **三选一后打开背包**：选完符文后背包自动弹出
- [ ] **敌人箭头**：站定 1.5 秒且屏幕内没有敌人 → 屏幕边缘出现黄色箭头
- [ ] **箭头消失**：走近任意敌人（敌人进入屏幕内）→ 箭头消失

---

## 参数速查

### WBP_WeaponGlassIcon 推荐参数

| 参数 | 推荐值 |
|------|--------|
| BlurStrength | 6 |
| CornerRadius | 0.10 |
| BorderWidth | 0.025 |
| FresnelPower | 2.5 |
| IridIntensity | 0.08 |
| IridSpeed | 0.04 |

### DA_WeaponGlassAnim 推荐参数

| 参数 | 推荐值 |
|------|--------|
| AutoCollapseDelay | 2.5s |
| CollapseDuration | 0.25s |
| ShrinkDuration | 0.35s |
| GlassIconSize | 64×64 px |
| FlyDuration | 0.45s |
| HUDOffsetFromBottomLeft | X=44, Y=120 |
| ExpandDuration | 0.20s |
| ExpandScale | 1.35 |
| ThumbnailFlyOpacity | 0.45 |

### WBP_EnemyArrow 推荐参数

| 参数 | 推荐值 |
|------|--------|
| AppearDelay | 1.5s |
| MaxArrows | 3 |
| ArrowSize | 32 px |
| EdgeMargin | 60 px |
| OnScreenShrink | 150 px |
| ArrowAngleOffset | 90°（贴图顶点朝上） |
| ArrowColor | (1.0, 0.8, 0.2, 0.9) |
