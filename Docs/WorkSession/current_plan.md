# 开发方案 v3：关卡事件总线 + LootSelection 视觉高亮 + 卡片堆叠修复 + 教程节奏

> 在 v2 基础上整合 Codex 审查（共 11 项问题）+ 用户决策（2026-04-27）
>
> v3 核心变化：**关卡事件总线从 P2 提到 P1**，连带重构现有教程触发位

## 需求描述（5 项 PIE 实测问题）

1. **OnCardFocused/OnSectionFocused 视觉高亮写入 C++** — 不依赖 BP 实现
2. **拾取后三选一卡片堆在一起** — 截图证实 3 张卡视觉重叠
3. **教程出现太生硬** — First Rune 教程必须等 slow-mo + 揭幕**完全结束**后再渐入；ActivationZone 教程默认不出
4. **关卡事件可管理化** — 用户在编辑器里配置"事件 → LevelFlowAsset"映射，替代硬编码教程触发
5. **RuneInfoCard 加 SetSelected** — 通用接口，背包/详情页/LootSelection 复用

## 用户已确认的设计决策（2026-04-27）

| # | 问题 | 决策 |
|---|---|---|
| Q1 | 事件总线范围 | **本次一起做**（不延期） |
| Q2 | 教程触发时机 | 等揭幕**完全结束**后 |
| Q3 | RuneInfoCard SetSelected 复用 | 背包/详情页将来也用 → 做通用 |
| Q4 | CardSize | **支持 DPI 自适应** |
| Q5 | OnCardFocused BP 接口 | 保留（可能已有 BP 实现，不清空） |
| Q6 | SelectionBorder 控件 | **UBorder**（保留交互扩展） |
| Q7 | 渐入 0.25s | OK |

---

## 方案设计

### 模块 1：关卡事件总线（**核心，新增**）

#### 1.1 EGameLifecycleEvent 枚举

新建 `Source/DevKit/Public/GameModes/GameLifecycleTypes.h`：

```cpp
UENUM(BlueprintType)
enum class EGameLifecycleEvent : uint8
{
    LevelClear            UMETA(DisplayName = "关卡完成（敌人全清）"),
    LevelClearRevealed    UMETA(DisplayName = "关卡揭幕完成（slow-mo + 黑屏退场后）"),
    FirstRuneAcquired     UMETA(DisplayName = "首次获得符文（任意来源）"),
    FirstRunePlaced       UMETA(DisplayName = "首次放置符文到背包"),
    HeatPhaseEntered      UMETA(DisplayName = "首次进入热度阶段（Phase>=1）"),
    PlayerDeath           UMETA(DisplayName = "玩家死亡"),
    GameStart             UMETA(DisplayName = "游戏开始（首关 BeginPlay）"),
};
```

#### 1.2 AYogGameMode 暴露映射

```cpp
// 编辑器内可配：事件 → 跑哪个 LevelFlowAsset（空 → 不跑）
UPROPERTY(EditDefaultsOnly, Category = "Lifecycle Events")
TMap<EGameLifecycleEvent, TObjectPtr<class ULevelFlowAsset>> LifecycleEventFlows;

// 一次性事件去重（FirstRune* / HeatPhase* / GameStart 只触发一次）
UPROPERTY(Transient)
TSet<EGameLifecycleEvent> FiredOnceEvents;

UFUNCTION(BlueprintCallable, Category = "Lifecycle Events")
void TriggerLifecycleEvent(EGameLifecycleEvent Event);
```

`TriggerLifecycleEvent` 实现：
- 一次性事件去重（First*/HeatPhase*/GameStart）：若 `FiredOnceEvents.Contains(Event)` 直接 return
- 查 `LifecycleEventFlows.Find(Event)` → 拿到 `ULevelFlowAsset*`
- `NewObject<UFlowComponent>` 挂在 GameMode 上（or AGameStateBase），调 `StartFlow(Asset)` 跑节点图
- 节点图里可放 `LENode_ShowTutorial` `LENode_Delay` `LENode_TimeDilation` 等已有节点
- 用户在 `BP_GameMode_Default` 里配 `LifecycleEventFlows[LevelClearRevealed] = LFA_FirstRuneTutorial`

#### 1.3 重构现有触发位（删除硬编码，全走事件总线）

- **YogGameMode::EnterArrangementPhase**：
  - 删除直接调 `TM->TryFirstRuneTutorial(PC)`
  - 改 `TriggerLifecycleEvent(EGameLifecycleEvent::LevelClear)`（关卡完成时立即触发，可让用户配 LFA 做"金币结算 UI"等）
