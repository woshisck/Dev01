# 终结技卡牌 — 编辑器配置指南

> 关联代码：`GA_FinisherCharge.cpp` · `GA_Player_FinisherAttack.cpp` · `GA_ApplyFinisherMark.cpp`  
> 适用版本：v3 FA 驱动架构（印记系统扩展版）  
> 最后更新：2026-05-09

---

## 概览

| 资产名 | 类型 | 路径 | 用途 |
|---|---|---|---|
| `DA_Rune_Finisher` | RuneDataAsset | `/Game/YogRuneEditor/Runes/` | 终结技符文数据资产，含数值表 |
| `GE_FinisherCharge` | GameplayEffect | `/Game/GEs/` | 充能层数（5 层 / 8s，Granted: `Buff.Status.FinisherCharge`） |
| `GE_Mark_Finisher` | GameplayEffect | `/Game/GEs/` | 终结技印记（12s，Granted: `Buff.Status.Mark.Finisher`） |
| `GE_FinisherDamage` | GameplayEffect | `/Game/GEs/` | 引爆伤害 GE（Instant，SetByCaller `Buff.Data.Damage` 扣血） |
| `BGA_FinisherCharge` | Blueprint GA | `/Game/Blueprints/GAs/` | 充能窗口管理 GA（继承 `GA_FinisherCharge`） |
| `BGA_Player_FinisherAttack` | Blueprint GA | `/Game/Blueprints/GAs/` | 终结技主 GA（继承 `GA_Player_FinisherAttack`） |
| `BGA_ApplyMark_Finisher` | Blueprint GA | `/Game/Blueprints/GAs/` | 印记施加 GA（继承 `GA_ApplyFinisherMark`） |
| `FA_FinisherCard_BaseEffect` | YogRuneFlowAsset | `/Game/YogRuneEditor/Flows/` | 打出效果：施加充能 GE + 激活充能 GA |
| `FA_FinisherCard_ChargeHit` | YogRuneFlowAsset | `/Game/YogRuneEditor/Flows/` | 充能命中：击退 + 施加印记（持续循环流） |
| `FA_FinisherCard_Detonate` | YogRuneFlowAsset | `/Game/YogRuneEditor/Flows/` | 引爆印记：伤害 + 割裂 + 条件击退（持续循环流） |

---

## 已注册 Gameplay Tag

> 均已写入 `Config/Tags/*.ini`，无需重复注册。

| Tag | 用途 |
|---|---|
| `Buff.Status.FinisherCharge` | 充能层数状态标记，由 GE_FinisherCharge 授予 |
| `Buff.Status.FinisherWindowOpen` | 充能窗口追踪标记（GE 消失后再保留 1 帧供蒙太奇 Notify 检测） |
| `Buff.Status.FinisherExecuting` | 终结技动作执行中（GA_Player_FinisherAttack ActivationOwnedTags） |
| `Buff.Status.Mark.Finisher` | 终结技印记状态，由 GE_Mark_Finisher 授予 |
| `Action.FinisherCharge.Activate` | 通知 GA_FinisherCharge 激活 |
| `Action.FinisherCharge.ChargeConsumed` | 每次充能命中后由 GA_FinisherCharge 向玩家 ASC 派发 |
| `Action.Mark.Apply.Finisher` | 申请对目标施加终结技印记 |
| `Action.Mark.Detonate.Finisher` | 引爆终结技印记（EventMagnitude = 确认倍率 1.0 / 2.0） |
| `Action.Player.FinisherAttack` | 触发 GA_Player_FinisherAttack |
| `Action.Finisher.Confirm` | 玩家在子弹时间内按下重攻击确认 |
| `Ability.Event.Finisher.HitFrame` | 终结技蒙太奇攻击判定帧事件 |
| `Action.Knockback` | 触发击退 GA |
| `Action.Slash` | 触发割裂效果（移动扣血） |

---

## 一、GE 资产创建

### GE_FinisherCharge

新建 Blueprint GameplayEffect，命名 `GE_FinisherCharge`：

| 字段 | 值 |
|---|---|
| Duration Policy | Has Duration |
| Duration Magnitude | `8.0` 秒 |
| Stacking: Stack Limit Count | `5` |
| Stacking: Stacking Type | Aggregate by Target |
| Stacking: Duration Refresh Policy | **Never Refresh** |
| Stacking: Period Reset Policy | Never Reset |
| Granted Tags | `Buff.Status.FinisherCharge` |

