# BuffFlow 符文系统 — 程序指南

> 版本：Sprint 4.15（2026-04-08）

---

## 一、架构三层模型

```
FA（Flow Asset）= 逻辑层
  ├── FlowGraph 内置节点：控制流 / 计时 / 计数
  ├── BFNode_*：GAS 集成（触发 / 属性 / Tag / GE / GA）
  └── 数据引脚：节点间值传递

GAS / GE / GA = 效果层（属性计算、Tag、Cue、能力激活）

数据 = 配置层（RuneDataAsset / Blueprint GE / Blueprint GA）
```

**核心原则：**
- 所有符文行为逻辑在 FA 可视化，不散落在蓝图或 C++
- 能用 `BFNode_ApplyAttributeModifier` 的，不创建 Blueprint GE
- Blueprint GE 仅在需要 ExecutionCalculation / Cue / SetByCaller 时才创建
- Blueprint GA 仅在需要 AbilityTask / 物理冲量 / 网络同步时才创建

---

## 二、核心组件

### BackpackGridComponent (BGC)

**职责：** 管理 5×5 背包格、热度 Phase（0–3）、激活区大小，根据符文是否在激活区内自动 Start/Stop FA。

**关键委托：**

| 委托 | 触发条件 |
|---|---|
| `OnHeatReachedZero` | 热度 >0 → 0（Phase>0 守卫，防止 Phase=0 时误触发） |
| `OnHeatAboveZero` | 热度 0 → >0 |
| `OnHeatTierChanged` | CurrentTier 变化 |
| `OnRunePlaced / Removed / ActivationChanged` | 符文槽位事件 |

**Phase 与激活区对应关系：**

| Phase | 激活区（默认配置） |
|---|---|
| 0 | 中心 1×1，格子 (2,2) |
| 1 | 中心 2×2，格子 (2,2)-(3,3) |
| 2 | 中心 4×4，格子 (1,1)-(4,4) |
| 3 | 同 Phase2（Transcendence 外观不同） |

**重要字段：**

```cpp
TArray<URuneDataAsset*> PermanentRunes;    // BeginPlay 自动激活，不受激活区限制
TArray<URuneDataAsset*> DebugTestRunes;    // 调试用，BeginPlay 自动放置
TArray<FIntPoint> DebugTestPositions;      // 与 DebugTestRunes 对应的初始位置
int32 CurrentPhase = 0;
EHeatTier CurrentTier = EHeatTier::Tier1;
```

---

### BuffFlowComponent (BFC)

**职责：** FA 实例生命周期管理 + 节点间共享数据通道。

**共享上下文（节点间数据传递桥梁）：**

| 字段 | 写入节点 | 读取场景 |
|---|---|---|
| `LastEventContext.DamageCauser` | OnDamageDealt | Target=DamageCauser |
| `LastEventContext.DamageReceiver` | OnDamageDealt | Target=LastDamageTarget |
| `LastEventContext.DamageAmount` | OnDamageDealt | DoDamage 倍率计算 |
| `LastKillLocation` | OnKill | SpawnActorAtLocation |
| `ActiveNiagaraEffects` | PlayNiagara | DestroyNiagara |

**FA 实例隔离（Dota 道具模型）：** 每个符文实例有独立的 FA 实例，`FActiveGameplayEffectHandle` 存储在对应的 BFNode member variable 里，Rune 卸下时只 Stop 对应 FA，不影响其他符文。

---

## 三、BFNode 开发规范

### 新节点基本结构

```cpp
UCLASS(NotBlueprintable, meta=(DisplayName="节点名", Category="BuffFlow|分类"))
class DEVKIT_API UBFNode_Xxx : public UFlowNode
{
    GENERATED_UCLASS_BODY()
protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;
};
```

**构造函数：**
```cpp
UBFNode_Xxx::UBFNode_Xxx(const FObjectInitializer& OI) : Super(OI)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|分类");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}
```

**纯数据节点（无执行引脚，如 LiteralFloat）：**
```cpp
InputPins  = {};
OutputPins = {};  // 只通过 FFlowDataPinOutputProperty_Float 暴露数据引脚
```

