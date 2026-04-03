# 符文效果系统 · 程序技术文档

> 版本：v1.0 | 日期：2026-04-03  
> 对应文档：Rune_Effects_Designer.md  
> 前置文档：BackpackGridComponent_TechDesign.md、LevelSystem_TechDesign_v2.md

---

## 一、总体架构

### 1.1 符文效果分类

所有符文效果按实现模式分为 **8 类**，每类有固定的实现套路：

| 类别 | 枚举值 | 典型例子 | 实现核心 |
|------|--------|---------|---------|
| A | AttributeModifier | 攻击力+10、移速+20% | GE Modifier |
| B | PassiveEventTrigger | 击退、命中燃烧、击杀回血 | GE 授予 GA + WaitEvent |
| C | SpawnOnEvent | 腐烂毒池、死亡爆炸 | GA + SpawnActor |
| D | StatusEffect (DoT/CC) | 持续毒伤、减速、眩晕 | GE Duration+Period |
| E | ConditionalBuff | 血量<30%时增伤、连杀buff | GA + WaitAttributeChange |
| F | Aura | 持续范围减速、范围回血 | GA + Sphere Overlap Timer |
| G | AttackModifier | 穿透、弹射、分裂 | GA 修改攻击行为 |
| H | OnDeathEffect | 爆尸、灵魂逃逸 | GA + WaitEvent(Death) |

### 1.2 统一数据结构扩展

在 `RuneData.h` 中添加：

```cpp
UENUM(BlueprintType)
enum class ERuneEffectCategory : uint8
{
    AttributeModifier       UMETA(DisplayName = "数值修改"),
    PassiveEventTrigger     UMETA(DisplayName = "被动事件触发"),
    SpawnOnEvent            UMETA(DisplayName = "事件生成物"),
    StatusEffect            UMETA(DisplayName = "状态/DoT/CC"),
    ConditionalBuff         UMETA(DisplayName = "条件触发Buff"),
    Aura                    UMETA(DisplayName = "持续范围效果"),
    AttackModifier          UMETA(DisplayName = "攻击行为修改"),
    OnDeathEffect           UMETA(DisplayName = "死亡触发效果"),
};

USTRUCT(BlueprintType)
struct FRuneTemplate
{
    // ... 已有字段 ...

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERuneEffectCategory EffectCategory;

    // 类B/C/G/H：直接授予哪个GA（由GE.GrantedAbilities填写，或此处显式引用）
    // 策划可不填，程序在GE中配置
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UGameplayAbility> GrantedAbility;

    // 等级数据表（C/E/F等需要等级参数的类型）
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UDataTable* LevelDataTable;
};
```

---

## 二、事件命名规范（全局约定）

所有 GA 监听的事件 Tag 统一在此处定义，避免字符串碎片化：

```
GameEvent.Combat.Attack.HitEnemy        攻击命中敌人（携带：Instigator=攻击者，Target=被击中敌人）
GameEvent.Combat.Attack.HitEnemy.Crit   暴击命中（同上）
GameEvent.Combat.Kill                   击杀（携带：Instigator=击杀者，Target=被杀者）
GameEvent.Combat.Damaged.ByEnemy        玩家受到伤害（携带：Instigator=伤害来源，Magnitude=伤害量）
GameEvent.Combat.Dodge.Performed        闪避成功
GameEvent.Life.Death.Self               自身死亡（GA挂在Owner上时用此Tag）
GameEvent.Life.Death.EnemyNearby        附近有敌人死亡（范围内广播）
GameEvent.Combat.Attack.Begin           开始攻击动作（攻击行为修改类用）
```

**广播规范（程序实现）：**

```cpp
// 在合适位置调用，例如 ApplyDamage 后：
FGameplayEventData EventData;
EventData.Instigator = Attacker;
EventData.Target = HitTarget;
EventData.EventMagnitude = DamageAmount;
UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
    Attacker,  // 事件发送给攻击者的ASC
    FGameplayTag::RequestGameplayTag("GameEvent.Combat.Attack.HitEnemy"),
    EventData
);
```

---

## 三、各类别详细实现

### 类A：AttributeModifier（数值修改）

**无需额外代码，策划在 GE 中直接配置 Modifiers。**

支持的 Modifier Op：
- `Add`：加法（攻击力+10）
- `Multiply`：乘法（攻击力×1.2，即+20%）
- `Override`：覆盖（强制设置为某值，慎用）

