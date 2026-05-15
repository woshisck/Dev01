# 材质编写规范 — 星骸降临

> 版本：2026-04-21  
> 涵盖：Custom Node 体系、.ush 文件管理、各类材质的制作模板、历史踩坑

---

## 一、核心规则：所有自定义 HLSL 走 .ush 文件

**不要把 HLSL 直接写在 Custom Node 的 Code 字段。**

| 做法 | 问题 |
|---|---|
| Code 字段内联几十行 HLSL | 无法复用、材质图乱、改一处要找多个材质 |
| 把逻辑放进 .ush，Code 字段一行调用 | 文本编辑器可用、可 include 共享工具函数、版本控制友好 |

### 标准 Code 字段写法

```hlsl
return XxxMain(UV, Time, Param1, Param2, ...);
```

### Include File Paths 字段写法（Custom 节点 Details 面板）

```
/Project/XxxEffect.ush
```

`/Project/` 映射到项目 `Shaders/` 目录，UE 会在 Shader 编译时自动解析。

---

## 二、.ush 文件清单

| 文件 | 入口函数 | 用途 |
|---|---|---|
| `Shaders/GlassRim.ush` | `GR_HueToRGB` / `GR_Fresnel` / `GR_Iridescence` | 共享工具库，其他 .ush 均 include 此文件 |
| `Shaders/PlayerGlowOverlay.ush` | `PlayerGlowMain(...)` | 玩家热度升阶 Overlay：扫射带 + Fresnel 边缘光 + 炫彩 |
| `Shaders/WeaponGlowOverlay.ush` | `WeaponGlowMain(...)` | 武器热度升阶 Overlay，炫彩更集中（pow 4.0） |
| `Shaders/GlassFrameUI.ush` | `GlassFrameMain(...)` | UI 液态玻璃边框：SDF 圆角 + 顶部高光 + 底边炫彩 |
| `Shaders/GlassBlurMask.ush` | `GlassBlurMaskMain(...)` | 玻璃中心高模糊遮罩（径向渐变，供 WBP BackgroundBlur 使用） |
| `Shaders/WeaponTrail.ush` | `WeaponTrailMain(...)` | 武器拾取流光拖尾：边缘衰减 + 流光脉冲 + 头端白核 |
| `Shaders/MusketAimArc.ush` | *(详见文件内)* | 瞄准弧线 UI 材质 |

---

## 三、材质类型与设置

### 3.1 角色/武器 Overlay 材质（3D Mesh 覆盖）

| 设置项 | 值 |
|---|---|
| Material Domain | Surface |
| Blend Mode | Translucent |
| Shading Model | Unlit |
| **Is Overlay Material** | ✓（必须勾选） |
| Two Sided | ✓（可选，防穿帮） |

Custom 节点输入：`UV`(Float2), `N`(VertexNormalWS/Float3), `V`(CameraVectorWS/Float3), `Time`(Float1), 其余为 ScalarParam / VectorParam。

输出：`float4`，`RGB → Emissive Color`，`A → Opacity`。

### 3.2 UI 材质（Widget）

| 设置项 | 值 |
|---|---|
| Material Domain | **User Interface** |
| Blend Mode | Translucent |
| Shading Model | Unlit |

Custom 节点无 `N`/`V`，只有 `UV`(TexCoord[0]) 和 `Time`。

输出：`float4`，`RGB → Final Color`，`A → Opacity`。

### 3.3 命中/前摇闪光材质（CharacterFlashMaterial）

复用同一个材质 + DynMat，通过 C++ 切换颜色：

| 参数名 | 类型 | 说明 |
|---|---|---|
| `FlashColor` | Vector3 | C++ SetVectorParameter，命中=`(3,3,3)` 白，前摇=`(3,0,0)` 红 |
| `FlashAlpha` | Scalar | C++ SetScalarParameter，控制强度/淡出 |
| `Power` | Scalar | Fresnel 指数，C++ 在 `StartPreAttackFlash` 时从 `PreAttackFresnelPower` 写入 |

