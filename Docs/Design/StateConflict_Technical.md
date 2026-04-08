# 状态冲突系统 — 技术文档

---

## 架构总览

```
UStateConflictDataAsset          策划填表（Rules[]）
        │
        │ InitConflictTable()
        ▼
TMap<FGameplayTag, FStateConflictRule>   ConflictMap（O(1) 查找）
        │
        │ OnTagUpdated() override
        ▼
YogAbilitySystemComponent
  ├── BlockAbilitiesWithTags()    → 阻止 GA 激活
  ├── UnBlockAbilitiesWithTags()  → 解除阻止
  └── CancelAbilities()          → 立即取消 GA
```

---

## 核心机制：OnTagUpdated

`UAbilitySystemComponent::OnTagUpdated(Tag, TagExists)` 是 GAS 内部的 virtual hook，
**所有来源的 tag 变化都经过它**：LooseTag、GE GrantedTag、GA 授予的 Tag 均覆盖。

```cpp
void UYogAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
    Super::OnTagUpdated(Tag, TagExists);  // 保留原有逻辑

    if (bProcessingConflict) return;      // 防止递归

    const FStateConflictRule* Rule = ConflictMap.Find(Tag);
    if (!Rule) return;

    TGuardValue<bool> Guard(bProcessingConflict, true);

    if (TagExists)
    {
        if (!Rule->BlockTags.IsEmpty())  BlockAbilitiesWithTags(Rule->BlockTags);
        if (!Rule->CancelTags.IsEmpty()) CancelAbilities(&Rule->CancelTags);
    }
    else
    {
        if (!Rule->BlockTags.IsEmpty())  UnBlockAbilitiesWithTags(Rule->BlockTags);
    }
}
```

### 为什么不 override AddLooseGameplayTag

`AddLooseGameplayTag` 不是 virtual，shadowing 只能拦截直接调用路径，
GE GrantedTag 和 GA 内部授予的 Tag 均会绕过。`OnTagUpdated` 是唯一全覆盖的 hook。

### 递归保护

`BlockAbilitiesWithTags` 内部会修改 `BlockedAbilityTags`（也是 FGameplayTagCountContainer），
这会再次触发 `OnTagUpdated`。用 `bProcessingConflict` guard 打断递归。

---

## 初始化

### 推荐位置（二选一）

**方案 A：全局（推荐）**

在 `GameInstance` 或项目级 `UDataAsset` 中持有一份引用，
在角色 `InitAbilityActorInfo` 之后调用：

```cpp
// PlayerCharacterBase.cpp
void APlayerCharacterBase::BeginPlay()
{
    Super::BeginPlay();
    // ...
    UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ASC);
    if (YogASC)
    {
        // 从 GameInstance / GameData 取全局表
        YogASC->SetConflictTable(UMyGameInstance::Get()->StateConflictTable);
    }
}
```

**方案 B：每个角色蓝图 CDO 直接配置**

在角色蓝图的 `YogAbilitySystemComponent` Details 面板中找到
`StateConflict → Conflict Table`，拖入 DataAsset。
`InitConflictTable()` 在首次 `InitAbilityActorInfo` 时自动调用。

> ⚠️ 方案 B 需要每个角色蓝图手动配置，漏配时系统静默跳过（输出 Verbose log），不崩溃。

---

## GA 侧唯一需要配置的字段

系统基于 `AbilityTags` 做匹配，GA 只需正确填写自己的身份 Tag：

```
GA_Attack   → Class Defaults → Ability Tags: Action.Attack
GA_Move     → Class Defaults → Ability Tags: Action.Move
```

**不需要**在 GA 里配置 `ActivationBlockedTags` 或 `BlockAbilitiesWithTag`。

---

## 文件清单

| 文件 | 类型 | 说明 |
|------|------|------|
| `Source/DevKit/Public/Data/StateConflictDataAsset.h` | C++ Header | FStateConflictRule + UStateConflictDataAsset 定义 |
| `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h` | C++ Header | ConflictTable 属性 + OnTagUpdated 声明 |
| `Source/DevKit/Private/AbilitySystem/YogAbilitySystemComponent.cpp` | C++ | InitConflictTable / SetConflictTable / OnTagUpdated 实现 |
| `Content/Data/DA_StateConflictTable.uasset` | DataAsset | 策划填写的规则表（待创建） |

---

## 扩展计划

| 功能 | 状态 | 说明 |
|------|------|------|
| 基础 Block / Cancel | ✅ 已实现 | |
| Tag 移除自动解除 Block | ✅ 已实现 | |
| Priority 优先级打断 | 🔲 预留字段 | 需要在 OnTagUpdated 中比较当前活跃规则优先级 |
| 每角色差异化规则 | 🔲 可选 | SetConflictTable() 接口已预留 |
| 运行时规则热更新 | 🔲 可选 | SetConflictTable() 接口已预留 |
