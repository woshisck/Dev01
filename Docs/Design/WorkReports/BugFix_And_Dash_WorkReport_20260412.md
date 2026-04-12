# Bug 修复与冲刺系统重构 工作报告 2026-04-12

> 适用范围：AbilityData 污染修复 / 冲刺 GA 架构重构 / Debug 工具  
> 适用人群：程序  
> 配套文档：[冲刺系统设计](../Systems/Dash_Design.md)、[充能系统指南](../Systems/SkillCharge_Guide.md)

---

## 一、本次完成内容

### 1.1 Bug 修复：AbilityData PIE 污染（根治）

**问题根因**

`RestoreRunStateFromGI`（在 `HandleStartingNewPlayer_Implementation` 中 `Possess` 之后调用）会执行 `WeaponDefinition::SetupWeaponToCharacter`，将武器的 `WeaponAbilityData` 写入 `CD->AbilityData`。此时 `CharacterDataComponent::BeginPlay` 尚未运行，`CharacterData` 仍指向内容浏览器中的原始资产（`IsTransient=0`），导致写入直接污染 DA 文件。第二次 PIE 启动时，该 DA 已被上次的武器数据污染，角色带错误技能集。

之前的 BeginPlay `DuplicateObject` 方案之所以无效：污染发生在 BeginPlay **之前**，DuplicateObject 复制的已经是污染后的数据。

**修复方案**

利用 `InitializeComponent`（在 Actor 的 `PostInitializeComponents` 阶段调用，**早于 Possess 和 BeginPlay**）捕获资产的干净引用和干净 AbilityData，在 `EndPlay` 时无条件还原原始资产。

**修改文件**

| 文件 | 修改 |
|---|---|
| `CharacterDataComponent.h` | 新增 `InitializeComponent()` 声明；新增 `OriginalCharacterDataRef` UPROPERTY |
| `CharacterDataComponent.cpp` | 构造函数加 `bWantsInitializeComponent = true`；实现 `InitializeComponent`（保存 Ref + AbilityData）；BeginPlay 不再覆盖 `OriginalAbilityData`；EndPlay 通过 `OriginalCharacterDataRef->AbilityData = OriginalAbilityData` 无条件还原 |

**执行时序**

```
SpawnActor
  └─ PostInitializeComponents
       └─ CDC::InitializeComponent  ← ✅ 在此捕获干净值
  └─ Possess → RestoreRunStateFromGI → SetupWeaponToCharacter
       └─ CD->AbilityData = WeaponAbilityData  ← 此时写入原始 DA（已有 Ref 兜底）
  └─ CDC::BeginPlay → DuplicateObject（创建运行时副本）
  └─ CDC::EndPlay → OriginalCharacterDataRef->AbilityData = OriginalAbilityData  ← 无条件还原
```

---

### 1.2 重构：冲刺系统架构（根运动驱动 + 越障穿墙）

**原架构**：`GetFurthestValidDashLocation` 分步 SphereTrace 找最远点 → `SetActorLocation` 瞬移

**新架构**：`ComputeDashTarget` 决策引擎 → 根运动驱动位移（或越障瞬移）

#### 位移方式对比

| | 原架构 | 新架构 |
|---|---|---|
| 位移驱动 | `SetActorLocation`（瞬移） | 根运动（`AnimRootMotionTranslationScale`） |
| Scale 计算 | N/A | `DashMaxDistance / DashMontageRootMotionLength` |
| 越障能力 | 无 | 有（前向 + 后向双扫描） |
| 配置参数 | `DashMaxDistance`, `DashSearchSteps` | `DashMaxDistance`, `DashMontageRootMotionLength`, `MaxTraversableWallThickness` |

#### ComputeDashTarget 算法

```
ComputeDashTarget(Start, Direction):

  Step 1：前向扫描 SphereTrace(Start → Start+DashMaxDistance, DashTrace)
    - 无障碍 → AnimScale = DashMaxDistance / RootMotionLength，正常根运动冲刺，返回
    - 有障碍（入口距离 = WallEntryDist）

  Step 2：障碍在后半段（WallEntryDist >= DashMax*0.5）或剩余空间不足
    → AnimScale = WallEntryDist / RootMotionLength，在墙前停止，返回

  Step 3：障碍在前半段 → 后向扫描 SphereTrace(Start+DashMax+MaxWallThick → 障碍入口)
    - 找到墙出口（WallExitDist）
    - 计算 WallThickness = WallExitDist - WallEntryDist

  Step 4：墙厚 > MaxTraversableWallThickness → 在墙前停止，返回

  Step 5：越障条件满足
    → bIsTraversal=true，TraversalEnd = 墙出口位置
    → SetActorLocation(TraversalEnd, false)，AnimScale=0（蒙太奇仅作视觉）
```

