# 攻击伤害系统 — 技术文档

> 适用范围：攻击判定系统的架构、接口、关键文件  
> 适用人群：程序  
> 配套文档：[设计说明](../../../00_入口与规范/缺失引用记录.md)、[配置指南](AttackDamage_ConfigGuide.md)  
> 最后更新：2026-04-10（C++ 完全迁移）

---

## 架构总览（当前版本）

```
蒙太奇播放（GA_MeleeAttack::ActivateAbility）
  └─ UYogAbilityTask_PlayMontageAndWaitForEvent
       └─ AN_MeleeDamage::Notify()                   ← AnimNotify（C++）
            └─ ASC::HandleGameplayEvent("GameplayEffect.DamageType.GeneralAttack")
                 └─ GA_MeleeAttack::OnEventReceived(EventTag, EventData)
                      │  EventData.OptionalObject = this（GA 实例）
                      └─ ApplyEffectContainer(EventTag, EventData)
                           └─ MakeEffectContainerSpec()
                                └─ OwningCharacter->DefaultMeleeDamageEffect / DefaultMeleeTargetType
                                     └─ UYogTargetType_Enemy / UYogTargetType_Player（C++ CDO）
                                          ├─ EventData.OptionalObject → UGA_MeleeAttack
                                          │    └─ GetAbilityActionData() → FActionData（hitboxTypes / ActRange）
                                          ├─ SphereOverlapActors（SearchRadius = ActRange）
                                          ├─ IsTargetHit()（Annulus / Triangle / 球形兜底）
                                          └─ OutActors → ApplyEffectContainerSpec → 施加 DefaultMeleeDamageEffect
```

---

## 与旧版本的对比

| 项目 | 旧版本 | 当前版本（2026-04-10） |
|---|---|---|
| 攻击 GA | Blueprint 逻辑 | C++（`GA_MeleeAttack` 基类） |
| 目标检测 | `B_TT_Enemy` / `B_TT_Player`（Blueprint） | `UYogTargetType_Enemy/Player`（C++） |
| 伤害触发方式 | 蒙太奇帧 AnimNotify | 同左，但 EventTag 固定 = `GameplayEffect.DamageType.GeneralAttack` |
| 命中框配置 | Blueprint B_TT 内部硬编码 | `DA_AbilityData → AbilityMap → hitboxTypes`（编辑器可配置） |
| `EffectContainerMap` 位置 | ASC（每个角色独立配置） | 不再使用；改为角色基类上的 `DefaultMeleeDamageEffect` |
| ActionData 来源 | B_TT 通过 `GetCurrentAbilityInstance()`（废弃 API）获取 | 通过 `EventData.OptionalObject`（精确 GA 引用） |
| 攻击前后摇 GE | 手动填写 GA Blueprint | C++ 构造函数自动绑定（玩家 GA 专用） |

---

## 核心数据流

### 1. ActionData 路由

`GA_MeleeAttack::OnEventReceived` 在调用 `ApplyEffectContainer` 前，将自身（`this`）写入 `EventData.OptionalObject`：

```cpp
EventData.OptionalObject = this;
ApplyEffectContainer(EventTag, EventData);
```

`UYogTargetType_MeleeBase::GetActionData` 优先从 `OptionalObject` 读取：

```cpp
if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(EventData.OptionalObject))
    return MeleeGA->GetAbilityActionData();  // 精确拿到当前攻击段的 FActionData
```

这消除了旧版 `GetCurrentAbilityInstance()` 的多 GA 并发歧义问题。

### 2. ActionData 查找路径

```
UGA_MeleeAttack::GetAbilityActionData_Implementation()
  └─ Owner (AYogCharacterBase)
       └─ CharacterDataComponent->GetCharacterData()
            └─ CharacterData->AbilityData（UAbilityData 资产）
                 └─ AbilityMap.Find(AbilityTags 第一个 Tag)
                      └─ FActionData { hitboxTypes, ActRange, ActDamage, Montage, … }
```

### 3. 伤害容器构建路径

