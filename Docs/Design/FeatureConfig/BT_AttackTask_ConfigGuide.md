# 行为树攻击任务配置指南

**功能模块**：`UBTTask_ActivateAbilityByTag` + `UYogAbilitySystemComponent`  
**适用对象**：所有使用行为树驱动攻击的敌人  
**状态**：已实现，可直接在编辑器使用

---

## 概述

`BTTask_ActivateAbilityByTag` 是一个行为树 Task 节点，用于：

1. 根据 Tag 激活匹配的 GameplayAbility（支持随机选择）
2. **阻塞等待** GA 执行完毕（蒙太奇播放结束 / GA 主动 EndAbility）
3. GA 结束后返回 `Succeeded`，BT 继续执行后续节点

Task 完全异步，不占用 BT 的主线程，适合与蒙太奇播放 GA 配合使用。

---

## Task 参数

| 参数 | 类型 | 说明 |
|---|---|---|
| `AbilityTags` | `FGameplayTagContainer` | 激活条件：匹配任意一个 Tag 的 GA 将被激活 |

---

## 基础用法：单一攻击

### 场景
敌人只有一种攻击，行为树直接调用该攻击 GA。

### BT 配置

```
Sequence
  ├─ BTTask_MoveTo（靠近目标）
  └─ BTTask_ActivateAbilityByTag
       └─ AbilityTags: [Enemy.Atk.Slam]
```

### GA 配置
在 `AbilityData` 的 `AbilityMap` 中，Key 填 `Enemy.Atk.Slam`，Montage 填攻击动画。

---

## 进阶用法：随机多种攻击

### 场景
敌人有多种攻击，希望随机选一个。

### 原理
`TryActivateRandomAbilitiesByTag` 支持父 Tag 匹配子 Tag 的层级结构。  
例如，填父 Tag `Enemy.Melee`，会从所有已注册的 `Enemy.Melee.Slam`、`Enemy.Melee.Sweep`、`Enemy.Melee.Combo` 中随机激活一个。

### 配置步骤

**1. Tag 命名使用层级结构**

```
Enemy.Melee
  ├─ Enemy.Melee.Slam
  ├─ Enemy.Melee.Sweep
  └─ Enemy.Melee.Combo
```

**2. 在 AbilityData 里分别配置每种攻击**

| AbilityMap Key | Montage | 说明 |
|---|---|---|
| `Enemy.Melee.Slam` | `AM_Slam` | 重击 |
| `Enemy.Melee.Sweep` | `AM_Sweep` | 横扫 |
| `Enemy.Melee.Combo` | `AM_Combo3Hit` | 三段连击 |

**3. BT Task 填父 Tag**

```
BTTask_ActivateAbilityByTag
  └─ AbilityTags: [Enemy.Melee]   ← 填父 Tag，自动随机选子级 GA
```

> 如果某个子 GA 当前无法激活（例如 CD 中、被 Block），会自动跳过选其他的。

---

## 进阶用法：条件选择不同攻击

### 场景
根据与目标的距离、血量、状态等条件选择不同攻击。

### 方案：BT Selector + Decorator

```
Selector
  ├─ [Decorator: 距离 ≤ 200] BTTask_ActivateAbilityByTag (Tag: Enemy.Atk.Close)
  ├─ [Decorator: 距离 ≤ 600] BTTask_ActivateAbilityByTag (Tag: Enemy.Atk.Mid)
  └─ BTTask_ActivateAbilityByTag (Tag: Enemy.Atk.Ranged)     ← 兜底，无 Decorator
```

Selector 从上往下尝试，第一个满足 Decorator 条件的 Task 会被执行。  
`Enemy.Atk.Close` 也可以是父 Tag，Task 内部会随机选一种近战攻击。

### Decorator 推荐类型

| Decorator | 用途 |
|---|---|
| `Blackboard: Distance To Target ≤ X` | 距离判断（需在 BT Service 中持续更新距离值到黑板） |
| `Blackboard: Health Percent ≤ X` | 血量判断（激活愤怒技/收尾技等） |
| `Cooldown` | 限制特定攻击的触发频率 |
| `GameplayTag Condition` | 检查 ASC 是否持有某个 Tag（如充能完毕时才使用重招） |

---

## 攻击距离判断

### 问题
Task 本身不判断距离，需要在 BT 外层提前判断"是否进入攻击范围"。

### 推荐结构

```
Sequence（攻击序列）
  ├─ BTDecorator: 距离 ≤ 攻击范围（条件不满足则整个 Sequence 失败）
  ├─ BTTask_ActivateAbilityByTag (攻击 Tag)
  └─ BTTask_Wait（攻击冷却 CD）
```

攻击范围数据建议从 `AbilityData.AbilityMap[Tag].ActRange` 中读取，由 BT Service 计算并写入黑板。

---

## 注意事项

| 情况 | 行为 |
|---|---|
| Tag 匹配到多个 GA | 随机选一个激活 |
| 没有匹配到任何 GA | Task 返回 `Failed`，BT 执行失败分支 |
| GA 在执行过程中被外部打断（死亡等） | `OnAbilityEnded` 委托触发，Task 返回 `Succeeded`（GA 已结束） |
| 同一 GA 还在执行中又被激活 | 取决于 GA 的 `InstancingPolicy` 和 `ReActivationPolicy`，通常会被阻断或重置 |

---

## 配套说明：AbilityData 中的攻击数据字段

| 字段 | 说明 |
|---|---|
| `Montage` | 攻击蒙太奇 |
| `ActDamage` | 基础伤害 |
| `ActRange` | 攻击范围（用于 BT 距离判断） |
| `ActResilience` | 韧性伤害（对目标的受击打断能力） |
| `ActRotateSpeed` | 攻击时追踪旋转速度 |
| `UniqueEffects` | 触发特殊 GE 的配置（对自身或目标施加效果） |
| `hitboxTypes` | 打击判定形状配置 |

---

## 常见问题

**Q：Task 配置好了但 GA 没有激活？**  
A：检查以下几点：
1. 角色 ASC 上是否已 GiveAbility 了对应 GA
2. AbilityData 中该 Tag 的 GA 是否已正确注册
3. Tag 名称是否完全一致（父子 Tag 层级关系正确）
4. GA 是否有 `ActivationBlockedTags` 阻止了激活

**Q：Task 激活了 GA，但 BT 没有等待 GA 结束就继续了？**  
A：确认 GA 的 `InstancingPolicy` 为 `InstancedPerActor`（默认为 `NonInstanced`，NonInstanced 时 `OnAbilityEnded` 可能行为不同）。GA_Dead 和攻击 GA 都应使用 `InstancedPerActor`。

**Q：想要攻击 GA 结束前行为树一直等待，攻击结束后才让 AI 继续移动？**  
A：这正是 Task 的默认行为：激活 GA → 返回 `InProgress`（BT 阻塞）→ GA 的 `EndAbility` 调用时委托触发 → Task 返回 `Succeeded` → BT 继续。