> `Never Refresh` 确保每次叠层时 8 秒计时不重置（5 次攻击后自然到期）。

---

### GE_Mark_Finisher

新建 Blueprint GameplayEffect，命名 `GE_Mark_Finisher`：

| 字段 | 值 |
|---|---|
| Duration Policy | Has Duration |
| Duration Magnitude | `12.0` 秒 |
| Stacking: Stack Limit Count | `1` |
| Stacking: Stacking Type | Aggregate by Target |
| Granted Tags | `Buff.Status.Mark.Finisher` |
| Asset Tags | `Buff.Status.Mark.Finisher` |

> 每个目标最多持有 1 个印记（Stack Limit = 1），时间到期自动消失。

---

### GE_FinisherDamage

新建 Blueprint GameplayEffect，命名 `GE_FinisherDamage`（可复用现有伤害 GE）：

| 字段 | 值 |
|---|---|
| Duration Policy | Instant |
| Modifier: Attribute | `BaseAttributeSet.Health` |
| Modifier: Modifier Op | Additive |
| Modifier: Magnitude Calculation Type | Set by Caller |
| Modifier: Data Tag | `Buff.Data.Damage` |

---

## 二、GA 蓝图配置

### BGA_ApplyMark_Finisher

打开 `BGA_ApplyMark_Finisher`（继承自 `GA_ApplyFinisherMark`），在 Class Defaults 确认：

| 字段 | 值 | 说明 |
|---|---|---|
| Finisher Mark GE Class | `GE_Mark_Finisher` | 施加到目标的印记 GE |

> `AbilityTriggers`（监听 `Action.Mark.Apply.Finisher`）已在 C++ 构造函数设置，Blueprint 侧无需重复配置。

---

### BGA_FinisherCharge

打开 `BGA_FinisherCharge`（继承自 `GA_FinisherCharge`），配置：

| 字段 | 值 |
|---|---|
| Ability Tags | `PlayerState.AbilityCast.FinisherCharge` |
| Activation Blocked Tags | `PlayerState.AbilityCast.FinisherCharge` |
| Max Charges | `5` |
| Finisher Charge GE Class | `GE_FinisherCharge` |

> `AbilityTriggers`（监听 `Action.FinisherCharge.Activate`）已在 C++ 中设置。`ActivationBlockedTags` 与 `AbilityTags` 相同，防止重复激活两个实例。

---

### BGA_Player_FinisherAttack

打开 `BGA_Player_FinisherAttack`（继承自 `GA_Player_FinisherAttack`），配置：

| 字段 | 值 |
|---|---|
| Ability Tags | `PlayerState.AbilityCast.Finisher` |
| Activation Owned Tags | `Buff.Status.FinisherExecuting` |
| Activation Required Tags | `Buff.Status.FinisherWindowOpen` |
| Activation Blocked Tags | `Buff.Status.Dead` · `Buff.Status.FinisherExecuting` |
| Cancel Abilities With Tag | `PlayerState.AbilityCast.LightAtk` · `PlayerState.AbilityCast.HeavyAtk` · `PlayerState.AbilityCast.Dash` |
| Finisher Montage | `AM_Player_FinisherAttack` |
| Confirmed Damage Multiplier | `2.0` |

> **重要：** `CancelAbilitiesWithTag` **不要**填 `PlayerState.AbilityCast.FinisherCharge`——终结技激活时不应取消充能窗口 GA。  
> `ActivationRequiredTags = Buff.Status.FinisherWindowOpen`（非 `FinisherCharge`）：最后一层消耗后 GE 消失，但 WindowOpen LooseTag 再多保留 1 帧，确保蒙太奇 Notify 能触发终结技。

---

### DA_Base_AbilitySet — 预授予

将以下 GA 添加到玩家的 `DA_Base_AbilitySet`（预授予到所有玩家角色）：

- `BGA_ApplyMark_Finisher` — 被动监听 `Action.Mark.Apply.Finisher`
- `BGA_FinisherCharge` — 充能窗口管理
- `BGA_Player_FinisherAttack` — 终结技主 GA

---

## 三、DA_Rune_Finisher — 数值表

在符文编辑器中选中 `DA_Rune_Finisher`，切换到**数值表**选项卡，添加：

| Key | 默认值 | 说明 |
|---|---|---|
| `DetonationDamage` | `80.0` | 引爆基础伤害（未确认 ×1.0，确认 ×2.0） |
| `KnockbackDistance` | `400.0` | 充能命中击退距离（单位：cm） |

