# 终结技卡牌 — 编辑器配置指南

> 关联代码：`GA_FinisherCharge.cpp` · `GA_Player_FinisherAttack.cpp` · `AN_TriggerFinisherAbility.cpp`  
> 适用版本：本文档对应 v3 FA驱动架构  
> 最后更新：2026-05-09

---

## 一、DA_Rune_Finisher — 数值表

在符文编辑器（Tools → DevKit Data → Rune Editor）中，选中 `DA_Rune_Finisher`，切换到**数值表**选项卡，添加以下行：

| Key | 默认值 | 说明 |
|-----|--------|------|
| `DetonationDamage` | `80.0` | 引爆基础伤害（未确认时 ×1.0，确认时 ×2.0） |
| `KnockbackDistance` | `400.0` | 充能命中时的击退距离（单位：cm） |

> 这两个值在 `FA_Finisher_ChargeHit` 和 `FA_Finisher_Detonate` 中通过 `BFNode_GetRuneTuningValue` 读取，之后可以随时在这里调平衡。

---

## 二、BGA_FinisherCharge — Blueprint 配置

打开 `BGA_FinisherCharge`（继承自 `GA_FinisherCharge`），在 Class Defaults 里配置：

| 字段 | 值 |
|------|-----|
| **Ability Tags** | `PlayerState.AbilityCast.FinisherCharge` |
| **Activation Blocked Tags** | `PlayerState.AbilityCast.FinisherCharge` |
| **Max Charges** | `5` |
| **Finisher Charge GE Class** | `GE_FinisherCharge` |

> `ActivationBlockedTags` 与 `AbilityTags` 相同，防止同一时间激活两个实例。

---

## 三、BGA_Player_FinisherAttack — Blueprint 配置

打开 `BGA_Player_FinisherAttack`（继承自 `GA_Player_FinisherAttack`），配置：

| 字段 | 值 |
|------|-----|
| **Ability Tags** | `PlayerState.AbilityCast.Finisher` |
| **Activation Owned Tags** | `Buff.Status.FinisherExecuting` |
| **Activation Required Tags** | `Buff.Status.FinisherWindowOpen` |
| **Activation Blocked Tags** | `Buff.Status.Dead` · `Buff.Status.FinisherExecuting` |
| **Cancel Abilities With Tag** | `PlayerState.AbilityCast.LightAtk` · `PlayerState.AbilityCast.HeavyAtk` · `PlayerState.AbilityCast.Dash` |
| **Finisher Montage** | `AM_Player_FinisherAttack` |
| **Confirmed Damage Multiplier** | `2.0` |

> **重要：** `CancelAbilitiesWithTag` 中**不要**填 `PlayerState.AbilityCast.FinisherCharge`，否则终结技激活时会把充能窗口 GA 也取消掉。  
> `ActivationRequiredTags = Buff.Status.FinisherWindowOpen`（不是 `FinisherCharge`）—— 这是为了让最后一层命中消耗GE后，窗口Tag仍存在，终结技才能正常触发。

---

## 四、GE_FinisherCharge — GE 配置

| 字段 | 值 |
|------|-----|
| **Duration Policy** | Has Duration |
| **Duration** | `8.0` 秒 |
| **Stacking: Stack Limit Count** | `5` |
| **Stacking: Stacking Type** | Aggregate by Target |
| **Stacking: Duration Refresh Policy** | Never Refresh |
| **Stacking: Period Reset Policy** | Never Reset |
| **Granted Tags** | `Buff.Status.FinisherCharge` |

> `Never Refresh` 保证叠加新层时8秒计时**不重置**。

---

## 五、FA_FinisherCard_BaseEffect — BaseFlow 节点连线

这是卡牌打出时执行**一次**的基础效果流程。

