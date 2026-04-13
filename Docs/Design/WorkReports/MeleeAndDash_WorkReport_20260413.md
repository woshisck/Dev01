# 近战 HitBox 修复 + 冲刺越障重构 工作报告

> 日期：2026-04-13  
> 类型：Bug 修复 + 重构  
> 影响范围：YogTargetType_Melee（命中判定）、GA_PlayerDash（越障算法 + 碰撞通道）、YogGameMode（波次触发修复）  
> 配套文档：[冲刺系统设计文档](../Systems/Dash_Design.md)、[关卡系统技术文档](../Systems/LevelSystem_ProgrammerDoc.md)

---

## 一、近战 HitBox InnerR 距离补偿（Bug 修复）

**问题**：`bAutoOffset=true` 时，环形命中框（Annulus）圆心向后偏移 `InnerR`，导致前向最远触达距离缩短为 `ActRange - InnerR`，配表直觉不一致。

**修复文件**：`Source/DevKit/Private/AbilitySystem/Abilities/YogTargetType_Melee.cpp`

### IsInAnnulus（命中判定）

```cpp
// 新增 EffectiveOuterRadius：bAutoOffset 时补偿后移量
const float EffectiveOuterRadius = (Annulus.bAutoOffset && Annulus.inner_radius > 0.f)
    ? OuterRadius + Annulus.inner_radius
    : OuterRadius;
if (Dist2D < Annulus.inner_radius || Dist2D > EffectiveOuterRadius)
    return false;
```

### DrawHitboxDebug（调试可视化）

同步更新 Debug 绘制用的外径为 `EffectiveOuterR`，保证可视化与判定逻辑一致。

**结果**：`ActRange` 配多少，前向最远攻击距离就是多少，InnerR 只影响内径空洞，不再缩短外径。

---

## 二、GA_PlayerDash 越障算法重构

### 2.1 旧算法（双向 SphereTrace + SetActorLocation）

```
前向扫描找墙入口 → 墙在前半段 → 后向扫描找墙出口 →
墙厚 ≤ MaxTraversableWallThickness → SetActorLocation(墙出口) + AnimScale=0
```

缺点：SetActorLocation 传送视觉突兀；AnimScale=0 蒙太奇停止位移，玩家卡在原地视觉不连贯；逻辑与蓝图原型不一致。

### 2.2 新算法（终点步进法，严格还原蓝图）

```
GetFurthestValidDashDistance(Start, End):

  1. SphereTraceSingle(Start → End, DashTrace)
     - 无命中 → 返回 DashMaxDistance

  2. hitDist > DashMaxDistance - 2×CapsuleRadius
     → 硬墙在末端 → 返回 hitDist（停在障碍前）

  3. hitDist ≤ 阈值（障碍在前段）：
     StepOrigin = ForwardHit.TraceEnd（= End 满距终点）
     循环 i=1..6，步长 50cm：
       SphereTraceMulti(StepOrigin+(i-1)×50, StepOrigin+i×50, DashTrace)
       IsValidDashLocation → 无 ECR_Block → true → 返回 Dist(Start, SegEnd)
     6步均无效 → 返回 hitDist
```

**行为变化对比**：

| 情况 | 旧算法 | 新算法 |
|------|--------|--------|
| 无障碍 | 满距冲刺 | 满距冲刺（同） |
| 末端硬墙 | 停在墙前 | 停在墙前（同） |
| 前段薄墙/敌人 | SetActorLocation 瞬传到墙后 + AnimScale=0 | AnimScale>1 根运动穿越，Capsule Overlap |
| 全程无刚体碰撞 | 否（传送前有刚体） | 是（Capsule 全程 Overlap） |

### 2.3 碰撞通道对齐

蓝图中 `SetCollisionResponseToChannel` 设置的通道：

| 时机 | 通道 | 响应 |
|------|------|------|
| 冲刺开始 | WorldDynamic | Overlap |
| 冲刺开始 | Pawn | Overlap |
| 冲刺开始 | Enemy (GameTraceChannel3) | Overlap |
| 冲刺开始 | DashThrough (GameTraceChannel5) | Overlap |
| 冲刺结束 | 以上全部 | Block |

旧版只设置 Enemy + DashThrough，本次补齐 WorldDynamic 和 Pawn。

### 2.4 移除配置项

`MaxTraversableWallThickness`（可越障的墙最大厚度）已移除——新算法通过步进 + IsValidDashLocation 隐式判断，不需要墙厚参数。

---

## 三、YogGameMode 波次触发 Bug 修复

**问题**：若本波刷怪时所有 Spawner 均不支持该怪型（全部刷出失败），`TotalAliveEnemies` 保持为 0，但没有死亡事件可以触发 `CheckWaveTrigger`，导致流程卡死无法推进下一波。

**修复**：`SetupWaveTrigger` 的 `AllEnemiesDead` 分支在刷怪队列处理完后主动调用一次 `CheckWaveTrigger()`。

**附带修复**：`PercentKilled_50` 和 `PercentKilled_20` 的 `TotalSpawnedInWave == 0` 早退检查改为各分支内独立判断（旧版在函数顶部统一判断，导致 AllEnemiesDead 分支也被误拦截）。

---

## 关键决策

**为什么步进法不需要 MaxTraversableWallThickness？**

旧算法需要预先限定可越障的墙厚，避免穿越实心大墙。新算法通过 IsValidDashLocation 检查每一步是否存在 DashTrace-Block 命中，实心墙每步均会命中返回 false，穿越不会发生，无需单独配置厚度阈值。

**为什么 SetActorLocation 传送改成 AnimScale>1 根运动驱动？**

传送有帧间位置跳变，配合 Overlap Capsule 实现的穿越更自然——蒙太奇根运动连续推进，物理上不产生阻挡，动画与位置严格同步。

---

## 遗留问题

| 问题 | 优先级 | 说明 |
|------|--------|------|
| 蓝图 Event Received → Apply Effect Container | 低 | 冲刺命中触发效果，当前 GA 传空 EventTags 不走此路径 |
| Remove Gameplay Tag (AbilityCast.Cooldown) | 低 | 蓝图在 EndAbility 前移除此 Tag，C++ 使用 SkillChargeComponent 替代，待确认是否需要对齐 |
| BP_GA_PlayerDash 子类 | 高 | 需新建 Blueprint 子类填写 GAS Tag + AbilityDA 蒙太奇配置 |
