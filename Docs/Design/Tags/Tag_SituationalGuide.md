# Tag 情景使用指南

> 适用范围：策划、程序均可查阅  
> 配套文档：`GA_TagFields_Guide.md`、`GameplayTag_MasterGuide.md`  
> 原则：先找情景，再看用哪个 Tag 和放在哪里

---

## 一、触发类情景（让某件事发生）

### 情景 1：血量归零，触发死亡 GA

```
发出方：YogCharacterBase::Die()
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Action.Dead, EventData)

接收方：GA_Dead
    AbilityTriggers → GameplayEvent: Action.Dead
```

**Tag**：`Action.Dead`  
**命名空间**：`Action.*`（一次性事件信号）  
**不要**：不要把 `Action.Dead` 挂在 ASC 上当状态用

---

### 情景 2：FA 判定命中，触发受击 GA

```
发出方：FA_Attack（FlowAbility 节点）
    BFNode_SendGameplayEvent → 目标 Actor → Action.HitReact

接收方：GA_HitReaction
    AbilityTriggers → GameplayEvent: Action.HitReact.Front 或 Action.HitReact.Back
```

**Tag**：`Action.HitReact.Front` / `Action.HitReact.Back`（方向区分）  
**为什么有两个**：PassiveMap 用方向 Tag 查找不同朝向的受击动画

---

### 情景 3：FA 判定韧性破防，触发击退 GA

```
发出方：FA_Attack
    BFNode_SendGameplayEvent → 目标 Actor → Action.Knockback

接收方：GA_Knockback
    AbilityTriggers → GameplayEvent: Action.Knockback
```

**Tag**：`Action.Knockback`

---

### 情景 4：BT Task 随机激活一种近战攻击

```
BT Task：BTTask_ActivateAbilityByTag
    AbilityTags: [Enemy.Melee]   ← 填父 Tag，自动从所有 Enemy.Melee.* GA 中随机选一个

各攻击 GA 的 Ability Tags：
    Enemy.Melee.LAtk1 / LAtk2 / HAtk1 ... 等
```

**Tag**：`Enemy.Melee`（父 Tag）→ `Enemy.Melee.LAtk1`（子 Tag）  
**原理**：UE Tag 父子匹配，`TryActivateRandomAbilitiesByTag(Enemy.Melee)` 能找到所有 `Enemy.Melee.*` 子级 GA

---

### 情景 5：BT Task 指定激活某个特定攻击

```
BT Task：BTTask_ActivateAbilityByTag
    AbilityTags: [Enemy.Melee.HAtk1]   ← 精确匹配，不随机
```

---

## 二、状态类情景（广播"当前在做什么"）

### 情景 6：GA 激活期间，让外部系统知道"角色死亡了"

```
GA_Dead
    Activation Owned Tags: Buff.Status.Dead

效果：
    GA 激活 → Buff.Status.Dead 自动挂到 ASC
    GA 结束 → Buff.Status.Dead 自动移除
```

**Tag**：`Buff.Status.Dead`  
**放在**：GA 的 `Activation Owned Tags`  
**不要**：不要手动 AddLooseGameplayTag，让 GAS 自动管理

---

### 情景 7：GA 激活期间，广播"玩家正在普攻第 2 连"

```
GA_LightAtk_Combo2
    Ability Tags:          PlayerState.AbilityCast.LightAtk.Combo2
    Activation Owned Tags: PlayerState.AbilityCast.LightAtk.Combo2
```

**两个字段填同一个 Tag 的原因**：
- `Ability Tags`：外部可以通过 Tag 取消/查找这个 GA
- `Activation Owned Tags`：ASC 上有这个 Tag，StateConflict 才能读到并执行冲突规则

---

### 情景 8：GE（GameplayEffect）生效期间标记"流血状态"

```
GE_Bleeding
    Granted Tags: Buff.Status.Bleeding

效果：
    GE 应用 → Buff.Status.Bleeding 挂到目标 ASC
    GE 移除 → Buff.Status.Bleeding 自动移除
```

**Tag**：`Buff.Status.Bleeding`  
**放在**：GE 的 `Granted Tags`（不是 GA 的 ActivationOwnedTags）  
**区别**：持续 GE 产生的状态用 Granted Tags，GA 激活产生的状态用 ActivationOwnedTags

---

## 三、阻断类情景（阻止 GA 激活 / 停止系统）

### 情景 9：死亡后阻止所有攻击 GA 激活

**方式一：StateConflict（推荐，批量处理）**

```
DA_Base_StateConflict_Initial → Rules
    ActiveTag:   Buff.Status.Dead
    Block Tags:  Action, Enemy, Enemy_Behavior    ← 填 GA 的 AbilityTag 父节点
    Cancel Tags: Action, Enemy, Enemy_Behavior
```

