# 开发方案：剧情节点绑定 Flow Asset（NodeEventFlow）v2

> 已整合 Codex 审查意见（2026-05-25）及用户确认答复。

## 需求描述

剧情编辑器中，所有剧情专属事件（如"木头人死亡掉落重击卡"）**不应修改角色类**，而应完全由剧情系统驱动。具体要求：

1. 每个剧情节点可绑定一个 **`NodeEventFlow`（`UFlowAsset*`）**，节点的 `Actions[]` 全部执行完后自动运行该 FA
2. FA 负责处理一切剧情专属行为（生成拾取物、播放序列、解锁特效等）
3. 角色/敌人类只暴露必要接口，剧情系统负责调用，角色类本身不感知剧情需求
4. 修复现有 `PlayLevelFlow` 动作在非 Trigger Actor 事件触发时退回到 GameMode 全局 Flow、丢失触发源上下文的问题

## 已确认设计决策

| 问题 | 决策 |
|------|------|
| `NodeEventFlow` 执行时机 | `Actions[]` 全部执行**之后** |
| 并发策略 | 无需考虑（单机，死亡是单次事件） |
| FA 上下文 | ContextSourceActor（弱引用）+ ContextTransform（快照）+ PlayerController |
| `PlayLevelFlow` 回退 | 统一使用 Proxy，保留 SourceActor 上下文；`bStopExistingStoryFlow=true` 时先停旧 Proxy |
| 木头人位置 | 触发时立即缓存 `GetActorTransform()`，FA 读快照不读弱引用 |

---

## 方案设计

### 核心思路

在 `FStoryEncounterNode`（及 `UStoryEncounterPointDA`）上增加 `NodeEventFlow` 字段（`UFlowAsset*`）。

**关键技术细节（已核实）：**
- `UStoryEncounterRuntimeSubsystem` 继承 `UGameInstanceSubsystem`，`GetWorld()` 需通过 `GetGameInstance()->GetWorld()`
- `UFlowComponent` 无结束委托；使用 **定时轮询 `GetRootInstances()`** 检测 Flow 是否完成后销毁 Proxy
- `PlayLevelFlow` 回退：在现有 cast 链的 else 分支调用 `RunFlowViaProxy()`，必须立即 `return` 防止继续走 GameMode 全局 Flow（双跑问题）
- 三条触发入口（`TriggerEncounterNode`、`TriggerEncounterPoint`、`TriggerEncounterGraphNode`）需**抽出公共方法** `ExecuteEncounterNodeCore()`

### 架构

```
Story Node [Death, 目标=木头人]
  Actions[]:  WeakHint → 显示"击败木头人！"
  NodeEventFlow = FA_DummyDeath_DropHeavyCard
        ↓ Actions 完成后
  RunFlowViaProxy(FA, SourceActor=木头人)
        ↓
  AStoryFlowProxy  (临时 Actor，Flow 完成后自动销毁)
    ├── FlowComp.RootFlow = FA
    ├── ContextSourceActor = 木头人（弱引用，仅供备用）
    ├── ContextTransform   = 木头人死亡时的 Transform（快照）
    └── PlayerController   = 当前本地玩家 PC
```

### `PlayLevelFlow` 问题澄清

现有行为：非 Trigger Actor 时，代码回退到 `StoryEngine->ExecuteStoryAction()` → `GameMode->RunStoryLevelFlow()`，Flow 执行但没有 SourceActor 上下文。**不是完全静默失败，而是上下文丢失**。

修复后：用 Proxy 运行，SourceActor 上下文完整，同时加 `return` 阻止继续走 GameMode。

---

## 实现步骤

### Step 1：数据结构 — `FStoryEncounterNode` 加字段

文件：`Source/DevKit/Public/Story/Encounter/StoryEncounterTypes.h`

```cpp
// 在文件顶部 forward declare 补充：
class UFlowAsset;

// 在 FStoryEncounterNode::Actions 字段之后插入：
/** Actions[] 执行完后自动运行此 FA，用于处理剧情专属行为（生成拾取物等）。
 *  FA 内通过 ULENode_GetStoryContext 获取上下文。 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "剧情节点|Flow")
TObjectPtr<UFlowAsset> NodeEventFlow = nullptr;
```

> 类型从 `ULevelFlowAsset` 改为 `UFlowAsset`（更通用，LevelFlowAsset 是其子类，也可直接赋值）。

