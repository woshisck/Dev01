# 冲刺（Dash）系统 — 设计与技术文档

> 版本：v2（2026-04-12）  
> 适用范围：玩家冲刺 GA 的完整设计、碰撞架构、位移实现、无敌帧、越障机制  
> 适用人群：策划 + 程序  

---

## 一、功能概述

玩家冲刺为**根运动驱动的短距离瞬冲**，具备：

| 特性 | 说明 |
|-----|------|
| 方向 | 跟随摇杆最后输入方向；无输入则沿角色正前方 |
| 旋转 | 冲刺前 Controller 已将角色旋转至摇杆方向，GA 直接取 ForwardVector |
| 位移 | `AnimRootMotionTranslationScale` 动态缩放，Scale = `DashMaxDistance / DashMontageRootMotionLength` |
| 越障 | 前半段遇薄墙（≤ 4m）→ 瞬间传送到墙出口，蒙太奇视觉播放 |
| 无敌帧 | 冲刺过程中授予 `Buff.Status.DashInvincible`，伤害管道检测此 Tag 跳过伤害 |
| 穿体 | 冲刺期间 Capsule 对 `Enemy` 通道设为 Overlap（穿过敌人） |
| 穿薄墙 | 冲刺期间 Capsule 对 `DashThrough` 通道设为 Overlap（穿过可穿越家具） |
| 次数/CD | `SkillChargeComponent` 管理（不走 GAS CommitAbility），支持多段冲刺和符文改 CD |

---

## 二、碰撞架构

### 2.1 通道总览

| 通道名 | 类型 | 编号 | 用途 |
|--------|------|------|------|
| `DashTrace` | Trace | ECC_GameTraceChannel1 | 路径障碍检测（SphereTrace） |
| `WorldDynamic` | Object | 内置 | 动态物体（家具/可推动物），冲刺期间 Overlap |
| `Pawn` | Object | 内置 | 其他 Pawn（敌人），冲刺期间 Overlap |
| `Enemy` | Object | ECC_GameTraceChannel3 | 敌人自定义通道，冲刺期间 Overlap |
| `DashThrough` | Object | ECC_GameTraceChannel5 | 可穿越几何体，冲刺期间 Overlap |

### 2.2 几何体配置规则

| 几何体 | Object Type | 对 DashTrace 的响应 | 冲刺期间 Capsule 行为 |
|--------|-------------|--------------------|-----------------------|
| 关卡外墙/地面 | `WorldStatic` | Block（不可穿） | Block（不修改） |
| 薄墙/家具（可穿） | `DashThrough` / `WorldDynamic` | Ignore（路径穿越） | Overlap（期间修改） |
| 敌人 | `Pawn` / `Enemy` | **Ignore**（不阻断路径） | Overlap（期间修改） |

> **注意**：敌人碰撞体对 `DashTrace` 通道需手动设为 **Ignore**，否则会截断冲刺距离。

---

## 三、位移实现

### 3.1 根运动缩放（正常冲刺）

```
AnimRootMotionTranslationScale = DashMaxDistance / DashMontageRootMotionLength
```

- 蒙太奇根运动总长固定（如 600 cm = 6m）
- 修改 `DashMaxDistance` 即可线性控制实际冲刺距离
- 不调用 `SetActorLocation`，完全由根运动驱动

### 3.2 越障（Wall Traversal）

