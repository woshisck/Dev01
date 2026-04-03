# 符文效果系统 · 策划操作文档

> 版本：v1.0 | 日期：2026-04-03  
> 对应文档：Rune_Effects_Programmer.md  
> 前置任务：程序完成 P-02 RuneData + 对应类别基础代码后才可操作

---

## 一、符文效果类型速查表

在制作任何符文前，先确认它属于哪种类型，再跳到对应章节：

| 类型 | 你的符文是否符合？ | 跳转 |
|------|-----------------|------|
| **A 数值修改** | 只修改角色的数值属性（攻击力/移速/最大HP等） | [→ 第二章](#二a类数值修改型) |
| **B 被动事件触发** | 攻击命中/击杀/受击/闪避 时触发某个效果 | [→ 第三章](#三b类被动事件触发型) |
| **C 生成物型** | 触发时在场景中生成一个持续存在的物体（毒池/陷阱/召唤物） | [→ 第四章](#四c类生成物型) |
| **D 状态/DoT/CC** | 给目标施加随时间持续的效果（毒/燃烧/减速/眩晕） | [→ 第五章](#五d类状态效果型dotcc) |
| **E 条件触发Buff** | 满足某个条件时（血量低于30%/连杀3个）激活增益 | [→ 第六章](#六e类条件触发型) |
| **F 持续范围效果（光环）** | 在自身周围范围内持续影响所有目标 | [→ 第七章](#七f类光环aura型) |
| **G 攻击行为修改** | 改变攻击本身（穿透/弹射/分裂/追踪） | [→ 第八章](#八g类攻击行为修改型) |
| **H 死亡触发型** | 持有者死亡时触发（爆炸/毒池/灵魂逃逸） | [→ 第九章](#九h类死亡触发型) |

> **复合型符文**：一个符文可以同时是多种类型，例如"击杀时在原地生成毒池"= B类+C类。  
> → 按顺序逐类制作，然后在最终 DataAsset 中关联即可。

---

## 二、A类：数值修改型

> **前置**：程序完成 P-02（RuneData编译完成）  
> **你需要创建**：GE + DA_Rune

### 制作步骤

**Step 1：创建 GE（GameplayEffect）**

1. Content Browser 右键 → `Blueprint Class`
2. 父类搜索选 `YogGameplayEffect`
3. 命名规范：`GE_Rune_[效果名]`，例如 `GE_Rune_AttackUp`
4. 打开，配置以下字段：

| 字段 | 填写 |
|------|------|
| Duration Policy | `Infinite`（背包组件负责手动移除） |
| Modifiers（点+添加） | 见下表 |

**Modifier 配置参考：**

| 效果 | Attribute | Modifier Op | 数值 |
|------|-----------|-------------|------|
| 攻击力+10 | `BaseAttributeSet.AttackPower` | Add | 10 |
| 攻击力+20% | `BaseAttributeSet.AttackPower` | Multiply | 0.2（不是1.2） |
| 移速+50 | `BaseAttributeSet.MoveSpeed` | Add | 50 |
| 最大HP+30 | `BaseAttributeSet.MaxHealth` | Add | 30 |
| 攻速+15% | `BaseAttributeSet.AttackSpeedMultiplier` | Add | 0.15 |
| 暴击率+5% | `BaseAttributeSet.CritChance` | Add | 0.05 |
| 暴击伤害+30% | `BaseAttributeSet.CritDamageMultiplier` | Add | 0.3 |

**Step 2：创建 DA_Rune（DataAsset）**

1. 右键 → `Miscellaneous` → `Data Asset` → 选 `RuneDataAsset`
2. 命名：`DA_Rune_[效果名]`
3. 填写字段：
   - `RuneName`：符文名称
   - `RuneIcon`：图标（测试用白方块即可）
   - `RuneDescription`：描述文字
   - `Shape.Cells`：符文形状格子坐标（见下方形状参考）
   - `ActivationEffect`：选择刚才创建的 GE

**形状格子坐标参考：**

```
1×1（单格）：(0,0)

1×2（横条）：(0,0) (1,0)

2×2（正方形）：(0,0) (1,0) (0,1) (1,1)

L形（3格）：(0,0) (0,1) (1,1)

T形（5格）：(0,0) (1,0) (2,0) (1,1) (1,2)

Z形（4格）：(0,0) (1,0) (1,1) (2,1)
```

---

## 三、B类：被动事件触发型

> **前置**：程序完成 B类 GA模板代码 + 事件广播  
> **你需要创建**：GE（授予GA用）+ GA蓝图（在模板基础上）+ DA_Rune  
> **适用**：击退、命中减速、命中燃烧、击杀回血、受击反伤、闪避留印记等

### 支持的监听事件（程序广播）

| 事件名（Tag） | 触发时机 | EventData携带内容 |
|--------------|---------|----------------|
| `GameEvent.Combat.Attack.HitEnemy` | 攻击命中敌人 | Target=被击中敌人 |
| `GameEvent.Combat.Attack.HitEnemy.Crit` | 暴击命中 | Target=被击中敌人 |
| `GameEvent.Combat.Kill` | 击杀敌人 | Target=被击杀者 |
| `GameEvent.Combat.Damaged.ByEnemy` | 玩家受伤 | Magnitude=伤害量 |
| `GameEvent.Combat.Dodge.Performed` | 闪避成功 | — |

### 制作步骤

**Step 1：创建具体GA蓝图**

1. 右键 → `Blueprint Class` → 父类选 `GA_PassiveEventBase`（程序提供的模板）
2. 命名：`GA_Passive_[效果名]`，例如 `GA_Passive_Knockback`
3. 打开，在 `Details` 面板设置：
   - `Listen Event Tag` = 选择你需要监听的事件（见上表）
4. 打开 Event Graph，找到 `Event OnEventReceived` 节点（模板已有）
5. 在此事件节点后连接你的效果逻辑

**各类B型符文的节点连接方式：**

---

#### B-1：击退

```
[Event OnEventReceived]
    ↓
[Get Target from EventData] → Cast to Character
    ↓
[Get Owner Location] [Get Target Location]
    ↓
[方向 = Target Loc - Owner Loc → Normalize]
    ↓
[Launch Character]
    Character = 目标角色
    Launch Velocity = 方向 × KnockbackStrength (变量，默认800)
    bXY Override = true
    bZ Override = true
```

**可配置变量（在 Details 中暴露）：**
- `KnockbackStrength`：Float，默认 800
- `KnockbackZBoost`：Float，默认 200（让目标略微上跳）

---

#### B-2：命中减速

```
[Event OnEventReceived]
    ↓
[Get Target ASC from EventData.Target]
    ↓
[Apply Gameplay Effect to ASC]
    GE Class = GE_Debuff_Slow（先单独创建这个GE）
    Level = 1
```

**需要先创建 `GE_Debuff_Slow`（D类步骤，见第五章）：**
- Duration = 1.5s
- Modifier：MoveSpeedMultiplier Add -0.5（移速减半）

---

#### B-3：命中燃烧

```
[Event OnEventReceived]
    ↓
[Apply Gameplay Effect to EventData.Target's ASC]
    GE Class = GE_Debuff_Burn（先创建）
```

---

#### B-4：击杀回血

监听 `GameEvent.Combat.Kill`：

```
[Event OnEventReceived]
    ↓
[Apply Gameplay Effect to Self（Owner's ASC）]
    GE Class = GE_Buff_HealOnKill
```

**`GE_Buff_HealOnKill` 配置：**
- Duration Policy = Instant
- Modifier：Health Add 20（固定值）或 Set by Caller（可配比例）

---

#### B-5：受击反伤

监听 `GameEvent.Combat.Damaged.ByEnemy`：

```
[Event OnEventReceived]
    ↓
[Get Instigator from EventData] → Cast to Character（伤害来源的敌人）
    ↓
[Apply Gameplay Effect to Instigator's ASC]
    GE Class = GE_Damage_Pure（纯粹伤害，固定值）
```

---

**Step 2：创建 GE（用于激活/授予GA）**

1. 创建 `GE_Rune_[效果名]`
2. Duration Policy = `Infinite`
3. `Granted Abilities` 数组点 `+`：
   - Ability = 选你刚创建的 GA
   - Level Source = Fixed，Level = 1
   - Removal Policy = Cancel Ability Immediately

**Step 3：创建 DA_Rune**

同 A类 Step 2，`ActivationEffect` 选 `GE_Rune_[效果名]`

---

## 四、C类：生成物型

> **前置**：程序完成 C类 Actor基类 + 对应具体Actor蓝图  
> **你需要创建**：DataTable（等级参数）+ 配置Actor细节 + GE + DA_Rune  
> **适用**：腐烂毒池、死亡爆炸、触发陷阱、荆棘区域、冰锥区域等

### 腐烂毒池（完整案例）

#### Step 1：创建等级参数 DataTable

1. 右键 → `Miscellaneous` → `Data Table`
2. Row Structure 选 `RotPoolLevelData`（程序提供）
3. 命名：`DT_RotPool_Levels`
4. 添加3行数据（RowName 必须是 `Level1`、`Level2`、`Level3`）：

| RowName | MaxRadius | ExpandDuration | PoolDuration | DamageTickInterval | DamagePctToEnemy | DamagePctToBoss | DamageToPlayer |
|---------|-----------|---------------|-------------|-------------------|-----------------|----------------|---------------|
| Level1  | 400       | 0.5           | 5.0         | 0.25              | 0.07            | 0.03           | 0             |
| Level2  | 500       | 0.5           | 5.0         | 0.25              | 0.09            | 0.04           | 0             |
| Level3  | 600       | 0.4           | 6.0         | 0.2               | 0.12            | 0.05           | 0             |

> `DamagePctToEnemy` = 每秒损失当前HP的百分比（0.07 = 7%/s）  
> `DamageTickInterval` = 0.25 表示每0.25秒触发一次，程序会自动折算

#### Step 2：配置 BP_RotPoisonPool Actor

程序会创建好 `BP_RotPoisonPool` 蓝图框架，你需要填写：

1. 打开 `BP_RotPoisonPool`
2. 在 `Details` 面板找到 `Level Data Table` → 选 `DT_RotPool_Levels`
3. 找到视效组件（NiagaraComponent 或 ParticleSystem）→ 指定毒池视效（测试用绿色球体即可）

#### Step 3：创建 GE_Rune_RotPool

同 B类 Step 2（授予 `GA_Passive_RotPool`）

#### Step 4：创建 DA_Rune_RotPool

- 这个 DataAsset 上还需要额外设置：`Rune Level`（1/2/3，决定读取哪行DataTable）

---

### 通用生成物制作模板

程序每制作一种新的生成物 Actor，你需要配置：

1. 对应的 `DT_[名称]_Levels` DataTable（和程序约定字段）
2. 视效/材质指定（可用临时占位）
3. `GE_Rune_[名称]` → 授予 `GA_Passive_[名称]`
4. `DA_Rune_[名称]` → 关联 GE

---

## 五、D类：状态效果型（DoT/CC）

> **前置**：程序确认 AttributeSet 有 MoveSpeedMultiplier / AttackSpeedMultiplier + 眩晕Tag响应  
> **你需要创建**：GE（直接配置，无需GA）

### D-1：持续毒伤（DoT）

创建 `GE_Debuff_Poison`：

| 字段 | 值 |
|------|---|
| Duration Policy | Has Duration |
| Duration Magnitude | Scalable Float = 5（持续5秒） |
| Period | 0.5（每0.5秒触发一次） |
| Execute Periodic On Application | true（立即触发第一次） |
| Modifier Attribute | `BaseAttributeSet.Health` |
| Modifier Op | Add |
| Magnitude Type | Scalable Float = -5（每次-5HP） |

> 注意：Health修改要填负数才是扣血

### D-2：燃烧（随时间加速）

创建 `GE_Debuff_Burn`：  
同毒伤，但每次伤害更高，时间更短（2s，每0.33s触发，每次-8HP）

**额外效果（视觉Tag）：**
- `Gameplay Cues` → `+` → Tag填 `GameplayCue.Debuff.Burn`（程序配置特效时用）

### D-3：减速

创建 `GE_Debuff_Slow`：

| 字段 | 值 |
|------|---|
| Duration Policy | Has Duration |
| Duration Magnitude | 2.0 |
| Modifier Attribute | `BaseAttributeSet.MoveSpeedMultiplier` |
| Modifier Op | Add |
| Magnitude | -0.5（减速50%） |

### D-4：眩晕

创建 `GE_Debuff_Stun`：

| 字段 | 值 |
|------|---|
| Duration Policy | Has Duration |
| Duration Magnitude | 1.5 |
| **Granted Tags** | `State.Debuff.Stun`（程序监听此Tag禁用角色输入） |

> Granted Tags 在 GE 详情页的 `Tags` → `Granted Tags to Target` 中添加

### D-5：冻结（完全停止）

创建 `GE_Debuff_Freeze`：

| 字段 | 值 |
|------|---|
| Duration Policy | Has Duration |
| Duration Magnitude | 2.0 |
| Granted Tags | `State.Debuff.Freeze` |

### DoT/CC 作为独立Debuff vs 作为符文激活效果

- **独立Debuff**（由B类符文施加给敌人）：不需要 DA_Rune，只需要 GE
- **作为符文本身效果**（玩家背包中的符文，激活时持续给自身加Buff）：  
  创建 DA_Rune，`ActivationEffect` 指向这个 GE

---

## 六、E类：条件触发型

> **前置**：程序完成 E类 GA 模板  
> **适用**：血量阈值增伤、连杀计数、多敌人环绕时触发等

### E-1：血量低于阈值时增伤

**Step 1：创建条件触发的增益 GE**

创建 `GE_Buff_BerserkMode`（血量低时的增益）：
- Duration Policy = `Infinite`（手动移除）
- Modifier：AttackPower Add 20（或Multiply 0.5 = +50%）

**Step 2：创建 GA_Passive_LowHpBuff**

1. 父类选 `GA_PassiveEventBase`（或程序为E类提供的专用模板）
2. 暴露变量：
   - `HpThreshold`：Float = 0.3（30%时触发）
   - `BuffEffect`：选 `GE_Buff_BerserkMode`
3. 蓝图逻辑（程序模板已有框架，你只需填变量）

**Step 3：创建 GE_Rune_LowHpBuff → DA_Rune_BerserkRune**（同B类流程）

---

### E-2：连杀 Buff

**可配置变量：**
- `StreakThreshold`：Int = 3（连杀3个触发）
- `ResetDelay`：Float = 4.0（4秒内没有击杀则重置）
- `BuffEffect`：选对应增益GE

---

### E-3：多敌人环绕时触发（围攻反制）

**可配置变量：**
- `EnemyCountThreshold`：Int = 3（周围≥3个敌人时触发）
- `DetectionRadius`：Float = 300
- `BuffEffect`：触发的增益

---

## 七、F类：光环（Aura）型

> **前置**：程序完成 F类 AuraBase 代码  
> **适用**：持续减速光环、回血光环、吸收伤害光环

### 光环型符文制作步骤

**Step 1：创建施加给目标的 GE（光环效果本体）**

例如 `GE_Aura_SlowField`：
- Duration Policy = `Infinite`（由光环GA手动移除）
- Modifier：MoveSpeedMultiplier Add -0.3（减速30%）

**Step 2：创建 GA_Passive_[光环名]**

1. 父类选 `GA_Passive_AuraBase`
2. 填写变量：

| 变量 | 说明 | 示例值 |
|------|------|--------|
| AuraRadius | 光环半径 | 350 |
| TickInterval | 检测间隔（秒） | 0.5 |
| AuraEffect | 施加给目标的GE | GE_Aura_SlowField |
| bAffectEnemies | 是否影响敌人 | ✓ |
| bAffectAllies | 是否影响队友 | ✗ |

**Step 3：创建 GE_Rune_[光环名] → DA_Rune**（同B类）

### 光环案例参考

| 符文名 | AuraEffect 内容 | 影响目标 | 半径 |
|--------|----------------|---------|------|
| 迟缓领域 | 移速-30% | 敌人 | 350 |
| 腐蚀光环 | 防御-20% | 敌人 | 250 |
| 生命共鸣 | 每0.5s回5HP | 友方 | 300 |
| 恐惧气场 | 附加恐惧Tag（敌人AI逃跑） | 敌人 | 400 |

---

## 八、G类：攻击行为修改型

> **前置**：程序完成攻击 Hook（AttackParams系统）  
> **适用**：穿透、弹射、分裂、追踪投射物

### G类符文制作步骤

**Step 1：创建 GA_Passive_[攻击修改名]**

1. 父类选 `GA_PassiveAttackModBase`（程序提供）
2. 在 Details 中勾选对应修改选项：

| 变量 | 说明 | 典型值 |
|------|------|--------|
| bPiercing | 投射物穿透 | true/false |
| PierceCount | 最多穿透几个目标 | 2 |
| BounceCount | 弹射次数 | 2 |
| bHoming | 是否追踪敌人 | true/false |
| ProjectileCountAdd | 额外发射数量 | +1（三连发） |
| SpreadAngle | 散布角度 | 15（度） |
| SizeMultiplier | 投射物大小倍率 | 1.5 |

**Step 2：GE + DA**（同B类）

### G类案例

| 符文名 | 修改方式 | 参数 |
|--------|---------|------|
| 穿刺弹 | 穿透2个目标 | bPiercing=true, PierceCount=2 |
| 弹射弹 | 弹射2次 | BounceCount=2 |
| 三叉弹 | 3发散射 | ProjectileCountAdd=2, SpreadAngle=20 |
| 追踪弹 | 自动追踪最近敌人 | bHoming=true |
| 巨型弹 | 投射物放大 | SizeMultiplier=2.0 |

---

## 九、H类：死亡触发型

> **前置**：程序完成死亡事件广播 + C类Actor基类  
> **适用**：腐烂毒池（死亡版）、死亡爆炸、亡灵化、灵魂逃逸

**H类与C类区别：**
- C类：由玩家符文激活时触发（如攻击后生成陷阱）
- H类：由持有者死亡时触发（敌人身上的符文，死亡后才生效）

### H-1：死亡爆炸

**Step 1：创建 GA_Passive_DeathExplosion**

- 父类 `GA_PassiveEventBase`
- `Listen Event Tag` = `GameEvent.Life.Death.Self`
- `OnEventReceived` 节点后：
  - Spawn Actor `BP_ExplosionArea` at Owner Location
  - Init with Level

**Step 2：创建 DT_Explosion_Levels DataTable**

| RowName | Radius | Damage | Duration |
|---------|--------|--------|---------|
| Level1  | 300    | 50     | 0.3     |
| Level2  | 400    | 80     | 0.3     |
| Level3  | 500    | 120    | 0.3     |

**Step 3：GE_Rune_DeathExplosion → DA_Rune（挂在敌人BP上）**

> 敌人符文的 DA_Rune 不会出现在玩家背包中，而是在敌人BP的 `InitialRunes` 数组中配置。  
> 程序会在敌人生成时自动激活其所有符文。

---

### H-2：腐烂毒池（敌人死亡版）

与第四章腐烂毒池完全相同，只是：
- GA 监听的事件改为 `GameEvent.Life.Death.Self`
- DA 挂在敌人BP上而非玩家背包

---

## 十、复合型符文案例

### 案例：「吸血蝙蝠」符文

- **命中时**对敌人施加出血DoT（B类 + D类）
- **击杀时**回复最大HP 5%（B类）

**制作：**
1. 创建 `GE_Debuff_Bleed`（D类DoT，1.5s，每0.25s扣当前HP 2%）
2. 创建 `GE_Buff_LifeSteal`（Instant，Health Add，值由程序传入比例计算）
3. 创建 `GA_Passive_VampireBat`：
   - 监听 `GameEvent.Combat.Attack.HitEnemy` → 对Target施加 GE_Debuff_Bleed
   - 监听 `GameEvent.Combat.Kill` → 对自身施加 GE_Buff_LifeSteal
4. 创建 `GE_Rune_VampireBat`（授予上面的GA）
5. 创建 `DA_Rune_VampireBat`

---

### 案例：「荆棘护甲」符文

- **受击时**对攻击者造成反伤（B类）
- **持续保持**防御属性+5（A类）

**制作：**
1. 创建 `GE_Rune_ThornArmor`：
   - Modifier：Defense Add 5（A类效果）
   - Granted Abilities：GA_Passive_ThornsCounter
2. GA_Passive_ThornsCounter 监听 `GameEvent.Combat.Damaged.ByEnemy` → 反伤

---

## 十一、命名规范速查

| 资产类型 | 命名格式 | 例子 |
|---------|---------|------|
| GE（激活GE） | `GE_Rune_[效果名]` | `GE_Rune_Knockback` |
| GE（Debuff） | `GE_Debuff_[效果名]` | `GE_Debuff_Slow` |
| GE（Buff） | `GE_Buff_[效果名]` | `GE_Buff_BerserkMode` |
| GE（伤害） | `GE_Damage_[类型]` | `GE_Damage_Poison` |
| GA | `GA_Passive_[效果名]` | `GA_Passive_Knockback` |
| Actor | `BP_[效果名]` | `BP_RotPoisonPool` |
| DataTable | `DT_[名称]_Levels` | `DT_RotPool_Levels` |
| DataAsset | `DA_Rune_[效果名]` | `DA_Rune_Knockback` |

---

## 十二、等级系统配置说明

当符文需要等级（C/E/F/G/H类）：

1. DataTable 行名固定格式：`Level1`、`Level2`、`Level3`
2. GA 的 `Ability Level` 由背包组件传入（`RuneInstance.Level`）
3. 策划在 `DA_Rune_*` 中设置 `Rune Level` 字段（默认1）
4. 升级机制（如何获得高等级符文）由关卡掉落池的配置决定（见 DA_LevelSequence）

---

## 十三、常见问题

**Q：GE 没有生效，符文放进背包了但没有效果？**  
A：检查 `BackpackGridComponent` 是否触发了 `ActivateRune`（需要符文在激活区内），确认 GE 的 Duration Policy = Infinite。

**Q：GA 不知道有没有在运行？**  
A：运行游戏后，选中玩家 → 在 Details 面板找 Gameplay Ability System Component → 展开 Activatable Abilities，确认 GA 名字在列表中。

**Q：DoT 只打了一次就停了？**  
A：检查 GE 的 `Period` 是否填了（不填 Period 则只触发一次），`Execute Periodic On Application` 建议勾选。

**Q：光环效果对自己也生效了？**  
A：在 GA_Passive_AuraBase 的变量中，检查 `bAffectAllies` 是否意外勾选。

**Q：死亡毒池不生成？**  
A：确认该GE挂在敌人的 `InitialRunes` 中（由程序在敌人BP配置），或确认死亡事件 `GameEvent.Life.Death.Self` 已被正确广播（找程序确认）。

---

*文档版本 v1.0 | 对应程序文档：Rune_Effects_Programmer.md*