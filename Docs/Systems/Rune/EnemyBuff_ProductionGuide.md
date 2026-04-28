# 敌人符文制作指南 v1.2

> 版本：v1.2（2026-04-28）
> 范围：E001–E005 敌人专属 Buff
> 资产路径：`Content/Docs/BuffDocs/EnemyBuff/`
> 授予方式：`YogGameMode::SpawnEnemyFromPool` 自动调用 `BFC->StartBuffFlow(DA->FlowAsset, Guid, Enemy)`
> FA 类型：**Category B — Actor Ability（Passive 持续型）**，从 `[Start]` 节点启动，敌人死亡时随 Actor 销毁自动 Cleanup

---

## 代码改动（已完成，无需手动操作）

| 文件 | 改动 |
|------|------|
| `Config/Tags/BuffTag.ini` | 新增 `Buff.Status.Enraged`、`Buff.Status.Cursed` |
| `Config/DefaultGameplayTags.ini` | 新增 `Ability.Event.DeathAnimComplete` |
| `Source/.../DamageExecution.cpp` | 修复 DmgTaken clamp：capture 失败才用 1.0 兜底，允许 < 1.0 减伤 |
| `Source/.../GA_Dead.cpp` | `StartDeathDelay()` 头部广播 `Ability.Event.DeathAnimComplete` |

---

## 资产总览

| ID | 名称 | 触发条件 | FA 资产 | DA 资产 |
|----|------|---------|---------|---------|
| E001 | 无畏 | HP < 75% | `FA_EnemyBuff_Fearless` | `DA_EnemyBuff_Fearless` |
| E002 | 死亡之毒 | 死亡动画播完 | `FA_EnemyBuff_DeathPoison` | `DA_EnemyBuff_DeathPoison` |
| E003 | 死咒 | 死亡时 | `FA_EnemyBuff_DeathCurse` | `DA_EnemyBuff_DeathCurse` |
| E004 | 激怒 | 首次受击 | `FA_EnemyBuff_Enrage` | `DA_EnemyBuff_Enrage` |
| E005 | 铁甲 | 随 Spawn 立即激活 | `FA_EnemyBuff_IronArmor` | `DA_EnemyBuff_IronArmor` |

---

## E001 — 无畏

**机制：** 持续监听 HP，当 HP < 75% 时一次性激活：
- `DmgTaken × 0.8`（受到伤害减少 20%）
- 授予 `Buff.Status.SuperArmor`（受击不产生硬直，YogAbilitySystemComponent 的 Poise 系统检测此 tag 跳过 HitReact 事件）

**表现：** 触发时爆发金色光芒 GC；激活期间轻微金色 Fresnel 边缘光。

### FA_EnemyBuff_Fearless — 节点连接

```
[Start]
    ↓ exec
[On Health Changed ①]          ← 每次 HP 变化时触发（持续监听）
    │ exec Out (每次 HP 变化)
    │ data  NewHealthOutput ──────────────────────────→ [Math Float ③.A]
    ↓ exec
[Get Attribute ②]               ← 读取 MaxHealth（每次都重新读，防止最大血量变化）
    Target    = BuffOwner
    Attribute = BaseAttributeSet.MaxHealth
    │ data CachedValue ───────────────────────────────→ [Math Float ③.B]
    ↓ exec
[Math Float ③]                  ← 计算 HP 比例：Health / MaxHealth
    A  = (data from ①)
    Op = ÷
    B  = (data from ②)
    │ data Result ────────────────────────────────────→ [Compare Float ④.A]
    ↓ exec
[Compare Float ④]               ← 比较是否低于 75%
    A  = (data from ③)
    Op = <
    B  = 0.75
    ↓ True
[Do Once ⑤]                     ← 只触发一次（HP 来回穿越 75% 不重复激活）
    ↓ Out
[Apply Attribute Modifier ⑥]    ← 20% 减伤
    Attribute    = BaseAttributeSet.DmgTaken
    ModOp        = Multiply
    Value        = 0.8
    DurationType = Infinite
    Target       = BuffOwner
    ↓ Out
[Add Tag ⑦]                     ← 授予霸体（无硬直）
    Tag    = Buff.Status.SuperArmor
    Target = BuffOwner
```

