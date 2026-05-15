# BuffFlow 符文制作流程

> 版本：Sprint 4.14（2026-04-14）
> 上级文档：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)
> 详细案例：[TestRune_CreationGuide.md](TestRune_CreationGuide.md)

---

## 制作前：明确设计意图

在打开 UE 之前，先用这张表把符文想清楚：

| 问题 | 常见答案 |
|---|---|
| 触发时机？ | 被动常驻 / 命中时 / 击杀时 / 受伤时 / 冲刺时 / 暴击时 / 定时 |
| 效果目标？ | 自己（BuffOwner）/ 敌人（LastDamageTarget）/ 事件携带目标 |
| 持续多久？ | 永久（FA 存活期）/ 有时限（N 秒自动过期）/ 一次性瞬发 |
| 需要叠加？ | 不叠 / 刷新时间（Unique）/ 叠层（Stackable） |
| 需要每秒触发？ | 是 → `Period` 字段或 `On Periodic` 节点 |
| 有条件限制？ | 是 → `Has Tag` 或 `Compare Float` 条件分支 |
| 需要额外资产？ | 简单属性 → 不需要；复杂公式/冲量 → 找程序 |

---

## Step 1：选择正确节点

### 效果类型决策树

```
需要修改属性值？
  ├─ 固定数值，永久（FA 活跃期）   →  Apply Attribute Modifier（Infinite）
  ├─ 固定数值，有时限               →  Apply Attribute Modifier（Has Duration）
  ├─ 固定数值，瞬发永久             →  Apply Attribute Modifier（Instant）
  ├─ 每秒 +N，可被条件暂停         →  On Periodic + Has Tag 判断 + Apply Attribute Modifier（Instant）
  ├─ 动态数值（读其他属性计算）     →  Get Attribute → Math Float → Apply Attribute Modifier（Value 数据引脚）
  ├─ 需要 GAS Debugger 可见        →  Apply Gameplay Effect Class（Blueprint GE）
  └─ 复杂伤害公式                   →  找程序写 C++ ExecCalc

需要状态标记？
  ├─ 永久（FA 停止时消失）          →  Add Tag
  └─ 有倒计时（自动过期）           →  Grant Tag (Timed)

需要技能执行（GA）？
  └─ 找程序写 Blueprint/C++ GA + Grant GA 节点

需要跨符文通信？
  ├─ 发送信号给其他符文             →  Send Gameplay Event
  └─ 接收其他符文的信号             →  Wait Gameplay Event
```

### 常见陷阱

| 误区 | 正确做法 |
|---|---|
| 用 `Instant` 做"临时"加成 | Instant 永久改基值，临时效果用 `Infinite` 或 `Has Duration` |
| 用 Blueprint GE 实现"有 Tag 时暂停 Period" | 改用 `On Periodic + Has Tag 条件跳过`，零额外资产 |
| On Periodic Out 引脚连回 In 引脚 | 触发节点自身已是循环，不需要手动重连 |
| Do Damage 忘加递归守卫 | 凡用 Do Damage 必须加 `ExtraDamageApplied` 守卫 |

---

## Step 2：创建 Flow Asset（FA）

1. Content Browser → 右键 → **Flow → Flow Asset**
2. 路径：`Content/Game/Runes/<符文名>/`
3. 命名：`FA_Rune_<功能名>`（例：`FA_Rune_AttackUp`）
4. 双击打开，**Start 节点**为入口，所有触发节点从 Start 出发连接

**FA 结构约定：**
- 一个 FA 通常有 1-3 个触发节点分支（Start → Trigger1，Start → Trigger2，…）
- 触发节点的 `Stop` 引脚无需手动连接，FA 停止时自动清理所有监听
- Cleanup 自动处理：Add Tag / Grant Tag / Apply Attribute Modifier（Infinite/Duration）创建的资源都会在 FA 停止时移除

---

## Step 3：创建 RuneDataAsset（DA）

1. Content Browser → 右键 → **Miscellaneous → Data Asset → RuneDataAsset**
2. 路径：`Content/Game/Runes/<符文名>/`
3. 命名：`DA_Rune_<功能名>`

**必填字段：**

