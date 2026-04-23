# FA 通用架构：全局符文/能力系统

> 项目：星骸降临
> 文档性质：系统架构设计规范
> 创建日期：2026-04-24
> 最后更新：2026-04-24（TriggerType 系统实现、Ability.Event.* 全量广播、节点引脚扩展）
> 关联文档：[BuffFlow_ArchitectureAndPlan.md](BuffFlow_ArchitectureAndPlan.md) · [BuffFlow_ProgrammerGuide.md](BuffFlow_ProgrammerGuide.md)

---

## 一、问题背景

### 1.1 原有设计的局限

最初的 FA（FlowAsset）体系以"玩家背包"为入口：

```
背包激活区 → 启动 FA → FA 监听战斗事件 → 触发 GE/GA
```

这导致三个问题：

1. **敌人无法使用同一套 FA**：敌人没有背包，所有 BuffFlow 节点都依赖背包激活路径
2. **动作作用域 FA 无终止机制**：特殊攻击 GA 启动 FA 后，没有标准方式在 GA 结束时停止它
3. **触发节点依赖背包专属事件**：FA 内部的触发节点如果只通过 BFC 的背包委托监听，敌人的 BFC 永远收不到相同信号

### 1.2 核心洞察

问题不在于 `BuffFlowComponent`（BFC），它本身已是通用的：

```cpp
// 任意 Actor 上的 BFC 都可以调用
BFC->StartBuffFlow(FlowAsset, Guid, GiverActor);
BFC->StopBuffFlow(Guid);
```

问题在于：
- **启动入口只有背包**，没有其他规范路径
- **触发信号不通用**，缺少全 Actor 共享的事件广播标准

---

## 二、架构总览：三层模型

```
┌──────────── 授予层（Grant Layer）─────────────────────────┐
│ 决定「谁」以「什么条件」获得一个 FA 能力                    │
│                                                            │
│  A. 玩家背包激活区   → BackpackGridComponent               │
│  B. 敌人 Spawn       → YogGameMode::SpawnEnemyFromPool     │
│  C. 关卡 Buff 注入   → YogGameMode::StartLevelSpawning     │
│  D. GA 动作作用域    → GA::ActivateAbility                 │
│  E. 献祭恩赐         → PlayerCharacterBase::AcquireSacrificeGrace │
│                                                            │
│       全部调用 → BuffFlowComponent::StartBuffFlow()        │
└───────────────────────────────┬────────────────────────────┘
                                │ StartBuffFlow(FA, Guid, Giver)
┌───────────────────────────────▼────────────────────────────┐
│                   执行层（FA Execution Layer）               │
│ FA 不感知来源，只做：监听事件 → 施加效果 → 循环/终止        │
│                                                            │
│  BFNode_OnDamageDealt / OnKill / OnDash / OnDamageReceived │
│  BFNode_ApplyAttributeModifier / ApplyEffect / GrantGA     │
│                                                            │
│  触发信号来源：UAbilitySystemBlueprintLibrary::             │
│               SendGameplayEventToActor (Ability.Event.*)   │
└───────────────────────────────┬────────────────────────────┘
                                │ StopBuffFlow(Guid)
┌───────────────────────────────▼────────────────────────────┐
│                   终止层（Revoke Layer）                     │
│ 决定「何时」停止 FA                                         │
│                                                            │
│  A. 玩家符文离开激活区 → BackpackGridComponent              │
│  B. 敌人死亡           → BuffFlowComponent 随 Actor 销毁   │
│  C. GA 结束            → GA::EndAbility 显式调用           │
└────────────────────────────────────────────────────────────┘
```

**核心原则：FA 内部零感知授予与终止来源。** 一个"流血"FA 不管是玩家符文还是敌人固有能力给的，节点图完全相同。

---

## 三、FA 三类生命周期

### Category A：持续型（Persistent）

FA 跟随"拥有权"存续，长期运行并等待战斗事件触发效果。

