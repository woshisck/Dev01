# Camera Occlusion Fade — 配置指南

**版本**：v1.0  
**日期**：2026-05-21  
**状态**：已实现  
**适用范围**：程序、TA  
**配套文档**：[Camera_Design.md](Camera_Design.md)

---

## 1. 系统概述

`UYogCameraOcclusionFadeComponent` 挂载在玩家角色上，每隔 `TraceInterval` 秒从相机位置向角色头部发射一次球体扫描（`SweepMultiByChannel`）。命中的 Mesh 材质被替换为动态实例，通过驱动 `CameraOcclusionAlpha` 标量参数实现淡出；解除遮挡后逐渐淡入并恢复原始材质。

```
Camera ──── SweepMultiByChannel ────► Player + Offset
                │
          Hit occluder?
         Yes ↓        No ↓
   SetTargetAlpha     SetTargetAlpha
   = MinVisibleAlpha  = 1.0 (fade back)
         │
   FInterpTo per frame
         │
   SetScalarParameter("CameraOcclusionAlpha", CurrentAlpha)
```

---

## 2. 组件挂载

在玩家角色 Blueprint 的 **Components** 面板中添加 `YogCameraOcclusionFadeComponent`，无需任何代码。

> 组件 Tick Group 为 `TG_PostUpdateWork`，确保在相机位置更新后再执行扫描。

---

## 3. 属性参数表

### 3.1 检测参数

| 属性 | 默认值 | 说明 |
|---|---|---|
| `bEnableOcclusionFade` | `true` | 总开关；关闭时所有遮挡物立即恢复 |
| `TraceInterval` | `0.05` s | 扫描间隔，越小响应越快，性能消耗越高 |
| `TraceRadius` | `28.0` | 球体扫描半径（cm） |
| `PlayerTargetOffset` | `(0, 0, 80)` | 扫描终点相对角色位置的偏移，通常对齐角色头部 |
| `TraceChannel` | `ECC_Visibility` | 碰撞通道 |

### 3.2 过滤参数

| 属性 | 默认值 | 说明 |
|---|---|---|
| `bOnlyFadeTaggedOccluders` | `true` | 仅处理带 Tag 的遮挡物；关闭则对所有命中 Mesh 生效 |
| `OccluderTag` | `CameraOccluder` | 在 Actor 或 PrimitiveComponent 上添加此 Tag 才会被淡出 |

### 3.3 淡出参数

| 属性 | 默认值 | 说明 |
|---|---|---|
| `MinVisibleAlpha` | `0.15` | 最大遮挡时的透明度（0 = 完全消失，1 = 无效果） |
| `FadeOutSpeed` | `12.0` | 淡出插值速度（`FInterpTo` interp speed） |
| `FadeInSpeed` | `8.0` | 淡入插值速度 |
| `FadeScalarParameterName` | `CameraOcclusionAlpha` | 驱动的材质标量参数名，必须与材质中保持一致 |

### 3.4 材质覆盖

| 属性 | 默认值 | 说明 |
|---|---|---|
| `OcclusionFadeMaterial` | `null` | 留空时从 Mesh 原有材质创建动态实例（推荐）；指定材质时所有材质槽统一替换为该材质 |

### 3.5 Debug

| 属性 | 默认值 | 说明 |
|---|---|---|
| `bDrawDebugOcclusionTrace` | `false` | 开启后在视口绘制调试形状 |
| `DebugDrawDuration` | `0.08` s | 调试形状持续时间 |

---

## 4. 材质配置

### 4.1 为什么不用 Translucent

Translucent 材质进入半透明渲染通道，存在排序问题、无法接收阴影、性能开销较高。对于场景中的墙体、建筑等大量 Opaque Mesh，不适合批量切换。

### 4.2 推荐方案：Masked + Dithered Opacity

**Blend Mode** 改为 `Masked`，**Shading Model** 保持 `Default Lit` 不变。材质仍在 Opaque 渲染通道，通过屏幕空间抖动 + TAA 时域积累模拟透明效果。

**材质节点连线：**

```
[Scalar Parameter: CameraOcclusionAlpha]
              │
        [DitherTemporalAA]
              │  (Alpha pin)
         [Opacity Mask]
```

- `CameraOcclusionAlpha = 1.0` → 完全可见
- `CameraOcclusionAlpha = 0.0` → 完全裁剪（结合 `MinVisibleAlpha = 0` 使用）
- `CameraOcclusionAlpha = 0.15`（默认）→ 约 15% 像素可见，呈现半透感

> 需要 TAA 或 TSR 开启时效果才平滑；截图或禁用 TAA 时会看到抖动像素点，这是已知限制。

### 4.3 Master Material 接入建议

在 Master Material 中添加一个 `Scalar Parameter` 节点（命名 `CameraOcclusionAlpha`，默认值 `1.0`），接入 `DitherTemporalAA` → `Opacity Mask`。所有遮挡物的 Material Instance 无需单独配置，继承父材质即可。

---

## 5. Mesh 标记

`bOnlyFadeTaggedOccluders = true`（默认）时，只有带 `CameraOccluder` Tag 的 Mesh 才会被淡出。

**添加 Tag 的两种方式（任选其一）：**

| 方式 | 操作位置 | 说明 |
|---|---|---|
| Actor Tag | Actor Details → Tags | 该 Actor 的所有组件均生效 |
| Component Tag | PrimitiveComponent Details → Component Tags | 仅该组件生效，更精细 |

推荐对场景中可能遮挡视线的建筑、墙体 Actor 统一添加 Actor Tag `CameraOccluder`。

---

## 6. Debug 视图说明

开启 `bDrawDebugOcclusionTrace` 后各形状含义：

| 颜色 | 位置 | 含义 |
|---|---|---|
| 蓝色球体 | 相机位置 | 扫描起点 |
| 绿色球体 | 角色头部位置 | 扫描终点，本帧无有效遮挡 |
| 橙色球体 | 角色头部位置 | 扫描终点，本帧命中至少一个有效遮挡物 |
| 红色小球 | 命中点 | 通过过滤、正在被淡出的组件 |
| 银色小球 | 命中点 | 命中但被过滤忽略（无 Tag、同 Actor、已隐藏等） |

---

## 7. 常用配置示例

### 场景遮挡（推荐）

```
bOnlyFadeTaggedOccluders = true
OccluderTag              = CameraOccluder
MinVisibleAlpha          = 0.15
FadeOutSpeed             = 12.0
FadeInSpeed              = 8.0
TraceInterval            = 0.05
TraceRadius              = 28.0
```

### 完全隐藏（墙体穿透视角）

```
MinVisibleAlpha = 0.0
FadeOutSpeed    = 20.0
```

### 大范围场景（性能优先）

```
TraceInterval = 0.1   // 降低扫描频率
TraceRadius   = 20.0  // 缩小球体
```
