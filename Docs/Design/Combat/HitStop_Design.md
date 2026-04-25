# HitStop 命中停顿系统设计文档

> 最后更新：2026-04-26

---

## 一、系统目标

让玩家攻击命中时产生"冻结帧"或"延缓帧"效果，增强打击感。
支持两种触发方式：**AN 模式直接驱动**（在 `AN_MeleeDamage` 选择 HitStopMode）和 **Tag 驱动**（FA 写入 HitStop Tag）。两者可共存。

---

## 二、核心概念

### 2.1 两种效果

| 效果 | 技术实现 | 持续时长 | 结束行为 |
| --- | --- | --- | --- |
| **冻结帧（Freeze）** | 暂停玩家蒙太奇（`Montage_Pause`） | FrozenDuration（真实秒） | 恢复蒙太奇继续播放 |
| **延缓帧（Slow）** | 降低蒙太奇速率（`Montage_SetPlayRate`） | SlowDuration（真实秒） | 以 CatchUpRate 追帧，补偿慢放丢失的动画帧 |

> **注意**：新版**不影响全局时间膨胀**，仅操作玩家自身蒙太奇，敌人和场景不受影响。

### 2.2 执行顺序

```text
Frozen（暂停蒙太奇）
    ↓ FrozenDuration 真实秒后
Slow（减速蒙太奇到 SlowRate）
    ↓ SlowDuration 真实秒后
CatchUp（加速到 CatchUpRate，持续 = 丢失帧 / (CatchUpRate - 1)）
    ↓
恢复正常（PlayRate = 1.0）
```

任意阶段时长为 0 则跳过该阶段。

---

## 三、触发方式

### 3.1 AN 模式驱动（推荐，自包含）

在蒙太奇的 `AN_MeleeDamage` Notify 上直接选择 `HitStopMode`：

| 模式 | 效果 | 显示参数 |
| --- | --- | --- |
| `None` | 无 HitStop | 无额外参数 |
| `Freeze` | 冻结帧 | `HitStopFrozenDuration`（默认 0.06s） |
| `Slow` | 延缓帧 | `HitStopSlowDuration` / `HitStopSlowRate` / `HitStopCatchUpRate` |

AN 选择模式后，Notify 触发时参数暂存到角色 `PendingHitStopOverride`，`BFNode_HitStop` 执行时直接读取激活对应阶段，**无需 FA 写 Tag**。

### 3.2 Tag 驱动（特殊场景，如暴击 FA）

位于 `Config/Tags/BuffTag.ini`，挂在**攻击方（玩家）** ASC：

| Tag | 含义 |
| --- | --- |
| `Buff.Status.HitStop.Freeze` | 下次命中时触发冻结帧 |
| `Buff.Status.HitStop.Slow` | 下次命中时触发延缓帧 |

- 两个 Tag 可同时存在（先冻结再延缓）
- `BFNode_HitStop` 执行时**消费**这两个 Tag（`RemoveLooseGameplayTag`），保证单次命中只触发一次

### 3.3 共存规则

AN 模式与 Tag 可同时生效。`BFNode_HitStop` 的判断逻辑：

1. 先检查 FA Tag（`Buff.Status.HitStop.Freeze/Slow`）
2. 再检查 AN 模式（`PendingHitStopOverride.bActive`），AN 指定的模式直接激活对应阶段
3. AN 覆盖参数**只作用于 AN 指定的模式**，Tag 触发的模式使用 BFNode 节点默认值

---

## 四、FA 接线方案

### 4.1 基础命中停顿 FA（FA_HitStop）

适用于所有攻击普通命中后触发：

```text
[On Damage Dealt]
      ↓
[BFNode_HitStop]
  FrozenDuration = 0.06
  SlowDuration   = 0.0  （默认不延缓）
  SlowRate       = 0.3
  CatchUpRate    = 2.0
```

当 AN 已配置 `HitStopMode` 时，BFNode 直接读取 AN 参数激活。
当 AN 为 `None` 且无 Tag 时，FA 静默跳过。

### 4.2 暴击触发冻结帧（在暴击 FA 中写 Tag）

```text
[On Crit Hit]
      ↓
[AddTag: Buff.Status.HitStop.Freeze]
  Target = BuffOwner
  Count  = 1
```

> 暴击 FA 写入 Tag，命中停顿 FA 的 `BFNode_HitStop` 在 `OnDamageDealt` 时消费该 Tag，触发冻结。此时使用 BFNode 节点的默认 FrozenDuration，不受 AN 覆盖参数影响。

### 4.3 特定攻击携带延缓帧（在攻击 GA 配套 FA 中写 Tag）

```text
[OnDamageDealt] / [Flow 开始]
      ↓
[AddTag: Buff.Status.HitStop.Slow]
  Target = BuffOwner
```

> 注意：若该攻击的 AN 已配置 `HitStopMode = Slow`，则无需在 FA 里再写 Tag。

---

## 五、典型参数配置

| 场景 | FrozenDuration | SlowDuration | SlowRate | CatchUpRate |
| --- | --- | --- | --- | --- |
| 轻击暴击 | 0.05 | 0 | — | — |
| 重击暴击 | 0.06 | 0.12 | 0.3 | 2.0 |
| 特殊技能重击 | 0.08 | 0.15 | 0.25 | 2.0 |

---

## 六、命中事件广播（OnHitEventTags）

`AN_MeleeDamage` 新增 `OnHitEventTags` 数组（`TArray<FGameplayTag>`），命中目标时 `GA_MeleeAttack::OnEventReceived` 向攻击者广播这些事件 Tag。

**典型用途**：
- 暴击镜头抖动：Tag `Ability.Event.CritHit` → 摄像机 GA 监听触发抖动
- 击退音效：Tag `Ability.Event.Knockback` → 音效系统监听播放
- 重击屏幕闪烁：Tag `Ability.Event.HeavyHit` → UI 系统监听

**生命周期**：AN Notify 时存入 `Character->PendingOnHitEventTags`，命中后广播并清空；蒙太奇被打断时 `EndAbility` 安全清空。

---

## 七、代码位置

| 文件 | 职责 |
| --- | --- |
| `Source/DevKit/Public/Animation/HitStopManager.h` | 管理器头文件 |
| `Source/DevKit/Private/Animation/HitStopManager.cpp` | 蒙太奇三阶段实现（Frozen/Slow/CatchUp） |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h` | FA 节点头文件 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_HitStop.cpp` | FA 节点实现：读 AN 模式 + Tag → 消费 → 调 Manager |
| `Source/DevKit/Public/Animation/AN_MeleeDamage.h` | AN 攻击参数配置（含 HitStopMode / OnHitEventTags） |
| `Source/DevKit/Private/Animation/AN_MeleeDamage.cpp` | AN 触发时暂存 HitStop 覆盖参数和事件 Tag 到角色 |
| `Source/DevKit/Public/Character/YogCharacterBase.h` | `PendingHitStopOverride` / `PendingOnHitEventTags` 暂存结构 |

---

## 八、追帧计算

```text
丢失帧时长 = SlowDuration × (1 - SlowRate)
追帧时长   = 丢失帧时长 / (CatchUpRate - 1)

例：SlowDuration=0.12, SlowRate=0.3, CatchUpRate=2.0
  丢失 = 0.12 × 0.7 = 0.084s
  追帧 = 0.084 / 1.0 = 0.084s  （以 2× 速度播 0.084s 即补回）
```
