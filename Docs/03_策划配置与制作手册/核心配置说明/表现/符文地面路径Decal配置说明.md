# 符文地面路径 Decal 配置说明

## 1. 作用

地面路径 Decal 只负责表现。真正的伤害和 Buff 由 `Spawn Rune Ground Path Effect` 节点生成的碰撞扫描区域处理。

| 类型 | 用途 |
| --- | --- |
| 毒路径 | 月光反向中毒连携，显示绿色腐蚀/毒液路径 |
| 火路径 | 月光反向燃烧连携，显示橙红火焰扇形路径 |

当前批量生成器会自动创建并填写：

| 类型 | 材质 |
| --- | --- |
| 毒路径 | `M_Rune512_GroundPath_Poison_Decal` |
| 火路径 | `M_Rune512_GroundPath_Burn_Fan_Decal` |

## 2. 材质设置

| 项 | 值 |
| --- | --- |
| `Material Domain` | `Deferred Decal` |
| `Blend Mode` | `Translucent` |
| `Decal Blend Mode` | `Translucent` 或 `DBuffer Translucent Color` |
| `Two Sided` | 不勾选即可 |
| `Receives Decals` | 地面材质需要允许接收 Decal |

推荐参数：

| 参数 | 毒路径 | 火路径 |
| --- | --- | --- |
| `Color` | `(0.04, 0.90, 0.22)` | `(1.0, 0.22, 0.04)` |
| `Opacity` | `0.58` | `0.66` |
| `EdgeSoftness` | `0.38` | `0.22` |
| `FanMask` | `0` | `1` |
| `ScrollSpeed` | 暂不使用 | 暂不使用 |

## 3. FA 节点配置

推荐先用 `Calc Rune Ground Path Transform` 计算位置，再把输出接到 `Spawn Rune Ground Path Effect`：

| Calc 输出 | Spawn 输入 |
| --- | --- |
| `SpawnLocation` | `SpawnLocationOverride` |
| `SpawnRotation` | `SpawnRotationOverride` |

在 `Spawn Rune Ground Path Effect` 节点里配置：

| 字段 | 说明 |
| --- | --- |
| `DecalMaterial` | 填毒/火路径 Decal 材质 |
| `Shape` | 毒路径填 `Rectangle`，火路径填 `Fan` |
| `Length` | 路径前后长度，决定 Decal 长度和判定长度 |
| `Width` | 路径宽度，决定 Decal 宽度和判定宽度 |
| `Height` | 碰撞扫描高度，不影响 Decal 平面大小 |
| `DecalProjectionDepth` | Decal 投射厚度，默认 `18`，用于避免投射到角色身上 |
| `DecalPlaneRotationDegrees` | 只旋转 Decal 纹理/遮罩，不改变伤害判定。当前月光反向毒/火默认 `0` |
| `NiagaraSystem` | 可选，叠加低强度烟、火苗、毒雾 |
| `NiagaraInstanceCount` | 沿路径生成几个 Niagara，火路径默认 `7` |
| `bApplyOncePerTarget` | 火路径需要勾选，避免重复刷新 `UGE_RuneBurn` 导致周期伤害不触发 |

## 4. Decal 方向调整

伤害判定、Niagara 铺放、黄色 Debug 区域都使用路径 Actor 的本地 X 轴作为前方。Decal 材质由 UE 的投影 UV 决定显示方向，因此单独提供 `DecalPlaneRotationDegrees` 调整视觉方向。

| 现象 | 调整 |
| --- | --- |
| 贴花前后反了 | `0` 和 `180` 互换 |
| 贴花横向/竖向反了 | 尝试 `90` 或 `270` |
| 贴花方向错但伤害范围正确 | 只改 `DecalPlaneRotationDegrees` |
| 整个伤害范围也错了 | 改 `Calc Rune Ground Path Transform` 的 `FacingMode` 或 `RotationYawOffset` |

当前生成器会把月光反向毒/火的 `DecalPlaneRotationDegrees` 填为 `0`。

## 5. 不投射到角色上的处理

当前方案不是关闭角色的 `Receives Decals`，而是让路径 Decal 的投射深度变浅：

| 参数 | 建议 |
| --- | --- |
| `DecalProjectionDepth` | `8-18` |
| 地面火路径默认 | `18` |
| 仍投到角色时 | 先调到 `12`，再观察 |

这样不会影响项目里其他可能需要投到角色身上的 Decal。

## 6. 火焰表现

参考图里的大面积地火由两层组成：

1. `M_Rune512_GroundPath_Burn_Fan_Decal` 负责地面焦黑、火焰底色和路径轮廓。
2. `NiagaraSystem` 负责地面上方的烟和火苗，`NiagaraInstanceCount=7` 会沿扇形路径铺开。

如果火焰太遮挡观察，优先调小 `NiagaraScale`。如果火焰覆盖范围太小，优先调大 `Length/Width`，这两个参数也会同步影响实际伤害判定范围。

## 7. 伤害说明

Decal 和 Niagara 都不造成伤害。反向火月光的伤害来自：

| 字段 | 当前值 |
| --- | --- |
| `Effect` | `UGE_RuneBurn` |
| `Damage Tag 1 (SetByCaller)` | `Data.Damage.Burn` |
| `Damage Value 1 / Tick` | `6` |

只调表现时不要改伤害字段。只调伤害时不要改 Decal 或 Niagara。

`Damage Value 1 / Tick` 是 Float 数据输入，可以接 `Literal Float`、`Math Float`、`Calc Damage` 等节点输出。需要策划可视化调参时，推荐先用 `Literal Float` 接入；需要随连击、攻击力或其他数值变化时，再改为 `Math Float` 或 `Calc Damage`。

火区扫描命中敌人时只授予一次 `UGE_RuneBurn` 被动。敌人离开火区后，燃烧被动仍按 GE 自己的持续时间继续掉血。燃烧 tick 不触发 HitReact、韧性计数或受击闪白。
