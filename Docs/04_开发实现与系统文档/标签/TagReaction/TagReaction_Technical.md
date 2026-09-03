# 标签反应系统 — 技术文档

Tag 出现 / 消失时自动触发 BuffFlow、GameplayEffect 或 GameplayAbility 的数据驱动系统。

---

## 与状态冲突系统的关系

两者是**兄弟系统**，不是替代关系。同样以「Tag 是否存在于 ASC 上」为键，同样从 `OnTagUpdated` 派发，但语义相反：

| | `UStateConflictDataAsset`（已有） | `UTagReactionDataAsset`（新增） |
|---|---|---|
| 语义 | 抑制 — Tag 存在期间**禁止**什么 | 追加 — Tag 变化时**触发**什么 |
| 配置 | `Rules`（阻断 / 取消 GA）、`BlockCategoryMap`（`Block.Movement`、`Block.AI`） | 启动 Flow / 施加 GE / 激活 GA |
| 移除时 | 多 Tag 仲裁：重新扫描是否**还有其他**阻断 Tag 存在，再决定是否解除 | 按缓存句柄成对回收 |
| 运行时状态 | 无状态 | 有状态 — 缓存 Flow GUID、GE 句柄、GA 句柄 |

**为什么不合并**：状态冲突的移除逻辑做的是多 Tag 仲裁（`YogAbilitySystemComponent.cpp:492-514`、`:536-548`）。若用简单的「加了就做、removed 就撤」表达移动阻断，当 `Buff.Status.HitReact` 消失而 `Buff.Status.Knockback` 仍在时会错误地恢复移动。这段仲裁不是配置能替代的逻辑。

---

## 架构总览

```
DefaultGame.ini → DevAssetManager → UTagReactionDataAsset（全局表）
                                              │
                        ASC.TagReactionTable（角色级覆盖，可选）
                                              │
                                   InitTagReactionTable()
                                              │
                        TMap<Tag, TArray<FTagReactionRule>>
                              ReactionMap（O(1) 查找，按 Priority 降序）
                                              │
                                      OnTagUpdated()
                                              │
        ┌─────────────────────────────────────┼─────────────────────────────┐
        │                                     │                             │
  ProcessStateConflict()            ProcessTagReactions()          OnGameplayTagChanged
   （先执行，阻断优先）                        │                    .Broadcast()（蓝图）
                                              │
                       ┌──────────────────────┼──────────────────────┐
                       │                      │                      │
                 StartBuffFlow          ApplyGameplayEffect     ActivateAbility
                       │                      │                      │
                  FlowGuid              EffectHandle           AbilityHandle
                       └──────────── ActiveTagReactions ─────────────┘
                                     （Tag 移除时按此回收）
```

---

## 派发顺序

`OnTagUpdated` 内的执行顺序是**有意设计**的，不可调换：

```cpp
void UYogAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
    Super::OnTagUpdated(Tag, TagExists);

    HandleStatusNiagaraTag(Tag, TagExists);   // 状态 Niagara
    /* BlockCategoryMap 阶段（移动 / AI 阻断） */
    /* Buff.SuperArmor 硬编码分支 */

    ProcessStateConflict(Tag, TagExists);     // 冲突阻断先行
    ProcessTagReactions(Tag, TagExists);      // 反应后行
    OnGameplayTagChanged.Broadcast(Tag, TagExists);
}
```

**阻断必须先于反应**，否则反应可能激活一个同一 Tag 正要阻断的 GA。

> 重构提示：原本冲突逻辑直接写在 `OnTagUpdated` 尾部，且以两个 `return` 结束（`bProcessingConflict` 与 `!Rule`）。这两个提前返回会吞掉后续所有阶段，因此已抽出为 `ProcessStateConflict()`，逻辑本身**未做任何改动**。

---

## 数据结构

`Source/DevKit/Public/Data/TagReactionDataAsset.h`