```
[Start]
  │
  ▼
[BFNode_ApplyEffect]
    Effect = GE_FinisherCharge
    Target = BuffOwner（玩家）
    StacksToApply = 5
  │ Out
  ▼
[BFNode_SendGameplayEvent]
    EventTag = Action.FinisherCharge.Activate
    Target   = BuffOwner（玩家）
  │ Out
  ▼
[BFNode_FinishBuff]
```

**注意：** GA_FinisherCharge 在收到 `Activate` 事件后激活，随即开始监听 `Ability.Event.Attack.Hit`。打出卡牌时的那次命中（触发卡牌的攻击）本身也会作为第1次充能消耗，不需要在 BaseFlow 里额外处理。

---

## 六、FA_Finisher_ChargeHit — 节点连线

**类型：** Category C（绑定 GE_FinisherCharge 生命周期，GE消失时自动停止）  
**职责：** 每次充能命中 → 击退被命中敌人 + 施加印记

```
[WaitGameplayEvent]                         ← 循环节点
    EventTag = Action.FinisherCharge.ChargeConsumed
    Target   = BuffOwner（玩家 ASC 上监听）
    Loop     = true
  │ Out（每次事件触发后往下执行）
  │
  │  EventData.Target 自动写入 BFC.LastEventContext（即被命中的敌人）
  │
  ▼
[BFNode_GetRuneTuningValue]
    Key          = KnockbackDistance
    DefaultValue = 400.0
  │ Value（float输出引脚，连到下方）
  ▼
[BFNode_SendGameplayEvent]
    EventTag   = Action.Knockback
    Target     = LastDamageTarget       ← 来自 LastEventContext（被命中敌人）
    Instigator = DamageCauser
    Magnitude  = ↑ KnockbackDistance   ← 连线上方的 Value 输出
  │ Out
  ▼
[BFNode_SendGameplayEvent]
    EventTag   = Action.ApplyFinisherMark
    Target     = LastDamageTarget
    Instigator = DamageCauser
  │
  └── （不连 FinishBuff，GE_FinisherCharge 消失时 Category C FA 自动停止）
```

> **Target 选择说明：**  
> - `BFNode_SendGameplayEvent` 的 Target 选 `LastDamageTarget`（即 `BFC.LastEventContext`），不是 `BuffOwner`  
> - `BFNode_WaitGameplayEvent` 收到事件时会把 `EventData.Target`（被命中敌人）写入 `BFC.LastEventContext`

---

## 七、FA_Finisher_Detonate — 节点连线

**类型：** Category C（绑定 GE_FinisherCharge 生命周期，GE消失时自动停止）  
**职责：** 引爆每个印记目标 → 读数值表 → 伤害计算 → 割裂 → 条件击退

```
[WaitGameplayEvent]                         ← 循环节点
    EventTag = Action.FinisherAttack.DetonateTarget
    Target   = BuffOwner（玩家 ASC 上监听）
    Loop     = true
  │ Out
  │   EventData.Target   → BFC.LastEventContext（被引爆的敌人）
  │   EventData.Magnitude → 确认倍率（1.0=未确认 / 2.0=已确认）
  │
  ▼
[BFNode_GetRuneTuningValue]
    Key          = DetonationDamage
    DefaultValue = 80.0
  │ Value（基础伤害）
  ▼
[BFNode_MathFloat]
    操作 = ×（乘法）
    A    = ↑ Value（基础伤害）
    B    = EventMagnitude（WaitGameplayEvent 输出的确认倍率）
  │ Result（最终伤害值）
  ▼
[BFNode_ApplyEffect]
    Effect            = GE_FinisherDamage
    Target            = LastDamageTarget
    SetByCallerTag    = Buff.Data.Damage          ← 或项目使用的伤害 SetByCaller Key
    SetByCallerValue  = ↑ Result（连线）
  │ Out
  ▼
[BFNode_SendGameplayEvent]
    EventTag   = Action.Slash
    Target     = LastDamageTarget
    Instigator = DamageCauser
  │ Out
  ▼
[BFNode_CompareFloat]                       ← 判断是否为确认路径
    A    = EventMagnitude
    B    = 2.0
    Mode = ≥（大于等于）
  │ True                   │ False
  ▼                         ▼
[BFNode_SendGameplayEvent]   （无节点，跳过击退）
    EventTag   = Action.Knockback
    Target     = LastDamageTarget
    Instigator = DamageCauser
  │
  └── （两条路径都不连 FinishBuff，Category C FA 随 GE 自动停止）
```

