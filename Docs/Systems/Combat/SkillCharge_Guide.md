# 技能充能/CD 系统使用指南

> 项目：星骸降临
> 关联文档：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · [BuffFlow_NodeReference.md](BuffFlow_NodeReference.md) · [GameplayTag_MasterGuide.md](GameplayTag_MasterGuide.md)
> 更新日期：2026-04-09

---

## 一、系统概述

**USkillChargeComponent** 统一管理所有技能的充能/CD，与 BuffFlow 符文系统协同工作：

| 职责 | 由谁负责 |
|------|---------|
| 充能当前格数的持有与计时 | `USkillChargeComponent`（C++） |
| MaxCharge / CDDuration 的数值配置 | `PlayerAttributeSet`（GAS 属性） |
| 符文修改 MaxCharge / CDDuration | BuffFlow FA 节点（`Apply Attribute Modifier`） |
| 技能能否激活的判断 | Blueprint GA → `HasCharge()` |
| 消耗充能并启动回复计时 | Blueprint GA → `ConsumeCharge()` |

**"用后触发"队列模式**：用掉 1 格时启动 CD 倒计时，CD 结束回复 1 格；若此时还有其他格在等待，继续计时。充能格满时 Timer 自动停止。

```
MaxCharge=2, CD=3s

t=1s  用1格 → [1/2], 启动 CD(3s)
t=2s  用1格 → [0/2], 已有 Timer，不重复启动
t=4s  CD 结束 → [1/2], 还有1格在等，继续 CD(3s)
t=7s  CD 结束 → [2/2], 满格，Timer 停止
```

---

## 二、"永久底座"设计

**玩家天然具有基础冲刺能力，无需任何符文 FA/DA。**

`PlayerAttributeSet` 的默认值即为"永久冲刺底座"：

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `MaxDashCharge` | `1` | 默认 1 格充能 |
| `DashCooldownDuration` | `3.0s` | 默认 3 秒 CD |

`SkillChargeComponent` 在 `RegisterSkill` 时读取这两个属性初始化，`CurrentCharge` 从满格开始。只要将冲刺 GA 授予玩家，冲刺就能正常工作。

**强化符文（可选）**：装备后通过 `Apply Attribute Modifier` 叠加到这两个属性上——充能格+1、CD缩短——符文移出激活区后自动恢复默认。

---

## 三、Tag 命名规范

### 3.1 技能标识层 = `PlayerState.AbilityCast.*`

**SkillChargeComponent 的注册键与 GA 的 `AbilityTags` 使用同一个 Tag**，不需要额外命名空间。

| Tag | 说明 |
|-----|------|
| `PlayerState.AbilityCast.Dash` | 冲刺技能（注册键 + GA AbilityTag） |
| `PlayerState.AbilityCast.Skill1` | 主动技能槽1（按实际功能命名） |
| `PlayerState.AbilityCast.Skill2` | 主动技能槽2 |

> ⚠️ 不要用 `Buff.*` 或 `Ability.*` 命名空间下的 Tag 作为注册键。  
> `Ability.*` 在本项目中不作为独立根命名空间使用（见 [GameplayTag_MasterGuide.md](GameplayTag_MasterGuide.md)）。

---

### 3.2 行为层扩展 `Buff.Effect.Attribute.*`（符文效果描述）

当一个符文的效果是"修改技能的 MaxCharge 或 CDDuration"时，在 RuneDataAsset 的 `RuneEffectTag` 字段上标注以下 Tag（供背包 UI 筛选）：

| Tag | 说明 | 对应 GAS 属性 |
|-----|------|--------------|
| `Buff.Effect.Attribute.MaxCharge` | 增加技能最大充能格数 | `Max{SkillName}Charge` |
| `Buff.Effect.Attribute.CooldownDuration` | 缩短技能 CD 间隔（Multiplicative < 1） | `{SkillName}CooldownDuration` |

> 这两个 Tag 贴在 RuneDataAsset 上用于背包 UI 分类/筛选，不挂角色 ASC，不参与运行时逻辑。

---

### 3.3 现有触发层（无需新增）

| Tag | 说明 |
|-----|------|
| `Buff.Trigger.OnDash` | 冲刺/闪避时触发符文 |
| `Buff.Trigger.OnSkillCast` | 主动施放技能时触发符文（不含普攻） |

> 这两个 Tag 贴在 GA 的 `AssetTags` 上，与 AbilityTags 不冲突，两者并存于同一个 GA。

---

### 3.4 Tag 对照总表

| Tag | 命名空间 | 贴在哪里 | 运行时挂 ASC？ |
|-----|---------|---------|--------------|
| `PlayerState.AbilityCast.Dash` | 玩家状态层 | GA.AbilityTags + RegisterSkill 参数 | 激活期间挂（ActivationOwnedTags）|
| `Buff.Trigger.OnDash` | 触发层 | GA.AssetTags | ❌ |
| `Buff.Effect.Attribute.MaxCharge` | 行为层 | DA.RuneEffectTag | ❌ |
| `Buff.Effect.Attribute.CooldownDuration` | 行为层 | DA.RuneEffectTag | ❌ |

