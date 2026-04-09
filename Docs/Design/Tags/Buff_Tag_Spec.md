# Buff Tag 规范文档

> 适用范围：符文系统（Rune）所有 GameplayTag 的命名、分层与使用规则。  
> 配套文件：`Config/Tags/BuffTag.ini`  
> 更新日期：2026-04-07

---

## 一、总览：四层职责模型

所有 `Buff.*` 标签按**职责**划分为四层，每一层只做一件事。

```
Buff.*
├── Buff.Rune.*       【身份层】符文数据资产（DA）的分类标签
├── Buff.Effect.*     【行为层】GE/GA 的效果描述标签
├── Buff.Trigger.*    【触发层】GA 触发时机的过滤标签
├── Buff.Status.*     【状态层】运行时挂在角色 ASC 上的状态标签
└── Buff.Data.*       【参数层】SetByCaller 的值传递通道键
```

> **核心原则**：同一个 Tag **只服务于一种职责**。  
> 不要把身份标签挂到 ASC 上，也不要用状态标签去描述 GE 是什么类型。

---

## 二、现有层级速查表

### 身份层 `Buff.Rune.*`

| Tag | 层级 | 说明 |
|-----|------|------|
| `Buff.Rune.Type` | 父节点 | 功能大类，每个符文必须且只有一个 |
| `Buff.Rune.Type.Attack` | 叶子 | 增强攻击/伤害输出的符文 |
| `Buff.Rune.Type.Defense` | 叶子 | 护盾、减伤、生存相关的符文 |
| `Buff.Rune.Type.Utility` | 叶子 | 移速、冷却、能量类的功能性符文 |
| `Buff.Rune.Type.Summon` | 叶子 | 召唤实体或创建持续对象的符文 |
| `Buff.Rune.Rarity` | 父节点 | 稀有度，每个符文必须且只有一个 |
| `Buff.Rune.Rarity.Common` | 叶子 | 普通，基础数值，掉落频率高 |
| `Buff.Rune.Rarity.Rare` | 叶子 | 稀有，中等强度，有附加条件 |
| `Buff.Rune.Rarity.Epic` | 叶子 | 史诗，高强度或独特机制，稀少掉落 |
| `Buff.Rune.Element` | 父节点 ⚠️预留 | 元素属性，当前无克制逻辑 |
| `Buff.Rune.Element.Fire` | 叶子 ⚠️预留 | 火元素 |
| `Buff.Rune.Element.Ice` | 叶子 ⚠️预留 | 冰元素 |
| `Buff.Rune.Element.Lightning` | 叶子 ⚠️预留 | 雷元素 |
| `Buff.Rune.Element.Void` | 叶子 ⚠️预留 | 虚空/无属性 |

### 行为层 `Buff.Effect.*`

| Tag | 层级 | 说明 |
|-----|------|------|
| `Buff.Effect.Damage` | 父节点 | 造成伤害的 GE 共同父节点 |
| `Buff.Effect.Damage.Physical` | 叶子 | 物理伤害，受防御减免 |
| `Buff.Effect.Damage.Elemental` | 叶子 | 元素伤害（预留元素克制接口） |
| `Buff.Effect.Damage.True` | 叶子 | 真实伤害，无视防御和抗性 |
| `Buff.Effect.Damage.Reflected` | 叶子 | 反射伤害，来源为受到伤害的百分比 |
| `Buff.Effect.Heal` | 父节点 | 恢复生命值的 GE 父节点 |
| `Buff.Effect.Heal.Instant` | 叶子 | 即时回血 |
| `Buff.Effect.Heal.Periodic` | 叶子 | 持续回血（Tick） |
| `Buff.Effect.Shield` | 叶子 | 添加护盾值（独立血条或吸收层） |
| `Buff.Effect.Attribute` | 父节点 | 修改角色属性的 GE 父节点 |
| `Buff.Effect.Attribute.AttackSpeed` | 叶子 | 攻击速度倍率 |
| `Buff.Effect.Attribute.MoveSpeed` | 叶子 | 移动速度 |
| `Buff.Effect.Attribute.MaxHP` | 叶子 | 最大生命值上限 |
| `Buff.Effect.Attribute.CritRate` | 叶子 | 暴击概率 |
| `Buff.Effect.Attribute.CritDamage` | 叶子 | 暴击伤害倍率 |
| `Buff.Effect.Attribute.MaxCharge` | 叶子 | 技能最大充能格数（对应 Max{SkillName}Charge 属性） |
| `Buff.Effect.Attribute.CooldownDuration` | 叶子 | 技能 CD 间隔（对应 {SkillName}CooldownDuration 属性，Multiplicative 缩短） |
| `Buff.Effect.Summon` | 叶子 | 在场景中生成持续对象（实体/投射物/区域） |

