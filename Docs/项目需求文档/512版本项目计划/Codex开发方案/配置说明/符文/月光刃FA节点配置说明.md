# 月光刃 FA 节点配置说明

## 作用

`Spawn Slash Wave Projectile` 用于在 FA 中生成月光刃、刀光等水平飞行投射物。

本节点用于卡牌出牌时立即发射月光刃。旧版 `GA_SlashWaveCounter` 仍用于“每 3 次命中触发一次月光刃”的被动符文，不建议直接拿来做本次月光卡牌连携。

如果项目中已经有 `BP_SlashWaveProjectile` 和 `GE_SlashWaveDamage`，这里直接复用，不需要重新做投射物 BP。

## 节点位置

在 FA 中添加节点：

```text
Spawn Slash Wave Projectile
```

推荐连线：

```text
Start -> Spawn Slash Wave Projectile
```

## 字段说明

| 字段 | 怎么填 | 影响 |
| --- | --- | --- |
| `Projectile Class` | 选择现有 `BP_SlashWaveProjectile`，父类为 `SlashWaveProjectile` | 决定表现和碰撞 Actor |
| `Damage Effect` | 选择现有 `GE_SlashWaveDamage` | 使用旧版月光刃伤害 GE；需要支持 `Attribute.ActDamage` SetByCaller |
| `Source Selector` | `BuffOwner` | 从玩家位置和朝向生成 |
| `Damage` | 伤害数值 | 月光刃命中敌人造成的伤害 |
| `Speed` | 飞行速度，单位 cm/s | 越大飞得越快 |
| `Max Distance` | 最大距离，单位 cm | 会换算为生命周期 |
| `Max Hit Count` | 最大命中目标数 | `2` 表示命中两个敌人后消失；`0` 表示不限 |
| `Damage Applications Per Target` | 同一目标伤害次数 | `1` 表示普通单次伤害；`3` 表示同一道月光对同一目标造成 3 次伤害 |
| `Damage Application Interval` | 多次伤害间隔 | 例如 `0.25` 表示每 0.25 秒伤害一次 |
| `Collision Box Extent` | 碰撞盒半径 | 越大越容易命中，也表示范围更大 |
| `Scale Visual With Collision Extent` | 勾选 | 碰撞范围变大时，月光刃视觉也按比例变大 |
| `Visual Scale Multiplier` | 默认 `(1,1,1)` | 额外视觉缩放倍率；不改变最终碰撞范围 |
| `Projectile Visual Niagara System` | 选择月光刀光 Niagara | 作为飞行投射物的主视觉 |
| `Projectile Visual Niagara Scale` | 默认 `(1,1,1)` | 只缩放 Niagara 主视觉 |
| `bHideDefaultProjectileVisuals` | 勾选 | 隐藏 `BP_SlashWaveProjectile` 旧视觉，避免两个刀光同时出现 |
| `Projectile Count` | 普通月光填 `1` | 一次基础生成数量；分裂类才直接填多发 |
| `Add Combo Stacks To Projectile Count` | 基础月光和正向连携月光勾选 | 根据连招层数增加月光数量 |
| `Projectiles Per Combo Stack` | `1` | 每层连招多一道月光 |
| `Max Bonus Projectiles` | `2` | 最多额外两道月光 |
| `Projectile Cone Angle Degrees` | 基础月光和正向连携填 `0` | 分裂月光才使用扇形角度 |
| `Spawn Projectiles Sequentially` | 基础月光和正向连携勾选 | 多道月光沿同一路径连续发射 |
| `Sequential Projectile Spawn Interval` | `0.12` | 连续月光发射间隔 |
| `SplitProjectileCount` | 普通月光填 `0`；月光分裂填 `4` | 主月刃碰撞后生成小月刃数量 |
| `MaxSplitGenerations` | 月光分裂填 `1` | 限制只分裂一次，避免无限分裂 |
| `SplitMaxDistanceMultiplier` | 月光分裂填 `1.25` | 小月刃飞行距离倍率；数值越大，小月刃存在越久 |
| `BounceSplitChildrenOnEnemyHit` | 月光分裂勾选 | 小月刃命中敌人后允许弹射 |
| `SplitChildMaxEnemyBounces` | 月光分裂填 `1` | 每个小月刃最多按入射角弹射 1 次 |
| `Launch Niagara System` | 一般留空 | 只用于额外的一次性发射闪光，不作为月光主体 |
| `Spawn Offset` | 生成偏移 | X 是玩家前方距离，Z 是高度 |

## 推荐参数

普通月光刃：

