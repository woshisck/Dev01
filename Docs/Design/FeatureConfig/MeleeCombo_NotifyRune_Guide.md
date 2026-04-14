# 蒙太奇命中符文配置指南

> 适用范围：在特定连段命中时触发一次性效果（击退、燃烧、减速等）  
> 适用人群：策划 + 程序  
> 相关文档：[BuffFlow 符文工作流](../BuffFlow/BuffFlow_RuneWorkflow.md) · [攻击伤害技术文档](../Systems/AttackDamage_Technical.md)  
> 更新：2026-04-14

---

## 功能说明

`AN_MeleeDamage` Notify 上有一个 `AdditionalRuneEffects` 数组，可填入 `NotifyRuneDataAsset`。当该 Notify 命中目标时，数组中的每个符文都会在**攻击者**（玩家或敌人）的 BuffFlowComponent 上临时启动一次 FA，执行完自动结束。

**执行上下文：**
- `BuffOwner` = 攻击者（发动攻击的角色）
- `BuffGiver` = 被命中的目标
- 效果结束后 FA 自动清理，无需手动停止

---

## 内置 GA 说明（无需创建蓝图）

以下 GA 已在 **C++ 构造函数**中设置好 `AbilityTriggers`，直接填 C++ 类即可，**不需要创建 Blueprint 子类**：

| GA 类（C++） | 监听事件 Tag | 激活效果 | 授予位置 |
|---|---|---|---|
| `GA_Knockback` | `Action.Knockback` | 物理击退（Root Motion MoveToForce） | `DA_Base_AbilitySet`（所有角色） |
| `GA_HitReaction` | `Action.HitReact` | 播放受击硬直蒙太奇 | `DA_Base_AbilitySet`（所有角色） |
| `GA_Dead` | `Action.Dead` | 播放死亡蒙太奇 + 销毁角色 | `DA_Base_AbilitySet`（所有角色） |

> ✅ 三个 GA 都已通过 `DA_Base_AbilitySet` 授予给所有角色。FA 只需发送对应的事件 Tag 即可直接触发，无需额外配置。

---

## 与背包符文的区别

| 对比项 | 背包常驻符文 | 蒙太奇命中符文（本文档） |
|---|---|---|
| 激活时机 | 装备时启动，持续存活 | 命中目标的瞬间启动，一次性 |
| FA 结构 | Start → OnDamageDealt → 效果 → 循环 | Start → 效果 → Finish |
| 数据资产类型 | `RuneDataAsset` | **`NotifyRuneDataAsset`** |
| 目标选择器 | `LastDamageTarget` | **`BuffGiver`**（被命中的目标） |

> ⚠️ **不要**把背包符文的 FA 直接填进 `AdditionalRuneEffects`。背包 FA 有 `OnDamageDealt` 等待节点，会导致"晚一段触发、之后每段都触发"的 Bug。

---

## 制作步骤

### Step 1：创建专用 Notify Flow Asset

右键 Content Browser → **Flow → Notify Flow Asset**，命名建议：`FA_Rune_<效果名>`

> 必须选 **Notify Flow Asset**（不是普通 Flow Asset），否则 NotifyRuneDataAsset 的字段过滤会拦截。

**FA 结构（固定）：**

```
[Start] → [Send Gameplay Event] → [Finish]
```

`Send Gameplay Event` 节点配置：

| 字段 | 填什么 |
|---|---|
| `EventTag` | 对应 GA 监听的事件（见上表），例如 `Action.Knockback` |
| `Target` | **`Buff施加者`（BuffGiver）**= 被命中的目标 |
| `Instigator` | `Buff拥有者`（BuffOwner）= 攻击者 |

> ⚠️ `Out` 和 `Failed` 管脚都要接到 `[Finish]`，FA 才能正常终止。不接会导致第二次命中无效。

### Step 2：创建专用 Notify Rune Data Asset

右键 Content Browser → **Miscellaneous → Data Asset → NotifyRuneDataAsset**，命名建议：`DA_NotifyRune_<效果名>`

**必填字段：**

| 字段 | 说明 |
|---|---|
| `FlowAsset` | 拖入 Step 1 创建的 Notify Flow Asset |

### Step 3：填入 Notify

1. 打开对应蒙太奇，找到目标连段的 `AN_MeleeDamage` Notify
2. 在 Details 面板找到 `AdditionalRuneEffects` 数组
3. 添加元素，拖入 Step 2 创建的 `DA_NotifyRune_XXX`

---

## 示例：第二段命中触发击退

**需求**：玩家双手剑第二段攻击命中时，将敌人击退。

