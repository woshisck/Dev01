# 材质规范

> Claude 在编写任何材质相关代码（.ush / C++ 参数设置 / 材质类型判断）前必须阅读。  
> 详细参数参考值见 [Material_Authoring_Guide.md](../系统/VFX/Material_Authoring_Guide.md)

---

## 核心规则

1. **所有 HLSL 写在 .ush 文件**，Custom Node Code 字段仅一行调用：`return XxxMain(...);`
2. **颜色不在材质或 C++ 硬编码**，从 `DA_BackpackStyle`（BackpackStyleDataAsset）读取
3. **UI 材质**（User Interface domain）不能用法线相关函数（GR_Fresnel / GR_Iridescence），只用 UV 方向
4. **.ush 文件放在项目根 `Shaders/` 目录**，路径写法 `/Project/XxxFile.ush`

---

## 分工

### Claude 写：
- `.ush` 文件（HLSL 函数体）
- C++ 驱动材质的触发 / Tick 代码（SetScalarParameterValue / SetVectorParameterValue）
- 材质类型设置说明（Domain / Blend Mode / Is Overlay Material）
- 参数命名约定

### 用户在引擎里做：
- 创建 Material Asset
- 添加 Custom Node，填写 Include File Path 和单行 Code
- 连接 TexCoord[0]、Time、ScalarParam、VectorParam 到 Custom Node 输入
- 勾选 Is Overlay Material（3D Overlay 材质）
- 将材质赋给角色/武器 BP 的 `CharacterFlashMaterial` / `HeatGlowMaterial` 等 UPROPERTY

---

## 材质类型速查

| 类型 | Domain | Blend | Shading | Is Overlay |
|---|---|---|---|---|
| 角色/武器 Overlay（热度/闪光） | Surface | Translucent | Unlit | ✓ |
| UI 液态玻璃 / 弧线 | User Interface | Translucent | Unlit | — |
| CharacterFlashMaterial（命中/前摇） | Surface | Translucent | Unlit | ✓ |

---

## .ush 文件清单

| 文件 | 入口函数 | 用途 |
|---|---|---|
| `Shaders/GlassRim.ush` | `GR_HueToRGB` / `GR_Fresnel` / `GR_Iridescence` | 共享工具库，其他 .ush 均 include |
| `Shaders/PlayerGlowOverlay.ush` | `PlayerGlowMain(...)` | 玩家热度升阶 Overlay |
| `Shaders/WeaponGlowOverlay.ush` | `WeaponGlowMain(...)` | 武器热度升阶 Overlay |
| `Shaders/GlassFrameUI.ush` | `GlassFrameMain(...)` | UI 液态玻璃边框 |
| `Shaders/GlassBlurMask.ush` | `GlassBlurMaskMain(...)` | 玻璃中心高模糊遮罩 |
| `Shaders/WeaponTrail.ush` | `WeaponTrailMain(...)` | 武器拾取流光拖尾 |
| `Shaders/MusketAimArc.ush` | *(内部函数)* | 瞄准弧线 UI 材质 |
| `Shaders/LiquidHealthBar.ush` | `LiquidHealthBarMain(...)` | 液态血条：横向填充 + 粘稠晃动 + 圆柱体积感 |
| `Shaders/GlassChromaDistort.ush` | `GlassChromaMain(...)` | RetainerBox 色差层：边缘对背景做 RGB 通道径向偏移 |

---

## C++ 驱动材质标准模板

```cpp
// Header
UPROPERTY(EditDefaultsOnly, Category = "VFX")
TObjectPtr<UMaterialInterface> XxxMaterial;

UPROPERTY()
TObjectPtr<UMaterialInstanceDynamic> XxxDynMat;

float XxxElapsed = -1.f;  // -1 = 未激活

// 触发
void AXxx::StartXxxEffect()
{
    if (!XxxMaterial) return;
    if (!XxxDynMat)
        XxxDynMat = UMaterialInstanceDynamic::Create(XxxMaterial, this);
    XxxDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
    GetMesh()->SetOverlayMaterial(XxxDynMat);
    XxxElapsed = 0.f;
}

// Tick
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

## CharacterFlashMaterial 参数约定

| 参数名 | 类型 | 命中闪白 | 前摇闪红 |
|---|---|---|---|
| `FlashColor` | Vector3 | `(3,3,3)` 白 | `(3,0,0)` 红 |
| `FlashAlpha` | Scalar | 1→0 线性淡出 | sin 脉冲 |
| `Power` | Scalar | 固定 | 从 `PreAttackFresnelPower` 写入 |

---

## Overlay 优先级规则

- 命中闪白 > 前摇红光（闪白 Tick 期间跳过红光更新）
- 热度发光（玩家）与命中闪白（敌人）不冲突，不同角色
- 重叠触发：重置 `Elapsed = 0`，重新播，不叠加

---

## 常见踩坑

| 问题 | 解决 |
|---|---|
| UI 材质里用 N/V 报错 | UI Domain 无法线，改用 UV 做方向 |
| `/Project/Xxx.ush` 找不到 | .ush 必须在项目根 `Shaders/` 目录，不是 `Content/Shaders/` |
| Overlay 被 GAS 授能重置 | BeginPlay 末尾调 `RelinkWeaponAnimLayer()` |
| HeatBar 颜色脏 | DA 读取后强制 `Color.A = 1.f` |