---

## 四、GAS 属性命名规范（PlayerAttributeSet）

每个充能型技能对应两个 GAS 属性：

```
Max{SkillName}Charge          → 最大充能格数（符文可 Additive +1）
{SkillName}CooldownDuration   → 每格回复间隔（符文可 Multiplicative × 加速）
```

| 技能 | MaxCharge 属性 | CDDuration 属性 | 默认值 |
|------|---------------|----------------|--------|
| 冲刺 | `MaxDashCharge` | `DashCooldownDuration` | 1格 / 3s |
| 技能1 | `MaxSkill1Charge` | `Skill1CooldownDuration` | 按设计填写 |

> **单次 CD 技能（不需要充能）**：MaxCharge 默认值 = 1，表现和"充能上限为1"完全等价，无需特殊处理。

---

## 五、冲刺 GA Blueprint 配置

冲刺 GA 已直接授予玩家，只需添加充能系统的两处节点。

### 5.1 CanActivateAbility（自定义检查）

```
GetOwningActorFromActorInfo
    → Cast to APlayerCharacterBase
        → GetSkillChargeComponent
            → HasCharge("PlayerState.AbilityCast.Dash")
                → True  → 允许激活
                → False → 拒绝激活（可播放 CD 音效/UI 反馈）
```

### 5.2 ActivateAbility 开头（第一个节点）

```
GetOwningActorFromActorInfo
    → Cast to APlayerCharacterBase
        → GetSkillChargeComponent
            → ConsumeCharge("PlayerState.AbilityCast.Dash")
→ 继续冲刺逻辑...
```

> ⚠️ **ConsumeCharge 必须在 ActivateAbility 最开始调用**，不要等技能逻辑执行完再调用，否则玩家在技能执行期间可能重复激活。

---

## 六、新技能接入流程

### Step 1：PlayerAttributeSet 新增两个属性

```cpp
// PlayerAttributeSet.h
/** 技能1 最大充能格数（符文可 Additive +1） */
UPROPERTY(BlueprintReadWrite, Category = "Attributes|Skill1")
FGameplayAttributeData MaxSkill1Charge;
ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxSkill1Charge);

/** 技能1 每格回复间隔（符文可 Multiplicative 缩短） */
UPROPERTY(BlueprintReadWrite, Category = "Attributes|Skill1")
FGameplayAttributeData Skill1CooldownDuration;
ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Skill1CooldownDuration);
```

```cpp
// PlayerAttributeSet.cpp 构造函数
MaxSkill1Charge        = 1.0f;   // 默认1格（单次CD）
Skill1CooldownDuration = 8.0f;   // 默认8秒CD
```

### Step 2：PlayerCharacterBase.cpp BeginPlay 注册

```cpp
// 在 RegisterSkill 区块追加一行即可，无需其他 C++ 改动
SkillChargeComponent->RegisterSkill(
    FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Skill1")),
    UPlayerAttributeSet::GetMaxSkill1ChargeAttribute(),
    UPlayerAttributeSet::GetSkill1CooldownDurationAttribute()
);
```

### Step 3：Blueprint GA 接入（同冲刺 GA 配置方式，换 Tag）

```
HasCharge("PlayerState.AbilityCast.Skill1")
ConsumeCharge("PlayerState.AbilityCast.Skill1")
```

---

## 七、符文修改充能/CD（BuffFlow FA 写法）

### 模式 A：增加最大充能格数（冲刺格 +1）

```
[Start] → [Apply Attribute Modifier]
               Attribute    = PlayerAttributeSet.MaxDashCharge
               Mod Op       = Additive
               Value        = 1.0
               Duration Type= Infinite
               Target       = Buff拥有者
```

效果：冲刺从 1 格变 2 格。符文移出激活区时 GE 移除，MaxDashCharge 恢复 1，SkillChargeComponent 自动钳制 CurrentCharge ≤ 1。

---

### 模式 B：缩短 CD 间隔（-25%）

```
[Start] → [Apply Attribute Modifier]
               Attribute    = PlayerAttributeSet.DashCooldownDuration
               Mod Op       = Multiplicative
               Value        = -0.25       ← -25% 即缩短到 75%（3s → 2.25s）
               Duration Type= Infinite
               Target       = Buff拥有者
```

> **注意：** Multiplicative 的 Value 填 `-0.25` 表示 ×(1 - 0.25) = ×0.75，即缩短 25%。

---

### 模式 C：条件触发 CD 加速（击杀后 10 秒内 CD 减半）