| 阶段 | 条件 | 调用方 |
|------|------|--------|
| 授予 | 符文进入激活区 / 拾取献祭恩赐 | `BackpackGridComponent::RefreshAllActivations` / `AcquireSacrificeGrace` |
| 运行 | 持续监听，无主动超时 | FA 内部节点 |
| 终止 | 符文离开激活区 / 角色死亡 | `BackpackGridComponent` / Actor 销毁 |

典型模式：
```
[Start] → [BFNode_OnDamageDealt] → [BFNode_ApplyEffect: GE_Bleed] → ↩
```

---

### Category B：实体能力型（Actor Ability）

FA 与实体（敌人/Boss）共生。是这个 Actor 在整个生存期内的"固有能力"。

| 阶段 | 条件 | 调用方 |
|------|------|--------|
| 授予 | 敌人 Spawn | `YogGameMode::SpawnEnemyFromPool`（**已实现**） |
| 运行 | 跟随实体存活期 | FA 内部节点 |
| 终止 | 敌人死亡 | `BuffFlowComponent` 随 Actor 销毁自动 Cleanup |

当前实现（`YogGameMode.cpp` 行 ~1220）：
```cpp
// 关卡 Buff（所有敌人共享）
for (const FBuffEntry& Entry : ActiveRoomBuffs)
{
    if (Entry.RuneDA && Entry.RuneDA->RuneInfo.Flow.FlowAsset)
        BFC->StartBuffFlow(Entry.RuneDA->RuneInfo.Flow.FlowAsset,
                           FGuid::NewGuid(), SpawnedEnemy);
}

// 敌人专属 Buff（BuildWavePlan 时从 EnemyBuffPool 选取）
if (Planned.SelectedEnemyBuff && Planned.SelectedEnemyBuff->RuneInfo.Flow.FlowAsset)
    BFC->StartBuffFlow(Planned.SelectedEnemyBuff->RuneInfo.Flow.FlowAsset,
                       FGuid::NewGuid(), SpawnedEnemy);
```

> **注意**：`SpawnEnemyFromPool` 已经对两种来源都调用了 `StartBuffFlow`，Category B 在 C++ 层已完整实现。
> 需要补充的是 FA 内部的**触发标签广播**（见第四节）。

---

### Category C：动作作用域型（GA Scoped）

FA 的生存期完全绑定单次 GA 执行。GA 启动时授予，GA 结束时撤销。

| 阶段 | 条件 | 调用方 |
|------|------|--------|
| 授予 | `GA::ActivateAbility` | GA C++ / Blueprint |
| 运行 | GA 存活期间 | FA 内部节点 |
| 终止 | `GA::EndAbility` | GA 显式调用 `StopBuffFlow` |

**标准模板（C++ GA）：**

```cpp
// GA 头文件
private:
    FGuid ActionFAGuid;

// ActivateAbility()
void UGA_SpecialAttack::ActivateAbility(...)
{
    // ... 其他初始化 ...
    if (UFlowAsset* ActionFA = GetActionFlowAsset())
    {
        ActionFAGuid = FGuid::NewGuid();
        GetBuffFlowComp()->StartBuffFlow(ActionFA, ActionFAGuid, GetAvatarActorFromActorInfo());
    }
}

// EndAbility()
void UGA_SpecialAttack::EndAbility(...)
{
    GetBuffFlowComp()->StopBuffFlow(ActionFAGuid);
    Super::EndAbility(...);
}
```

> 同一 GA 可能同时运行多个 Category C FA（比如主 FA + VFX FA），各自持有不同的 Guid，互不干扰。

---

## 四、触发系统标准化

### 4.1 问题

原有 BFNode 触发节点（`BFNode_OnDamageDealt` 等）通过 **BFC 的内部委托**接收事件。这套委托需要调用方显式通知 BFC，目前只有玩家伤害流水线规范地通知了它。

敌人拥有 FA 后，如果攻击事件没有通知到敌人的 BFC，FA 内的触发节点永远不会触发。

### 4.2 方案：统一 Gameplay Event 广播

所有攻击/冲刺 GA 在关键时机通过 **Gameplay Event Tag** 广播事件。BFNode 内部统一用 `WaitGameplayEvent` 监听这些 Tag。

**标准事件 Tag 表（已在 `Config/DefaultGameplayTags.ini` 注册，所有广播已实装）：**