**方式二：Activation Blocked Tags（单个 GA 的兜底）**

```
GA_Enemy_Melee_LAtk1
    Activation Blocked Tags: Buff.Status.Dead
```

**通常两者并用**：StateConflict 批量处理，Activation Blocked Tags 作为最后保险

---

### 情景 10：受击硬直期间停止 AI 行为树

```
DA_Base_StateConflict_Initial → BlockCategoryMap
    Key:   Block.AI
    Value: Buff.Status.HitReact, Buff.Status.Dead, Buff.Status.Knockback

代码自动执行（YogAbilitySystemComponent::OnTagUpdated）：
    Buff.Status.HitReact 挂上 → Brain->PauseLogic()
    Buff.Status.HitReact 移除 → Brain->ResumeLogic()（若没有其他阻断状态）
```

**Tag**：`Block.AI`（BlockCategoryMap 的 Key，不挂 ASC）+ `Buff.Status.HitReact`（触发条件）

---

### 情景 11：受击硬直期间停止角色移动

```
DA_Base_StateConflict_Initial → BlockCategoryMap
    Key:   Block.Movement
    Value: Buff.Status.HitReact, Buff.Status.Dead, Buff.Status.Knockback

代码自动执行：
    Buff.Status.HitReact 挂上 → DisableMovement() + AI StopMovement()
    Buff.Status.HitReact 移除 → EnableMovement()（若角色存活且没有其他阻断状态）
```

---

### 情景 12：玩家普攻期间无法闪避

```
DA_Base_StateConflict_Initial → Rules
    ActiveTag:  PlayerState.AbilityCast.LightAtk.Combo1
    Block Tags: PlayerState.AbilityCast.Dash
```

或在 GA_Dash 里：

```
GA_Dash
    Activation Blocked Tags: PlayerState.AbilityCast.LightAtk
    ← 用父 Tag 匹配所有连招段
```

---

## 四、数据查询类情景（Tag 当数据表的 Key）

### 情景 13：GA 激活时查找对应的攻击蒙太奇和伤害数据

```cpp
// GA_PlayMontage 内部
const FGameplayTag LookupTag = AbilityTags.GetByIndex(0);  // Enemy.Melee.LAtk1
FActionData Data = CharData->GetAbilityData()->GetAbility(LookupTag);
// Data.Montage → 攻击蒙太奇
// Data.ActDamage → 伤害值
```

**Tag**：GA 的 `Ability Tags` 同时是 AbilityData.AbilityMap 的 Key  
**对应 DA**：各角色的 `DA_Ability_XXX`

---

### 情景 14：GA_Dead 查找死亡蒙太奇和消解特效

```cpp
// GA_Dead 内部
const FGameplayTag LookupTag = FGameplayTag::RequestGameplayTag("Action.Dead");
FPassiveActionData DeadData = CharData->GetAbilityData()->GetPassiveAbility(LookupTag);
// DeadData.Montage → 死亡动画
// DeadData.DissolveGameplayCueTag → 消解粒子 GC Tag
```

**Tag**：`Action.Dead` 同时是 PassiveMap 的 Key  
**对应 DA**：各角色的 `DA_Ability_XXX` → PassiveMap

---

### 情景 15：GA_HitReaction 根据方向查找受击蒙太奇

```cpp
// 方向判断后
FGameplayTag DirectionTag = bFront
    ? FGameplayTag::RequestGameplayTag("Action.HitReact.Front")
    : FGameplayTag::RequestGameplayTag("Action.HitReact.Back");

FPassiveActionData HitData = CharData->GetAbilityData()->GetPassiveAbility(DirectionTag);
```

**Tag**：`Action.HitReact.Front` / `Action.HitReact.Back` 是 PassiveMap 的 Key

---

## 五、数值传递类情景（SetByCaller）

### 情景 16：攻击 GA 把伤害值传给 GE

```cpp
// GA_PlayMontage 内，应用伤害 GE 时
Spec->SetSetByCallerMagnitude(
    FGameplayTag::RequestGameplayTag("Attribute.ActDamage"),
    action_data->ActDamage);  // 从 AbilityData 读到的值

// GE 里
// Modifier → Magnitude Calculation Type: Set By Caller
//           → Data Tag: Attribute.ActDamage
```

**Tag**：`Attribute.ActDamage`（值通道，不是状态）

---

### 情景 17：符文 GE 传伤害量

```
FA 节点中 → BFNode_ApplyGE → SetByCallerMap:
    Buff.Data.Damage = 50.0

对应 GE → Modifier → DataTag: Buff.Data.Damage
```