---

## 四、共享工具库 GlassRim.ush

```hlsl
// Hue → RGB（不依赖外部库）
float3 GR_HueToRGB(float h);

// NdotV Fresnel（3D mesh 有法线）
float GR_Fresnel(float3 Normal, float3 CameraDir, float Power);

// 世界法线 + Time 驱动的炫彩颜色
float3 GR_Iridescence(float3 WorldNormal, float Time, float IridSpeed);
```

3D Overlay 材质全部 `#include "/Project/GlassRim.ush"`，UI 材质只使用 `GR_HueToRGB`（无法线，无法用 Fresnel / Iridescence）。

---

## 五、C++ 驱动材质的标准模式

```cpp
// Header
UPROPERTY(EditDefaultsOnly, Category = "VFX")
TObjectPtr<UMaterialInterface> XxxMaterial;

UPROPERTY()
TObjectPtr<UMaterialInstanceDynamic> XxxDynMat;

float XxxElapsed = -1.f;   // -1 = 未激活

// CPP 触发
void AXxx::StartXxxEffect()
{
    if (!XxxMaterial) return;
    if (!XxxDynMat)
        XxxDynMat = UMaterialInstanceDynamic::Create(XxxMaterial, this);

    XxxDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
    XxxDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 0.f);
    GetMesh()->SetOverlayMaterial(XxxDynMat);
    XxxElapsed = 0.f;
}

// CPP Tick
void AXxx::TickXxxEffect(float DeltaTime)
{
    if (XxxElapsed < 0.f) return;
    XxxElapsed += DeltaTime;
    float Alpha = FMath::Max(0.f, 1.f - XxxElapsed / XxxDuration);
    XxxDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), Alpha);
    if (XxxElapsed >= XxxDuration)
    {
        GetMesh()->SetOverlayMaterial(nullptr);
        XxxElapsed = -1.f;
    }
}
```

---

## 六、颜色统一管理：BackpackStyleDataAsset

所有热度相关颜色从 `DA_BackpackStyle` 读取，**不在材质或 C++ 里硬编码**。

| DA 字段 | 用途 |
|---|---|
| `HeatZone0Color` | HeatBar 进度条：Phase0→1 过渡色（淡蓝） |
| `HeatZone1Color` | HeatBar 进度条：Phase1→2 过渡色（暖橙） |
| `HeatZone2Color` | HeatBar 进度条：Phase2→3 过渡色（金色） |
| `Phase1GlowColor` | 玩家/武器 Overlay 热度 Phase1 颜色（HDR，可 >1） |
| `Phase2GlowColor` | 玩家/武器 Overlay 热度 Phase2 颜色 |
| `Phase3GlowColor` | 玩家/武器 Overlay 热度 Phase3 颜色 |

读取路径：
- 玩家 `PlayerCharacterBase::HeatStyleDA`（需在 `B_PlayerOne` Details → Heat|Visual 填入 `DA_BackpackStyle`）
- 武器通过 `Cast<APlayerCharacterBase>(GetOwner())->HeatStyleDA` 读取
- HeatBar 通过 `Cast<APlayerCharacterBase>(CachedBackpack->GetOwner())->HeatStyleDA` 读取

**注意**：读颜色后强制 `Alpha = 1.f`，避免 DA 里 Alpha<1 导致颜色显示脏。

---

## 七、UI 双层模糊方案（WBP_GlassFrame）

UE5 `BackgroundBlur` 只支持均匀模糊。要实现"中心高模糊、边缘低模糊"：

```
[GlassBG]        BackgroundBlur，低强度（6–14），全区域，Apply Alpha to Blur = OFF
[GlassBGCenter]  BackgroundBlur，高强度（20–40），Apply Alpha to Blur = ON
                  → Brush 用 M_GlassBlurMask 材质（径向渐变，中心白=全模糊，边缘透明=不模糊）
```

