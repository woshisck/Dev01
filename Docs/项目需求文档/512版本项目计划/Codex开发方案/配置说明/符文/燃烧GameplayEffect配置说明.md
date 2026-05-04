# 燃烧 GameplayEffect 配置说明

## 1. 用途

燃烧是持续状态，不是一帧命中特效。卡牌、月光火区域或其他系统命中敌人后，只需要给目标施加 `UGE_RuneBurn`。

目标获得 `Buff.Status.Burning` 后，`UYogAbilitySystemComponent` 会自动在目标身上挂载通用燃烧 Niagara；燃烧 tag 移除时自动清理特效。这样月光火区域、普通燃烧卡、后续怪物技能都复用同一套携带特效。

## 2. 资产位置

| 项 | 值 |
| --- | --- |
| GE 类 | `UGE_RuneBurn` |
| C++ 路径 | `Source/DevKit/Public/AbilitySystem/GameplayEffect/GE_RuneBurn.h` |
| 状态 Tag | `Buff.Status.Burning` |
| 身上携带特效 | `/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor` |
| 自动挂载代码 | `UYogAbilitySystemComponent::HandleStatusNiagaraTag` |

## 3. GE 固定逻辑

`UGE_RuneBurn` 是 C++ GE，不需要策划再创建蓝图 GE。

| 字段 | 当前值 |
| --- | --- |
| Duration | `4.0s` |
| Period | `1.0s` |
| Execute Periodic Effect On Application | `false` |
| Granted Tag | `Buff.Status.Burning` |
| Execution | `GEExec_BurnDamage` |
| Stack Mode | 目标身上唯一燃烧，重复应用刷新持续时间 |
| Damage Source | `SetByCaller: Data.Damage.Burn` |

## 4. FA 配置方式

FA 不需要再额外接“身上燃烧 Niagara”节点，只负责施加燃烧 GE。

### 普通燃烧卡

```text
Start
  -> Apply Gameplay Effect Class(UGE_RuneBurn, Target=LastDamageTarget, Data.Damage.Burn=8)
```

### 月光反向火区域

```text
Start
  -> Spawn Rune Ground Path Effect
       Effect=UGE_RuneBurn
       TargetPolicy=EnemiesOnly
       Shape=Fan
       SetByCallerTag1=Data.Damage.Burn
       SetByCallerValue1=6
```

地面火焰和地面贴花仍由 `Spawn Rune Ground Path Effect` 配置；敌人进入区域后获得 `UGE_RuneBurn`，身上的燃烧表现由 `Buff.Status.Burning` 自动触发。

## 5. 可调参数

| 调整目标 | 配置位置 |
| --- | --- |
| 每秒燃烧伤害 | FA 节点的 `SetByCallerValue1`，Tag 必须是 `Data.Damage.Burn` |
| 燃烧持续时间 | `UGE_RuneBurn` C++ 中的 Duration |
| 燃烧 tick 间隔 | `UGE_RuneBurn` C++ 中的 Period |
| 身上燃烧特效大小 | `UYogAbilitySystemComponent::StartStatusNiagara` 中 Burning 配置的 Scale，当前 `(0.28,0.28,0.28)` |
| 身上燃烧挂点 | `spine_03`， fallback 为 `spine_02`、`pelvis`、`root` |
| 月光火区域范围 | `FA_Rune512_Moonlight_Reversed_Burn` 的 `Length / Width / Shape` |

## 6. 验收

1. 敌人进入月光火区域后获得 `Buff.Status.Burning`。
2. 敌人身上出现并跟随通用燃烧 Niagara。
3. 敌人离开火区域后，燃烧仍持续到 `UGE_RuneBurn` 到期。
4. 燃烧 tick 掉血不触发受击状态。
5. `Buff.Status.Burning` 移除后，身上燃烧特效消失。
