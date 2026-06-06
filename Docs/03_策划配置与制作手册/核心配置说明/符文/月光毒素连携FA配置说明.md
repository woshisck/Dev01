# 月光毒素连携 FA 配置说明

## 1. 目标

月光毒素连携采用原子节点组合，不把毒素命中、扩散、次级伤害塞进投射物节点。

当前默认效果：

1. 月刃命中主目标。
2. 主目标播放小尺寸中毒 Niagara。
3. 主目标获得 3 层 `GE_Poison`。
4. 主目标位置播放一次毒素扩散 Niagara。
5. 半径 300 内最多 3 个次级敌人获得小额毒伤/毒层，排除主目标。

## 2. 推荐节点链

```text
Spawn Slash Wave Projectile
  -> Wait Gameplay Event(Action.Rune.MoonlightPoisonHit)
  -> Play Niagara(Rune.Moonlight.PoisonHitNiagara)
  -> Apply Gameplay Effect Class(GE_Poison)
  -> Play Niagara(Rune.Moonlight.PoisonSpreadNiagara)
  -> ApplyGEInRadius(GE_PoisonSplash/GE_Poison)
```

## 3. 投射物节点

| 字段 | 值 |
| --- | --- |
| `Hit Gameplay Event Tag` | `Action.Rune.MoonlightPoisonHit` |
| `Launch Niagara System` | `None` |
| `Hit Niagara System` | `None` |
| `Expire Niagara System` | `None` |
| `AdditionalHitEffect` | `None` |

## 4. 命中 Niagara

| 字段 | 值 |
| --- | --- |
| `Niagara System` | `NS_Smoke_7_acid` |
| `Effect Name` | `Rune.Moonlight.PoisonHitNiagara` |
| `Attach Target` | `LastDamageTarget` |
| `Attach Socket Name` | `spine_02` |
| `Attach Socket Fallback Names` | `spine_03, spine_02, spine_01, pelvis, root` |
| `bAttachToTarget` | 勾选 |
| `Location Offset` | `(0,0,8)` |
| `Scale` | `(0.32,0.32,0.32)` |
| `Lifetime` | `1.2` |
| `bDestroyWithFlow` | 不勾选 |

## 5. 主目标中毒

| 字段 | 值 |
| --- | --- |
| 节点 | `Apply Gameplay Effect Class` |
| `Effect` | `GE_Poison` |
| `Target` | `LastDamageTarget` |
| `ApplicationCount` | `3` |
| `bRemoveEffectOnCleanup` | 不勾选 |

## 6. 扩散 Niagara

| 字段 | 值 |
| --- | --- |
| `Niagara System` | `NS_Smoke_7_acid` |
| `Effect Name` | `Rune.Moonlight.PoisonSpreadNiagara` |
| `Attach Target` | `LastDamageTarget` |
| `bAttachToTarget` | 不勾选 |
| `Location Offset` | `(0,0,18)` |
| `Scale` | `(0.45,0.45,0.45)` |
| `Lifetime` | `1.4` |

## 7. 次级目标传播

| 字段 | 值 |
| --- | --- |
| 节点 | `ApplyGEInRadius` |
| `Effect` | `GE_PoisonSplash`，没有时可用 `GE_Poison` |
| `Radius` | `300` |
| `LocationSource` | `LastDamageTarget` |
| `bEnemyOnly` | 勾选 |
| `bExcludeLocationSourceActor` | 勾选 |
| `MaxTargets` | `3` |
| `ApplicationCount` | `1` |
| `SetByCallerTag1 / Value1` | `Data.Damage / 5` |

## 8. 验收

1. 月光毒素连携成功时，命中主目标后出现 `[PlayNiagara] Spawned Effect=Rune.Moonlight.PoisonHitNiagara`。
2. 主目标获得 `Buff.Status.Poisoned`。
3. 两个敌人靠近时，次级敌人能受到小额毒伤/毒层。
4. 半径外敌人不受影响。
5. 连携失败或连招断开时，不播放毒素 LinkFlow。