| Tag | 含义 | 广播时机 | 广播者 | 状态 |
|-----|------|---------|--------|------|
| `Ability.Event.Attack.Hit` | 攻击命中目标 | 近战命中判定后（GE 命中目标时） | `GA_MeleeAttack::OnEventReceived` | ✅ |
| `Ability.Event.Attack.Miss` | 攻击未命中 | 判定为未命中时 | 武器 GA | ⏳ 待实现 |
| `Ability.Event.Kill` | 击杀目标 | 目标 HP 归零时 | `DamageAttributeSet::PostGameplayEffectExecute` | ✅ |
| `Ability.Event.Death` | 自身死亡 | HP 归零后广播给死亡者 | `DamageAttributeSet::PostGameplayEffectExecute` | ✅ |
| `Ability.Event.Damaged` | 自身受到伤害 | 受击处理时 | `YogCharacterBase::OnDamaged` | ⏳ 待实现 |
| `Ability.Event.Dash` | 执行冲刺 | 冲刺位移开始时（Task->ReadyForActivation 后） | `GA_PlayerDash::ActivateAbility` | ✅ |

**Payload 约定：**

| Tag | `Payload.Instigator` | `Payload.Target` |
|-----|---------------------|-----------------|
| Attack.Hit | 攻击者（玩家/敌人） | 被命中目标 |
| Kill | 击杀者 | 死亡目标 |
| Death | 击杀者 | 死亡者自身 |
| Dash | 冲刺者 | — |

**广播方式（GA 端）：**

```cpp
void UGA_MeleeAttack::OnHit(AActor* HitTarget)
{
    FGameplayEventData Payload;
    Payload.Instigator = GetAvatarActorFromActorInfo();
    Payload.Target     = HitTarget;

    // 广播给攻击者（触发攻击者身上的 FA）
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        Payload.Instigator,
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit")),
        Payload);

    // 如果需要触发目标身上的 FA（例如目标有"被命中时反弹"符文）
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        HitTarget,
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged")),
        Payload);
}
```

**FA 端监听（BFNode_OnDamageDealt 内部，或直接用 BFNode_WaitGameplayEvent）：**

```cpp
// BFNode 内部
void UBFNode_OnDamageDealt::ExecuteInput(const FName& PinName)
{
    // 向 ASC 注册 WaitGameplayEvent 监听
    // 当 Ability.Event.Attack.Hit 到来时，读取 Payload.Target 填入 LastEventContext
    ASC->GenericGameplayEventCallbacks.FindOrAdd(HitTag)
        .AddUObject(this, &UBFNode_OnDamageDealt::OnEventReceived);
}
```

> **关键**：同一套 BFNode 代码，玩家和敌人的 FA 无需任何修改。只要攻击 GA 广播了事件，拥有对应 FA 的任何 Actor 都会响应。

---

## 五、玩家背包的正确定位

玩家背包不是"FA 的触发器"，是"**能力的授予条件（Grant Condition）**"。

```
玩家获得符文 DA
    │
    ├── 放入背包格子
    │       │
    │       ├── 进入激活区 → StartBuffFlow(FA, Guid, Player)   ← 能力授予
    │       │
    │       └── 离开激活区 → StopBuffFlow(Guid)                ← 能力撤销
    │
    └── [激活区外] → FA 未运行，符文无效果
```

**背包比敌人的授予条件更严格**（需要正确放置 + 热度足够），但 FA 执行层完全相同。

玩家可以把同一个 `FA_Rune_Bleed` 放入背包，敌人 Spawn 时也可以获得同一个 `FA_Rune_Bleed`。两者共用同一个 FA 资产，节点图不需要任何区分。

---

## 六、各授予源实现状态

