# 月光地面路径连携 FA 配置说明

## 1. 用途

本说明只针对月光反向连携的毒/火路径：

| Flow | 表现 | 作用 |
| --- | --- | --- |
| `FA_Rune512_Moonlight_Reversed_Poison` | 地面毒路径 | 敌人踩中后获得 `GE_Poison` |
| `FA_Rune512_Moonlight_Reversed_Burn` | 地面火焰扇形路径 | 敌人踩中后获得 `UGE_RuneBurn`，持续掉血 |

反向毒/火不再使用 `Spawn Slash Wave Projectile`。普通月光、正向月光、反向攻击月光仍走投射物逻辑。

## 2. 节点链

推荐节点链：

```text
Start
  -> Calc Rune Ground Path Transform
  -> Spawn Rune Ground Path Effect
```

`Calc Rune Ground Path Transform` 只负责计算生成位置和朝向。`Spawn Rune Ground Path Effect` 只负责生成地面区域、Decal/Niagara 表现和对目标应用 GE。

旧资产中如果还能看到 `Spawn Slash Wave Projectile`、`Wait Gameplay Event`、`Play Niagara`，它们只作为兼容残留节点存在，不应从 `Start` 连到这些节点。

## 3. 位置计算节点

`Calc Rune Ground Path Transform` 推荐参数：

| 字段 | 值 | 说明 |
| --- | --- | --- |
| `Source` | `BuffOwner` | 从玩家位置开始计算 |
| `FacingMode` | `ToLastDamageTarget` | 朝向本次 AN 命中的敌人；没有命中目标时回退到玩家朝向 |
| `Length` | 火 `520` / 毒 `560` | 与生成节点的 `Length` 保持一致 |
| `SpawnOffset` | `(45, 0, 6)` | 路径起点离玩家前方 45cm |
| `bCenterOnPathLength` | 勾选 | 输出位置是路径中心，方便碰撞盒和 Decal 对齐 |
| `RotationYawOffset` | `0` | 需要整体旋转时再调整 |
| `bProjectToGround` | 勾选 | 自动贴地 |

把它的两个数据输出连接到生成节点：

| Calc 输出 | Spawn 输入 |
| --- | --- |
| `SpawnLocation` | `SpawnLocationOverride` |
| `SpawnRotation` | `SpawnRotationOverride` |

## 4. 反向中毒参数

| 字段 | 值 |
| --- | --- |
| `Effect` | `GE_Poison` |
| `TargetPolicy` | `EnemiesOnly` |
| `Shape` | `Rectangle` |
| `Duration` | `4.5` |
| `TickInterval` | `1.0` |
| `Length` | `560` |
| `Width` | `210` |
| `Height` | `120` |
| `DecalProjectionDepth` | `18` |
| `DecalPlaneRotationDegrees` | `0` |
| `SpawnOffset` | `(45, 0, 6)` |
| `DecalMaterial` | `M_Rune512_GroundPath_Poison_Decal` |
| `NiagaraSystem` | 小尺寸毒烟路径表现 |
| `NiagaraInstanceCount` | `1` |
| `bApplyOncePerTarget` | 不勾选 |

## 5. 反向燃烧参数

| 字段 | 值 |
| --- | --- |
| `Effect` | `UGE_RuneBurn` |
| `TargetPolicy` | `EnemiesOnly` |
| `Shape` | `Fan` |
| `Duration` | `4.0` |
| `TickInterval` | `0.5` |
| `Length` | `520` |
| `Width` | `230` |
| `Height` | `120` |
| `DecalProjectionDepth` | `18` |
| `DecalPlaneRotationDegrees` | `0` |
| `SpawnOffset` | `(45, 0, 6)` |
| `DecalMaterial` | `M_Rune512_GroundPath_Burn_Fan_Decal` |
| `NiagaraSystem` | 小尺寸地面火焰路径表现 |
| `NiagaraInstanceCount` | `7` |
| `Damage Tag 1 (SetByCaller)` | `Data.Damage.Burn` |
| `Damage Value 1 / Tick` | `6` |
| `bApplyOncePerTarget` | 勾选 |

## 6. 伤害如何调整

当前反向火月光的伤害由 `Spawn Rune Ground Path Effect` 节点里的 `Ground Path|Effect` 字段控制：

| 字段 | 作用 |
| --- | --- |
| `Effect=UGE_RuneBurn` | 决定使用燃烧持续伤害 GE |
| `Damage Tag 1 (SetByCaller)=Data.Damage.Burn` | 告诉燃烧 GE 读取哪个伤害参数 |
| `Damage Value 1 / Tick=6` | 每次燃烧跳伤的基础伤害 |

策划只需要改 `Damage Value 1 / Tick`。例如改成 `10`，敌人获得燃烧后每跳基础伤害就是 10。`UGE_RuneBurn` 当前周期为 1 秒，不是立即首跳。

火区本身不直接让敌人持续扣血。敌人第一次进入火区时获得 `UGE_RuneBurn` 被动；离开火区后，被动仍按自身持续时间继续每秒掉血。火区只负责“授予燃烧”，不是“站在区域内才掉血”。

燃烧、中毒、流血这类 DoT 伤害不会触发敌人的受击状态、韧性计数或受击闪白。普通刀光命中仍按正常伤害走受击逻辑。

如果需要把伤害也节点化，不要写死在 `Spawn Rune Ground Path Effect` 里，可以这样连：

```text
Literal Float / Math Float / Calc Damage
  -> Damage Value 1 / Tick
```

常用方式：

| 需求 | 配置 |
| --- | --- |
| 固定伤害 | `Literal Float.Value -> Damage Value 1 / Tick` |
| 公式伤害 | `Math Float.Result -> Damage Value 1 / Tick` |
| 读取战斗伤害计算 | `Calc Damage.Result -> Damage Value 1 / Tick` |

位置和伤害分开处理：`Calc Rune Ground Path Transform` 只算位置/朝向，`Damage Value 1 / Tick` 只控制 GE 的每跳伤害。

## 7. 贴花不投到角色上的设置

`DecalProjectionDepth` 控制 Decal 投射厚度。当前默认 `18cm`，只覆盖地面附近，避免投射到角色身体上。

如果仍看到角色脚面或身体被贴花染色，优先把 `DecalProjectionDepth` 调小到 `8-12`。不要先缩小 `Length/Width`，那会直接改变实际判定范围。

`DecalPlaneRotationDegrees` 只旋转地面贴花的纹理/遮罩，不改变伤害判定范围。当前反向毒/火默认填 `0`，用于让 Decal 视觉前向和黄色 Debug 判定区域保持一致。

如果后续更换 Decal 材质后视觉方向仍不一致，按这个顺序调整：

| 现象 | 调整 |
| --- | --- |
| 贴花前后反了 | `0` 和 `180` 互换 |
| 贴花横向/竖向反了 | 尝试 `90` 或 `270` |
| 伤害范围正确但贴花偏向错误 | 只改 `DecalPlaneRotationDegrees`，不要改 `RotationYawOffset` |

## 8. 引擎验收

1. 普通月光仍能发射月光刃。
2. 月光反向毒 + 攻击成功时，玩家前方出现毒路径，敌人踩中获得中毒。
3. 月光反向火 + 攻击成功时，玩家前方出现扇形火焰路径，敌人踩中获得燃烧持续伤害。
4. 火路径 Decal 只在地面明显显示，不应大片投射到角色身上。
5. 连招断开时不生成毒/火路径，只按断链规则释放普通月光。
