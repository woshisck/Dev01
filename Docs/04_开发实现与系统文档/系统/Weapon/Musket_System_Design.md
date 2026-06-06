# 火绳枪武器系统设计文档

> 版本：v2.0 | 日期：2026-04-19 | 架构：全 C++ 逻辑，BP 仅配置

---

## 系统概览

火绳枪是本游戏第一件远程武器，所有游戏逻辑在 C++ 中实现，Blueprint 子类只需在 Class Defaults 中填写资产引用和数值参数。

---

## C++ 文件索引

| 类 | 头文件 | 说明 |
|---|---|---|
| `UGA_MusketBase` | `Abilities/Musket/GA_MusketBase.h` | 基类，弹药/移动/子弹工具方法 |
| `UGA_Musket_LightAttack` | `Abilities/Musket/GA_Musket_LightAttack.h` | 速射 |
| `UGA_Musket_HeavyAttack` | `Abilities/Musket/GA_Musket_HeavyAttack.h` | 蓄力瞄准（TickAbility + WaitInputRelease） |
| `UGA_Musket_Reload_Single` | `Abilities/Musket/GA_Musket_Reload_Single.h` | 单发循环上弹 |
| `UGA_Musket_Reload_All` | `Abilities/Musket/GA_Musket_Reload_All.h` | 全换弹 |
| `UGA_Musket_SprintAttack` | `Abilities/Musket/GA_Musket_SprintAttack.h` | 冲刺全弹扇射 |
| `UGA_Musket_SprintReload` | `Abilities/Musket/GA_Musket_SprintReload.h` | 冲刺换弹（并发） |
| `AYogAimArcActor` | `Item/Weapon/AimArcActor.h` | Decal 扇形指示器 |
| `AMusketBullet` | `Projectile/MusketBullet.h` | 单目标子弹 |

**属性（已加入 `UPlayerAttributeSet`）：**
- `CurrentAmmo`（默认 6）
- `MaxAmmo`（默认 6）

---

## Gameplay Tags

| Tag | 来源 | 用途 |
|---|---|---|
| `State.Musket.Aiming` | HeavyAttack ActivationOwnedTags | 蓄力期间标记，符文钩子 |
| `State.Musket.Reloading` | Reload GAs ActivationOwnedTags | 换弹期间标记 |
| `Ability.Musket.Light` | LightAttack AbilityTags | 用于 Cancel/Block |
| `Ability.Musket.Heavy` | HeavyAttack AbilityTags | |
| `Ability.Musket.Reload` | Reload GAs AbilityTags | Light/Heavy 激活时自动取消换弹 |
| `Ability.Musket.SprintAtk` | SprintAttack AbilityTags | |
| `Ability.Musket.SprintReload` | SprintReload AbilityTags | |
| `GameplayCue.Musket.Fire` | 开枪时执行 | 音效+枪口特效 |
| `GameplayCue.Musket.Reload` | 上弹时执行 | 上弹音效 |
| `GameplayCue.Musket.ChargeFull` | 蓄力满时执行一次 | 提示音+弧颜色变化 |
| `GameplayEffect.DamageType.Ranged` | 伤害 GE Asset Tag | 远程伤害分类 |

---

## 弹药操作原理

所有弹药变更通过 `ASC->SetNumericAttributeBase(CurrentAmmo, ...)` 直接修改属性，`PlayerAttributeSet::PreAttributeChange` 负责钳制到 [0, MaxAmmo]。

**不需要为弹药创建 GE 内容**（除非需要 GE 驱动的符文钩子）。

---

## 伤害 GE：`GE_MusketBullet_Damage`（需创建）

| 字段 | 值 |
|---|---|
| Duration Policy | Instant |
| Modifiers | `BaseAttributeSet.DmgTaken` → Additive → SetByCaller(`Attribute.ActDamage`) |
| Asset Tags | `GameplayEffect.DamageType.Ranged`, `GameplayEffect.DamageTrait.Instant` |

---

## Blueprint 子类创建清单

所有子类：父类选对应 C++ 类，**只在 Class Defaults 里填字段，不写任何 Blueprint 逻辑节点**。

### BP_GA_Musket_LightAttack

| 字段 | 值 |
|---|---|
| **基类公用** | |
| `BulletClass` | `BP_MusketBullet` |
| `BulletDamageEffectClass` | `GE_MusketBullet_Damage` |
| `MuzzleSocketName` | `"Muzzle"` |
| **轻攻击专属** | |
| `FireMontage` | 速射蒙太奇资产 |
| `FireEventTag` | 蒙太奇 AnimNotify 发送的 Tag（可留空，留空=激活时立即开枪） |
| `DamageMultiplier` | `0.8` |
| `HalfAngleDeg` | `15.0` |

---

### BP_GA_Musket_HeavyAttack

| 字段 | 值 |
|---|---|
| **基类公用** | 同上 |
| `AimArcClass` | `BP_AimArcActor` |
| `FireMontage` | 重攻击开枪蒙太奇 |
| `ChargeTime` | `1.8` |
| `StartHalfAngle` | `45.0` |
| `EndHalfAngle` | `8.0` |
| `StartRadius` | `300.0` |
| `EndRadius` | `1200.0` |
| `BaseDamageMultiplier` | `1.2` |
| `FullChargeMultiplier` | `2.0` |
| `NormalArcColor` | 橙色 (1.0, 0.55, 0.0, 1.0) |
| `FullChargeArcColor` | 亮黄 (1.0, 1.0, 0.1, 1.0) |