### 触发层 `Buff.Trigger.*`

| Tag | 层级 | 说明 |
|-----|------|------|
| `Buff.Trigger.Passive` | 叶子 | 被动常驻，装备即生效，无需事件 |
| `Buff.Trigger.OnHit` | 叶子 | 成功命中敌人时触发 |
| `Buff.Trigger.OnKill` | 叶子 | 击杀敌人时触发 |
| `Buff.Trigger.OnTakeDamage` | 叶子 | 自身受到伤害时触发 |
| `Buff.Trigger.OnSkillCast` | 叶子 | 主动施放技能时触发（不含普攻） |
| `Buff.Trigger.OnDash` | 叶子 | 闪避/冲刺时触发 |
| `Buff.Trigger.OnComboEnd` | 叶子 | 连招结束（最后一击判定）时触发 |

### 状态层 `Buff.Status.*`

| Tag | 层级 | 说明 |
|-----|------|------|
| `Buff.Status.Heat` | 父节点 | 热度相关状态总父节点 |
| `Buff.Status.Heat.Active` | 叶子 | 战斗保热守卫：持有时 C++ 阻断 GE_HeatDecay 执行 |
| `Buff.Status.Heat.Phase` | 父节点 | 热度阶段状态，同一时刻只有一个子节点存在 |
| `Buff.Status.Heat.Phase.1` | 叶子 | 第一阶段：基础状态 |
| `Buff.Status.Heat.Phase.2` | 叶子 | 第二阶段：效果增强，部分符文解锁附加行为 |
| `Buff.Status.Heat.Phase.3` | 叶子 | 第三阶段：满阶，符文效果最大化 |
| `Buff.Status.Shielded` | 叶子 | 当前有护盾层 |
| `Buff.Status.HitReact` | 叶子 | 受击硬直状态，GA_HitReaction ActivationOwnedTags 挂载，驱动移动阻断与技能冲突 |
| `Buff.Status.Dead` | 叶子 | 死亡状态，GA_Dead ActivationOwnedTags 挂载，阻断所有 GA + 停止移动 |
| `Buff.Status.Knockback` | 叶子 | 击退硬直状态，GA_Knockback ActivationOwnedTags 挂载，驱动移动阻断 |
| `Buff.Status.CC` | 父节点 ⚠️预留 | 控制类状态父节点，当前无控制机制 |
| `Buff.Status.CC.Stun` | 叶子 ⚠️预留 | 眩晕，无法行动 |
| `Buff.Status.CC.Frozen` | 叶子 ⚠️预留 | 冰冻，受额外元素伤害 |

> ⚠️预留 = 已定义 Tag，当前无对应系统逻辑，可安全填写但不会产生效果。

### 参数层 `Buff.Data.*`

| Tag | 层级 | 说明 |
|-----|------|------|
| `Buff.Data.Damage` | 叶子 | 伤害量，传给伤害类 GE 的 SetByCaller 键 |
| `Buff.Data.Heal` | 叶子 | 治疗量，传给治疗类 GE 的 SetByCaller 键 |
| `Buff.Data.ShieldAmount` | 叶子 | 护盾量，传给护盾类 GE 的 SetByCaller 键 |
| `Buff.Data.Duration` | 叶子 | 效果持续时间（秒） |
| `Buff.Data.AttributeMod` | 叶子 | 属性修改量，配合 ApplyAttributeModifier 使用 |

---

## 三、通用命名规则

| 规则 | 说明 | 示例 |
|------|------|------|
| 大写驼峰 | 每段首字母大写，其余小写 | `Buff.Effect.Damage.Physical` ✅ |
| 不用缩写 | 全拼，除非行业标准缩写 | `AttackSpeed` 不写 `AtkSpd` |
| 英文单数 | 分类节点用单数名词 | `Rune` 不写 `Runes` |
| 不超过 5 层 | 超过则考虑重新划分 | 4 层以内最佳 |
| DevComment 必填 | 叶子节点说明用途，父节点说明范围 | 见各层规范 |

