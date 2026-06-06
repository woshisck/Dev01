> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# Camera Occlusion Fade Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Diablo-like camera occlusion fade system: when world objects block the camera's view of the player, those objects fade out, then fade back in when they no longer block the player.

**Architecture:** Add a player-owned `UYogCameraOcclusionFadeComponent` that queries the final camera-to-player view path and manages fade state for hit `UPrimitiveComponent`s. The camera manager continues to own camera movement; this component owns only occlusion detection and material fade state.

**Tech Stack:** Unreal Engine 5.4, C++ `UActorComponent`, `SweepMultiByChannel` / sphere trace, `UMaterialInstanceDynamic`, Automation Tests, existing `APlayerCharacterBase` camera setup.

---

## 功能定位

这个功能不是一个场景 `Volume`，也不是一个碰撞盒 Actor。

它是挂在玩家身上的逻辑组件：

```text
CameraLocation ===== sphere trace / sweep ===== PlayerTargetLocation
```

每次检测时，组件从当前相机位置到玩家身上一个目标点做球形 Trace。命中的遮挡物进入淡出状态；上一轮淡出但本轮没有命中的遮挡物进入淡入恢复状态。

推荐类名：

```cpp
UYogCameraOcclusionFadeComponent
```

推荐挂载位置：

```cpp
APlayerCharacterBase
```

---

## 为什么不用纯 Overlap

Overlap 可以做，但不适合作为这个功能的主判断。

纯 Overlap 的问题：

- 它只能说明“某物在某个体积里”，不能稳定说明“某物挡在相机和玩家之间”。
- 如果做一个从相机拉到玩家的长盒/胶囊 Overlap，本质上是在手写一个持续存在的 Sweep 体积，更新旋转、长度、端点会更麻烦。
- 它容易把相机侧边、玩家身后、相机背后的物体误判为遮挡物。

推荐使用 `Sphere Trace Multi` / `SweepMultiByChannel`：

- 查询范围窄，误判少。
- 不需要维护额外碰撞体。
- 对本地玩家每 `0.03s - 0.08s` 做一次检测通常足够便宜。
- 球形半径能覆盖柱子边缘、墙角这种“细射线刚好擦过”的情况。

结论：

```text
主检测：Sphere Trace Multi
插值更新：每帧 Tick
检测频率：可配置，默认 0.05s
```

---

## 材质淡入淡出的关键约束

UE 里不是所有材质都能直接透明。

如果一个普通 Opaque 材质没有接入透明/抖动淡化逻辑，即使 C++ 给它设置 `Opacity = 0.2`，画面也可能完全不变。

推荐材质方案：

1. 场景中可能遮挡玩家的材质统一支持一个标量参数：

```text
CameraOcclusionAlpha
```

2. 材质里用这个参数做 Dither Fade，推荐接入 `DitherTemporalAA` 或项目已有的遮罩淡化材质函数。

3. 遮挡物默认 `CameraOcclusionAlpha = 1.0`，被遮挡时插值到 `MinVisibleAlpha`，例如 `0.15`。

4. 如果某些旧材质暂时不支持该参数，组件可以创建 Dynamic Material 并尝试设置参数，但不会保证视觉生效。

可选兜底：

- 给组件提供 `FallbackFadeMaterial`，遇到不支持参数的材质时临时替换。
- 缺点是会丢失原材质外观，只适合开发期或低优先级场景物。

---

## 默认设计参数

