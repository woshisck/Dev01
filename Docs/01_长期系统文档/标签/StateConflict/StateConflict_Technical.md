# 状态冲突系统 — 技术文档

---

## 架构总览

```
DefaultGame.ini → DevAssetManager → UStateConflictDataAsset（全局单例）
                                            │
                          ┌─────────────────┴──────────────────┐
                          │                                     │
                    Rules[]                            BlockCategoryMap
                          │                                     │
                    InitConflictTable()              InitConflictTable()
                          │                                     │
               TMap<Tag, FStateConflictRule>      TMap<Tag, FGameplayTagContainer>
               ConflictMap（O(1)查找）             BlockCategoryMap（O(1)查找）
                          │                                     │
                          └──────────── OnTagUpdated() ─────────┘
                                               │
                         ┌─────────────────────┼──────────────────────┐
                         │                     │                      │
              BlockAbilitiesWithTags    DisableMovement()     PauseLogic() (AI)
              CancelAbilities()         EnableMovement()      ResumeLogic()
```

---

## 核心机制：OnTagUpdated

`UAbilitySystemComponent::OnTagUpdated(Tag, TagExists)` 是 GAS 内部的 virtual hook，
**所有来源的 tag 变化都经过它**：LooseTag、GE GrantedTag、GA 授予的 Tag 均覆盖。

### GA 冲突逻辑（Rules[]）

```cpp
if (bProcessingConflict) return;  // 防递归

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
```

### 系统级阻断逻辑（BlockCategoryMap）

```cpp
if (const TArray<FGameplayTag>* Categories = StateToBlockCategories.Find(Tag))
{
    // Block.Movement：控制移动组件
    if (Categories->Contains(FGameplayTag::RequestGameplayTag("Block.Movement")))
    {
        if (TagExists)
        {
            Owner->DisableMovement();
            AI->StopMovement();
        }
        else
        {
            // 检查同类别其他 Tag 是否还在激活
            bool bStillBlocked = /* 遍历 BlockCategoryMap[Block.Movement] */;
            if (!bStillBlocked && Owner->IsAlive())
                Owner->EnableMovement();
        }
    }

    // Block.AI：控制行为树（对玩家无效）
    if (Categories->Contains(FGameplayTag::RequestGameplayTag("Block.AI")))
    {
        if (TagExists) Brain->PauseLogic(Tag.ToString());
        else if (!bStillBlocked) Brain->ResumeLogic(Tag.ToString());
    }
}
```

### 为什么不 override AddLooseGameplayTag

`AddLooseGameplayTag` 不是 virtual，shadowing 只能拦截直接调用路径，
GE GrantedTag 和 GA 内部授予的 Tag 均会绕过。`OnTagUpdated` 是唯一全覆盖的 hook。

### 递归保护

`BlockAbilitiesWithTags` 内部会修改 `BlockedAbilityTags`（也是 FGameplayTagCountContainer），
这会再次触发 `OnTagUpdated`。用 `bProcessingConflict` guard 打断递归。
系统级阻断（BlockCategoryMap）在递归保护之前执行，不受影响。

---

## 初始化

### 全局自动加载（当前方案）

DA 路径在 `Config/DefaultGame.ini` 中配置：

```ini
[/Script/DevKit.DevAssetManager]
StateConflictData=/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_StateConflict_Initial.DA_Base_StateConflict_Initial
```

`InitConflictTable()` 在 `InitAbilityActorInfo` 之后调用时，若角色蓝图未手动赋值 `ConflictTable`，
自动从 `UDevAssetManager::Get().GetStateConflictData()` 加载全局表。

**无需在每个角色蓝图中手动配置。**

### 每角色单独覆盖（可选）

若某个角色需要不同规则，在其 ASC 的 Details 面板设置 `StateConflict → ConflictTable`，
或运行时调用：

```cpp
YogASC->SetConflictTable(MyCustomTable);
```

---

## InitConflictTable 内部流程

```cpp
void UYogAbilitySystemComponent::InitConflictTable()
{
    // 1. 若蓝图未赋值，从 DevAssetManager 全局加载
    if (!ConflictTable)
        ConflictTable = UDevAssetManager::Get().GetStateConflictData();

    // 2. 构建 GA 冲突查找表
    for (const FStateConflictRule& Rule : ConflictTable->Rules)
        ConflictMap.Add(Rule.ActiveTag, Rule);

    // 3. 构建系统级阻断表 + 反向索引
    for (const auto& [Category, StateTags] : ConflictTable->BlockCategoryMap)
    {
        BlockCategoryMap.Add(Category, StateTags);
        for (const FGameplayTag& StateTag : StateTags)
            StateToBlockCategories.FindOrAdd(StateTag).Add(Category);
    }
}
```

---

## GA 侧唯一需要配置的字段

系统基于 `AbilityTags` 做匹配，GA 只需正确填写自己的身份 Tag：

```
GA_Attack   → Class Defaults → Ability Tags: Action.Attack
GA_Dead     → Class Defaults → Ability Tags: Action.Dead
              ActivationOwnedTags: Buff.Status.Dead
GA_HitReact → Class Defaults → Ability Tags: Action.HitReact
              ActivationOwnedTags: Buff.Status.HitReact
```

**不需要**在 GA 里配置 `ActivationBlockedTags` 或 `BlockAbilitiesWithTag`。

---

## 文件清单

| 文件 | 类型 | 说明 |
|------|------|------|
| `Source/DevKit/Public/Data/StateConflictDataAsset.h` | C++ Header | FStateConflictRule + UStateConflictDataAsset 定义（含 BlockCategoryMap） |
| `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h` | C++ Header | ConflictTable 属性 + OnTagUpdated 声明 |
| `Source/DevKit/Private/AbilitySystem/YogAbilitySystemComponent.cpp` | C++ | InitConflictTable / SetConflictTable / OnTagUpdated 实现 |
| `Source/DevKit/Public/DevAssetManager.h` | C++ Header | StateConflictData Config 属性 + GetStateConflictData() |
| `Source/DevKit/Private/DevAssetManager.cpp` | C++ | GetStateConflictData() 实现 |
| `Config/DefaultGame.ini` | INI | StateConflictData 全局路径配置 |
| `Content/Docs/GlobalSet/CharacterBaseSet/DA_Base_StateConflict_Initial.uasset` | DataAsset | 策划填写的规则表 |

---

## 扩展计划

| 功能 | 状态 | 说明 |
|------|------|------|
| 基础 Block / Cancel（GA级） | ✅ 已实现 | |
| Tag 移除自动解除 Block | ✅ 已实现 | |
| 移动组件阻断（Block.Movement） | ✅ 已实现 | BlockCategoryMap 驱动 |
| AI 行为树暂停（Block.AI） | ✅ 已实现 | BlockCategoryMap 驱动 |
| 全局 DA 自动加载 | ✅ 已实现 | DevAssetManager Config |
| Priority 优先级打断 | 🔲 预留字段 | 需要在 OnTagUpdated 中比较当前活跃规则优先级 |
| 每角色差异化规则 | 🔲 可选 | SetConflictTable() 接口已预留 |