> **数据流连接（蓝色 data 线）：**
> - `①.NewHealthOutput` → `③.A`
> - `②.CachedValue` → `③.B`
> - `③.Result` → `④.A`
>
> **DmgTaken 减伤原理：** `DamageExecution.cpp` 公式 `FinalDamage = AttackPower × Attack × DmgTaken`，DmgTaken = 0.8 即减伤 20%。（已修复 clamp 使 < 1.0 生效）
>
> **SuperArmor 原理：** `YogAbilitySystemComponent.cpp` Poise 系统检测 `Buff.Status.SuperArmor` tag，持有时跳过 HitReact GA 的触发事件，攻击动画不被打断。

### DA_EnemyBuff_Fearless

| 字段 | 值 |
|------|----|
| RuneID | 2001 |
| DisplayName | 无畏 |
| FlowAsset | FA_EnemyBuff_Fearless |

---

## E002 — 死亡之毒

**机制：** 监听 `Ability.Event.DeathAnimComplete`（GA_Dead 在死亡动画播完后广播），触发后以死亡位置为圆心，在 300 单位半径内对所有角色施加 `GE_PoisonSplash`（中毒 DoT）。

**表现：** 死亡动画结束瞬间原地喷涌毒液 VFX（GC）；中毒目标 Fresnel 绿色边缘光。

> **事件时序：** `Die() → GA_Dead 激活 → 死亡蒙太奇播放 → OnDeathMontageCompleted/BlendOut → StartDeathDelay() → 广播 DeathAnimComplete → FA 响应 → ApplyGEInRadius → FinishBuff`

### 需要先创建的编辑器资产：GE_PoisonSplash

路径：`Content/Docs/BuffDocs/EnemyBuff/GE_PoisonSplash`

| 字段 | 值 |
|------|----|
| Duration Policy | Has Duration |
| Duration Magnitude | 6.0 |
| Period | 2.0 |
| Execute Periodic Effect on Application | false |
| Modifier — Attribute | DamageAttributeSet.DamageBuff |
| Modifier — Op | Additive |
| Modifier — Magnitude | 5.0 |
| Granted Tags | Buff.Status.Poisoned |

### FA_EnemyBuff_DeathPoison — 节点连接

```
[Start]
    ↓ exec
[Wait Gameplay Event ①]
    EventTag        = Ability.Event.DeathAnimComplete
    Target          = BuffOwner
    OnlyTriggerOnce = true
    ↓ Out
[Apply GE In Radius ②]          ← AOE 毒液溅射
    GameplayEffect = GE_PoisonSplash
    Radius         = 300.0
    Target         = BuffOwner  ← 以死亡者位置为圆心
    ↓ Out
[Finish Buff]
```

> `HasDuration` GE 由 GAS 自主管理生命周期，FA FinishBuff 后 GE 仍持续运行在受害者 ASC 上，6s 后自然到期。

### DA_EnemyBuff_DeathPoison

| 字段 | 值 |
|------|----|
| RuneID | 2002 |
| DisplayName | 死亡之毒 |
| FlowAsset | FA_EnemyBuff_DeathPoison |

---

## E003 — 死咒

**机制：** 监听自身死亡，死亡时对击杀者施加 MaxHealth × 0.9（Infinite，本关卡内），最多叠加 3 层（最多削减至 72.9% 最大生命）。

> **关卡清除说明：** GE 为 Infinite，随关卡重置（玩家死亡/重新开始）自然清除。后期如需跨关卡持续生效或主动清除，可在 GameMode 关卡切换时调用 `ASC->RemoveActiveEffectsWithGrantedTags(Buff.Status.Cursed)`。

**表现：** 死亡时紫黑咒印向击杀者飞去 GC；Cursed 状态期间血条紫色污染 UI 效果。

### FA_EnemyBuff_DeathCurse — 节点连接

```
[Start]
    ↓ exec
[Wait Gameplay Event ①]
    EventTag        = Ability.Event.Death
    Target          = BuffOwner
    OnlyTriggerOnce = true
    ↓ Out
[Apply Attribute Modifier ②]
    Attribute    = BaseAttributeSet.MaxHealth
    ModOp        = Multiply
    Value        = 0.9
    DurationType = Infinite
    GrantedTags  = Buff.Status.Cursed
    StackMode    = Stacking
    MaxStacks    = 3
    Target       = LastDamageCauser
    ↓ Out
[Finish Buff]
```