---

## 四、FA_FinisherCard_BaseEffect（打出效果流）

### 定位

卡牌打出时**执行一次**的基础效果流。执行完毕即结束。

- 施加 `GE_FinisherCharge`（5 次叠加 → 5 层充能）
- 发送 `Action.FinisherCharge.Activate` 激活 `GA_FinisherCharge`

**在 DA 中**：填入 `DA_Rune_Finisher` 的**打出效果（BaseFlow）**槽位。

### 节点图

```
[Start]
  │
  ▼
[Apply Gameplay Effect Class]   ← 施加充能 GE（5次，不自动移除）
  │ Out
  ▼
[Send Gameplay Event]           ← 激活充能 GA
  │ Out
  ▼
[Finish Buff]                   ← FA 结束
```

### 节点配置

#### 节点 A：Apply Gameplay Effect Class

| 字段 | 值 |
|---|---|
| 要施加的效果类 | `GE_FinisherCharge` |
| 施加目标 | `BuffOwner` |
| 施加次数 | `5` |
| FA结束时移除效果 | ✗（GE 需在本 FA 结束后继续存在） |

- **Out** → 节点 B

#### 节点 B：Send Gameplay Event

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.FinisherCharge.Activate` |
| 事件接收目标 | `BuffOwner` |
| Payload 目标 | `BuffOwner` |
| 发起者 | `BuffOwner` |

- **Out** → 节点 C

#### 节点 C：Finish Buff

无配置，终止 FA。

---

## 五、FA_FinisherCard_ChargeHit（充能命中持续流）

### 定位

**持续运行的循环流**。每次 `GA_FinisherCharge` 消耗一层充能后，向玩家 ASC 派发 `Action.FinisherCharge.ChargeConsumed`，本 FA 每次收到后执行：
1. 向被命中目标发送击退事件（Magnitude = KnockbackDistance）
2. 向被命中目标申请施加终结技印记

**FA 生命周期**：绑定**符文卡牌自身永久 GE**（非 GE_FinisherCharge）。FA 在符文装备全程激活；充能未激活时节点安静等待，不消耗资源。

**在 DA 中**：填入 `DA_Rune_Finisher` 的**持续效果（PassiveFlow）**槽位，绑定符文永久 GE。

### 节点图

```
┌─ [Wait Gameplay Event]  ←─────────────────────────────────────────────┐
│    事件 Tag = Action.FinisherCharge.ChargeConsumed                     │（每次触发后循环）
│    监听目标 = BuffOwner                                                 │
│    │ Out                                                               │
│    ▼                                                                   │
│  [Get Rune Tuning Value]                                               │
│    Key = KnockbackDistance                                             │
│    数值（输出）─────────────────────────┐（数据连线）                   │
│    │ Found                             │                               │
│    ▼                                   ▼                               │
│  [Send Gameplay Event（击退）]          └──→ 事件强度值                  │
│    事件 Tag = Action.Knockback                                          │
│    事件接收目标 = LastDamageTarget                                      │
│    │ Out                                                               │
│    ▼                                                                   │
│  [Send Gameplay Event（施加印记）]──────────────────────────────────────┘
│    事件 Tag = Action.Mark.Apply.Finisher
│    事件接收目标 = LastDamageTarget
│    （Out 不连接，自动回到 WaitGameplayEvent 等待下一次事件）
```

### 节点配置

#### 节点 A：Wait Gameplay Event（循环节点）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.FinisherCharge.ChargeConsumed` |
| 监听目标 | `BuffOwner`（玩家） |
| 事件强度（输出） | （不连线，此事件无附加 Magnitude） |

- **Out** → 节点 B
- 事件触发时，`EventData.Target`（被命中敌人）自动写入 `BFC.LastEventContext`，下游节点通过 `LastDamageTarget` 选择器读取。

#### 节点 B：Get Rune Tuning Value

| 字段 | 值 |
|---|---|
| 数值表 Key | `KnockbackDistance` |
| 默认值 | `400.0` |
| 数值（输出） | **[数据引脚]** → 连线到节点 C 的**事件强度值** |

- **Found** → 节点 C

#### 节点 C：Send Gameplay Event（击退）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.Knockback` |
| 事件接收目标 | `LastDamageTarget` |
| Payload 目标 | `LastDamageTarget` |
| 发起者 | `DamageCauser` |
| 事件强度值 | **[数据引脚]** ←── 连接自节点 B 的**数值（输出）** |

- **Out** → 节点 D