```
MakeEffectContainerSpec(EventTag, EventData)
  └─ OwningCharacter->DefaultMeleeDamageEffect != nullptr？
       ├─ YES → 构建 FYogGameplayEffectContainer：
       │         TargetType = OwningCharacter->DefaultMeleeTargetType
       │         Effects    = [DefaultMeleeDamageEffect]
       │         → MakeEffectContainerSpecFromContainer()
       └─ NO  → 空 Spec，流程静默终止（需检查角色 BP 配置）
```

---

## 关键类说明

### `UGA_MeleeAttack`

文件：`Source/DevKit/Public/AbilitySystem/Abilities/GA_MeleeAttack.h`

所有近战攻击 GA 的 C++ 基类。完整封装了：
- `ActivateAbility`：CommitAbility → StatBeforeATK GE → Montage + EventListener
- `EndAbility`：移除 StatBeforeATK GE，若正常结束则施加 StatAfterATK GE
- `OnEventReceived`：写入 OptionalObject → `ApplyEffectContainer`
- `GetAbilityActionData_Implementation`：从角色 CharacterData 的 AbilityData 查表

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
TSubclassOf<UGameplayEffect> StatBeforeATKEffect;   // 攻击前摇 GE（动作期间临时属性）

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
TSubclassOf<UGameplayEffect> StatAfterATKEffect;    // 攻击后摇 GE（正常结束后施加）
```

### `UGA_PlayerMeleeAttack`

文件：`Source/DevKit/Public/AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h`

玩家近战 GA 中间基类，构造函数自动绑定：
- `StatBeforeATKEffect` = `/Game/Code/GAS/Abilities/Shared/GE_StatBeforeATK`
- `StatAfterATKEffect`  = `/Game/Code/GAS/Abilities/Shared/GE_StatAfterATK`

子类只设置 AbilityTags / ActivationOwnedTags / ActivationRequiredTags / ActivationBlockedTags，无需填写 GE。

### 玩家连击链

```
LightAtk1：AbilityTags={LC1}  OwnedTags={LC1}            Required={}    Blocked={LC1}
LightAtk2：AbilityTags={LC2}  OwnedTags={LC1,LC2}        Required={CanCombo,LC1}  Blocked={LC2}
LightAtk3：AbilityTags={LC3}  OwnedTags={LC1,LC2,LC3}    Required={CanCombo,LC1,LC2}  Blocked={LC3}
LightAtk4：AbilityTags={LC4}  OwnedTags={LC1,LC2,LC3,LC4} Required={CanCombo,LC1,LC2,LC3}  Blocked={LC4}
```

`CanCombo` Tag 由蒙太奇 AnimNotify 在连招窗口期添加/移除，控制下一段是否可触发。

### `UYogTargetType_MeleeBase` / Enemy / Player

文件：`Source/DevKit/Public/AbilitySystem/Abilities/YogTargetType_Melee.h`

| 类 | 搜索目标类型 | Debug 颜色 |
|---|---|---|
| `UYogTargetType_Enemy` | `APlayerCharacterBase`（敌人攻玩家） | 橙色 |
| `UYogTargetType_Player` | `AEnemyCharacterBase`（玩家攻敌人） | 黄色 |

支持命中框类型：
- **Annulus**（环形扇区）：inner_radius / degree / offset_degree
- **Triangle**（三角扇面）：N 个角度点，每相邻两点构成一个扇形片
- **兜底**（空 hitboxTypes）：全向球形，半径 = ActRange

### `YogCharacterBase` 中的配置属性

```cpp
// 角色父类 BP 上配置一次，所有子类继承
UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
TSubclassOf<UYogTargetType>      DefaultMeleeTargetType;    // 设为 UYogTargetType_Enemy/Player