### Step 2：数据结构 — `UStoryEncounterPointDA` 加同名字段

文件：`Source/DevKit/Public/Story/Encounter/StoryEncounterPointDataAsset.h`

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "节点 Flow")
TObjectPtr<UFlowAsset> NodeEventFlow = nullptr;
```

### Step 3：`ToEncounterNode()` 同步字段

文件：`Source/DevKit/Private/Story/Encounter/StoryEncounterPointDataAsset.cpp`

```cpp
Node.NodeEventFlow = NodeEventFlow;
```

### Step 4：新建 `AStoryFlowProxy`

**`Source/DevKit/Public/Story/Encounter/StoryFlowProxy.h`**

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryFlowProxy.generated.h"

class UFlowComponent;
class UFlowAsset;

/**
 * 轻量代理 Actor，为 StoryEncounterRuntimeSubsystem 提供 FlowComponent 宿主。
 * Flow 执行完毕后自动 Destroy。
 * 提供触发上下文：ContextSourceActor（弱引用）/ ContextTransform（快照）/ PlayerController。
 */
UCLASS(NotBlueprintable)
class DEVKIT_API AStoryFlowProxy : public AActor
{
    GENERATED_BODY()
public:
    AStoryFlowProxy();
    virtual void BeginPlay() override;

    void RunFlow(UFlowAsset* FlowAsset);

    // --- 上下文接口（供 ULENode_GetStoryContext FA 节点读取）---
    UFUNCTION(BlueprintPure, Category = "Story|Context")
    AActor* GetContextSourceActor() const { return ContextSourceActor.Get(); }

    UFUNCTION(BlueprintPure, Category = "Story|Context")
    FTransform GetContextTransform() const { return ContextTransform; }

    UFUNCTION(BlueprintPure, Category = "Story|Context")
    APlayerController* GetContextPlayerController() const { return ContextPlayerController.Get(); }

    // --- 由 Subsystem 在 RunFlow 前设置 ---
    UPROPERTY()
    TWeakObjectPtr<AActor> ContextSourceActor;

    UPROPERTY()
    FTransform ContextTransform;

    UPROPERTY()
    TWeakObjectPtr<APlayerController> ContextPlayerController;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UFlowComponent> FlowComp;

    FTimerHandle PollTimer;
    void PollFlowCompletion();
};
```

**`Source/DevKit/Private/Story/Encounter/StoryFlowProxy.cpp`**

```cpp
#include "Story/Encounter/StoryFlowProxy.h"
#include "Flow/FlowComponent.h"   // 确认实际 include 路径

AStoryFlowProxy::AStoryFlowProxy()
{
    PrimaryActorTick.bCanEverTick = false;
    // 设为 Transient，不参与存档、不显示在 Outliner 调试以外
    bNetLoadOnClient = false;

    FlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComp"));
    FlowComp->bAutoStartRootFlow = false;
}

void AStoryFlowProxy::BeginPlay()
{
    Super::BeginPlay();
}

void AStoryFlowProxy::RunFlow(UFlowAsset* FlowAsset)
{
    if (!FlowAsset || !FlowComp) return;

    FlowComp->RootFlow = FlowAsset;
    FlowComp->StartRootFlow();

    // UFlowComponent 没有结束委托，改用轮询检测 Flow 完成
    GetWorldTimerManager().SetTimer(
        PollTimer, this, &AStoryFlowProxy::PollFlowCompletion,
        0.25f, /*bLoop=*/true);
}

void AStoryFlowProxy::PollFlowCompletion()
{
    if (!FlowComp) { Destroy(); return; }

    // GetRootInstances 返回所有仍在运行的 Root Flow 实例
    TSet<UFlowAsset*> Running = FlowComp->GetRootInstances(FlowComp);
    if (Running.IsEmpty())
    {
        GetWorldTimerManager().ClearTimer(PollTimer);
        Destroy();
    }
}
```

> `GetRootInstances(const UObject* Owner)` 的 Owner 参数传 `FlowComp` — 编译时核对签名，若参数类型不符按实际调整。

### Step 5：Subsystem — 提取公共执行方法

文件：`Source/DevKit/Public/Story/Encounter/StoryEncounterRuntimeSubsystem.h`（private 区）

