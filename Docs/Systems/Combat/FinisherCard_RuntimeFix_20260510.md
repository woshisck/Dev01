# 终结技卡牌运行时修正

## 结论

终结技卡牌的 `FA_FinisherCard_ChargeHit` 和 `FA_FinisherCard_Detonate` 不应只依赖背包符文的 Passive 触发逻辑。

战斗卡牌运行时现在由 `CombatDeckComponent` 负责启动和停止 `RuneInfo.Flow.PassiveFlows`：

- 卡牌进入 deck：启动该卡牌 DA 上的 `RuneInfo.Flow.PassiveFlows`
- 卡牌从 deck 移除或更换武器/deck：停止这些 PassiveFlows
- 卡牌 BaseFlow 仍只负责“打出卡牌时的一次性效果”

## DA_Rune_Finisher 必填

`DA_Rune_Finisher` 需要保持以下配置：

- `RuneInfo.CombatCard.CardType = Finisher`
- `RuneInfo.CombatCard.TriggerTiming = OnHit`
- `RuneInfo.CombatCard.BaseFlow = FA_FinisherCard_BaseEffect`
- `RuneInfo.Flow.PassiveFlows[0] = FA_FinisherCard_ChargeHit`
- `RuneInfo.Flow.PassiveFlows[1] = FA_FinisherCard_Detonate`

## 事件链

`FA_FinisherCard_BaseEffect`：

- 施加 `GE_FinisherCharge`
- 发送 `Action.FinisherCharge.Activate`

`FA_FinisherCard_ChargeHit`：

- 等待 `Action.FinisherCharge.ChargeConsumed`
- 对 `LastDamageTarget` 发送 `Action.Knockback`
- 对 `LastDamageTarget` 发送 `Action.Mark.Apply.Finisher`

`FA_FinisherCard_Detonate`：

- 等待 `Action.Mark.Detonate.Finisher`
- 对 `LastDamageTarget` 造成引爆伤害
- 发送割裂事件
- 确认输入成功时额外发送 `Action.Knockback`