```
GetFurthestValidDashDistance(Start, End):

  1. 前向 SphereTrace（DashTrace 通道）：Start → End（满距终点）
     - 无命中 → 返回 DashMaxDistance（正常满距）

  2. hitDist > DashMaxDistance - 2×CapsuleRadius
     → 硬墙在冲刺末端 → 返回 hitDist（停在障碍前）

  3. hitDist ≤ 阈值（障碍在前段）：
     StepOrigin = ForwardHit.TraceEnd  ← UE5 中 TraceEnd = 完整 End 参数（满距终点）
     循环 i = 1..6：
       SphereTraceMulti(StepOrigin+(i-1)×50cm, StepOrigin+i×50cm, DashTrace)
       IsValidDashLocation(命中列表) → 无 DashTrace-Block 命中 → true
         true → 返回 Dist(Start, StepOrigin+i×50cm)  ← 大于 DashMaxDistance
     6步均无效 → 返回 hitDist（停在最初障碍前）

IsValidDashLocation(Hits):
  空列表 → true
  任意命中体对 DashTrace 响应为 ECR_Block → false
  否则 → true
```

**行为说明：**
- 障碍在冲刺末端（紧贴终点）→ 停在障碍前，AnimScale < 1
- 障碍在冲刺前段（路径中段）→ 从满距终点向前步进找空旷落点，AnimScale > 1，根运动超出满距驱动玩家穿越（Capsule 全程 Overlap）
- 实心墙（WorldStatic）：每步 SphereTraceMulti 均命中 ECR_Block → IsValidDashLocation=false → 停在墙前
- 薄墙/敌人（DashThrough/Pawn，DashTrace=Ignore）：TraceMulti 命中但非 ECR_Block → IsValidDashLocation=true → 穿越

---

## 四、GA_PlayerDash 架构

### 4.1 策划配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DashMaxDistance` | 600 cm | 最远冲刺距离；越障时实际位移可超过此值 |
| `DashCapsuleRadius` | 35 cm | SphereTrace 半径（匹配角色 Capsule） |
| `DashMontageRootMotionLength` | 600 cm | 蒙太奇根运动总长，改此值要匹配蒙太奇实际数据 |

### 4.2 Tag 配置（Blueprint 子类 Class Defaults）

| 字段 | Tag |
|-----|-----|
| `AbilityTags` | `PlayerState.AbilityCast.Dash.Dash1` |
| `ActivationOwnedTags` | `Buff.Status.DashInvincible` |
| `ActivationBlockedTags` | `Buff.Status.Dead`、`PlayerState.AbilityCast.Dash` |
| `CancelAbilitiesWithTag` | `PlayerState.AbilityCast` |

### 4.3 蒙太奇配置

蒙太奇从角色 `CharacterData → AbilityData` 的 MontageMap 中读取，Key = `AbilityTags` 第一个 Tag。

> 与 `GA_MeleeAttack` 保持一致的数据驱动方式，无需在 GA Blueprint 上直接引用蒙太奇资产。

### 4.4 次数/CD 配置

通过 GAS 属性（`PlayerAttributeSet`）控制：

| GAS 属性 | 默认值 | 说明 |
|---------|--------|------|
| `MaxDashCharge` | 1 | 最大冲刺次数 |
| `DashCooldownDuration` | 3.0 s | 每格 CD 时长 |

修改：在角色 `CharacterData → AttributeData` 中填写初始值；符文可在运行时修改 GAS 属性。

### 4.5 ActivateAbility 流程

```
1. ConsumeCharge（SkillChargeComponent，消耗 1 格充能并启动 CD 回复）
2. 从 AbilityDA 读取蒙太奇
3. 取 ActorForwardVector 作为冲刺方向（Controller 已在激活前旋转角色）
4. GetFurthestValidDashDistance(Start, Start+Dir×DashMaxDistance) → EffectiveDist
5. AnimScale = EffectiveDist / DashMontageRootMotionLength
   - 正常冲刺：AnimScale ≤ 1（满距或停在障碍前）
   - 穿越冲刺：AnimScale > 1（延伸越障，根运动超出满距）
6. 修改 Capsule 碰撞：WorldDynamic + Pawn + Enemy + DashThrough → Overlap
7. PlayMontageAndWaitForEvent（AnimRootMotionTranslationScale = AnimScale）
   无 SetActorLocation，全程由根运动 + Capsule Overlap 驱动位移
```

### 4.6 EndAbility 流程