| 授予源 | C++ 状态 | 涉及文件 |
|--------|---------|---------|
| 玩家背包激活区（Passive） | ✅ 完整实现 | `BackpackGridComponent::ActivateRune` |
| 玩家背包激活区（TriggerType 事件驱动） | ✅ 完整实现 | `BackpackGridComponent::ActivateRune` → 注册 ASC 事件监听器 |
| 敌人 Spawn（关卡 Buff） | ✅ 完整实现 | `YogGameMode::SpawnEnemyFromPool` 行 ~1220 |
| 敌人 Spawn（专属 Buff） | ✅ 完整实现 | `YogGameMode::SpawnEnemyFromPool` 行 ~1228 |
| GA 动作作用域 | ⚠️ 有标准模板，各 GA 按需接入 | 见第三节 Category C 模板 |
| 献祭恩赐 | ✅ 完整实现 | `PlayerCharacterBase::AcquireSacrificeGrace` |

---

## 七、待完成工作

### ✅ 已完成（原 P0）：攻击 GA Gameplay Event 广播

| GA | 广播 Tag | 实现位置 |
|----|---------|---------|
| `GA_MeleeAttack` | `Ability.Event.Attack.Hit` | `OnEventReceived`，对每个命中目标广播 |
| `GA_PlayerDash` | `Ability.Event.Dash` | `ActivateAbility`，Task->ReadyForActivation 后 |
| 伤害系统 | `Ability.Event.Kill` / `Ability.Event.Death` | `DamageAttributeSet::PostGameplayEffectExecute` |

### ✅ 已完成（原 P0）：TriggerType 激活系统

见第十二节。`BackpackGridComponent::ActivateRune` 现在读取 `DA.RuneInfo.RuneConfig.TriggerType`，对非 Passive 符文注册 ASC 事件监听器而非立即启动 FA。

### P1：验证敌人死亡时 FA 自动 Cleanup

确认 `AEnemyCharacterBase::Die()` 路径最终触发 Actor 销毁，从而触发 `BuffFlowComponent` 的 `EndPlay` → `StopAllBuffFlows()`。

涉及文件：`Source/DevKit/Private/Character/EnemyCharacterBase.cpp`

### P1：Attack.Miss / Damaged 广播补充

`Ability.Event.Attack.Miss` 和 `Ability.Event.Damaged` 尚未广播，如果有符文使用这两个 TriggerType 时再实现。

### P2（可选）：EnemyCharacterBase::GrantRuneAbility 封装

当前 `SpawnEnemyFromPool` 直接操作 `BFC->StartBuffFlow`，功能完整。
如需在蓝图中手动给敌人授予能力（如事件触发赋能），可封装为：

```cpp
UFUNCTION(BlueprintCallable, Category = "Ability")
void GrantRuneAbility(URuneDataAsset* RuneDA, FGuid& OutAbilityID);

UFUNCTION(BlueprintCallable, Category = "Ability")
void RevokeRuneAbility(FGuid AbilityID);
```

涉及文件：`Source/DevKit/Public/Character/EnemyCharacterBase.h`

---

## 八、链路系统补充说明

链路系统是玩家背包专属的授予条件扩展，不影响 Category B / C。

```
激活区（直接区域）
  + BFS 传导区（Producer 符文向指定方向传出）
  = 有效激活区

Consumer 符文只能放在有效激活区之外
Producer 在有效激活区内时，向其 ChainDirections 指定方向的相邻符文也会被激活
```

相关文件：
- 枚举定义：`Source/DevKit/Public/Data/RuneDataAsset.h`（`ERuneChainRole` / `EChainDirection`）
- 算法实现：`Source/DevKit/Public/Component/BackpackGridComponent.h`（`ComputeChainActivatedCells`）

---

## 九、献祭恩赐补充说明

献祭恩赐是一个 Category A FA，但授予源是"拾取物"而非背包：

```
EnterArrangementPhase → 15% 概率 → SpawnSacrificePickup
玩家拾取 → BP_SacrificeGracePickup 弹出接受/拒绝弹窗
接受 → PlayerCharacterBase::AcquireSacrificeGrace(DA)
      ├── 热度设置为 MaxHeat
      ├── 应用 BonusEffect GE
      └── StartBuffFlow(DA->FlowAsset, NewGuid, Player)
            └── BFNode_SacrificeDecay 开始计时衰减热度
```

每次进入新关卡时（`BeginPlay`），若 `ActiveSacrificeGrace` 非空，自动重新调用 `AcquireSacrificeGrace` 重置衰减速率并充满热度。

