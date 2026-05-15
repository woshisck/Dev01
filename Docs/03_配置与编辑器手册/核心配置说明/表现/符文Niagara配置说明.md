# 符文 Niagara 配置说明

## 1. 当前规则

符文卡牌表现默认恢复使用 Niagara，但必须放在 FA 的独立 `Play Niagara` 节点里。

不要把燃烧、中毒、命中表现直接填进 `Spawn Slash Wave Projectile` 的 `Launch/Hit/Expire Niagara` 字段。投射物节点只负责月刃生成、飞行、伤害和命中事件。

推荐链路：

```text
Spawn Slash Wave Projectile
  -> Wait Gameplay Event
  -> Play Niagara
  -> Apply Gameplay Effect Class / ApplyGEInRadius
```

## 2. Play Niagara 参数

节点位置：

```text
BuffFlow | Visual -> Play Niagara
```

| 字段 | 当前推荐 |
| --- | --- |
| `Niagara System` | 填实际 Niagara 资产 |
| `Effect Name` | 必填，便于日志和后续清理，例如 `Rune.Moonlight.BurnHitNiagara` |
| `Attach Target` | 命中特效用 `LastDamageTarget` |
| `Attach Socket Name` | 燃烧 `spine_03`，中毒 `spine_02` |
| `Attach Socket Fallback Names` | `spine_03, spine_02, spine_01, spine, pelvis, body, root` |
| `bAttachToTarget` | 持续挂身效果填 `true`；范围扩散一次性表现可填 `false` |
| `Location Offset` | 燃烧 `(0,0,6)`；中毒命中 `(0,0,8)`；中毒扩散 `(0,0,18)` |
| `Scale` | 燃烧 `(0.28,0.28,0.28)`；中毒命中 `(0.32,0.32,0.32)`；扩散 `(0.45,0.45,0.45)` |
| `Lifetime` | 燃烧 `3.2`；中毒命中 `1.2`；扩散 `1.4` |
| `bDestroyWithFlow` | 当前默认不勾选，避免 FA 停止时把状态表现立刻删掉 |

## 3. 当前使用资产

| 用途 | Niagara System |
| --- | --- |
| 燃烧命中/持续表现 | `/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor` |
| 中毒命中 | `/Game/Art/EnvironmentAsset/VFX/Niagara/Smoke/NS_Smoke_7_acid` |
| 中毒扩散 | `/Game/Art/EnvironmentAsset/VFX/Niagara/Smoke/NS_Smoke_7_acid` |

## 4. 月光 FA 规则

月光弹道仍使用 `Spawn Slash Wave Projectile`。

| 字段 | 填写 |
| --- | --- |
| `Projectile Visual Niagara System` | `None` |
| `Launch Niagara System` | `None` |
| `Hit Niagara System` | `None` |
| `Expire Niagara System` | `None` |
| `Hit Gameplay Event Tag` | 根据连携填写，例如 `Action.Rune.MoonlightBurnHit` 或 `Action.Rune.MoonlightPoisonHit` |

月光 + 燃烧：

```text
Spawn Slash Wave Projectile
  -> Wait Gameplay Event(Action.Rune.MoonlightBurnHit)
  -> Play Niagara(NS_Fire_Floor, Target=LastDamageTarget)
  -> Apply Gameplay Effect Class(UGE_RuneBurn)
```

月光 + 中毒：

```text
Spawn Slash Wave Projectile
  -> Wait Gameplay Event(Action.Rune.MoonlightPoisonHit)
  -> Play Niagara(NS_Smoke_7_acid, Target=LastDamageTarget)
  -> Apply Gameplay Effect Class(GE_Poison, Target=LastDamageTarget, ApplicationCount=3)
  -> Play Niagara(NS_Smoke_7_acid, Target=LastDamageTarget, bAttachToTarget=false)
  -> ApplyGEInRadius(GE_PoisonSplash/GE_Poison, Radius=300, MaxTargets=3)
```

## 5. 验收

1. 月刃命中后，日志出现 `[PlayNiagara] Spawned`。
2. `Spawn Slash Wave Projectile` 内的 Niagara 字段保持 `None`。
3. 燃烧和中毒表现来自独立 `Play Niagara` 节点。
4. 燃烧 GE 不随 FA 停止立刻清理，敌人应持续获得 `Buff.Status.Burning` 并周期掉血。
5. 如果特效过大，优先调 `Play Niagara.Scale`，不要改投射物碰撞体。