#### 节点 D：Send Gameplay Event（施加印记）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.Mark.Apply.Finisher` |
| 事件接收目标 | `LastDamageTarget` |
| Payload 目标 | `LastDamageTarget` |
| 发起者 | `DamageCauser` |

- **Out** → （不连接，FA 循环等待下一次 WaitGameplayEvent 触发）

> `BGA_ApplyMark_Finisher` 在 `LastDamageTarget` 的 ASC 上收到此事件后，检查目标是否已有 `Buff.Status.Mark.Finisher`（有则跳过），无则施加 `GE_Mark_Finisher`。

---

## 六、FA_FinisherCard_Detonate（引爆印记持续流）

### 定位

**持续运行的循环流**。`GA_Player_FinisherAttack` 在蒙太奇攻击判定帧扫描所有带 `Buff.Status.Mark.Finisher` 的目标，向玩家 ASC 逐一派发 `Action.Mark.Detonate.Finisher`（`EventMagnitude` = 1.0 未确认 / 2.0 已确认）。本 FA 每次收到后执行：
1. 读取 `DetonationDamage`，乘以确认倍率
2. 对目标造成最终伤害
3. 向目标发送割裂事件
4. 若倍率 ≥ 2.0（已确认）→ 额外发送击退事件

**FA 生命周期**：绑定**符文卡牌自身永久 GE**（非 GE_FinisherCharge）。

**在 DA 中**：填入 `DA_Rune_Finisher` 的**持续效果（PassiveFlow）**槽位，绑定符文永久 GE。

### 节点图

```
┌─ [Wait Gameplay Event]  ←────────────────────────────────────────────────────────────┐
│    事件 Tag = Action.Mark.Detonate.Finisher                                           │（循环）
│    监听目标 = BuffOwner                                                               │
│    事件强度（输出）─────────────────────────────────────┐（确认倍率 1.0/2.0）         │
│    │ Out                                               │                              │
│    ▼                                                   │                              │
│  [Get Rune Tuning Value]                               │                              │
│    Key = DetonationDamage                              │                              │
│    数值（输出）──→ A ─┐                                │                              │
│    │ Found            │（数据连线）                    │                              │
│    ▼                  ▼                                ▼                              │
│  [Math Float: ×]   A = 数值（输出）              B = 事件强度（输出）                  │
│    Result（输出）──→ 固定伤害值（数据连线）                                            │
│    │ Out                                                                              │
│    ▼                                                                                  │
│  [Do Damage]                                                                          │
│    伤害目标 = LastDamageTarget                                                         │
│    固定伤害值 ←── Result（数据引脚）                                                   │
│    伤害效果类 = GE_FinisherDamage                                                     │
│    │ Out                                                                              │
│    ▼                                                                                  │
│  [Send Gameplay Event（割裂）]                                                         │
│    事件 Tag = Action.Slash                                                             │
│    事件接收目标 = LastDamageTarget                                                     │
│    │ Out                                                                              │
│    ▼                                                                                  │
│  [Compare Float]                                                                      │
│    A ←── 事件强度（输出）（数据引脚）                                                  │
│    B = 2.0                                                                            │
│    Operator = ≥ (GreaterOrEqual)                                                      │
│    │ True                              │ False                                        │
│    ▼                                   ▼                                              │
│  [Send Gameplay Event（击退）]          └────────────────────────────────────────────┘
│    事件 Tag = Action.Knockback
│    事件接收目标 = LastDamageTarget
│    发起者 = DamageCauser
│    │ Out ──────────────────────────────────────────────────────────────────────────────┘
```

### 节点配置

#### 节点 A：Wait Gameplay Event（循环节点）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.Mark.Detonate.Finisher` |
| 监听目标 | `BuffOwner`（玩家） |
| 事件强度（输出） | **[数据引脚]** → 连线到节点 C 的 **B** 输入和节点 F 的 **A** 输入 |

- **Out** → 节点 B
- 事件触发时，`EventData.Target`（被引爆敌人）写入 `BFC.LastEventContext`，下游节点通过 `LastDamageTarget` 读取。

#### 节点 B：Get Rune Tuning Value

| 字段 | 值 |
|---|---|
| 数值表 Key | `DetonationDamage` |
| 默认值 | `80.0` |
| 数值（输出） | **[数据引脚]** → 连线到节点 C 的 **A** 输入 |

- **Found** → 节点 C

#### 节点 C：Math Float（乘法）