---

## 四、各层详细规范

---

### 3.1 身份层 `Buff.Rune.*`

**职责**：描述"这个符文**是什么**"，贴在 `RuneDataAsset` 的 `IdentityTags` 字段上。  
**用于**：背包 UI 过滤、互斥/叠加判断、掉落权重筛选。  
**不用于**：运行时状态、GE 效果描述。

#### 3.1.1 功能大类 `Buff.Rune.Type.*`

描述符文的**主要玩法功能**。每个符文必须有且只有一个 Type 标签。

```ini
; 填写规范：Buff.Rune.Type.<功能名>
; DevComment 格式：【Type】简述该类符文的主要作用

GameplayTagList=(Tag="Buff.Rune.Type",            DevComment="符文功能大类，每个符文必须且只有一个")
GameplayTagList=(Tag="Buff.Rune.Type.Attack",      DevComment="【Type】增强攻击/伤害输出的符文")
GameplayTagList=(Tag="Buff.Rune.Type.Defense",     DevComment="【Type】护盾、减伤、生存相关的符文")
GameplayTagList=(Tag="Buff.Rune.Type.Utility",     DevComment="【Type】移速、冷却、能量类的功能性符文")
GameplayTagList=(Tag="Buff.Rune.Type.Summon",      DevComment="【Type】召唤实体或创建持续对象的符文")
```

> **新增规则**：新增 Type 前先确认现有分类是否能覆盖。Type 数量建议控制在 6 个以内。

---

#### 3.1.2 稀有度 `Buff.Rune.Rarity.*`

描述符文的**获取难度和强度区间**。每个符文必须有且只有一个 Rarity 标签。

```ini
; 填写规范：Buff.Rune.Rarity.<等级名>
; DevComment 格式：【Rarity】数值强度区间说明

GameplayTagList=(Tag="Buff.Rune.Rarity",           DevComment="符文稀有度，每个符文必须且只有一个")
GameplayTagList=(Tag="Buff.Rune.Rarity.Common",    DevComment="【Rarity】普通，基础数值，掉落频率高")
GameplayTagList=(Tag="Buff.Rune.Rarity.Rare",      DevComment="【Rarity】稀有，中等强度，有附加条件")
GameplayTagList=(Tag="Buff.Rune.Rarity.Epic",      DevComment="【Rarity】史诗，高强度或独特机制，稀少掉落")
```

---

#### 3.1.3 元素属性 `Buff.Rune.Element.*`（预留接口）

描述符文的**元素归属**，用于未来元素克制/共鸣系统。  
**当前状态**：已定义节点，暂不赋予实际逻辑，可以安全填写标签但不需要接入系统。

```ini
; 填写规范：Buff.Rune.Element.<元素名>
; DevComment 格式：【Element-Reserved】说明当前未激活

GameplayTagList=(Tag="Buff.Rune.Element",              DevComment="元素属性（预留接口，当前无克制逻辑）")
GameplayTagList=(Tag="Buff.Rune.Element.Fire",         DevComment="【Element-Reserved】火元素")
GameplayTagList=(Tag="Buff.Rune.Element.Ice",          DevComment="【Element-Reserved】冰元素")
GameplayTagList=(Tag="Buff.Rune.Element.Lightning",    DevComment="【Element-Reserved】雷元素")
GameplayTagList=(Tag="Buff.Rune.Element.Void",         DevComment="【Element-Reserved】虚空/无属性")
```

---

### 3.2 行为层 `Buff.Effect.*`

**职责**：描述"这个 GE/GA **做了什么**"，贴在 GE 的 `AssetTags` 字段上。  
**用于**：Flow Graph BFNode 查询（"找出所有伤害类效果"）、GA 的 `CancelAbilitiesWithTag` / `BlockAbilitiesWithTag`。  
**不用于**：符文身份分类、运行时角色状态。

> 一个 GE 可以有**多个** Effect 标签（如同时是 `Damage.Physical` + `Damage.Elemental`）。

#### 3.2.1 伤害类 `Buff.Effect.Damage.*`