相关文件：
- DA 定义：`Source/DevKit/Public/Data/SacrificeGraceDA.h`
- 衰减节点：`Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h`
- 玩家接入：`Source/DevKit/Private/Character/PlayerCharacterBase.cpp`
- 掉落逻辑：`Source/DevKit/Private/GameModes/YogGameMode.cpp`（`EnterArrangementPhase`）

---

## 十、快速决策树

```
需要给某个角色/动作赋予 FA 效果？

├── 是「玩家符文」吗？
│     是 → 在 RuneDataAsset 填写 FA，放入背包激活区即可（Category A）
│
├── 是「敌人固有能力」吗？
│     是 → 在 EnemyData.EnemyBuffPool 填写 RuneDA（Category B，已实现）
│
├── 是「关卡给所有敌人的 Buff」吗？
│     是 → 在 RoomDataAsset.BuffPool 填写 RuneDA（Category B，已实现）
│
├── 是「某个特殊攻击动作的临时效果」吗？
│     是 → 在 GA::ActivateAbility 调用 StartBuffFlow，
│          在 GA::EndAbility 调用 StopBuffFlow（Category C）
│
└── 是「全局运行的 Run Buff」吗？
      是 → 新建 DA + FA，在合适时机调用 StartBuffFlow（Category A 变体）
```

---

## 十一、关键接口速查

```cpp
// 启动 FA（通用入口）
BuffFlowComponent->StartBuffFlow(UFlowAsset* FA, FGuid Guid, AActor* Giver);

// 停止特定 FA
BuffFlowComponent->StopBuffFlow(FGuid Guid);

// 停止全部 FA（Actor 死亡时）
BuffFlowComponent->StopAllBuffFlows();

// 广播战斗事件（攻击/冲刺 GA 端）
UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
    Actor, FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit")), Payload);

// 检查 BFC 是否存在（敌人和玩家的父类 AYogCharacterBase 均有 BFC）
UBuffFlowComponent* BFC = Actor->FindComponentByClass<UBuffFlowComponent>();
```

---

---

## 十二、TriggerType 激活系统

### 12.1 设计意图

原有所有符文都是 Passive——进入激活区后 FA 立即启动，FA 内部再用 `OnDamageDealt`、`OnKill` 等触发节点等待事件。这导致：
- FA 长期挂起，占用 Flow 实例
- 符文"常驻运行"与"事件驱动"的语义耦合在 FA 内部，不直观

**TriggerType** 把触发时机提前到 BGC 层：符文只有在指定事件发生时才启动 FA 实例，FA 本身不再需要事件等待节点。

### 12.2 枚举定义

```cpp
// Source/DevKit/Public/Data/RuneDataAsset.h
UENUM(BlueprintType)
enum class ERuneTriggerType : uint8
{
    Passive          // 进激活区立即启动 FA（默认）
    OnAttackHit      // Ability.Event.Attack.Hit 时启动
    OnDash           // Ability.Event.Dash 时启动
    OnKill           // Ability.Event.Kill 时启动
    OnCritHit        // Ability.Event.Attack.Hit（暴击，复用同 Tag）时启动
    OnDamageReceived // Ability.Event.Damaged 时启动
};
```

字段位置：`RuneDataAsset → RuneInfo → RuneConfig → TriggerType`（编辑器 Category: "Trigger"）

### 12.3 BGC 的行为差异

| TriggerType | BGC::ActivateRune 行为 | BGC::DeactivateRune 行为 |
|-------------|----------------------|------------------------|
| Passive | 立即 `BFC->StartBuffFlow(FA, RuneGuid, ...)` | `BFC->StopBuffFlow(RuneGuid)` |
| 其他类型 | 向玩家 ASC 注册事件监听器，每次事件触发 `BFC->StartBuffFlow(FA, NewGuid, HitTarget)` | 从 ASC 移除监听器（已启动的 FA 实例自行运行至结束） |

**关键差异**：事件驱动型符文的每次触发都生成新 GUID，FA 实例独立运行；不像 Passive 那样单个 FA 实例持续持有 GE handle。

