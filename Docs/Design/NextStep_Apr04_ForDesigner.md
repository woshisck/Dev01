# 后续工作方案 · 策划版
**日期：** 2026-04-04  
**前置：编译成功后才可进行以下操作**

---

## 一、今天已完成 ✅

| 功能 | 状态 |
|------|------|
| 背包背格子系统（BackpackGridComponent） | ✅ 代码完整 |
| 符文激活/取消激活（GE施加/移除） | ✅ 代码完整 |
| 关卡流程（战斗→整理→下一关） | ✅ 代码完整 |
| 刷怪系统（难度分预算波次） | ✅ 代码完整 |
| BuffFlow 符文效果 Flow Graph 系统 | ✅ 今天新增 |
| 击退符文资产（DA/GA/GE） | ✅ 今天创建 |

---

## 二、P0 — 编译成功后立即要做

### 2.1 GameplayTag 注册（必须）

**打开方式：** UE编辑器 → Edit → Project Settings → GameplayTags → Manage Gameplay Tags

检查并添加以下 Tag（可以在 `Config/Tags/` 下对应 .ini 手动填）：

**Buff.ini（符文激活Tag）：**
```
Rune.ZhanDouKewang.Active   → 战斗渴望符文激活标记
Rune.QuanNeng.Active        → 全能符文激活标记
Rune.FenLiYiJi.Active      → 奋力一击符文激活标记
Rune.TuXi.Active            → 突袭符文激活标记
```

**YogEffect.ini（游戏事件Tag）：**
```
GameEvent.Combat.Attack.HitEnemy         → 命中事件
GameEvent.Combat.Attack.HitEnemy.Crit    → 暴击命中事件
GameEvent.Combat.Kill                    → 击杀事件
GameEvent.Combat.Damaged.ByEnemy         → 受伤事件
GameEvent.Combat.Dodge.Performed         → 闪避事件
GameEvent.Life.Death.Self                → 自身死亡事件
GameEvent.Life.Death.EnemyNearby         → 附近敌人死亡事件
GameEvent.Combat.Attack.Begin            → 攻击开始事件
```

**PlayerGameplayTag.ini（角色状态Tag）：**
```
State.DoubleHit          → 双重打击状态（下次攻击双倍）
State.Debuff.Stun        → 眩晕（禁止移动和攻击）
State.Debuff.Freeze      → 冻结（完全停止）
Action.Combo.LastHit     → 连击最后一击
```

**Data.ini（SetByCaller数值Tag）：**
```
Data.ZhanDouKewang.AttackSpeedBonus   → 战斗渴望攻速加成
Data.QuanNeng.CritRateBonus           → 全能暴击率加成
SetByCaller.Damage.Physical           → 物理伤害
SetByCaller.Damage.Poison             → 毒伤
SetByCaller.Heal                      → 治疗量
```

---

### 2.2 WBP_LootSelection 蓝图步骤

打开 `Content/UI/Playtest_UI/WBP_LootSelection`：

**第一步：添加界面控件（如果还没有）**

在 Designer 面板搭建以下结构：
```
[Canvas]
  └─ [VerticalBox]
       ├─ [TextBlock] Title = "选择一个符文"
       ├─ [HorizontalBox]
       │    ├─ [Button] btn_Loot_0
       │    │    └─ [VerticalBox]
       │    │         ├─ [Image]       img_Icon_0
       │    │         ├─ [TextBlock]   txt_Name_0
       │    │         └─ [TextBlock]   txt_Desc_0
       │    ├─ [Button] btn_Loot_1  （同上）
       │    └─ [Button] btn_Loot_2  （同上）
       └─ [Button] btn_Confirm = "确认整理，进入下一关"
```

**第二步：Event Graph 绑定**

```
Event Construct
  ↓
[Get Game Mode] → Cast to YogGameMode
  ↓
[Bind Event to OnLootGenerated]
    Event: OnLootGenerated → [自定义事件 OnLootOptionsReceived(LootOptions)]
```

**第三步：OnLootOptionsReceived 事件实现**

```
[Event OnLootOptionsReceived]  参数: LootOptions (Array<FLootOption>)
  ↓
[For Each Loop] index 0/1/2:
  ↓
  [Get LootOptions[i]] → 拿到 FLootOption
  ├─ [Set Texture] img_Icon_i ← LootOption.RuneData.RuneIcon
  ├─ [Set Text]    txt_Name_i ← LootOption.RuneData.RuneName
  └─ [Set Text]    txt_Desc_i ← LootOption.RuneData.RuneDescription
```

**第四步：三个按钮的点击事件**

```
[btn_Loot_0 OnClicked]
  ↓
[Get Game Mode] → Cast to YogGameMode
  ↓
[SelectLoot(0)]

（btn_Loot_1 传 1，btn_Loot_2 传 2）
```

**第五步：确认按钮**

```
[btn_Confirm OnClicked]
  ↓
[Get Game Mode] → Cast to YogGameMode
  ↓
[ConfirmArrangementAndTransition]
```

**第六步：在哪里显示这个 UI？**