**Tag**：`Buff.Data.Damage`

---

## 六、条件判断类情景（Blueprint 里查 Tag）

### 情景 18：Blueprint 判断角色是否死亡

```
ASC → Has Matching Gameplay Tag → Buff.Status.Dead
```

**不要用**：不要查 `Action.Dead`，`Action.*` 是瞬时信号不会留在 ASC 上

---

### 情景 19：Blueprint 判断角色当前处于哪个热度阶段

```
ASC → Has Matching Gameplay Tag → Buff.Status.Heat.Phase.1   ← 第一阶段
ASC → Has Matching Gameplay Tag → Buff.Status.Heat.Phase.2   ← 第二阶段
ASC → Has Matching Gameplay Tag → Buff.Status.Heat.Phase.3   ← 第三阶段
```

同一时刻只有一个阶段 Tag 存在（由热度系统保证互斥）

---

### 情景 20：Blueprint 判断当前是否有护盾

```
ASC → Has Matching Gameplay Tag → Buff.Status.Shielded
```

---

## 七、符文系统情景

### 情景 21：设计一个"命中时触发"的攻击符文

```
符文 GA（BP_Rune_OnHitAttack）
    AssetTags: Buff.Trigger.OnHit        ← 触发条件：命中时
               Buff.Rune.Type.Attack     ← 分类：攻击型
               Buff.Rune.Rarity.Rare     ← 稀有度：稀有
    AbilityTriggers: GameplayEvent → Buff.Trigger.OnHit
    （以上均不是 Ability Tags，是 AssetTags）
```

**注意**：`Buff.Trigger.OnHit` 既是 AssetTags（描述符文特征）也是触发信号（AbilityTriggers 监听），但它不是 `Ability Tags`。

---

### 情景 22：同类符文只能装备一个

```
DA_Base_StateConflict_Initial → Rules
    ActiveTag:  Buff.Rune.Type.Attack
    Block Tags: Buff.Rune.Type.Attack
    Priority:   -1

对应符文 GA
    Activation Owned Tags: Buff.Rune.Type.Attack   ← 激活时广播分类 Tag
```

当一个攻击型符文激活时，StateConflict 屏蔽其他攻击型符文 GA

---

## 八、GameplayCue 情景

### 情景 23：死亡消解粒子

```
AbilityData.PassiveMap[Action.Dead].DissolveGameplayCueTag = GameplayCue.Character.Dissolve.Humanoid

GA_Dead 激活后调用：
    ASC->ExecuteGameplayCue(GameplayCue.Character.Dissolve.Humanoid, CueParams)
```

**Tag 命名规范**：必须以 `GameplayCue.` 开头，否则 UE 不识别为 GC Tag  
**GC Blueprint**：命名对应（如 `GC_Character_Dissolve_Humanoid`），Details 里 GameplayCueTag 设为同名 Tag

---

## 九、Tag 决策速查

| 我要做什么 | Tag 命名空间 | 放在哪 |
|---|---|---|
| 触发某个 GA | `Action.*` | SendGameplayEvent 的 Key，GA AbilityTriggers |
| GA 激活期间广播状态 | `Buff.Status.*` | GA Activation Owned Tags |
| GE 生效期间广播状态 | `Buff.Status.*` | GE Granted Tags |
| 让 BT 能找到这个 GA | `Enemy.*` | GA Ability Tags |
| StateConflict 能阻断这个 GA | `Enemy.*` / `PlayerState.*` | GA Ability Tags |
| 死亡/受击时这个 GA 不能激活 | `Buff.Status.*` | GA Activation Blocked Tags |
| 批量阻断同类 GA | StateConflict Rules Block Tags | DataAsset 配置 |
| 停止 AI / 停止移动 | `Block.AI` / `Block.Movement` | BlockCategoryMap 的 Key（不挂 ASC）|
| 查询攻击数据（蒙太奇/伤害）| `Enemy.Melee.*` | AbilityMap 的 Key（DataAsset） |
| 查询死亡/受击动画数据 | `Action.*` | PassiveMap 的 Key（DataAsset） |
| GE 传数值给 GA | `Attribute.*` / `Buff.Data.*` | SetByCallerMagnitude 的 Key |
| Blueprint 查角色状态 | `Buff.Status.*` | Has Matching Gameplay Tag |
| 符文分类/稀有度 | `Buff.Rune.*` | 符文 DA 的 AssetTags |
| 符文触发条件 | `Buff.Trigger.*` | 符文 GA 的 AssetTags + AbilityTriggers |
| 触发消解/特效粒子 | `GameplayCue.*` | ExecuteGameplayCue 的 Key |
