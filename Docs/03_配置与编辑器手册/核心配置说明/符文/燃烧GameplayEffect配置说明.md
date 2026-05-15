# 燃烧 Base Effect 配置说明

## 1. 设计结论

燃烧拆成两层：

| 层级 | 职责 | 配置位置 |
| --- | --- | --- |
| 状态与掉血 | 授予 `Buff.Status.Burning`，周期掉血 | `UGE_RuneBurn` + `EffectProfile.Effect` |
| 表现 | 目标身上的火焰 Niagara、地面火焰 | FA 节点或 `EffectProfile.VFX / Area` |

`Buff.Status.Burning` 不再自动播放 Niagara。不要再把燃烧特效写进 `YogAbilitySystemComponent` 的 Tag 监听里，否则会和 FA 的 `Play Niagara` 重复。

## 2. 当前 Base Effect

| 项 | 值 |
| --- | --- |
| GE 类 | `UGE_RuneBurn` |
| C++ 路径 | `Source/DevKit/Public/AbilitySystem/GameplayEffect/GE_RuneBurn.h` |
| 状态 Tag | `Buff.Status.Burning` |
| 伤害执行 | `GEExec_BurnDamage` |
| 伤害输入 | `SetByCaller: Data.Damage.Burn` |
| 默认持续 | `4.0s` |
| 默认周期 | `1.0s` |

如果需要让策划改持续时间或 Tick 间隔，优先在对应 `EP_Rune512_...` 资产里改：

| Profile 字段 | 用途 |
| --- | --- |
| `Effect.GameplayEffectClass` | 填 `UGE_RuneBurn` |
| `Effect.bOverrideDuration / Duration` | 覆盖燃烧持续时间 |
| `Effect.bOverridePeriod / Period` | 覆盖每次掉血间隔 |
| `Effect.SetByCallerValues` | 填 `Data.Damage.Burn` 和每 Tick 伤害 |

## 3. FA 配置方式

### 普通燃烧卡

推荐链路：

```text
Start
  -> Play Rune VFX Profile 或 Play Niagara
  -> Apply Rune Effect Profile(Effect=UGE_RuneBurn, Target=LastDamageTarget)
```

### 月光反向火区域

推荐链路：

```text
Start
  -> Calc Rune Ground Path Transform
  -> Spawn Rune Area Profile
```

关键配置在 `EP_Rune512_Moonlight_Reversed_Burn`：

| 字段 | 建议值 |
| --- | --- |
| `Area.Effect` | `UGE_RuneBurn` |
| `Area.TargetPolicy` | `EnemiesOnly` |
| `Area.Shape` | `Fan` |
| `Area.SetByCallerTag1` | `Data.Damage.Burn` |
| `Area.SetByCallerValue1` | 每 Tick 伤害，例如 `6` |
| `VFX.NiagaraSystem` | 目标身上的燃烧 Niagara |
| `VFX.AttachSocketName` | `spine_03` |
| `VFX.Scale` | 例如 `(0.28, 0.28, 0.28)` |
| `VFX.Lifetime` | 通常等于燃烧持续时间 |

## 4. 验收

1. 敌人进入月光火区域后获得 `Buff.Status.Burning`。
2. 敌人离开火区域后，燃烧仍持续到 `UGE_RuneBurn` 到期。
3. 火焰地面表现来自 `Area.NiagaraSystem / DecalMaterial`。
4. 敌人身上火焰表现来自 `EffectProfile.VFX` 或 FA 的 `Play Niagara`，不会再由 GE Tag 自动播放。
5. 燃烧 Tick 伤害不触发受击状态。
