# 512 符文 EffectProfile 配置说明

## 1. 用途

`EffectProfile` 是 512 卡牌效果的统一调参入口。FA 只负责触发顺序和节点连线；伤害、持续时间、Tick 间隔、投射物、地面区域和 VFX 优先放到 Profile 中调整。

生成路径：

```text
/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile
```

## 2. 字段说明

| 分类 | 关键字段 | 用途 |
| --- | --- | --- |
| Identity | `EffectIdTag`, `DebugName`, `DebugColor` | Trace 和调试显示 |
| Damage | `DamageMode`, `DamageValue`, `DamageLogType` | 投射物或直接伤害数值 |
| Effect | `GameplayEffectClass`, `ApplicationCount`, `SetByCallerValues` | 施加 GE、层数、每 Tick 伤害 |
| Effect Duration | `bOverrideDuration`, `Duration`, `bOverridePeriod`, `Period` | 覆盖 GE 的持续时间和周期 |
| Projectile | `Speed`, `MaxDistance`, `ProjectileCount`, `CollisionBoxExtent` | 月光刃、分裂月刃等投射物 |
| Area | `Shape`, `Duration`, `TickInterval`, `Length`, `Width`, `Effect`, `DecalMaterial`, `NiagaraSystem` | 地面路径区域，判定来自 Area Actor |
| VFX | `NiagaraSystem`, `AttachTarget`, `AttachSocketName`, `Scale`, `Lifetime` | 命中、持续、状态类表现 |

## 3. 燃烧配置规则

燃烧状态使用 `UGE_RuneBurn`，但是表现不再由 `Buff.Status.Burning` 自动触发。

| 效果 | Profile | 主要调参 |
| --- | --- | --- |
| 正向燃烧月光 | `EP_Rune512_Moonlight_Forward_Burn` | `Projectile.*`, `Effect.*`, `VFX.*` |
| 反向燃烧地面路径 | `EP_Rune512_Moonlight_Reversed_Burn` | `Area.*`, `Effect.*`, `VFX.*` |

燃烧 Profile 必填：

| 字段 | 值 |
| --- | --- |
| `Effect.GameplayEffectClass` | `UGE_RuneBurn` |
| `Effect.SetByCallerValues[0].Tag` | `Data.Damage.Burn` |
| `Effect.SetByCallerValues[0].Value` | 每 Tick 伤害 |
| `Effect.bOverrideDuration` | 勾选 |
| `Effect.Duration` | 燃烧持续时间 |
| `Effect.bOverridePeriod` | 勾选 |
| `Effect.Period` | 掉血间隔 |

反向燃烧路径额外使用：

| 字段 | 用途 |
| --- | --- |
| `Area.NiagaraSystem` | 地面火焰 |
| `Area.DecalMaterial` | 地面范围提示 |
| `VFX.NiagaraSystem` | 敌人身上的燃烧火焰 |
| `VFX.AttachSocketName` | 敌人骨骼挂点，例如 `spine_03` |

## 4. 配置顺序

1. 打开卡牌 DA，确认 `Combat Card -> Base Flow / Link Recipes` 指向正确 FA。
2. 打开对应 `EP_Rune512_...`，先改伤害、持续、范围和 VFX。
3. 只有触发顺序不对时，再打开 FA 检查连线。
4. 调试时控制台执行 `BuffFlow.Trace 1`，之后用 `Yog_DumpBuffFlowTrace` 查看 Flow、节点、Profile、目标和失败原因。

## 5. 注意

- 不要再通过 `YogAbilitySystemComponent` 的 Tag 监听自动播放燃烧特效。
- 地面贴花只负责显示，真正伤害来自 `Area.Effect` 和 `Area.TickInterval`。
- 如果有特效没有伤害，检查 `Effect.GameplayEffectClass` 和 `SetByCallerValues`。
- 如果有伤害但没有身上特效，检查 `VFX.NiagaraSystem`、挂点和 `Lifetime`。