```text
Projectile Class = BP_SlashWaveProjectile
Damage Effect = GE_SlashWaveDamage
Damage = 10
Speed = 1400
Max Distance = 800
Max Hit Count = 2
Damage Applications Per Target = 1
Damage Application Interval = 0.25
Collision Box Extent = (30, 60, 35)
Scale Visual With Collision Extent = true
Visual Scale Multiplier = (1, 1, 1)
Spawn Offset = (80, 0, 45)
Projectile Visual Niagara System = NS_Free_Magic_Slash
Projectile Visual Niagara Scale = (0.85, 0.85, 0.85)
bHideDefaultProjectileVisuals = true
Projectile Count = 1
Add Combo Stacks To Projectile Count = true
Projectiles Per Combo Stack = 1
Max Bonus Projectiles = 2
Projectile Cone Angle Degrees = 0
Spawn Projectiles Sequentially = true
Sequential Projectile Spawn Interval = 0.12
Launch Niagara System = None
```

强化月光刃：

```text
Projectile Class = BP_SlashWaveProjectile
Damage Effect = GE_SlashWaveDamage
Damage = 20
Speed = 1400
Max Distance = 800
Max Hit Count = 2
Damage Applications Per Target = 1
Damage Application Interval = 0.25
Collision Box Extent = (45, 90, 45)
Scale Visual With Collision Extent = true
Visual Scale Multiplier = (1, 1, 1)
Spawn Offset = (80, 0, 45)
Projectile Visual Niagara System = NS_Free_Magic_Slash2
Projectile Visual Niagara Scale = (1.35, 1.35, 1.2)
bHideDefaultProjectileVisuals = true
Launch Niagara System = None
```

反向连携慢速月光刃：

```text
Projectile Class = BP_SlashWaveProjectile
Damage Effect = GE_SlashWaveDamage
Damage = 10
Speed = 120
Max Distance = 120
Max Hit Count = 0
Damage Applications Per Target = 3
Damage Application Interval = 0.25
Collision Box Extent = (75, 150, 65)
Scale Visual With Collision Extent = true
Visual Scale Multiplier = (1, 1, 1)
Spawn Offset = (100, 0, 45)
Projectile Visual Niagara System = NS_Free_Magic_Area2
Projectile Visual Niagara Scale = (2.2, 2.2, 1.6)
bHideDefaultProjectileVisuals = true
Launch Niagara System = None
```

月光反向分裂：

```text
Projectile Class = BP_SlashWaveProjectile
Damage Effect = GE_SlashWaveDamage
Damage = 20
Speed = 900
Max Distance = 800
Max Hit Count = 3
Damage Applications Per Target = 1
Damage Application Interval = 0.25
Collision Box Extent = (45, 95, 45)
Scale Visual With Collision Extent = true
Spawn Offset = (80, 0, 45)
Projectile Visual Niagara System = NS_Free_Magic_Slash
bHideDefaultProjectileVisuals = true
SplitProjectileCount = 4
MaxSplitGenerations = 1
SplitConeAngle = 100
RandomizeSplitDirections = true
SplitRandomYawJitter = 22
SplitDamageMultiplier = 0.5
SplitSpeedMultiplier = 1.6
SplitMaxDistanceMultiplier = 1.25
SplitCollisionExtentMultiplier = (0.5, 0.5, 0.5)
BounceSplitChildrenOnEnemyHit = true
SplitChildMaxEnemyBounces = 1
Destroy On World Static Hit = true
```

效果：主月刃第一次碰到敌人或场景后分裂成 4 个小月刃。小月刃飞行距离更长，命中敌人后会根据入射角反射弹射 1 次，并刷新自身剩余生存时间。

## 注意事项

- `Max Hit Count = 0` 不是不能命中，而是不限命中目标数量。
- 普通月光建议 `Damage Applications Per Target = 1`；反向连携巨大月光建议填 `3`。
- `Collision Box Extent` 是最终碰撞范围；勾选 `Scale Visual With Collision Extent` 后，视觉会跟着范围比例变化，不需要再手动放大 BP。
- 如果贴脸攻击看不到弹道，增大 `Spawn Offset.X`。
- 如果月光刃太容易打中，降低 `Collision Box Extent.Y`。
- 月光主体不要同时配置旧 BP 视觉和 `Launch Niagara System`。当前推荐做法是 `Projectile Visual Niagara System` 负责刀光主体，`bHideDefaultProjectileVisuals=true`。
- 基础月光和正向连携月光的连招数量加成必须使用连续同路径发射，不要用扇形角度。分裂月光才使用多方向弹道。
