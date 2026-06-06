# 连携配方 LinkFA 配置说明

## 作用

`LinkRecipes` 用来让连携卡按配方变成不同效果。配方只在 `CardType = Link` 的卡牌上配置。

## 配置位置

`RuneDataAsset -> RuneInfo -> CombatCard -> LinkRecipes`

## LinkRecipes 字段

| 字段 | 怎么填 | 说明 |
| --- | --- | --- |
| `Direction` | `Forward` 或 `Reversed` | 正向读取前一张，反向等待后一张 |
| `Condition.RequiredNeighborIdTags` | 例如 `Card.ID.Moonlight` | 要求邻卡是指定卡牌 ID |
| `Condition.RequiredNeighborEffectTags` | 例如 `Card.Effect.Attack` | 要求邻卡带有指定效果 Tag |
| `Condition.RequiredAction` | 通常 `Any` | 需要限制轻/重攻击时再填 |
| `Condition.RequiredComboTags` | 通常留空 | 需要指定连招分支时再填 |
| `LinkFlow` | 对应 LinkFA | 配方命中后执行 |
| `Multiplier` | 默认 `1.0` | 只作为结果参数/显示，不自动改伤害 |
| `ReasonText` | 简短说明 | HUD 连携提示 |

旧字段 `RequiredNeighborTypes / RequiredNeighborTags` 只保留历史兼容。新配方优先使用 `RequiredNeighborIdTags` 和 `RequiredNeighborEffectTags`。

## 月光示例

正向：攻击牌 -> 月光

| 字段 | 值 |
| --- | --- |
| `Direction` | `Forward` |
| `Condition.RequiredNeighborEffectTags` | `Card.Effect.Attack` |
| `LinkFlow` | `FA_Moonlight_Forward_Attack` |
| `ReasonText` | 攻击牌强化月光 |

反向：月光 -> 攻击牌

| 字段 | 值 |
| --- | --- |
| `Direction` | `Reversed` |
| `Condition.RequiredNeighborEffectTags` | `Card.Effect.Attack` |
| `LinkFlow` | `FA_Moonlight_Reversed_Attack` |
| `ReasonText` | 月光赋能下一张攻击牌 |

## FA 特效

连携特效不要配在 CombatCard 上。每个 `LinkFlow` 自己放 `Play Niagara` 节点。

推荐：

```text
Start -> Play Niagara -> 该连携的实际效果节点
```

这样月光 + 攻击、月光 + 燃烧、月光 + 护盾等配方可以各自拥有不同表现。

## 规则

- 连携卡不会和连携卡触发配方，即使 Tag 条件满足也不触发。
- `LinkRecipes` 非空时，运行时优先使用新配方；旧 `LinkConfig` 不参与该卡判断。
- 反向配方需要卡牌实例方向为 `Reversed`。
- 反向配方只会在下一张牌仍属于有效连招时结算。战斗系统传入 `bComboContinued=false` 时，未结算的反向连携会被清理，下一张牌只执行自己的 `BaseFlow`。
