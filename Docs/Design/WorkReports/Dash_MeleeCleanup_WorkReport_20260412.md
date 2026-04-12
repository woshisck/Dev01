# 冲刺系统 + 近战清理 工作报告

> 日期：2026-04-12  
> 类型：新功能 + 重构  
> 影响范围：GA_PlayerDash（新增）、GA_MeleeAttack（重构）、AN_MeleeDamage（清理）、DamageAttributeSet（无敌帧）  
> 配套文档：[冲刺系统设计文档](../Systems/Dash_Design.md)

---

## 本次完成

### 一、GA_PlayerDash — C++ 实现（新增）

**新增文件：**
- `Source/DevKit/Public/AbilitySystem/Abilities/GA_PlayerDash.h`
- `Source/DevKit/Private/AbilitySystem/Abilities/GA_PlayerDash.cpp`

**核心功能：**

| 功能 | 实现方式 |
|---|---|
| 方向 | Controller 激活前旋转角色，GA 直接取 `ActorForwardVector` |
| 位移 | `AnimRootMotionTranslationScale = DashMaxDistance / DashMontageRootMotionLength`，根运动驱动，不调用 `SetActorLocation` |
| 越障（薄墙穿越） | 前向 + 后向双 SphereTrace，满足条件时 `SetActorLocation(墙出口)` + `AnimScale = 0` |
| 无敌帧 | `ActivationOwnedTags = Buff.Status.DashInvincible`，GAS 自动授予/移除 |
| 穿敌人 | 冲刺期间 Capsule 对 `Enemy` 通道 → Overlap，EndAbility 恢复 Block |
| 穿薄墙 | 冲刺期间 Capsule 对 `DashThrough` 通道 → Overlap，EndAbility 恢复 Block |
| 次数/CD | `SkillChargeComponent`：`CanActivate → HasCharge`；`Activate → ConsumeCharge` |
| 蒙太奇 | 从 `CharacterData → AbilityData → GetMontage(FirstTag)` 读取，与 `GA_MeleeAttack` 一致 |
| Debug | 屏幕充能/CD 信息（每 0.5s）+ EndAbility 绘制位移线段 |

**策划可配参数（Blueprint Class Defaults）：**

| 参数 | 默认值 | 说明 |
|---|---|---|
| `DashMaxDistance` | 600 cm | 最远冲刺距离 |
| `DashCapsuleRadius` | 35 cm | 障碍检测球半径 |
| `DashMontageRootMotionLength` | 600 cm | 蒙太奇根运动总长（匹配实际蒙太奇数据） |
| `MaxTraversableWallThickness` | 400 cm | 可越障的墙最大厚度 |

---

### 二、越障算法（ComputeDashTarget）

```
越障判断条件（同时满足）：
  ① 墙在冲刺前半段（WallEntryDist < DashMaxDistance * 0.5）
  ② 离墙剩余 ≥ 100 cm（排除接触时刻刚刚触墙的情况）
  ③ 后向扫描能找到墙出口
  ④ 墙厚 ≤ MaxTraversableWallThickness（默认 4m）

越障行为：
  SetActorLocation(墙出口, Z 不变)  ← 瞬间传送
  AnimRootMotionTranslationScale = 0  ← 蒙太奇纯视觉，不产生位移

停在墙前（不越障）：
  AnimScale = WallEntryDist / DashMontageRootMotionLength  ← 根运动缩短到墙前
```

---

### 三、新增碰撞通道 DashThrough

| 项目 | 值 |
|---|---|
| 通道名 | `DashThrough` |
| 编号 | `ECC_GameTraceChannel5` |
| 类型 | Object Channel |
| 默认响应 | Block |
| 用途 | 可穿越几何体（薄墙/家具），冲刺期间 Capsule 改为 Overlap |
| 配置文件 | `Config/DefaultEngine.ini` |

---

### 四、无敌帧接入伤害管道

在 `DamageAttributeSet::PostGameplayEffectExecute` 的 **Physical** 和 **Pure** 两个分支均加入检查：

```cpp
static const FGameplayTag TAG_DashInvincible =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.DashInvincible"));
if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_DashInvincible))
    return; // 冲刺期间跳过所有伤害
```

新增 Tag：`Config/Tags/BuffTag.ini` → `Buff.Status.DashInvincible`

---

### 五、AN_MeleeDamage 字段清理

**移除字段：**

| 字段 | 移除原因 |
|---|---|
| `ActRotateSpeed` | 旋转逻辑迁移到独立 AnimNotifyState（ANS_AttackRotate） |
| `JumpFrameTime` | 已无调用方，逻辑未实现 |
| `FreezeFrameTime` | 已无调用方，逻辑未实现 |