在 GameMode BP 里，或者在玩家 BP 里，绑定 `OnPhaseChanged` 委托：
```
[Bind Event to OnPhaseChanged]
    Event: OnPhaseChanged(NewPhase)
      ↓
      If NewPhase == Arrangement:
        [Create Widget WBP_LootSelection] → [Add to Viewport]
      If NewPhase == Combat:
        [Remove WBP_LootSelection from parent]（如果有引用的话）
```

---

### 2.3 符文 DataAsset 配置（击退符文测试用）

打开 `Content/Docs/BuffDocs/Playtest_GA/KnockBack/DA_Rune_Knockback`：

| 字段 | 填写 |
|------|------|
| RuneName | 击退符文（测试用） |
| RuneIcon | 任意图标 |
| RuneDescription | 命中时将目标向后击飞 |
| Shape.Cells | (0,0) (1,0) （1×2横条） |
| ActivationEffect | GE_Rune_Knockback |
| **ActiveTag** | `Rune.Knockback.Active`（程序新增字段，等程序加好后填） |
| BuffFlowAsset | 暂时留空（击退在GA里实现） |

---

### 2.4 GA_Passive_knockback 蓝图步骤（必须！击退还没接上）

打开 `Content/Docs/BuffDocs/Playtest_GA/KnockBack/GA_Passive_knockback`：

**Details 面板设置：**
- 父类：确认是 `GA_PassiveEventBase`（或 `RunePassiveAbility`）
- `Listen Event Tag`：`GameEvent.Combat.Attack.HitEnemy`
- Instancing Policy：`Instanced Per Actor`

**Event Graph 实现（OnEventReceived 节点之后）：**

```
[Event OnEventReceived]  参数: EventData
  ↓
[Get Target from EventData] → Cast to Character
  ↓（成功才继续）
[Get Owner] → Get Actor Location    [Get Target Actor Location]
  ↓
Direction = [Target Loc - Owner Loc] → [Normalize]
  ↓
KnockVelocity = Direction × KnockbackStrength (800) + Up × KnockbackZBoost (200)
  ↓
[Launch Character]
  Character = 目标角色
  Launch Velocity = KnockVelocity
  bOverrideXY = true
  bOverrideZ = true
```

**可配置变量（在 Details 里暴露）：**
- `KnockbackStrength`：Float，默认 800
- `KnockbackZBoost`：Float，默认 200

---

## 三、P1 — 之后要做

### 3.1 其余符文 DataAsset 创建（按照 Rune_Effects_Designer.md）

优先创建用于测试的 A类符文（最简单）：

| 资产 | 配置 |
|------|------|
| `GE_Rune_AttackUp` | Duration=Infinite，Modifier: AttackPower Add 10 |
| `DA_Rune_AttackUp` | Shape=(0,0)，ActivationEffect=GE_Rune_AttackUp |

创建 2-3 个不同类型的符文用于：
- 三选一掉落池测试
- 背包格子放置测试

### 3.2 DA_Room_Prison_Normal 完整填充

目前已创建，需要填充：
- `EnemyPool`：拖入至少1种敌人类 + 难度分（建议 3 分）
- `LootPool`：拖入至少 3 个 DA_Rune（包括新建的 AttackUp + Knockback）
- `LowConfig.AllowedTriggers`：添加 AllEnemiesDead（0分）
- `LowConfig.AllowedSpawnModes`：添加 Wave（1分）
- `LowConfig.WaveBudgets`：[15, 20]
- `LowConfig.GoldMin/Max`：10 / 20

### 3.3 DA_Campaign_MainRun 完整填充

| FloorNumber | RoomData | Difficulty | LevelName |
|-------------|----------|------------|-----------|
| 1 | DA_Room_Prison_Normal | Low | Level_Prison（填你地图的实际名字） |
| 2 | DA_Room_Prison_Normal | Low | Level_Prison |

---

## 四、验收标准（4月15日 Demo）

- [ ] 进游戏 → 关卡自动刷怪（波次系统）
- [ ] 打完一波 → 下一波自动触发
- [ ] 所有波次结束 → 弹出三选一 UI
- [ ] 选符文 → 符文进入背包待放置区
- [ ] 在背包格子上拖放符文 → 符文进入激活区时生效（数值提升 / BuffFlow 启动）
- [ ] 点确认 → 加载下一关
- [ ] 击退符文放在激活区内 → 打怪时触发击飞效果

---

## 五、命名规范速查

| 类型 | 格式 | 例子 |
|------|------|------|
| 符文激活 GE | `GE_Rune_[效果名]` | `GE_Rune_Knockback` |
| 被动 GA | `GA_Passive_[效果名]` | `GA_Passive_Knockback` |
| 符文 DataAsset | `DA_Rune_[效果名]` | `DA_Rune_Knockback` |
| 房间配置 | `DA_Room_[场景名]_[类型]` | `DA_Room_Prison_Normal` |
| 关卡序列 | `DA_Campaign_[run名]` | `DA_Campaign_MainRun` |
| BuffFlow Asset | `FA_[效果名]` | `FA_Test_Log` |

---

*对应程序文档：NextStep_Apr04_ForProgrammer.md*
