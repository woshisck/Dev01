# C++ Coding Style Guide

> 适用范围：DevKit 模块所有 C++ 源文件（.h / .cpp）  
> 适用人群：程序  
> 配套文档：[编码规范总览](_README.md) | [GAS 规范](GAS.md)  
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

// 文件局部辅助函数 / 常量 —— 直接用 static
static bool MyClass_IsTargetValid(const AActor* Actor)
{
    return IsValid(Actor);
}

// 文件局部类型 —— 直接裸写，用文件名前缀保证全局唯一
struct FMyClassLocalHelper
{
    int32 Index = INDEX_NONE;
};
```

### 禁止匿名命名空间

**`.cpp` 里一律不准写匿名 `namespace {}`。** 这是硬性规定，没有例外。

过去的规范要求把文件局部 helper 包进匿名命名空间，结果整个代码库里堆了 176 个这样的块，
读代码时每个文件都要多扒一层缩进，收益却几乎为零。现在全部改掉。

替代写法：

| 文件局部的东西 | 写法 |
|---|---|
| 函数、模板函数 | `static`（模板写在 `template <...>` 行之后） |
| 常量、变量 | `static`（`static constexpr` / `static const`） |
| `struct` / `class` / `enum` / `using` / `typedef` | 裸写，不包任何命名空间；靠**文件名前缀**保证名字全局唯一 |

```cpp
template <typename TAttributeSet>
static TAttributeSet* GetMutableRegisteredAttributeSet(UAbilitySystemComponent* ASC)
{
    ...
}
```

#### 为什么类型要加文件名前缀

`DevKit` 运行时模块启用了 Unity 构建（`bUseUnity` 默认 `true`），多个 `.cpp` 会被拼成同一个
翻译单元。函数和变量有 `static` 兜底，重名也不会冲突；但 `static` 不能作用于类型，所以文件
局部的 `struct` 一旦和别的 `.cpp` 里的同名 `struct` 撞上，就会在 Unity 块里重定义——这种报错
只在部分构建配置下出现，单文件重编时又会消失，极难排查。

所以文件局部类型必须带上所属文件/系统的前缀，让名字天然唯一：

```cpp
// PortalPreviewWidget.cpp
struct FPortalPreviewRoomTypeStyle { ... };   // 不是 FRoomTypeStyle

// GA_MeleeAttack.cpp
struct FMeleeAttackComboEntry { ... };        // 不是 FComboEntry
```

函数命名同样沿用 `ModuleName_FuncName`，便于在调试器和 Profiler 里区分同名 helper。

> 命名空间本身没有被全面禁止：头文件里用具名 `namespace`（如 `YogStateTree`、
> `CombatVFXCascade`）给一组公开 API 做分组仍然可以。被禁的只是 `.cpp` 里的匿名命名空间。

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
    FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));

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
| 在 `.cpp` 里写匿名 `namespace {}` | 禁止；函数 / 常量用 `static`，局部类型裸写并加文件名前缀 |
| `float` 除法前未检查除数 | 用 `> KINDA_SMALL_NUMBER` 判断 |
