# 角色特效制作规范

> 更新：2026-04-18  
> 本文档记录项目中制作角色 Overlay 特效的固定方案和习惯，新特效按此模板扩展。

---

## 一、核心方案：Overlay Material + C++ Tick

本项目所有角色身体特效**统一使用 Overlay Material**，不修改角色原有材质。

```
触发事件（GAS Tag / 血量变化 / 蓝图调用）
  → C++ 方法设置 DynMat 参数 + SetOverlayMaterial()
  → Tick 每帧更新参数（SweepProgress / Alpha 等）
  → 动画结束 → SetOverlayMaterial(nullptr) 清除
```

**为什么不用 Blueprint Timeline？**  
Timeline 需要蓝图配合，改参数需打开每个 BP。C++ Tick 驱动统一管理，参数在 `EditDefaultsOnly` 里暴露，策划在 BP Details 面板调整即可。

---

## 二、新建特效标准流程

### Step 1 — 确定归属类

| 特效类型 | 加在哪个类 |
|---|---|
| 所有角色通用（受击/攻击警示） | `YogCharacterBase` |
| 仅玩家（热度/冲刺） | `PlayerCharacterBase` |
| 仅武器 | `WeaponInstance` |

### Step 2 — Header 添加属性和方法

```cpp
// 材质资产（EditDefaultsOnly，蓝图 Details 填入）
UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual")
TObjectPtr<UMaterialInterface> XxxMaterial;

// 可调参数
UPROPERTY(EditDefaultsOnly, Category = "Combat|Visual")
float XxxDuration = 0.3f;

// 触发接口（BlueprintCallable 供蓝图调用）
UFUNCTION(BlueprintCallable, Category = "Combat|Visual")
void StartXxxEffect();

// 私有状态
UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> XxxDynMat;
float XxxElapsed = -1.f;   // -1 = 未激活
```

### Step 3 — CPP 实现

**触发方法**：
```cpp
void AXxx::StartXxxEffect()
{
    if (!XxxMaterial) return;
    if (!XxxDynMat)
        XxxDynMat = UMaterialInstanceDynamic::Create(XxxMaterial, this);

    XxxDynMat->SetVectorParameterValue(TEXT("FlashColor"), FLinearColor(...));
    XxxDynMat->SetScalarParameterValue(TEXT("FlashAlpha"), 1.f);
    GetMesh()->SetOverlayMaterial(XxxDynMat);
    XxxElapsed = 0.f;
}
```

**Tick 驱动**（在 `Tick()` 或 `TickXxx()` 里）：
```cpp
if (XxxElapsed < 0.f) return;
XxxElapsed += DeltaTime;

// 计算 Alpha / 其他参数
float Alpha = FMath::Max(0.f, 1.f - XxxElapsed / XxxDuration);
XxxDynMat->SetScalarParameterValue(TEXT("FlashAlpha"), Alpha);

if (XxxElapsed >= XxxDuration)
{
    GetMesh()->SetOverlayMaterial(nullptr);
    XxxElapsed = -1.f;
}
```

### Step 4 — 材质 Custom 节点（固定模板）

所有 Overlay 材质共用此结构：

```hlsl
// Fresnel 边缘光（法线驱动，准确）
float NdotV = saturate(dot(normalize(N), normalize(V)));
float Rim   = pow(1.0 - NdotV, Power);

// 特效专属逻辑（插入此处）
// ...

float3 Color = FlashColor * Rim * FlashAlpha;
return float4(Color, Rim * FlashAlpha);
```

**材质设置固定值**：Blend Mode=Translucent，Shading Model=Unlit，Is Overlay Material=✓

### Step 5 — 阶段颜色约定（HDR 线性空间）

| 效果 | 颜色值 |
|---|---|
| 命中闪白 | `(3, 3, 3)` |
| 攻击前红 | `(3, 0, 0)` |
| 热度 Phase1 白光 | `(2, 2, 2)` |
| 热度 Phase2 绿光 | `(0, 3, 0)` |
| 热度 Phase3 橙黄 | `(4, 2, 0)` |
| 热度 Phase4 红光 | `(5, 0, 0)` |

### Step 6 — FeatureLog 更新

完成后在 `Docs/FeatureLog.md` 新增条目，格式参考 `[VFX-XXX]`。

---

## 三、优先级规则（多特效共存）

同一角色同时触发多个特效时：

1. **命中闪白 > 攻击前闪红**：闪白期间跳过红光 Tick，闪白结束后若红光仍激活无缝衔接
2. **热度发光独立于命中闪白**：热度发光在玩家身上，命中闪白在敌人身上，互不干扰
3. **同类特效重叠**：重置 `Elapsed = 0` 重新播放（不叠加，覆盖）

---

## 四、调参习惯

- **Fresnel Power**：5~8 中等边缘，12+ 细边缘，在材质实例里实时调
- **HDR 亮度**：Emissive 乘数 2~5，太低看不见，太高过曝失真
- **闪白时长**：0.1~0.15s，太长像卡顿，太短感知不到
- **脉冲频率**：3~5 Hz，太快眼花，太慢没紧迫感
- **扫射时长**：0.2~0.5s，配合角色攻击节奏