```
1. 恢复 Capsule 碰撞：Overlap → Block（WorldDynamic / Pawn / Enemy / DashThrough）
2. 绘制 Debug 线（起点→终点，显示实际位移距离）
3. Super::EndAbility（自动移除 ActivationOwnedTags → DashInvincible 消失）
```

---

## 五、无敌帧实现

**授予方式**：`ActivationOwnedTags = Buff.Status.DashInvincible`，GAS 激活时自动挂载，EndAbility 自动移除。

**伤害管道接入**（`DamageAttributeSet::PostGameplayEffectExecute`，Physical 和 Pure 两个分支均已接入）：

```cpp
static const FGameplayTag TAG_DashInvincible =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"));
UAbilitySystemComponent* TargetASC =
    Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();
if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible))
{
    return; // 跳过伤害
}
```

---

## 六、Debug 辅助

| 功能 | 说明 |
|-----|------|
| 屏幕消息（Key 200） | 每 0.5s 显示冲刺充能格数 + CD 剩余时间 |
| Debug 线 | EndAbility 时从起点→终点画青色线 + 绿球（起）+ 红球（终） |
| 屏幕消息（-1） | EndAbility 时显示总位移 UU / 米数 / 配置最大值 |

---

## 七、新增配置清单

| 类别 | 内容 | 位置 |
|-----|------|------|
| Tag | `Buff.Status.DashInvincible` | `Config/Tags/BuffTag.ini` |
| 碰撞通道 | `DashThrough` (ECC_GameTraceChannel5) | `Config/DefaultEngine.ini` |
| C++ 类 | `UGA_PlayerDash` | `Source/DevKit/Public/AbilitySystem/Abilities/GA_PlayerDash.h` |

> **v2 变更（2026-04-13）**：越障算法由双向 SphereTrace + SetActorLocation 传送改为终点步进法；碰撞修改通道新增 WorldDynamic 和 Pawn；移除 `MaxTraversableWallThickness` 配置项。

---

## 八、策划配置速查

### 敌人碰撞设置
敌人碰撞体（Capsule）对 `DashTrace` 通道设为 **Ignore**，否则冲刺路径会被敌人截断。

### 可穿越几何体（薄墙/家具）
1. 选中 Mesh → Collision → Object Type = `DashThrough`
2. 对 `DashTrace` 的 Response = **Ignore**（路径检测时跳过）

### Blueprint 子类创建
1. 新建 Blueprint，Parent Class = `GA_PlayerDash`
2. Class Defaults 填写 Tag（见 4.2 节）
3. 蒙太奇在 `CharacterData → AbilityData` MontageMap 中配置（Key = `PlayerState.AbilityCast.Dash.Dash1`）

---

## ⚠️ Claude 编写注意事项

- **越障用 SweepMultiByChannel + DashTrace 通道**：`GetFurthestValidDashDistance` 的扫描全部走 `ECC_GameTraceChannel2`（DashTrace），不要用 WorldStatic
- **台阶偏移不可删**：`StepOffset = CMC->MaxStepHeight`，所有 SweepStart/End 必须加这个高度偏移，否则台阶竖面会阻挡冲刺
- **距离计算用 Dist2D**：引入 Z 偏移后用 `FVector::Dist2D` 计算 XY 平面距离，不用 `Dist`（Z 差会导致 AnimScale 算错）
- **SetDashCollision 必须在 EndAbility 还原**：`ActivateAbility` 里改 Capsule 碰撞为 Overlap，`EndAbility` 里无论成功/取消都必须还原，否则角色永远穿透敌人
- **调试用 CVar，不用 UPROPERTY**：GA 是纯 C++ 无 Blueprint 子类，开关调试信息用 `CVarDashDebugTrace`（`Dash.DebugTrace 1`）
- **`PendingSaveComboTags` 是 mutable**：CanActivateAbility 是 const，需要缓存连招桥接 Tag 时只能用 mutable 成员