| 字段 | 说明 |
|---|---|
| `RuneConfig.RuneName` | 符文显示名 |
| `RuneConfig.RuneID` | 策划表 ID（如 1001） |
| `RuneConfig.RuneType` | Buff / Debuff / None |
| `Shape.Cells` | 至少填 `(0,0)` = 1格 |
| `Flow.FlowAsset` | 拖入对应 FA |

---

## Step 4：测试

1. 将 DA 填入角色的 `DebugTestRunes` 数组（或 `PermanentRunes`）
2. 运行游戏，打开 **GAS Debugger**（按 `'` 键）

**验证清单：**

| 验证项 | 在哪看 |
|---|---|
| 属性变化是否正确 | GAS Debugger → Attributes |
| Tag 是否正确授予 | GAS Debugger → Granted Tags |
| 激活 GE 列表 | GAS Debugger → Active Effects |
| 移出激活区后效果是否清理 | 拖出格子后再看 Debugger |
| 无递归/无异常叠加 | 观察属性值变化曲线是否符合预期 |

---

## 常用 FA 模式速查

### 模式 A：被动属性加成（最简单）
```
[Start] → [Apply Attribute Modifier]（Infinite，BuffOwner）
```
*示例：攻击强化 1001*

---

### 模式 B：命中后叠加 Buff（Stackable）
```
[Start] → [On Damage Dealt] → [Apply Attribute Modifier]（Stackable, HasDuration）
```
*示例：速度叠加 1003*

---

### 模式 C：定时循环 + 条件跳过（替代 OngoingTagRequirements）
```
[Start] → [On Periodic] → [Has Tag] → No → [Apply Attribute Modifier]（Instant）
                                     ↑ Yes → 跳过本 Tick
          [On Damage Received] → [Grant Tag (Timed)]（守卫 Tag）
```
*示例：热度提升 1002（受伤暂停热度积累）*

---

### 模式 D：一次性消耗窗口
```
[Start]
[On Dash] → [Grant Tag (Timed)]（窗口状态，N秒）

[On Damage Dealt] → [Has Tag]（窗口状态）→ Yes
                  → [Remove Tag]（消耗窗口）
                  → [Do Damage]（效果）
```
*示例：突刺连击 1010（冲刺后2秒内下次攻击双倍）*

---

### 模式 E：递归守卫 Do Damage
```
[On Damage Dealt] → [Has Tag]（ExtraDamageApplied）→ No
                  → [Add Tag]（ExtraDamageApplied）
                  → [Do Damage]
                  → [Remove Tag]（ExtraDamageApplied）
```
*示例：额外伤害 1006，弱点窥破 1009*

---

### 模式 F：跨符文通信（Send / Wait Gameplay Event）

两个独立符文通过玩家 ASC 传信号：

```
符文A - FA（持续监听伤害）：
  [Start] → [On Damage Dealt] → [Send Gameplay Event]（Tag=Action.Rune.XXX, Target=BuffOwner）

符文B - FA（等待信号）：
  [Start] → [Wait Gameplay Event]（Tag=Action.Rune.XXX, Target=BuffOwner）→ Out → [效果节点]
```

**关键：Target 填 BuffOwner（玩家自身）**，不是敌人。两个符文通过玩家 ASC 通信，效果再通过 `LastDamageTarget` 指向敌人。

*示例：击退 1004（发 `Action.Rune.KnockbackApplied` 到 BuffOwner）+ 击退减速 1007（Wait 监听后对 LastDamageTarget 施加减速）*

---

### 模式 G：Send Gameplay Event → C++ GA 激活链
```
FA：
  [Apply Attribute Modifier]
      GrantedTagsToASC = Buff.Status.Bleeding（计时 Tag，到期自动移除）
      GrantedAbilities = GA_Bleed（随 GE 生命周期授予/撤销）
      DurationType = HasDuration / Infinite
      ↓ Out
  [Send Gameplay Event]
      EventTag = Buff.Event.Bleed
      Target = LastDamageTarget（敌人）
      Instigator = BuffOwner（玩家，用于伤害日志）
      Magnitude = 每秒伤害量（可连数据引脚）

GA_Bleed（C++ 实现，挂在目标身上）：
  TriggerTag = Buff.Event.Bleed → 事件激活，EventMagnitude 传入 DPS
  BleedTick（每 0.5s）→ ApplyModToAttributeUnsafe(Health, -DPS×0.5)
  监听 Buff.Status.Bleeding Tag 消失 → EndAbility
  流血 Tick 不触发受击动画（YogCharacterBase 检测 Bleeding Tag 跳过 HitReact）
```
*示例：流血 1005*  
> **注意**：GA_Bleed 已迁移为 C++（`Source/DevKit/.../GA_Bleed`），不需要创建 Blueprint GA 子类。