`UYogCameraOcclusionFadeComponent` 建议暴露这些参数给蓝图：

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
bool bEnableOcclusionFade = true;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0"))
float TraceInterval = 0.05f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0"))
float TraceRadius = 28.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
FVector PlayerTargetOffset = FVector(0.0f, 0.0f, 80.0f);

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
bool bOnlyFadeTaggedOccluders = true;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
FName OccluderTag = TEXT("CameraOccluder");

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
float MinVisibleAlpha = 0.15f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.01"))
float FadeOutSpeed = 12.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.01"))
float FadeInSpeed = 8.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
FName FadeScalarParameterName = TEXT("CameraOcclusionAlpha");
```

默认需要遮挡物带 `CameraOccluder` Tag，是为了避免误淡出敌人、掉落物、玩家武器、UI 代理 Actor 等非场景遮挡物。

Tag 可以放在：

- Actor Tags
- Component Tags

命中任一即可允许淡化。

---

## 文件与职责

### 新增文件

```text
Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h
Source/DevKit/Private/Component/YogCameraOcclusionFadeComponent.cpp
Source/DevKit/Private/Tests/CameraOcclusionFadeComponentTests.cpp
```

### 修改文件

```text
Source/DevKit/Public/Character/PlayerCharacterBase.h
Source/DevKit/Private/Character/PlayerCharacterBase.cpp
```

### 职责边界

`UYogCameraOcclusionFadeComponent`：

- 获取当前相机位置。
- 计算玩家被观察目标点。
- 做 Sphere Trace Multi。
- 过滤可淡化组件。
- 维护每个被淡化组件的当前透明度、目标透明度和原始材质。
- 创建并复用 `UMaterialInstanceDynamic`。
- 离开遮挡后恢复原材质。

`APlayerCharacterBase`：

- 创建默认子对象。
- 暴露组件给蓝图读取。

`AYogPlayerCameraManager`：

- 不修改。
- 继续只负责相机位置、偏移、边界和震动。

---

## 运行时流程

```text
TickComponent
  -> UpdateFadeInterpolation(DeltaTime)
  -> 如果 TraceInterval 到期
      -> GetCameraLocation()
      -> GetPlayerTargetLocation()
      -> SweepMultiByChannel()
      -> BuildCurrentOccluderSet()
      -> NewlyHit components: TargetAlpha = MinVisibleAlpha
      -> PreviouslyHit but no longer hit components: TargetAlpha = 1.0
  -> Alpha 回到 1.0 的组件恢复原材质并从缓存移除
```

---

## 核心数据结构

建议内部使用 `TObjectKey<UPrimitiveComponent>` 做 Map key，避免组件销毁后弱引用比较混乱。

```cpp
struct FYogOcclusionFadeTarget
{
	TWeakObjectPtr<UPrimitiveComponent> Component;
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
	float CurrentAlpha = 1.0f;
	float TargetAlpha = 1.0f;
	bool bHitThisTrace = false;
};

TMap<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget> FadeTargets;
```

---

## 过滤规则

命中后按顺序过滤：

1. `Hit.Component` 必须有效。
2. 忽略 Owner，也就是玩家自己。
3. 忽略玩家附属 Actor，例如武器、特效挂件，除非显式带遮挡 Tag。
4. `bOnlyFadeTaggedOccluders == true` 时，Actor 或 Component 必须包含 `CameraOccluder`。
5. 组件必须是可渲染的 `UPrimitiveComponent`，且至少有一个材质槽。
6. 不处理已经 `HiddenInGame` 的组件。

---

## Editor 配置建议

### 遮挡物配置

墙、柱子、树冠、大型装饰物、门框等可能挡住玩家的 Actor 或 Mesh Component 加：

```text
CameraOccluder
```

### 碰撞配置

遮挡物需要能被 `TraceChannel` 命中。默认方案使用：

```text
ECC_Visibility
```

如果项目后续需要更严格控制，可以新建 Project Trace Channel：

```text
CameraOcclusion
```

然后只让可遮挡玩家的物体 Block 这个通道。

### 材质配置

可能遮挡玩家的材质需要支持：

```text
Scalar Parameter: CameraOcclusionAlpha
```

建议用 Dither Fade，不建议用普通 Translucent 直接透明化大面积场景物，因为可能带来排序、深度和性能问题。

---

## 实现任务

### Task 1: 添加组件默认值测试

**Files:**

- Create: `Source/DevKit/Private/Tests/CameraOcclusionFadeComponentTests.cpp`
- Create later: `Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h`

- [ ] **Step 1: 写失败测试**

添加 Automation Test，验证默认参数符合方案：

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Component/YogCameraOcclusionFadeComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYogCameraOcclusionFadeDefaultsTest,
	"DevKit.Camera.OcclusionFade.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYogCameraOcclusionFadeDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UYogCameraOcclusionFadeComponent* Component = NewObject<UYogCameraOcclusionFadeComponent>();
	TestTrue(TEXT("Occlusion fade starts enabled"), Component->bEnableOcclusionFade);
	TestEqual(TEXT("Trace interval defaults to 0.05s"), Component->TraceInterval, 0.05f);
	TestEqual(TEXT("Trace radius defaults to 28"), Component->TraceRadius, 28.0f);
	TestEqual(TEXT("Min visible alpha defaults to 0.15"), Component->MinVisibleAlpha, 0.15f);
	TestEqual(TEXT("Fade parameter name"), Component->FadeScalarParameterName, FName(TEXT("CameraOcclusionAlpha")));
	TestTrue(TEXT("Tagged occluders are required by default"), Component->bOnlyFadeTaggedOccluders);

	return true;
}

#endif
```