```cpp
/** Actions[] 执行 + NodeEventFlow 运行，三条入口共用。 */
void ExecuteEncounterNodeCore(const FStoryEncounterNode& Node, const FStoryEventContext& Context);

/** 通过临时 Proxy Actor 运行 FlowAsset，传递 SourceActor 上下文。
 *  bStopExisting=true 时先销毁与该 FlowAsset 关联的已有 Proxy。 */
void RunFlowViaProxy(UFlowAsset* FlowAsset, AActor* SourceActor,
                     APlayerController* PC, bool bStopExisting = false);
```

文件：`Source/DevKit/Private/Story/Encounter/StoryEncounterRuntimeSubsystem.cpp`

```cpp
void UStoryEncounterRuntimeSubsystem::ExecuteEncounterNodeCore(
    const FStoryEncounterNode& Node, const FStoryEventContext& Context)
{
    // 执行全部 Actions
    for (const FStoryEncounterAction& Action : Node.Actions)
    {
        ExecuteEncounterAction(Action, Context);
    }

    // Actions 完成后运行 NodeEventFlow
    if (Node.NodeEventFlow)
    {
        APlayerController* PC = ResolveEncounterPlayer(Context.SourceActor);
        RunFlowViaProxy(Node.NodeEventFlow, Context.SourceActor, PC);
    }
}

void UStoryEncounterRuntimeSubsystem::RunFlowViaProxy(
    UFlowAsset* FlowAsset, AActor* SourceActor,
    APlayerController* PC, bool bStopExisting)
{
    if (!FlowAsset) return;

    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World) return;

    // bStopExisting：销毁已有的同 FlowAsset Proxy
    if (bStopExisting)
    {
        for (TActorIterator<AStoryFlowProxy> It(World); It; ++It)
        {
            if (It->FlowComp && It->FlowComp->RootFlow == FlowAsset)
            {
                It->Destroy();
            }
        }
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStoryFlowProxy* Proxy = World->SpawnActor<AStoryFlowProxy>(
        AStoryFlowProxy::StaticClass(), FTransform::Identity, Params);
    if (!Proxy)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[StoryEncounter] RunFlowViaProxy: SpawnActor failed for %s"),
            *FlowAsset->GetName());
        return;
    }

    // 写入上下文
    Proxy->ContextSourceActor = SourceActor;
    Proxy->ContextTransform   = SourceActor
        ? SourceActor->GetActorTransform()
        : FTransform::Identity;
    Proxy->ContextPlayerController = PC;

    Proxy->RunFlow(FlowAsset);
}
```

### Step 6：三条触发入口统一使用 `ExecuteEncounterNodeCore()`

将 `TriggerEncounterNode()`、`TriggerEncounterPoint()`、`TriggerEncounterGraphNode()` 中
原本分散的 "for each Action → ExecuteEncounterAction" 循环
替换为一次调用 `ExecuteEncounterNodeCore(Node, Context)`。

### Step 7：修复 `PlayLevelFlow` 上下文丢失问题

在 `ExecuteEncounterAction()` 处理 `PlayLevelFlow` 的 cast 链末尾加 else 分支，并加 `return`：

```cpp
if (AStoryEncounterTrigger* T = Cast<AStoryEncounterTrigger>(Context.SourceActor))
{
    if (T->RunLevelFlow(StoryAction.LevelFlow, StoryAction.bStopExistingStoryFlow))
        return;
}
else if (ALevelEventTrigger* LE = Cast<ALevelEventTrigger>(Context.SourceActor))
{
    if (LE->RunLevelFlow(StoryAction.LevelFlow, StoryAction.bStopExistingStoryFlow))
        return;
}
else
{
    // 非 Trigger Actor（如角色死亡）：用 Proxy 保留 SourceActor 上下文
    APlayerController* PC = ResolveEncounterPlayer(Context.SourceActor);
    RunFlowViaProxy(StoryAction.LevelFlow, Context.SourceActor, PC,
                    StoryAction.bStopExistingStoryFlow);
    return;   // ← 必须 return，防止继续走 GameMode 全局 Flow（双跑）
}
```

### Step 8：新建 FA 上下文节点（供 Flow 编辑器使用）

新建 `ULENode_GetStoryContext`（`FlowNode` 子类）：

```cpp
// Source/DevKit/Public/LevelFlow/Nodes/LENode_GetStoryContext.h
UCLASS(meta = (DisplayName = "Get Story Context"))
class DEVKIT_API ULENode_GetStoryContext : public UFlowNode
{
    GENERATED_BODY()
protected:
    virtual void ExecuteInput(const FName& PinName) override;
    // Output pins: SourceActor(Object), ContextTransform, PlayerController
};
```

