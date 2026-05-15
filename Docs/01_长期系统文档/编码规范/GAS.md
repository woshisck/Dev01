# GAS 规范（GA / GE / Attribute）

> Claude 在创建 GA、GE、Attribute 相关 C++ 代码前必须阅读。  
> Tag 字段配置规则见 [GA_TagFields_Guide.md](../标签/GA_TagFields_Guide.md)

---

## 核心规则

1. **GA 是 C++ 父类 + Blueprint 子类**：C++ 写逻辑，Blueprint 填 Tag 字段和蒙太奇
2. **Tag 字段在 Blueprint Class Defaults 里填**，不在 C++ 硬编码
3. **GA 激活判断走 CanActivateAbility**，不在 ActivateAbility 里做条件判断后 EndAbility
4. **GE 伤害 / 属性修改**走 EffectContainerMap，不在 GA C++ 里直接操作属性
5. **符文/Buff 效果** 走 BuffFlow FA，不直接授予 GA 到角色蓝图

---

## 分工

### Claude 写：
- GA C++ 父类（ActivateAbility / EndAbility / CanActivateAbility 逻辑）
- GE C++ 基类（如需自定义计算）
- AttributeSet C++ 类（属性定义 + getter/setter 宏）
- 事件绑定（WaitGameplayEvent / WaitMontageNotify）

### 用户在引擎里做：
- 新建 Blueprint GA，Parent Class 选 C++ GA 类
- 填写 Tag 字段（AbilityTags / ActivationOwnedTags / ActivationBlockedTags 等）
- 在 CharacterData → AbilityData Map 里配置蒙太奇（Key = AbilityTag）
- 创建 GE Blueprint，设置 Modifiers
- 在 GA Blueprint 的 EffectContainerMap 里配置 GE

---

## GA Tag 字段模板

```
AbilityTags              = PlayerState.AbilityCast.Attack.Light
ActivationOwnedTags      = Buff.Status.Attacking          ← GA 激活期间持有
ActivationBlockedTags    = Buff.Status.Dead, Buff.Status.Stunned
CancelAbilitiesWithTag   = PlayerState.AbilityCast        ← 激活时取消同类 GA
BlockAbilitiesWithTag    = （通常不填）
```

---

## GA 标准结构

```cpp
UCLASS(BlueprintType, Blueprintable)
class UGA_XxxAttack : public UYogGameplayAbility
{
    GENERATED_BODY()
public:
    // 策划配置 ─────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float Damage = 30.f;

    virtual bool CanActivateAbility(...) const override;
protected:
    virtual void ActivateAbility(...) override;
    virtual void EndAbility(...) override;
private:
    UFUNCTION() void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
};
```

```cpp
void UGA_XxxAttack::ActivateAbility(...)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 1. 绑定蒙太奇事件
    FGameplayAbilityTargetingLocationInfo DummyInfo;
    auto* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, GetMontageFromAbilityData(ActorInfo), 1.f);
    Task->OnCompleted.AddDynamic(this, &UGA_XxxAttack::OnMontageCompleted);
    Task->OnInterrupted.AddDynamic(this, &UGA_XxxAttack::OnMontageInterrupted);
    Task->OnCancelled.AddDynamic(this, &UGA_XxxAttack::OnMontageInterrupted);
    Task->ReadyForActivation();

    // 2. 等待命中事件（在蒙太奇 AnimNotify 里触发 Event.Attack 事件）
    auto* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(TEXT("Event.Attack")));
    EventTask->EventReceived.AddDynamic(this, &UGA_XxxAttack::OnHitEvent);
    EventTask->ReadyForActivation();
}
```

---

## GE 伤害配置约定

- 伤害 GE 继承 `UGameplayEffect`，Modifiers 用 `Attribute Based` 或 `Set By Caller`
- 在 GA 的 `EffectContainerMap` 里配置，Key = `Event.Attack.Hit`
- 不在 C++ 里 `ApplyGameplayEffectToTarget` 硬编码，除非有特殊计算逻辑

---

## AttributeSet 模式

```cpp
// Header：每个属性三个宏
UPROPERTY(BlueprintReadOnly, Category = "Ammo")
FGameplayAttributeData Ammo;
ATTRIBUTE_ACCESSORS(UMusketAttributeSet, Ammo)

// CPP：PreAttributeChange 做 Clamp
void UMusketAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetAmmoAttribute())
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAmmo());
}
```

---

## 充能/CD 走 SkillChargeComponent

冲刺等多段充能 GA **不用** `CommitAbility`（UE 自带 CD 不支持多段），走 `USkillChargeComponent`：
- `MaxCharge`：最大充能次数
- `CooldownDuration`：单次充能恢复时间
- GA 的 `CanActivateAbility` 调 `Component->HasCharge()`
- GA 的 `ActivateAbility` 里调 `Component->ConsumeCharge()`

---

## 常见踩坑

| 问题 | 解决 |
|---|---|
| GA 蒙太奇不播放 | 检查 CharacterData → AbilityData Map，Key 必须匹配 AbilityTags |
| GAS deprecated API 警告 | `InheritableGameplayEffectTags` / `InheritableOwnedTagsContainer` 旧 API，暂不影响运行 |
| CanActivateAbility 里修改状态 | 不允许，CanActivate 是 const 方法。需要缓存则用 `mutable` |
| EndAbility 没调 Super | 必须调 `Super::EndAbility`，否则 GA 不释放 |