### DA_EnemyBuff_DeathCurse

| 字段 | 值 |
|------|----|
| RuneID | 2003 |
| DisplayName | 死咒 |
| FlowAsset | FA_EnemyBuff_DeathCurse |

---

## E004 — 激怒

**机制：** 首次受击时 AttackSpeed × 1.4（永久），攻击动画播放速率加快 40%（`GA_MeleeAttack` 直接读取 AttackSpeed 作为蒙太奇速率）。

**表现：** 首次受击时红色爆气 GC + 眼部发光；全程红色 Fresnel 边缘光。

### FA_EnemyBuff_Enrage — 节点连接

```
[Start]
    ↓ exec
[Wait Gameplay Event ①]
    EventTag        = Ability.Event.Damaged
    Target          = BuffOwner
    OnlyTriggerOnce = true
    ↓ Out
[Apply Attribute Modifier ②]
    Attribute    = BaseAttributeSet.AttackSpeed
    ModOp        = Multiply
    Value        = 1.4
    DurationType = Infinite
    GrantedTags  = Buff.Status.Enraged
    StackMode    = Unique
    Target       = BuffOwner
    ↓ Out
[Finish Buff]
```

### DA_EnemyBuff_Enrage

| 字段 | 值 |
|------|----|
| RuneID | 2004 |
| DisplayName | 激怒 |
| FlowAsset | FA_EnemyBuff_Enrage |

---

## E005 — 铁甲

**机制：** 随 Spawn 立即激活，无需等待触发条件。永久 ArmorHP + 80（随 Actor 死亡自动销毁）。`BaseAttributeSet::PostAttributeChange` 自动维护 `Buff.Status.Armored` tag（ArmorHP > 0 时持有，归零时移除），无需手动授予。

**表现：** Spawn 时银色护甲纹路 GC；护甲被打完时碎盾 GC。

### FA_EnemyBuff_IronArmor — 节点连接

```
[Start]
    ↓ exec
[Apply Attribute Modifier ①]
    Attribute    = BaseAttributeSet.ArmorHP
    ModOp        = Additive
    Value        = 80
    DurationType = Infinite
    StackMode    = Unique
    Target       = BuffOwner
```

> 无 FinishBuff——FA 随敌人存活期持续运行（Infinite GE 的 Cleanup 会在 Actor 销毁时自动触发）。

### DA_EnemyBuff_IronArmor

| 字段 | 值 |
|------|----|
| RuneID | 2005 |
| DisplayName | 铁甲 |
| FlowAsset | FA_EnemyBuff_IronArmor |

---

## 编辑器制作流程

### 步骤 1 — 创建目录
Content Browser 新建文件夹：`Content/Docs/BuffDocs/EnemyBuff/`

### 步骤 2 — 创建 GE_PoisonSplash（E002 专用）
右键 → GameplayEffect，按上方表格配置 DamageBuff 周期性伤害。

### 步骤 3 — 创建 FA 资产（×5）
右键 → Miscellaneous → Flow Asset，命名 `FA_EnemyBuff_*`。  
**注意：FA 第一个节点是 `[Start]`，不是 `[Custom Input]`。**

### 步骤 4 — 创建 DA 资产（×5）
右键 → Miscellaneous → Data Asset → RuneDataAsset，填写 RuneID / DisplayName / FlowAsset。

### 步骤 5 — 分配给敌人
**A. 关卡全局 Buff（所有敌人共享）：** `RoomDataAsset.BuffPool` 中添加 DA。  
**B. 敌人专属 Buff：** `EnemyData.EnemyBuffPool` 中添加 DA，Spawn 时随机选取一个激活。

---

## 参数快速调整

| 符文 | 可调参数 | 节点 |
|------|---------|------|
| 无畏 | HP 触发阈值 / 减伤比例 | ④ B 值 / ⑥ Value |
| 死亡之毒 | 溅射半径 / 毒伤值 / 持续时间 | ② Radius / GE_PoisonSplash |
| 死咒 | MaxHealth 倍率 / 叠层上限 | ② Value / ② MaxStacks |
| 激怒 | 攻速倍率 | ② Value |
| 铁甲 | 护甲初始值 | ① Value |
