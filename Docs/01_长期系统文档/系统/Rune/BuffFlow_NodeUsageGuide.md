# BuffFlow 节点使用指南

> 版本：Sprint 4.15（2026-04-08）
> 上级文档：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)
> 节点速查：[BuffFlow_NodeReference.md](BuffFlow_NodeReference.md)

本文档用实际例子说明各类节点的**用法、连线方式和常见误区**。

---

## 一、触发节点：怎么"开始监听"和"停止监听"

触发节点不是"瞬间执行一次"，而是**持续监听事件**。它的两个输入引脚：

- **In**：开始监听（Start 连到这里）
- **Stop**：停止监听（一般不用手连，FA 停止时自动清理）

```
[Start] ──▶ [On Damage Dealt]
                  │ OnDamage
                  ▼
            [Apply Attribute Modifier]
```

**误区**：不要把 `Out` 连回 `In`——触发节点本身已经是循环的，每次事件触发都会走 `OnDamage`，不需要手动"重连"。

---

## 二、Apply Attribute Modifier：最常用的效果节点

### Duration Type 怎么选

| 你想要的效果 | Duration Type | 说明 |
|---|---|---|
| 符文在激活区内持续加攻击力 | **Infinite** | FA 停止时自动移除 GE |
| 击杀后加速 3 秒 | **Has Duration** | 3 秒后自动到期 |
| 捡到道具永久+10 最大血量 | **Instant** | 立刻改属性基值，不可撤销 |
| 每秒给目标加热度 | **Infinite + Period=1.0** | 每秒执行一次 |

> ⚠️ **Instant ≠ 临时**。Instant 是永久修改属性基础值，不能被移除。用"符文激活有效果、失活撤销"这种模式必须用 **Infinite**。

### 连固定数值

```
[Start] ──▶ [Apply Attribute Modifier]
                Attribute    = BaseAttributeSet.Attack
                Mod Op       = Additive
                Value        = 100.0          ← 直接填在节点上
                Duration Type= Infinite
                Target       = Buff拥有者
```

### 连数据引脚（动态数值）

```
[Get Attribute]              [Apply Attribute Modifier]
   Attribute = MaxHealth         Attribute = Attack
   Target    = Buff拥有者        Mod Op    = Additive
      │ CachedValue              Value     ← 连这里
      └──────────────────────────────────▶
```

`CachedValue` 是数据引脚（金色圆点），用**数据线**（细线）连到 `Value`，不是执行线（粗白线）。

### 一次性消耗型 Buff（Infinite + Remove 引脚）

```
[Start] ──▶ [Apply Attribute Modifier]   ← 施加 Infinite buff
                  │ Out
                  ▼
            [On Damage Dealt]             ← 等待下次攻击
                  │ OnDamage
                  ▼
            [Apply Attribute Modifier].Remove  ← 触发 Remove 引脚移除 buff
                  │ Out
                  ▼
            [Finish Buff]                ← 结束整个 FA
```

---

## 三、Apply Gameplay Effect Class：需要 GE 资产时用这个

当你需要：
- GAS Debugger 里可以看到效果名
- 多个 SetByCaller 参数传值
- 复杂的 GE 配置（如执行计算、多 Modifier）

**流程**：
1. 在内容浏览器新建 `GameplayEffect` 蓝图（如 `GE_Poison`）
2. 在 GE 蓝图里配置好 Modifier
3. 节点上选择这个 GE 类

```
[On Damage Dealt] ──▶ [Apply Gameplay Effect Class]
                           Effect = GE_Poison
                           Target = 上次伤害目标
                           Level  = 1.0
```

### SetByCaller 传值

GE 蓝图里的 Modifier 数值来源选 `SetByCaller`，填一个 Tag（如 `Data.Damage`）。节点上配置：

```
Slot 1 Tag   = Data.Damage
Slot 1 Value = 50.0         ← 也可以连数据引脚
```

---

## 四、Tag 节点三兄弟

| 节点 | 用途 | FA 停止时 |
|---|---|---|
| **Add Tag** | 永久添加 Loose Tag | ✅ 自动移除 |
| **Grant Tag (Timed)** | 添加 + 可设倒计时 | ✅ 自动移除 |
| **Has Tag** | 只读查询，不修改 | — |
| **Remove Tag** | 立即移除 | — |

### 场景 A：受伤后暂停热度积累 5 秒

```
[On Damage Received]
      │ OnDamage
      ▼
[Grant Tag (Timed)]
   Tag      = Buff.Status.HeatInhibit
   Duration = 5.0
   Target   = Buff拥有者
      │ Out       → （继续其他逻辑）
      │ Expired   → （5 秒后 Tag 移除，热度恢复）
```

### 场景 B：防止效果重复触发（守卫 Tag）