- **YogHUD::TickLevelEndEffect**：
  - 揭幕完成时（`RevealProgress >= 1.f` 且首次到达）广播 `FSimpleMulticastDelegate OnLevelEndEffectFinished`
  - **HUD 不调 GameMode 业务逻辑**（修复 Codex 指出的职责反转）
- **YogGameMode::BeginPlay**：
  - 监听 `HUD->OnLevelEndEffectFinished.AddUObject(this, &::HandleLevelEndEffectFinished)`
  - `HandleLevelEndEffectFinished` 内调 `TriggerLifecycleEvent(LevelClearRevealed)`
- **BackpackGridComponent::HandleHeatTutorial**：
  - 删除直接调 `TM->TryHeatPhaseTutorial(PC)`
  - 改 `TriggerLifecycleEvent(HeatPhaseEntered)`
  - 用户没在 GameMode 配映射 → 啥都不发生（解决"ActivationZone 教程不出"诉求）

#### 1.4 LFA_FirstRuneTutorial 用户在编辑器创建（用户操作，不在本次 C++ 改动）

节点图：`In → ShowTutorial(EventID="tutorial_first_rune") → Out`

配到 `BP_GameMode_Default.LifecycleEventFlows[LevelClearRevealed]`。

→ 揭幕动画走完才弹教程，且玩家可随时清空映射关掉教程。

### 模块 2：卡片堆叠修复（C++ 兜底 + WBP 自适应）

#### 2.1 卡片尺寸支持自适应（Q4 决策）

不写死 320×420，改用项目级 DPI Scale + SizeBox 的 Min/Max 范围：

```cpp
// LootSelectionWidget.h
UPROPERTY(EditAnywhere, Category = "Loot|Layout")
FVector2D MinCardSize = FVector2D(280.f, 380.f);

UPROPERTY(EditAnywhere, Category = "Loot|Layout")
FVector2D MaxCardSize = FVector2D(420.f, 560.f);

UPROPERTY(EditAnywhere, Category = "Loot|Layout", meta = (ClampMin = "0"))
float CardHorizontalPadding = 16.f;  // 单边 padding，左右各 16
```

`RebuildCards`：
- 套 `USizeBox`，设 `MinDesiredWidth/Height` + `MaxDesiredWidth/Height`（不用 WidthOverride，让卡片在 Min/Max 范围内根据内容自适应）
- HSlot Padding = `FMargin(CardHorizontalPadding, 0)`
- DPI 缩放靠项目 Project Settings → User Interface → DPI Curve（不在 widget 内处理）

#### 2.2 WBP_RuneInfoCard 文档要求（用户操作）

- 根 SizeBox（DesiredOnScreenSize 或与 C++ 套的 SizeBox 配合）
- 内部 CanvasPanel 子节点全部用 Anchor + Position 而非纯 Auto 布局，避免内容塌缩
- 详细参数见 `Docs/Systems/UI/WBP_RuneInfoCard_Layout.md`（本次新建）

### 模块 3：视觉高亮 C++ 化（RuneInfoCard.SetSelected）

#### 3.1 RuneInfoCard 改动

```cpp
// .h
UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
TObjectPtr<class UBorder> SelectionBorder;  // UBorder 类型（Q6 决策）

UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
void SetSelected(bool bInSelected);

UFUNCTION(BlueprintPure, Category = "RuneInfoCard")
bool IsSelected() const { return bSelected; }

// .h private
bool  bSelected = false;
float CurrentRenderScale = 1.0f;
UPROPERTY(EditAnywhere, Category = "Visual")
float SelectedRenderScale = 1.06f;
UPROPERTY(EditAnywhere, Category = "Visual")
float ScaleInterpSpeed = 12.f;
```

```cpp
// .cpp NativeConstruct
SetRenderTransformPivot(FVector2D(0.5f, 0.5f));  // ⚠️ Codex 指出：必须设中心 pivot

// SetSelected
bSelected = bInSelected;
if (SelectionBorder) SelectionBorder->SetVisibility(bInSelected ? Visible : Hidden);

// NativeTick
const float Target = bSelected ? SelectedRenderScale : 1.0f;
CurrentRenderScale = FMath::FInterpTo(CurrentRenderScale, Target, InDeltaTime, ScaleInterpSpeed);
SetRenderScale(FVector2D(CurrentRenderScale));
```

**关键**：
- `ShowRune` **不调** `SetSelected(false)`（Codex P7：避免与 FocusCard 状态竞争）
- 状态由调用方（LootSelection/背包/详情页）统一管理
- 默认 `bSelected = false` → 背包不调用就不会高亮

#### 3.2 LootSelectionWidget 调用规范

`RebuildCards` 末尾遍历所有卡 `SetSelected(false)`（清场），然后 `FocusCard(0)` 内只对当前 idx `SetSelected(true)`。

