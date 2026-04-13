# 蒙太奇命中符文配置指南

> 适用范围：在特定连段命中时触发一次性效果（击退、燃烧、减速等）  
> 适用人群：策划 + 程序  
> 相关文档：[BuffFlow 符文工作流](../BuffFlow/BuffFlow_RuneWorkflow.md) · [攻击伤害技术文档](../Systems/AttackDamage_Technical.md)  
> 更新：2026-04-14

---

## 功能说明

`AN_MeleeDamage` Notify 上有一个 `AdditionalRuneEffects` 数组，可填入 `RuneDataAsset`。当该 Notify 命中目标时，数组中的每个符文都会在**攻击者**（玩家或敌人）的 BuffFlowComponent 上临时启动一次 FA，执行完自动结束。

**执行上下文：**
- `BuffOwner` = 攻击者（发动攻击的角色）
- `BuffGiver` = 被命中的目标
- 效果结束后 FA 自动清理，无需手动停止

---

## 与背包符文的区别

| 对比项 | 背包常驻符文 | 蒙太奇命中符文（本文档） |
|---|---|---|
| 激活时机 | 装备时启动，持续存活 | 命中目标的瞬间启动，一次性 |
| FA 结构 | Start → OnDamageDealt → 效果 → 循环 | Start → 效果 → Finish |
| 目标选择器 | `LastDamageTarget` | **`BuffGiver`**（被命中的目标） |
| 能否复用同一个 FA | ❌ 不能 | — |

> ⚠️ **不要**把背包符文的 FA 直接填进 `AdditionalRuneEffects`。背包 FA 有 `OnDamageDealt` 等待节点，会导致"晚一段触发、之后每段都触发"的 Bug。

---

## 制作步骤

### Step 1：创建专用 FlowAsset

新建 FlowAsset，命名建议：`FA_HitEffect_<效果名>`（区别于背包符文的 `FA_Rune_<名>`）

**路径**：`Content/Game/Runes/HitEffects/` 或对应符文目录

**FA 结构（固定）：**

```
[Start] → [Send Gameplay Event] → [Finish]
```

`Send Gameplay Event` 节点配置：

| 字段 | 填什么 |
|---|---|
| `EventTag` | 要触发的事件，例如 `Action.Knockback` |
| `Target` | **`BuffGiver`**（= 被命中的目标） |
| `Instigator` | `BuffOwner`（= 攻击者） |

### Step 2：创建专用 RuneDataAsset

新建 RuneDataAsset，命名建议：`DA_HitRune_<效果名>`

**必填字段：**

| 字段 | 说明 |
|---|---|
| `RuneConfig.RuneName` | 随意，仅用于调试识别 |
| `Shape.Cells` | 填 `(0,0)` 即可（不会出现在背包格子里） |
| `Flow.FlowAsset` | 拖入 Step 1 创建的 FA |

### Step 3：填入 Notify

1. 打开对应蒙太奇，找到目标连段的 `AN_MeleeDamage` Notify
2. 在 Details 面板找到 `AdditionalRuneEffects` 数组
3. 添加元素，拖入 Step 2 创建的 `DA_HitRune_XXX`

---

## 示例：第二段命中触发击退

**需求**：玩家双手剑第二段攻击命中时，将敌人击退。

1. 新建 `FA_HitEffect_Knockback`：
   ```
   [Start] → [Send Gameplay Event]
               EventTag   = Action.Knockback
               Target     = BuffGiver
               Instigator = BuffOwner
             → [Finish]
   ```

2. 新建 `DA_HitRune_Knockback`，FlowAsset = `FA_HitEffect_Knockback`

3. 在 `AM_Player_TwoHandSword_Combo` 的第二段 `AN_MeleeDamage` Notify：
   - `AdditionalRuneEffects[0]` = `DA_HitRune_Knockback`

**效果链**：
```
AN_MeleeDamage 命中
  → ReceiveOnHitRune（C++）
    → 攻击者 BFC.StartBuffFlow(FA_HitEffect_Knockback)
      BuffOwner = 玩家，BuffGiver = 敌人
      → Send Gameplay Event(Action.Knockback → BuffGiver=敌人)
        → GA_Knockback 激活，敌人被击退
```

---

## Debug

编辑器日志中搜索 `[ReceiveOnHitRune]`，每次命中触发时输出：

```
[ReceiveOnHitRune] StartBuffFlow | Rune=DA_HitRune_Knockback | FlowAsset=FA_HitEffect_Knockback
  | Attacker(BuffOwner)=BP_PlayerCharacter_C_0 | HitTarget(BuffGiver)=B_EnemyDummy_C_0
```

**常见问题：**

| 现象 | 原因 | 解决 |
|---|---|---|
| Log 没有输出 | Notify 的 `AdditionalRuneEffects` 未填或没命中 | 检查命中框配置 |
| 晚一段才触发 | 用了背包符文的 FA（有 OnDamageDealt 等待） | 换用专用一次性 FA |
| 之后每段都触发 | 同上，FA 启动后持续监听了新的 Damage 事件 | 同上 |
| `BuffGiver` 解析失败 | Target 填错为 `LastDamageTarget` | 改成 `BuffGiver` |
| 攻击者 BFC 为 null | 攻击者（玩家/敌人）没有 BuffFlowComponent | 检查角色蓝图是否挂载 BuffFlowComponent |

---

## 技术背景（程序参考）

核心入口：`YogCharacterBase::ReceiveOnHitRune_Implementation`（`YogCharacterBase.cpp`）

```
GA_MeleeAttack::OnEventReceived
  → HitChar->ReceiveOnHitRune(RuneDA, AttackOwner)
    → AttackOwner->FindComponentByClass<UBuffFlowComponent>()
    → BFC->StartBuffFlow(FlowAsset, FGuid::NewGuid(), this /* BuffGiver=HitChar */)
```

`BuffGiver` 在 `StartBuffFlow` 时通过 `Giver` 参数写入 `BFC->CurrentBuffGiver`，FA 内所有节点使用 `BuffGiver` 选择器时都读取这个值。