```ini
; 填写规范：Buff.Effect.Damage.<伤害类型>
; DevComment 格式：【Effect-Damage】伤害计算说明或特殊规则

GameplayTagList=(Tag="Buff.Effect.Damage",                 DevComment="造成伤害的 GE 共同父节点")
GameplayTagList=(Tag="Buff.Effect.Damage.Physical",        DevComment="【Effect-Damage】物理伤害，受防御减免")
GameplayTagList=(Tag="Buff.Effect.Damage.Elemental",       DevComment="【Effect-Damage】元素伤害（预留元素克制接口）")
GameplayTagList=(Tag="Buff.Effect.Damage.True",            DevComment="【Effect-Damage】真实伤害，无视防御和抗性")
GameplayTagList=(Tag="Buff.Effect.Damage.Reflected",       DevComment="【Effect-Damage】反射伤害，来源为受到伤害的百分比")
```

#### 3.2.2 治疗类 `Buff.Effect.Heal.*`

```ini
GameplayTagList=(Tag="Buff.Effect.Heal",                   DevComment="恢复生命值的 GE 父节点")
GameplayTagList=(Tag="Buff.Effect.Heal.Instant",           DevComment="【Effect-Heal】即时回血")
GameplayTagList=(Tag="Buff.Effect.Heal.Periodic",          DevComment="【Effect-Heal】持续回血（Tick）")
```

#### 3.2.3 护盾类 `Buff.Effect.Shield`

```ini
GameplayTagList=(Tag="Buff.Effect.Shield",                 DevComment="【Effect-Shield】添加护盾值（独立血条或吸收层）")
```

#### 3.2.4 属性修改类 `Buff.Effect.Attribute.*`

对应 `ApplyAttributeModifier` 节点操作的属性类型，方便 Flow Graph 精确查询。

```ini
; 填写规范：Buff.Effect.Attribute.<属性名>
; DevComment 格式：【Effect-Attr】对应 GAS 属性名

GameplayTagList=(Tag="Buff.Effect.Attribute",              DevComment="修改角色属性的 GE 父节点")
GameplayTagList=(Tag="Buff.Effect.Attribute.AttackSpeed",  DevComment="【Effect-Attr】攻击速度倍率")
GameplayTagList=(Tag="Buff.Effect.Attribute.MoveSpeed",    DevComment="【Effect-Attr】移动速度")
GameplayTagList=(Tag="Buff.Effect.Attribute.MaxHP",        DevComment="【Effect-Attr】最大生命值上限")
GameplayTagList=(Tag="Buff.Effect.Attribute.CritRate",     DevComment="【Effect-Attr】暴击概率")
GameplayTagList=(Tag="Buff.Effect.Attribute.CritDamage",   DevComment="【Effect-Attr】暴击伤害倍率")
```

#### 3.2.5 召唤类 `Buff.Effect.Summon`

```ini
GameplayTagList=(Tag="Buff.Effect.Summon",                 DevComment="【Effect-Summon】在场景中生成持续对象（实体/投射物/区域）")
```

---

### 3.3 触发层 `Buff.Trigger.*`

**职责**：描述"这个符文效果**在什么时机激活**"，贴在 GA 的 `AssetTags` 上，或作为 Flow Graph 事件节点的过滤条件。  
**用于**：BFNode 检查 `Actor.HasTag(Buff.Trigger.OnKill)` 来决定是否执行对应逻辑。  
**不用于**：分类符文本身、描述效果行为。

> 规则：一个符文 GA 通常**只有一个** Trigger 标签（被动例外，用 Passive）。

```ini
; 填写规范：Buff.Trigger.<时机名>
; DevComment 格式：【Trigger】触发事件来源及参数说明

GameplayTagList=(Tag="Buff.Trigger",                       DevComment="触发时机标签父节点，贴在 GA AssetTags 上")
GameplayTagList=(Tag="Buff.Trigger.Passive",               DevComment="【Trigger】被动常驻，装备即生效，无需事件")
GameplayTagList=(Tag="Buff.Trigger.OnHit",                 DevComment="【Trigger】成功命中敌人时触发（需伤害事件确认命中）")
GameplayTagList=(Tag="Buff.Trigger.OnKill",                DevComment="【Trigger】击杀敌人时触发")
GameplayTagList=(Tag="Buff.Trigger.OnTakeDamage",          DevComment="【Trigger】自身受到伤害时触发")
GameplayTagList=(Tag="Buff.Trigger.OnSkillCast",           DevComment="【Trigger】主动施放技能时触发（不含普攻）")
GameplayTagList=(Tag="Buff.Trigger.OnDash",                DevComment="【Trigger】闪避/冲刺时触发")
GameplayTagList=(Tag="Buff.Trigger.OnComboEnd",            DevComment="【Trigger】连招结束（最后一击判定）时触发")
```

