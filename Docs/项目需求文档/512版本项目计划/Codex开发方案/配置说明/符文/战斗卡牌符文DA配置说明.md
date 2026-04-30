# 战斗卡牌符文 DA 配置说明

## 作用
把一个 `RuneDataAsset` 标记为“可进入战斗卡组的卡牌”。只有开启 Combat Card 的符文，才会被武器初始卡组、三选一奖励、HUD 卡组条识别。

## 配置位置
打开目标 `RuneDataAsset`，找到：

`RuneInfo -> CombatCard`

同时建议填写：

`RuneInfo -> RuneConfig -> RuneName / RuneIcon / RuneDescription`

HUD 卡槽会优先显示 `CombatCard.DisplayName`，没有填写时会回退到符文资产名称；图标来自 `RuneConfig.RuneIcon`。

## 字段说明
| 字段 | 怎么填 | 影响 |
| --- | --- | --- |
| `bIsCombatCard` | 勾选 | 勾选后该符文才能作为战斗卡牌进入卡组 |
| `CardType` | `Attack` / `Link` / `Finisher` / `Passive` | 决定卡牌类型显示和特殊触发逻辑 |
| `RequiredAction` | `Light` / `Heavy` / `Any` | 决定轻击、重击、任意攻击是否匹配 |
| `BaseFlow` | 填一个 FlowAsset，可为空 | 只要卡被消耗就执行，不要求动作匹配 |
| `MatchedFlow` | 填一个 FlowAsset，可为空 | 只有动作匹配时执行 |
| `LinkMode` | V1 通常填 `None` | 后续连携卡扩展使用 |
| `bRequiresComboFinisher` | 终结卡需要 Combo4 时勾选 | 只有 LightAtk4 / HeavyAtk4 命中时才触发 Finisher 额外逻辑 |
| `DisplayName` | HUD 上显示的卡名 | 留空也可，但不推荐 |
| `HUDReasonText` | 简短说明 | 预留给后续详细 HUD 提示 |

## 推荐配置示例
普通轻击卡：

- `bIsCombatCard = true`
- `CardType = Attack`
- `RequiredAction = Light`
- `BaseFlow = 基础命中效果`
- `MatchedFlow = 轻击匹配奖励效果`
- `bRequiresComboFinisher = false`

重击终结卡：

- `bIsCombatCard = true`
- `CardType = Finisher`
- `RequiredAction = Heavy`
- `MatchedFlow = 重击终结奖励效果`
- `bRequiresComboFinisher = true`

## 测试方式
1. 把该符文加入武器的 `InitialCombatDeck`。
2. 进入战斗房间，确认 HUD 卡组条出现该卡。
3. 用对应 Light / Heavy 命中敌人，确认卡被消耗。
4. 如果动作不匹配，卡仍会消耗，但只执行 `BaseFlow`。