| 字段 | 值 |
|---|---|
| Operator | `×`（Multiply） |
| A | **[数据引脚]** ←── 节点 B 的**数值（输出）**（基础伤害） |
| B | **[数据引脚]** ←── 节点 A 的**事件强度（输出）**（确认倍率） |
| Result（输出） | **[数据引脚]** → 连线到节点 D 的**固定伤害值** |

- **Out** → 节点 D

#### 节点 D：Do Damage

| 字段 | 值 |
|---|---|
| 伤害目标 | `LastDamageTarget` |
| 固定伤害值 | **[数据引脚]** ←── 节点 C 的 **Result（输出）** |
| 伤害倍率 | `1.0`（固定伤害值 > 0 时倍率不生效，保持默认即可） |
| 伤害效果类 | `GE_FinisherDamage` |

- **Out** → 节点 E

#### 节点 E：Send Gameplay Event（割裂）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.Slash` |
| 事件接收目标 | `LastDamageTarget` |
| Payload 目标 | `LastDamageTarget` |
| 发起者 | `DamageCauser` |

- **Out** → 节点 F

#### 节点 F：Compare Float（确认路径判断）

| 字段 | 值 |
|---|---|
| A | **[数据引脚]** ←── 节点 A 的**事件强度（输出）**（确认倍率） |
| B | `2.0`（直接填写） |
| Operator | `≥`（GreaterOrEqual） |

- **True** → 节点 G（已确认，额外击退）
- **False** → （不连接，结束本次引爆，FA 循环等待下一次事件）

#### 节点 G：Send Gameplay Event（击退，仅确认路径）

| 字段 | 值 |
|---|---|
| 事件 Tag | `Action.Knockback` |
| 事件接收目标 | `LastDamageTarget` |
| 发起者 | `DamageCauser` |

- **Out** → （不连接，结束本次引爆，FA 循环等待下一次事件）

> **印记清理**：`GE_Mark_Finisher` 由 12 秒 Duration 自然过期，或在 `GA_FinisherCharge.EndAbility` 的 `ClearAllMarks` 中清理（仅当 `Buff.Status.FinisherExecuting` 不存在时执行）。如需立即清理，可在节点 G 之后添加一个 `Apply Gameplay Effect Class`，向 `LastDamageTarget` 施加 Instant GE 并通过 `RemoveEffectsWithGrantedTag` 移除 `GE_Mark_Finisher`。

---

## 七、创建检查清单

### GE 资产

- [ ] `GE_FinisherCharge`：Duration 8s，Stack 5，Granted `Buff.Status.FinisherCharge`，**Never Refresh**
- [ ] `GE_Mark_Finisher`：Duration 12s，Stack 1，Granted `Buff.Status.Mark.Finisher`
- [ ] `GE_FinisherDamage`：Instant，SetByCaller `Buff.Data.Damage` → Additive 扣减 Health

### GA 蓝图配置

- [ ] `BGA_ApplyMark_Finisher`：`FinisherMarkGEClass = GE_Mark_Finisher`（AbilityTriggers 已在 C++ 设置）
- [ ] `BGA_FinisherCharge`：`Ability Tags = PlayerState.AbilityCast.FinisherCharge`，`MaxCharges = 5`，`FinisherChargeGEClass = GE_FinisherCharge`
- [ ] `BGA_Player_FinisherAttack`：`ActivationOwnedTags = Buff.Status.FinisherExecuting`，`ActivationRequiredTags = Buff.Status.FinisherWindowOpen`，**CancelAbilitiesWithTag 不含 FinisherCharge**
- [ ] 三个 GA 均添加到 `DA_Base_AbilitySet`（预授予玩家角色）

### FA 创建

- [ ] `FA_FinisherCard_BaseEffect`（BaseFlow 槽）
  - 节点 A: Apply Gameplay Effect Class（GE_FinisherCharge，目标 BuffOwner，5次，**FA结束时移除效果 = 关闭**）
  - 节点 B: Send Gameplay Event（`Action.FinisherCharge.Activate`，目标 BuffOwner）
  - 节点 C: Finish Buff
  - 连线：Start → A → B → C

- [ ] `FA_FinisherCard_ChargeHit`（PassiveFlow 槽，绑定符文永久 GE）
  - 节点 A: Wait Gameplay Event（`Action.FinisherCharge.ChargeConsumed`，监听 BuffOwner）
  - 节点 B: Get Rune Tuning Value（Key = `KnockbackDistance`，默认 400）
  - 节点 C: Send Gameplay Event（`Action.Knockback`，LastDamageTarget，Magnitude 数据连线自 B）
  - 节点 D: Send Gameplay Event（`Action.Mark.Apply.Finisher`，LastDamageTarget）
  - 连线：A.Out → B.Found → C.Out → D.Out（不连，循环）；B.数值 → C.事件强度值

