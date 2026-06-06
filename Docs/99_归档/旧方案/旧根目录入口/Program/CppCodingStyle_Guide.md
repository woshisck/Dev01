> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# C++ Coding Style Guide

> 适用范围：DevKit 模块所有 C++ 源文件（.h / .cpp）  
> 适用人群：程序  
> 配套文档：[编码规范总览](../../../../04_开发实现与系统文档/编码规范/_README.md) | [GAS 规范](../../../../04_开发实现与系统文档/编码规范/GAS.md)  
> 最后更新：2026-05-20

---

## 命名规则

| 类型 | 前缀 / 规则 | 示例 |
|---|---|---|
| 项目 UObject / AActor 类 | `Yog` 前缀 | `AYogGameMode`, `UYogAbilitySystemComponent` |
| Gameplay Ability | `GA_` 前缀 | `UGA_MeleeAttack`, `UGA_PlayerDash` |
| AnimNotify | `AN_` 前缀 | `UAN_MeleeDamage` |
| AnimNotifyState | `ANS_` 前缀 | `UANS_AddGameplayTag` |
| BuffFlow 节点 | `BFNode_` 前缀 | `UBFNode_HitStop` |
| DataAsset 类 | `*DataAsset` 或 `*DA` 后缀 | `URuneDataAsset`, `UMontageConfigDA` |
| bool 成员变量 | `b` 前缀 | `bActiveComboNodeValid`, `bWasCancelled` |
| bool 局部变量 | `b` 前缀 | `bool bFound = false;` |
| 文件局部辅助函数 | 匿名命名空间包裹，`ModuleName_FuncName` 命名 | `MeleeAttack_FrameToMontageTime` |
| 静态常量 GameplayTag | `static const FGameplayTag TAG_Xxx` | `static const FGameplayTag TAG_ActDamage` |

---

## 文件结构

### 头文件（.h）

```
#pragma once

#include "CoreMinimal.h"
#include <UE 头文件>
#include "Xxx.generated.h"   // 始终最后一个 include

// Forward declarations（仅 pointer/reference 用到的类型）
class UFoo;

UCLASS()
class DEVKIT_API UMyClass : public UBase
{
    GENERATED_BODY()

public:
    // 1. 构造函数
    // 2. public UPROPERTY（Blueprint 可见的配置项）
    // 3. public 方法

protected:
    // virtual 覆写（ActivateAbility / Tick 等）
    // 子类需要调用的方法

private:
    // UFUNCTION 回调（OnMontageCompleted 等）
    // 内部状态变量
    // 内部辅助方法
};
```

### 源文件（.cpp）

```
#include "Module/MyClass.h"   // 对应头文件始终第一行

// UE 引擎头文件
#include "AbilitySystemComponent.h"

// 项目内头文件（按子系统分组，组内字母序）
#include "AbilitySystem/..."
#include "Character/..."
#include "Component/..."

// 文件局部辅助（匿名命名空间）
namespace
{
    // helper structs / free functions
}
```

---

## 格式规范

### 大括号风格

所有代码块均使用 **Allman 风格**，无例外：开括号始终另起一行。

适用范围：函数体、类体、if / for / while / switch、lambda、初始化列表块。

```cpp
void UGA_MeleeAttack::ActivateAbility(...)
{
    if (!Montage)
    {
        EndAbility(...);
        return;
    }

    for (AActor* Actor : Actors)
    {
        Actor->Destroy();
    }

    auto Callback = [this]()
    {
        DoSomething();
    };
}
```

**禁止**将开括号放在同一行：

```cpp
// 错误
void Foo() {
if (bFlag) {
```

### 缩进与对齐

- 缩进：**1 Tab**（与 UE 默认一致）
- 同组连续赋值允许对齐等号，但同一语句块内保持统一：

```cpp
bActiveComboNodeValid       = false;
bCombatDeckFromDashSave     = false;
ActiveAttackGuid.Invalidate();
```

- 长参数列表在第二个参数起对齐到第一个参数：

```cpp
UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
    this,
    NAME_None,
    Montage,
    DamageEventTags,
    AttackSpeedRate,
    NAME_None,
    true,
    1.0f);
```

---

## 指针与内存安全

| 场景 | 做法 |
|---|---|
| 非拥有的 UObject 引用（跨帧持有） | `TWeakObjectPtr<T>` |
| 使用前检查 | `IsValid(Ptr)` 或 `Ptr != nullptr` |
| UObject 不允许裸指针拥有 | 用 `UPROPERTY()` 确保 GC 追踪 |
| Map key 为 UObject* | `TObjectKey<T>` 而非裸指针 |
| 临时取得的 ASC / Component | 局部变量 + 立即使用，不缓存 |

