# 512 符文 EffectProfile 配置说明

## 1. 用途
EffectProfile 是 512 卡牌效果的数值与表现配置入口。FA 只负责“什么时候触发、节点怎么连”，伤害、持续时间、堆叠、投射物、地面区域、Niagara 表现优先在 Profile 里调整。

生成路径：

`/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile`

当前生成器会为月光基础、月光正向/反向连携生成 `EP_Rune512_...` Profile，并让新 Profile 节点读取这些配置。旧节点保留在 FA 内用于对照，不作为优先调参入口。

## 2. Profile 字段

| 分类 | 关键字段 | 用途 |
| --- | --- | --- |
| Identity | `EffectIdTag`, `DebugName`, `DebugColor` | 调试显示用。`DebugName` 会出现在 BuffFlow Trace 中。 |
| Damage | `DamageMode`, `DamageValue`, `DamageLogType` | 基础伤害与伤害日志类型。月光投射物伤害优先改这里。 |
| Effect | `GameplayEffectClass`, `ApplicationCount`, `SetByCallerValues` | 施加 GE、层数、每秒伤害等。燃烧用 `Data.Damage.Burn` 这类 SetByCaller。 |
| Projectile | `Speed`, `MaxDistance`, `MaxHitCount`, `ProjectileCount`, `SequentialProjectileSpawnInterval`, `CollisionBoxExtent` | 月光刃、分裂月刃等投射物参数。 |
| Area | `Shape`, `Duration`, `TickInterval`, `Length`, `Width`, `Effect`, `DecalMaterial`, `NiagaraSystem` | 火/毒地面路径区域。伤害判定来自区域 Actor，不来自贴花。 |
| VFX | `NiagaraSystem`, `AttachTarget`, `AttachSocketName`, `Scale`, `Lifetime` | 命中、持续、状态类表现。 |

## 3. 月光配置入口

| 效果 | Profile | 主要调参 |
| --- | --- | --- |
| 普通月光 | `EP_Rune512_Moonlight_Base` | `DamageValue`, `Projectile.Speed`, `Projectile.MaxDistance`, `Projectile.MaxHitCount` |
| 正向燃烧月光 | `EP_Rune512_Moonlight_Forward_Burn` | `DamageValue`, `Projectile.ProjectileCount`, `Projectile.SequentialProjectileSpawnInterval` |
| 正向中毒月光 | `EP_Rune512_Moonlight_Forward_Poison` | `DamageValue`, `Projectile.Speed`, `Projectile.HitGameplayEventTag` |
| 反向火路径 | `EP_Rune512_Moonlight_Reversed_Burn` | `Area.Shape=Fan`, `Area.Duration`, `Area.TickInterval`, `Area.Length`, `Area.Width`, `Area.SetByCallerValue1` |
| 反向毒路径 | `EP_Rune512_Moonlight_Reversed_Poison` | `Area.Duration`, `Area.TickInterval`, `Area.Length`, `Area.Width`, `Area.Effect` |
| 反向分裂月刃 | `EP_Rune512_Moonlight_Reversed_Split` | `Projectile.bSplitOnFirstHit`, `SplitProjectileCount`, `SplitConeAngleDegrees`, `SplitMaxDistanceMultiplier`, `SplitChildMaxEnemyBounces` |

## 4. 配置顺序

1. 打开对应卡牌 DA，确认 `Combat Card -> Base Flow / Link Recipes` 指向正确 FA。
2. 打开对应 `EP_Rune512_...` Profile，先改数值、范围、持续时间、VFX。
3. 只有触发顺序不对时，再打开 FA 检查连线。
4. 打开控制台执行 `BuffFlow.Trace 1`，攻击触发后用 `Yog_DumpBuffFlowTrace` 查看节点、Profile、目标、失败原因。

## 5. 注意

- 不要优先在旧 `Spawn Slash Wave Projectile` 或旧 `Spawn Rune Ground Path Effect` 节点里调 512 月光数值；这些节点现在主要用于兼容和对照。
- 地面路径的贴花只负责显示，真正伤害来自 Profile 的 `Area.Effect` 和 `Area.TickInterval`。
- 燃烧、毒等持续效果要同时确认 `GameplayEffectClass` 和 `SetByCallerValues`，否则会出现有特效但没有实际掉血。
