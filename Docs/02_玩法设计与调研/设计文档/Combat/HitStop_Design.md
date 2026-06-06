# HitStop 命中停顿系统设计文档

> 最后更新：2026-04-23

---

## 一、系统目标

让玩家攻击命中时产生"冻结帧"或"延缓帧"效果，增强打击感。
效果通过 **Tag 驱动**，由 FA（BuffFlow）控制触发条件，而非写死在蒙太奇 AnimNotify 上。

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

## 三、Tag 设计

位于 `Config/Tags/BuffTag.ini`，挂在**攻击方（玩家）** ASC：

| Tag | 含义 |
| --- | --- |
| `Buff.Status.HitStop.Freeze` | 下次命中时触发冻结帧 |
| `Buff.Status.HitStop.Slow` | 下次命中时触发延缓帧 |

- 两个 Tag 可同时存在（先冻结再延缓）
- `BFNode_HitStop` 执行时**消费**这两个 Tag（`RemoveLooseGameplayTag`），保证单次命中只触发一次

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

默认情况下，FA 会因 ASC 上没有 HitStop Tag 而**直接跳过**（无效果）。

### 4.2 暴击触发冻结帧（在暴击 FA 中写 Tag）

```text
[On Crit Hit]
      ↓
[AddTag: Buff.Status.HitStop.Freeze]
  Target = BuffOwner
  Count  = 1
```

> 暴击 FA 写入 Tag，命中停顿 FA 的 `BFNode_HitStop` 在 `OnDamageDealt` 时消费该 Tag，触发冻结。

### 4.3 特定攻击携带延缓帧（在攻击 GA 配套 FA 中写 Tag）

```text
[OnDamageDealt] / [Flow 开始]
      ↓
[AddTag: Buff.Status.HitStop.Slow]
  Target = BuffOwner
```

---

## 五、典型参数配置

| 场景 | FrozenDuration | SlowDuration | SlowRate | CatchUpRate |
| --- | --- | --- | --- | --- |
| 轻击暴击 | 0.05 | 0 | — | — |
| 重击暴击 | 0.06 | 0.12 | 0.3 | 2.0 |
| 特殊技能重击 | 0.08 | 0.15 | 0.25 | 2.0 |

---

## 六、代码位置

| 文件 | 职责 |
| --- | --- |
| `Source/DevKit/Public/Animation/HitStopManager.h` | 管理器头文件 |
| `Source/DevKit/Private/Animation/HitStopManager.cpp` | 蒙太奇三阶段实现（Frozen/Slow/CatchUp） |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_HitStop.h` | FA 节点头文件 |
| `Source/DevKit/Private/BuffFlow/Nodes/BFNode_HitStop.cpp` | FA 节点实现：读 Tag → 消费 → 调 Manager |

---

## 七、追帧计算

```text
丢失帧时长 = SlowDuration × (1 - SlowRate)
追帧时长   = 丢失帧时长 / (CatchUpRate - 1)

例：SlowDuration=0.12, SlowRate=0.3, CatchUpRate=2.0
  丢失 = 0.12 × 0.7 = 0.084s
  追帧 = 0.084 / 1.0 = 0.084s  （以 2× 速度播 0.084s 即补回）
```