- [ ] **Step 2: 运行测试并确认失败**

```powershell
.\Binaries\Win64\UnrealEditor-Cmd.exe DevKit.uproject -ExecCmds="Automation RunTests DevKit.Camera.OcclusionFade.Defaults; Quit" -unattended -nop4 -nosplash -NullRHI
```

Expected: 编译失败或测试失败，因为组件类还不存在。

### Task 2: 创建 `UYogCameraOcclusionFadeComponent`

**Files:**

- Create: `Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h`
- Create: `Source/DevKit/Private/Component/YogCameraOcclusionFadeComponent.cpp`

- [ ] **Step 1: 添加组件声明**

组件需要 `BlueprintSpawnableComponent`，方便后续蓝图手动挂载到其他角色或测试 Actor。

```cpp
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UYogCameraOcclusionFadeComponent : public UActorComponent
```

- [ ] **Step 2: 添加默认参数**

按“默认设计参数”章节添加 `UPROPERTY`。

- [ ] **Step 3: 添加 Tick**

构造函数中启用 Tick：

```cpp
PrimaryComponentTick.bCanEverTick = true;
PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
```

`TG_PostUpdateWork` 的目的是让相机更新后再读取最终相机位置。

- [ ] **Step 4: 运行默认值测试并确认通过**

```powershell
.\Binaries\Win64\UnrealEditor-Cmd.exe DevKit.uproject -ExecCmds="Automation RunTests DevKit.Camera.OcclusionFade.Defaults; Quit" -unattended -nop4 -nosplash -NullRHI
```

Expected: `DevKit.Camera.OcclusionFade.Defaults` PASS。

### Task 3: 实现 Trace 与命中过滤

**Files:**

- Modify: `Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h`
- Modify: `Source/DevKit/Private/Component/YogCameraOcclusionFadeComponent.cpp`
- Modify: `Source/DevKit/Private/Tests/CameraOcclusionFadeComponentTests.cpp`

- [ ] **Step 1: 写过滤测试**

测试 `CameraOccluder` Tag 规则。建议把过滤函数做成小的可测函数：

```cpp
bool IsFadeAllowedForComponent(const UPrimitiveComponent* Component) const;
```

测试重点：

- 没有 Tag 时默认不允许。
- Actor 有 `CameraOccluder` Tag 时允许。
- Component 有 `CameraOccluder` Tag 时允许。

- [ ] **Step 2: 实现过滤函数**

过滤函数只负责判断一个组件是否允许淡化，不做 Trace。

- [ ] **Step 3: 实现 `RunOcclusionTrace()`**

核心逻辑：

```cpp
const FVector Start = CameraManager->GetCameraLocation();
const FVector End = GetOwner()->GetActorLocation() + PlayerTargetOffset;
GetWorld()->SweepMultiByChannel(
	Hits,
	Start,
	End,
	FQuat::Identity,
	TraceChannel,
	FCollisionShape::MakeSphere(TraceRadius),
	QueryParams);
```