- [ ] `FA_FinisherCard_Detonate`（PassiveFlow 槽，绑定符文永久 GE）
  - 节点 A: Wait Gameplay Event（`Action.Mark.Detonate.Finisher`，监听 BuffOwner）
  - 节点 B: Get Rune Tuning Value（Key = `DetonationDamage`，默认 80）
  - 节点 C: Math Float（×，A←B.数值，B←A.事件强度）
  - 节点 D: Do Damage（LastDamageTarget，固定伤害值←C.Result，GE_FinisherDamage）
  - 节点 E: Send Gameplay Event（`Action.Slash`，LastDamageTarget）
  - 节点 F: Compare Float（A←A.事件强度，B=2.0，≥）
  - 节点 G: Send Gameplay Event（`Action.Knockback`，LastDamageTarget）[True 路径]
  - 连线：A.Out → B.Found → C.Out → D.Out → E.Out → F.True → G.Out（不连，循环）；F.False → （不连）

### DA 配置

- [ ] `DA_Rune_Finisher` 数值表：`DetonationDamage = 80.0`，`KnockbackDistance = 400.0`
- [ ] `DA_Rune_Finisher` BaseFlow 槽：指向 `FA_FinisherCard_BaseEffect`
- [ ] `DA_Rune_Finisher` PassiveFlow 列表：添加 `FA_FinisherCard_ChargeHit` 和 `FA_FinisherCard_Detonate`，各绑定符文永久 GE

---

## 八、事件流向总览

```
卡牌打出
  → FA_FinisherCard_BaseEffect:
      ApplyEffect(GE_FinisherCharge, 5次, 不自动移除)
      + SendEvent(Action.FinisherCharge.Activate → GA_FinisherCharge 激活)
      + FinishBuff（FA结束，GE和GA继续）

每次攻击命中（Ability.Event.Attack.Hit）
  → GA_FinisherCharge:
      SendEvent(Action.FinisherCharge.ChargeConsumed, Target=被命中敌人)
      + 移除1层 GE_FinisherCharge
    → FA_FinisherCard_ChargeHit (WaitGameplayEvent 触发):
        GetTuning(KnockbackDistance=400)
        → SendEvent(Action.Knockback, 敌人, Magnitude=400)  → GA_Knockback 执行击退
        + SendEvent(Action.Mark.Apply.Finisher, 敌人)
          → BGA_ApplyMark_Finisher: 无印记则施加 GE_Mark_Finisher(12s)

最后一层消耗完（Buff.Status.FinisherCharge Tag 消失）
  → GA_FinisherCharge: 停止 WaitHitTask + 延迟1帧 EndAbility
  → 同帧: AN_TriggerFinisherAbility 检测 Buff.Status.FinisherWindowOpen（还在！）
         → SendEvent(Action.Player.FinisherAttack)
           → GA_Player_FinisherAttack 激活（Buff.Status.FinisherExecuting 出现，时间膨胀）
  → 下1帧: GA_FinisherCharge.EndAbility → ClearAllMarks（跳过，FinisherExecuting 存在）

玩家重攻击（可选，子弹时间内）
  → GA_Player_FinisherAttack: bPlayerConfirmed = true, 恢复时间流速

蒙太奇 HitFrame（Ability.Event.Finisher.HitFrame）
  → GA_Player_FinisherAttack.DetonateMarks:
      遍历所有 Buff.Status.Mark.Finisher 目标
      → 对每个目标: SendEvent(Action.Mark.Detonate.Finisher, Target=敌人, Magnitude=1.0或2.0)
        → FA_FinisherCard_Detonate (WaitGameplayEvent 触发):
            GetTuning(DetonationDamage=80) × Magnitude → DoDamage(敌人)
            + SendEvent(Action.Slash, 敌人)
            + [Magnitude≥2.0] SendEvent(Action.Knockback, 敌人)

蒙太奇完成
  → GA_Player_FinisherAttack.EndAbility:
      恢复时间流速
      Buff.Status.FinisherExecuting 自动移除
      (印记由 GE 自然过期，或下次 GA_FinisherCharge 启动时 ClearAllMarks 清理)
```