**Step 1** — 新建 `FA_Rune_Knockback`（Notify Flow Asset）：
```
[Start]
  ↓
[Send Gameplay Event]
    EventTag   = Action.Knockback
    Target     = Buff施加者 (BuffGiver = 敌人)
    Instigator = Buff拥有者 (BuffOwner = 攻击者)
  ↓ Out          ↓ Failed
[Finish]       [Finish]
```

**Step 2** — 新建 `DA_NotifyRune_Knockback`（NotifyRuneDataAsset）：
- `FlowAsset` = `FA_Rune_Knockback`

**Step 3** — 在蒙太奇第二段 `AN_MeleeDamage` Notify：
- `AdditionalRuneEffects[0]` = `DA_NotifyRune_Knockback`

**效果链**：
```
AN_MeleeDamage 命中
  → ReceiveOnHitRune（C++）
    → 攻击者 BFC.StartBuffFlow(FA_Rune_Knockback, BuffGiver=敌人)
      → Send Gameplay Event(Action.Knockback → 敌人 ASC)
        → GA_Knockback 激活，敌人被击退 ✓
      → [Finish]（FA 终止，下次命中可再次触发）
```

---

## 跨符文通信：击退后减速（背包符文联动）

当 Notify 符文触发击退，同时希望**背包常驻符文**感知到"击退发生了"并施加减速：

**关键**：Notify FA 发送两个事件，背包符文 FA 监听第二个：

```
[Start]
  ↓
[Send Gameplay Event]               ← 第一个：触发 GA_Knockback
    EventTag = Action.Knockback
    Target   = Buff施加者 (敌人)
  ↓ Out
[Send Gameplay Event]               ← 第二个：通知背包符文
    EventTag = Action.Rune.KnockbackApplied
    Target   = Buff拥有者 (玩家)
  ↓ Out         ↓ Failed
[Finish]      [Finish]
```

背包减速符文 FA：
```
[Start]
  ↓
[Wait Gameplay Event]
    EventTag = Action.Rune.KnockbackApplied
    Target   = Buff拥有者 (玩家)
  ↓
[Apply Attribute Modifier]
    Attribute = MoveSpeed, Add -300, 持续 3s
    Target    = Buff施加者 (敌人)
  ↓ Out
(循环回 Wait)
```

> 两个 FA 都在**玩家的 BFC** 上运行：Notify FA 启动时写入 `CurrentBuffGiver=敌人`，背包 FA 的 `Apply` 节点通过 `Buff施加者` 读到同一个敌人引用。

---

## Debug

编辑器日志中搜索 `[ReceiveOnHitRune]`，每次命中触发时输出：

```
[ReceiveOnHitRune] StartBuffFlow | Rune=DA_NotifyRune_Knockback | FlowAsset=FA_Rune_Knockback
  | Attacker(BuffOwner)=BP_PlayerCharacter_C_0 | HitTarget(BuffGiver)=B_EnemyDummy_C_0
```

**常见问题：**

| 现象 | 原因 | 解决 |
|---|---|---|
| Log 没有输出 | Notify 的 `AdditionalRuneEffects` 未填或没命中 | 检查命中框配置 |
| 第二次命中无效 | FA 的 Out/Failed 没连 `[Finish]` | 两个管脚都接 `[Finish]` |
| 晚一段才触发 | 用了背包符文的 FA（有 OnDamageDealt 等待） | 换用 Notify Flow Asset |
| 减速没生效 | Wait 监听 Tag 与 Send 发出 Tag 不一致 | 确认两边都用 `Action.Rune.KnockbackApplied` |
| `BuffGiver` 解析失败 | Target 填错为 `Buff拥有者` | 改成 `Buff施加者` |

---

## 技术背景（程序参考）

核心入口：`YogCharacterBase::ReceiveOnHitRune_Implementation`（`YogCharacterBase.cpp`）

```
GA_MeleeAttack::OnEventReceived
  → HitChar->ReceiveOnHitRune(RuneDA, AttackOwner)
    → AttackOwner->FindComponentByClass<UBuffFlowComponent>()
    → BFC->StartBuffFlow(FlowAsset, FGuid::NewGuid(), HitChar)
          ↑ 同时写入 CurrentBuffGiver = HitChar（敌人）
```

GA 触发机制（C++ 构造函数已配置，无需 Blueprint）：

```cpp
// GA_Knockback.cpp — 构造函数
FAbilityTriggerData TriggerData;
TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"));
TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
AbilityTriggers.Add(TriggerData);
// GA_HitReaction、GA_Dead 同理，分别监听 Action.HitReact / Action.Dead
```
