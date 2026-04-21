# 液态玻璃框 — 技术文档

> 版本：2026-04-19  
> 适用范围：背包主界面、武器图案框、玩家 HUD 背包缩略框  
> 设计意向：Apple 液态玻璃质感——中心毛玻璃模糊、边框折射半透明、角落菲涅尔极强处点缀神秘炫彩

---

## 一、效果层次

```text
[BackgroundBlur]       ← 实时模糊游戏场景（毛玻璃底）
[GlassBorderImage]     ← 玻璃边框：SDF + 菲涅尔 + 炫彩（自定义材质）
[Content NamedSlot]    ← 背包格子 / 武器图标 / HUD 缩略图
```

- **中心**：BackgroundBlur 采样并模糊其后方的游戏帧画面（活的，不是静帧）
- **边框**：材质用 SDF 计算圆角方形轮廓，菲涅尔向角落加强，炫彩仅在菲涅尔峰值区出现
- **不暂停**：背包打开时游戏不暂停，BackgroundBlur 后方场景实时变化

---

## 二、C++ 基类

**文件**：`Source/DevKit/Public/UI/GlassFrameWidget.h` / `.cpp`

### 关键属性

| 属性 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `GlassBorderMaterial` | UMaterialInterface* | — | 必填，玻璃边框材质 |
| `BlurStrength` | float | 14.0 | BackgroundBlur 强度 |
| `CornerRadius` | float | 0.06 | UV 空间圆角半径 |
| `BorderWidth` | float | 0.025 | UV 空间边框宽度 |
| `FresnelPower` | float | 2.5 | 菲涅尔集中程度（大→更集中在角落） |
| `IridIntensity` | float | 0.18 | 炫彩强度 |
| `IridSpeed` | float | 0.04 | 炫彩流速 |

### 关键接口

| 函数 | 说明 |
|---|---|
| `NativeConstruct()` | 自动创建 DynMat，绑定到 GlassBorderImage，调用 ApplyGlassStyle |
| `ApplyGlassStyle()` | 将所有属性写入 DynMat + 更新 BlurStrength。运行时修改参数后调用 |
| `GetGlassDynMat()` | 返回 DynMat 供 Blueprint 做额外参数扩展 |

### BindWidget 命名规则

| 控件类型 | 名称（精确） | 说明 |
|---|---|---|
| BackgroundBlur | `GlassBG` | BindWidgetOptional，缺失不崩溃 |
| Image | `GlassBorderImage` | BindWidgetOptional |
| NamedSlot | `Content` | 放实际内容，命名自由（Blueprint 里 expose） |

---

## 三、材质制作（M_GlassFrame）

### 3.1 基本设置

| 项 | 值 |
|---|---|
| Material Domain | User Interface |
| Blend Mode | Translucent |
| Shading Model | Unlit |

### 3.2 Custom 节点输入列表

| Name | Type | 连接 |
|---|---|---|
| UV | Float2 | TexCoord[0] |
| Time | Float1 | Time 节点 |
| CornerRadius | Float1 | Scalar Parameter（默认 0.06） |
| BorderWidth | Float1 | Scalar Parameter（默认 0.025） |
| FresnelPower | Float1 | Scalar Parameter（默认 2.5） |
| IridIntensity | Float1 | Scalar Parameter（默认 0.18） |
| IridSpeed | Float1 | Scalar Parameter（默认 0.04） |

### 3.3 Custom 节点配置

与玩家/武器 Overlay 材质相同的 .ush include 体系：

**Include File Paths**（Custom 节点 Details 面板）：

```text
/Project/GlassFrameUI.ush
```

**Code 字段（只需一行）**：

```hlsl
return GlassFrameMain(UV, Time, CornerRadius, BorderWidth, FresnelPower, IridIntensity, IridSpeed);
```

所有 HLSL 逻辑在 `Shaders/GlassFrameUI.ush`，该文件 include 了 `GlassRim.ush` 复用 `GR_HueToRGB`。

### 3.4 节点连接

```text
TexCoord[0]     → Custom.UV
Time(node)      → Custom.Time
各 ScalarParam  → Custom 对应 Input（参数名与 C++ SetScalarParameterValue 的 Key 必须完全一致）
Custom.RGBA     → Break Float4 → RGB→EmissiveColor, A→Opacity
```

---

## 四、Blueprint 搭建步骤（WBP_GlassFrame）

1. **Content Browser → New Blueprint → 父类选 `GlassFrameWidget`**
2. **Designer 搭建层级**：
   ```
   [Root] Overlay（Fill 填满）
   ├── BackgroundBlur   命名 "GlassBG"         Size Fill，Apply Alpha to Blur ✓，Brush Alpha 0.55
   ├── Image            命名 "GlassBorderImage"  Size Fill，Material 留空（C++ 运行时创建 DynMat）
   └── NamedSlot        命名 "Content"           放具体内容
   ```
3. **Details → 玻璃框|材质 → GlassBorderMaterial** 填入 `M_GlassFrame`
4. 根据使用场景在 Details 调整参数（见下方场景推荐值）

---

## 五、各场景推荐参数

| 场景 | BlurStrength | CornerRadius | IridIntensity | 备注 |
|---|---|---|---|---|
| 背包主界面 | 14 | 0.06 | 0.18 | 标准值 |
| 武器图案框 | 10 | 0.08 | 0.12 | 稍小、圆一点 |
| HUD 背包缩略框 | 6 | 0.10 | 0.08 | 小尺寸，炫彩要弱，否则抢眼 |
| 三选一符文奖励框 | 16 | 0.05 | 0.22 | 大框，炫彩可稍强 |

---

## 六、运行时动态修改

```cpp
// 找到 Widget 实例后
if (UGlassFrameWidget* Frame = Cast<UGlassFrameWidget>(MyWidget))
{
    Frame->IridIntensity = 0.35f;  // 升阶时炫彩加强
    Frame->ApplyGlassStyle();
}
```

Blueprint 同理：调用 `ApplyGlassStyle` 节点即可。

---

## 七、常见问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 边框不显示 | GlassBorderMaterial 未填 | Details → 玻璃框|材质 填入 M_GlassFrame |
| 背景没有模糊 | BackgroundBlur 命名错误 | 确认控件名称精确为 `GlassBG` |
| 炫彩一直闪烁 | IridSpeed 太大 | 降低到 0.02–0.05 |
| 移动端模糊不显示 | BackgroundBlur 移动端默认禁用 | 项目设置开启或改用 PostProcess 方案 |
| 参数修改无效 | 没调 ApplyGlassStyle | 修改属性后必须手动调用 ApplyGlassStyle |

---

## 八、相关文件速查

| 文件 | 说明 |
|---|---|
| `Source/DevKit/Public/UI/GlassFrameWidget.h` | C++ 基类头文件 |
| `Source/DevKit/Private/UI/GlassFrameWidget.cpp` | C++ 实现 |
| `Content/UI/Materials/M_GlassFrame`（待创建） | 玻璃边框材质 |
| `Content/UI/Widgets/WBP_GlassFrame`（待创建） | Blueprint Widget |