### 12.4 FA 设计规则（按 TriggerType）

#### Passive（持续型）
FA 长期运行，内部需要事件节点才能做条件触发：
```
[Start] → [OnDamageDealt] → [ApplyEffect ...] → ↩ 循环
```
符文离开激活区时 BGC 调用 `StopBuffFlow`，FA Cleanup 清除 GE/GA。

#### 事件驱动型（OnAttackHit / OnKill / OnDash）
FA 收到事件时才启动，**每次独立实例**，适合**即发即止**的效果：
```
[Start] → [DoDamage / SpawnActor / instant GE] → [FinishBuff]
```
> ⚠️ **不适合 Duration GE**：FA 实例结束时 BFNode_ApplyEffect 会 Cleanup 移除 GE。
> 需要"持续 N 秒 buff"的符文应保持 **Passive + 内部 OnDamageDealt 节点**，不要改为事件驱动型。

#### 事件驱动型 + Delay（受控持续）
若确实需要事件触发但效果持续一段时间：
```
[Start] → [ApplyAttributeModifier: Additive +30, HasDuration=3s] → [FinishBuff]
```
`ApplyAttributeModifier` 使用 `HasDuration` 模式：GE 的持续时间由 GAS 管理，FA 可立即 Finish 而不会破坏 GE 生命周期（AttributeModifier 的 GE 是 Transient，不存 handle，Cleanup 无副作用）。

---

## 十三、GE 生命周期驱动 GA 模式

### 13.1 问题场景

"玩家攻击给敌人打上 3 秒的腐蚀印记，期间敌人持续受到灼伤（GA_Burn）。印记到期后灼伤自动消失。"

传统方案需要在 Blueprint GE 里填写 `GrantedAbilities`——不可见、不可热更新。

### 13.2 新节点能力

**BFNode_ApplyEffect** 新增 `OnRemoved` 执行输出引脚：GE 被外部移除（到期或手动 Remove）时触发，受 `bCleaningUp` 保护（FA 自身 Cleanup 移除 GE 时不触发此引脚，避免循环）。

**BFNode_GrantGA** 新增 `Grant` / `Revoke` 输入引脚和 `Revoked` 输出引脚：
- 旧 `In` 引脚已更名为 `Grant`（已有 FA 需在编辑器中重连）
- `Revoke` 引脚：调用 `ClearAbility` 并触发 `Revoked` 输出
- `Cleanup` 仍会自动撤销 GA（FA 整体停止时兜底）

### 13.3 FA 连线方案（腐蚀印记示例）

```
[Start]
  │
  ▼
[OnDamageDealt]
  │
  ▼
[ApplyEffect: GE_CorrosionMark, Duration=3s] ──Out──▶ [GrantGA: GA_Burn, Grant]
                                              │
                                              OnRemoved
                                              │
                                              ▼
                                         [GrantGA: GA_Burn, Revoke]
```

**工作流程**：
1. 命中目标 → 施加 GE_CorrosionMark（3s），同时授予 GA_Burn
2. GE 到期 → `OnRemoved` 触发 → `Revoke` 撤销 GA_Burn
3. 符文离开激活区 → FA Cleanup → GE 和 GA 一并清理（`bCleaningUp` 防止 OnRemoved 重复触发）

### 13.4 适用场景

| 场景 | 推荐方案 |
|------|---------|
| GE 持续期间给攻击者 buff，GE 到期 buff 消失 | ApplyEffect.OnRemoved → GrantGA.Revoke |
| GE 到期触发爆炸 | ApplyEffect.OnRemoved → DoDamage / SpawnActor |
| GE 到期开始下一阶段 | ApplyEffect.OnRemoved → 下一个 ApplyEffect |
| GA 需要独立于 GE 存活 | 不使用此模式，让 GrantGA 由 FA Cleanup 管理 |

---

*本文档描述 FA 通用架构的设计规范与实现现状。具体节点用法见 [BuffFlow_NodeReference.md](BuffFlow_NodeReference.md)，符文创建流程见 [BuffFlow_RuneWorkflow.md](BuffFlow_RuneWorkflow.md)。*