| 字段 | 类型 | 说明 |
|------|------|------|
| `TriggerTag` | `FGameplayTag` | 计数 0 → 非零时触发，非零 → 0 时回收 |
| `ReactionType` | `ETagReactionType` | `StartBuffFlow` / `ApplyGameplayEffect` / `ActivateAbility` |
| `UndoPolicy` | `ETagReactionUndo` | `Auto` 自动回收；`Persist` 不回收 |
| `FlowAsset` | `TObjectPtr<UFlowAsset>` | 仅 `StartBuffFlow` 可见 |
| `EffectClass` | `TSubclassOf<UYogGameplayEffect>` | 仅 `ApplyGameplayEffect` 可见 |
| `AbilityClass` | `TSubclassOf<UGameplayAbility>` | 仅 `ActivateAbility` 可见 |
| `Priority` | `int32` | 同一 Tag 多条规则时的执行顺序，数值越高越先执行 |

三个资产字段用 `EditCondition + EditConditionHides` 控制，编辑器中只显示与所选 `ReactionType` 匹配的那一个。

---

## 生命周期

```
Tag 计数 0 → 1
    ProcessTagReactions(Tag, true)
        ReactionMap.Find(Tag) → 命中则按 Priority 依次 ApplyTagReaction
        成功的记录写入 ActiveTagReactions[Tag]

Tag 计数 1 → 0
    ProcessTagReactions(Tag, false)
        UndoTagReactions(Tag)
            RemoveAndCopyValue 取出记录
            UndoPolicy == Auto 时：StopBuffFlow / RemoveActiveGameplayEffect / CancelAbilityHandle
```

每条规则用 `FGuid::NewGuid()` 生成独立 Flow GUID，因此同一 Tag 上的多条规则不会互相覆盖。

### 回收兜底

`OnUnregister()` 调用 `ClearAllTagReactions()`，保证角色死亡或关卡切换时不残留 Flow 和无限时长 GE。

---

## 初始化

### 全局自动加载

```ini
[/Script/DevKit.DevAssetManager]
TagReactionData=/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_TagReaction_Initial.DA_Base_TagReaction_Initial
```

`InitTagReactionTable()` 在 `AYogCharacterBase::PostInitializeComponents()` 中紧跟 `InitConflictTable()` 调用。

### 角色级覆盖

在 ASC 的 Details 面板设置 `TagReaction → TagReactionTable`，或运行时调用 `SetTagReactionTable()`。

合并规则：先加载全局表，再叠加角色表；角色表中 `TriggerTag` **且** `ReactionType` 都相同的条目**替换**全局条目，其余追加。

---

## 蓝图接入

不配表也能用的逃生口：

```cpp
UPROPERTY(BlueprintAssignable, Category = "TagReaction")
FGameplayTagChangedDelegate OnGameplayTagChanged;   // (FGameplayTag Tag, bool bAdded)
```

任意 Tag 跨越 0 边界时广播，在派发阶段之后触发。

---

## 注意事项

| 问题 | 说明 |
|------|------|
| Instant GE 无法 Auto 回收 | `ApplyGameplayEffectSpecToSelf` 对 Instant GE 不返回有效句柄，`Auto` 回收无从谈起。需要回收就用 Duration / Infinite GE，否则会打 Warning 并判定该条规则失败 |
| 阻断优先于反应 | 同一 Tag 同时配在冲突表和反应表中是合法的，但阻断先执行并生效。`InitTagReactionTable()` 检测到重叠时会打 Warning |
| 递归保护 | 施加 GE 或启动 Flow 会授予新 Tag，从而重入 `OnTagUpdated`。用 `bProcessingTagReaction` guard 打断，与 `bProcessingConflict` 独立 |
| Avatar 尚未就绪 | `OnTagUpdated` 可能在 `InitAbilityActorInfo` 期间触发，此时 Avatar 和 `UBuffFlowComponent` 都还不存在。`ApplyTagReaction` 会提前返回并打 Warning，不会崩溃 |
| 重复触发保护 | 若某 Tag 已有活跃反应却再次触发（未经历移除），会打 Warning 并跳过，不会叠加第二份 |

---

## 与已有 Tag 监听方式的关系

