# 月光毒素连携 FA 配置说明

月光毒素连携采用“月刃命中事件 + 毒素命中表现 + 主目标毒层 + 次级目标扩散”的原子节点链。不要把毒素逻辑继续写进 `Spawn Slash Wave Projectile` 的单个字段里。

## 1. 适用 Flow

| Flow | 用途 |
| --- | --- |
| `FA_Rune512_Moonlight_Forward_Poison` | 正向月光 + 毒素 |
| `FA_Rune512_Moonlight_Reversed_Poison` | 反向月光 + 毒素 |

## 2. 节点链

```text
Start
  -> Spawn Slash Wave Projectile
  -> Wait Gameplay Event(Action.Rune.MoonlightPoisonHit)
  -> Play Flipbook VFX(毒素命中)
  -> ApplyEffect(GE_Poison, 主目标 x3)
  -> Play Flipbook VFX(毒素扩散)
  -> ApplyGEInRadius(GE_PoisonSplash/GE_Poison, 排除主目标)
```

## 3. Spawn Slash Wave Projectile

| 字段 | 正向推荐值 | 反向推荐值 |
| --- | --- | --- |
| `Damage` | `25` | `12` |
| `Speed` | `900` | `280` |
| `Max Distance` | `800` | `220` |
| `Max Hit Count` | `3` | `0` |
| `Projectile Count` | `1` | `3` |
| `Damage Applications Per Target` | `1` | `3` |
| `Damage Application Interval` | `0.25` | `0.2` |
| `Projectile Visual Niagara System` | `None` | `None` |
| `Hit Gameplay Event Tag` | `Action.Rune.MoonlightPoisonHit` | `Action.Rune.MoonlightPoisonHit` |
| `Expire Gameplay Event Tag` | `Action.Rune.MoonlightPoisonExpired` | `Action.Rune.MoonlightPoisonExpired` |
| `Hit Niagara System` | `None` | `None` |
| `Expire Niagara System` | `None` | `None` |
| `Additional Hit Effect` | `None` | `None` |

说明：月刃命中时只发事件，毒素命中特效和扩散效果由后面的节点处理。512 当前验收版不再使用旧 `NS_Free_Magic_*` 投射物 Niagara，月刃主体使用 BP/投射物默认可视，避免旧特效过大或和月刃重复显示。

## 4. Wait Gameplay Event

| 字段 | 值 |
| --- | --- |
| `Event Tag` | `Action.Rune.MoonlightPoisonHit` |
| `Target` | `BuffOwner` |

该节点等待月刃命中事件。连携没有成功或月刃没有命中时，后续毒素节点不会执行。

## 5. Play Flipbook VFX

毒素命中：

| 字段 | 值 |
| --- | --- |
| `Texture` | `T_Rune512_VFX_Poison_Hit` |
| `Material` | `M_Rune512_FlipbookSprite` |
| `Duration` | `0.38` |
| `Size` | `72` |
| `Target` | `LastDamageTarget` |
| `Socket` | `spine_02` |
| `Offset` | `(0,0,8)` |

毒素扩散：

| 字段 | 值 |
| --- | --- |
| `Texture` | `T_Rune512_VFX_Poison_Spread` |
| `Material` | `M_Rune512_FlipbookSprite` |
| `Duration` | `0.45` |
| `Size` | `180` |
| `Target` | `LastDamageTarget` |
| `Socket` | 留空 |
| `Offset` | `(0,0,18)` |

## 6. ApplyEffect

主目标中毒：

| 字段 | 值 |
| --- | --- |
| `Effect` | `/Game/Code/GAS/Abilities/Shared/GE_Poison` |
| `Target` | `LastDamageTarget` |
| `Application Count` | `3` |

## 7. ApplyGEInRadius

次级敌人扩散：

| 字段 | 值 |
| --- | --- |
| `Effect` | 优先 `GE_PoisonSplash`，没有时用 `GE_Poison` |
| `Radius` | `300` |
| `Location Source` | `LastDamageTarget` |
| `Exclude Location Source Actor` | 勾选 |
| `Max Targets` | `3` |
| `Application Count` | `1` |
| `SetByCallerTag1` | `Data.Damage` |
| `SetByCallerValue1` | `5` |

## 8. 验收

1. 月光毒素连携成功时，命中敌人身上出现小毒素命中序列帧。
2. 主目标获得毒层。
3. 半径 300cm 内最多 3 个次级敌人获得扩散毒层/小伤害。
4. 半径外敌人不受影响。
5. 连携失败、断链、没有命中时不播放毒素 LinkFlow 特效。

## 9. 关联 GE

中毒本身由 `GEExec_PoisonDamage` 计算，不再使用旧的 `B_MaxHealthDamage` Modifier。配置参考同目录：

`中毒GameplayEffect配置说明.md`