支持的 Magnitude Type：
- `Scalable Float`：固定值，可配曲线
- `Set by Caller`：运行时传入（用于等级系统）
- `Attribute Based`：基于其他属性计算

---

### 类B：PassiveEventTrigger（被动事件触发）

**套路：** `GE（Infinite + GrantedAbilities）→ GA（OnGranted自动激活 + WaitGameplayEvent循环）`

#### 程序模板：`GA_PassiveEventBase.h/.cpp`

```cpp
UCLASS(Abstract)
class UGA_PassiveEventBase : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    // 子类在蓝图中重写
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag ListenEventTag;

    virtual void ActivateAbility(...) override;

protected:
    // 子类实现：收到事件时的逻辑
    UFUNCTION(BlueprintImplementableEvent)
    void OnEventReceived(const FGameplayEventData& EventData);
};
```

```cpp
void UGA_PassiveEventBase::ActivateAbility(...)
{
    Super::ActivateAbility(...);
    // 不调用 CommitAbility，被动GA无需消耗
    // 启动循环监听（在蓝图中通过 WaitGameplayEvent + 循环节点实现）
}
```

**注意事项：**
1. `Instancing Policy` = `Instanced Per Actor`（每个Actor独立实例）
2. `Net Execution Policy` = `Local or Server`
3. `Activation Policy` = `On Granted`（需要程序在父类中实现，或由BackpackGridComponent在GrantAbility后手动激活）

#### 击退具体实现

```cpp
// GA_Passive_Knockback 在蓝图中实现 OnEventReceived：
// 1. 从 EventData.Target 获取被击目标
// 2. 计算方向：TargetLoc - OwnerLoc
// 3. 调用 LaunchCharacter(Direction * KnockbackStrength, true, true)
//    或 Target->GetCharacterMovement()->AddImpulse(...)

// 可配置变量：
UPROPERTY(EditAnywhere)
float KnockbackStrength = 800.f;

UPROPERTY(EditAnywhere)
float KnockbackZBoost = 200.f; // 上挑高度
```

#### 命中减速

```cpp
// OnEventReceived 中：
// 对 EventData.Target 的 ASC 施加 GE_Debuff_Slow
// GE_Debuff_Slow：Duration=1.5s，Modify MoveSpeed × 0.5

FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SlowEffectClass, GetAbilityLevel());
ApplyGameplayEffectSpecToTarget(Handle, CurrentActualActivationInfo, SpecHandle, Target->GetAbilitySystemComponent());
```

---

### 类C：SpawnOnEvent（事件生成物）

**套路：** `GE → GA（监听事件）→ 事件触发 → SpawnActor → Actor内部自治逻辑`

#### Actor设计规范

生成物 Actor 需遵守以下规范，便于策划配置：

```cpp
UCLASS()
class ARuneSpawnedActorBase : public AActor
{
    GENERATED_BODY()

public:
    // 由 GA 在 Spawn 后立即调用
    UFUNCTION(BlueprintCallable)
    virtual void InitWithLevel(int32 Level, AActor* Owner, UAbilitySystemComponent* OwnerASC);

    // 伤害来源标识（用于计算归因）
    UPROPERTY()
    TWeakObjectPtr<AActor> DamageInstigator;
    
    UPROPERTY()
    TWeakObjectPtr<UAbilitySystemComponent> InstigatorASC;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 CurrentLevel = 1;
};
```

#### 腐烂毒池 Actor 实现要点

