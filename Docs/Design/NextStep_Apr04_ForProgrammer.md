# 后续工作方案 · 程序版
**日期：** 2026-04-04  
**前置：** BuffFlow系统已完成，BackpackGridComponent已完成，关卡流程已完成

---

## 一、今日代码冲突修复记录

| 文件 | 问题 | 修复 |
|------|------|------|
| `DamageExecution.cpp:82` | `TargetDmgTaken` capture 失败时为 0，导致全场 0 伤害 | 加 `FMath::Max(TargetDmgTaken, 1.f)` |
| `BaseAttributeSet.cpp:231` | `GetBackpackGridComponent()` 无 null 检查，异常构造序时崩溃 | 改为 if 保护 |
| `DamageExecution.h:4&11` | `GameplayEffectExecutionCalculation.h` 被 include 两次 | 删除重复行 |

**已确认不需要修改（分析结论）：**
- `PostGameplayEffectExecute` 内 Apply GE（战斗渴望/全能）：GAS 内部使用 pending queue 延迟执行，不会递归，单机无风险
- `BuffFlowComponent::StartRootFlow` 存模板指针：Flow 插件以模板为 key 管理实例，`FinishRootFlow` 传模板是正确的
- `BFNode_Base::TryFindActorOwner`：`UBuffFlowComponent` 是 `UActorComponent`，`TryFindActorOwner` 能正确溯到宿主 Actor
- `DamageExecution::Execute_Implementation` 内 `RemoveGameplayTagWithCount`：单机安全，联机再处理

---

## 二、P0 — 必须做（4月15日 Demo 前）

### 2.1 GameplayTag 注册
**问题：** BuffFlow 节点和 DamageExecution 里用了很多硬编码 Tag（`Rune.ZhanDouKewang.Active`、`State.DoubleHit` 等），但 DefaultGameplayTags.ini 里可能没有注册。

**任务：**
1. 打开 UE 编辑器 → Project Settings → GameplayTags，确认以下 Tag 存在（或在 `Config/Tags/` 下的 .ini 里添加）：

```
Rune.ZhanDouKewang.Active
Rune.QuanNeng.Active
Rune.FenLiYiJi.Active
Rune.TuXi.Active
State.DoubleHit
State.Debuff.Stun
State.Debuff.Freeze
Action.Combo.LastHit
GameEvent.Combat.Attack.HitEnemy
GameEvent.Combat.Attack.HitEnemy.Crit
GameEvent.Combat.Kill
GameEvent.Combat.Damaged.ByEnemy
GameEvent.Combat.Dodge.Performed
GameEvent.Life.Death.Self
GameEvent.Life.Death.EnemyNearby
GameEvent.Combat.Attack.Begin
Data.ZhanDouKewang.AttackSpeedBonus
Data.QuanNeng.CritRateBonus
SetByCaller.Damage.Physical
SetByCaller.Damage.Poison
SetByCaller.Heal
```

2. 检查是否已在对应 .ini 文件中（`Config/Tags/Buff.ini`、`PlayerGameplayTag.ini`、`YogEffect.ini` 等），没有的补上。

---

### 2.2 WBP_LootSelection UI 蓝图联通

| 步骤 | 操作 |
|------|------|
| 1 | 打开 `WBP_LootSelection`，在 Event Graph 绑定 `AYogGameMode::OnLootGenerated` 委托 |
| 2 | 委托回调 `FOnLootGenerated(TArray<FLootOption>)` → 循环填充三个按钮 |
| 3 | 每个按钮 `OnClicked` → 调用 `GameMode->SelectLoot(0/1/2)` |
| 4 | 添加"确认整理"按钮 → `GameMode->ConfirmArrangementAndTransition()` |
| 5 | 在 `LootSelectionWidget.cpp` 的 `NativeConstruct()` 中绑定 GameMode 委托 |

**`LootSelectionWidget.h/.cpp` 已有骨架，需要填充实现：**
- `ULootSelectionWidget::BindToGameMode(AYogGameMode*)` — 绑定 `OnLootGenerated`
- `ULootSelectionWidget::OnLootOptionsReceived(const TArray<FLootOption>&)` — 更新 UI

---

### 2.3 符文激活 Tag 广播（让 DamageExecution 里的 Rune Tag 生效）

背包激活符文时，除施加 GE 外，还需给 ASC 打上对应 Tag（例如 `Rune.ZhanDouKewang.Active`），卸下时移除。