```cpp
// 正确
if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
{
    ASC->AddLooseGameplayTag(...);
}

// 跨帧引用
TWeakObjectPtr<UAbilitySystemComponent> WeakASC(CombatCardASC);
```

---

## GameplayTag 使用规则

```cpp
// 高频标签 → static const，避免每帧字符串查找
static const FGameplayTag TAG_CanCombo =
    FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));

// 一次性使用 → 内联即可
ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Attacking")));

// 不要在头文件里声明 static FGameplayTag 成员（构造顺序不确定）
```

---

## 注释规则

只在 **WHY 不明显** 时写注释：隐藏约束、微妙不变量、绕过特定 Bug 的 workaround。

```cpp
// 必须在 Super::ActivateAbility 之前清空，否则 retrigger 时旧帧数据污染新激活。
bActiveComboNodeValid = false;
```

**不写**描述代码在做什么的注释（好的命名已经说明了）：

```cpp
// 不要这样写：
// 获取玩家角色
APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(...);
```

**All comments must be in English only. Never use Chinese in code comments.**

When writing doc files (`.md`), CJK and Latin characters must be separated by a space: `GA_Dead 激活时` not `GA_Dead激活时`.

---

## Early Return 模式

优先用 early return 减少嵌套：

```cpp
void UGA_MeleeAttack::TryStartEnemyRadialLunge()
{
    if (!ActiveEnemyAttackContext.bValid) return;

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    AActor* TargetActor = ActiveEnemyAttackContext.TargetActor.Get();
    if (!Character || !TargetActor) return;

    // 主逻辑
}
```

---

## 浮点比较

```cpp
// 比较浮点相等
FMath::IsNearlyEqual(A, B, KINDA_SMALL_NUMBER)

// 比较接近零
FMath::IsNearlyZero(Value)

// 防止除零
const float Rate = (PlayRate > KINDA_SMALL_NUMBER) ? PlayRate : 1.0f;
```

---

## Lambda 规范

- 显式列出捕获变量，不用 `[=]` / `[&]` 全捕获
- 跨帧 lambda 捕获 UObject 用 `TWeakObjectPtr`，使用前 `IsValid` 检查
- 类型复杂时用 `auto` 声明 lambda，但参数类型需明确

```cpp
TWeakObjectPtr<UAbilitySystemComponent> WeakASC(CombatCardASC);
GetWorld()->GetTimerManager().SetTimerForNextTick(
    FTimerDelegate::CreateLambda([WeakASC, PreAttack]()
    {
        if (UAbilitySystemComponent* ASC = WeakASC.Get())
        {
            ASC->SetNumericAttributeBase(..., PreAttack);
        }
    }));
```

---

## UPROPERTY 标注规则

| 场景 | Specifier |
|---|---|
| 策划在 BP 默认值里配置 | `EditDefaultsOnly` |
| 运行时只读（蓝图可读） | `BlueprintReadOnly` |
| 运行时可读写 | `BlueprintReadWrite` |
| 纯内部状态，不暴露 | 不加 `UPROPERTY`（不被 GC 追踪）或 `UPROPERTY()` 空标注（被 GC 追踪但不暴露编辑器） |
| Category | 按子系统命名，如 `"Attack"`, `"Combo"`, `"CombatDeck"` |

```cpp
// 策划配置
UPROPERTY(EditDefaultsOnly, Category = "Attack")
TSubclassOf<UGameplayEffect> StatBeforeATKEffect;

// GC 追踪的内部指针
UPROPERTY()
TObjectPtr<UAN_MeleeDamage> CachedDamageNotify;
```

---

## 常见踩坑

| 问题 | 规则 |
|---|---|
| `EndAbility` 没调 `Super` | 必须调，否则 GA 不释放 |
| `CanActivateAbility` 里修改状态 | 禁止，是 `const` 方法；需缓存则用 `mutable` |
| 跨帧裸 UObject* | 改用 `TWeakObjectPtr` + `IsValid` 检查 |
| 高频 `RequestGameplayTag` 字符串查找 | 缓存为 `static const FGameplayTag` |
| 文件局部 helper 污染全局命名空间 | 包在匿名 `namespace {}` 内 |
| `float` 除法前未检查除数 | 用 `> KINDA_SMALL_NUMBER` 判断 |

