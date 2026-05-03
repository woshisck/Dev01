# 512 符文 Niagara 配置说明

## 作用

给 512 符文卡牌配置可见反馈。当前规则是：特效跟随 FA 配置，不在 `CombatCard` 里配置通用 VFX。

普通出牌效果放在 `BaseFlow`；连携效果放在对应 `LinkFlow`。

当前 512 验收版已经清理旧的大型 Niagara：月光投射物、燃烧命中、中毒命中不再使用 `NS_Free_Magic_*`、`NS_Fire_Torch`、`NS_Smoke_7_acid` 作为默认生成配置。命中/状态表现优先使用 `Play Flipbook VFX`，本文件只作为后续需要 Niagara 时的兼容说明。

## Play Niagara

节点位置：

```text
BuffFlow|Visual -> Play Niagara
```

常用连线：

```text
Start -> Play Niagara -> Apply Attribute Modifier / Spawn Slash Wave Projectile / 其他效果节点
```

关键参数：

| 参数 | 说明 |
| --- | --- |
| `Niagara System` | 要播放的 Niagara 资源 |
| `Attach Target` | 播放目标，常用 `BuffOwner` 或 `LastDamageTarget` |
| `Attach Socket Name` | 角色挂点，不需要挂点就留空 |
| `bAttachToTarget` | true 为跟随目标；false 为在世界位置生成一次 |
| `Location Offset` | 相对目标的位置偏移 |
| `Rotation Offset` | 旋转偏移 |
| `Scale` | 特效缩放 |
| `Effect Name` | 需要后续销毁时填写；一次性特效可留空 |
| `bDestroyWithFlow` | FA 停止时是否清理这个特效；命中闪光类通常填 `false`，持续挂身类才填 `true` |

## 月光弹道

月光弹道主体使用 `Spawn Slash Wave Projectile`，不要用 `Play Niagara` 代替弹道逻辑。

在对应 Flow 中打开 `Spawn Slash Wave Projectile`，按效果配置：

| 效果 | 推荐字段 |
| --- | --- |
| 扇形多发 | `ProjectileCount`、`ProjectileConeAngleDegrees` |
| 首目标分裂 | `bSplitOnFirstHit`、`SplitProjectileCount`、`SplitConeAngleDegrees`、`SplitDamageMultiplier`、`SplitSpeedMultiplier` |
| 穿透/撞墙消失 | `MaxHitCount`、`bDestroyOnWorldStaticHit` |
| 重复伤害 | `DamageApplicationsPerTarget`、`DamageApplicationInterval` |
| 护甲转伤 | `bAddSourceArmorToDamage`、`SourceArmorToDamageMultiplier`、`bConsumeSourceArmorOnSpawn` |
| 额外护甲伤害 | `BonusArmorDamageMultiplier` |
| 命中附加状态 | `AdditionalHitEffect`、`AdditionalHitSetByCallerTag`、`AdditionalHitSetByCallerValue` |
| 视觉缩放 | `CollisionBoxExtent`、`bScaleVisualWithCollisionExtent`、`VisualScaleMultiplier` |
| 发射 Niagara | 512 当前验收版填 `None` |
| 命中 Niagara | 512 当前验收版填 `None`，改用 `Play Flipbook VFX` |
| 消失 Niagara | 512 当前验收版填 `None`，改用 `Play Flipbook VFX` 或不配置 |

## 月光当前默认值

| FA | 表现处理 |
| --- | --- |
| `FA_Rune512_Moonlight_Base` | 默认月光刃；`Projectile/Hit/Expire/Launch Niagara=None`，使用 BP/投射物默认可视 |
| `FA_Rune512_Moonlight_Forward_Attack` | 范围扩大、伤害提高；不配置旧 Niagara |
| `FA_Rune512_Moonlight_Reversed_Attack` | 玩家面前出现慢速巨大月光，对同一敌人造成 3 次伤害；不配置旧 Niagara |

月光其他连携不再把表现塞进 `Spawn Slash Wave Projectile` 的 Niagara 字段。毒素配方使用 `Wait Gameplay Event -> Play Flipbook VFX -> ApplyEffect -> ApplyGEInRadius`；燃烧/中毒基础牌使用小尺寸序列帧面片。完整表见 `512符文卡牌VFX配置说明.md`。

## 其他卡牌表现建议

| 卡牌 | Niagara/表现处理 |
| --- | --- |
| 燃烧 | 在 `FA_Rune512_Burn_Base` 中用 `Play Flipbook VFX`，`Texture=T_Rune512_VFX_Burn_Hit`，`Target=LastDamageTarget`，`Socket=spine_03`，`Size=78` |
| 中毒 | 在 `FA_Rune512_Poison_Base` 中用 `Play Flipbook VFX`，`Texture=T_Rune512_VFX_Poison_Hit`，`Target=LastDamageTarget`，`Socket=spine_02`，`Size=72` |
| 溅射/分裂 | 分裂时短闪和分裂尾迹 |
| 护盾 | 护盾获得、反弹、护盾破碎反馈 |
| 穿透 | 穿透敌人、撞墙消失、击退冲击波 |
| 攻击 | 出手强化闪光或武器斩击光 |
| 减伤 | 自身护罩、脚下圆、受击减伤提示 |

## 验收

1. 卡牌在攻击 AN 后触发，不在蒙太奇开始时触发。
2. 普通卡只播放 `BaseFlow` 中的 Flipbook/表现节点。
3. 月光不同连携只播放对应 `LinkFlow` 中的表现，投射物节点内 Niagara 字段保持 `None`。
4. 没有连携成功时，不播放连携 FA 中的特效。