> **印记移除：** `Action.ApplyFinisherMark` 对应的 GA 或 GE 在引爆时应自动移除（由 `Action.Slash` 触发的效果里处理），或在此 FA 里额外加一个 `BFNode_RemoveTag / RemoveEffect`。  
> 如果项目里印记是通过 `GE_FinisherMark` 授予的 Tag，可以在 `Action.Slash` 之后加：
> ```
> [BFNode_ApplyEffect]
>     Effect = GE_Remove_FinisherMark      ← 一个 Instant GE，移除 FinisherMark
>     Target = LastDamageTarget
> ```

---

## 八、配置检查清单

完成上述配置后，逐项确认：

- [ ] `DA_Rune_Finisher` 数值表：`DetonationDamage` = 80，`KnockbackDistance` = 400
- [ ] `BGA_FinisherCharge`：`ActivationBlockedTags` 含 `PlayerState.AbilityCast.FinisherCharge`，`FinisherChargeGEClass` 指向 `GE_FinisherCharge`
- [ ] `BGA_Player_FinisherAttack`：`ActivationOwnedTags` 含 `Buff.Status.FinisherExecuting`，`ActivationRequiredTags` 是 `Buff.Status.FinisherWindowOpen`
- [ ] `BGA_Player_FinisherAttack`：`CancelAbilitiesWithTag` **不含** `FinisherCharge`
- [ ] `GE_FinisherCharge`：`GrantedTags = Buff.Status.FinisherCharge`，Stack = 5，`Never Refresh`
- [ ] `FA_Finisher_ChargeHit`：Category C，监听 `ChargeConsumed`，Target = LastDamageTarget
- [ ] `FA_Finisher_Detonate`：Category C，监听 `DetonateTarget`，包含 CompareFloat 条件分支
- [ ] `BGA_FinisherCharge` 和 `BGA_Player_FinisherAttack` 已预授予到玩家的 `DA_Base_AbilitySet`

---

## 九、事件流向总览（快速参考）

```
卡牌打出
  → BaseFlow: ApplyEffect(GE_FinisherCharge, 5层)
            + SendEvent(Action.FinisherCharge.Activate)
              → GA_FinisherCharge 激活，WaitHitTask 开始

每次攻击命中（Ability.Event.Attack.Hit）
  → GA_FinisherCharge: 派发 ChargeConsumed(Target=敌人) + 移除1层GE
    → FA_Finisher_ChargeHit: Knockback(敌人) + ApplyMark(敌人)

最后一层消耗完（Buff.Status.FinisherCharge Tag消失）
  → GA_FinisherCharge: 停止 WaitHitTask + 延迟1帧结束
  → 同帧：AN_TriggerFinisherAbility 检测到 Buff.Status.FinisherWindowOpen → 还在！
         → 派发 Action.Player.FinisherAttack
           → GA_Player_FinisherAttack 激活（FinisherExecuting Tag出现）
  → 下1帧：GA_FinisherCharge EndAbility（跳过 ClearAllMarks，因为 FinisherExecuting 存在）

终结技蒙太奇 HitFrame（Ability.Event.Finisher.HitFrame）
  → GA_Player_FinisherAttack: 遍历 FinisherMark 目标
    → 对每个目标派发 DetonateTarget(Target=敌人, Magnitude=1.0或2.0)
      → FA_Finisher_Detonate: 计算伤害 + ApplyGE + Slash + 条件Knockback
```