#### 新配置参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| `DashMaxDistance` | 600 cm | 目标冲刺距离 |
| `DashMontageRootMotionLength` | 600 cm | 蒙太奇原始根运动长度（需与美术确认） |
| `MaxTraversableWallThickness` | 400 cm | 可越障的最大墙厚 |

**修改文件**

| 文件 | 修改 |
|---|---|
| `GA_PlayerDash.h` | 新增 `DashMontageRootMotionLength`、`MaxTraversableWallThickness` UPROPERTY；新增 `ComputeDashTarget` 声明；移除 `DashSearchSteps` |
| `GA_PlayerDash.cpp` | 实现 `ComputeDashTarget`；`ActivateAbility` 改用根运动驱动；越障分支保留 `SetActorLocation` |

---

### 1.3 新增：冲刺 Debug 工具

#### 屏幕持续打印（每 0.5s）

在 `OnAvatarSet` 启动 0.5s 循环 Timer，打印：

```
冲刺: X/N 次 | CD: X.Xs
冲刺: X/N 次 | CD: 就绪
```

`OnRemoveAbility` 时清除 Timer。

#### EndAbility 时画 Debug 线

蒙太奇结束（根运动完成）后，在 `EndAbility` 绘制：
- 青色线：起点 → 真实落点（含根运动位移）
- 绿色球：起点
- 红色球：终点
- 屏幕打印：`[Dash] 总位移: XXX UU (≈X.Xm) | 配置最大: 600 UU`

> 之前在 `ActivateAbility` 的 `SetActorLocation` 后画线，只反映瞬移段，不含根运动。移到 `EndAbility` 才是完整位移。

**新增 API**

| 文件 | 新增 |
|---|---|
| `SkillChargeComponent.h/.cpp` | `GetCDRemaining(FGameplayTag)` — 返回当前 RecoveryTimer 剩余秒数 |
| `GA_PlayerDash.h/.cpp` | `OnAvatarSet`, `OnRemoveAbility`, `PrintDashDebugInfo`, `DebugPrintTimer`, `DashDebugStartLocation` |

---

### 1.4 调整：冲刺 CD 默认值

| 文件 | 修改 |
|---|---|
| `PlayerAttributeSet.cpp` | `DashCooldownDuration` 3.0f → **1.0f** |

---

### 1.5 新增：刷怪 & 奖励 Debug 打印

| 触发点 | 内容 |
|---|---|
| `YogGameMode::SpawnNextOneByOne` | 每次刷出敌人时屏幕打印：`[刷怪] 波次X | 队列X/N | 存活X | 敌人类名` |
| `YogGameMode::EnterArrangementPhase` | 生成 RewardPickup 后打印：`[奖励拾取物] LootPool(N): 符文1 | 符文2 | ...`（持续 10s） |

---

## 二、已确认架构决策

| 决策 | 说明 |
|---|---|
| CDC 污染修复用 InitializeComponent | BeginPlay 太晚（Possess 已发生）；PostInit 阶段最早、最安全 |
| 冲刺改根运动驱动 | 消除 SetActorLocation 瞬移感；实际位移与动画自然吻合 |
| 越障用双向扫描 | 前向找入口，后向找出口，精确计算墙厚判断是否可穿 |
| Debug 线在 EndAbility 绘制 | 根运动结束后位置才准确；ActivateAbility 时绘制只有瞬移段 |
| DashMontageRootMotionLength 独立配置 | 蒙太奇根运动长度由美术决定，策划配置 DashMaxDistance，Scale 自动算 |

---

## 三、遗留问题 / 待确认

| 项目 | 说明 |
|---|---|
| `DashMontageRootMotionLength` 校准 | 需美术确认冲刺蒙太奇的实际根运动位移量（cm），填入 BP_GA_PlayerDash |
| 越障视觉效果 | 越障时 AnimScale=0 → 蒙太奇在原地播放，视觉上角色瞬移后播放冲刺动画；如需隐藏可加隐身帧 |
| 血量/伤害倍率应用 | 刷怪时仍未将 HealthMultiplier / DamageMultiplier 施加到敌人属性 |
| 符文三选一 UI | 委托 `OnLootGenerated` 已存在，蓝图 UI 未实现 |
| 金币 HUD | 委托 `OnGoldChanged` 已存在，HUD 绑定未实现 |
