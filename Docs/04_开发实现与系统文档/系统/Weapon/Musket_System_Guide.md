# 火绳枪武器系统 — 完整实现指南

> 版本：v4.0 | 日期：2026-04-21 | 状态：C++ 全部已编译，待 Editor 配置

---

## 目录

1. [系统概览](#1-系统概览)
2. [C++ 文件索引](#2-c-文件索引)
3. [Gameplay Tags 速查](#3-gameplay-tags-速查)
4. [GA 激活逻辑表](#4-ga-激活逻辑表)
5. [M_AimArc 材质制作](#5-m_aimarc-材质制作)
6. [Editor 配置步骤](#6-editor-配置步骤)
7. [无动画资产时的测试方式](#7-无动画资产时的测试方式)
8. [数值参数参考](#8-数值参数参考)
9. [常见问题与排查](#9-常见问题与排查)

---

## 1. 系统概览

火绳枪是第一件远程武器，所有游戏逻辑在 **C++** 实现，Blueprint 子类只做资产赋值和数值调整，不写任何蓝图逻辑。

### 完整功能清单

| 功能 | 状态 | 负责 GA |
|------|------|---------|
| 速射（LMB 一按即发） | C++ 完成 | `GA_Musket_LightAttack` |
| 蓄力重击（RMB 按住蓄力，松手开枪） | C++ 完成 | `GA_Musket_HeavyAttack` |
| 单发换弹（循环上弹，可中断） | C++ 完成 | `GA_Musket_Reload_Single` |
| 全弹换弹（一次换满） | C++ 完成 | `GA_Musket_Reload_All` |
| 冲刺攻击（冲刺中按 LMB，全弹扇射） | C++ 完成 | `GA_Musket_SprintAttack` |
| 冲刺换弹（冲刺中按 R，瞬换不打断冲刺） | C++ 完成 | `GA_Musket_SprintReload` |
| 弹药 HUD（金色/灰色图标列） | C++ 完成 | `UWBP_AmmoCounter` |
| 地面瞄准弧（Deferred Decal） | C++ 完成 | `AYogAimArcActor` |
| 子弹 Actor + 伤害 GE | C++ 完成 | `AMusketBullet` / `UGE_MusketBullet_Damage` |

### 核心架构原则

- **无 Blueprint 逻辑**：所有 GA 的 C++ 构造函数已设好默认值（`BulletClass`、`BulletDamageEffectClass`），不需要 Blueprint 子类即可运行
- **无动画也能测试**：所有 GA 在 `FireMontage == nullptr` 时跳过蒙太奇，直接完成逻辑
- **输入走 `TryActivateAbilitiesByTag`**：Controller 按 Tag 激活 GA，不依赖 InputID 绑定

---

## 2. C++ 文件索引

### 2.1 核心类

| 类 | 路径 | 职责 |
|----|------|------|
| `UGA_MusketBase` | `Source/DevKit/Public/AbilitySystem/Abilities/Musket/GA_MusketBase.h` | 基类，弹药/移动/子弹/Cue 工具方法 |
| `UGA_Musket_LightAttack` | `…/GA_Musket_LightAttack.h` | 速射 GA |
| `UGA_Musket_HeavyAttack` | `…/GA_Musket_HeavyAttack.h` | 蓄力 GA（含 AimArc 管理） |
| `UGA_Musket_Reload_Single` | `…/GA_Musket_Reload_Single.h` | 单发循环换弹 GA |
| `UGA_Musket_Reload_All` | `…/GA_Musket_Reload_All.h` | 全弹换弹 GA |
| `UGA_Musket_SprintAttack` | `…/GA_Musket_SprintAttack.h` | 冲刺攻击 GA（扇射全弹） |
| `UGA_Musket_SprintReload` | `…/GA_Musket_SprintReload.h` | 冲刺换弹 GA（瞬换） |
| `UAbilityTask_MusketCharge` | `Source/DevKit/Public/AbilitySystem/AbilityTask/AbilityTask_MusketCharge.h` | 蓄力帧更新 Task（bTickingTask=true） |
| `AYogAimArcActor` | `Source/DevKit/Public/Item/Weapon/AimArcActor.h` | 地面弧形 Decal 指示器 |
| `AMusketBullet` | `Source/DevKit/Public/Projectile/MusketBullet.h` | 子弹 Actor |
| `UGE_MusketBullet_Damage` | `Source/DevKit/Public/AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h` | 子弹伤害 GE（Instant，走 Execution） |
| `UMusketBulletDamageExecution` | `Source/DevKit/Public/AbilitySystem/GameplayEffect/MusketBulletDamageExecution.h` | 伤害 Execution（读 SetByCaller → 写 DmgTaken） |
| `UAmmoCounter` | `Source/DevKit/Public/UI/AmmoCounter.h` | 弹药计数 HUD Widget（纯 C++ 驱动） |

### 2.2 Shader 资源

| 文件 | 路径 | 说明 |
|------|------|------|
| `MusketAimArc.ush` | `Shaders/MusketAimArc.ush` | 扇形遮罩 HLSL 函数，供 Custom 节点 `#include` |

### 2.3 GA_MusketBase 提供的工具方法（子类可直接调用）

```cpp
// 弹药
float GetCurrentAmmo() const;
float GetMaxAmmo()     const;
bool  HasAmmo()        const;
void  ConsumeOneAmmo();        // 扣 1，钳制到 [0, MaxAmmo]
void  AddOneAmmo();            // 加 1，钳制到 MaxAmmo
void  ClearAllAmmo();          // 清空
void  SetAmmoToMax();          // 补满

// 移动
void LockMovement();           // 禁止移动
void UnlockMovement();         // 恢复移动

// 子弹生成
AMusketBullet* SpawnBullet(float YawOffsetDeg, float Damage);

// Cue
void ExecuteFireCue();         // GameplayCue.Musket.Fire
void ExecuteReloadCue();       // GameplayCue.Musket.Reload
void ExecuteChargeFullCue();   // GameplayCue.Musket.ChargeFull

// 伤害基值
float GetBaseAttack() const;   // BaseAttributeSet.Attack
```

---

## 3. Gameplay Tags 速查

所有 Tag 均已写入 `Config/DefaultGameplayTags.ini`，无需手动添加。

### 3.1 能力标识 Tag（AbilityTags）

| Tag | 用于哪个 GA |
|-----|------------|
| `Ability.Musket.Light` | `GA_Musket_LightAttack` |
| `Ability.Musket.Heavy` | `GA_Musket_HeavyAttack` |
| `Ability.Musket.Reload` | `GA_Musket_Reload_Single` / `GA_Musket_Reload_All` |
| `Ability.Musket.SprintAtk` | `GA_Musket_SprintAttack` |
| `Ability.Musket.SprintReload` | `GA_Musket_SprintReload` |

### 3.2 输入激活 Tag（Controller 发出，GA AbilityTags 里接收）

| Tag | 触发方式 | 激活哪个 GA |
|-----|---------|------------|
| `PlayerState.AbilityCast.LightAtk` | LMB 按下（Triggered） | `LightAttack`（无 DashInvincible）/ `SprintAttack`（有 DashInvincible） |
| `PlayerState.AbilityCast.HeavyAtk` | RMB 按下（Triggered） | `HeavyAttack` |
| `PlayerState.AbilityCast.Reload` | R 键按下（Triggered） | `Reload_Single` / `Reload_All`（无 DashInvincible）/ `SprintReload`（有 DashInvincible） |
| `PlayerState.AbilityCast.Dash` | Dash 键（已有） | 冲刺 GA |

### 3.3 状态 Tag（GA 持有期间自动添加）

| Tag | 添加时机 | 效果 |
|-----|---------|------|
| `State.Musket.Aiming` | `HeavyAttack` 激活期间 | 阻断 `LightAttack` / 两个换弹 GA 的激活 |
| `State.Musket.Reloading` | `Reload_Single` / `Reload_All` 激活期间 | 阻断所有攻击类 GA |
| `Buff.Status.DashInvincible` | 冲刺 GA 激活期间（已有） | 让 `SprintAttack` / `SprintReload` 能激活 |

### 3.4 事件 Tag（GameplayEvent）

| Tag | 发出方 | 接收方 |
|-----|--------|--------|
| `GameplayEvent.Musket.HeavyRelease` | Controller `HeavyAttackReleased()` | `GA_Musket_HeavyAttack` 的 `WaitGameplayEvent` Task |

### 3.5 GameplayCue Tag

| Tag | 触发时机 |
|-----|---------|
| `GameplayCue.Musket.Fire` | 任意 GA 开枪瞬间 |
| `GameplayCue.Musket.Reload` | 换弹每发上弹完成 |
| `GameplayCue.Musket.ChargeFull` | 重攻击蓄力满时 |

---

## 4. GA 激活逻辑表

### 4.1 GA Tag 配置（构造函数已写死，无需 Blueprint 修改）

| GA | AbilityTags（含两组） | ActivationBlockedTags | ActivationRequiredTags | CancelAbilitiesWithTag |
|----|----------------------|----------------------|----------------------|----------------------|
| `LightAttack` | `Ability.Musket.Light`<br>`PlayerState.AbilityCast.LightAtk` | `State.Musket.Aiming`<br>`State.Musket.Reloading`<br>`Buff.Status.DashInvincible` | — | `Ability.Musket.Reload` |
| `HeavyAttack` | `Ability.Musket.Heavy`<br>`PlayerState.AbilityCast.HeavyAtk` | `State.Musket.Reloading` | — | `Ability.Musket.Reload` |
| `Reload_Single` | `Ability.Musket.Reload`<br>`PlayerState.AbilityCast.Reload` | `State.Musket.Aiming` | — | — |
| `Reload_All` | `Ability.Musket.Reload`<br>`PlayerState.AbilityCast.Reload` | `State.Musket.Aiming` | — | — |
| `SprintAttack` | `Ability.Musket.SprintAtk`<br>`PlayerState.AbilityCast.LightAtk` | — | `Buff.Status.DashInvincible` | `PlayerState.AbilityCast.Dash` |
| `SprintReload` | `Ability.Musket.SprintReload`<br>`PlayerState.AbilityCast.Reload` | — | `Buff.Status.DashInvincible` | — |

### 4.2 LightAtk 按键分支逻辑（`TryActivateAbilitiesByTag` 自动筛选）

```
LightAtk 按键按下
├── DashInvincible 存在 → SprintAttack 激活（LightAttack 被 BlockedTags 阻断）
└── DashInvincible 不存在 → LightAttack 激活（SprintAttack 被 RequiredTags 拦截）
```

### 4.3 Reload 按键分支逻辑

```
Reload 按键按下
├── DashInvincible 存在 → SprintReload 激活
├── DashInvincible 不存在
│   ├── 弹药不满 → Reload_Single（或 Reload_All，取决于谁被 Grant）激活
│   └── 弹药已满 → ActivateAbility 内部检测，立即 EndAbility
```

> **注**：`Reload_Single` 和 `Reload_All` 共用同一 `AbilityTag`（`PlayerState.AbilityCast.Reload`）。只 Grant 其中一个即可；如果两个都 Grant，`TryActivateAbilitiesByTag` 会同时尝试激活两个（可能冲突），建议只 Grant 一个。

---

## 5. M_AimArc 材质制作

### 5.1 根因说明（SM6 报错）

**报错内容**：
```
[SM6] /Project/MusketAimArc.ush:15:1: error: function definition is not allowed here
[SM6] error: use of undeclared identifier 'MusketAimArc_GetMask'
```

**根因**：SM6（DXC 编译器，DX12 路径）要求 `.ush` 文件里通过 `#include` 引入的函数必须声明为 `static`，否则视为非法的全局函数定义。SM5（FXC）无此限制。

**修复**：[Shaders/MusketAimArc.ush](../../../../Shaders/MusketAimArc.ush) 第 14 行已修改为：
```hlsl
static float MusketAimArc_GetMask(float2 UV, float HalfAngleDeg, float Softness)
```
已修复，重新打开编辑器后材质会自动重编译。

---

### 5.2 .ush 文件内容（最终版）

文件路径：`Shaders/MusketAimArc.ush`

```hlsl
// MusketAimArc.ush
// 扇形瞄准弧遮罩。
// Custom 节点写法：
//   #include "/Project/MusketAimArc.ush"
//   return MusketAimArc_GetMask(UV, HalfAngle, Softness);
// Output Type: CMOT Float 1

static float MusketAimArc_GetMask(float2 UV, float HalfAngleDeg, float Softness)
{
    float2 C = UV - 0.5;                                   // 中心化到 [-0.5, 0.5]
    float Angle = atan2(C.x, C.y);                        // +Y 朝前，右正左负
    float HalfRad = HalfAngleDeg * (3.14159265 / 180.0);  // 度→弧度
    float AngleMask = 1.0 - smoothstep(HalfRad - Softness, HalfRad + Softness, abs(Angle));
    float R = length(C);
    float OuterMask = 1.0 - smoothstep(0.44, 0.50, R);    // 外圆渐隐
    float InnerMask = smoothstep(0.0, 0.04, R);            // 中心小孔
    return AngleMask * OuterMask * InnerMask;
}
```

---

### 5.3 材质属性设置

在 M_AimArc 的 **Details 面板**（选中最终材质输出节点时右侧显示）：

| 属性 | 值 |
|------|----|
| **Material Domain** | `Deferred Decal` |
| **Blend Mode** | `Translucent` |
| **Decal Blend Mode** | `Emissive` |
| **Two Sided** | ✓ 勾选 |
| Cast Shadow | 不需要勾选 |

> `Decal Blend Mode = Emissive` 代表此 Decal 只写入 Emissive 通道，不影响场景 GBuffer 的 BaseColor/Roughness，是地面 VFX 的标准用法。

---

### 5.4 材质参数列表

| 参数名 | 类型 | 默认值 | 由 C++ 驱动 | 说明 |
|--------|------|--------|------------|------|
| `HalfAngle` | Scalar | 45 | ✓（每帧） | 扇形半角，度数。蓄力期间从 45 插值到 8 |
| `ArcRadius` | Scalar | 300 | ✓（每帧） | 扇形半径（cm）。**在材质内不需要连线**，仅需以参数形式存在（C++ SetScalarParameterValue 才不会静默失败） |
| `Color` | Vector | (1, 0.55, 0.1, 1) | ✓（状态切换） | 弧颜色。普通蓄力=金色，满蓄=亮黄色 |
| `Softness` | Scalar | 0.05 | ✗（静态） | 扇形边缘柔化宽度（弧度单位）。不由 C++ 驱动，在材质中设好默认值即可 |

> **ArcRadius 为什么需要存在但不连线**：  
> `AimArcActor::UpdateArc()` 调用了 `DynMaterial->SetScalarParameterValue("ArcRadius", RadiusCm)`。如果材质里这个参数根本不存在，调用会静默失败（不报错），但 Decal 的物理大小已由 `ArcDecal->DecalSize = FVector(128, RadiusCm, RadiusCm)` 控制，UV 0~1 自动对应物理范围，所以材质内不需要用 ArcRadius 再做 UV 缩放。

---

### 5.5 Custom 节点配置

在材质图中添加 **Custom** 节点，配置如下：

| 字段 | 值 |
|------|----|
| **Output Type** | `CMOT Float 1` |
| **Code** | 见下方 |
| 输入 Pin 1 名称 | `UV` （类型 float2） |
| 输入 Pin 2 名称 | `HalfAngle` （类型 float） |
| 输入 Pin 3 名称 | `Softness` （类型 float） |

Custom 节点 Code 字段：
```hlsl
#include "/Project/MusketAimArc.ush"
return MusketAimArc_GetMask(UV, HalfAngle, Softness);
```

---

### 5.6 节点连线图（文字描述）

```
[TexCoord[0]]                  → Custom "AimArcMask" 的 UV 输入
[ScalarParameter "HalfAngle"]  → Custom "AimArcMask" 的 HalfAngle 输入
[ScalarParameter "Softness"]   → Custom "AimArcMask" 的 Softness 输入

[ScalarParameter "ArcRadius"]  → 不连接任何输出（仅需在图中存在）

Custom "AimArcMask" 输出 → [Multiply] 的 A 输入
[VectorParameter "Color"]      → [Multiply] 的 B 输入
[Multiply] 输出                → 材质 Emissive Color

Custom "AimArcMask" 输出 → 材质 Opacity
```

**注意**：AimArcMask 的输出线要分叉：一路给 Multiply（乘颜色后做 Emissive），另一路直接给 Opacity（控制透明度，让弧外区域完全透明）。

---

### 5.7 之前截图中的错误连线

截图中 `ArcRadius (300)` 接进了 `Multiply` 的 B 输入，导致：
- Emissive = mask × 300（值域 0~300，极度过曝且没有颜色）
- Opacity 没有输出（弧外不透明，整个 Decal 区域覆盖地面）

正确做法：**Multiply B 接 `Color` VectorParameter**，`ArcRadius` 悬空不连。

---

## 6. Editor 配置步骤

按以下顺序操作，每步完成后再进行下一步。

---

### Step 1 — 确认 .ush 文件已修复

打开 `Shaders/MusketAimArc.ush`，确认第 14 行为：
```hlsl
static float MusketAimArc_GetMask(float2 UV, float HalfAngleDeg, float Softness)
```
（已在代码中修复，打开编辑器后材质会自动重编译）

---

### Step 2 — 创建 / 修复 M_AimArc 材质

1. 在 Content Browser 找到 `M_AimArc`（或新建一个）
2. 按照 [5.3](#53-材质属性设置) 设置材质属性
3. 添加所有参数节点（见 [5.4](#54-材质参数列表)）
4. 添加 Custom 节点（见 [5.5](#55-custom-节点配置)），**删除原来连到 Multiply B 的 ArcRadius 连线**
5. 按照 [5.6](#56-节点连线图文字描述) 重新连线
6. Apply → Save

---

### Step 3 — 创建 BP_AimArcActor

1. Content Browser 右键 → `Blueprint Class`
2. 父类选 **`YogAimArcActor`**（C++ 类，搜索 Yog）
3. 命名：`BP_AimArcActor`，保存在 `Blueprints/Weapon/` 或 `Blueprints/VFX/`
4. 打开 `BP_AimArcActor` → 左侧 Components 面板选 `ArcDecal`，Details 无需改
5. 在 **Class Defaults** → **Aim Arc** 分类 → **Arc Material** → 填入 `M_AimArc`
6. Compile → Save

---

### Step 4 — 创建 GA Blueprint 子类（可选，有资产时再做）

> 如果当前没有动画资产，可以跳过蒙太奇相关字段，GA 会自动跳过蒙太奇直接完成逻辑。

**GA_Musket_HeavyAttack（必须创建，因为需要填 AimArcClass）**：
1. Content Browser 右键 → `Blueprint Class` → 父类 `GA_Musket_HeavyAttack`
2. 命名 `BGA_Musket_HeavyAttack`（BP GA 前缀 BGA）
3. Class Defaults 填写：

| 字段（分类 Musket\|HeavyAtk） | 值 |
|-------------------------------|---|
| **Aim Arc Class** | `BP_AimArcActor` |
| Fire Montage | 留空（无动画时跳过） |
| Charge Time | `1.8` |
| Start Half Angle | `45` |
| End Half Angle | `8` |
| Start Radius | `300` |
| End Radius | `1200` |
| Base Damage Multiplier | `1.2` |
| Full Charge Multiplier | `2.0` |
| Normal Arc Color | `(1, 0.55, 0, 1)` 金色 |
| Full Charge Arc Color | `(1, 1, 0.1, 1)` 亮黄 |

其他 GA 如果需要 Blueprint 子类（主要是为了填蒙太奇），以同样方式创建，父类对应的 C++ 类即可。**无资产情况下直接用 C++ 类（不创建 BP 子类）也能测试**，但 HeavyAttack 必须用 BP 子类填 `AimArcClass`。

---

### Step 5 — 创建输入资产 IA_Reload

1. Content Browser 右键 → `Input` → `Input Action`
2. 命名 `IA_Reload`
3. Value Type = **`Digital (bool)`**
4. Save

---

### Step 6 — 配置 IMC_Default（输入映射上下文）

1. 打开项目已有的 `IMC_Default`（Default Mapping Context）
2. `Mappings` 数组 → `+` 添加一项：
   - **Action** → `IA_Reload`
   - **Key** → `R`
3. Save

---

### Step 7 — 配置 BP_PlayerController

1. 打开 `BP_PlayerController`（继承自 `AYogPlayerControllerBase`）
2. Class Defaults → **Input** 分类 → **Input Reload** → 填入 `IA_Reload`
3. Compile → Save

> 其他输入（LightAttack/HeavyAttack/Dash）已有对应的 `IA_*` 资产，不需要修改。

---

### Step 8 — 配置 BP_PlayerController 重攻击松键

无需额外操作。`Input_HeavyAttack` 的 `Completed` 绑定已在 C++ `SetupInputComponent()` 里完成，使用已有的 `IA_HeavyAttack` 资产自动生效。

---

### Step 9 — 向角色 ASC 授予所有火绳枪 GA

打开 `B_PlayerOne`（或角色蓝图）→ 选中 `AbilitySystemComponent` → Details → **Default Abilities** → 添加：

| GA 类（有 BP 子类用 BP 子类，否则用 C++ 类） | 说明 |
|----------------------------------------------|------|
| `BGA_Musket_HeavyAttack`（或 `GA_Musket_HeavyAttack`） | 必须授予 |
| `GA_Musket_LightAttack` | 必须授予 |
| `GA_Musket_Reload_Single` **或** `GA_Musket_Reload_All` | 二选一，不要同时授予 |
| `GA_Musket_SprintAttack` | 可选，测试冲刺攻击 |
| `GA_Musket_SprintReload` | 可选，测试冲刺换弹 |

> **不要同时授予 Reload_Single 和 Reload_All**：两者共用 `PlayerState.AbilityCast.Reload` Tag，都被授予时同时激活会引发弹药逻辑冲突。

---

### Step 10 — 配置弹药 HUD（WBP_AmmoCounter）

1. 创建 Widget Blueprint，父类 `WBP_AmmoCounter`
2. 打开 Widget Designer：在 Canvas 里放一个 **HorizontalBox**
   - 命名必须精确：`BulletIconBox`（`BindWidget` 要求完全匹配）
   - 可不设固定大小，C++ 会动态填充图标
3. 在 `BP_CombatHUD`（或你的战斗 HUD Widget）里放置 `WBP_AmmoCounter`
4. C++ 已自动从玩家 ASC 读取 `CurrentAmmo` / `MaxAmmo` 属性并绑定委托，无需蓝图逻辑

WBP_AmmoCounter 的可调参数（Class Defaults）：

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `Icon Size` | Vector2D | (22, 22) | 每个子弹图标大小（px） |
| `Icon Padding` | float | 4 | 图标间距（px） |
| `Filled Color` | LinearColor | 金色 (1, 0.8, 0.1, 1) | 有弹图标颜色 |
| `Empty Color` | LinearColor | 灰色 (0.2, 0.2, 0.2, 0.6) | 空弹图标颜色 |

---

### Step 11 — 配置属性初始值

确认 `B_PlayerOne` 的 ASC 上挂有 `PlayerAttributeSet`，并在 `DefaultAttributes GE`（或直接在 BeginPlay 里）设置：

| 属性 | 推荐初始值 |
|------|-----------|
| `PlayerAttributeSet.CurrentAmmo` | 6 |
| `PlayerAttributeSet.MaxAmmo` | 6 |

---

## 7. 无动画资产时的测试方式

所有 GA 均支持 **无蒙太奇** 运行：

| GA | 无蒙太奇时的行为 |
|----|----------------|
| `LightAttack` | 立即生成子弹（`FireMontage == nullptr` → 跳过 PlayMontage，直接 `DoFire()` → `EndAbility`） |
| `HeavyAttack` | 蓄力弧正常显示；松键后立即生成子弹，跳过开枪蒙太奇，直接 `EndAbility` |
| `Reload_Single` | `ReloadOneMontage == nullptr` → 循环延迟为 0（立即一次全部 +1），功能逻辑正确 |
| `Reload_All` | `ReloadAllMontage == nullptr` → 立即 SetAmmoToMax，直接结束 |
| `SprintAttack` | `SprintAtkMontage == nullptr` → 立即扇射所有子弹，EndAbility |
| `SprintReload` | 无蒙太奇需求，本身是纯 Timer + 属性逻辑，始终工作 |

---

## 8. 数值参数参考

### 8.1 HeavyAttack 调参建议

| 参数 | 推荐范围 | 说明 |
|------|---------|------|
| `ChargeTime` | 1.5 ~ 2.5s | 蓄力总时长；越短操作感越紧迫 |
| `StartHalfAngle` | 40° ~ 50° | 初始散布大；让玩家觉得"需要瞄准" |
| `EndHalfAngle` | 5° ~ 12° | 满蓄精度；8° ≈ 15m 处约 2m 误差 |
| `StartRadius` | 200 ~ 400cm | 弧地面初始半径 |
| `EndRadius` | 800 ~ 1500cm | 弧地面最终半径（视觉暗示"射程更远"） |
| `BaseDamageMultiplier` | 1.0 ~ 1.5 | 未满蓄倍率 |
| `FullChargeMultiplier` | 1.8 ~ 2.5 | 满蓄倍率 |

### 8.2 LightAttack 调参建议

| 参数 | 推荐范围 | 说明 |
|------|---------|------|
| `DamageMultiplier` | 0.6 ~ 1.0 | 速射单发伤害系数 |
| `HalfAngleDeg` | 10° ~ 20° | 随机散布；越大越不准 |

### 8.3 SprintAttack 调参建议

| 参数 | 推荐范围 | 说明 |
|------|---------|------|
| `HalfFanAngle` | 25° ~ 40° | 扇射覆盖范围（总角度 = 2 × HalfFanAngle） |
| `DamageMultiplier` | 0.5 ~ 0.8 | 每颗子弹系数（6 发全中则总伤 = 6 × 0.6 = 3.6× BaseAttack） |

---

## 9. 常见问题与排查

### Q1：M_AimArc 材质仍然报 SM6 错误

**排查**：
1. 确认 `Shaders/MusketAimArc.ush` 第 14 行有 `static` 关键字
2. 在 UE 编辑器里，Edit → Editor Preferences → General → Performance → 勾选 "Use Less CPU when in Background" 取消勾选看看是否 Live Update 影响重编译
3. 关闭 M_AimArc → 重新打开 → Stats 面板应显示无错误
4. 如果仍然报错，在 Custom 节点里直接 **内联 HLSL**（不用 #include），把函数体粘贴进 Code 字段，只改第一行为 `static`

### Q2：按 RMB 松手后不开枪

**根因**：`WaitGameplayEvent(GameplayEvent.Musket.HeavyRelease)` 没有收到事件。

**排查**：
1. 确认 `BP_PlayerController` 的 `Input_HeavyAttack` 字段已填 `IA_HeavyAttack`
2. 确认 `IA_HeavyAttack` 的触发器包含 `Completed`（Enhanced Input 默认行为：按下 = Started/Triggered，松手 = Completed）
3. 在 `HeavyAttackReleased()` 里加 `UE_LOG` 确认 Completed 事件有触发
4. 确认 `GameplayEvent.Musket.HeavyRelease` Tag 在 `DefaultGameplayTags.ini` 中存在

### Q3：弹药 HUD 不显示图标

**排查**：
1. 确认 Widget Designer 里有一个 `HorizontalBox`，名称 **精确** 为 `BulletIconBox`（区分大小写）
2. 确认玩家 ASC 上挂有 `PlayerAttributeSet`，且 `MaxAmmo > 0`
3. 在 `WBP_AmmoCounter::BindToASC()` 里加日志，确认 `ASC` 不为 null

### Q4：换弹 GA 与重复触发

如果 `Reload_Single` 和 `Reload_All` 都被 Grant，按 R 时两个会同时激活，弹药逻辑冲突。**解决**：只 Grant 一个。

### Q5：冲刺攻击没有触发

**排查**：
1. 确认冲刺 GA 在激活期间确实添加了 `Buff.Status.DashInvincible` Tag 到 ASC
2. 确认 `GA_Musket_SprintAttack` 已被 Grant
3. 用 `showdebug abilitysystem` 确认 RequiredTags 判断

### Q6：瞄准弧不显示

**排查**：
1. 确认 `HeavyAttack` 用的是 BP 子类 `BGA_Musket_HeavyAttack`，且 `AimArcClass` 填了 `BP_AimArcActor`
2. 确认 `BP_AimArcActor` 的 `ArcMaterial` 已填 `M_AimArc`
3. 确认 `M_AimArc` 编译无错误（Stats 面板显示绿色）
4. 在 `BeginPlay` 中 `AimArcActor::BeginPlay()` 里 `ArcMaterial` 为 null 则不会创建 `DynMaterial`，`UpdateArc` 早返回

---

## 附录：输入映射完整表

| 输入 | 资产 | 键位 | Controller 函数 | 激活 Tag |
|------|------|------|----------------|---------|
| 轻攻击（按下） | `IA_LightAttack` | LMB | `LightAtack()` | `PlayerState.AbilityCast.LightAtk` |
| 重攻击（按下） | `IA_HeavyAttack` | RMB | `HeavyAtack()` | `PlayerState.AbilityCast.HeavyAtk` |
| 重攻击（松手） | `IA_HeavyAttack` | RMB Completed | `HeavyAttackReleased()` | 发送 GameplayEvent |
| 换弹 | `IA_Reload` | R | `MusketReload()` | `PlayerState.AbilityCast.Reload` |
| 冲刺 | `IA_Dash` | Space | `Dash()` | `PlayerState.AbilityCast.Dash` |

---

*文档由 Claude Code 维护，上次更新：2026-04-21*

---

## ⚠️ Claude 编写注意事项

- **所有 GA 必须有 C++ 父类 + BP 子类**：GA_Musket_* 的 C++ 父类已编译完成，BP 子类（BGA_Musket_*）在编辑器里创建，蒙太奇和 GE 引用在 BP 里配置，C++ 里不硬编码资产路径
- **重击蓄力用 WaitInputRelease**：`GA_Musket_HeavyAttack` 的松手触发靠 `WaitInputRelease` Task，不要用 Tick 轮询，蓄力进度靠 `GetWorld()->GetTimeSeconds() - ChargeStartTime`
- **弹药用 GAS Attribute 不用变量**：弹药值必须存在 `UMusketAttributeSet::CurrentAmmo`，不要在 GA C++ 里用成员变量存弹药，否则跨 GA 同步会出问题
- **瞄准弧材质参数名区分大小写**：`M_AimArc` 的 MPC 参数名（`ArcProgress`/`ArcColor` 等）与 C++ 里 `SetVectorParameterValue` 的字符串必须完全一致，大小写错误不报错但不生效
- **子弹 Actor 生命周期**：`BP_MusketBullet` Hit 后必须调 `Destroy()`，不要依赖 LifeSpan 做清理（LifeSpan 在网络下行为不一致）
- **SprintReload 能力激活条件**：`GA_Musket_SprintReload` 的 ActivationRequiredTags 必须包含 `Character.State.Sprinting`，否则普通站立状态也会触发错误动画
- **AmmoCounter 监听方式**：`AmmoCounter` Widget 通过 `GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate` 监听，不要用 Tick 轮询 Attribute 值
