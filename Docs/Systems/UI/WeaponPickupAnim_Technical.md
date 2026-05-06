# 武器拾取 HUD 动画系统 — 技术文档

> 更新：2026-05-06

## 概述

玩家靠近武器时，武器信息浮窗由 HUD 层创建并加入视口；`WeaponSpawner`
每帧提供武器世界位置，`AYogHUD` 将其投影到屏幕并夹在可视范围内，让浮窗保持在武器旁边且不会越过屏幕边界。武器浮窗显示期间会隐藏关卡/Portal 预览浮窗，避免两个浮窗重叠。

拾取武器后，HUD 信息区内的 **武器卡片** 折叠，然后缩略图飞向左下角 **液态玻璃图标**，飞行过程中带**流光拖尾**。

```
WeaponSpawner::TryPickupWeapon()
  └─ YogHUD::StartWeaponFloatPickup(Def)
       ├─ WeaponFloatWidget::StartCollapse()
       └─ OnCollapseComplete
            └─ YogHUD::TriggerWeaponPickup(Def, ThumbnailScreenCenter)
                 ├─ WeaponThumbnailFlyWidget::StartFly()
                 │    └─ OnFlyProgress.Broadcast() ← 每帧
                 │         └─ WeaponTrailWidget::SetTrailEndpoints()
                 └─ OnFlyComplete.Broadcast()
                      ├─ WeaponTrailWidget::StartFadeOut()
                      └─ WeaponGlassIconWidget::Show()
```

---

## 数据资产：WeaponGlassAnimDA

资产路径：`Content/Docs/UI/Tutorial/DA_WeaponGlassAnim.uasset`

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `AutoCollapseDelay` | 2.5s | 浮窗显示多久后自动开始折叠 |
| `CollapseDuration` | 0.25s | 折叠等待（InfoContainer 瞬间隐藏）|
| `ShrinkDuration` | 0.35s | 缩小至玻璃图标大小所需时间 |
| `FlyDuration` | 0.45s | 飞向左下角所需时间 |
| `GlassIconSize` | (64, 64) | 飞行目标尺寸（影响 ShrinkScale 计算）|
| `HUDOffsetFromBottomLeft` | (44, 120) | 图标距屏幕左/下边缘的像素偏移 |
| `ThumbnailFlyOpacity` | 0.45 | 飞行中缩略图透明度 |
| `ExpandDuration` | 0.20s | 打开背包时图标放大消失时长 |
| `ExpandScale` | 1.35 | 放大消失倍率 |

---

## C++ 类说明

### WeaponFloatWidget

**文件**：`Source/DevKit/Public/UI/WeaponFloatWidget.h`

WBP 命名要求（BindWidgetOptional）：

| 控件名 | 类型 | 说明 |
|--------|------|------|
| `WeaponThumbnail` | Image | 缩略图，折叠/飞行时保留 |
| `InfoContainer` | Widget（任意容器） | 折叠阶段整体 Collapsed |
| `WeaponNameText` | TextBlock | 武器名 |
| `WeaponDescText` | TextBlock | 描述（空时自动隐藏）|
| `WeaponSubDescText` | TextBlock | 子描述 |
| `ZoneGrid1/2/3` | CanvasPanel | 激活区点阵（60×60 px）|
| `Zone1/2/3Image` | Image | 激活区覆盖图（优先于点阵）|
| `RuneListBox` | VerticalBox | 初始卡牌列表，优先显示 `WeaponDefinition.InitialCombatDeck` |
| `CardScrollBox` | ScrollBox | 可选；若 WBP 未提供，C++ 会运行时把 `RuneListBox` 包进 ScrollBox |

主要接口：

```cpp
// 设置武器数据（显示阶段调用）
void SetWeaponDefinition(const UWeaponDefinition* Def);

// 开始折叠；完成后由 HUD 根据缩略图位置触发飞行动画
void StartCollapse(float InDuration = 0.25f);

// 折叠完成广播（参数：缩略图屏幕中心）
FOnWeaponFloatCollapseComplete OnCollapseComplete;
```

**WBP 布局关键规则**：
- Root Canvas Panel：Fill Screen，**无不透明背景**（透明叠层）
- 卡片背景 Image 必须放在 `InfoContainer` 内部，否则折叠后留白
- 运行时 HUD 会用 360×560 的 `SizeBox` 包住本 Widget，并启用 ClipToBoundsAlways；文本必须开启自动换行。

**卡牌数据来源**：
- 优先读取 `WeaponDefinition.InitialCombatDeck`。
- `InitialCombatDeck` 为空时，从旧字段 `InitialRunes` 兼容回退。
- 只显示 `RuneInfo.CombatCard.bIsCombatCard = true` 的条目。
- 卡牌名优先用 `CombatCard.DisplayName`。
- 卡牌摘要优先用 `CombatCard.HUDSummaryText`；为空时从 `HUDReasonText` / 符文描述自动裁成短摘要。
- 卡牌列表只显示图标、名称和 1-2 行摘要，不显示 CardId、效果 Tag、连携配方等调试信息。
- 鼠标滚轮可滚动卡牌列表；手柄十字键/左摇杆上下由 `YogPlayerControllerBase` 转发到 `YogHUD::ScrollWeaponFloatCards`。

### WeaponTrailWidget

**文件**：`Source/DevKit/Public/UI/WeaponTrailWidget.h`