**字段重命名：**

| 旧名 | 新名 | 类型变化 |
|---|---|---|
| `AdditionalTargetEffects` | `AdditionalRuneEffects` | `TArray<TObjectPtr<UGameplayEffect>>` → `TArray<TObjectPtr<URuneDataAsset>>` |

**YogCharacterBase 对应变更：**

| 旧名 | 新名 |
|---|---|
| `PendingAdditionalHitEffects` | `PendingAdditionalHitRunes` |
| （无） | 新增 `ReceiveOnHitRune` BlueprintNativeEvent |

---

### 六、ANS_AttackRotate — 新建 AnimNotifyState

**新增文件：**
- `Source/DevKit/Public/Animation/ANS_AttackRotate.h`
- `Source/DevKit/Private/Animation/ANS_AttackRotate.cpp`

替代 `AN_MeleeDamage.ActRotateSpeed`，作为独立的 AnimNotifyState 挂在蒙太奇时间轴上：
- 在 NotifyBegin/Tick 期间，按摇杆 `LastInputDirection` 平滑旋转角色朝向
- 策划可在蒙太奇任意时间段插入，控制旋转窗口长度

---

### 七、GA_MeleeAttack 重构

**数据读取方式调整：**

| 旧方式 | 新方式 |
|---|---|
| `AbilityMap.Find(FirstTag)` → `FActionData`（含 Montage） | `AbilityData->GetMontage(FirstTag)`（独立 MontageMap） |
| ActionData 包含所有攻击参数 | 攻击参数直接从 `AN_MeleeDamage` Notify 对象读取 |

**新增机制：**

| 成员 | 说明 |
|---|---|
| `CachedDamageNotify` | ActivateAbility 时从蒙太奇扫描第一个 `AN_MeleeDamage`，供 `StatBeforeATK` 使用 |
| `LastFiredDamageNotify` | OnEventReceived 时更新，供 `StatAfterATK` 使用（多段命中取最后一击）|

**OptionalObject 路由变更：**

| 旧方式 | 新方式 |
|---|---|
| `EventData.OptionalObject = this`（GA 自身） | OptionalObject 由 `AN_MeleeDamage::Notify()` 设置为 Notify 本身 |
| `YogTargetType_Melee` 从 GA 拿 ActionData | 从 `OptionalObject`（Notify）直接读 HitboxTypes / ActRange |

---

## 关键决策

**为什么位移改用根运动而非 SetActorLocation？**

原实现同时调用 `SetActorLocation` 和 `AnimRootMotionTranslationScale=1.0`，导致位移翻倍。只用根运动缩放可以让动画和物理位移严格同步，也支持用蒙太奇时间线精确控制位移曲线。

**为什么越障选择瞬间传送（SetActorLocation）而非继续用根运动穿墙？**

根运动无法穿越碰撞几何体（被 ACharacter 的物理阻挡）。越障必须在碰撞响应被修改之前确定落点，然后直接设置位置，AnimScale=0 让蒙太奇继续播放以保留视觉动画。

**为什么 AdditionalTargetEffects 改成 RuneDataAsset？**

原来直接挂 GE，策划无法用 Flow Graph 可视化编排复杂触发逻辑。改为 RuneDA 后，复杂 buff 效果在 Flow Graph 中实现，程序层只负责"命中时触发对应符文"。

---

## 遗留问题

| 问题 | 优先级 | 说明 |
|---|---|---|
| 敌人碰撞设置 | 高 | 敌人胶囊对 `DashTrace` 通道需手动设为 Ignore，否则冲刺被截断 |
| BP_GA_PlayerDash 子类 | 高 | 需新建 Blueprint 子类填写 GAS Tag 和蒙太奇配置 |
| 可穿越几何体配置 | 中 | 薄墙/家具网格体 Object Type 需手动改为 `DashThrough` |
| 血量/伤害倍率施加 | 中 | `FDifficultyConfig.HealthMultiplier / DamageMultiplier` 刷怪时未对敌人施加 GE |
| 符文三选一 UI | 中 | 逻辑层完整，`OnLootGenerated` 委托未绑定 UI |
| 金币 HUD | 中 | `OnGoldChanged` 委托未绑定 UI |

---

## 下次计划

1. **BP_GA_PlayerDash**：新建 Blueprint 子类，填写 Tag + 在 AbilityDA 中配置蒙太奇
2. **敌人碰撞**：将敌人胶囊碰撞预设中 DashTrace 响应改为 Ignore
3. **符文三选一 UI**：绑定 `OnLootGenerated` 委托到 WBP_LootSelection
4. **金币 HUD**：绑定 `OnGoldChanged` 委托
