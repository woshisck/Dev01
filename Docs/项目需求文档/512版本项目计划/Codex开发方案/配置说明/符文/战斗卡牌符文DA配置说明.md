# 战斗卡牌符文 DA 配置说明

## 作用

把 `RuneDataAsset` 配成可进入战斗卡组的卡牌。当前卡牌分类只有：普通卡牌、连携卡牌、终结技卡牌。攻击、月光、攻击提升等不是分类，而是效果 Tag。

## 配置位置

打开目标 `RuneDataAsset`：

`RuneInfo -> CombatCard`

展示信息继续配置：

`RuneInfo -> RuneConfig -> RuneName / RuneIcon / RuneDescription`

## 字段配置

| 字段 | 推荐配置 | 说明 |
| --- | --- | --- |
| `bIsCombatCard` | 勾选 | 勾选后才会进入战斗卡组 |
| `CardType` | `Normal` / `Link` / `Finisher` | 新资产不要再用旧 `Attack`；旧 `Attack` 会兼容为普通卡 |
| `CardIdTag` | 例如 `Card.ID.Moonlight` | 卡牌唯一 ID，用于连携配方查找 |
| `CardEffectTags` | 例如 `Card.Effect.Attack` | 表示卡牌效果类型，攻击卡通过这里标记 |
| `RequiredAction` | 普通卡填 `Any` | 高级字段；普通卡不依赖 Light / Heavy |
| `TriggerTiming` | 当前建议 `OnHit` | 高级字段；卡牌会在 `AN_MeleeDamage` 命中帧结算和消耗 |
| `BaseFlow` | 普通释放 FA | 卡牌默认效果 |
| `LinkRecipes` | 连携卡填写 | 新连携配方数组 |
| `DefaultLinkOrientation` | 默认 `Forward` | 反向连携测试时可改为 `Reversed` |
| `bRequiresComboFinisher` | 默认不勾 | 高级字段；终结技卡后续再系统整理 |
| `DisplayName` | 卡牌名 | HUD 和信息卡显示 |
| `HUDReasonText` | 简短说明 | 连携提示 fallback 文案 |

旧字段 `CardTags / MatchedFlow / LinkMode / LinkConfig` 已从 DA 编辑界面隐藏，只保留给历史资源兼容。

## 示例

普通攻击提升卡：

| 字段 | 值 |
| --- | --- |
| `CardType` | `Normal` |
| `CardIdTag` | `Card.ID.AttackUp` |
| `CardEffectTags` | `Card.Effect.Attack`, `Card.Effect.Buff.AttackUp` |
| `BaseFlow` | `FA_Rune_AttackUp_01` |
| `LinkRecipes` | 空 |

月光连携卡：

| 字段 | 值 |
| --- | --- |
| `CardType` | `Link` |
| `CardIdTag` | `Card.ID.Moonlight` |
| `CardEffectTags` | `Card.Effect.Moonlight` |
| `BaseFlow` | `FA_Moonlight_Base` |
| `LinkRecipes` | 配正向、反向两条配方 |

## 验收

1. 武器 `InitialCombatDeck` 中加入卡牌 DA。
2. HUD 卡组条能显示卡牌。
3. 普通卡命中 AN 时执行 `BaseFlow`。
4. 连携卡按 `LinkRecipes` 命中目标 Tag 后执行对应 `LinkFlow`。