```
[On Damage Dealt]
      │ OnDamage
      ▼
[Has Tag]  Tag = Buff.Guard.AttackUp, Target = Buff拥有者
      │ Yes → （已有 Tag，跳过）
      │ No
      ▼
[Add Tag]  Tag = Buff.Guard.AttackUp    ← 先加守卫 Tag
      │ Out
      ▼
[Apply Attribute Modifier] ...效果...
      │ Out
      ▼
[Remove Tag]  Tag = Buff.Guard.AttackUp  ← 效果结束后移除守卫
```

---

## 五、数据引脚：怎么传值

数据引脚（金色圆点）用**细线**连接，执行引脚（白色三角）用**粗线**连接，两者不能混用。

### 常用数据节点

**Literal Float**：输出一个固定数值，本身没有执行引脚。

```
[Literal Float: 50.0]
      │ Value（数据线）
      ▼
[Apply Attribute Modifier].Value
```

**Math Float**：做运算后输出结果。

```
[Get Attribute] CachedValue ──▶ [Math Float] A
                                  Operator = Multiply
[Literal Float: 0.1]     ──▶              B
                                  │ Result（数据线）
                                  ▼
                        [Apply Attribute Modifier].Value
```

（上面的例子：取当前属性值 × 0.1，即 10% 作为修改量）

**Get Attribute**：读属性值，有执行引脚（In/Out），需要接在流程里。

```
[On Kill]
    │ OnKill
    ▼
[Get Attribute]  Attribute=MaxHealth, Target=Buff拥有者
    │ Out
    ▼
[Apply Attribute Modifier]  ← Value 连 CachedValue 数据引脚
```

---

## 六、Compare Float：条件分支

A 和 B 都支持数据引脚连线，也可以直接填固定值。

### 场景：血量低于 30% 时触发效果

```
[On Damage Received]
      │ OnDamage
      ▼
[Get Attribute]  Attribute=Health, Target=Buff拥有者
      │ Out
      ▼
[Compare Float]
   A        ← 连 CachedValue（当前血量）
   Operator = <=
   B        = 30.0                 ← 直接填 30
      │ True  → [Apply Attribute Modifier] 紧急增益
      │ False → （不做任何事）
```

---

## 七、Phase 节点：热度升降阶流程

### 标准升阶流程

```
[Start] ──▶ [On Phase Up Ready]    ← 监听"热度满 + 可升阶"
                  │ OnPhaseUp
                  ▼
            [Phase Decay Timer]    ← Duration=2.0，给玩家操作窗口
               Duration = 2.0
                  │ Out（2秒内未取消）
                  ▼
            [Increment Phase]      ← 执行升阶
                  │ Out
                  ▼
            [Play Niagara]         ← 播放升阶特效
```

### 标准降阶流程

```
[Start] ──▶ [On Heat Reached Zero]   ← 监听热度归零
                  │ OnReachedZero
                  ▼
            [Decrement Phase]         ← 降一阶
```

---

## 八、On Periodic：定时循环

`Interval` 设几秒触发一次，`Fire Immediately` 决定是否在 FA 启动时立刻触发第一次。

### 场景：每 2 秒扣 5 点热度

```
[Start] ──▶ [On Periodic]
               Interval = 2.0
               Fire Immediately = false
                  │ Tick（每 2 秒）
                  ▼
            [Apply Attribute Modifier]
               Attribute    = BaseAttributeSet.Heat
               Mod Op       = Additive
               Value        = -5.0
               Duration Type= Instant      ← 每次都是直接改基值
               Target       = Buff拥有者
```

> 注意：这里用 Instant 是因为 On Periodic 已经在外层控制频率了，每次"加/减一个量"是瞬发行为。如果想用 GAS 内置的 Period，在 Apply Attribute Modifier 节点上配 Period=2.0, Duration=Infinite，不需要 On Periodic 节点。

---

## 九、Do Damage：对目标造成伤害

**必须提前创建一个 Blueprint GE**（用于实际扣血）。

| 字段 | 说明 |
|---|---|
| Target Selector | 打谁（上次伤害目标 / Buff拥有者） |
| Flat Damage | 固定伤害值，>0 时优先使用 |
| Damage Multiplier | Flat Damage=0 时生效，对上次伤害量乘倍率 |
| Damage Effect | 扣血用的 GE 类，必须配置 |

GE 里的 Modifier 用 `SetByCaller`（Tag=`Data.Damage`），节点把计算好的值注入进去。

```
[On Kill] ──▶ [Do Damage]
                Target         = 上次伤害目标
                Flat Damage    = 0
                Damage Multipler = 0.5     ← 对击杀那次伤害的 50% 再打一次
                Damage Effect  = GE_BasicDamage
```

---

## 十、Grant GA：授予技能

```
[Start] ──▶ [Grant GA]
               Ability Class = GA_DoubleJump
               Target        = Buff拥有者
               Level         = 1
                  │ Out     → （成功，继续逻辑）
                  │ Failed  → （目标无 ASC，记录日志）
```

GA 会在 Cleanup（FA 停止）时自动移除。不需要手动 Remove。