UPROPERTY(EditDefaultsOnly, Category = "Combat|Melee")
TSubclassOf<UYogGameplayEffect>  DefaultMeleeDamageEffect;  // 设为伤害 GE
```

---

## 编辑器配置清单（接入新近战角色）

| 步骤 | 操作位置 | 操作内容 |
|---|---|---|
| 1 | 角色父类 BP | `DefaultMeleeTargetType` = `UYogTargetType_Enemy`（敌人）或 `UYogTargetType_Player`（玩家） |
| 2 | 角色父类 BP | `DefaultMeleeDamageEffect` = 对应伤害 GE |
| 3 | `CharacterData` 资产 → `AbilityData` | 每个攻击行填写 Montage、hitboxTypes、ActRange 等 |
| 4 | `CharacterData` 资产 → `GASTemplate` | 添加对应的 `GA_Enemy_LAtk1` 等 C++ 类 |
| 5 | 蒙太奇 | 攻击帧放 `AN_MeleeDamage` 通知（C++，无需设置 EventTag） |

---

## 关键文件索引

| 文件 | 作用 |
|---|---|
| `Public/AbilitySystem/Abilities/GA_MeleeAttack.h/.cpp` | 近战 GA C++ 基类，封装完整攻击流程 |
| `Public/AbilitySystem/Abilities/GA_PlayerMeleeAttacks.h/.cpp` | 玩家连击 GA（LightAtk1-4, HeavyAtk1-4, DashAtk） |
| `Public/AbilitySystem/Abilities/GA_EnemyMeleeAttacks.h/.cpp` | 敌人攻击 GA（LAtk1-4, HAtk1-4） |
| `Public/AbilitySystem/Abilities/YogTargetType_Melee.h/.cpp` | C++ 命中框检测（替代 B_TT Blueprint） |
| `Public/Animation/AN_MeleeDamage.h/.cpp` | 近战伤害 AnimNotify（C++） |
| `Public/Character/YogCharacterBase.h` | `DefaultMeleeTargetType` / `DefaultMeleeDamageEffect` 定义 |
| `Public/Data/AbilityData.h` | `FActionData`（hitboxTypes / ActRange / Montage 等） |

---

## 已解决的历史问题

| 旧问题 | 解决方式 |
|---|---|
| `EffectContainerMap` 在 ASC 上，漏配静默失败 | 改为角色基类 `DefaultMeleeDamageEffect`，父类配置一次 |
| `GetCurrentAbilityInstance()` 废弃 API，多 GA 并发时返回错误实例 | 改为 `EventData.OptionalObject`（GA 自己塞入精确引用） |
| B_TT Blueprint 里 Cast 失败分支未连接 | 整个 B_TT 替换为 C++ |
| Triangle 碰撞检测循环 index 0 被跳过 | C++ 重写，从 index 1 开始正确迭代相邻点对 |
| 连击首段会同时激活所有段 | 正确实现 ActivationOwnedTags 累积链 + ActivationRequiredTags 顺序门 |
| AnimNotify EventTag 为空，OnEventReceived 不触发 | 明确传入 `GameplayEffect.DamageType.GeneralAttack` Tag |
| GE_StatBeforeATK SetByCaller 缺 Magnitude | 在 ActivateAbility 中读取 ActionData 后设置 4 个 Magnitude |

---

## ⚠️ Claude 编写注意事项

- **伤害必须走 EffectContainerMap**：不允许在 GA C++ 里直接 `ApplyGameplayEffectToTarget`，所有伤害 GE 配置在 Blueprint EffectContainerMap 的 `Event.Attack.Hit` Key 下
- **TargetType 碰撞通道**：近战判定用 `ECC_WorldDynamic + ECC_Pawn`，不要用 `ECC_Visibility`（会打到 Trigger Volume）
- **CDO 问题**：不要在 `UGameplayAbility` 构造函数里 Spawn Actor 或访问 World，只在 `ActivateAbility` 里做
- **连击 Tag 累积链**：每段连击靠 `ActivationOwnedTags` 累积 + `ActivationRequiredTags` 顺序门控制，改连击段数时两个 Tag 容器必须同步
- **AnimNotify EventTag 不能为空**：`WaitGameplayEvent` 的 Tag 必须显式填入，如 `GameplayEffect.DamageType.GeneralAttack`，空 Tag 不触发回调
- **SetByCaller Magnitude**：GE 用 SetByCaller 传伤害时，`ActivateAbility` 里必须先 `SetByCallerTagMagnitude`，否则 GE Apply 后伤害为 0
