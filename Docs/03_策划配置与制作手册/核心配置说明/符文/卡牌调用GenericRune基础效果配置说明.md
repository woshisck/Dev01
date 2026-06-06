# 核心卡牌调用 Generic Rune 基础效果配置说明

## 作用

把已有 Generic Rune 基础效果接入 512 战斗卡牌。核心卡牌只负责出牌、消耗和调用 FA；流血、中毒、燃烧等状态的具体规则，统一在 Generic Rune 的 GA/GE 内维护。

## 生成位置

| 类型 | 路径 |
| --- | --- |
| 卡牌 DA | `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_*` |
| 512 BaseFlow | `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow/FA_Rune512_*_Base` |
| Generic 模板 FA | `/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_*` |

## 卡牌配置

| 卡牌 | CardIdTag | CardEffectTags | BaseFlow 模板 | 当前用途 |
| --- | --- | --- | --- | --- |
| 燃烧 | `Card.ID.Burn` | `Card.Effect.Burn` | `FA_Effect_Burn` | 命中后赋予燃烧持续效果 |
| 中毒 | `Card.ID.Poison` | `Card.Effect.Poison` | `FA_Effect_Poison` | 命中后赋予中毒持续效果 |
| 流血 | `Card.ID.Bleed` | `Card.Effect.Bleed` | `FA_Effect_Bleed` | 命中后赋予流血 |
| 撕裂 | `Card.ID.Rend` | `Card.Effect.Rend` | `FA_Effect_Rend` | 命中后赋予移动距离掉血 |
| 伤口 | `Card.ID.Wound` | `Card.Effect.Wound` | `FA_Effect_Wound` | 命中后赋予受击额外扣血 |
| 击退 | `Card.ID.Knockback` | `Card.Effect.Knockback` | `FA_Effect_Knockback` | 命中后赋予受击击退状态 |
| 恐惧 | `Card.ID.Fear` | `Card.Effect.Fear` | `FA_Effect_Fear` | 命中后赋予逃离触发点效果 |
| 冻结 | `Card.ID.Freeze` | `Card.Effect.Freeze` | `FA_Effect_Freeze` | 命中后赋予冻结预警 |
| 眩晕 | `Card.ID.Stun` | `Card.Effect.Stun` | `FA_Effect_Stun` | 命中后尝试赋予眩晕 |
| 诅咒 | `Card.ID.Curse` | `Card.Effect.Curse` | `FA_Effect_Curse` | 命中后赋予诅咒 |

所有这些卡牌都应保持：

| 字段 | 值 |
| --- | --- |
| `Is Combat Card` | 勾选 |
| `Card Type` | `Normal` |
| `Required Action` | `Any` |
| `Trigger Timing` | `On Hit` |
| `Link Recipes` | 空 |

## 需要策划检查的规则

| 效果 | 检查点 |
| --- | --- |
| 流血 | 有护甲时不触发 |
| 中毒 | 按最大生命值掉血、不至死；有护甲时可触发，并对护甲造成额外扣除 |
| 撕裂 | 根据移动距离掉血；原地 2 秒后消失；有护甲时触发概率下降 |
| 伤口 | 目标再次受击时额外扣血；有护甲时触发概率下降 |
| 击退 | 受击时击退；有护甲时击退距离降低并受到额外护甲伤害 |
| 燃烧 | 固定持续伤害；无护甲或有流血时伤害提高 |
| 眩晕 | 霸体免疫；获得霸体时驱散已有眩晕 |
| 恐惧 | 玩家不获得恐惧；敌人 2 秒内未离开 800 单位时受伤 |
| 冻结 | 3 秒内未离开 800 单位时冻结眩晕并受伤 |
| 诅咒 | 每个负面效果降低最大生命值 |

如果上述规则不符合预期，优先修改对应 Generic Rune 的 GA/GE。不要直接改 `DA_Rune512_*` 的连线来临时绕过，否则后续同类状态会出现两套逻辑。

## 月光连携关系

新增状态卡不会自动获得月光连携配方。月光 LinkFlow 只保留已设计过的有限配方，例如火焰路径、中毒路径、攻击强化等。

如果后续要做“月光 + 流血”或“月光 + 眩晕”，需要单独新增月光 `LinkRecipes` 和对应 LinkFA。

## 测试

1. 把目标卡加入武器 `InitialCombatDeck`。
2. 攻击命中 `AN_MeleeDamage` 后卡牌才消耗。
3. 命中敌人后检查目标身上是否获得对应状态 Tag 或 GA/GE。
4. 针对护甲、霸体、移动距离等特殊条件分别做单项测试。
