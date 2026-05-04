# 燃烧 GameplayEffect 配置说明

## 1. 用途

燃烧不是一次性命中特效。卡牌或月光连携命中敌人后，应给敌人施加 `UGE_RuneBurn`，由 GE 自己按周期持续扣血。

表现使用独立 `Play Niagara` 节点，持续伤害使用独立 `Apply Gameplay Effect Class` 节点。不要把燃烧逻辑写进投射物节点。

## 2. 资产位置

| 项 | 值 |
| --- | --- |
| GE 类 | `UGE_RuneBurn` |
| C++ 路径 | `Source/DevKit/Public/AbilitySystem/GameplayEffect/GE_RuneBurn.h` |
| 使用 Flow | `FA_Rune512_Burn_Base`、`FA_Rune512_Moonlight_Forward_Burn`、`FA_Rune512_Moonlight_Reversed_Burn` |
| 自动配置工具 | `RuneCardBatchGenerator -Apply` |

## 3. GE 固定逻辑

`UGE_RuneBurn` 是 C++ GE，不需要策划再创建蓝图 GE。

| 字段 | 当前值 |
| --- | --- |
| Duration | `4.0s` |
| Period | `1.0s` |
| Execute Periodic Effect On Application | `false` |
| Granted Tag | `Buff.Status.Burning` |
| Execution | `GEExec_BurnDamage` |
| Stack Mode | 目标身上唯一燃烧，重复应用刷新持续时间/周期 |
| Damage Source | `SetByCaller: Data.Damage.Burn` |

## 4. FA 节点配置

### 普通燃烧卡

```text
Start
  -> Play Niagara(Rune.Burn.ApplyNiagara, NS_Fire_Floor)
  -> Apply Gameplay Effect Class(UGE_RuneBurn, Data.Damage.Burn=8)
```

### 月光 + 燃烧连携

```text
Spawn Slash Wave Projectile
  -> Wait Gameplay Event(Action.Rune.MoonlightBurnHit)
  -> Play Niagara(Rune.Moonlight.BurnHitNiagara, NS_Fire_Floor)
  -> Apply Gameplay Effect Class(UGE_RuneBurn, Data.Damage.Burn=6)
```

| 节点 | 字段 | 普通燃烧卡 | 月光燃烧连携 |
| --- | --- | --- | --- |
| `Play Niagara` | `Niagara System` | `NS_Fire_Floor` | `NS_Fire_Floor` |
| `Play Niagara` | `Attach Target` | `LastDamageTarget` | `LastDamageTarget` |
| `Play Niagara` | `Attach Socket Name` | `spine_03` | `spine_03` |
| `Play Niagara` | `Scale` | `(0.28,0.28,0.28)` | `(0.28,0.28,0.28)` |
| `Play Niagara` | `Lifetime` | `3.2` | `3.2` |
| `Apply Gameplay Effect Class` | `Effect` | `UGE_RuneBurn` | `UGE_RuneBurn` |
| `Apply Gameplay Effect Class` | `Target` | `LastDamageTarget` | `LastDamageTarget` |
| `Apply Gameplay Effect Class` | `ApplicationCount` | `1` | `1` |
| `Apply Gameplay Effect Class` | `bRemoveEffectOnCleanup` | 不勾选 | 不勾选 |
| `Apply Gameplay Effect Class` | `SetByCallerTag1` | `Data.Damage.Burn` | `Data.Damage.Burn` |
| `Apply Gameplay Effect Class` | `SetByCallerValue1` | `8` | `6` |

`bRemoveEffectOnCleanup` 必须不勾选。否则 FA 停止时会清掉 GE，燃烧只出现一下就结束。

## 5. 验收

1. 命中敌人后出现 `NS_Fire_Floor`，位置跟随敌人骨骼。
2. 敌人获得 `Buff.Status.Burning`。
3. 伤害日志能看到燃烧周期伤害。
4. FA 停止后，燃烧仍持续到 `UGE_RuneBurn` 自己到期。
5. 如果表现过大，调整 `Play Niagara.Scale`，不要改 `UGE_RuneBurn`。