```
[Start] → [OnKill]
               → [Apply Attribute Modifier]
                      Attribute    = PlayerAttributeSet.DashCooldownDuration
                      Mod Op       = Multiplicative
                      Value        = -0.5
                      Duration Type= Has Duration
                      Duration     = 10.0
                      Target       = Buff拥有者
```

10 秒后 GE 自然过期，CDDuration 恢复原值。

---

## 八、调试速查

| 现象 | 首先检查 |
|------|---------|
| 技能按了没反应 | `HasCharge()` 返回 false → CurrentCharge 是否为 0？GAS Debugger 看 MaxCharge 属性是否正常 |
| CD 不回复 | `ConsumeCharge()` 是否调用？SkillChargeComponent 是否在 BeginPlay 里初始化（`InitWithASC` + `RegisterSkill`）？ |
| 符文增加格数无效 | GAS Debugger → Attributes 面板：MaxDashCharge 是否变化？Mod Op 是否用 Additive？ |
| 符文离区后格数没钳制 | MaxCharge 属性的 OnAttributeChange 委托是否触发（RegisterSkill 内部绑定）？检查 ASC 是否有效 |
| 充能 UI 不刷新 | 是否绑定 `SkillChargeComponent.OnChargeChanged` 委托？ |
| CDDuration 符文改了没效果 | CDDuration 改变只影响"下次 Timer 调度"，正在倒计时的那格不受影响，等当前格回复后才生效 |

---

## 九、Tag 决策流程

```
需要一个跟技能充能相关的新 Tag？
│
├─ 标识"这是哪个技能"（RegisterSkill 的键 + GA.AbilityTags）？
│   └─ → PlayerState.AbilityCast.<功能名>    （如 PlayerState.AbilityCast.Blink）
│
├─ 描述"这个符文修改了什么属性"（贴 DA.RuneEffectTag）？
│   ├─ 修改最大格数 → Buff.Effect.Attribute.MaxCharge（已有，复用）
│   └─ 修改 CD 间隔 → Buff.Effect.Attribute.CooldownDuration（已有，复用）
│
├─ 描述"何时触发"（符文触发时机，贴 GA.AssetTags）？
│   ├─ 冲刺时触发 → Buff.Trigger.OnDash（已有，复用）
│   └─ 施放技能时触发 → Buff.Trigger.OnSkillCast（已有，复用）
│
└─ 描述"当前处于 CD 状态"？
    └─ ❌ 不需要 Status Tag。CD 状态由 SkillChargeComponent 内部管理，
          不在 GAS ASC 上挂 Tag。UI 通过 OnChargeChanged 委托刷新。
```

---

## 十、与 BuffFlow 架构的关系

```
符文 FA（BuffFlow 逻辑层）
  └─ Apply Attribute Modifier: MaxDashCharge +1
       └─ GAS（效果执行层）修改属性值
            └─ SkillChargeComponent 监听 MaxCharge 变化
                 └─ 自动钳制 CurrentCharge ≤ MaxCharge

GA_PlayerDash（技能执行层）
  └─ CanActivate → SkillChargeComponent.HasCharge("PlayerState.AbilityCast.Dash")
  └─ Activate    → SkillChargeComponent.ConsumeCharge("PlayerState.AbilityCast.Dash")
                       └─ CurrentCharge-- ，启动 CDDuration Timer
                       └─ Timer 触发 → CurrentCharge++
                       └─ OnChargeChanged.Broadcast → UI 刷新
```

符文系统和充能系统**正交**：符文只改 GAS 属性（MaxCharge / CDDuration），充能系统自动读取属性运行，两边互不耦合。

---

## ⚠️ Claude 编写注意事项

- **SkillChargeComponent 替代 GAS 原生 CD**：不要用 `UGameplayAbility::CommitAbilityCooldown`，所有充能/冷却逻辑通过 `SkillChargeComponent::ConsumeCharge()` 和 `HasCharge()` 管理
- **GA 的 CanActivateAbility 必须调 HasCharge()**：在 `CanActivateAbility` 里显式调用 `SkillChargeComponent->HasCharge(AbilityTag)`，GAS 默认 CD 检查不管 SkillCharge
- **充能恢复走 TimerManager**：`CooldownDuration` 用 `GetWorld()->GetTimerManager().SetTimer` 触发单次充能恢复，不要在 Tick 里累积时间（Tick 在 TimeDilation=0 时可能不走）
- **符文修改 CD 的接口**：符文 FA 节点通过 `SkillChargeComponent::SetCooldownMultiplier(float)` 修改冷却倍率，不直接改 Timer 句柄
- **CurrentCharge 不走 GAS Attribute**：充能次数存在 `SkillChargeComponent` 成员变量里，不是 GAS Attribute，不要用 `GetAttributeValue` 读取
- **挂载位置**：`SkillChargeComponent` 挂在 `AYogCharacterBase` 或其子类上，不要挂在 GA 对象上（GA 对象没有稳定的生命周期）
