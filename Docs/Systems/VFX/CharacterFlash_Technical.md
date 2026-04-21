# 角色闪光特效系统 — 技术文档

> 更新：2026-04-18  
> 涵盖：命中闪白 / 攻击前闪红 / 热度升阶发光

---

## 一、系统概览

所有角色身体特效均通过 `UMeshComponent::SetOverlayMaterial` 实现——叠加在原始材质之上，不破坏原始着色。  
动画由 C++ Tick 驱动，无需蓝图 Timeline。

| 特效 | 作用对象 | 触发方式 | 持续模式 |
|---|---|---|---|
| 命中闪白 | 敌人 / 任意角色 | 血量减少时自动触发 | 线性淡出（0.12s） |
| 攻击前闪红 | 敌人 | 蓝图调用 `StartPreAttackFlash()` | 正弦脉冲，持续到 `StopPreAttackFlash()` |
| 热度升阶发光 | 玩家 | GAS Tag `Buff.Status.Heat.Phase.*` | 扫射(0.5s) + 保持(3s) + 淡出(0.5s) |

---

## 二、命中闪白 / 攻击前闪红

### 核心文件

| 文件 | 说明 |
|---|---|
| `YogCharacterBase.h/.cpp` | 特效逻辑，所有角色共用 |

### C++ 接口

```cpp
// 由 HealthChanged 自动调用（血量减少且存活时）
void StartHitFlash();

// 攻击前摇开始时调用（AnimNotify 或 GA 中）
UFUNCTION(BlueprintCallable)
void StartPreAttackFlash();

// 攻击出手 / 取消时调用
UFUNCTION(BlueprintCallable)
void StopPreAttackFlash();
```

### 可配置属性（蓝图 Details → Combat|Visual）

| 属性 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `CharacterFlashMaterial` | UMaterialInterface | — | 角色闪光 Overlay 材质 |
| `HitFlashDuration` | float | 0.12 | 命中闪白淡出时长（秒） |
| `PreAttackPulseFreq` | float | 4.0 | 攻击前红光脉冲频率（次/秒） |

### 优先级规则

命中闪白期间跳过红光更新（白光覆盖红光）；闪白结束后若红光仍激活则无缝衔接。

---

## 三、热度升阶发光

### 核心文件

| 文件 | 说明 |
|---|---|
| `PlayerCharacterBase.h/.cpp` | 玩家身体发光逻辑 |
| `WeaponInstance.h/.cpp` | 武器发光逻辑（独立 Tick） |

### 触发流程

```
GAS Tag Buff.Status.Heat.Phase.N 新增
  → OnHeatPhaseTagChanged()
    → OnHeatPhaseChanged.Broadcast(N)   ← 武器订阅此委托
    → StartPlayerPhaseGlow(N)           ← 玩家身体发光
    → ClientPlayForceFeedback()         ← 手柄震动
```

### 可配置属性（蓝图 Details → Heat|Visual）

| 属性 | 默认值 | 说明 |
|---|---|---|
| `PhaseUpPlayerOverlayMaterial` | — | 玩家身体 Overlay 材质 |
| `GlowSweepDuration` | 0.5s | 扫射时长 |
| `GlowHoldDuration` | 3.0s | 边缘光保持时长 |
| `GlowFadeDuration` | 0.5s | 淡出时长 |

武器侧同名属性在 `WeaponInstance` 蓝图 Details 中配置，独立控制。

---

## 四、Overlay 材质规范

### 通用材质设置

| 设置项 | 值 |
|---|---|
| Blend Mode | Translucent |
| Shading Model | Unlit |
| Is Overlay Material | ✓ 勾选 |

### 命中闪白 / 攻击前闪红材质（M_CharacterFlash）

Custom 节点 Inputs：`N`(VertexNormalWS)、`V`(CameraVectorWS)、`Power`(Scalar,默认5)、`FlashColor`(Vector)、`FlashAlpha`(Scalar)

```hlsl
float NdotV = saturate(dot(normalize(N), normalize(V)));
float Rim   = pow(1.0 - NdotV, Power);
float3 Color = FlashColor * Rim * FlashAlpha;
return float4(Color, Rim * FlashAlpha);
```

输出：`.rgb → Emissive Color`，`.a → Opacity`

### 热度升阶发光材质（M_HeatPhaseOverlay）

Custom 节点 Inputs：`N`、`V`、`Power`、`UV`(TexCoordinate)、`SweepProgress`(Scalar)、`GlowAlpha`(Scalar)、`EmissiveColor`(Vector)、`SwipeCount`(Scalar,默认1)

```hlsl
// Fresnel 边缘光
float NdotV   = saturate(dot(normalize(N), normalize(V)));
float Fresnel = pow(1.0 - NdotV, Power);

// 扫射带（支持多次重复）
float RepeatedProgress = frac(SweepProgress * SwipeCount);
float BandPos   = 1.0 - RepeatedProgress;
float BandWidth = 0.15;
float SweepBand = pow(saturate(1.0 - abs(UV.y - BandPos) / BandWidth), 2.0);

// 合并
float  Glow  = saturate(SweepBand * 5.0 + Fresnel);
float3 Color = EmissiveColor * Glow * GlowAlpha;
float  Alpha = saturate(Glow * GlowAlpha);
return float4(Color, Alpha);
```

---

## 五、蓝图配置清单

### 敌人蓝图

- [ ] Details → Combat|Visual → `CharacterFlashMaterial` 填入 `M_CharacterFlash`
- [ ] 攻击前摇 AnimNotify → 调用 `StartPreAttackFlash()`
- [ ] 攻击出手/取消 AnimNotify → 调用 `StopPreAttackFlash()`

### 玩家蓝图（BP_PlayerCharacterBase）

- [ ] Details → Heat|Visual → `PhaseUpPlayerOverlayMaterial` 填入 `M_HeatPhaseOverlay`
- [ ] Details → Heat|Feedback → `PhaseUpForceFeedback` 填入震动资产

### 武器蓝图（WeaponInstance 子类）

- [ ] `WeaponDefinition` DA → `HeatOverlayMaterial` 填入 `M_HeatPhaseOverlay`（由 Spawner 自动传入）