实现：在 `ExecuteInput` 里从 `GetFlowComponent()->GetOwner()` Cast 到 `AStoryFlowProxy`，读取三个字段，通过 Output Pin 输出，然后 `TriggerOutput("Out")`。

---

## 用法示例：木头人死亡掉落重击卡

```
剧情图节点配置：
  Kind          = Death
  目标 Tag      = Enemy.TrainingDummy
  Actions[]     = [ WeakHint: "木头人已击败！" ]
  NodeEventFlow = FA_DummyDeath_DropHeavyCard （新建 FlowAsset）

FA_DummyDeath_DropHeavyCard 节点流程：
  [Start]
    → [Get Story Context]
        ├── ContextTransform ──→ [Spawn Actor: BP_Pickup_HeavyCard]
        │                           Location = ContextTransform.Location + (0, 0, 50)
        └── [Finish]
```

FA 内读取 **`ContextTransform`（快照）** 而非 Actor 弱引用，不受木头人 0.1s 后复位的影响。

---

## 涉及文件

| 文件 | 改动内容 |
|------|---------|
| `Source/DevKit/Public/Story/Encounter/StoryEncounterTypes.h` | `FStoryEncounterNode` 加 `NodeEventFlow` 字段；forward declare `UFlowAsset` |
| `Source/DevKit/Public/Story/Encounter/StoryEncounterPointDataAsset.h` | `UStoryEncounterPointDA` 加同名字段 |
| `Source/DevKit/Private/Story/Encounter/StoryEncounterPointDataAsset.cpp` | `ToEncounterNode()` 同步新字段 |
| `Source/DevKit/Public/Story/Encounter/StoryFlowProxy.h` | **新建**：Proxy Actor 头文件 |
| `Source/DevKit/Private/Story/Encounter/StoryFlowProxy.cpp` | **新建**：Proxy Actor 实现（含轮询销毁） |
| `Source/DevKit/Public/Story/Encounter/StoryEncounterRuntimeSubsystem.h` | 声明 `ExecuteEncounterNodeCore()` + `RunFlowViaProxy()` |
| `Source/DevKit/Private/Story/Encounter/StoryEncounterRuntimeSubsystem.cpp` | 实现两个方法；三入口改用 `ExecuteEncounterNodeCore`；`PlayLevelFlow` 加 else + return |
| `Source/DevKit/Public/LevelFlow/Nodes/LENode_GetStoryContext.h` | **新建**：FA 上下文读取节点头文件 |
| `Source/DevKit/Private/LevelFlow/Nodes/LENode_GetStoryContext.cpp` | **新建**：FA 上下文读取节点实现 |

---

## 潜在风险

- **`GetRootInstances()` Owner 参数**：传 `FlowComp` 还是 `this`（Proxy Actor），需编译时核对函数签名
- **`FlowComp` include 路径**：可能是 `Flow/FlowComponent.h` 或 `FlowGraph/FlowComponent.h`，需按项目插件实际路径调整
- **Proxy 对 FA Root Owner 的影响**：部分现有 LevelFlow 节点（如 `LENode_ShowInfoPopup`）可能识别 `ALevelEventTrigger`；用 Proxy 可能改变其行为，需逐一测试
- **SaveGame 时 Proxy 存在**：`UFlowComponent` 有 SaveGame 字段，确保 Proxy 不被意外存档（考虑设 `bNetLoadOnClient=false` + 不加 SaveGame Tag）
- **硬引用加载成本**：`TObjectPtr<UFlowAsset>` 是硬引用，DA 加载时连带加载 Flow 资产；后期可改 `TSoftObjectPtr` 按需异步加载

---

## 需要手动创建的引擎资产

| 资产名 | 类型 | 存放路径建议 | 说明 |
|--------|------|-------------|------|
| `FA_DummyDeath_DropHeavyCard` | FlowAsset（LevelFlowAsset） | `Content/Story/Flows/Tutorial/` | 木头人死亡掉落重击卡的 FA |
| 对应 EncounterMap / PointDA 配置 | 编辑器 DA 配置 | 现有剧情资产 | 死亡节点的 `NodeEventFlow` 槽设为该 FA |