---

### 获取 ASC 的标准方式

```cpp
UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
    GetFlowAsset()->GetOwner()
);
if (!ASC) { TriggerOutput(TEXT("Failed"), true); return; }
```

---

### 数据引脚读写

**输入属性（支持连数据线覆盖默认值）：**
```cpp
UPROPERTY(EditAnywhere, Category="BuffFlow")
FFlowDataPinInputProperty_Float Value;
// 读取时：
float ResolvedValue = Value.GetValue();  // 自动取数据引脚或默认值
```

**输出属性（下游节点可读取）：**
```cpp
UPROPERTY(EditAnywhere, Category="BuffFlow")
FFlowDataPinOutputProperty_Float Result;
// 写入时：
Result.Value = 42.f;
```

---

### FActiveGameplayEffectHandle 管理

- 每个 BFNode 实例自持 `FActiveGameplayEffectHandle GEHandle`
- `ExecuteInput("In")` 时 Apply，赋值给 GEHandle
- `Cleanup()` 时 `ASC->RemoveActiveGameplayEffect(GEHandle)`
- **禁止** 使用 `TArray<FActiveGameplayEffectHandle>` 全局共享

---

### 动态创建内联 GE（ApplyAttributeModifier 模式）

```cpp
UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
// Period（每秒执行）
if (Period > 0.f)
{
    GE->Period = FScalableFloat(Period);
    GE->bExecutePeriodicEffectOnApplication = bFireImmediately;
}
FGameplayModifierInfo ModInfo;
ModInfo.Attribute = FGameplayAttribute(...);
ModInfo.ModifierOp = EGameplayModOp::Additive;
ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
GE->Modifiers.Add(ModInfo);
```

---

### 动态创建含 ExecCalc 的 GE（ApplyExecution 模式）

```cpp
if (!CachedGE || CachedExecClass != ExecCalcClass)
{
    CachedGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
    CachedGE->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayEffectExecutionDefinition ExecDef;
    ExecDef.CalculationClass = ExecCalcClass;
    CachedGE->Executions.Add(ExecDef);
    CachedExecClass = ExecCalcClass;
}
FGameplayEffectSpec DirectSpec(CachedGE, Context, 1.f);
// SetByCaller
DirectSpec.SetSetByCallerMagnitude(SetByCallerTag1, SetByCallerValue1.GetValue());
FActiveGameplayEffectHandle H = ASC->ApplyGameplayEffectSpecToSelf(DirectSpec);
```

---

### Timer 使用（GrantTag / Delay 模式）

```cpp
FTimerHandle TimerHandle;
FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this]()
{
    // 到期逻辑
    TriggerOutput(TEXT("Expired"), true);
});
GetWorld()->GetTimerManager().SetTimer(TimerHandle, Delegate, Duration, false);

// Cleanup 时清理
void UBFNode_Xxx::Cleanup()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
    Super::Cleanup();
}
```

---

## 四、GAS 功能 FA 化对照表

| GAS 功能 | FA 实现方式 | 需要资产？ |
|---|---|---|
| 属性修改（普通） | `BFNode_ApplyAttributeModifier` | ❌ |
| 属性修改（每秒 +N） | `BFNode_ApplyAttributeModifier` Period 字段 | ❌ |
| ExecutionCalculation | `BFNode_ApplyExecution` 直接引用 C++ 类 | ❌ |
| Tag 授予（永久） | `BFNode_AddTag` | ❌ |
| Tag 授予（临时） | `BFNode_GrantTag` Duration | ❌ |
| SetByCaller | `BFNode_ApplyEffect` SetByCaller 槽 | ✅ Blueprint GE |
| GA 授予/撤销 | `BFNode_GrantGA` | ✅ Blueprint GA |
| OngoingTagRequirements.IgnoreTags（Inhibit） | 需要 Blueprint GE 配置 | ✅ Blueprint GE |
| Gameplay Cue | ⏳ BFNode_PlayGameplayCue（P1 未完成） | — |
| AbilityTask / 物理冲量 | 保留 Blueprint GA | ✅ Blueprint GA |