```cpp
// BP_RotPoisonPool 中的关键逻辑

// 1. 等级数据表行结构
USTRUCT(BlueprintType)
struct FRotPoolLevelData : public FTableRowBase
{
    UPROPERTY() float MaxRadius = 400.f;
    UPROPERTY() float ExpandDuration = 0.5f;      // 扩展到最大半径的时间
    UPROPERTY() float PoolDuration = 5.f;          // 毒池存在时长
    UPROPERTY() float DamageTickInterval = 0.25f;  // 伤害间隔
    UPROPERTY() float DamagePctToEnemy = 0.07f;    // 普通敌人每秒百分比
    UPROPERTY() float DamagePctToBoss = 0.03f;     // Boss每秒百分比
    UPROPERTY() float DamageToPlayer = 0.f;        // 对玩家固定伤害（0=不伤玩家）
};

// 2. DealDamage 中区分目标类型
void ABP_RotPoisonPool::DealDamageTick()
{
    TArray<AActor*> Overlapping;
    SphereComp->GetOverlappingActors(Overlapping, ACharacterBase::StaticClass());

    for (AActor* Actor : Overlapping)
    {
        if (Actor == DamageInstigator) continue; // 不伤害自己

        float DamageAmount = 0.f;
        ACharacterBase* Char = Cast<ACharacterBase>(Actor);
        if (!Char) continue;

        float CurrentHP = Char->GetAbilitySystemComponent()
            ->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());

        if (Char->IsPlayerCharacter())
        {
            DamageAmount = LevelData.DamageToPlayer * LevelData.DamageTickInterval;
        }
        else if (Char->IsBossCharacter())
        {
            DamageAmount = CurrentHP * LevelData.DamagePctToBoss * LevelData.DamageTickInterval;
        }
        else
        {
            DamageAmount = CurrentHP * LevelData.DamagePctToEnemy * LevelData.DamageTickInterval;
        }

        if (DamageAmount > 0.f)
        {
            // 通过 Set By Caller 传入伤害量
            FGameplayEffectSpecHandle Spec = InstigatorASC->MakeOutgoingSpec(
                DamageGEClass, CurrentLevel, InstigatorASC->MakeEffectContext());
            UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
                Spec, FGameplayTag::RequestGameplayTag("SetByCaller.Damage.Poison"), -DamageAmount);
            InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Char->GetAbilitySystemComponent());
        }
    }
}
```

---

### 类D：StatusEffect（DoT/CC 状态效果）

**套路：** 纯 GE 实现，策划可自行配置，程序只需确保 AttributeSet 有对应字段。

#### DoT（持续伤害）GE 配置规范

```
Duration Policy = Has Duration
Duration Magnitude = [持续时间，如5.0]
Period = [伤害间隔，如0.5]
Execute Periodic Effect on Application = true（立即触发第一次）

Modifier：
  Attribute = Health
  Op = Add
  Magnitude = Set by Caller (Tag: SetByCaller.Damage.Poison)
  值为负数（扣血）
```

#### CC（控制）效果：需要程序支持的 Attribute

```cpp
// BaseAttributeSet 中需要有：
UPROPERTY()
FGameplayAttributeData MoveSpeedMultiplier;   // 移动速度倍率，默认1.0

UPROPERTY()
FGameplayAttributeData AttackSpeedMultiplier; // 攻击速度倍率，默认1.0

// 眩晕需要 GameplayTag：
// State.Debuff.Stun → 角色收到此Tag时禁用输入
// State.Debuff.Freeze → 禁用输入+停止动画
```

#### 程序需实现的 Tag 响应

```cpp
// APlayerCharacterBase::BeginPlay 中注册：
AbilitySystemComponent->RegisterGameplayTagEvent(
    FGameplayTag::RequestGameplayTag("State.Debuff.Stun"),
    EGameplayTagEventType::NewOrRemoved
).AddUObject(this, &APlayerCharacterBase::OnStunTagChanged);

void APlayerCharacterBase::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    bIsStunned = NewCount > 0;
    if (bIsStunned)
    {
        GetCharacterMovement()->DisableMovement();
        // 也可禁用InputComponent
    }
    else
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}
```

---

### 类E：ConditionalBuff（条件触发）

**套路：** `GA（OnGranted激活）→ WaitAttributeChange 或 WaitGameplayTagAdd → 条件满足时Apply/Remove GE`

#### 血量阈值触发

```cpp
// GA_Passive_LowHpBuff 蓝图逻辑：
// 1. WaitAttributeChange(Health)
// 2. 每次变化：计算 HP% = Health / MaxHealth
// 3. HP% < Threshold → ApplyGameplayEffect(GE_Buff_LowHp) → 记录ActiveHandle
// 4. HP% >= Threshold → RemoveActiveGameplayEffect(ActiveHandle)

// 程序需暴露：
UPROPERTY(EditAnywhere)
float HpThreshold = 0.3f;  // 30%触发

UPROPERTY(EditAnywhere)
TSubclassOf<UGameplayEffect> BuffEffect;
```

#### 连杀计数触发

```cpp
// GA_Passive_KillStreakBuff
// 1. WaitGameplayEvent(GameEvent.Combat.Kill)，计数器+1
// 2. Set Timer 重置计数器（如3秒内没有击杀则重置）
// 3. KillCount >= StreakThreshold → Apply GE_Buff_KillStreak

// 注意：KillCount 不要存在 AttributeSet，存在 GA 实例变量中即可
int32 CurrentKillStreak = 0;
FTimerHandle ResetTimerHandle;
```