> **新增规则**：新增 Trigger 前确认 Flow Graph 能否接收该事件。每个 Trigger 对应一个 BFNode 事件入口。

---

### 3.4 状态层 `Buff.Status.*`

**职责**：描述"角色**当前处于什么状态**"，运行时由 GE 通过 `GrantedTags` 授予角色 ASC，GE 失效时自动移除。  
**用于**：GAS 的 `RequireTags` / `BlockedTags`、Flow Graph 条件查询 `Actor.HasTag`。  
**不用于**：描述符文类型或效果行为。

> 这一层的 Tag 是**动态的**——角色身上有 / 没有取决于运行时 GE 是否激活。

#### 3.4.1 热度状态树 `Buff.Status.Heat.*`

热度相关状态统一挂在 `Heat` 子树下，分为**守卫**和**阶段**两类。

```ini
; 填写规范：Buff.Status.Heat.<子类型>
; DevComment 格式：【Status-Heat】作用描述 | 授予/移除方式

; -- 战斗保热守卫 --
; 授予：进入战斗时（由战斗状态 GE 的 GrantedTags 或 FA 的 AddTag 节点写入）
; 移除：离开战斗时自动撤销
; 作用：C++ PreGameplayEffectExecute 检测到此 Tag 时阻断 GE_HeatDecay 执行
GameplayTagList=(Tag="Buff.Status.Heat",                   DevComment="热度相关状态总父节点")
GameplayTagList=(Tag="Buff.Status.Heat.Active",            DevComment="【Status-Heat】战斗保热守卫，持有时阻断 GE_HeatDecay | 来源：战斗状态 GE GrantedTags")

; -- 热度阶段（同一时刻只能有一个）--
; 授予：BFNode_IncrementPhase 调用后，对应阶段 GE 通过 GrantedTags 授予
; 移除：BFNode_DecrementPhase 移除上一阶段 GE 时自动撤销
GameplayTagList=(Tag="Buff.Status.Heat.Phase",             DevComment="热度阶段父节点，同一时刻只有一个子节点存在于 ASC 上")
GameplayTagList=(Tag="Buff.Status.Heat.Phase.1",           DevComment="【Status-Phase】第一阶段：基础状态")
GameplayTagList=(Tag="Buff.Status.Heat.Phase.2",           DevComment="【Status-Phase】第二阶段：效果增强，部分符文解锁附加行为")
GameplayTagList=(Tag="Buff.Status.Heat.Phase.3",           DevComment="【Status-Phase】第三阶段：满阶，符文效果最大化")
```

#### 3.4.2 护盾状态 `Buff.Status.Shielded`

```ini
GameplayTagList=(Tag="Buff.Status.Shielded",               DevComment="【Status】当前有护盾层，BlockedTags 可用于阻止某些伤害 GA")
```

#### 3.4.3 控制效果（预留）`Buff.Status.CC.*`

```ini
GameplayTagList=(Tag="Buff.Status.CC",                     DevComment="控制类状态父节点（预留，当前无控制机制）")
GameplayTagList=(Tag="Buff.Status.CC.Stun",                DevComment="【Status-CC-Reserved】眩晕，无法行动")
GameplayTagList=(Tag="Buff.Status.CC.Frozen",              DevComment="【Status-CC-Reserved】冰冻，受额外元素伤害")
```

---

### 3.5 参数层 `Buff.Data.*`

**职责**：作为 `SetByCaller` 的键名，在 GA 应用 GE 时传递运行时浮点值。  
**用于**：GE Modifier 的 `SetByCaller` Magnitude 字段 + GA 调用 `SetSetByCallerMagnitude`。  
**不用于**：任何语义描述，不挂 ASC，不做条件判断。

> 每个 Tag 代表一种**值的类型**，不绑定具体某个 GE。同类型的多个 GE 可以共用同一个 Data Tag。