| 方式 | 适用场景 |
|------|----------|
| `UTagReactionDataAsset` | 策划配表即可完成的 Tag → 反应，无需改 C++ |
| `BFNode_OnTagAdded` / `BFNode_OnTagRemoved` | 已在运行的 BuffFlow 图内监听 Tag。注意：这两个节点**不能启动**新 Flow，只能在 Flow 已启动后监听 |
| `OnGameplayTagChanged` 委托 | 蓝图一次性接入，不值得配表的场合 |
| 直接 `RegisterGameplayTagEvent` | 需要复杂状态机的 GA 内部逻辑（如 `GA_Wound`、`GA_Bleed`） |

`ERuneTriggerType`（`RuneDataAsset.h:125`）仍然只有 `Passive / OnAttackHit / OnDash / OnKill / OnCritHit / OnDamageReceived`，**没有** Tag 触发项。需要「Tag 启动符文 Flow」时走本系统的 `StartBuffFlow`。

---

## 文件清单

| 文件 | 类型 | 说明 |
|------|------|------|
| `Source/DevKit/Public/Data/TagReactionDataAsset.h` | C++ Header | 枚举 + `FTagReactionRule` + `UTagReactionDataAsset` |
| `Source/DevKit/Public/AbilitySystem/YogAbilitySystemComponent.h` | C++ Header | `TagReactionTable`、`OnGameplayTagChanged`、`ReactionMap`、`FActiveTagReaction` |
| `Source/DevKit/Private/AbilitySystem/YogAbilitySystemComponent.cpp` | C++ | `InitTagReactionTable` / `ProcessTagReactions` / `ApplyTagReaction` / `UndoTagReactions` / `ClearAllTagReactions` / `OnUnregister` |
| `Source/DevKit/Private/Character/YogCharacterBase.cpp` | C++ | `PostInitializeComponents` 中的初始化调用 |
| `Source/DevKit/Public/DevAssetManager.h` | C++ Header | `TagReactionData` Config 属性 + `GetTagReactionData()` |
| `Source/DevKit/Private/DevAssetManager.cpp` | C++ | `GetTagReactionData()` 实现 |
| `Config/DefaultGame.ini` | INI | `TagReactionData` 全局路径配置 |
| `Content/Docs/GlobalSet/CharacterBaseSet/DA_Base_TagReaction_Initial.uasset` | DataAsset | 策划填写的反应表（当前为空） |

---

## 验证记录

2026-09-03 PIE 实测：

- 全局表经 ini 正确解析，所有角色（玩家 + 各类敌人）打出 `[TagReaction] Initialized 1 rules across 1 tags`。
- 临时规则 `Buff.HitReact → ApplyGameplayEffect`，玩家连续受击 11 次，每次都打出 `applied effect`。
- 全程无 `already has active reactions` Warning。该 Warning 只在 `ActiveTagReactions` 仍持有该 Tag 记录时触发，因此其缺席反证每次 `UndoTagReactions` 都正确回收 —— apply 与 undo 双向成立。
- `ProcessStateConflict` 抽取未造成回归：`[StateConflict] Initialized 4 rules, 2 block categories` 照常，阻断分支照常进入。

验证后已清空临时规则，表以空状态交付。

---

## 扩展计划

| 功能 | 状态 | 说明 |
|------|------|------|
| StartBuffFlow / ApplyGE / ActivateGA 三类反应 | ✅ 已实现 | |
| Auto / Persist 回收策略 | ✅ 已实现 | |
| 全局表 + 角色级覆盖 | ✅ 已实现 | |
| `OnGameplayTagChanged` 蓝图委托 | ✅ 已实现 | |
| `OnUnregister` 回收兜底 | ✅ 已实现 | |
| Priority 排序 | ✅ 已实现 | 仅决定执行顺序，暂不做互斥抢占 |
| 反应条件（如仅玩家 / 仅敌人 / 血量阈值） | 🔲 未做 | 目前只能靠 Tag 本身区分 |
| 吸收 StateConflict 为 `Block` 反应类型 | 🔲 未定 | 需要把多 Tag 仲裁逻辑一并搬过来，不是纯配置迁移 |