`SetSection(Buttons)` 时遍历所有卡 `SetSelected(false)`（按钮段不让卡片继续高亮）。

#### 3.3 BtnSkip / BtnBackpackPreview 高亮

放弃改 WidgetStyle.Normal.TintColor（Codex P4：会污染 Hovered/Pressed 状态）。

替代方案：在 WBP_LootSelection 的两个按钮**外层各套一个 UBorder**（命名 `SkipHighlightBorder` / `PreviewHighlightBorder`，BindWidgetOptional），C++ 调 `SetVisibility` 切换显隐 + `SetRenderScale(1.04)`。

### 模块 4：教程节奏改造（依赖模块 1 事件总线）

- TutorialPopupWidget 加渐入：
  - `Source/DevKit/Public/UI/GameDialogWidget.h`：加 `bool bIsFadingIn = false; float FadeInElapsedReal = 0.f; double FadeInStartRealTime = 0.0;`
  - `Source/DevKit/Private/UI/GameDialogWidget.cpp` `ShowPopup` 末尾：`SetRenderOpacity(0.f); FadeInStartRealTime = GetWorld()->GetRealTimeSeconds(); bIsFadingIn = true;`
  - `NativeTick` 用 `RealTime - FadeInStartRealTime` 计算 alpha（**Codex P2 修正**：不用 DeltaTime，避免 dilation 影响）
  - 0.25s 渐入到 1.0
- 触发链：`HUD::OnLevelEndEffectFinished` → `GameMode::HandleLevelEndEffectFinished` → `TriggerLifecycleEvent(LevelClearRevealed)` → 跑用户配的 `LFA_FirstRuneTutorial` → `LENode_ShowTutorial` → `TutorialManager::ShowByEventID("tutorial_first_rune")` → 弹窗渐入

---

## 涉及文件

### C++ 改动

| 文件 | 操作 | 说明 |
|---|---|---|
| `Source/DevKit/Public/GameModes/GameLifecycleTypes.h` | **新建** | EGameLifecycleEvent 枚举 |
| `Source/DevKit/Public/GameModes/YogGameMode.h` | 编辑 | LifecycleEventFlows / TriggerLifecycleEvent / OnLevelEndEffectFinished 监听 |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | 编辑 | TriggerLifecycleEvent 实现 + 重构 EnterArrangementPhase + BeginPlay 监听 |
| `Source/DevKit/Public/UI/YogHUD.h` | 编辑 | OnLevelEndEffectFinished 委托 + bRevealCompleteFired |
| `Source/DevKit/Private/UI/YogHUD.cpp` | 编辑 | TickLevelEndEffect 揭幕完成广播；TriggerLevelEndEffect 重置标记 |
| `Source/DevKit/Private/Component/BackpackGridComponent.cpp` | 编辑 | HandleHeatTutorial 改走事件总线 |
| `Source/DevKit/Public/UI/RuneInfoCardWidget.h` | 编辑 | SelectionBorder（UBorder）+ SetSelected + 缩放字段 |
| `Source/DevKit/Private/UI/RuneInfoCardWidget.cpp` | 编辑 | SetSelected + Tick 插值 + Pivot 中心 |
| `Source/DevKit/Public/UI/LootSelectionWidget.h` | 编辑 | MinCardSize/MaxCardSize/CardHorizontalPadding + SkipHighlightBorder/PreviewHighlightBorder |
| `Source/DevKit/Private/UI/LootSelectionWidget.cpp` | 编辑 | RebuildCards 套 SizeBox + FocusCard/FocusButton 调 SetSelected/SetVisibility |
| `Source/DevKit/Public/UI/GameDialogWidget.h` | 编辑 | bIsFadingIn / FadeInStartRealTime 字段 |
| `Source/DevKit/Private/UI/GameDialogWidget.cpp` | 编辑 | ShowPopup 启动渐入 + Tick 插值（用 RealTime） |

**新建 1 文件 + 编辑 11 文件**（C++ 改动面比 v2 大约 60% 增量，主要是事件总线本体）

### 文档改动

| 文件 | 操作 | 说明 |
|---|---|---|
| `Docs/Systems/Lifecycle/EventBus_Design.md` | 新建 | 事件总线设计 + 7 个事件含义 + 用户配置流程 |
| `Docs/Systems/UI/WBP_LootSelection_Layout.md` | 编辑 | 加 SkipHighlightBorder/PreviewHighlightBorder + Event Graph 段更新 |
| `Docs/Systems/UI/WBP_RuneInfoCard_Layout.md` | 新建 | SizeBox 根 + SelectionBorder 配置 |