```ini
; 填写规范：Buff.Data.<值的类型>
; DevComment 格式：【Data】值的含义 | 单位说明

GameplayTagList=(Tag="Buff.Data",              DevComment="SetByCaller 值通道键父节点，不挂 ASC，不做条件判断")
GameplayTagList=(Tag="Buff.Data.Damage",       DevComment="【Data】伤害量 | 单位：点数，传给伤害类 GE Modifier")
GameplayTagList=(Tag="Buff.Data.Heal",         DevComment="【Data】治疗量 | 单位：点数，传给治疗类 GE Modifier")
GameplayTagList=(Tag="Buff.Data.ShieldAmount", DevComment="【Data】护盾量 | 单位：点数，传给护盾类 GE Modifier")
GameplayTagList=(Tag="Buff.Data.Duration",     DevComment="【Data】效果持续时间 | 单位：秒")
GameplayTagList=(Tag="Buff.Data.AttributeMod", DevComment="【Data】属性修改量 | 配合 ApplyAttributeModifier 节点使用")
```

---

## 五、完整使用范例：击杀护盾符文

> 场景：一个**稀有**等级的**防御型**符文，击杀敌人时为自己**附加护盾**，处于**热度第二阶段**时护盾值翻倍。

### Step 1 — RuneDataAsset 填写身份层 Tag

```
IdentityTags:
  Buff.Rune.Type.Defense      ← 功能大类：防御
  Buff.Rune.Rarity.Rare       ← 稀有度：稀有
```

### Step 2 — GE（护盾效果）填写行为层 Tag

```
GE_RuneShield_OnKill:
  AssetTags:
    Buff.Effect.Shield         ← 这个 GE 的行为是：给护盾
  GrantedTags:
    Buff.Status.Shielded       ← GE 激活期间授予角色此状态
```

### Step 3 — GA（能力入口）填写触发层 Tag

```
GA_RuneShield_OnKill:
  AssetTags:
    Buff.Trigger.OnKill        ← 这个 GA 的触发时机是：击杀时
```

### Step 4 — Flow Graph 逻辑节点配置

```
[Event: OnKill]
  └─ BFNode: CheckActorTag → Buff.Trigger.OnKill    ← 过滤：只响应击杀触发的符文
      └─ BFNode: ApplyGE → GE_RuneShield_OnKill

[Event: OnKill] （热度第二阶段加成分支）
  └─ BFNode: CheckActorTag → Buff.Status.Phase.2    ← 条件：角色当前在第二阶段
      └─ BFNode: MultiplyShieldValue × 2             ← 护盾翻倍
```

### Step 5 — 互斥控制（可选）

若需要限制同时只能有一个防御类符文主动效果：

```
GA_RuneShield_OnKill:
  BlockAbilitiesWithTag:
    Buff.Rune.Type.Defense     ← 触发时阻止同类型其他符文 GA 激活
```

---

## 六、Tag 使用对照表

| 场景 | 使用的 Tag 层 | 填写位置 |
|------|--------------|----------|
| 背包显示符文分类图标 | `Buff.Rune.Type.*` | RuneDataAsset → IdentityTags |
| 限制同类符文只能装一个 | `Buff.Rune.Type.*` | GA → BlockAbilitiesWithTag |
| Flow Graph 检测"击杀触发" | `Buff.Trigger.OnKill` | GA → AssetTags |
| Flow Graph 检测"效果是伤害类" | `Buff.Effect.Damage.*` | GE → AssetTags |
| GAS 阻止眩晕状态下触发符文 | `Buff.Status.CC.Stun` | GA → BlockedTags |
| 热度第三阶段符文加成 | `Buff.Status.Phase.3` | GA → RequireTags |
| GE 激活期间标记角色有护盾 | `Buff.Status.Shielded` | GE → GrantedTags |

---

## 七、DevComment 填写规范

DevComment 是 Tag 的内联文档，**每个叶子节点必须填写**。

```
格式：【层级标识】主要说明 | 补充说明（可选）
```

