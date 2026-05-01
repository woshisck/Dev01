# 战斗卡牌 Pending Commit 后续开发方案

## Summary

本方案补充当前战斗卡牌与动作系统的后续开发方向。当前测试阶段可以先用 `Combat Card -> Base Flow` 和短持续时间属性加成验证基础链路；正式版本需要让卡牌消耗与攻击动作命中帧对齐。

目标是避免“玩家按下攻击但攻击被取消时，卡牌已经被消耗”的问题。

本阶段暂不开发，先作为后续任务记录。

## 核心规则

卡牌不在攻击输入时立即消耗，而是分成三个阶段：

| 阶段 | 触发时机 | 卡牌是否消耗 | UI 是否移除卡牌 |
| --- | --- | --- | --- |
| Prepare | 玩家攻击输入、GA Commit、蒙太奇开始 | 否 | 否，可高亮当前卡 |
| Commit | 攻击 AN 命中帧确认打出攻击 | 是 | 是 |
| Cancel | 蒙太奇中断、冲刺取消、受击打断、未到攻击 AN 帧 | 否 | 否，取消高亮 |

## 目标流程

```text
玩家攻击输入
  -> 动作系统开始攻击动作
  -> 卡组系统 Prepare 当前卡牌
  -> FA / 卡牌效果登记 Pending 效果
  -> UI 高亮当前卡，但不移除

攻击 AN_MeleeDamage 到达并确认攻击成功
  -> 动作系统通知 Commit
  -> Pending 效果被消耗并参与本次攻击
  -> 卡组系统正式消耗卡牌
  -> UI 移除当前卡
  -> 卡牌效果结束或进入后续生命周期

攻击在 AN 前被取消
  -> 动作系统通知 Cancel
  -> Pending 效果被清理
  -> 卡牌不消耗
  -> UI 不移除当前卡
```

## 推荐接口

后续建议 `CombatDeckComponent` 增加以下接口：

```cpp
FCombatCardResolveResult PrepareAttackCard(const FCombatDeckActionContext& Context);
FCombatCardResolveResult CommitPreparedAttackCard(const FGuid& AttackGuid);
void CancelPreparedAttackCard(const FGuid& AttackGuid, ECombatCardCancelReason Reason);
```

建议新增取消原因：

```cpp
UENUM(BlueprintType)
enum class ECombatCardCancelReason : uint8
{
    MontageInterrupted,
    DashCancelled,
    HitInterrupted,
    ComboExited,
    AttackWindowMissed,
    WeaponChanged
};
```

## Pending 效果

FA 中后续可以增加类似节点：

```text
Apply Next Attack Modifier
```

节点只负责登记效果，不独立决定生命周期。

| 字段 | 用途 |
| --- | --- |
| `Attribute` | 影响属性，例如 `BaseAttributeSet.Attack` |
| `Mod Op` | `Additive` / `Multiplicative` / `Override` |
| `Value` | 数值，例如 `10` |
| `Consume Count` | 消耗次数，攻击卡通常为 `1` |
| `Consume Timing` | 建议 `OnAttackDamageAN` |
| `Expire Policy` | `AttackEnd` / `ComboExit` / `MontageInterrupted` |
| `Target` | 通常为 `BuffOwner` |

示例：

```text
Start
  -> Apply Next Attack Modifier
       Attribute = BaseAttributeSet.Attack
       Mod Op = Additive
       Value = 10
       Consume Count = 1
       Consume Timing = OnAttackDamageAN
       Expire Policy = AttackEnd
       Target = BuffOwner
```

## 系统职责

| 系统 | 职责 |
| --- | --- |
| 动作系统 | 判断攻击开始、攻击 AN 到达、蒙太奇中断、冲刺取消、受击打断 |
| 卡组系统 | 管理 PreparedCard、Commit 后推进 CurrentIndex、Cancel 后保留卡牌 |
| FA / BuffFlow | 登记下一次攻击效果，并在 Commit 或 Cancel 时消耗/清理 |
| UI | Prepare 时高亮，Commit 时移除，Cancel 时恢复 |

## UI 规则

- Prepare 阶段只做高亮或预备提示。
- Commit 阶段才移除卡牌并推进卡组显示。
- Cancel 阶段取消高亮，当前卡保留。
- 洗牌只应在 Commit 消耗最后一张卡后触发。

## 测试计划

| 场景 | 预期 |
| --- | --- |
| 攻击输入后到达命中 AN | 卡牌 Commit，UI 移除，效果参与本次攻击 |
| 攻击输入后被冲刺取消 | Pending 清理，卡牌不消耗，UI 不移除 |
| 攻击输入后受击打断 | Pending 清理，卡牌不消耗，UI 不移除 |
| 攻击没有命中目标但到达攻击 AN | 需要按设计确认是否 Commit；建议“攻击打出即 Commit”，命中效果另算 |
| 最后一张卡 Commit | UI 移除后进入洗牌 |
| 最后一张卡 Cancel | 不进入洗牌 |

## 当前临时测试方案

当前先不开发 Pending Commit。普通攻击符文可以临时使用：

```text
Combat Card -> Base Flow
  Apply Attribute Modifier
    Attribute = BaseAttributeSet.Attack
    Mod Op = Additive
    Value = 10
    Duration Type = Has Duration
    Duration = 0.5
    Target = BuffOwner
```

该方案只用于验证 Combat Card 能触发 FA、FA 能修改攻击属性、伤害结果能变化。它不是最终正式逻辑。

## Assumptions

- 后续动作系统可以提供稳定的 `AttackGuid`。
- `AN_MeleeDamage` 或等价攻击 AN 能在伤害计算前通知卡组系统 Commit。
- 蒙太奇中断、冲刺取消、受击打断能通知卡组系统 Cancel。
- 卡牌 UI 消耗时机以 Commit 为准，不以攻击输入为准。
