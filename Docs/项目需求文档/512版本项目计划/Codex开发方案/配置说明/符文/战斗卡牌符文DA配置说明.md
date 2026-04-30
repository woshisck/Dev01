# 战斗卡牌符文 DA 配置说明

## 快速入口

如果只是要在引擎里配置当前版本可用的符文卡牌，优先阅读：

`符文系统引擎配置操作指南.md`

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
| `CardType` | `Attack` / `Link` / `Passive`，本阶段暂不扩展 `Finisher` | 决定卡牌类型显示和连携判断 |
| `RequiredAction` | 普通卡建议填 `Any` | 旧字段；普通卡不再依赖 Light / Heavy 激活，后续只作为连携条件或兼容字段 |
| `BaseFlow` | 填一个 FlowAsset，可为空 | 卡牌正常释放效果；只要有效出牌就执行 |
| `MatchedFlow` | 暂不推荐新资产继续依赖 | 旧字段；不再作为普通卡动作匹配奖励，后续会拆分为正向连携、反向连携或断链 Flow |
| `LinkMode` | 旧资产可保留，新增 Link 卡等待新配置 | 旧字段；不能表达正向/反向/断链差异，后续会替换为更明确的 `LinkConfig` |
| `bRequiresComboFinisher` | 本阶段不处理 | 终结技后续阶段再重新整理 |
| `DisplayName` | HUD 上显示的卡名 | 留空也可，但不推荐 |
| `HUDReasonText` | 简短说明 | 预留给后续详细 HUD 提示 |

## 推荐配置示例
普通攻击卡：

- `bIsCombatCard = true`
- `CardType = Attack`
- `RequiredAction = Any`
- `BaseFlow = 普通释放效果`
- `MatchedFlow = 空`
- `bRequiresComboFinisher = false`

连携卡（月光示例，等待新 LinkConfig 字段落地）：

- `bIsCombatCard = true`
- `CardType = Link`
- `RequiredAction = Any`
- `BaseFlow = 月光普通释放效果`
- 正向连携：前一张为攻击牌时，月光触发效果 A
- 反向连携：后一张为攻击牌时，月光先正常释放，并让后一张攻击牌触发效果 B
- 断链释放：暂停连招或后一张不满足条件时，月光回落到普通释放效果或执行 BreakFlow

## 测试方式
1. 把该符文加入武器的 `InitialCombatDeck`。
2. 进入战斗房间，确认 HUD 卡组条出现该卡。
3. 用任意攻击命中敌人，确认普通卡被消耗并执行 `BaseFlow`。
4. 配置“攻击牌 -> 月光”，确认月光触发正向连携效果。
5. 配置“月光 -> 攻击牌”，确认月光先正常释放，后一张攻击牌获得反向赋能。
6. 在月光等待后一张时暂停连招，确认连携断链并回落到断链释放。
