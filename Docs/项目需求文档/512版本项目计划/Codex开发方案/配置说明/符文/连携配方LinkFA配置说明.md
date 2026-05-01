# 连携配方 LinkFA 配置说明

## 作用

`LinkRecipes` 用来让连携卡按配方变成不同效果。配方只在 `CardType = Link` 的卡牌上配置。

## 配置位置

`RuneDataAsset -> RuneInfo -> CombatCard -> LinkRecipes`

## LinkRecipes 字段

| 字段 | 怎么填 | 说明 |
| --- | --- | --- |
| `Direction` | `Forward` 或 `Reversed` | 正向读前一张，反向等后一张 |
| `Condition.RequiredNeighborIdTags` | 例如 `Card.ID.Moonlight` | 要求邻卡是指定卡牌 ID |
| `Condition.RequiredNeighborEffectTags` | 例如 `Card.Effect.Attack` | 要求邻卡有指定效果 Tag |
| `Condition.RequiredNeighborTags` | 新资产尽量少用 | 旧 CardTags 条件，兼容保留 |
| `Condition.RequiredNeighborTypes` | 可留空 | 新逻辑优先用 ID / Effect Tag |
| `Condition.RequiredAction` | 通常 `Any` | 需要限制轻/重攻击时再填 |
| `LinkFlow` | 对应 LinkFA | 配方命中后执行 |
| `Multiplier` | 默认 1.0 | 只作为结果参数/显示，不自动改伤害 |
| `ReasonText` | 简短说明 | HUD 连携提示 |

## 月光示例

正向：攻击牌 -> 月光

| 字段 | 值 |
| --- | --- |
| `Direction` | `Forward` |
| `Condition.RequiredNeighborEffectTags` | `Card.Effect.Attack` |
| `LinkFlow` | `FA_Moonlight_Forward` |
| `ReasonText` | 攻击牌强化月光 |

反向：月光 -> 攻击牌

| 字段 | 值 |
| --- | --- |
| `Direction` | `Reversed` |
| `Condition.RequiredNeighborEffectTags` | `Card.Effect.Attack` |
| `LinkFlow` | `FA_Moonlight_Backward` |
| `ReasonText` | 月光赋能下一张攻击牌 |

## 规则

- 连携卡不会和连携卡触发配方，即使 Tag 条件满足也不会触发。
- `LinkRecipes` 非空时，运行时优先使用新配方；旧 `LinkConfig` 不再参与该卡判断。
- 反向配方需要卡牌实例方向为 `Reversed`。当前先通过 DA 的 `DefaultLinkOrientation` 测试，后续再接背包反转 UI。