| 层级 | 标识前缀 | 示例 |
|------|----------|------|
| 身份-类型 | `【Type】` | `【Type】增强攻击/伤害输出的符文` |
| 身份-稀有度 | `【Rarity】` | `【Rarity】史诗，高强度或独特机制` |
| 身份-元素（预留） | `【Element-Reserved】` | `【Element-Reserved】火元素` |
| 行为-伤害 | `【Effect-Damage】` | `【Effect-Damage】真实伤害，无视防御` |
| 行为-属性 | `【Effect-Attr】` | `【Effect-Attr】对应 GAS 属性名` |
| 触发 | `【Trigger】` | `【Trigger】成功命中敌人时触发` |
| 状态 | `【Status】` | `【Status】当前有护盾层` |
| 状态（预留） | `【Status-CC-Reserved】` | `【Status-CC-Reserved】眩晕，无法行动` |

**父节点**（非叶子）的 DevComment 说明**该节点管辖的范围**，不需要标识前缀：

```ini
GameplayTagList=(Tag="Buff.Effect",    DevComment="GE/GA 行为描述标签，贴在 AssetTags 上，不贴角色 ASC")
GameplayTagList=(Tag="Buff.Trigger",   DevComment="触发时机标签，贴在 GA AssetTags 上，对应 BFNode 事件入口")
```

---

## 八、Q&A

**Q：一个符文同时有"击杀触发"和"受伤触发"两种效果，怎么写？**  
A：拆成两个 GA，每个 GA 各自有一个 Trigger Tag。共享同一个 RuneDataAsset，身份 Tag 只写一份。

---

**Q：Buff.Effect 和 Buff.Trigger 都贴在 GA/GE 上，会不会混淆？**  
A：不会。Effect 贴在 **GE** 的 AssetTags（描述效果），Trigger 贴在 **GA** 的 AssetTags（描述时机）。GE 不应该有 Trigger 标签，GA 不应该有 Effect 标签。

---

**Q：Flow Graph 里要判断"角色是否装备了攻击类符文"，用哪个 Tag？**  
A：用 `Buff.Rune.Type.Attack`，在 Flow Graph 中查询 ASC 上是否有这个 Tag。  
但注意：这个 Tag 要由符文装备时的 GE 通过 `GrantedTags` 授予，符文卸下时移除。身份 Tag 本身只挂在 DA 上，不会自动同步到 ASC。

---

**Q：热度阶段 Tag `Buff.Status.Phase.*` 同一时刻可以同时存在多个吗？**  
A：不应该。热度 GE 授予 Phase Tag 前，应先移除上一阶段的 Tag（通过 RemoveGameplayEffectWithTag 或同一 GE 覆盖）。Flow Graph 的热度升阶节点负责保证互斥。

---

**Q：什么时候需要新建 Tag，什么时候复用现有 Tag？**  
A：如果新符文的行为、触发时机、状态语义与现有 Tag 完全一致，直接复用。只有当现有 Tag 的 DevComment 描述与新需求有语义差异时，才新建叶子 Tag。

---

**Q：`Buff.Rune.Element.*` 是预留的，现在能填吗？**  
A：可以填在 RuneDataAsset 的 IdentityTags 上，填了不影响任何逻辑。等元素系统上线时只需接入 GE/GA 逻辑，已有的 Tag 数据不需要修改。

---

## 九、新增 Tag 决策流程

```
需要一个新 Tag？
│
├─ 描述"这个符文是什么类型/稀有度"？
│   └─ → 加到 Buff.Rune.*
│
├─ 描述"这个 GE 做了什么"？
│   └─ → 加到 Buff.Effect.*
│
├─ 描述"在什么时机触发"？
│   └─ → 加到 Buff.Trigger.*
│       └─ 确认 Flow Graph 有对应事件节点再添加
│
└─ 描述"角色现在的运行时状态"？
    └─ → 加到 Buff.Status.*
        └─ 确认 GE 的 GrantedTags 会授予、GE 失效自动移除
```

---

## 十、配套 .ini 文件

完整 Tag 定义在 [`Config/Tags/BuffTag.ini`](../../Config/Tags/BuffTag.ini) 中维护。  
当前项目的其他系统 Tag 文件参考：

| 文件 | 管理范围 |
|------|----------|
| `Config/Tags/BuffTag.ini` | 符文系统（本文档） |
| `Config/Tags/PlayerGameplayTag.ini` | 玩家状态机、连招 |
| `Config/Tags/EnemyTag.ini` | 敌人行为、攻击 |
| `Config/DefaultGameplayTags.ini` | GameplayCue、跨系统通用 GE 标签 |