`QueryParams` 至少需要：

```cpp
QueryParams.AddIgnoredActor(GetOwner());
QueryParams.bTraceComplex = false;
```

- [ ] **Step 4: 更新当前遮挡集合**

本轮命中的组件设置：

```text
TargetAlpha = MinVisibleAlpha
bHitThisTrace = true
```

旧组件本轮没命中则设置：

```text
TargetAlpha = 1.0
bHitThisTrace = false
```

### Task 4: 实现材质淡入淡出

**Files:**

- Modify: `Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h`
- Modify: `Source/DevKit/Private/Component/YogCameraOcclusionFadeComponent.cpp`

- [ ] **Step 1: 命中组件第一次进入时缓存原材质**

记录每个 material slot 原来的 `UMaterialInterface*`。

- [ ] **Step 2: 创建 Dynamic Material**

对每个材质槽调用：

```cpp
UMaterialInstanceDynamic* DynMat = Component->CreateDynamicMaterialInstance(SlotIndex);
```

然后设置：

```cpp
DynMat->SetScalarParameterValue(FadeScalarParameterName, CurrentAlpha);
```

- [ ] **Step 3: 每帧插值**

```cpp
const float Speed = TargetAlpha < CurrentAlpha ? FadeOutSpeed : FadeInSpeed;
CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, Speed);
```

- [ ] **Step 4: 恢复原材质**

当 `TargetAlpha == 1.0` 且 `CurrentAlpha` 基本回到 `1.0` 时：

```cpp
Component->SetMaterial(SlotIndex, OriginalMaterials[SlotIndex]);
FadeTargets.Remove(ComponentKey);
```

### Task 5: 挂载到玩家

**Files:**

- Modify: `Source/DevKit/Public/Character/PlayerCharacterBase.h`
- Modify: `Source/DevKit/Private/Character/PlayerCharacterBase.cpp`

- [ ] **Step 1: 添加前向声明**

```cpp
class UYogCameraOcclusionFadeComponent;
```

- [ ] **Step 2: 添加组件属性**

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
TObjectPtr<UYogCameraOcclusionFadeComponent> CameraOcclusionFadeComponent;
```

- [ ] **Step 3: 构造函数创建默认子对象**

```cpp
CameraOcclusionFadeComponent = CreateDefaultSubobject<UYogCameraOcclusionFadeComponent>(TEXT("CameraOcclusionFadeComponent"));
```

### Task 6: 手动验证

**Files:**

- No code changes.
- Editor asset setup.

- [ ] **Step 1: 找一个会挡住玩家的墙/柱子/树**

给 Actor 或 Mesh Component 添加 Tag：

```text
CameraOccluder
```

- [ ] **Step 2: 确认材质支持参数**

材质需要响应：

```text
CameraOcclusionAlpha
```

- [ ] **Step 3: PIE 验证**

移动玩家到遮挡物后面，观察：

- 遮挡物淡出到 `MinVisibleAlpha`。
- 玩家重新露出后遮挡物淡入。
- 敌人、掉落物、武器不会误淡出。
- 快速移动相机/玩家时不闪烁。

---

## 验收标准

- 玩家被 `CameraOccluder` 物体遮挡时，遮挡物平滑淡出。
- 遮挡关系解除后，遮挡物平滑淡入并恢复原材质。
- 不修改 `AYogPlayerCameraManager` 的相机偏移逻辑。
- 不会淡出玩家、敌人、掉落物和没有配置 Tag 的普通 Actor。
- 检测频率可调，默认不需要每帧 Trace。
- Automation Test 覆盖组件默认值和 Tag 过滤逻辑。

---

## 后续可选增强

- 支持按 Actor 淡化，而不是按单个 Component 淡化。
- 支持多个玩家或分屏时按本地 PlayerController 区分相机。
- 支持自定义 `CameraOcclusion` Trace Channel。
- 支持材质不兼容时自动使用 `FallbackFadeMaterial`。
- Debug 绘制 Trace 路径、命中点和当前淡化组件列表。