---

### 模式 H：动态数值计算（数据引脚链）
```
[Get Attribute] → CachedValue
                        ↓（数据线）
              [Math Float A - B] → Result（bonus）
                        ↓（数据线，同时连 Compare Float.A）
              [Compare Float > 0] → True → [Math Float A ÷ B] → Result（pct）
                                                                      ↓（数据线）
                                              [Apply Attribute Modifier].Value
```
*示例：幽风低语 1012（速度加成 % → 暴击率）*

---

### 模式 I（新）：蒙太奇命中触发 — 一次性效果

用于**特定连段命中时立即触发一次**的效果。FA 从 Start 直接执行，不等待任何事件，执行完立即 Finish。

```
[Start] → [Send Gameplay Event]（EventTag, Target=BuffGiver）→ [Finish]
```

**注意：Target 必须填 BuffGiver**（不是 LastDamageTarget）。此类 FA 由 C++ `ReceiveOnHitRune` 启动，`BuffGiver` 已被设置为被命中的敌人，而 `LastDamageTarget` 尚未在新 FA 上下文中更新。

配套资产：需要单独的 `DA_HitEffect_XXX` 和 `FA_HitEffect_XXX`，不复用背包符文的 FA。

*参考：[蒙太奇命中符文配置指南](../Combat/MeleeCombo_NotifyRune_Guide.md)*

---

### 两种 FA 结构对比

| 类型 | 激活方式 | FA 结构 | Target 选择器 |
|---|---|---|---|
| **背包常驻符文** | 装备时启动，持续监听 | Start → OnDamageDealt → 效果 → (循环) | `LastDamageTarget` |
| **蒙太奇命中触发** | 命中时启动，执行一次结束 | Start → 效果 → Finish | `BuffGiver` |

> ⚠️ **不要把背包符文的 FA 直接填进 `AdditionalRuneEffects`**。背包 FA 有 `OnDamageDealt` 等待节点，启动后会延迟一次才触发，产生"晚一段才生效、之后每段都生效"的 Bug。

---

## 资产目录结构

```
Content/Game/Runes/
├── AttackUp/       DA + FA
├── HeatUp/         DA + FA（无 GE）
├── SpeedStack/     DA + FA
├── Knockback/      DA + FA + BGA_Knockback（C++ 子类）
├── KnockbackStagger/ DA + FA
├── Bleed/          DA + FA + GA_Bleed
├── ExtraDamage/    DA + FA
├── SlashWave/      DA + FA + BGA_SlashWaveCounter + BP_SlashWaveProjectile + GE_SlashWaveDamage
├── WeaknessUnveiled/ DA + FA
├── DuoAssault/     DA + FA
├── VenomFang/      DA + FA（测试版）/ + GE_Poison（正式版）
└── WraithwindWhisper/ DA + FA
```

---

## 什么时候需要找程序

| 需求 | 原因 | 对应符文 |
|---|---|---|
| 物理冲量/精确位移 | RootMotion 必须在 GA C++ 层实现 | 1004 击退 |
| 速度扣血（自定义逻辑循环） | GA 蓝图实现，复杂循环不在 FA 节点范围内 | 1005 流血 |
| 穿透投射物 | Actor 类、碰撞、生命周期管理在 C++ 层 | 1008 刀光波 |
| 持久被动计数器 GA | C++ 持久 GA 持续监听事件计数 | 1008 刀光波 |
| 真实暴击强制（触发 On Crit Hit） | 暴击判定在 C++ 伤害管线中，需加 Tag 检测 | 1009 弱点窥破正式版 |
| AttributeBased GE（% of MaxHP） | FA 层 Math Float 只能固定值；% 依赖 GE Magnitude 配置 | 1011 毒牙正式版 |