---

### 类F：Aura（持续范围效果）

**套路：** `GE → GA（OnGranted）→ SetTimer 周期检测 → Apply/Remove GE 到范围内目标`

#### 通用 Aura GA 模板

```cpp
UCLASS()
class UGA_Passive_AuraBase : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    float AuraRadius = 300.f;

    UPROPERTY(EditAnywhere)
    float TickInterval = 0.5f;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UGameplayEffect> AuraEffect;  // 施加给范围内目标的GE

    // 影响的目标类型
    UPROPERTY(EditAnywhere)
    bool bAffectEnemies = true;

    UPROPERTY(EditAnywhere)
    bool bAffectAllies = false;

protected:
    FTimerHandle AuraTimerHandle;
    TMap<AActor*, FActiveGameplayEffectHandle> ActiveEffects; // 记录已施加的GE以便移除

    void AuraTick();
};
```

```cpp
void UGA_Passive_AuraBase::AuraTick()
{
    TArray<AActor*> InRange = GetOverlappingActors(AuraRadius);
    TSet<AActor*> InRangeSet(InRange);

    // 移除已不在范围内的GE
    for (auto It = ActiveEffects.CreateIterator(); It; ++It)
    {
        if (!InRangeSet.Contains(It->Key))
        {
            UAbilitySystemComponent* TargetASC = GetASC(It->Key);
            if (TargetASC) TargetASC->RemoveActiveGameplayEffect(It->Value);
            It.RemoveCurrent();
        }
    }

    // 对新进入范围的目标施加GE
    for (AActor* Actor : InRange)
    {
        if (!ActiveEffects.Contains(Actor))
        {
            UAbilitySystemComponent* TargetASC = GetASC(Actor);
            if (TargetASC)
            {
                FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(AuraEffect, GetAbilityLevel());
                FActiveGameplayEffectHandle Handle = ApplyGameplayEffectSpecToTarget(...);
                ActiveEffects.Add(Actor, Handle);
            }
        }
    }
}
```

---

### 类G：AttackModifier（攻击行为修改）

**攻击行为修改需要在攻击系统层面支持Hook点，程序需要预留以下扩展：**

#### 程序需在攻击系统中预留的接口

```cpp
// 在 AttackComponent 或 GA_Attack 中：

// Hook 1: 攻击发射前（修改方向、数量、类型）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeforeAttackLaunch, FAttackParams&, Params);
UPROPERTY(BlueprintAssignable)
FOnBeforeAttackLaunch OnBeforeAttackLaunch;

// Hook 2: 投射物/判定框生成后（修改属性）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileSpawned, AYogProjectile*, Projectile);
UPROPERTY(BlueprintAssignable)
FOnProjectileSpawned OnProjectileSpawned;

// FAttackParams 结构：
USTRUCT(BlueprintType)
struct FAttackParams
{
    UPROPERTY() int32 ProjectileCount = 1;    // 发射数量
    UPROPERTY() float SpreadAngle = 0.f;      // 散布角
    UPROPERTY() bool bPiercing = false;       // 穿透
    UPROPERTY() int32 BounceCount = 0;        // 弹射次数
    UPROPERTY() bool bHoming = false;         // 追踪
    UPROPERTY() float SizeMultiplier = 1.f;   // 大小倍率
};
```

#### 穿透符文 GA

```cpp
// GA_Passive_Piercing：OnGranted 后绑定 OnBeforeAttackLaunch
// 在回调中设置 Params.bPiercing = true

// 投射物 AYogProjectile 需要支持：
int32 RemainingPierceCount = 0;

void AYogProjectile::OnHit(AActor* OtherActor)
{
    // 正常伤害逻辑...
    if (RemainingPierceCount > 0)
    {
        RemainingPierceCount--;
        // 不销毁，继续飞行
    }
    else
    {
        Destroy();
    }
}
```

---

### 类H：OnDeathEffect（死亡触发效果）

**适用于敌人身上的符文，或玩家的"死亡爆发"类符文。**

#### 死亡事件广播规范