---

### BP_GA_Musket_Reload_Single

| 字段 | 值 |
|---|---|
| `ReloadOneMontage` | 单发上弹动画（约 0.7s） |
| （基类公用字段不影响此 GA，可留空） | |

**GAS Tag 配置（Class Defaults → Gameplay Ability → Tags）：**  
构造函数已设置，无需手动填写。

---

### BP_GA_Musket_Reload_All

| 字段 | 值 |
|---|---|
| `ReloadAllMontage` | 整体换弹动画 |

---

### BP_GA_Musket_SprintAttack

| 字段 | 值 |
|---|---|
| **基类公用** | `BulletClass` / `BulletDamageEffectClass` / `MuzzleSocketName` |
| `SprintAtkMontage` | 冲刺攻击动画 |
| `KnockbackEffectClass` | 击退 GE（可复用现有 KnockBack GE） |
| `HalfFanAngle` | `30.0` |
| `DamageMultiplier` | `0.6` |

**击退说明：** `KnockbackEffectClass` 会被存入子弹，子弹命中时在 `BP_OnHitEnemy` 中对目标施加。若要纯 C++ 驱动击退，为 `AMusketBullet` 增加 `KnockbackEffectClass` 字段（后续迭代）。

---

### BP_GA_Musket_SprintReload

| 字段 | 值 |
|---|---|
| `RefillDelay` | `0.3`（冲刺动画中点时机） |

---

## BP_AimArcActor 配置

1. 创建 Blueprint，父类 = `AYogAimArcActor`
2. 在 Class Defaults → `ArcMaterial` 填写 `M_AimArc`
3. `M_AimArc` Decal 材质节点说明（见下）

### M_AimArc 材质节点（新建材质资产）

```
Material Domain: Deferred Decal
Blend Mode: Translucent
Decal Blend Mode: Translucent

[TexCoord] → 减 0.5 → [UVCentered]
[UVCentered.y] → Atan2([UVCentered.x], [UVCentered.y]) → [Angle]（atan2 返回弧度）
[HalfAngle Param（标量，度数）] → ×(π/180) → [HalfRad]
Abs(Angle) < HalfRad → [InAngle]
Length(UVCentered) < 0.5 → [InRadius]     ← UV归一化，0.5=Decal边界
InAngle AND InRadius → [Mask]

[Mask] × SmoothStep → Opacity（边缘柔化）
[Color Param（向量）] × [Mask] → Emissive Color
```

**参数名必须与 AimArcActor.cpp 中常量一致：**
- `HalfAngle`（标量，度数）
- `ArcRadius`（标量，仅用于可选径向渐变）
- `Color`（向量）

---

## BP_MusketBullet 配置

1. 父类 = `AMusketBullet`
2. `BP_OnHitEnemy`（Blueprint Implementable Event）：
   - 播放命中 Niagara 特效
   - 播放命中音效
   - （可选）对目标施加击退 GE
3. `BP_OnMiss`：播放消散特效（Niagara）
4. Class Defaults → `Speed` = `2800`，`Lifetime` = `0.8`

---

## DA_Weapon_Musket 配置（WeaponDefinition）

`WeaponAbilityData`（UAbilityData 资产，Key=AbilityTag, Value=Montage）装备时自动 Grant：

```
GA_Musket_LightAttack   AbilityTag = Ability.Musket.Light
GA_Musket_HeavyAttack   AbilityTag = Ability.Musket.Heavy
GA_Musket_Reload_Single AbilityTag = Ability.Musket.Reload
GA_Musket_Reload_All    AbilityTag = Ability.Musket.Reload
GA_Musket_SprintAttack  AbilityTag = Ability.Musket.SprintAtk
GA_Musket_SprintReload  AbilityTag = Ability.Musket.SprintReload
```

---

## WBP_AmmoCounter

HUD Widget，绑定 `PlayerAttributeSet.CurrentAmmo` 和 `MaxAmmo`，显示 `当前/最大` 弹药数（建议图标行展示，每颗子弹一个 Icon）。

---

## 符文预留钩子

| 参数 | GAS 接口 | 典型符文 |
|---|---|---|
| 弹仓容量 | `PlayerAttributeSet.MaxAmmo` Additive | +2 弹仓 |
| 满蓄弧度 | `HeavyAtk.EndHalfAngle` → 替换为更小值 | 神枪手 |
| 速射散射 | `LightAtk.HalfAngleDeg` | 散弹手 |
| 子弹穿透 | `BP_MusketBullet.BP_OnHitEnemy` 不 Destroy | 穿透符文 |
| 缓慢移动 | 响应 `State.Musket.Aiming` 添加 → 解锁移动 + GE MoveSpeed ×0.3 | 游击手 |
| 双管 | HeavyAtk 蓄力满后 SpawnBullet 调用两次 | 双管符文 |
| 冲刺不换弹 | SprintReload 激活条件符文修改 | |

---

## 每颗子弹独立伤害

`AMusketBullet` 命中时各自执行 `ApplyDamageTo`（一次 `ApplyGameplayEffectSpecToTarget`）。冲刺攻击 N 颗子弹 = N 次独立伤害判定，暴击在 DamageExecution 中各自 Roll。

---

*文档对应 v2.0：全 C++ 逻辑，BP 仅配置资产和数值参数*