WBP 要求：根 Canvas Panel（全屏，HitTestInvisible）+ Image 命名 `TrailLine`

```cpp
// 每帧由 YogHUD 调用
void SetTrailEndpoints(FVector2D Start, FVector2D Current, float Alpha);

// 飞行完成后调用，0.25s 后自动 RemoveFromParent
void StartFadeOut();

// Details 面板赋值
UPROPERTY(EditDefaultsOnly) UMaterialInterface* TrailMaterial;
UPROPERTY(EditDefaultsOnly) float LineHeight = 8.f;
```

线段定位原理：`Position = Start`，`Size = (dist, LineHeight)`，`RenderTransformPivot = (0, 0.5)`，`Angle = atan2(dy, dx)`。

### GlassFrameWidget / WeaponGlassIconWidget

**文件**：`Source/DevKit/Public/UI/GlassFrameWidget.h`

```cpp
// 显示缩略图 + 播放出现动画
void ShowForWeapon(UTexture2D* Thumbnail, const UWeaponGlassAnimDA* DA);

// 打开背包前调用：放大→渐隐消失
void StartExpandAndHide();
```

屏幕位置由 `YogHUD::GetWeaponGlassIconScreenCenter()` 根据 DA 参数计算。

---

## 材质体系

### M_WeaponTrail（流光拖尾）

- Domain：UI
- Custom Node：`#include "/Project/WeaponTrail.ush"`，调用 `WeaponTrailMain(UV, Alpha, Progress)`
- 输入参数：`Alpha`（整体透明度）、`Progress`（流光相位）
- 输出：Final Color（RGB）+ Opacity（A）

**WeaponTrail.ush 效果层**：
1. 垂直衰减（edgeFade）：中心亮，边缘暗
2. 水平尾部淡出（tailFade）：头部 U=1 亮，尾部 U=0 暗
3. 流光脉冲（pulse）：`frac(UV.x - Progress*1.6)` 产生向前移动亮带
4. 头部白核（headCore）：最前端 38% 固定白亮区
5. 颜色：深电蓝 → 亮蓝白 → 纯白；Emissive 增益

### M_GlassFrame（液态玻璃框）

- Domain：UI
- Custom Node：`#include "/Project/GlassFrameUI.ush"`，调用 `GlassFrameMain(...)`
- 输入参数：`Time`、`CornerRadius`、`BorderWidth`、`FresnelPower`、`IridIntensity`、`IridSpeed`

**三层结构**：
1. **边缘高光线**：SDF dist≈0 处极细高光，产生玻璃边框感
2. **顶部镜面高光**：Fresnel 模拟天光
3. **底边炫彩**：atan2 驱动 Hue，随 Time 流动

---

## BP 配置清单

### BP_YogHUD（HB_PlayerMain）

| 属性 | 值 |
|------|----|
| `MainHUDClass` | WBP_HUDRoot |
| `WeaponFloatWidgetClass` | WBP_WeaponFloat（可空，C++ 会按默认路径兜底加载） |
| `ThumbnailFlyClass` | WBP_WeaponThumbnailFly |
| `TrailWidgetClass` | WBP_WeaponTrail |
| `WeaponGlassAnimDA` | DA_WeaponGlassAnim |

### WBP_WeaponTrail

1. 根：Canvas Panel，Anchors Fill，Visibility HitTestInvisible，无背景
2. 子：Image，命名 `TrailLine`，Canvas Slot Fill，Brush 材质 = MI_WeaponTrail（或 M_WeaponTrail）

### WBP_WeaponGlassIcon

- 继承 `GlassFrameWidget`
- 内含 `WeaponThumbnailImage`（BindWidget）
- 背景 Image Brush = MI_GlassFrame

---

## 剩余待完成（编辑器任务）

| 任务 | 优先级 |
|------|--------|
| WBP_WeaponFloat：将背景 Image 移入 InfoContainer（修复白屏）| P0 |
| BP_YogHUD：`TrailWidgetClass` 赋值 WBP_WeaponTrail | P0 |
| M_WeaponTrail：创建 Custom Node + 材质实例 | P1 |
| DA_WeaponGlassAnim：调整 AutoCollapseDelay 到合适时长 | P1 |

---

## ⚠️ Claude 编写注意事项

- **WeaponFloatWidget 不再作为世界 WidgetComponent 主显示**：主显示由 `AYogHUD` 创建并加入视口，位置来自武器世界点的屏幕投影。
- **武器浮窗位置由 HUD 统一夹取**：`WeaponSpawner` 只判断玩家是否看向武器，然后调用 `ShowWeaponFloatInfoAtLocation` / `HideWeaponFloatInfo`；不要在 `WeaponSpawner` 里直接改 UI 坐标。
- **浮窗冲突处理**：武器浮窗显示或淡出期间，`TickPortalPreview` 会隐藏 Portal/关卡预览浮窗，避免右侧信息重叠。
- **拾取触发时序**：玩家拾取后调用 `YogHUD::StartWeaponFloatPickup()`，折叠完成后再触发缩略图飞行。
- **WeaponTrailWidget 用 CustomPaint**：拖尾效果继承 `UWidget` 并重写 `OnPaint`，用 `FPaintGeometry` + `FSlateDrawElement::MakeLines` 绘制，不用 Canvas Panel 拼接多个 Image
- **颜色从 DA 读**：拾取光效颜色从 `UWeaponDefinition` 里的 `PickupGlowColor`（`FLinearColor`）字段读，不硬编码