**实现位置：** `BackpackGridComponent::ActivateRune` / `DeactivateRune`

```cpp
// ActivateRune 末尾加：
if (Placed.bIsActivated && Placed.Rune.ActiveTag.IsValid())
{
    ASC->AddLooseGameplayTag(Placed.Rune.ActiveTag);
}

// DeactivateRune 末尾加：
if (Placed.Rune.ActiveTag.IsValid())
{
    if (UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(CachedASC.Get()))
    {
        ASC->RemoveLooseGameplayTag(Placed.Rune.ActiveTag);
    }
}
```

**需要在 `FRuneInstance` 中添加字段：**
```cpp
// RuneDataAsset.h 的 FRuneInstance 中：
UPROPERTY(BlueprintReadOnly)
FGameplayTag ActiveTag;  // 符文激活时打在 ASC 上的 Tag（策划填 DA_Rune 时配置）
```

---

### 2.4 死亡事件广播（H类符文前置）

在 `YogCharacterBase` 或 `EnemyCharacterBase::Die()` 中广播死亡事件：

```cpp
void AYogCharacterBase::Die()
{
    // ... 原有死亡逻辑 ...
    FGameplayEventData EventData;
    EventData.Instigator = this;
    EventData.Target = this;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        this,
        FGameplayTag::RequestGameplayTag("GameEvent.Life.Death.Self"),
        EventData
    );
}
```

---

### 2.5 MoveSpeedMultiplier / AttackSpeedMultiplier 属性扩展（D类符文前置）

`BaseAttributeSet` 中目前有 `AttackSpeed` 但没有 `MoveSpeedMultiplier`。减速符文需要倍率字段。

在 `BaseAttributeSet.h` 中添加（如还没有）：
```cpp
UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeedMultiplier, Category = "Base")
FGameplayAttributeData MoveSpeedMultiplier;
ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeedMultiplier)
```

并在角色移动更新逻辑中乘以该倍率。

---

## 三、P1 — 符文效果类型实现

按优先级（B → D → C/H → E/F → G）：

### 3.1 B类：事件广播（已有 BuffFlow，但需验证事件 Tag 正确广播）

DamageExecution.cpp 已有：
```cpp
TargetASC->ReceiveDamage(SourceASC, FinalDamage);   // → BFNode_OnDamageReceived 监听
SourceASC->OnCritHit.Broadcast(...)                  // → BFNode_OnCritHit 监听
```

需要确认 `YogAbilitySystemComponent` 中的委托定义和 `BFNode_On*` 节点的订阅逻辑匹配。

### 3.2 D类：CC 状态响应（眩晕/冻结禁用输入）

在 `PlayerCharacterBase::BeginPlay` 中注册：
```cpp
GetAbilitySystemComponent()->RegisterGameplayTagEvent(
    FGameplayTag::RequestGameplayTag("State.Debuff.Stun"),
    EGameplayTagEventType::NewOrRemoved
).AddUObject(this, &APlayerCharacterBase::OnStunTagChanged);
```

### 3.3 C/H类：生成物 Actor 基类
- `ARuneSpawnedActorBase`：`InitWithLevel(int32, AActor*, UAbilitySystemComponent*)` 接口
- 第一个具体实现：`BP_RotPoisonPool`（腐烂毒池）

---

## 四、P2 — 推迟到完整版

| 功能 | 原因 |
|------|------|
| G类攻击行为修改（穿透/弹射/分裂） | 需要 AttackParams Hook 系统，攻击逻辑较复杂 |
| `PercentKilled_20/50` 触发条件 | Demo 用 AllEnemiesDead 够 |
| 联机下 `Execute_Implementation` 内改 Tag 的问题 | 单机项目暂不涉及 |
| E/F类符文（条件触发/光环） | 优先级低于主流程 |

---

## 五、注意事项

1. **BuffFlow 的 `CurrentBuffGiver`** 是全局覆盖，未来若有 Giver 不是 Owner 自身的符文，需改为 per-flow 存储
2. **`YogBuffDefinition::CreateTransientGE`** 的命名在 PIE 多次运行时会产生后缀，属 GC 压力问题，正式版前修复
3. **编译后必须验证** `UBuffFlowComponent` 在 PIE 中能正确 Start/Stop Flow，建议通过 `DA_FlowTest_Rune_Log` 测试资产验证