---

## 五、数据引脚类型

| 类型 | 使用情况 | 场景 |
|---|---|---|
| Bool | ✅ | bGEApplied, bGAGranted 等输出 |
| Int | ✅ | StackCount, Goal 等 |
| Float | ✅ | Value, Duration 等 |
| GameplayTag | ❌ P1 计划 | 动态 Tag 参数 |
| Class | ❌ P1 计划 | 动态 GE/GA Class |
| Object | ❌ 未使用 | Actor 引用传递 |
| Vector | ❌ 未使用 | 击退方向等 |
| Name | ❌ 未使用 | 动态 EffectName |

---

## 六、P0–P3 开发计划

### ✅ 已完成（P0）

| 模块 |
|---|
| BFNode 基础框架（约 40 个节点） |
| `BFNode_ApplyAttributeModifier`（含 Period / StackMode） |
| `BFNode_ApplyEffect`（含 Remove 引脚 + SetByCaller） |
| `BFNode_ApplyExecution`（零资产 ExecutionCalculation） |
| `BFNode_GrantTag`（Duration 自动过期） |
| `BFNode_Delay`（可取消计时器） |
| `BFNode_LiteralFloat/Int/Bool`（字面量数据节点） |
| 热度升降阶系统（IncrementPhase / DecrementPhase / PhaseDecayTimer） |
| RuneDataAsset 精简结构（RuneInfo + FRuneConfig） |

### P1 — 框架扩展

| 任务 | 说明 |
|---|---|
| `BFNode_PlayGameplayCue` | 触发 GameplayCue，不绑 GE |
| `BFNode_Base` Blueprintable | 开放蓝图继承，支持 Flow Node Blueprint |
| GameplayTag / Class 数据引脚 | Tag、GE Class 参数可动态传入 |

### P2 — 战斗能力节点

| 任务 | 说明 |
|---|---|
| `BFNode_ActivateAbilityByTag` | 触发目标 GA |
| `BFNode_SendGameplayEvent` | 发送 GameplayEvent |

### P3 — 体验完善

| 任务 | 说明 |
|---|---|
| 升阶 CanPhaseUp 检查移入 FA | 当前在 FNode 内部 |
| 升阶特效/音效 | PlayGameplayCue 完成后接入 |
| 热度 UI | 阶段显示 + 激活区视觉反馈 |

---

## 七、文件清单

```
Source/DevKit/Public/BuffFlow/Nodes/   — 所有 BFNode 头文件
Source/DevKit/Private/BuffFlow/Nodes/  — 所有 BFNode 实现
Source/DevKit/Public/Component/BackpackGridComponent.h
Source/DevKit/Public/BuffFlow/BuffFlowComponent.h
Source/DevKit/Public/Data/RuneDataAsset.h
```

---

## ⚠️ Claude 编写注意事项

- **所有 Buff/符文效果必须走 FA**：不允许把 GA 预授予到角色 Blueprint，需要动态效果时用 FA 的 BFNode_GrantAbility 节点动态 grant
- **BFNode 必须调且只调一次 `ExecuteOutput()`**：同步节点在 Execute() 末尾调，异步节点在回调里调，忘记调会导致 FA 卡死不往下执行
- **FA 执行是同帧同步**：不要在 BFNode 里做耗时操作或 latent 等待，如需异步用 UE 的 Delay Task 或 AbilityTask
- **BFNode 的输入 Pin 参数**：在 `GetInputPinNames()` 里注册的 Pin 名必须与 BP 里连线的名字完全一致（大小写敏感）
- **RuneDataAsset 的 FA 引用用软引用**：`TSoftObjectPtr<UFlowAsset>` 防止启动时全部加载，需要时调 `LoadSynchronous()`
- **自定义 BFNode 必须在 DevKit.Build.cs 添加模块依赖**：如果新 BFNode 用到 GameplayAbilities 模块，确认 Build.cs 里已有
