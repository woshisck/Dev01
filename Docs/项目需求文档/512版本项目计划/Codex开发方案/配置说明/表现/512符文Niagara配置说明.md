# 512 符文 Niagara 配置说明

## 作用

给 512 符文卡牌配置可见反馈。代码侧已经把月光弹道需要的参数暴露到 `BFNode_SpawnSlashWaveProjectile`，复杂 Niagara 图仍需在编辑器内检查。

## 月光弹道

基础表现沿用 `BP_SlashWaveProjectile`。

在对应 Flow 中打开 `BFNode_SpawnSlashWaveProjectile`，按效果配置：

| 效果 | 推荐字段 |
| --- | --- |
| 扇形多发 | `ProjectileCount`、`ProjectileConeAngleDegrees` |
| 首目标分裂 | `bSplitOnFirstHit`、`SplitProjectileCount`、`SplitConeAngleDegrees`、`SplitDamageMultiplier`、`SplitSpeedMultiplier` |
| 穿透/撞墙消失 | `MaxHitCount <= 0`、`bDestroyOnWorldStaticHit=true` |
| 重复伤害 | `DamageApplicationsPerTarget`、`DamageApplicationInterval` |
| 护甲转伤 | `bAddSourceArmorToDamage`、`SourceArmorToDamageMultiplier`、`bConsumeSourceArmorOnSpawn` |
| 额外护甲伤害 | `BonusArmorDamageMultiplier` |
| 命中附加状态 | `AdditionalHitEffect`、`AdditionalHitSetByCallerTag`、`AdditionalHitSetByCallerValue` |
| 视觉缩放 | `CollisionBoxExtent`、`bScaleVisualWithCollisionExtent`、`VisualScaleMultiplier` |

## 具体表现建议

| 卡牌 | Niagara/表现处理 |
| --- | --- |
| 燃烧 | 火焰命中、持续灼烧、范围灼烧提示。需要火焰颜色参数和持续时间参数。 |
| 中毒 | 命中毒雾、毒液路径、3 秒爆发提示。毒液路径需要碰撞或 GameplayCue 落点。 |
| 月光 | 蓝白月刃主体，Forward/Reversed 可用亮度或尾迹色区分。 |
| 溅射/分裂 | 分裂子弹生成时播放短闪和分裂尾迹。 |
| 护盾 | 护甲获得、反弹、护盾破碎三类反馈。 |
| 穿透 | 撞墙消失、穿透敌人、击退冲击波。 |
| 攻击 | 竖向月光斩或大月刃慢速持续伤害。 |
| 减伤 | 自身月光包裹，敌人减速/降攻速时用覆盖材质或脚下圈。 |

## Blueprint 传参

`BP_SlashWaveProjectile` 子类可在 `InitProjectileWithConfig` 后读取：

- Actor Scale：已按 `CollisionBoxExtent` 和 `VisualScaleMultiplier` 自动设置。
- `BP_OnHitEnemy`：命中新目标时播放命中 Niagara。
- `BP_OnExpired`：生命周期结束或撞墙消失时播放消散 Niagara。

## 验收

1. 月刃放大/缩小时碰撞和视觉尺寸一致。
2. 分裂月刃不会无限递归，受 `MaxSplitGenerations` 限制。
3. 穿透月刃能穿过敌人，撞静态物体消失。
4. 每个基础符文至少有命中或持续状态反馈。