`M_GlassBlurMask` 制作：Custom Node，Code 字段：
```hlsl
return GlassBlurMaskMain(UV, Radius, Softness);
```
Include：`/Project/GlassBlurMask.ush`，Domain=User Interface，Blend=Translucent。

C++ 参数：`BlurStrength`（边缘层）、`CenterBlurStrength`（中心层），均在 `NativeConstruct` 和 `ApplyGlassStyle` 时写入。

---

## 八、历史踩坑与反馈

| 问题 | 根因 | 解决 |
|---|---|---|
| Custom Node Code 字段报编译错误 | HLSL 逻辑内联且长，不好 debug | 全部迁移到 .ush，Code 字段仅一行调用 |
| UI 材质里用了 `N`/`V`（法线/视角） | User Interface domain 没有顶点法线 | UI 材质不能用 GR_Fresnel / GR_Iridescence，改用 UV 做方向 |
| GlassFrame 边框"有点丑"，颜色太重 | 旧版本有高 alpha 的实心边框填充 | 改为 Apple 液态玻璃风格：极细 SDF 边缘线 + 顶部镜面高光 + 底边炫彩，内部透明 |
| 前摇红光"Fresnel 太强，不清晰" | `Power` 参数没从 C++ 写入 DynMat，材质用了固定高值 | 在 `StartPreAttackFlash` 里写 `Power = PreAttackFresnelPower`（默认 1.0，越小越整体发红） |
| HeatBar 颜色修改 DA 后没变 | `GetOwningPlayerPawn()` 在 HUD 子控件中返回 null | 改用 `Cast<APlayerCharacterBase>(CachedBackpack->GetOwner())` 反查 |
| HeatBar 颜色"脏" | DA 里颜色 Alpha < 1，SetFillColorAndOpacity 保留了 alpha | 读 DA 颜色后强制 `C.A = 1.f` |
| Overlay Material 被 GAS 授能覆盖 | BeginPlay 中 GAS 授能可能 LinkAnimLayer 重置 Overlay | 在 `BeginPlay` 末尾调 `RelinkWeaponAnimLayer()` 重新 Link |
| `#include "/Project/XxxFile.ush"` 找不到文件 | 文件放在了 `Content/Shaders/` 而非 `Shaders/` 根目录 | .ush 必须放在项目根目录 `Shaders/` 下才能用 `/Project/` 路径 |

---

## 九、Overlay 优先级规则

同一角色同时触发多个 Overlay 时（`SetOverlayMaterial` 只能有一个生效）：

1. **命中闪白 > 前摇红光**：闪白 Tick 期间跳过红光更新，闪白结束后若红光仍激活无缝衔接
2. **热度发光 + 命中闪白**：热度在玩家身上，命中闪白在敌人身上，不冲突
3. **重叠触发**：重置 `Elapsed = 0`，重新播，不叠加

---

## 十、调参参考值

| 参数 | 常用范围 | 备注 |
|---|---|---|
| Fresnel Power（3D Overlay） | 2.0–5.0 | 越大越集中在边缘，<2 整体泛光 |
| 前摇 PreAttackFresnelPower | 0.5–2.0 | 默认 1.0，越小越清晰 |
| HDR Emissive 亮度 | 2–6 | 低于 1.5 看不见，高于 8 过曝 |
| 闪白时长 HitFlashDuration | 0.10–0.15s | 太长像卡顿 |
| 前摇脉冲频率 PreAttackPulseFreq | 3–5 Hz | 太快眼花，太慢没紧迫感 |
| 扫射时长 GlowSweepDuration | 0.3–0.6s | 配合攻击节奏 |
| 边缘保持 GlowHoldDuration | 2.0–4.0s | 热度保持阶段可见时间 |
| 炫彩流速 IridSpeed | 0.02–0.06 | 太快像闪烁 disco，太慢看不出流动 |
| BackgroundBlur 强度 | 6–14（边缘）/ 20–40（中心） | 移动端慎用，性能消耗大 |