```cpp
// ACharacterBase::Die() 中（程序实现）：
void ACharacterBase::Die()
{
    // ... 正常死亡逻辑 ...

    FGameplayEventData EventData;
    EventData.Instigator = this;
    EventData.Target = this;

    // 发给自己（自身GA监听）
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        this,
        FGameplayTag::RequestGameplayTag("GameEvent.Life.Death.Self"),
        EventData
    );

    // 发给周围300范围内的Actor（旁观者GA监听）
    // 例如"附近敌人死亡时回血"类符文需要这个
    TArray<AActor*> Nearby = GetActorsInRadius(300.f);
    for (AActor* Actor : Nearby)
    {
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
            Actor,
            FGameplayTag::RequestGameplayTag("GameEvent.Life.Death.EnemyNearby"),
            EventData
        );
    }
}
```

---

## 四、Set By Caller Tag 规范

所有通过 Set By Caller 传入的参数，Tag 统一命名：

```
SetByCaller.Damage.Physical       物理伤害量（负数）
SetByCaller.Damage.Poison         毒伤量（负数）
SetByCaller.Damage.Fire           火焰伤害（负数）
SetByCaller.Damage.Pure           纯粹伤害，无视防御（负数）
SetByCaller.Heal                  治疗量（正数）
SetByCaller.Duration              持续时间覆盖
SetByCaller.Radius                范围覆盖
SetByCaller.Magnitude             通用数值覆盖
SetByCaller.StackCount            叠层数
```

---

## 五、等级系统与 DataTable 规范

### 5.1 标准等级行结构基类

```cpp
// 所有符文等级DataTable的行结构都继承此基类
USTRUCT(BlueprintType)
struct FRuneLevelDataBase : public FTableRowBase
{
    GENERATED_BODY()
    
    // 等级描述，供策划标注
    UPROPERTY(EditAnywhere)
    FText LevelDescription;
};
```

### 5.2 从 GA 中读取等级数据

```cpp
// 在 GA 的 ActivateAbility 或 Init 中：
template<typename T>
T* GetLevelData(UDataTable* Table, int32 Level)
{
    FString RowName = FString::Printf(TEXT("Level%d"), Level);
    return Table->FindRow<T>(FName(*RowName), TEXT(""));
}

// 使用：
FRotPoolLevelData* Data = GetLevelData<FRotPoolLevelData>(LevelTable, GetAbilityLevel());
```

### 5.3 RowName 规范

DataTable 中行名固定格式：`Level1`、`Level2`、`Level3`（不用 0 开头）

---

## 六、GE 激活与移除时序

```
BackpackGridComponent::ActivateRune(RuneInstance)
    → ASC->ApplyGameplayEffectToSelf(RuneInstance.ActivationEffect, Level, Context)
    → GE.GrantedAbilities 中的 GA 被授予并自动激活
    → GA 开始监听事件

BackpackGridComponent::DeactivateRune(RuneInstance)
    → ASC->RemoveActiveGameplayEffect(RuneInstance.ActiveGEHandle)
    → GE 移除 → GA 被取消（Cancel + Remove）
    → GA.EndAbility 中清理 Timer、解绑委托
```

**GA 必须在 EndAbility 中清理：**

```cpp
void UGA_PassiveBase::EndAbility(...)
{
    // 清理所有Timer
    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    
    // 清理施加给其他Actor的GE
    for (auto& Pair : ActiveEffectsOnTargets)
    {
        if (UAbilitySystemComponent* ASC = GetASC(Pair.Key))
            ASC->RemoveActiveGameplayEffect(Pair.Value);
    }
    ActiveEffectsOnTargets.Empty();

    Super::EndAbility(...);
}
```

---

## 七、各类别所需程序工作量估算

| 类别 | 程序工作 | 策划工作 | 依赖 |
|------|---------|---------|------|
| A 数值修改 | 0h（已完成） | 直接配GE | BackpackGridComponent |
| B 被动触发 | 4h（事件广播+GA模板） | 创建具体GA蓝图 | 事件系统 |
| C 生成物 | 3h/个（Actor基类+样例） | 配DataTable+BP逻辑 | B类基础 |
| D DoT/CC | 2h（Tag响应+Attribute扩展） | 直接配GE | AttributeSet扩展 |
| E 条件触发 | 2h（GA模板） | 配参数 | B类基础 |
| F Aura | 3h（AuraBase类） | 配参数+GE | B类基础 |
| G 攻击修改 | 5h（AttackParams Hook） | 配GA | 攻击系统Hook |
| H 死亡触发 | 1h（死亡广播） | 同C类 | C类基础 |

**建议实现顺序：** A → B → D → C/H → E → F → G

---

*文档版本 v1.0 | 对应策划文档：Rune_Effects_Designer.md*