### 用户编辑器侧（非本次 C++）

| 资产 | 改动 |
|---|---|
| `Content/UI/Playtest_UI/Runes/WBP_RuneInfoCard.uasset` | 加 SizeBox 根 + SelectionBorder（UBorder） |
| `Content/UI/Playtest_UI/WBP_LootSelection.uasset` | 按文档加 SkipHighlightBorder/PreviewHighlightBorder |
| `Content/Docs/Map/LevelEvent/LFA_FirstRuneTutorial.uasset` | **新建**：节点图 = ShowTutorial(EventID="tutorial_first_rune") |
| `BP_GameMode_Default` Class Defaults | 配 `LifecycleEventFlows[LevelClearRevealed] = LFA_FirstRuneTutorial` |

---

## 落地分组（C++ 独立可验证 vs 依赖 WBP）

**Phase 1：纯 C++（编译即可验证）**
- 模块 1 事件总线本体（枚举 + GameMode 接口 + HUD 委托 + 重构触发位）
- 模块 3 SetSelected 接口（不调 BP 也能工作，缩放即生效）
- 模块 4 渐入逻辑

**Phase 2：依赖 WBP 改动（编译完后用户在编辑器配）**
- WBP_RuneInfoCard 加 SelectionBorder → 选中时才有视觉边框
- WBP_LootSelection 加 SkipHighlightBorder/PreviewHighlightBorder
- 创建 LFA_FirstRuneTutorial + 配到 BP_GameMode_Default

→ Phase 1 完成后即使 WBP 没改，三选一也能正常横排显示（仅缺缩放/边框视觉），保证主流程可用。

---

## 潜在风险

1. **事件总线一次做完工作量翻倍** — Phase 1 编译后无法立即看到教程出现（需 Phase 2 用户配 LFA），要明确告知
2. **现有 LFA_RuneInfoGuid / LFA_DashTutorial / LFA_WeaponInfoGuid 是否影响** — 需检查它们是否依赖被删除的硬编码触发位（应该不依赖，它们走 LevelEventTrigger 触发）
3. **BackpackGridComponent 取 GameMode 单机 OK** — 本项目纯单机 Roguelite，GameMode 客户端可访问，无服务端/客户端分裂问题
4. **MinCardSize/MaxCardSize 自适应** — 卡片在 Min/Max 范围内根据 desired size 浮动，需要 PIE 测 1080p / 1440p / 4K 三档确认无视觉跳动
5. **SetRenderTransformPivot 中心化** — RuneInfoCard 现在已有淡入动画用 SetRenderOpacity，加 SetRenderScale 不冲突；但要确认 NativeConstruct 时机比首次 Tick 早
6. **OnLevelEndEffectFinished 单一事件 vs 多次关卡** — 跨关卡时 HUD 实例可能复用，bRevealCompleteFired 必须在 TriggerLevelEndEffect 时重置为 false
7. **一次性事件 FiredOnceEvents 跨关卡** — `Transient` 修饰符，PIE 重启会重置；存档读取时不需要恢复（玩家重玩同一档不应再触发首符文教程，需另存档逻辑 — **暂不处理，留 TODO**）
8. **保留 OnCardFocused BP 接口（Q5）** — 文档加注释提醒"⚠️ C++ 已自管缩放/边框，BP 仅做音效/粒子，不要再做视觉重复"

---

## 验证流程

### Phase 1（编译后）
1. 编译通过，无 unresolved external
2. PIE → 杀完敌人 → 关卡结束揭幕 → **不应有教程弹窗**（用户没配 LFA）
3. 拾取奖励 → LootSelection 弹出 → **3 张卡横排**（即使 WBP 没改 SelectionBorder 也应横排，仅缺高亮视觉）
4. 键盘 LR / 手柄 DPad LR → 卡片缩放有变化（C++ Tick 插值）
5. Output Log 应有 `[LifecycleEvents] TriggerLifecycleEvent(LevelClear)` 等日志

### Phase 2（用户配 WBP + LFA 后）
6. 创建 LFA_FirstRuneTutorial（节点：ShowTutorial EventID="tutorial_first_rune"）
7. 在 BP_GameMode_Default 配 LifecycleEventFlows[LevelClearRevealed] = LFA_FirstRuneTutorial
8. PIE → 关卡结束 → **slow-mo + 黑屏揭幕完全结束后** → 教程弹窗渐入 0.25s
9. WBP_RuneInfoCard 加 SelectionBorder → 焦点切换有边框 + 缩放
10. WBP_LootSelection 加 SkipHighlightBorder → DPad Down 段切换时按钮高亮

---

## 待确认问题

无。所有方向已在 Q1-Q7 中确认，可以开工。
